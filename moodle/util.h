#ifndef __UTIL_H
#define __UTIL_H

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/json.h"

struct MemoryStruct {
    char *memory;
    size_t size;
};

typedef struct Param {
    char *name, *value;
} Param;

json_value *get_by_key(json_value *json, const char *key) {
    if (json->type != json_object)
        return NULL;
    for (int i = 0; i < json->u.object.length; ++i) {
        if (strcmp(json->u.object.values[i].name, key) == 0) {
            // return NULL;
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
    char *str = (char*) malloc(strlen(s) + 1);
    if (str) {
        strcpy(str, s);
    }
    return str;
}

// TODO
char *addParamsToUrl(char *url, Param params[], int paramCount) {
    size_t len = strlen(url) + 1;
    for (int i = 0; i < paramCount; ++i) {
        // TODO
        //char *esc = curl_easy_escape(curl_handle, params[i].value);
        len += strlen(params[i].name) + strlen(params[i].value) + 1;
    }
    char *str = malloc(len + 1);
    strcpy(str, url);
}

char *httpRequest(char *url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        chunk.memory = NULL;
    }
    curl_easy_cleanup(curl_handle);
    return chunk.memory;
}
#endif
