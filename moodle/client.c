/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * Part of moodle library, general api implementation. See moodle.h
*/

#include <curl/curl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "json.h"
#define MD_SERVICE_URL "/webservice/rest/server.php"
#define MD_UPLOAD_URL "/webservice/upload.php"
#define MD_PARAM_JSON "moodlewsrestformat=json"
#define MD_WSTOKEN "wstoken"
#define MD_WSFUNCTION "wsfunction"
#define MD_URL_LENGTH 4096
#define MD_NO_IDENTIFIER -1
#define MD_NO_ITEM_ID 0

// MD_CLIENT_STRING_FIELDS is a macro that expands to array initializer with
// pointers to every string (char *) in given client (MDClient *).
#define MD_CLIENT_STRING_FIELDS(client) \
    { &client->fullName, &client->siteName, &client->token, &client->website }

// List of known modules. Order is important. Indeces should match the types (MDModType).
static MDMod mdModList[MD_MOD_COUNT] = {
    {
        .type = MD_MOD_ASSIGNMENT,
        .name = "assign",
        .parseWsFunction = "mod_assign_get_assignments",
        .parseFunc = md_client_courses_set_mod_assignment_data,
        .statusWsFunction = "mod_assign_get_submission_status",
        .statusInstanceName = "assignid",
        .statusParseFunc = md_mod_assign_parse_status,
        .initFunc = (MDInitFunc)md_mod_assignment_init,
        .cleanupFunc = (MDCleanupFunc)md_mod_assignment_cleanup,
    },
    {
        .type = MD_MOD_WORKSHOP,
        .name = "workshop",
        .parseWsFunction = "mod_workshop_get_workshops_by_courses",
        .parseFunc = md_client_courses_set_mod_workshop_data,
        .statusWsFunction = "mod_workshop_get_submissions",
        .statusInstanceName = "workshopid",
        .statusParseFunc = md_mod_workshop_parse_status,
        .initFunc = (MDInitFunc)md_mod_workshop_init,
        .cleanupFunc = (MDCleanupFunc)md_mod_workshop_cleanup,
    },
    {
        .type = MD_MOD_RESOURCE,
        .name = "resource",
        .parseWsFunction = "mod_resource_get_resources_by_courses",
        .parseFunc = md_client_courses_set_mod_resource_data,
        .statusParseFunc = NULL,
        .initFunc = (MDInitFunc)md_mod_resource_init,
        .cleanupFunc = (MDCleanupFunc)md_mod_resource_cleanup,
    },
    {
        .type = MD_MOD_URL,
        .name = "url",
        .parseWsFunction = "mod_url_get_urls_by_courses",
        .parseFunc = md_client_courses_set_mod_url_data,
        .statusParseFunc = NULL,
        .initFunc = (MDInitFunc)md_mod_url_init,
        .cleanupFunc = (MDCleanupFunc)md_mod_url_cleanup,
    },
};

void md_init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void md_cleanup() {
    curl_global_cleanup();
}

MDClient *md_client_new(cchar *token, cchar *website, MDError *error) {
    *error = MD_ERR_NONE;
    MDClient *client = (MDClient *)md_malloc(sizeof(MDClient), error);
    if (client) {
        client->token = clone_str(token, error);
        client->website = clone_str(website, error);
        client->fullName = client->siteName = NULL;
    }
    return client;
}

MDClient *md_client_load_from_file(cchar *filename, MDError *error) {
    *error = MD_ERR_NONE;
    FILE *file = fopen(filename, "rb");
    MDClient *client = NULL;
    if (!file) {
        *error = MD_ERR_FILE_OPERATION;
    } else {
        client = md_malloc(sizeof(MDClient), error);
        if (client) {
            char **clientStringFields[] = MD_CLIENT_STRING_FIELDS(client);
            const int fieldCount = sizeof(clientStringFields) / sizeof(clientStringFields[0]);

            for (int i = 0; i < fieldCount; ++i) {
                *clientStringFields[i] = NULL;
            }

            if (!fread(client, sizeof(MDClient), 1, file)) {
                *error = MD_ERR_FILE_OPERATION;
            } else {
                for (int i = 0; i < fieldCount && !*error; ++i) {
                    *clientStringFields[i] = fread_string(file, error);
                }
            }
        }
        if (*error) {
            md_client_cleanup(client);
            client = NULL;
        }
        fclose(file);
    }

    if (*error == MD_ERR_FILE_OPERATION)
        md_error_set_message(filename);

    return client;
}

void md_client_save_to_file(MDClient *client, cchar *filename, MDError *error) {
    *error = MD_ERR_NONE;
    FILE *file = fopen(filename, "wb");
    if (!file) {
        *error = MD_ERR_FILE_OPERATION;
    } else {
        if (!fwrite(client, sizeof(MDClient), 1, file)) {
            *error = MD_ERR_FILE_OPERATION;
        } else {
            char **clientStringFields[] = MD_CLIENT_STRING_FIELDS(client);
            const int fieldCount = sizeof(clientStringFields) / sizeof(clientStringFields[0]);

            for (int i = 0; i < fieldCount && !*error; ++i) {
                if (!fwrite(*clientStringFields[i], strlen(*clientStringFields[i]) + 1, 1, file)) {
                    *error = MD_ERR_FILE_OPERATION;
                }
            }
        }
        fclose(file);
    }

    if (*error == MD_ERR_FILE_OPERATION)
        md_error_set_message(filename);
}

Json *md_client_do_http_json_request(MDClient *client, MDError *error, char *wsfunction, cchar *format, ...) {
    va_list args;
    va_start(args, format);
    char url[MD_URL_LENGTH];
    md_client_write_url_varg(client, url, wsfunction, format, args);
    va_end(args);

    // printf("<%s>\n", url);
    char *data = http_get_request(url, error);
    if (!data)
        return NULL;
    Json *json = md_parse_moodle_json(data, error);
    free(data);
    return json;
}

int md_client_write_url_varg(MDClient *client, char *url, cchar *wsfunction, cchar *format, va_list args) {
    int len = snprintf(url, MD_URL_LENGTH, "%s%s?" MD_WSTOKEN "=%s&%s&" MD_WSFUNCTION "=%s", client->website, MD_SERVICE_URL, client->token, MD_PARAM_JSON, wsfunction);
    return len + vsnprintf(url + len, MD_URL_LENGTH - len, format, args);
}

int md_client_write_url(MDClient *client, char *url, cchar *wsfunction, cchar *format, ...) {
    va_list args;
    va_start(args, format);
    int len = md_client_write_url_varg(client, url, wsfunction, format, args);
    va_end(args);
    return len;
}

void md_client_init(MDClient *client, MDError *error) {
    *error = MD_ERR_NONE;
    Json *json = md_client_do_http_json_request(client, error, "core_webservice_get_site_info", "");
    if (!*error) {
        client->fullName = json_get_string(json, "fullname", error);
        client->siteName = json_get_string(json, "sitename", error);
        client->userid = json_get_integer(json, "userid", error);
        client->uploadLimit = json_get_integer(json, "usermaxuploadfilesize", error);
    }
    md_cleanup_json(json);
}

static int compareByCourseName(const void *a, const void *b) {
    const char *s1 = ((MDCourse *)a)->name, *s2 = ((MDCourse *)b)->name;
    // Skip leading whitespaces.
    return strcmp(s1 + strspn(s1, " "), s2 + strspn(s2, " "));
}

MDArray md_client_fetch_courses(MDClient *client, bool sortByName, MDError *error) {
    *error = MD_ERR_NONE;
    Json *jsonCourses = md_client_do_http_json_request(client, error, "core_enrol_get_users_courses", "&userid=%d", client->userid);

    MDArray courses;
    md_array_init(&courses);
    if (!*error && jsonCourses->type == JSON_ARRAY) {
        md_array_init_new(&courses, sizeof(MDCourse), jsonCourses->array.len, (MDInitFunc)md_course_init, error);
        MDCourse *courseArr = MD_ARR(courses, MDCourse);
        if (!*error) {
            courses.len = jsonCourses->array.len;

            for (int i = 0; i < courses.len; ++i)
                courseArr[i].topics.len = 0;

            int skip = 0;
            for (int i = 0; i < courses.len && !*error; ++i) {
                Json *course = &jsonCourses->array.values[i];
                cchar *format = json_get_string_no_alloc(course, "format", error);
                // only topics or weeks format courses are supported
                if (format && strcmp(format, "topics") && strcmp(format, "weeks")) {
                    ++skip;
                } else {
                    courseArr[i - skip].id = json_get_integer(course, "id", error);
                    courseArr[i - skip].name = json_get_string(course, "fullname", error);
                }
            }
            courses.len -= skip;
        } else {
            *error = MD_ERR_ALLOC;
        }
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    md_courses_fetch_topic_contents(client, courses, error);
    md_cleanup_json(jsonCourses);
    if (sortByName) {
        sort(courses._data, courses.len, sizeof(MDCourse), compareByCourseName);
    }
    return courses;
}

void md_courses_cleanup(MDArray courses) {
    md_array_cleanup(&courses, sizeof(MDCourse), (MDCleanupFunc)md_course_cleanup);
}

void md_array_init_new(MDArray *array, size_t size, int length, MDInitFunc callback, MDError *error) {
    array->len = length;
    if (size && length) {
        array->_data = md_malloc(length * size, error);
        if (array->_data && callback) {
            for (int i = 0; i < length; ++i) {
                callback((void *)((char *)array->_data + i * size));
            }
        }
    } else {
        array->_data = NULL;
    }
}

void md_array_cleanup(MDArray *array, size_t size, MDCleanupFunc callback) {
    if (callback) {
        for (int i = 0; i < array->len; ++i)
            callback((void *)((char *)array->_data + i * size));
    }
    free(array->_data);
    array->len = 0;
    array->_data = NULL;
}

void md_array_init(MDArray *array) {
    array->len = 0;
    array->_data = NULL;
}

void md_course_init(MDCourse *course) {
    course->name = NULL;
    course->id = MD_NO_IDENTIFIER;
    md_array_init(&course->topics);
}

void md_course_cleanup(MDCourse *course) {
    free(course->name);
    md_array_cleanup(&course->topics, sizeof(MDTopic), (MDCleanupFunc)md_topic_cleanup);
}

void md_topic_init(MDTopic *topic) {
    topic->name = NULL;
    topic->id = MD_NO_IDENTIFIER;
    md_array_init(&topic->modules);
}

void md_topic_cleanup(MDTopic *topic) {
    free(topic->name);
    md_rich_text_cleanup(&topic->summary);
    md_array_cleanup(&topic->modules, sizeof(MDModule), (MDCleanupFunc)md_module_cleanup);
    md_topic_init(topic);
}

void md_module_init(MDModule *module) {
    module->name = NULL;
    module->id = MD_NO_IDENTIFIER;
    module->instance = MD_NO_IDENTIFIER;
    module->type = MD_MOD_UNSUPPORTED;
}

void md_module_cleanup(MDModule *module) {
    if (module->type != MD_MOD_UNSUPPORTED) {
        free(module->name);
        mdModList[module->type].cleanupFunc(module);
    }
}

void md_mod_assignment_init(MDModule *module) {
    MDModAssignment *assignment = &module->contents.assignment;
    assignment->fromDate = assignment->dueDate = assignment->cutOffDate = MD_DATE_NONE;
    md_rich_text_init(&assignment->description);
    md_file_submission_init(&assignment->fileSubmission);
    md_text_submission_init(&assignment->textSubmission);
    md_array_init(&assignment->files);
}

void md_mod_assignment_cleanup(MDModule *module) {
    MDModAssignment *assignment = &module->contents.assignment;
    md_rich_text_cleanup(&assignment->description);
    md_file_submission_cleanup(&assignment->fileSubmission);
    md_text_submission_cleanup(&assignment->textSubmission);
    md_array_cleanup(&assignment->files, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_mod_workshop_init(MDModule *module) {
    MDModWorkshop *workshop = &module->contents.workshop;
    workshop->fromDate = workshop->dueDate = MD_DATE_NONE;
    md_rich_text_init(&workshop->description);
    md_rich_text_init(&workshop->instructions);
    workshop->lateSubmissions = false;
    md_file_submission_init(&workshop->fileSubmission);
    md_text_submission_init(&workshop->textSubmission);
}

void md_mod_workshop_cleanup(MDModule *module) {
    MDModWorkshop *workshop = &module->contents.workshop;
    md_rich_text_cleanup(&workshop->description);
    md_rich_text_cleanup(&workshop->instructions);
    md_file_submission_cleanup(&workshop->fileSubmission);
    md_text_submission_cleanup(&workshop->textSubmission);
}

void md_mod_url_init(MDModule *module) {
    MDModUrl *url = &module->contents.url;
    md_rich_text_init(&url->description);
    url->name = url->url = NULL;
}

void md_mod_url_cleanup(MDModule *module) {
    MDModUrl *url = &module->contents.url;
    md_rich_text_cleanup(&url->description);
    free(url->name);
    free(url->url);
}

void md_mod_resource_init(MDModule *module) {
    MDModResource *resource = &module->contents.resource;
    md_rich_text_init(&resource->description);
    md_array_init(&resource->files);
}

void md_mod_resource_cleanup(MDModule *module) {
    MDModResource *resource = &module->contents.resource;
    md_rich_text_cleanup(&resource->description);
    md_array_cleanup(&resource->files, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_rich_text_init(MDRichText *richText) {
    richText->text = NULL;
    richText->format = MD_FORMAT_PLAIN;
}

void md_rich_text_cleanup(MDRichText *richText) {
    free(richText->text);
}

void md_file_submission_init(MDFileSubmission *submission) {
    submission->status = MD_SUBMISSION_DISABLED;
    submission->maxUploadedFiles = MD_NO_FILE_LIMIT;
    submission->maxSubmissionSize = 0;
    submission->acceptedFileTypes = NULL;
}

void md_file_submission_cleanup(MDFileSubmission *submission) {
    free(submission->acceptedFileTypes);
}

void md_text_submission_init(MDTextSubmission *submission) {
    submission->status = MD_SUBMISSION_DISABLED;
    submission->wordLimit = 0;
}

void md_text_submission_cleanup(MDTextSubmission *submission) {
    //
}

void md_mod_assignment_status_init(MDModAssignmentStatus *status) {
    status->grade = NULL;
    md_rich_text_init(&status->submittedText);
    md_array_init(&status->submittedFiles);
}

void md_mod_assignment_status_cleanup(MDModAssignmentStatus *status) {
    free(status->grade);
    md_rich_text_cleanup(&status->submittedText);
    md_array_cleanup(&status->submittedFiles, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_mod_workshop_status_init(MDModWorkshopStatus *status) {
    status->title = NULL;
    md_rich_text_init(&status->submittedText);
    md_array_init(&status->submittedFiles);
}

void md_mod_workshop_status_cleanup(MDModWorkshopStatus *status) {
    free(status->title);
    md_rich_text_cleanup(&status->submittedText);
    md_array_cleanup(&status->submittedFiles, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_status_ref_init(MDStatusRef *status) {
    switch (status->module->type) {
        case MD_MOD_ASSIGNMENT:
            md_mod_assignment_status_init(&status->status.assignment);
            break;

        case MD_MOD_WORKSHOP:
            md_mod_workshop_status_init(&status->status.workshop);
            break;

        default:
            break;
    }
}

void md_status_ref_cleanup(MDStatusRef *status) {
    switch (status->module->type) {
        case MD_MOD_ASSIGNMENT:
            md_mod_assignment_status_cleanup(&status->status.assignment);
            break;

        case MD_MOD_WORKSHOP:
            md_mod_workshop_status_cleanup(&status->status.workshop);
            break;

        default:
            break;
    }
}

void md_file_init(MDFile *file) {
    file->filename = NULL;
    file->url = NULL;
    file->filesize = 0;
}

void md_file_cleanup(MDFile *file) {
    free(file->filename);
    free(file->url);
}

MDModType md_get_mod_type(cchar *module) {
    for (MDModType mod = 0; mod < MD_MOD_COUNT; ++mod) {
        if (!strcmp(module, mdModList[mod].name))
            return mdModList[mod].type;  // equal to mod anyway
    }
    return MD_MOD_UNSUPPORTED;
}

// not NULL expected
MDArray md_parse_modules(Json *json, MDError *error) {
    MDArray modulesArr;
    md_array_init(&modulesArr);
    if (json->type == JSON_ARRAY) {
        int skip = 0;
        md_array_init_new(&modulesArr, sizeof(MDModule), json->array.len, (MDInitFunc)md_module_init, error);
        MDModule *modules = MD_ARR(modulesArr, MDModule);

        for (int i = 0; i < json->array.len && !*error; ++i) {
            Json *jsonModule = &json->array.values[i];
            bool userVisible = json_get_bool(jsonModule, "uservisible", error);
            cchar *type = json_get_string_no_alloc(jsonModule, "modname", error);

            if (*error || !userVisible) {
                ++skip;
            } else if (!type || (modules[i - skip].type = md_get_mod_type(type)) == MD_MOD_UNSUPPORTED) {
                ++skip;
            } else {
                mdModList[modules[i - skip].type].initFunc(&modules[i - skip]);
                modules[i - skip].name = json_get_string(jsonModule, "name", error);
                modules[i - skip].id = json_get_integer(jsonModule, "id", error);
                modules[i - skip].instance = json_get_integer(jsonModule, "instance", error);
            }
        }
        modulesArr.len -= skip;
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    return modulesArr;
}

MDArray md_parse_topics(Json *json, MDError *error) {
    MDArray topicArr;
    md_array_init(&topicArr);
    if (json->type == JSON_ARRAY) {
        md_array_init_new(&topicArr, sizeof(MDTopic), json->array.len, (MDInitFunc)md_topic_init, error);
        for (int i = 0; i < json->array.len && (!*error); ++i) {
            MDTopic *topic = &MD_ARR(topicArr, MDTopic)[i];
            topic->id = json_get_integer(&json->array.values[i], "id", error);
            topic->name = json_get_string(&json->array.values[i], "name", error);
            topic->summary.text = json_get_string(&json->array.values[i], "summary", error);
            topic->summary.format = json_get_integer(&json->array.values[i], "summaryformat", error);
            Json *modules = json_get_array(&json->array.values[i], "modules", error);

            if (*error)
                break;
            MD_ARR(topicArr, MDTopic)[i].modules = md_parse_modules(modules, error);
        }
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    return topicArr;
}

void md_courses_fetch_topic_contents(MDClient *client, MDArray courses, MDError *error) {
    int count = courses.len + MD_MOD_COUNT;
    char urls[count][MD_URL_LENGTH];
    for (int i = 0; i < courses.len; ++i) {
        md_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d", MD_ARR(courses, MDCourse)[i].id);
    }
    for (int i = 0; i < MD_MOD_COUNT; ++i) {
        md_client_write_url(client, urls[courses.len + i], mdModList[i].parseWsFunction, "");
    }

    char *urlArray[count];
    for (int i = 0; i < count; ++i)
        urlArray[i] = urls[i];

    char **results = http_get_multi_request(urlArray, count, error);

    if (!*error) {
        for (int i = 0; i < courses.len && (!*error); ++i) {
            Json *topics = md_parse_moodle_json(results[i], error);
            if (!*error) {
                MD_ARR(courses, MDCourse)[i].topics = md_parse_topics(topics, error);
            }
            md_cleanup_json(topics);
        }
        for (int i = 0; i < MD_MOD_COUNT && (!*error); ++i) {
            Json *json = md_parse_moodle_json(results[courses.len + i], error);
            if (!*error)
                mdModList[i].parseFunc(client, courses, json, error);
            md_cleanup_json(json);
        }
        for (int i = 0; i < count; ++i)
            free(results[i]);
    }
    free(results);
}

void md_client_cleanup(MDClient *client) {
    if (client) {
        free(client->token);
        free(client->website);
        free(client->fullName);
        free(client->siteName);
    }
    free(client);
}

// Parses json and looks for moodle exeption. on success json value needs to be freed.
Json *md_parse_moodle_json(char *data, MDError *error) {
    ENSURE_EMPTY_ERROR(error);
    Json *json = md_parse_json(data, error);
    if (json) {
        if (json_get_string_no_alloc(json, "exception", &(MDError){0})) {
            cchar *msg = json_get_string_no_alloc(json, "message", &(MDError){0});
            md_error_set_message(msg ? msg : data);
            *error = MD_ERR_MOODLE_EXCEPTION;
        }
    }
    return json;
}

long md_client_upload_file(MDClient *client, cchar *filename, long itemId, MDError *error) {
    *error = MD_ERR_NONE;
    char url[MD_URL_LENGTH];
    long resultId = 0;
    sprintf(url,
            "%s%s"
            "?token=%s"
            "&itemid=%ld",
            client->website, MD_UPLOAD_URL, client->token, itemId);
    char *data = http_post_file(url, filename, "file_box", error);
    if (!*error) {
        Json *json = md_parse_moodle_json(data, error);
        if (!*error) {
            if (json->type == JSON_ARRAY && json->array.len > 0)
                resultId = json_get_integer(&json->array.values[0], "itemid", error);
            else
                *error = MD_ERR_INVALID_JSON_VALUE;
        }
        md_cleanup_json(json);
    }
    free(data);
    return resultId;
}

long md_client_upload_files(MDClient *client, MDArray filenames, MDError *error) {
    *error = MD_ERR_NONE;
    cchar **files = MD_ARR(filenames, cchar *);
    long itemId = md_client_upload_file(client, files[0], MD_NO_ITEM_ID, error);
    if (!*error) {
        for (int i = 1; i < filenames.len; ++i) {
            md_client_upload_file(client, files[i], itemId, error);
            if (*error)
                break;
        }
    }
    return itemId;
}

cchar *md_find_moodle_warning(Json *json) {
    Json *warnings;
    if ((warnings = json_get_property_silent(json, "warnings"))) {
        json = warnings;
    }
    if (json->type == JSON_ARRAY && json->array.len > 0) {
        if (json_get_property_silent(&json->array.values[0], "warningcode")) {
            return json_get_string_no_alloc(&json->array.values[0], "message", &(MDError){0});
        }
    }
    return NULL;
}

void md_client_mod_assign_submit(MDClient *client, MDModule *assignment, MDArray *filenames, MDRichText *text, MDError *error) {
    *error = MD_ERR_NONE;
    char params[MD_URL_LENGTH] = "";
    int len = 0;
    if (filenames) {
        long itemId = md_client_upload_files(client, *filenames, error);
        if (*error)
            return;
        len = snprintf(params + len, MD_URL_LENGTH - len, "&plugindata[files_filemanager]=%ld", itemId);
    }
    if (text) {
        char *escaped = url_escape(text->text, error);
        if (escaped) {
            len = snprintf(params + len, MD_URL_LENGTH - len,
                           "&plugindata[onlinetext_editor][text]=%s"
                           "&plugindata[onlinetext_editor][format]=%d"
                           "&plugindata[onlinetext_editor][itemid]=0",
                           escaped, text->format);
            curl_free(escaped);
        }
        if (*error)
            return;
    }
    Json *json = md_client_do_http_json_request(client, error, "mod_assign_save_submission",
                                                "&assignmentid=%d"
                                                "%s",
                                                assignment->instance, params);

    if (!*error) {
        cchar *message = md_find_moodle_warning(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_error_set_message(message);
        }
    }
    md_cleanup_json(json);
}

void md_client_mod_workshop_submit(MDClient *client, MDModule *workshop, MDArray *filenames, MDRichText *text, cchar *title, MDError *error) {
    *error = MD_ERR_NONE;
    char params[MD_URL_LENGTH] = "";
    int len = 0;
    if (filenames) {
        long itemId = md_client_upload_files(client, *filenames, error);
        if (*error)
            return;
        len = snprintf(params + len, MD_URL_LENGTH - len, "&attachmentsid=%ld", itemId);
    }
    if (text) {
        len = snprintf(params + len, MD_URL_LENGTH - len, "&content=%s&contentformat=%d", url_escape(text->text, error), text->format);
    }
    title = url_escape(title, error);
    if (*error)
        return;
    Json *json = md_client_do_http_json_request(client, error, "mod_workshop_add_submission",
                                                "&workshopid=%d"
                                                "&title=%s"
                                                "%s",
                                                workshop->instance, title, params);
    free((char *)title);
    if (!*error) {
        cchar *message = md_find_moodle_warning(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_error_set_message(message);
        }
    }
    md_cleanup_json(json);
}

MDModule *md_courses_locate_module(MDArray courses, int courseId, int moduleId, int instance, MDError *error) {
    // TODO: optimize for log n lookup
    for (int i = 0; i < courses.len; ++i) {
        MDCourse *course = &MD_COURSES(courses)[i];
        if (course->id == courseId) {
            for (int j = 0; j < course->topics.len; ++j) {
                MDTopic *topic = &MD_TOPICS(course->topics)[j];
                for (int k = 0; k < topic->modules.len; ++k) {
                    MDModule *module = &MD_MODULES(topic->modules)[k];
                    if (module->instance == instance && module->id == moduleId) {
                        return module;
                    }
                }
            }
        }
    }
    *error = MD_ERR_MISMACHING_MOODLE_DATA;
    return NULL;
}

MDModule *md_courses_locate_json_module(MDArray courses, Json *json, cchar *moduleIdJsonName, MDError *error) {
    int courseId = json_get_integer(json, "course", error);
    int moduleId = json_get_integer(json, moduleIdJsonName, error);
    int instance = json_get_integer(json, "id", error);
    return *error ? NULL : md_courses_locate_module(courses, courseId, moduleId, instance, error);
}

MDArray md_parse_files(Json *jsonFiles, MDError *error) {
    MDArray files;
    md_array_init_new(&files, sizeof(MDFile), jsonFiles->array.len, (MDInitFunc)md_file_init, error);
    for (int i = 0; i < jsonFiles->array.len && (!*error); ++i) {
        Json *jsonAttachment = &jsonFiles->array.values[i];
        MD_ARR(files, MDFile)[i].filename = json_get_string(jsonAttachment, "filename", error);
        MD_ARR(files, MDFile)[i].filesize = json_get_integer(jsonAttachment, "filesize", error);
        MD_ARR(files, MDFile)[i].url = json_get_string(jsonAttachment, "fileurl", error);
    }
    return files;
}

void md_parse_mod_assignment_plugins(Json *configs, MDModAssignment *assignment, MDError *error) {
    for (int i = 0; i < configs->array.len && (!*error); ++i) {
        Json *config = &configs->array.values[i];
        cchar *plugin = json_get_string_no_alloc(config, "plugin", error);
        if (*error)
            break;
        if (strcmp(plugin, "file") == 0) {
            cchar *name = json_get_string_no_alloc(config, "name", error);
            cchar *value = json_get_string_no_alloc(config, "value", error);
            if (*error)
                break;
            if (strcmp(name, "enabled") == 0)
                assignment->fileSubmission.status = !atoi(value) ? MD_SUBMISSION_DISABLED : MD_SUBMISSION_REQUIRED;
            else if (strcmp(name, "maxfilesubmissions") == 0)
                assignment->fileSubmission.maxUploadedFiles = atoi(value);
            else if (strcmp(name, "filetypeslist") == 0)
                assignment->fileSubmission.acceptedFileTypes = clone_str(value, error);
            else if (strcmp(name, "maxsubmissionsizebytes") == 0)
                assignment->fileSubmission.maxSubmissionSize = atoll(value);
        } else if (strcmp(plugin, "onlinetext") == 0) {
            cchar *name = json_get_string_no_alloc(config, "name", error);
            cchar *value = json_get_string_no_alloc(config, "value", error);
            if (*error)
                break;
            if (strcmp(name, "enabled") == 0)
                assignment->textSubmission.status = !atoi(value) ? MD_SUBMISSION_DISABLED : MD_SUBMISSION_REQUIRED;
            else if (strcmp(name, "wordlimit") == 0)
                assignment->textSubmission.wordLimit = atoi(value);
        }
    }
}

void md_client_courses_set_mod_assignment_data(MDClient *client, MDArray courses, Json *json, MDError *error) {
    Json *jsonCourses = json_get_array(json, "courses", error);
    if (!*error) {
        for (int i = 0; i < jsonCourses->array.len; ++i) {
            Json *jsonAssignments = json_get_array(&jsonCourses->array.values[i], "assignments", error);

            // Parse the actual assignment details.
            for (int j = 0; j < jsonAssignments->array.len && (!*error); ++j) {
                Json *jsonAssignment = &jsonAssignments->array.values[j];
                MDModule *module = md_courses_locate_json_module(courses, jsonAssignment, "cmid", error);
                if (*error)
                    break;
                module->type = MD_MOD_ASSIGNMENT;
                MDModAssignment *assignment = &module->contents.assignment;
                assignment->fromDate = json_get_integer(jsonAssignment, "allowsubmissionsfromdate", error);
                assignment->dueDate = json_get_integer(jsonAssignment, "duedate", error);
                assignment->cutOffDate = json_get_integer(jsonAssignment, "cutoffdate", error);
                assignment->description.text = json_get_string(jsonAssignment, "intro", error);
                assignment->description.format = json_get_integer(jsonAssignment, "introformat", error);
                Json *jsonAttachments = json_get_array(jsonAssignment, "introattachments", error);
                if (jsonAttachments)
                    assignment->files = md_parse_files(jsonAttachments, error);
                Json *configs = json_get_array(jsonAssignment, "configs", error);
                if (configs)
                    md_parse_mod_assignment_plugins(configs, assignment, error);
            }
        }
    }
}

void md_client_courses_set_mod_workshop_data(MDClient *client, MDArray courses, Json *json, MDError *error) {
    Json *jsonWorkshops = json_get_array(json, "workshops", error);
    if (!*error) {
        for (int i = 0; i < jsonWorkshops->array.len; ++i) {
            Json *jsonWorkshop = &jsonWorkshops->array.values[i];
            MDModule *module = md_courses_locate_json_module(courses, jsonWorkshop, "coursemodule", error);
            if (*error)
                break;
            module->type = MD_MOD_WORKSHOP;
            MDModWorkshop *workshop = &module->contents.workshop;
            workshop->fromDate = json_get_integer(jsonWorkshop, "submissionstart", error);
            workshop->dueDate = json_get_integer(jsonWorkshop, "submissionend", error);
            workshop->lateSubmissions = json_get_bool(jsonWorkshop, "latesubmissions", error);
            workshop->description.text = json_get_string(jsonWorkshop, "intro", error);
            workshop->description.format = json_get_integer(jsonWorkshop, "introformat", error);
            workshop->instructions.text = json_get_string(jsonWorkshop, "instructauthors", error);
            workshop->instructions.format = json_get_integer(jsonWorkshop, "instructauthorsformat", error);
            workshop->textSubmission.status = json_get_integer(jsonWorkshop, "submissiontypetext", error);
            workshop->textSubmission.wordLimit = MD_NO_WORD_LIMIT;
            workshop->fileSubmission.status = json_get_integer(jsonWorkshop, "submissiontypefile", error);
            if (workshop->fileSubmission.status != MD_SUBMISSION_DISABLED) {
                workshop->fileSubmission.acceptedFileTypes = json_get_string(jsonWorkshop, "submissionfiletypes", error);
                workshop->fileSubmission.maxSubmissionSize = json_get_integer(jsonWorkshop, "maxbytes", error);
                if (workshop->fileSubmission.maxSubmissionSize == 0)
                    workshop->fileSubmission.maxSubmissionSize = client->uploadLimit;
                workshop->fileSubmission.maxUploadedFiles = json_get_integer(jsonWorkshop, "nattachments", error);
            }
        }
    }
}

void md_client_courses_set_mod_resource_data(MDClient *client, MDArray courses, Json *json, MDError *error) {
    Json *jsonResources = json_get_array(json, "resources", error);
    if (!*error) {
        for (int i = 0; i < jsonResources->array.len; ++i) {
            Json *jsonResource = &jsonResources->array.values[i];
            MDModule *module = md_courses_locate_json_module(courses, jsonResource, "coursemodule", error);
            if (*error)
                break;
            module->type = MD_MOD_RESOURCE;
            MDModResource *resource = &module->contents.resource;
            resource->description.text = json_get_string(jsonResource, "intro", error);
            resource->description.format = json_get_integer(jsonResource, "introformat", error);
            Json *jsonFiles = json_get_array(jsonResource, "contentfiles", error);
            if (jsonFiles)
                resource->files = md_parse_files(jsonFiles, error);
        }
    }
}

void md_client_courses_set_mod_url_data(MDClient *client, MDArray courses, Json *json, MDError *error) {
    Json *jsonUrls = json_get_array(json, "urls", error);
    if (!*error) {
        for (int i = 0; i < jsonUrls->array.len; ++i) {
            Json *jsonUrl = &jsonUrls->array.values[i];
            MDModule *module = md_courses_locate_json_module(courses, jsonUrl, "coursemodule", error);
            if (*error)
                break;
            module->type = MD_MOD_URL;
            MDModUrl *url = &module->contents.url;
            url->description.text = json_get_string(jsonUrl, "intro", error);
            url->description.format = json_get_integer(jsonUrl, "introformat", error);
            url->name = json_get_string(jsonUrl, "name", error);
            url->url = json_get_string(jsonUrl, "externalurl", error);
        }
    }
}

void md_client_download_file(MDClient *client, MDFile *file, FILE *stream, MDError *error) {
    *error = MD_ERR_NONE;
    char url[MD_URL_LENGTH];
    snprintf(url, MD_URL_LENGTH, "%s?token=%s", file->url, client->token);
    http_get_request_to_file(url, stream, error);
}

MDLoadedStatus md_courses_load_status(MDClient *client, MDArray courses, MDError *error) {
    *error = MD_ERR_NONE;
    int count = 0;
    for (int i = 0; i < courses.len; ++i) {
        MDCourse *course = &MD_COURSES(courses)[i];
        for (int j = 0; j < course->topics.len; ++j) {
            MDTopic *topic = &MD_TOPICS(course->topics)[j];
            for (int k = 0; k < topic->modules.len; ++k) {
                MDModule *module = &MD_MODULES(topic->modules)[k];
                if (mdModList[module->type].statusParseFunc) {
                    ++count;
                }
            }
        }
    }
    MDLoadedStatus result;
    md_array_init_new(&result.internalReferences, sizeof(MDStatusRef), count, NULL, error);
    char urls[count][MD_URL_LENGTH], *urlArray[count];
    int index = 0;
    if (*error)
        return result;
    for (int i = 0; i < courses.len; ++i) {
        MDCourse *course = &MD_COURSES(courses)[i];
        for (int j = 0; j < course->topics.len; ++j) {
            MDTopic *topic = &MD_TOPICS(course->topics)[j];
            for (int k = 0; k < topic->modules.len; ++k) {
                MDModule *module = &MD_MODULES(topic->modules)[k];
                if (mdModList[module->type].statusParseFunc) {
                    MDStatusRef *statusRef = &MD_ARR(result.internalReferences, MDStatusRef)[index];
                    statusRef->module = module;
                    md_status_ref_init(statusRef);
                    md_client_write_url(client, urls[index], mdModList[module->type].statusWsFunction, "&%s=%d", mdModList[module->type].statusInstanceName, module->instance);
                    urlArray[index] = urls[index];
                    ++index;
                }
            }
        }
    }
    char **data = http_get_multi_request(urlArray, count, error);
    if (!*error) {
        for (int i = 0; i < count && !*error; ++i) {
            // printf("<%s>\n[%s]\n", urls[i], data[i]);
            MDStatusRef *statusRef = &MD_ARR(result.internalReferences, MDStatusRef)[i];
            Json *json = md_parse_moodle_json(data[i], error);
            if (!*error) {
                mdModList[statusRef->module->type].statusParseFunc(json, statusRef, error);
            }
            md_cleanup_json(json);
        }
        for (int i = 0; i < count; ++i) {
            free(data[i]);
        }
        free(data);
    }
    return result;
}

void md_parse_mod_assignment_status_plugins(Json *configs, MDModAssignment *assignment, MDError *error) {
    for (int i = 0; i < configs->array.len && (!*error); ++i) {
        Json *config = &configs->array.values[i];
        cchar *plugin = json_get_string_no_alloc(config, "plugin", error);
        if (*error)
            break;
        if (strcmp(plugin, "file") == 0) {
            cchar *name = json_get_string_no_alloc(config, "name", error);
            cchar *value = json_get_string_no_alloc(config, "value", error);
            if (*error)
                break;
            if (strcmp(name, "enabled") == 0)
                assignment->fileSubmission.status = !atoi(value) ? MD_SUBMISSION_DISABLED : MD_SUBMISSION_REQUIRED;
            else if (strcmp(name, "maxfilesubmissions") == 0)
                assignment->fileSubmission.maxUploadedFiles = atoi(value);
            else if (strcmp(name, "filetypeslist") == 0)
                assignment->fileSubmission.acceptedFileTypes = clone_str(value, error);
            else if (strcmp(name, "maxsubmissionsizebytes") == 0)
                assignment->fileSubmission.maxSubmissionSize = atoll(value);
        } else if (strcmp(plugin, "onlinetext") == 0) {
            cchar *name = json_get_string_no_alloc(config, "name", error);
            cchar *value = json_get_string_no_alloc(config, "value", error);
            if (*error)
                break;
            if (strcmp(name, "enabled") == 0)
                assignment->textSubmission.status = !atoi(value) ? MD_SUBMISSION_DISABLED : MD_SUBMISSION_REQUIRED;
            else if (strcmp(name, "wordlimit") == 0)
                assignment->textSubmission.wordLimit = atoi(value);
        }
    }
}

void md_mod_assign_parse_status(Json *json, MDStatusRef *statusRef, MDError *error) {
    Json *lastAttempt = json_get_object(json, "lastattempt", error);
    if (*error)
        return;
    MDError dummy = MD_ERR_NONE;
    Json *submission = json_get_object(lastAttempt, "submission", &dummy);
    if (dummy) {
        dummy = MD_ERR_NONE;
        // Threat team submissions the same way.
        submission = json_get_object(lastAttempt, "teamsubmission", &dummy);
    }
    if (dummy) {
        // Our best guess.
        statusRef->status.assignment.state = MD_MOD_ASSIGNMENT_STATE_NEW;
        return;
    }
    cchar *state = json_get_string_no_alloc(submission, "status", error);
    if (*error)
        return;
    MDModAssignmentStatus *status = &statusRef->status.assignment;
    status->state = !strcmp(state, "new") ? MD_MOD_ASSIGNMENT_STATE_NEW : MD_MOD_ASSIGNMENT_STATE_SUBMITTED;
    if (status->state != MD_MOD_ASSIGNMENT_STATE_NEW) {
        status->graded = json_get_bool(lastAttempt, "graded", error);
        status->submitDate = json_get_integer(submission, "timecreated", error);
        Json *plugins = json_get_array(submission, "plugins", error);
        for (int i = 0; i < plugins->array.len && !*error; ++i) {
            cchar *type = json_get_string_no_alloc(&plugins->array.values[i], "type", error);

            if (!*error && !strcmp(type, "file")) {

                Json *fileareas = json_get_array(&plugins->array.values[i], "fileareas", error);
                for (int j = 0; j < fileareas->array.len && !*error; ++j) {

                    cchar *area = json_get_string_no_alloc(&fileareas->array.values[j], "area", error);
                    if (!*error && !strcmp(area, "submission_files")) {
                        Json *files = json_get_array(&fileareas->array.values[j], "files", error);
                        if (!*error) {
                            status->submittedFiles = md_parse_files(files, error);
                        }
                    }
                }
            } else if (!*error && !strcmp(type, "onlinetext")) {

                Json *editorfields = json_get_array(&plugins->array.values[i], "editorfields", error);
                for (int j = 0; j < editorfields->array.len && !*error; ++j) {
                    Json *jsonField = &editorfields->array.values[j];
                    cchar *name = json_get_string_no_alloc(jsonField, "name", error);

                    if (!*error && !strcmp(name, "onlinetext")) {
                        status->submittedText.text = json_get_string(jsonField, "text", error);
                        status->submittedText.format = json_get_integer(jsonField, "format", error);
                        break;
                    }
                }
            }
        }
        if (!*error && status->graded) {
            Json *feedback = json_get_object(json, "feedback", error);
            if (!*error) {
                status->grade = json_get_string(feedback, "gradefordisplay", error);
                str_replace(status->grade, "&nbsp;", " ");
                status->gradeDate = json_get_integer(feedback, "gradeddate", error);
            }
        }
    }
}
void md_mod_workshop_parse_status(Json *json, MDStatusRef *statusRef, MDError *error) {
    Json *submissions = json_get_array(json, "submissions", error);
    if (*error)
        return;
    MDModWorkshopStatus *status = &statusRef->status.workshop;
    status->submitted = submissions->array.len > 0;
    if (status->submitted) {
        Json *submission = &submissions->array.values[0];
        status->submitDate = json_get_integer(submission, "timecreated", error);
        status->title = json_get_string(submission, "title", error);
        status->submittedText.text = json_get_string(submission, "content", error);
        status->submittedText.format = json_get_integer(submission, "contentformat", error);
        Json *files = json_get_array(submission, "attachmentfiles", error);
        if (!*error) {
            status->submittedFiles = md_parse_files(files, error);
        }
    }
}

void md_loaded_status_apply(MDLoadedStatus status) {
    for (int i = 0; i < status.internalReferences.len; ++i) {
        MDStatusRef *statusRef = &MD_ARR(status.internalReferences, MDStatusRef)[i];
        switch (statusRef->module->type) {
            case MD_MOD_ASSIGNMENT:
                statusRef->module->contents.assignment.status = statusRef->status.assignment;
                break;

            case MD_MOD_WORKSHOP:
                statusRef->module->contents.workshop.status = statusRef->status.workshop;
                break;

            default:
                break;
        }
    }
}

void md_loaded_status_cleanup(MDLoadedStatus status) {
    md_array_cleanup(&status.internalReferences, sizeof(MDStatusRef), (MDCleanupFunc)md_status_ref_cleanup);
}
