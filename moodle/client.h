#ifndef __CLIENT_H
#define __CLIENT_H

#include <curl/curl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/json.h"
#include "util.h"
#define SERVICE_URL "/webservice/rest/server.php"
#define PARAM_JSON "moodlewsrestformat=json"
#define URL_LENGTH 4096
enum {
    ERR_NONE,
    ERR_ALLOC,
    ERR_JSON,
    ERR_MOODLE,
    ERR_RESULT,
};

typedef struct Client {
    char *fullName, *siteName;
    char *token, *website;
} Client;

Client *mt_new_client(char *token, char *website) {
    Client *client = (Client *)malloc(sizeof(Client));
    client->token = (char *)malloc(strlen(token) + 1);
    client->website = (char *)malloc(strlen(website) + 1);
    strcpy(client->token, token);
    strcpy(client->website, website);
    return client;
}

json_value *__mt_client_json_request(Client *client, char *wsfunction, const char *format, ...) {
    char url[URL_LENGTH] = "";
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL,
             client->token, PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);

    char *data = httpRequest(url);
    if (!data)
        return NULL;
    json_value *json = json_parse(data, strlen(data));
    free(data);
    return json;
}

int mt_init_client(Client *client) {
    int err = ERR_NONE;

    json_value *json = __mt_client_json_request(client, "core_webservice_get_site_info", "");
    if (json) {
        // TODO object check
        if (!get_by_key(json, "exception")) {
            json_value *value = get_by_key(json, "fullname");
            if (value && value->type == json_string) {
                client->fullName = cloneStr(value->u.string.ptr);
            } else {
                client->fullName = cloneStr("");
            }
            value = get_by_key(json, "sitename");
            if (value && value->type == json_string) {
                client->siteName = cloneStr(value->u.string.ptr);
            } else {
                client->siteName = cloneStr("");
            }
        } else {
            err = ERR_MOODLE;
        }
        json_value_free(json);
    }
    return err;
}



void mt_destroy_client(Client *client) {
    free(client->token);
    free(client->website);
    free(client->fullName);
    free(client->siteName);
    free(client);
}
#endif