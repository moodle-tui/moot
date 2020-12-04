#include <curl/curl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"
#include "json.h"
#include "util.h"
#define SERVICE_URL "/webservice/rest/server.php"
#define UPLOAD_URL "/webservice/upload.php"
#define PARAM_JSON "moodlewsrestformat=json"
#define URL_LENGTH 4096


void mt_load_courses_topics(Client *client, Courses courses, ErrorCode *error);
json_value *mt_parse_moodle_json(char *data, ErrorCode *error);
Client *mt_new_client(char *token, char *website);
json_value *__mt_client_json_request(Client *client, ErrorCode *error, char *wsfunction, const char *format, ...);
void mt_client_write_url(Client *client, char *url, char *wsfunction, const char *format, ...);
ErrorCode mt_init_client(Client *client);
Courses mt_get_courses(Client *client, ErrorCode *error);
void mt_free_modules(Modules modules);
void mt_free_topics(Topics topics);
void mt_free_courses(Courses courses);
void mt_reset_courses_topics(Courses courses);
ModuleType mt_get_mod_type(const char *module);
Modules mt_create_modules(json_value *json);
Topics mt_create_topics(json_value *json, ErrorCode *error);
void mt_load_courses_topics(Client *client, Courses courses, ErrorCode *error);
void mt_destroy_client(Client *client);
json_value *mt_parse_moodle_json(char *data, ErrorCode *error);
#define ITEM_ID_NONE 0
long mt_client_upload_file(Client *client, const char *filename, long itemId, ErrorCode *error);
long mt_client_upload_files(Client *client, const char *filenames[], int len, ErrorCode *error);
const char *mt_find_moodle_warnings(json_value *json);

Client *mt_new_client(char *token, char *website) {
    Client *client = (Client *)malloc(sizeof(Client));
    client->token = cloneStr(token);
    client->website = cloneStr(website);
    client->fullName = client->siteName = NULL;
    return client;
}

json_value *__mt_client_json_request(Client *client, ErrorCode *error, char *wsfunction, const char *format, ...) {
    char url[URL_LENGTH] = "";
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL, client->token,
             PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);

    char *data = httpRequest(url, error);
    if (!data)
        return NULL;
    json_value *json = mt_parse_moodle_json(data, error);
    free(data);
    return json;
}

void mt_client_write_url(Client *client, char *url, char *wsfunction, const char *format, ...) {
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL, client->token,
             PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);
}

ErrorCode mt_init_client(Client *client) {
    ErrorCode err = ERR_NONE;
    json_value *json = __mt_client_json_request(client, &err, "core_webservice_get_site_info", "");

    if (!err) {
        err = assingJsonValues(json, "ssd", "fullname", &client->fullName, "sitename", &client->siteName, "userid",
                               &client->userid);
    }
    json_value_free(json);
    return err;
}

Courses mt_get_courses(Client *client, ErrorCode *error) {
    ErrorCode err = ERR_NONE;
    json_value *courses =
        __mt_client_json_request(client, &err, "core_enrol_get_users_courses", "&userid=%d", client->userid);

    Courses result = {0, NULL};
    if (err) {
        *error = err;
        return result;
    }
    if (courses->type == json_array) {
        result.data = (Course *)malloc(courses->u.array.length * sizeof(Course));
        if (result.data) {
            result.len = courses->u.array.length;

            for (int i = 0; i < result.len; ++i)
                result.data[i].topics.len = 0;

            int skip = 0;
            for (int i = 0; i < result.len; ++i) {
                json_value *course = courses->u.array.values[i];
                json_value *value = get_by_key(course, "format");
                result.data[i].topics.len = 0;
                if (!value || value->type != json_string || strcmp(value->u.string.ptr, "topics") != 0) {
                    // only topics format courses are supported
                    ++skip;
                    continue;
                }
                err = assingJsonValues(course, "ds", "id", &result.data[i - skip].id, "fullname",
                                       &result.data[i - skip].name);
                if (err) {
                    *error = err;
                    break;
                }
            }
            result.len -= skip;
        } else {
            *error = ERR_ALLOC;
        }
    } else {
        *error = ERR_INVALID_JSON_VALUE;
    }
    mt_load_courses_topics(client, result, error);
    json_value_free(courses);
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

ModuleType mt_get_mod_type(const char *module) {
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
        modules.data = (Module *)malloc(modules.len * sizeof(Topic));
        for (int i = 0; i < json->u.array.length; ++i) {
            json_value *module = json->u.array.values[i];
            json_value *type = get_by_key(module, "modname");
            ModuleType modtype;
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
Topics mt_create_topics(json_value *json, ErrorCode *error) {
    Topics topics = {0, NULL};
    if (json->type == json_array) {
        topics.data = (Topic *)malloc(json->u.array.length * sizeof(Topic));
        if (!topics.data) {
            *error = ERR_ALLOC;
        } else {
            for (int i = 0; i < json->u.array.length; ++i) {
                Topic *topic = &topics.data[i];
                topic->modules.len = 0;
                json_value *modules;
                ErrorCode err = assingJsonValues(json->u.array.values[i], "ssda", "summary", &topic->summaryHtml,
                                                 "name", &topic->name, "id", &topic->id, "modules", &modules);
                ++topics.len;
                if (err) {
                    *error = err;
                    break;
                }
                topics.data[i].modules = mt_create_modules(modules);
            }
        }
    } else {
        *error = ERR_INVALID_JSON_VALUE;
    }
    return topics;
}

void mt_load_courses_topics(Client *client, Courses courses, ErrorCode *error) {
    char urls[courses.len][URL_LENGTH + 1];
    for (int i = 0; i < courses.len; ++i) {
        mt_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d", courses.data[i].id);
    }
    char *urlArray[URL_LENGTH + 1];
    for (int i = 0; i < courses.len; ++i)
        urlArray[i] = urls[i];
    char **results = httpMultiRequest(urlArray, courses.len, error);

    if (results) {
        for (int i = 0; i < courses.len; ++i) {
            if (results[i]) {
                ErrorCode err = ERR_NONE;
                json_value *topics = mt_parse_moodle_json(results[i], &err);
                if (!err) {
                    courses.data[i].topics = mt_create_topics(topics, &err);
                    if (err)
                        *error = err;
                    json_value_free(topics);
                } else {
                    *error = err;
                }
            }
        }
        for (int i = 0; i < courses.len; ++i)
            free(results[i]);
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

// Parses json and looks for moodle exeption. on success json value needs to be
// freed.
json_value *mt_parse_moodle_json(char *data, ErrorCode *error) {
    json_value *json = json_parse(data, strlen(data));
    if (json) {
        if (get_by_key(json, "exception")) {
            json_value *msg = get_by_key(json, "message");
            if (msg && msg->type == json_string)
                setErrorMessage(msg->u.string.ptr);
            json_value_free(json);
            json = NULL;
            *error = ERR_MOODLE_EXCEPTION;
        }
    } else {
        *error = ERR_INVALID_JSON;
    }
    return json;
}

long mt_client_upload_file(Client *client, const char *filename, long itemId, ErrorCode *error) {
    *error = ERR_NONE;
    char url[URL_LENGTH];
    long resultId = 0;
    sprintf(url,
            "%s%s"
            "?token=%s"
            "&itemid=%d",
            client->website, UPLOAD_URL, client->token, itemId);
    char *data = httpPostFile(url, filename, "file_box", error);
    if (!*error) {
        json_value *json = mt_parse_moodle_json(data, error);
        if (!*error) {
            if (json->type == json_array && json->u.array.length > 0)
                *error = assingJsonValues(json->u.array.values[0], "l", "itemid", &resultId);
            else
                *error = ERR_INVALID_JSON_VALUE;
        }
        json_value_free(json);
    }
    free(data);
    return resultId;
}

long mt_client_upload_files(Client *client, const char *filenames[], int len, ErrorCode *error) {
    *error = ERR_NONE;
    long itemId = mt_client_upload_file(client, filenames[0], ITEM_ID_NONE, error);
    if (!*error) {
        for (int i = 1; i < len; ++i) {
            mt_client_upload_file(client, filenames[i], itemId, error);
            if (*error)
                break;
        }
    }
    return itemId;
}

const char *mt_find_moodle_warnings(json_value *json) {
    json_value *warnings;
    if (warnings = get_by_key(json, "warnings")) {
        json = warnings;
    }
    if (json->type == json_array && json->u.array.length > 0) {
        if (get_by_key(json->u.array.values[0], "warningcode")) {
            json_value *message = get_by_key(json->u.array.values[0], "message");
            if (message && message->type == json_string)
                return message->u.string.ptr;
        }
    }
    return NULL;
}

void mt_client_mod_assign_submit(Client *client, Module assignment, const char *filenames[], int len,
                                 ErrorCode *error) {
    *error = assignment.type == MODULE_ASSIGNMENT ? ERR_NONE : ERR_MISUSED_MOODLE_API;
    if (*error)
        return;
    long itemId = mt_client_upload_files(client, filenames, len, error);
    if (*error)
        return;
    // ignore submit text and comments for now.
    json_value *json = __mt_client_json_request(client, error, "mod_assign_save_submission",
                                                "&assignmentid=%d"
                                                "&plugindata[files_filemanager]=%ld"
                                                "&plugindata[onlinetext_editor][text]="
                                                "&plugindata[onlinetext_editor][format]=4"
                                                "&plugindata[onlinetext_editor][itemid]=%ld",
                                                assignment.instance, itemId, itemId);
    if (!*error) {
        const char *message = mt_find_moodle_warnings(json);
        if (message) {
            *error = ERR_MOODLE_EXCEPTION;
            setErrorMessage(message);
        }
    }
    json_value_free(json);
}

void mt_client_mod_workshop_submit(Client *client, Module workshop, const char *filenames[], int len, const char *title,
                                   ErrorCode *error) {
    *error = workshop.type == MODULE_WORKSHOP ? ERR_NONE : ERR_MISUSED_MOODLE_API;
    if (*error)
        return;
    long itemId = mt_client_upload_files(client, filenames, len, error);
    if (*error)
        return;

    json_value *json = __mt_client_json_request(client, error, "mod_workshop_add_submission",
                                                "&workshopid=%d"
                                                "&title=%s"
                                                "&attachmentsid=%ld",
                                                workshop.instance, title, itemId);
    if (!*error) {
        const char *message = mt_find_moodle_warnings(json);
        if (message) {
            *error = ERR_MOODLE_EXCEPTION;
            setErrorMessage(message);
        }
    }
    json_value_free(json);
}