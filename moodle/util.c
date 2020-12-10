#include "util.h"
#include "error.h"
#include "json.h"
#include <assert.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_PARALLEL 20

void md_array_init_new(MDArray *array, size_t size, int length, MDInitFunc callback, MDError *error) {
    array->len = length;
    if (size && length) {
        array->_data = calloc(length, size);
        if (!array->_data) {
            *error = MD_ERR_ALLOC;
        } else if (callback) {
            for (int i = 0; i < length; ++i) {
                callback(array->_data + i * size, error);
            }
        }
    } else {
        array->_data = NULL;
    }
}

void md_array_cleanup(MDArray *array, size_t size, MDCleanupFunc callback) {
    if (callback) {
        for (int i = 0; i < array->len; ++i)
            callback(array->_data + i * size);
    }
    free(array->_data);
    array->len = 0;
    array->_data = NULL;
}

// struct to temporarily hold data while performing http request.
struct Memblock;

// CURL callback to write data to Memblock;
static size_t write_memblock_callback(void *contents, size_t size, size_t nmemb, void *userp);

struct Memblock {
    char *memory;
    size_t size;
};

json_value *json_get_by_key(json_value *json, const char *key) {
    if (json->type != json_object)
        return NULL;
    for (int i = 0; i < json->u.object.length; ++i) {
        if (strcmp(json->u.object.values[i].name, key) == 0) {
            return json->u.object.values[i].value;
        }
    }
    return NULL;
}

// CURL callback to write data to Memblock;
static size_t write_memblock_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memblock *mem = (struct Memblock *)userp;

    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *clone_str(const char *s) {
    char *str = (char *)malloc(strlen(s) + 1);
    if (str) {
        strcpy(str, s);
    }

    return str;
}

char *cloneStrErr(const char *s, MDError *error) {
    char *str = clone_str(s);
    if (!str)
        *error = MD_ERR_ALLOC;
    return str;
}

typedef size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata);

CURL *createCurl(const char *url, void *data, WriteCallback callback) {
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "moodle-tui/moot");
    return handle;
}

static size_t write_stream_callback(void *contents, size_t size, size_t nmemb, void *stream) {
    return fwrite(contents, size, nmemb, (FILE *)stream);
}

void http_request_to_file(char *url, FILE *stream, MDError *error) {
    CURL *handle = createCurl(url, (void *)stream, write_stream_callback);
    CURLcode res = curl_easy_perform(handle);

    if (res != CURLE_OK) {
        md_set_error_message(curl_easy_strerror(res));
        *error = MD_ERR_HTTP_REQUEST_FAIL;
    }

    curl_easy_cleanup(handle);
}

char *httpRequest(char *url, MDError *error) {
    CURL *curl_handle;
    CURLcode res;

    struct Memblock chunk;
    chunk.memory = (char *)malloc(1);
    if (!chunk.memory) {
        *error = MD_ERR_ALLOC;
        return NULL;
    }
    chunk.size = 0;

    curl_handle = createCurl(url, (void *)&chunk, write_memblock_callback);
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        md_set_error_message(curl_easy_strerror(res));
        *error = MD_ERR_HTTP_REQUEST_FAIL;
        free(chunk.memory);
        chunk.memory = NULL;
    }
    curl_easy_cleanup(curl_handle);
    return chunk.memory;
}

char **httpMultiRequest(char *urls[], unsigned int size, MDError *error) {
    CURLM *cm;
    CURLMsg *msg;
    unsigned int transfers = 0;
    int msgsLeft = -1;
    int stillAlive = 1;
    
    cm = curl_multi_init();
    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

    // TODO
    struct Memblock chunks[size];
    for (int i = 0; i < size; ++i) {
        chunks[i].size = 0;
        chunks[i].memory = (char *)malloc(1);
        if (!chunks[i].memory) {
            *error = MD_ERR_ALLOC;
            for (int j = 0; j < i - 1; ++j)
                free(chunks[i].memory);
            return NULL;
        }
        chunks[i].memory[0] = 0;
    }
    for (transfers = 0; transfers < MAX_PARALLEL && transfers < size; transfers++)
        curl_multi_add_handle(cm, createCurl(urls[transfers], (void *)&chunks[transfers], write_memblock_callback));

    do {
        curl_multi_perform(cm, &stillAlive);

        while ((msg = curl_multi_info_read(cm, &msgsLeft))) {
            // TODO curl error
            if (msg->msg = CURLMSG_DONE)
                curl_easy_cleanup(msg->easy_handle);
            else {
                md_set_error_message(curl_easy_strerror(msg->data.result));
                *error = MD_ERR_HTTP_REQUEST_FAIL;
            }

            if (transfers < size)
                curl_multi_add_handle(cm,
                                      createCurl(urls[transfers], (void *)&chunks[transfers], write_memblock_callback));
        }
        if (stillAlive)
            curl_multi_wait(cm, NULL, 0, 500, NULL);

    } while (stillAlive || (transfers < size));

    curl_multi_cleanup(cm);
    char **result = malloc(size * sizeof(char *));
    if (result) {
        for (int i = 0; i < size; ++i)
            result[i] = chunks[i].memory;
    } else {
        for (int i = 0; i < size; ++i) {
            free(chunks[i].memory);
        }
        *error = MD_ERR_ALLOC;
    }
    return result;
}

char *httpPostFile(const char *url, const char *filename, const char *name, MDError *error) {
    struct Memblock chunk;
    chunk.size = 0;
    chunk.memory = (char *)malloc(1);
    if (!chunk.memory) {
        *error = MD_ERR_ALLOC;
        return NULL;
    }
    int fail = 0;
    CURL *handle = createCurl(url, &chunk.memory, write_memblock_callback);

    if (handle) {
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
                md_set_error_message(curl_easy_strerror(response));
                *error = MD_ERR_HTTP_REQUEST_FAIL;
                fail = 1;
            }
        } else {
            *error = MD_ERR_FILE_OPERATION;
            md_set_error_message(filename);
            fail = 1;
        }
        curl_easy_cleanup(handle);
        curl_mime_free(mime);

    } else {
        *error = MD_ERR_CURL_FAIL;
        fail = 1;
    }

    if (fail) {
        free(chunk.memory);
        chunk.memory = NULL;
    }

    return chunk.memory;
}

json_value *getJsonProperty(json_value *json, const char *key, json_type type, MDError *error) {
    json_value *value = json_get_by_key(json, key);
    if (value) {
        if (value->type != type) {
            *error = MD_ERR_INVALID_JSON_VALUE;
            value = NULL;
        }
    } else {
        *error = MD_ERR_MISSING_JSON_KEY;
        md_set_error_message(key);
    }
    return value;
}

long jsonGetInteger(json_value *json, const char *key, MDError *error) {
    json_value *value = getJsonProperty(json, key, json_integer, error);
    if (value)
        return value->u.integer;
    return 0;
}

int jsonGetBool(json_value *json, const char *key, MDError *error) {
    json_value *value = getJsonProperty(json, key, json_boolean, error);
    if (value)
        return value->u.boolean;
    return 0;
}

char *jsonGetString(json_value *json, const char *key, MDError *error) {
    MDError prev = *error;
    json_value *value = getJsonProperty(json, key, json_string, error);
    if (*error == MD_ERR_INVALID_JSON_VALUE) {
        value = getJsonProperty(json, key, json_null, error);
        if (value) {
            *error = prev;
            return NULL;
        }
    }
    if (value)
        return cloneStrErr(value->u.string.ptr, error);
    return NULL;
}

const char *jsonGetStringNoAlloc(json_value *json, const char *key, MDError *error) {
    json_value *value = getJsonProperty(json, key, json_string, error);
    if (value)
        return value->u.string.ptr;
    return NULL;
}

json_value *jsonGetArray(json_value *json, const char *key, MDError *error) {
    return getJsonProperty(json, key, json_array, error);
}

// a - json_array, o - json_object, i/d - int, l - long, s - char*
MDError assingJsonValues(json_value *json, const char *format, ...) {
    if (json->type != json_object)
        return MD_ERR_INVALID_JSON_VALUE;
    va_list args;
    va_start(args, format);
    for (int i = 0; format[i]; ++i) {
        const char *name = va_arg(args, const char *);
        json_value *val = json_get_by_key(json, name);
        if (!val) {
            md_set_error_message(name);
            return MD_ERR_MISSING_JSON_KEY;
        }

        switch (format[i]) {
        case 'd':
        case 'i':
            if (val->type == json_integer) {
                *va_arg(args, int *) = val->u.integer;
                break;
            } else
                return MD_ERR_INVALID_JSON_VALUE;

        case 'l':
            printf("{%d}", val->type);
            if (val->type == json_integer) {
                *va_arg(args, long *) = val->u.integer;
                break;
            } else
                return MD_ERR_INVALID_JSON_VALUE;

        case 's':
            if (val->type == json_string) {
                char **str = va_arg(args, char **);
                *str = clone_str(val->u.string.ptr);
                if (!*str)
                    return MD_ERR_ALLOC;
                break;
            } else
                return MD_ERR_INVALID_JSON_VALUE;

        case 'a':
            if (val->type == json_array) {
                *va_arg(args, json_value **) = val;
                break;
            } else
                return MD_ERR_INVALID_JSON_VALUE;
        default:
            assert(0);
        }
    }
    va_end(args);
    return MD_ERR_NONE;
}