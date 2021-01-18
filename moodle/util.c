#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "json.h"
#define CURL_MAX_PARALLEL 20
#define FREAD_CHUNK_SIZE 4096

void md_array_append(MDArray *array, const void *ptr, size_t size, MDError *error) {
    ++array->len;
    array->_data = md_realloc(array->_data, array->len * size, error);
    if (array->_data) {
        memcpy((char *)array->_data + (array->len - 1) * size, ptr, size);
    }
}

void md_array_free(MDArray *array) {
    if (array) {
        free(array->_data);
        array->len = 0;
    }
}

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

CURL *create_curl(cchar *url, void *data, WriteCallback callback, MDError *error) {
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
    CURL *handle = create_curl(url, (void *)stream, write_stream_callback, error);
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
        handle = create_curl(url, (void *)&chunk, write_memblock_callback, error);
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
        handles[i] = create_curl(urls[i], (void *)&chunks[i], write_memblock_callback, error);
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
    CURL *handle = create_curl(url, &chunk, write_memblock_callback, error);
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

char *fread_string(FILE *file, MDError *error) {
    char buffer[FREAD_CHUNK_SIZE];
    size_t bufferLength = 0, outputSize = 0;
    char *output = NULL;
    int c = 1;

    while (!*error && c != '\0') {
        c = fgetc(file);
        if (c == EOF || ferror(file)) {
            *error = MD_ERR_FILE_OPERATION;
        } else {
            if (bufferLength == FREAD_CHUNK_SIZE || c == '\0') {
                output = md_realloc(output, outputSize + bufferLength + 1, error);
                if (output) {
                    memcpy(output + outputSize, buffer, bufferLength);
                    output[outputSize + bufferLength] = (char)c;
                    outputSize += bufferLength + 1;
                    bufferLength = 0;
                }
            } else {
                buffer[bufferLength++] = (char)c;
            }
        }
    }

    if (*error) {
        free(output);
        output = NULL;
    }

    return output;
}

void merge(char *left, char *right, char *end, char *buffer, size_t size, compFunc comp) {
    for (char *l = left, *r = right; l < right || r < end; buffer += size) {
        char **value = r == end || (l < right && comp(l, r) < 0) ? &l : &r;
        memcpy(buffer, *value, size);
        *value += size;
    }
    memcpy(left, buffer - (end - left), end - left);
}

void sort(void *array, size_t count, size_t size, compFunc comp) {
    // Switch to char for easier pointer arithmetics.
    char *arr = array, *end = arr + count * size, buffer[end - arr];

    for (size_t unit = 1; unit < count; unit *= 2) {
        for (size_t i = 0; i + unit < count; i += unit * 2) {
            char *rightEnd = arr + (i + 2 * unit) * size;
            if (rightEnd > end) {
                rightEnd = end;
            }
            merge(arr + i * size, arr + (i + unit) * size, rightEnd, buffer, size, comp);
        }
    }
}

Json *md_parse_json(cchar *data, MDError *error) {
    Json *json = md_malloc(sizeof(Json), error);
    if (json) {
        if (json_parse(json, data)) {
            *error = MD_ERR_INVALID_JSON;
            json = NULL;
        }
    }
    return json;
}

void md_cleanup_json(Json *json) {
    if (json) {
        json_cleanup(json);
        free(json);
    }
}

Json *json_get_property_silent(Json *json, cchar *key) {
    if (json->type != JSON_OBJECT)
        return NULL;
    for (int i = 0; i < json->object.len; ++i) {
        if (strcmp(json->object.entries[i].key.str, key) == 0) {
            return &json->object.entries[i].value;
        }
    }
    return NULL;
}

Json *json_get_property(Json *json, cchar *key, JsonType type, MDError *error) {
    Json *value = json_get_property_silent(json, key);
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

long json_get_integer(Json *json, cchar *key, MDError *error) {
    Json *value = json_get_property(json, key, JSON_NUMBER, error);
    if (value)
        return value->number;
    return 0;
}

int json_get_bool(Json *json, cchar *key, MDError *error) {
    Json *value = json_get_property(json, key, JSON_BOOLEAN, error);
    if (value)
        return value->boolean;
    return 0;
}

char *json_get_string(Json *json, cchar *key, MDError *error) {
    MDError prev = *error;
    Json *value = json_get_property(json, key, JSON_STRING, error);
    if (*error == MD_ERR_INVALID_JSON_VALUE) {
        value = json_get_property(json, key, JSON_NULL, error);
        if (value) {
            *error = prev;
            return NULL;
        }
    }
    if (value)
        return clone_str(value->string.str, error);
    return NULL;
}

cchar *json_get_string_no_alloc(Json *json, cchar *key, MDError *error) {
    Json *value = json_get_property(json, key, JSON_STRING, error);
    if (value)
        return value->string.str;
    return NULL;
}

Json *json_get_array(Json *json, cchar *key, MDError *error) {
    return json_get_property(json, key, JSON_ARRAY, error);
}

Json *json_get_object(Json *json, cchar *key, MDError *error) {
    return json_get_property(json, key, JSON_OBJECT, error);
}