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

struct MemoryStruct {
    char *memory;
    size_t size;
};

json_value *get_by_key(json_value *json, const char *key) {
    if (json->type != json_object)
        return NULL;
    for (int i = 0; i < json->u.object.length; ++i) {
        if (strcmp(json->u.object.values[i].name, key) == 0) {
            return json->u.object.values[i].value;
        }
    }
    return NULL;
}

static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

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

char *cloneStr(const char *s) {
    char *str = (char *)malloc(strlen(s) + 1);
    if (str) {
        strcpy(str, s);
    }
    
    return str;
}

CURL *createCurl(const char *url, void *data) {
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    return handle;
}

char *httpRequest(char *url, ErrorCode *error) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);
    if (!chunk.memory) {
        *error = ERR_ALLOC;
        return NULL;
    }
    chunk.size = 0;

    curl_handle = createCurl(url, (void *)&chunk);
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        setErrorMessage(curl_easy_strerror(res));
        *error = ERR_HTTP_REQUEST_FAIL;
        free(chunk.memory);
        chunk.memory = NULL;
    }
    curl_easy_cleanup(curl_handle);
    return chunk.memory;
}

char **httpMultiRequest(char *urls[], unsigned int size, ErrorCode *error) {
    CURLM *cm;
    CURLMsg *msg;
    unsigned int transfers = 0;
    int msgsLeft = -1;
    int stillAlive = 1;

    cm = curl_multi_init();
    curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

    // TODO
    struct MemoryStruct chunks[size];
    for (int i = 0; i < size; ++i) {
        chunks[i].size = 0;
        chunks[i].memory = (char *)malloc(1);
        if (!chunks[i].memory) {
            *error = ERR_ALLOC;
            for (int j = 0; j < i - 1; ++j)
                free(chunks[i].memory);
            return NULL;
        }
        chunks[i].memory[0] = 0;
    }
    for (transfers = 0; transfers < MAX_PARALLEL && transfers < size; transfers++)
        curl_multi_add_handle(cm, createCurl(urls[transfers], (void *)&chunks[transfers]));

    do {
        curl_multi_perform(cm, &stillAlive);

        while ((msg = curl_multi_info_read(cm, &msgsLeft))) {
            // TODO curl error
            if (msg->msg = CURLMSG_DONE)
                curl_easy_cleanup(msg->easy_handle);
            else {
                setErrorMessage(curl_easy_strerror(msg->data.result));
                *error = ERR_HTTP_REQUEST_FAIL;
            }

            if (transfers < size)
                curl_multi_add_handle(cm, createCurl(urls[transfers], (void *)&chunks[transfers]));
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
        *error = ERR_ALLOC;
    }
    return result;
}

char *httpPostFile(const char *url, const char *filename, const char *name, ErrorCode *error) {
    struct MemoryStruct chunk;
    chunk.size = 0;
    chunk.memory = (char *)malloc(1);
    if (!chunk.memory) {
        *error = ERR_ALLOC;
        return NULL;
    }
    int fail = 0;
    CURL *handle = createCurl(url, &chunk.memory);

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
                setErrorMessage(curl_easy_strerror(response));
                *error = ERR_HTTP_REQUEST_FAIL;
                fail = 1;
            }
        } else {
            *error = ERR_FILE_OPERATION;
            setErrorMessage(filename);
            fail = 1;
        }
        curl_easy_cleanup(handle);
        curl_mime_free(mime);

    } else {
        *error = ERR_CURL_FAIL;
        fail = 1;
    }

    if (fail) {
        free(chunk.memory);
        chunk.memory = NULL;
    }

    return chunk.memory;
}

// a - json_array, o - json_object, i/d - int, l - long, s - char*
ErrorCode assingJsonValues(json_value *json, const char *format, ...) {
    if (json->type != json_object)
        return ERR_INVALID_JSON_VALUE;
    va_list args;
    va_start(args, format);
    for (int i = 0; format[i]; ++i) {
        const char *name = va_arg(args, const char *);
        json_value *val = get_by_key(json, name);
        if (!val)
            return ERR_MISSING_JSON_KEY;

        switch (format[i]) {
        case 'd':
        case 'i':
            if (val->type == json_integer) {
                *va_arg(args, int *) = val->u.integer;
                break;
            } else
                return ERR_INVALID_JSON_VALUE;

        case 'l':
            printf("{%d}", val->type);
            if (val->type == json_integer) {
                *va_arg(args, long *) = val->u.integer;
                break;
            } else
                return ERR_INVALID_JSON_VALUE;

        case 's':
            if (val->type == json_string) {
                char **str = va_arg(args, char **);
                *str = cloneStr(val->u.string.ptr);
                if (!*str)
                    return ERR_ALLOC;
                break;
            } else
                return ERR_INVALID_JSON_VALUE;

        case 'a':
            if (val->type == json_array) {
                *va_arg(args, json_value **) = val;
                break;
            } else
                return ERR_INVALID_JSON_VALUE;
        default:
            assert(0);
        }
    }
    va_end(args);
    return ERR_NONE;
}