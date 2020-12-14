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

MDClient *md_client_new(char *token, char *website, MDError *error) {
    *error = MD_ERR_NONE;
    MDClient *client = (MDClient *)md_malloc(sizeof(MDClient), error);
    if (client) {
        client->token = clone_str(token, error);
        client->website = clone_str(website, error);
        client->fullName = client->siteName = NULL;
    }
    return client;
}

json_value *md_client_do_http_json_request(MDClient *client, MDError *error, char *wsfunction, cchar *format, ...) {
    va_list args;
    va_start(args, format);
    char url[MD_URL_LENGTH];
    md_client_write_url_varg(client, url, wsfunction, format, args);
    va_end(args);

    char *data = http_get_request(url, error);
    if (!data)
        return NULL;
    json_value *json = md_parse_moodle_json(data, error);
    free(data);
    return json;
}

void md_client_write_url_varg(MDClient *client, char *url, cchar *wsfunction, cchar *format, va_list args) {
    snprintf(url, MD_URL_LENGTH, "%s%s?" MD_WSTOKEN "=%s&%s&" MD_WSFUNCTION "=%s", client->website, MD_SERVICE_URL,
             client->token, MD_PARAM_JSON, wsfunction);
    size_t len = strlen(url);
    vsnprintf(url + len, MD_URL_LENGTH - len, format, args);
}

void md_client_write_url(MDClient *client, char *url, cchar *wsfunction, cchar *format, ...) {
    va_list args;
    va_start(args, format);
    md_client_write_url_varg(client, url, wsfunction, format, args);
    va_end(args);
}

void md_client_init(MDClient *client, MDError *error) {
    *error = MD_ERR_NONE;
    json_value *json = md_client_do_http_json_request(client, error, "core_webservice_get_site_info", "");
    if (!*error) {
        client->fullName = json_get_string(json, "fullname", error);
        client->siteName = json_get_string(json, "sitename", error);
        client->userid = json_get_integer(json, "userid", error);
        client->uploadLimit = json_get_integer(json, "usermaxuploadfilesize", error);
    }
    json_value_free(json);
}

MDArray md_client_fetch_courses(MDClient *client, MDError *error) {
    *error = MD_ERR_NONE;
    json_value *jsonCourses =
        md_client_do_http_json_request(client, error, "core_enrol_get_users_courses", "&userid=%d", client->userid);

    MDArray courses;
    md_array_init(&courses);
    if (!*error && jsonCourses->type == json_array) {
        md_array_init_new(&courses, sizeof(MDCourse), jsonCourses->u.array.length, (MDInitFunc)md_course_init, error);
        MDCourse *courseArr = MD_ARR(courses, MDCourse);
        if (!*error) {
            courses.len = jsonCourses->u.array.length;

            for (int i = 0; i < courses.len; ++i)
                courseArr[i].topics.len = 0;

            int skip = 0;
            for (int i = 0; i < courses.len && !*error; ++i) {
                json_value *course = jsonCourses->u.array.values[i];
                cchar *format = json_get_string_no_alloc(course, "format", error);
                // only topics or weeks format courses are supported
                if (format && strcmp(format, "topics") && strcmp(format, "weeks")) {
                    ++skip;
                    continue;
                }
                courseArr[i - skip].id = json_get_integer(course, "id", error);
                courseArr[i - skip].name = json_get_string(course, "fullname", error);
            }
            courses.len -= skip;
        } else {
            *error = MD_ERR_ALLOC;
        }
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    md_courses_fetch_topic_contents(client, courses, error);
    json_value_free(jsonCourses);
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

void md_array_init(MDArray *array) {
    array->len = 0;
    array->_data = NULL;
}

void md_course_init(MDCourse *course) {
    course->name = NULL;
    course->id = MD_NO_IDENTIFIER;
    // md_array_init(&course->topics);
}

void md_course_cleanup(MDCourse *course) {
    free(course->name);
    md_array_cleanup(&course->topics, sizeof(MDTopic), (MDCleanupFunc)md_topic_cleanup);
    // md_course_init(course);
}

void md_topic_init(MDTopic *topic) {
    topic->name = NULL;
    topic->id = MD_NO_IDENTIFIER;
    // md_array_init(&topic->modules);
}

void md_topic_cleanup(MDTopic *topic) {
    free(topic->name);
    md_rich_text_cleanup(&topic->summary);
    md_array_cleanup(&topic->modules, sizeof(MDModule), (MDCleanupFunc)md_module_cleanup);
    // md_topic_init(topic);
}

void md_module_init(MDModule *module) {
    module->name = NULL;
    module->id = MD_NO_IDENTIFIER;
    module->instance = MD_NO_IDENTIFIER;
    module->type = MD_MOD_UNSUPPORTED;
}

void md_module_cleanup(MDModule *module) {
    free(module->name);
    switch (module->type) {
        case MD_MOD_ASSIGNMENT:
            md_mod_assignment_cleanup(&module->contents.assignment);
            break;
        case MD_MOD_WORKSHOP:
            md_mod_workshop_cleanup(&module->contents.workshop);
            break;
        case MD_MOD_RESOURCE:
            md_mod_resource_cleanup(&module->contents.resource);
            break;
        case MD_MOD_URL:
            md_mod_url_cleanup(&module->contents.url);
            break;
        default:
            break;
    }
}

void md_mod_assignment_init(MDModAssignment *assignment) {
    assignment->fromDate = assignment->dueDate = assignment->cutOffDate = MD_DATE_NONE;
    md_rich_text_init(&assignment->description);
    md_file_submission_init(&assignment->fileSubmission);
    md_array_init(&assignment->files);
}

void md_mod_assignment_cleanup(MDModAssignment *assignment) {
    md_rich_text_cleanup(&assignment->description);
    md_file_submission_cleanup(&assignment->fileSubmission);
    md_array_cleanup(&assignment->files, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_mod_workshop_init(MDModWorkshop *workshop) {
    workshop->fromDate = workshop->dueDate = MD_DATE_NONE;
    md_rich_text_init(&workshop->description);
    md_rich_text_init(&workshop->instructions);
    workshop->lateSubmissions = false;
    md_file_submission_init(&workshop->fileSubmission);
}

void md_mod_workshop_cleanup(MDModWorkshop *workshop) {
    md_rich_text_cleanup(&workshop->description);
    md_rich_text_cleanup(&workshop->instructions);
    md_file_submission_cleanup(&workshop->fileSubmission);
}

void md_mod_url_init(MDModUrl *url) {
    md_rich_text_init(&url->description);
    url->name = url->url = NULL;
}

void md_mod_url_cleanup(MDModUrl *url) {
    md_rich_text_cleanup(&url->description);
    free(url->name);
    free(url->url);
}

void md_mod_resource_init(MDModResource *resource) {
    md_rich_text_init(&resource->description);
    md_array_init(&resource->files);
}

void md_mod_resource_cleanup(MDModResource *resource) {
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
    if (!strcmp(module, "assign"))
        return MD_MOD_ASSIGNMENT;
    if (!strcmp(module, "resource"))
        return MD_MOD_RESOURCE;
    if (!strcmp(module, "workshop"))
        return MD_MOD_WORKSHOP;
    if (!strcmp(module, "url"))
        return MD_MOD_URL;
    return MD_MOD_UNSUPPORTED;
}

// not NULL expected
MDArray md_parse_modules(json_value *json, MDError *error) {
    MDArray modulesArr;
    md_array_init(&modulesArr);
    if (json->type == json_array) {
        int skip = 0;
        md_array_init_new(&modulesArr, sizeof(MDModule), json->u.array.length, (MDInitFunc)md_module_init, error);
        MDModule *modules = MD_ARR(modulesArr, MDModule);

        for (int i = 0; i < json->u.array.length && !*error; ++i) {
            json_value *jsonModule = json->u.array.values[i];
            cchar *type = json_get_string_no_alloc(jsonModule, "modname", error);
            if (!type || (modules[i - skip].type = md_get_mod_type(type)) == MD_MOD_UNSUPPORTED) {
                ++skip;
                continue;
            }
            bool userVisible = json_get_bool(jsonModule, "uservisible", error);
            if (*error || !userVisible) {
                ++skip;
                continue;
            }
            switch (modules[i - skip].type) {
                case MD_MOD_ASSIGNMENT:
                    md_mod_assignment_init(&modules[i - skip].contents.assignment);
                    break;
                case MD_MOD_WORKSHOP:
                    md_mod_workshop_init(&modules[i - skip].contents.workshop);
                    break;
                case MD_MOD_RESOURCE:
                    md_mod_resource_init(&modules[i - skip].contents.resource);
                    break;
                case MD_MOD_URL:
                    md_mod_url_init(&modules[i - skip].contents.url);
                    break;
                default:
                    break;
            }
            modules[i - skip].name = json_get_string(jsonModule, "name", error);
            modules[i - skip].id = json_get_integer(jsonModule, "id", error);
            modules[i - skip].instance = json_get_integer(jsonModule, "instance", error);
        }
        modulesArr.len -= skip;
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    return modulesArr;
}

MDArray md_parse_topics(json_value *json, MDError *error) {
    MDArray topicArr;
    md_array_init(&topicArr);
    if (json->type == json_array) {
        md_array_init_new(&topicArr, sizeof(MDTopic), json->u.array.length, (MDInitFunc)md_topic_init, error);
        for (int i = 0; i < json->u.array.length && (!*error); ++i) {
            MDTopic *topic = &MD_ARR(topicArr, MDTopic)[i];
            topic->id = json_get_integer(json->u.array.values[i], "id", error);
            topic->name = json_get_string(json->u.array.values[i], "name", error);
            topic->summary.text = json_get_string(json->u.array.values[i], "summary", error);
            topic->summary.format = json_get_integer(json->u.array.values[i], "summaryformat", error);
            json_value *modules = json_get_array(json->u.array.values[i], "modules", error);

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
    // TODO: add other mod loading.
    int count = courses.len + 4;
    char urls[count][MD_URL_LENGTH];
    for (int i = 0; i < courses.len; ++i) {
        md_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d",
                            MD_ARR(courses, MDCourse)[i].id);
    }
    md_client_write_url(client, urls[count - MD_MOD_ASSIGNMENT], "mod_assign_get_assignments", "");
    md_client_write_url(client, urls[count - MD_MOD_WORKSHOP], "mod_workshop_get_workshops_by_courses", "");
    md_client_write_url(client, urls[count - MD_MOD_RESOURCE], "mod_resource_get_resources_by_courses", "");
    md_client_write_url(client, urls[count - MD_MOD_URL], "mod_url_get_urls_by_courses", "");

    char *urlArray[count];
    for (int i = 0; i < count; ++i)
        urlArray[i] = urls[i];

    char **results = http_get_multi_request(urlArray, count, error);

    if (results) {
        for (int i = 0; i < courses.len && (!*error); ++i) {
            if (results[i]) {
                MDError err = MD_ERR_NONE;
                json_value *topics = md_parse_moodle_json(results[i], &err);
                if (!err) {
                    MD_ARR(courses, MDCourse)[i].topics = md_parse_topics(topics, &err);
                    if (err)
                        *error = err;
                    json_value_free(topics);
                } else {
                    *error = err;
                }
            }
        }
        char *assignmentsData = results[count - MD_MOD_ASSIGNMENT];
        if (assignmentsData && !*error) {
            md_client_courses_set_mod_assignment_data(client, courses, assignmentsData, error);
        }
        char *workshopsData = results[count - MD_MOD_WORKSHOP];
        if (workshopsData && !*error) {
            md_client_courses_set_mod_workshop_data(client, courses, workshopsData, error);
        }
        char *resourcesData = results[count - MD_MOD_RESOURCE];
        if (resourcesData && !*error) {
            md_client_courses_set_mod_resource_data(client, courses, resourcesData, error);
        }
        char *urlsData = results[count - MD_MOD_URL];
        if (urlsData && !*error) {
            md_client_courses_set_mod_url_data(client, courses, urlsData, error);
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
json_value *md_parse_moodle_json(char *data, MDError *error) {
    ENSURE_EMPTY_ERROR(error);
    json_value *json = json_parse(data, strlen(data));
    if (json) {
        if (json_get_string_no_alloc(json, "exception", &(MDError){0})) {
            cchar *msg = json_get_string_no_alloc(json, "message", &(MDError){0});
            md_error_set_message(msg ? msg : data);
            *error = MD_ERR_MOODLE_EXCEPTION;
        }
    } else {
        *error = MD_ERR_INVALID_JSON;
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
        json_value *json = md_parse_moodle_json(data, error);
        if (!*error) {
            if (json->type == json_array && json->u.array.length > 0)
                resultId = json_get_integer(json->u.array.values[0], "itemid", error);
            else
                *error = MD_ERR_INVALID_JSON_VALUE;
        }
        json_value_free(json);
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

cchar *md_find_moodle_warning(json_value *json) {
    json_value *warnings;
    if ((warnings = json_get_property_silent(json, "warnings"))) {
        json = warnings;
    }
    if (json->type == json_array && json->u.array.length > 0) {
        if (json_get_property_silent(json->u.array.values[0], "warningcode")) {
            return json_get_string_no_alloc(json->u.array.values[0], "message", &(MDError){0});
        }
    }
    return NULL;
}

void md_client_mod_assign_submit(MDClient *client, MDModule *assignment, MDArray filenames, MDError *error) {
    *error = assignment->type == MD_MOD_ASSIGNMENT ? MD_ERR_NONE : MD_ERR_MISUSED_MOODLE_API;
    if (*error)
        return;
    long itemId = md_client_upload_files(client, filenames, error);
    if (*error)
        return;
    // ignore submit text and comments for now.
    json_value *json = md_client_do_http_json_request(client, error, "mod_assign_save_submission",
                                                      "&assignmentid=%d"
                                                      "&plugindata[files_filemanager]=%ld"
                                                      "&plugindata[onlinetext_editor][text]="
                                                      "&plugindata[onlinetext_editor][format]=4"
                                                      "&plugindata[onlinetext_editor][itemid]=0",
                                                      assignment->instance, itemId);
    if (!*error) {
        cchar *message = md_find_moodle_warning(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_error_set_message(message);
        }
    }
    json_value_free(json);
}

void md_client_mod_workshop_submit(MDClient *client,
                                   MDModule *workshop,
                                   MDArray filenames,
                                   cchar *title,
                                   MDError *error) {
    *error = workshop->type == MD_MOD_WORKSHOP ? MD_ERR_NONE : MD_ERR_MISUSED_MOODLE_API;
    long itemId = md_client_upload_files(client, filenames, error);
    if (*error)
        return;
    char content[2] = {0, 0};
    if (workshop->contents.workshop.textSubmissionRequired)
        content[0] = '-';
    json_value *json = md_client_do_http_json_request(client, error, "mod_workshop_add_submission",
                                                      "&workshopid=%d"
                                                      "&title=%s"
                                                      "&content=%s"
                                                      "&attachmentsid=%ld",
                                                      workshop->instance, title, content, itemId);
    if (!*error) {
        cchar *message = md_find_moodle_warning(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_error_set_message(message);
        }
    }
    json_value_free(json);
}

MDModule *md_course_locate_module(MDCourse course, int instance, int id, MDError *error) {
    // TODO: optimize for log n lookup
    for (int i = 0; i < course.topics.len; ++i) {
        MDTopic topic = MD_ARR(course.topics, MDTopic)[i];
        for (int j = 0; j < topic.modules.len; ++j) {
            MDModule *module = &MD_ARR(topic.modules, MDModule)[j];
            if (module->instance == instance && module->id == id) {
                return module;
            }
        }
    }
    *error = MD_ERR_MISMACHING_MOODLE_DATA;
    return NULL;
}

MDArray md_parse_files(json_value *jsonFiles, MDError *error) {
    MDArray files;
    md_array_init_new(&files, sizeof(MDFile), jsonFiles->u.array.length, (MDInitFunc)md_file_init, error);
    for (int i = 0; i < jsonFiles->u.array.length && (!*error); ++i) {
        json_value *jsonAttachment = jsonFiles->u.array.values[i];
        MD_ARR(files, MDFile)[i].filename = json_get_string(jsonAttachment, "filename", error);
        MD_ARR(files, MDFile)[i].filesize = json_get_integer(jsonAttachment, "filesize", error);
        MD_ARR(files, MDFile)[i].url = json_get_string(jsonAttachment, "fileurl", error);
    }
    return files;
}

void md_parse_mod_assignment_plugins(json_value *configs, MDModAssignment *assignment, MDError *error) {
    for (int i = 0; i < configs->u.array.length && (!*error); ++i) {
        json_value *config = configs->u.array.values[i];
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
        }
    }
}

void md_client_courses_set_mod_assignment_data(MDClient *client, MDArray courses, char *data, MDError *error) {
    json_value *json = md_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonCourses = json_get_array(json, "courses", error);
    if (!*error) {
        for (int i = 0; i < jsonCourses->u.array.length; ++i) {
            json_value *jsonCourse = jsonCourses->u.array.values[i];
            int courseId = json_get_integer(jsonCourse, "id", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and assignment courses is the same;
                if (MD_ARR(courses, MDCourse)[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                // TODO: check if this is actually not a topics course
                continue;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];
            json_value *jsonAssignments = json_get_array(jsonCourse, "assignments", error);
            if (*error)
                break;

            // Parse the actual assignment details.
            for (int j = 0; j < jsonAssignments->u.array.length && (!*error); ++j) {
                json_value *jsonAssignment = jsonAssignments->u.array.values[j];
                int instance = json_get_integer(jsonAssignment, "id", error);
                int moduleId = json_get_integer(jsonAssignment, "cmid", error);
                if (*error)
                    break;
                MDModule *module = md_course_locate_module(course, instance, moduleId, error);
                if (*error)
                    break;
                module->type = MD_MOD_ASSIGNMENT;
                MDModAssignment *assignment = &module->contents.assignment;
                assignment->fromDate = json_get_integer(jsonAssignment, "allowsubmissionsfromdate", error);
                assignment->dueDate = json_get_integer(jsonAssignment, "duedate", error);
                assignment->cutOffDate = json_get_integer(jsonAssignment, "cutoffdate", error);
                assignment->description.text = json_get_string(jsonAssignment, "intro", error);
                assignment->description.format = json_get_integer(jsonAssignment, "introformat", error);
                json_value *jsonAttachments = json_get_array(jsonAssignment, "introattachments", error);
                if (jsonAttachments)
                    assignment->files = md_parse_files(jsonAttachments, error);
                json_value *configs = json_get_array(jsonAssignment, "configs", error);
                if (configs)
                    md_parse_mod_assignment_plugins(configs, assignment, error);
            }
        }
    }
    json_value_free(json);
}

void md_client_courses_set_mod_workshop_data(MDClient *client, MDArray courses, char *data, MDError *error) {
    json_value *json = md_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonWorkshops = json_get_array(json, "workshops", error);
    if (!*error) {
        for (int i = 0; i < jsonWorkshops->u.array.length; ++i) {
            json_value *jsonWorkshop = jsonWorkshops->u.array.values[i];
            int courseId = json_get_integer(jsonWorkshop, "course", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and workshop courses is the same
                if (MD_ARR(courses, MDCourse)[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                // TODO: check if this is actualy not a topics course
                continue;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];

            int instance = json_get_integer(jsonWorkshop, "id", error);
            int moduleId = json_get_integer(jsonWorkshop, "coursemodule", error);
            if (*error)
                break;
            MDModule *module = md_course_locate_module(course, instance, moduleId, error);
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
            workshop->textSubmissionRequired = json_get_integer(jsonWorkshop, "submissiontypetext", error);
            workshop->fileSubmission.status = json_get_integer(jsonWorkshop, "submissiontypefile", error);
            if (workshop->fileSubmission.status != MD_SUBMISSION_DISABLED) {
                workshop->fileSubmission.acceptedFileTypes =
                    json_get_string(jsonWorkshop, "submissionfiletypes", error);
                workshop->fileSubmission.maxSubmissionSize = json_get_integer(jsonWorkshop, "maxbytes", error);
                if (workshop->fileSubmission.maxSubmissionSize == 0)
                    workshop->fileSubmission.maxSubmissionSize = client->uploadLimit;
                workshop->fileSubmission.maxUploadedFiles = json_get_integer(jsonWorkshop, "nattachments", error);
            }
        }
    }
    json_value_free(json);
}

void md_client_courses_set_mod_resource_data(MDClient *client, MDArray courses, char *data, MDError *error) {
    json_value *json = md_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonResources = json_get_array(json, "resources", error);
    if (!*error) {
        for (int i = 0; i < jsonResources->u.array.length; ++i) {
            json_value *jsonResource = jsonResources->u.array.values[i];
            int courseId = json_get_integer(jsonResource, "course", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and workshop courses is the same
                if (MD_ARR(courses, MDCourse)[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                // TODO: check if this is actualy not a topics course
                continue;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];

            int instance = json_get_integer(jsonResource, "id", error);
            int moduleId = json_get_integer(jsonResource, "coursemodule", error);
            if (*error)
                break;
            MDModule *module = md_course_locate_module(course, instance, moduleId, error);
            if (*error)
                break;
            module->type = MD_MOD_RESOURCE;
            MDModResource *resource = &module->contents.resource;
            resource->description.text = json_get_string(jsonResource, "intro", error);
            resource->description.format = json_get_integer(jsonResource, "introformat", error);
            json_value *jsonFiles = json_get_array(jsonResource, "contentfiles", error);
            if (jsonFiles)
                resource->files = md_parse_files(jsonFiles, error);
        }
    }
    json_value_free(json);
}

void md_client_courses_set_mod_url_data(MDClient *client, MDArray courses, char *data, MDError *error) {
    json_value *json = md_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonUrls = json_get_array(json, "urls", error);
    if (!*error) {
        for (int i = 0; i < jsonUrls->u.array.length; ++i) {
            json_value *jsonUrl = jsonUrls->u.array.values[i];
            int courseId = json_get_integer(jsonUrl, "course", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and workshop courses is the same
                if (MD_ARR(courses, MDCourse)[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                // TODO: check if this is actualy not a topics course
                continue;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];

            int instance = json_get_integer(jsonUrl, "id", error);
            int moduleId = json_get_integer(jsonUrl, "coursemodule", error);
            if (*error)
                break;
            MDModule *module = md_course_locate_module(course, instance, moduleId, error);
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
    json_value_free(json);
}

void md_client_download_file(MDClient *client, MDFile *file, FILE *stream, MDError *error) {
    *error = MD_ERR_NONE;
    char url[MD_URL_LENGTH];
    snprintf(url, MD_URL_LENGTH, "%s?token=%s", file->url, client->token);
    http_get_request_to_file(url, stream, error);
}