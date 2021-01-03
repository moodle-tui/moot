#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "json.h"
#define CURL_MAX_PARALLEL 20

size_t write_memblock_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memblock *mem = (struct Memblock *)userp;

    char *ptr = (char *)md_realloc(mem->memory, mem->size + realsize + 1, mem->error);
    if (ptr == NULL)
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void *md_malloc(size_t size, MDError *error) {
    return md_realloc(NULL, size, error);
}

void *md_realloc(void *data, size_t size, MDError *error) {
    data = realloc(data, size);
    if (!data && size)
        *error = MD_ERR_ALLOC;
    return data;
}

char *clone_str(cchar *s, MDError *error) {
    char *str = (char *)md_malloc(strlen(s) + 1, error);
    if (str) {
        strcpy(str, s);
    }
    return str;
}

typedef size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata);

char *url_escape(cchar *url, MDError *error) {
    char *escaped = curl_escape(url, 0);
    if (!escaped)
        *error = MD_ERR_ALLOC;
    return escaped;
}

void str_replace(char *str, cchar *needle, cchar *replacement) {
    size_t matched = 0, offset = 0;
    size_t lenNeedle = strlen(needle), lenReplacement = strlen(replacement), len = strlen(str);
    if (lenReplacement > lenNeedle)
        return;
    for (size_t i = 0; i <= len; ++i) {
        str[i - offset] = str[i];
        matched = str[i - offset] == needle[matched] ? matched + 1 : 0;
        if (matched == lenNeedle) {
            memcpy(str + i - offset - matched + 1, replacement, lenReplacement);
            offset += lenNeedle - lenReplacement;
            matched = 0;
        }
    }
}

CURL *createCurl(cchar *url, void *data, WriteCallback callback, MDError *error) {
    CURL *handle = curl_easy_init();
    if (handle) {
        curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, data);
        curl_easy_setopt(handle, CURLOPT_USERAGENT, "moodle-tui/moot");
    } else {
        *error = MD_ERR_CURL_FAIL;
    }
    return handle;
}

size_t write_stream_callback(void *contents, size_t size, size_t nmemb, void *stream) {
    return fwrite(contents, size, nmemb, (FILE *)stream);
}

void http_get_request_to_file(char *url, FILE *stream, MDError *error) {
    CURL *handle = createCurl(url, (void *)stream, write_stream_callback, error);
    if (!handle)
        return;

    CURLcode res = curl_easy_perform(handle);
    if (res != CURLE_OK) {
        md_error_set_message(curl_easy_strerror(res));
        *error = MD_ERR_HTTP_REQUEST_FAIL;
    }

    curl_easy_cleanup(handle);
}

char *http_get_request(char *url, MDError *error) {
    ENSURE_EMPTY_ERROR(error);
    CURL *handle;
    CURLcode res;

    struct Memblock chunk;
    chunk.size = 0;
    chunk.memory = (char *)md_malloc(1, error);
    if (!*error) {
        handle = createCurl(url, (void *)&chunk, write_memblock_callback, error);
        if (!*error) {
            res = curl_easy_perform(handle);
            if (res != CURLE_OK) {
                md_error_set_message(curl_easy_strerror(res));
                *error = MD_ERR_HTTP_REQUEST_FAIL;
                free(chunk.memory);
                chunk.memory = NULL;
            }
            curl_easy_cleanup(handle);
        }
    }
    return chunk.memory;
}

char **http_get_multi_request(char *urls[], unsigned int size, MDError *error) {
    ENSURE_EMPTY_ERROR(error);
    CURLMsg *msg;
    unsigned int transfers = 0;
    int msgsLeft = -1;
    int stillAlive = 1;

    CURLM *multi = curl_multi_init();
    if (!multi) {
        *error = MD_ERR_CURL_FAIL;
        return NULL;
    }
    curl_multi_setopt(multi, CURLMOPT_MAXCONNECTS, (long)CURL_MAX_PARALLEL);

    struct Memblock chunks[size];
    for (int i = 0; i < size; ++i) {
        chunks[i].memory = (char *)md_malloc(1, error);
        chunks[i].size = 0;
        chunks[i].memory[0] = 0;
    }
    CURL *handles[size];
    for (int i = 0; i < size; ++i) {
        handles[i] = createCurl(urls[i], (void *)&chunks[i], write_memblock_callback, error);
    }
    if (!*error) {
        for (transfers = 0; transfers < CURL_MAX_PARALLEL && transfers < size; transfers++)
            curl_multi_add_handle(multi, handles[transfers]);

        do {
            curl_multi_perform(multi, &stillAlive);

            while ((msg = curl_multi_info_read(multi, &msgsLeft))) {
                // TODO curl error
                if (msg->msg != CURLMSG_DONE) {
                    md_error_set_message(curl_easy_strerror(msg->data.result));
                    *error = MD_ERR_HTTP_REQUEST_FAIL;
                }
                curl_multi_remove_handle(multi, msg->easy_handle);
                curl_easy_cleanup(msg->easy_handle);

                if (transfers < size)
                    curl_multi_add_handle(multi, handles[transfers]);
            }
            if (stillAlive)
                curl_multi_wait(multi, NULL, 0, 500, NULL);

        } while (stillAlive || (transfers < size));

        curl_multi_cleanup(multi);
    }

    char **result = md_malloc(size * sizeof(char *), error);
    if (!*error) {
        for (int i = 0; i < size; ++i) {
            result[i] = chunks[i].memory;
        }
    } else {
        for (int i = 0; i < size; ++i) {
            free(chunks[i].memory);
        }
        free(result);
        result = NULL;
    }
    return result;
}

char *http_post_file(cchar *url, cchar *filename, cchar *name, MDError *error) {
    ENSURE_EMPTY_ERROR(error);
    struct Memblock chunk;
    chunk.size = 0;
    chunk.memory = (char *)md_malloc(1, error);
    CURL *handle = createCurl(url, &chunk, write_memblock_callback, error);
    if (!*error) {
        curl_mime *mime;
        curl_mimepart *part;

        mime = curl_mime_init(handle);
        part = curl_mime_addpart(mime);

        // Add the file;
        curl_mime_name(part, name);
        CURLcode ok = curl_mime_filedata(part, filename);
        if (ok == CURLE_OK) {
            curl_easy_setopt(handle, CURLOPT_MIMEPOST, mime);

            CURLcode response = curl_easy_perform(handle);
            if (response != CURLE_OK) {
                md_error_set_message(curl_easy_strerror(response));
                *error = MD_ERR_HTTP_REQUEST_FAIL;
            }
        } else {
            *error = MD_ERR_FILE_OPERATION;
            md_error_set_message(filename);
        }
        curl_easy_cleanup(handle);
        curl_mime_free(mime);
    }
    if (*error) {
        free(chunk.memory);
        chunk.memory = NULL;
    }
    return chunk.memory;
}

json_value *json_get_property_silent(json_value *json, cchar *key) {
    if (json->type != json_object)
        return NULL;
    for (int i = 0; i < json->u.object.length; ++i) {
        if (strcmp(json->u.object.values[i].name, key) == 0) {
            return json->u.object.values[i].value;
        }
    }
    return NULL;
}

json_value *json_get_property(json_value *json, cchar *key, json_type type, MDError *error) {
    json_value *value = json_get_property_silent(json, key);
    if (value) {
        if (value->type != type) {
            *error = MD_ERR_INVALID_JSON_VALUE;
            value = NULL;
            md_error_set_message(key);
        }
    } else {
        *error = MD_ERR_MISSING_JSON_KEY;
        md_error_set_message(key);
    }
    return value;
}

long json_get_integer(json_value *json, cchar *key, MDError *error) {
    json_value *value = json_get_property(json, key, json_integer, error);
    if (value)
        return value->u.integer;
    return 0;
}

int json_get_bool(json_value *json, cchar *key, MDError *error) {
    json_value *value = json_get_property(json, key, json_boolean, error);
    if (value)
        return value->u.boolean;
    return 0;
}

char *json_get_string(json_value *json, cchar *key, MDError *error) {
    MDError prev = *error;
    json_value *value = json_get_property(json, key, json_string, error);
    if (*error == MD_ERR_INVALID_JSON_VALUE) {
        value = json_get_property(json, key, json_null, error);
        if (value) {
            *error = prev;
            return NULL;
        }
    }
    if (value)
        return clone_str(value->u.string.ptr, error);
    return NULL;
}

cchar *json_get_string_no_alloc(json_value *json, cchar *key, MDError *error) {
    json_value *value = json_get_property(json, key, json_string, error);
    if (value)
        return value->u.string.ptr;
    return NULL;
}

json_value *json_get_array(json_value *json, cchar *key, MDError *error) {
    return json_get_property(json, key, json_array, error);
}

json_value *json_get_object(json_value *json, cchar *key, MDError *error) {
    return json_get_property(json, key, json_object, error);
}