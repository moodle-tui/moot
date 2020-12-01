#include "client.h"
#include <curl/curl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "util.h"
#define SERVICE_URL "/webservice/rest/server.php"
#define PARAM_JSON "moodlewsrestformat=json"
#define URL_LENGTH 4096

int err;

#define error err

int mt_load_courses_topics(Client *client, Courses courses);

Client *mt_new_client(char *token, char *website) {
    Client *client = (Client *)malloc(sizeof(Client));
    client->token = cloneStr(token);
    client->website = cloneStr(website);
    client->fullName = client->siteName = NULL;
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

void mt_client_write_url(Client *client, char *url, char *wsfunction, const char *format, ...) {
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL,
             client->token, PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);
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
            value = get_by_key(json, "userid");
            if (value && value->type == json_integer) {
                client->userid = value->u.integer;
            }
        } else {
            err = ERR_MOODLE;
        }
        json_value_free(json);
    }
    return err;
}

Courses mt_get_courses(Client *client) {
    Courses result = {0, NULL};
    json_value *courses =
        __mt_client_json_request(client, "core_enrol_get_users_courses", "&userid=%d", client->userid);
    if (!courses) {
        error = ERR_MOODLE;
        return result;
    }
    if (courses->type == json_array) {
        result.data = (Course *)malloc(courses->u.array.length * sizeof(Course));
        if (result.data) {
            result.len = courses->u.array.length;
            int skip = 0;
            for (int i = 0; i < result.len; ++i) {
                json_value *value = get_by_key(courses->u.array.values[i], "format");
                if (!value || value->type != json_string ||
                    strcmp(value->u.string.ptr, "topics") != 0) {
                    // only topics format courses are supported
                    ++skip;
                    continue;
                }

                value = get_by_key(courses->u.array.values[i], "id");
                if (value && value->type == json_integer) {
                    result.data[i - skip].id = value->u.integer;
                } else {
                    error = ERR_MOODLE;
                    break;
                }

                value = get_by_key(courses->u.array.values[i], "fullname");
                if (value && value->type == json_string) {
                    result.data[i - skip].name = cloneStr(value->u.string.ptr);
                    if (!result.data[i - skip].name)
                        error = ERR_ALLOC;
                } else {
                    error = ERR_MOODLE;
                    break;
                }
            }
            result.len -= skip;
        }
    }
    json_value_free(courses);
    mt_load_courses_topics(client, result);
    return result;
}

void mt_free_modules(Modules modules) {
    for (int i = 0; i < modules.len; ++i) {
        free(modules.data[i].name);
    }
    free(modules.data);
}

void mt_free_topics(Topics topics) {
    for (int i = 0; i < topics.len; ++i) {
        free(topics.data[i].name);
        free(topics.data[i].summaryHtml);
        mt_free_modules(topics.data[i].modules);
    }
    free(topics.data);
}

void mt_free_courses(Courses courses) {
    for (int i = 0; i < courses.len; ++i) {
        free(courses.data[i].name);
        mt_free_topics(courses.data[i].topics);
    }
    free(courses.data);
}

void mt_reset_courses_topics(Courses courses) {
    for (int i = 0; i < courses.len; ++i) {
        mt_free_topics(courses.data[i].topics);
        courses.data[i].topics.data = NULL;
    }
}

moduleType mt_get_mod_type(const char *module) {
    if (!strcmp(module, "assign"))
        return MODULE_ASSIGNMENT;
    if (!strcmp(module, "resource"))
        return MODULE_RESOURCE;
    if (!strcmp(module, "workshop"))
        return MODULE_WORKSHOP;
    return MOD_UNSUPPORTED;
}

// not NULL expected
Modules mt_create_modules(json_value *json) {
    Modules modules = {0, NULL};
    if (json->type == json_array) {
        modules.len = json->u.array.length;
        int skip = 0;
        // TODO
        modules.data = (Module *) malloc(modules.len * sizeof(Topic));
        for (int i = 0; i < json->u.array.length; ++i) {
            json_value *module = json->u.array.values[i];
            json_value *type = get_by_key(module, "modname");
            moduleType modtype;
            if (!type || type->type != json_string ||
                (modtype = mt_get_mod_type(type->u.string.ptr)) == MOD_UNSUPPORTED) {
                ++skip;
                continue;
            }
            modules.data[i - skip].type = modtype;

            json_value *name = get_by_key(module, "name");
            if (name && name->type == json_string)
                modules.data[i - skip].name = cloneStr(name->u.string.ptr);

            json_value *id = get_by_key(module, "id");
            if (id && id->type == json_integer)
                modules.data[i - skip].id = id->u.integer;

            json_value *instance = get_by_key(module, "instance");
            if (instance && instance->type == json_integer)
                modules.data[i - skip].instance = instance->u.integer;
        }
        modules.len -= skip;
    } else {
        // TODO
    }
    return modules;
}

// not NULL expected
Topics mt_create_topics(json_value *json) {
    Topics topics = {0, NULL};
    if (json->type == json_array) {
        topics.len = json->u.array.length;
        // TODO
        topics.data = (Topic *) malloc(topics.len * sizeof(Topic));
        for (int i = 0; i < json->u.array.length; ++i) {
            json_value *topic = json->u.array.values[i];

            json_value *summary = get_by_key(topic, "summary");
            if (summary && summary->type == json_string)
                topics.data[i].summaryHtml = cloneStr(summary->u.string.ptr);

            json_value *name = get_by_key(topic, "name");
            if (name && name->type == json_string)
                topics.data[i].name = cloneStr(name->u.string.ptr);

            json_value *id = get_by_key(topic, "id");
            if (id && id->type == json_integer)
                topics.data[i].id = id->u.integer;

            json_value *modules = get_by_key(topic, "modules");
            if (modules) {
                topics.data[i].modules = mt_create_modules(modules);
            }
        }
    } else {
        // TODO
    }
    return topics;
}

int mt_load_courses_topics(Client *client, Courses courses) {
    char urls[courses.len][URL_LENGTH + 1];
    for (int i = 0; i < courses.len; ++i) {
        mt_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d",
                            courses.data[i]);
    }
    char *urlArray[URL_LENGTH + 1];
    for (int i = 0; i < courses.len; ++i)
        urlArray[i] = urls[i];
    char **results = httpMultiRequest(urlArray, courses.len);
    // return 0;
    if (results) {
        for (int i = 0; i < courses.len; ++i) {
            if (results[i]) {
                json_value *topics = json_parse(results[i], strlen(results[i]));
                if (topics) {
                    courses.data[i].topics = mt_create_topics(topics);
                } else {
                    // TODO
                }
                json_value_free(topics);
            }
            free(results[i]);
        }
    }
    free(results);
}

void mt_destroy_client(Client *client) {
    free(client->token);
    free(client->website);
    free(client->fullName);
    free(client->siteName);
    free(client);
}