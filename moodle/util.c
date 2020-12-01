#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
#include "util.h"
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
    char *str = (char*) malloc(strlen(s) + 1);
    if (str) {
        strcpy(str, s);
    }
    return str;
}

CURL *createCurl(char *url, void *data) {
    CURL *handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    return handle;
}

char *httpRequest(char *url) {
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    curl_handle = createCurl(url, (void *)&chunk);
    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        chunk.memory = NULL;
    }
    curl_easy_cleanup(curl_handle);
    return chunk.memory;
}

char **httpMultiRequest(char *urls[], unsigned int size) {
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
        // TODO allo check
        chunks[i].memory = (char *)malloc(1);
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

            if (transfers < size)
                curl_multi_add_handle(cm, createCurl(urls[transfers], (void *)&chunks[transfers]));
        }
        if (stillAlive)
            curl_multi_wait(cm, NULL, 0, 500, NULL);

    } while (stillAlive || (transfers < size));

    curl_multi_cleanup(cm);
    // TODO
    char **result = malloc(size * sizeof(char*));
    for (int i = 0; i < size; ++i)
        result[i] = chunks[i].memory;
    return result;
}