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
#define NO_IDENTIFIER -1
#define NO_ITEM_ID 0

MDClient* md_client_new(char* token, char* website) {
    MDClient* client = (MDClient*)malloc(sizeof(MDClient));
    client->token = clone_str(token);
    client->website = clone_str(website);
    client->fullName = client->siteName = NULL;
    return client;
}

json_value* md_client_do_http_json_request(MDClient* client,
                                           MDError* error,
                                           char* wsfunction,
                                           const char* format,
                                           ...) {
    char url[URL_LENGTH] = "";
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL, client->token,
             PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);

    char* data = httpRequest(url, error);
    if (!data)
        return NULL;
    json_value* json = mt_parse_moodle_json(data, error);
    free(data);
    return json;
}

void mt_client_write_url(MDClient* client, char* url, char* wsfunction, const char* format, ...) {
    snprintf(url, URL_LENGTH, "%s%s?wstoken=%s&%s&wsfunction=%s", client->website, SERVICE_URL, client->token,
             PARAM_JSON, wsfunction);
    va_list args;
    va_start(args, format);
    size_t len = strlen(url);
    vsnprintf(url + len, URL_LENGTH - len, format, args);
    va_end(args);
}

MDError md_client_init(MDClient* client) {
    MDError err = MD_ERR_NONE;
    json_value* json = md_client_do_http_json_request(client, &err, "core_webservice_get_site_info", "");

    if (!err) {
        err = assingJsonValues(json, "ssd", "fullname", &client->fullName, "sitename", &client->siteName, "userid",
                               &client->userid);
        client->uploadLimit = jsonGetInteger(json, "usermaxuploadfilesize", &err);
    }
    json_value_free(json);
    return err;
}

MDArray md_client_fetch_courses(MDClient* client, MDError* error) {
    *error = MD_ERR_NONE;
    json_value* jsonCourses =
        md_client_do_http_json_request(client, error, "core_enrol_get_users_courses", "&userid=%d", client->userid);

    MDArray courses = MD_NEW_ARRAY();
    if (!*error && jsonCourses->type == json_array) {
        md_array_init_new(&courses, sizeof(MDCourse), jsonCourses->u.array.length, (MDInitFunc)md_course_init, error);
        MDCourse* courseArr = MD_ARR(courses, MDCourse);
        if (!*error) {
            courses.len = jsonCourses->u.array.length;

            for (int i = 0; i < courses.len; ++i)
                courseArr[i].topics.len = 0;

            int skip = 0;
            for (int i = 0; i < courses.len && !*error; ++i) {
                json_value* course = jsonCourses->u.array.values[i];
                const char* format = jsonGetStringNoAlloc(course, "format", error);
                // only topics format courses are supported
                if (format && strcmp(format, "topics") != 0) {
                    ++skip;
                    continue;
                }
                courseArr[i - skip].id = jsonGetInteger(course, "id", error);
                courseArr[i - skip].name = jsonGetString(course, "fullname", error);
            }
            courses.len -= skip;
        } else {
            *error = MD_ERR_ALLOC;
        }
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    mt_load_courses_topics(client, courses, error);
    json_value_free(jsonCourses);
    return courses;
}

void md_courses_cleanup(MDArray courses) {
    md_array_cleanup(&courses, sizeof(MDCourse), (MDCleanupFunc)md_course_cleanup);
}

void md_array_init(MDArray* array) {
    array->len = 0;
    array->_data = NULL;
}

void md_course_init(MDCourse* course) {
    course->name = NULL;
    course->id = NO_IDENTIFIER;
    // md_array_init(&course->topics);
}

void md_course_cleanup(MDCourse* course) {
    free(course->name);
    md_array_cleanup(&course->topics, sizeof(MDTopic), (MDCleanupFunc)md_topic_cleanup);
    // md_course_init(course);
}

void md_topic_init(MDTopic* topic) {
    topic->name = NULL;
    topic->id = NO_IDENTIFIER;
    // md_array_init(&topic->modules);
}

void md_topic_cleanup(MDTopic* topic) {
    free(topic->name);
    md_rich_text_cleanup(&topic->summary);
    md_array_cleanup(&topic->modules, sizeof(MDModule), (MDCleanupFunc)md_module_cleanup);
    // md_topic_init(topic);
}

void md_module_init(MDModule* module) {
    module->name = NULL;
    module->id = NO_IDENTIFIER;
    module->instance = NO_IDENTIFIER;
    module->type = MD_MOD_UNSUPPORTED;
}

void md_module_cleanup(MDModule* module) {
    free(module->name);
    switch (module->type) {
        case MD_MOD_ASSIGNMENT:
            md_mod_assignment_cleanup(&module->contents.assignment);
            break;
        case MD_MODULE_WORKSHOP:
            md_mod_workshop_cleanup(&module->contents.workshop);
            break;
        case MD_MODULE_RESOURCE:
            md_mod_resource_cleanup(&module->contents.resource);
            break;
    }
}

MDModType mt_get_mod_type(const char* module) {
    if (!strcmp(module, "assign"))
        return MD_MOD_ASSIGNMENT;
    if (!strcmp(module, "resource"))
        return MD_MODULE_RESOURCE;
    if (!strcmp(module, "workshop"))
        return MD_MODULE_WORKSHOP;
    return MD_MOD_UNSUPPORTED;
}

// not NULL expected
MDArray mt_create_modules(json_value* json, MDError* error) {
    MDArray modulesArr;
    md_array_init(&modulesArr);
    if (json->type == json_array) {
        int skip = 0;
        md_array_init_new(&modulesArr, sizeof(MDModule), json->u.array.length, (MDInitFunc)md_module_init, error);
        MDModule* modules = MD_ARR(modulesArr, MDModule);

        for (int i = 0; i < json->u.array.length && !*error; ++i) {
            json_value* jsonModule = json->u.array.values[i];
            const char* type = jsonGetStringNoAlloc(jsonModule, "modname", error);
            if (!type || (modules[i - skip].type = mt_get_mod_type(type)) == MD_MOD_UNSUPPORTED) {
                ++skip;
                continue;
            }
            switch (modules[i - skip].type) {
                case MD_MOD_ASSIGNMENT:
                    md_mod_assignment_init(&modules[i - skip].contents.assignment);
                    break;
                case MD_MODULE_WORKSHOP:
                    md_mod_workshop_init(&modules[i - skip].contents.workshop);
                    break;
                case MD_MODULE_RESOURCE:
                    md_mod_resource_init(&modules[i - skip].contents.resource);
                    break;
            }

            json_value* name = json_get_by_key(jsonModule, "name");
            if (name && name->type == json_string)
                modules[i - skip].name = clone_str(name->u.string.ptr);

            json_value* id = json_get_by_key(jsonModule, "id");
            if (id && id->type == json_integer)
                modules[i - skip].id = id->u.integer;

            json_value* instance = json_get_by_key(jsonModule, "instance");
            if (instance && instance->type == json_integer)
                modules[i - skip].instance = instance->u.integer;
        }
        modulesArr.len -= skip;
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    return modulesArr;
}

MDArray mt_parse_topics(json_value* json, MDError* error) {
    MDArray topicArr;
    md_array_init(&topicArr);
    if (json->type == json_array) {
        md_array_init_new(&topicArr, sizeof(MDTopic), json->u.array.length, (MDInitFunc)md_topic_init, error);
        for (int i = 0; i < json->u.array.length && (!*error); ++i) {
            MDTopic* topic = &MD_ARR(topicArr, MDTopic)[i];
            topic->id = jsonGetInteger(json->u.array.values[i], "id", error);
            topic->name = jsonGetString(json->u.array.values[i], "name", error);
            topic->summary.text = jsonGetString(json->u.array.values[i], "summary", error);
            topic->summary.format = jsonGetInteger(json->u.array.values[i], "summaryformat", error);
            json_value* modules = jsonGetArray(json->u.array.values[i], "modules", error);

            if (*error)
                break;
            MD_ARR(topicArr, MDTopic)[i].modules = mt_create_modules(modules, error);
        }
    } else {
        *error = MD_ERR_INVALID_JSON_VALUE;
    }
    return topicArr;
}

void mt_load_courses_topics(MDClient* client, MDArray courses, MDError* error) {
    // TODO: add other mod loading.
    int count = courses.len + 3;
    char urls[count][URL_LENGTH];
    for (int i = 0; i < courses.len; ++i) {
        mt_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d",
                            MD_ARR(courses, MDCourse)[i].id);
    }
    mt_client_write_url(client, urls[count - MD_MOD_ASSIGNMENT], "mod_assign_get_assignments", "");
    mt_client_write_url(client, urls[count - MD_MODULE_WORKSHOP], "mod_workshop_get_workshops_by_courses", "");
    mt_client_write_url(client, urls[count - MD_MODULE_RESOURCE], "mod_resource_get_resources_by_courses", "");

    char* urlArray[count];
    for (int i = 0; i < count; ++i)
        urlArray[i] = urls[i];

    char** results = httpMultiRequest(urlArray, count, error);

    if (results) {
        for (int i = 0; i < courses.len; ++i) {
            if (results[i]) {
                MDError err = MD_ERR_NONE;
                json_value* topics = mt_parse_moodle_json(results[i], &err);
                if (!err) {
                    MD_ARR(courses, MDCourse)[i].topics = mt_parse_topics(topics, &err);
                    if (err)
                        *error = err;
                    json_value_free(topics);
                } else {
                    *error = err;
                }
            }
        }
        char* assignmentsData = results[count - MD_MOD_ASSIGNMENT];
        if (assignmentsData) {
            mt_client_courses_set_mod_assign_data(client, courses, assignmentsData, error);
        }
        char* workshopsData = results[count - MD_MODULE_WORKSHOP];
        if (workshopsData) {
            mt_client_courses_set_mod_workshop_data(client, courses, workshopsData, error);
        }
        char* resourcesData = results[count - MD_MODULE_RESOURCE];
        if (resourcesData) {
            mt_client_courses_set_mod_resource_data(client, courses, resourcesData, error);
        }
        for (int i = 0; i < count; ++i)
            free(results[i]);
    }
    free(results);
}

void md_client_destroy(MDClient* client) {
    free(client->token);
    free(client->website);
    free(client->fullName);
    free(client->siteName);
    free(client);
}

// Parses json and looks for moodle exeption. on success json value needs to be freed.
json_value* mt_parse_moodle_json(char* data, MDError* error) {
    json_value* json = json_parse(data, strlen(data));
    if (json) {
        if (json_get_by_key(json, "exception")) {
            json_value* msg = json_get_by_key(json, "message");
            if (msg && msg->type == json_string)
                md_set_error_message(msg->u.string.ptr);
            json_value_free(json);
            json = NULL;
            *error = MD_ERR_MOODLE_EXCEPTION;
        }
    } else {
        *error = MD_ERR_INVALID_JSON;
    }
    return json;
}

long mt_client_upload_file(MDClient* client, const char* filename, long itemId, MDError* error) {
    *error = MD_ERR_NONE;
    char url[URL_LENGTH];
    long resultId = 0;
    sprintf(url,
            "%s%s"
            "?token=%s"
            "&itemid=%d",
            client->website, UPLOAD_URL, client->token, itemId);
    char* data = httpPostFile(url, filename, "file_box", error);
    if (!*error) {
        json_value* json = mt_parse_moodle_json(data, error);
        if (!*error) {
            if (json->type == json_array && json->u.array.length > 0)
                resultId = jsonGetInteger(json->u.array.values[0], "itemid", error);
            else
                *error = MD_ERR_INVALID_JSON_VALUE;
        }
        json_value_free(json);
    }
    free(data);
    return resultId;
}

long mt_client_upload_files(MDClient* client, MDArray filenames, MDError* error) {
    *error = MD_ERR_NONE;
    const char** files = MD_ARR(filenames, const char*);
    long itemId = mt_client_upload_file(client, files[0], NO_ITEM_ID, error);
    if (!*error) {
        for (int i = 1; i < filenames.len; ++i) {
            mt_client_upload_file(client, files[i], itemId, error);
            if (*error)
                break;
        }
    }
    return itemId;
}

const char* mt_find_moodle_warnings(json_value* json) {
    json_value* warnings;
    if (warnings = json_get_by_key(json, "warnings")) {
        json = warnings;
    }
    if (json->type == json_array && json->u.array.length > 0) {
        if (json_get_by_key(json->u.array.values[0], "warningcode")) {
            json_value* message = json_get_by_key(json->u.array.values[0], "message");
            if (message && message->type == json_string)
                return message->u.string.ptr;
        }
    }
    return NULL;
}

void md_client_mod_assign_submit(MDClient* client, MDModule* assignment, MDArray filenames, MDError* error) {
    *error = assignment->type == MD_MOD_ASSIGNMENT ? MD_ERR_NONE : MD_ERR_MISUSED_MOODLE_API;
    if (*error)
        return;
    long itemId = mt_client_upload_files(client, filenames, error);
    if (*error)
        return;
    // ignore submit text and comments for now.
    json_value* json = md_client_do_http_json_request(client, error, "mod_assign_save_submission",
                                                      "&assignmentid=%d"
                                                      "&plugindata[files_filemanager]=%ld"
                                                      "&plugindata[onlinetext_editor][text]="
                                                      "&plugindata[onlinetext_editor][format]=4"
                                                      "&plugindata[onlinetext_editor][itemid]=%ld",
                                                      assignment->instance, itemId, itemId);
    if (!*error) {
        const char* message = mt_find_moodle_warnings(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_set_error_message(message);
        }
    }
    json_value_free(json);
}

void md_client_mod_workshop_submit(MDClient* client,
                                   MDModule* workshop,
                                   MDArray filenames,
                                   const char* title,
                                   MDError* error) {
    *error = workshop->type == MD_MODULE_WORKSHOP ? MD_ERR_NONE : MD_ERR_MISUSED_MOODLE_API;
    if (*error)
        return;
    long itemId = mt_client_upload_files(client, filenames, error);
    if (*error)
        return;

    json_value* json = md_client_do_http_json_request(client, error, "mod_workshop_add_submission",
                                                      "&workshopid=%d"
                                                      "&title=%s"
                                                      "&attachmentsid=%ld",
                                                      workshop->instance, title, itemId);
    if (!*error) {
        const char* message = mt_find_moodle_warnings(json);
        if (message) {
            *error = MD_ERR_MOODLE_EXCEPTION;
            md_set_error_message(message);
        }
    }
    json_value_free(json);
}

MDModule* mt_locate_courses_module(MDCourse course, int instance, MDError* error) {
    // TODO: oprimize for log n lookup
    for (int i = 0; i < course.topics.len; ++i) {
        MDTopic topic = MD_ARR(course.topics, MDTopic)[i];
        for (int j = 0; j < topic.modules.len; ++j) {
            if (MD_ARR(topic.modules, MDModule)[j].instance == instance) {
                return &MD_ARR(topic.modules, MDModule)[j];
            }
        }
    }
    *error = MD_ERR_MISMACHING_MOODLE_DATA;
    return NULL;
}

MDArray mt_parse_files(json_value* jsonFiles, MDError* error) {
    MDArray files;
    md_array_init_new(&files, sizeof(MDFile), jsonFiles->u.array.length, (MDInitFunc)md_file_init, error);
    for (int i = 0; i < jsonFiles->u.array.length && (!*error); ++i) {
        json_value* jsonAttachment = jsonFiles->u.array.values[i];
        MD_ARR(files, MDFile)[i].filename = jsonGetString(jsonAttachment, "filename", error);
        MD_ARR(files, MDFile)[i].filesize = jsonGetInteger(jsonAttachment, "filesize", error);
        MD_ARR(files, MDFile)[i].url = jsonGetString(jsonAttachment, "fileurl", error);
    }
    return files;
}

// void mt_free_rich_text(MDRichText text) {
//     free(text.text);
// }

// void mt_free_file_submission(MDFileSubmission submission) {
//     free(submission.acceptedFileTypes);
// }

// void mt_free_mod_assignment(MDModAssignment assignment) {
//     mt_free_rich_text(assignment.description);
//     mt_free_file_submission(assignment.fileSubmission);
// }

// void mt_free_mod_workshop(MDModWorkshop* workshop) {
//     mt_free_rich_text(workshop->description);
//     mt_free_rich_text(workshop->instructions);
//     mt_free_file_submission(workshop->fileSubmission);
// }

// void mt_free_mod_resource(MDModResource* resource) {
//     mt_free_rich_text(resource->description);
//     mt_free_attachments(resource->files);
// }

void md_mod_assignment_init(MDModAssignment* assignment) {
    assignment->fromDate = assignment->dueDate = assignment->cutOffDate = DATE_NONE;
    md_rich_text_init(&assignment->description);
    md_file_submission_init(&assignment->fileSubmission);
}

void md_mod_assignment_cleanup(MDModAssignment* assignment) {
    md_rich_text_cleanup(&assignment->description);
    md_file_submission_cleanup(&assignment->fileSubmission);
    md_array_cleanup(&assignment->files, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_mod_workshop_init(MDModWorkshop* workshop) {
    workshop->fromDate = workshop->dueDate = DATE_NONE;
    md_rich_text_init(&workshop->description);
    md_rich_text_init(&workshop->instructions);
    workshop->lateSubmissions = false;
    md_file_submission_init(&workshop->fileSubmission);
}

void md_mod_workshop_cleanup(MDModWorkshop* workshop) {
    md_rich_text_cleanup(&workshop->description);
    md_rich_text_cleanup(&workshop->instructions);
    md_file_submission_cleanup(&workshop->fileSubmission);
}

void md_mod_resource_init(MDModResource* resource) {
    md_rich_text_init(&resource->description);
    md_array_init(&resource->files);
}

void md_mod_resource_cleanup(MDModResource* resource) {
    md_rich_text_cleanup(&resource->description);
    md_array_cleanup(&resource->files, sizeof(MDFile), (MDCleanupFunc)md_file_cleanup);
}

void md_rich_text_init(MDRichText* richText) {
    richText->text = NULL;
    richText->format = MD_FORMAT_PLAIN;
}

void md_rich_text_cleanup(MDRichText* richText) {
    free(richText->text);
}

void md_file_submission_init(MDFileSubmission* submission) {
    submission->status = SUBMISSION_DISABLED;
    submission->maxUploadedFiles = NO_FILE_LIMIT;
    submission->maxSubmissionSize = 0;
    submission->acceptedFileTypes = NULL;
}

void md_file_submission_cleanup(MDFileSubmission* submission) {
    free(submission->acceptedFileTypes);
}

void md_file_init(MDFile* file) {
    file->filename = NULL;
    file->url = NULL;
    file->filesize = 0;
}

void md_file_cleanup(MDFile* file) {
    free(file->filename);
    free(file->url);
}

void mt_parse_assignment_plugins(json_value* configs, MDModAssignment* assignment, MDError* error) {
    for (int i = 0; i < configs->u.array.length && (!*error); ++i) {
        json_value* config = configs->u.array.values[i];
        const char* plugin = jsonGetStringNoAlloc(config, "plugin", error);
        if (*error)
            break;
        if (strcmp(plugin, "file") == 0) {
            const char* name = jsonGetStringNoAlloc(config, "name", error);
            const char* value = jsonGetStringNoAlloc(config, "value", error);
            if (*error)
                break;
            if (strcmp(name, "enabled") == 0)
                assignment->fileSubmission.status = !atoi(value) ? SUBMISSION_DISABLED : SUBMISSION_REQUIRED;
            else if (strcmp(name, "maxfilesubmissions") == 0)
                assignment->fileSubmission.maxUploadedFiles = atoi(value);
            else if (strcmp(name, "filetypeslist") == 0)
                assignment->fileSubmission.acceptedFileTypes = cloneStrErr(value, error);
            else if (strcmp(name, "maxsubmissionsizebytes") == 0)
                assignment->fileSubmission.maxSubmissionSize = atoll(value);
        }
    }
}

void mt_client_courses_set_mod_assign_data(MDClient* client, MDArray courses, char* data, MDError* error) {
    json_value* json = mt_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value* jsonCourses = jsonGetArray(json, "courses", error);
    if (!*error) {
        for (int i = 0; i < jsonCourses->u.array.length; ++i) {
            json_value* jsonCourse = jsonCourses->u.array.values[i];
            int courseId = jsonGetInteger(jsonCourse, "id", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and assignment courses is the same
                if (MD_ARR(courses, MDCourse)[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                *error = MD_ERR_MISMACHING_MOODLE_DATA;
                break;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];
            json_value* jsonAssignments = jsonGetArray(jsonCourse, "assignments", error);
            if (*error)
                break;

            // Parse the actual assignment details.
            for (int j = 0; j < jsonAssignments->u.array.length && (!*error); ++j) {
                json_value* jsonAssignment = jsonAssignments->u.array.values[j];
                int instance = jsonGetInteger(jsonAssignment, "id", error);
                if (*error)
                    break;
                MDModule* module = mt_locate_courses_module(course, instance, error);
                if (*error)
                    break;
                module->type = MD_MOD_ASSIGNMENT;
                MDModAssignment* assignment = &module->contents.assignment;
                assignment->fromDate = jsonGetInteger(jsonAssignment, "allowsubmissionsfromdate", error);
                assignment->dueDate = jsonGetInteger(jsonAssignment, "duedate", error);
                assignment->cutOffDate = jsonGetInteger(jsonAssignment, "cutoffdate", error);
                assignment->description.text = jsonGetString(jsonAssignment, "intro", error);
                assignment->description.format = jsonGetInteger(jsonAssignment, "introformat", error);
                json_value* jsonAttachments = jsonGetArray(jsonAssignment, "introattachments", error);
                if (jsonAttachments)
                    assignment->files = mt_parse_files(jsonAttachments, error);
                json_value* configs = jsonGetArray(jsonAssignment, "configs", error);
                if (configs)
                    mt_parse_assignment_plugins(configs, assignment, error);
            }
        }
    }
    json_value_free(json);
}

void mt_client_courses_set_mod_workshop_data(MDClient* client, MDArray courses, char* data, MDError* error) {
    json_value* json = mt_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value* jsonWorkshops = jsonGetArray(json, "workshops", error);
    if (!*error) {
        for (int i = 0; i < jsonWorkshops->u.array.length; ++i) {
            json_value* jsonWorkshop = jsonWorkshops->u.array.values[i];
            int courseId = jsonGetInteger(jsonWorkshop, "course", error);
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
                *error = MD_ERR_MISMACHING_MOODLE_DATA;
                break;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];

            int instance = jsonGetInteger(jsonWorkshop, "id", error);
            if (*error)
                break;
            MDModule* module = mt_locate_courses_module(course, instance, error);
            if (*error)
                break;
            module->type = MD_MODULE_WORKSHOP;
            MDModWorkshop* workshop = &module->contents.workshop;
            workshop->fromDate = jsonGetInteger(jsonWorkshop, "submissionstart", error);
            workshop->dueDate = jsonGetInteger(jsonWorkshop, "submissionend", error);
            workshop->lateSubmissions = jsonGetBool(jsonWorkshop, "latesubmissions", error);
            workshop->description.text = jsonGetString(jsonWorkshop, "intro", error);
            workshop->description.format = jsonGetInteger(jsonWorkshop, "introformat", error);
            workshop->instructions.text = jsonGetString(jsonWorkshop, "instructauthors", error);
            workshop->instructions.format = jsonGetInteger(jsonWorkshop, "instructauthorsformat", error);
            workshop->fileSubmission.status = jsonGetInteger(jsonWorkshop, "submissiontypefile", error);
            if (workshop->fileSubmission.status != SUBMISSION_DISABLED) {
                workshop->fileSubmission.acceptedFileTypes = jsonGetString(jsonWorkshop, "submissionfiletypes", error);
                workshop->fileSubmission.maxSubmissionSize = jsonGetInteger(jsonWorkshop, "maxbytes", error);
                if (workshop->fileSubmission.maxSubmissionSize == 0)
                    workshop->fileSubmission.maxSubmissionSize = client->uploadLimit;
                workshop->fileSubmission.maxUploadedFiles = jsonGetInteger(jsonWorkshop, "nattachments", error);
            }
        }
    }
    json_value_free(json);
}

void mt_client_courses_set_mod_resource_data(MDClient* client, MDArray courses, char* data, MDError* error) {
    json_value* json = mt_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value* jsonResources = jsonGetArray(json, "resources", error);
    if (!*error) {
        for (int i = 0; i < jsonResources->u.array.length; ++i) {
            json_value* jsonResource = jsonResources->u.array.values[i];
            int courseId = jsonGetInteger(jsonResource, "course", error);
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
                *error = MD_ERR_MISMACHING_MOODLE_DATA;
                break;
            }
            MDCourse course = MD_ARR(courses, MDCourse)[index];

            int instance = jsonGetInteger(jsonResource, "id", error);
            if (*error)
                break;
            MDModule* module = mt_locate_courses_module(course, instance, error);
            if (*error)
                break;
            module->type = MD_MODULE_RESOURCE;
            MDModResource* resource = &module->contents.resource;
            resource->description.text = jsonGetString(jsonResource, "intro", error);
            resource->description.format = jsonGetInteger(jsonResource, "introformat", error);
            json_value* jsonAttachments = jsonGetArray(jsonResource, "contentfiles", error);
            if (jsonAttachments)
                resource->files = mt_parse_files(jsonAttachments, error);
        }
    }
    json_value_free(json);
}

void md_client_download_file(MDClient* client, MDFile* file, FILE* stream, MDError* error) {
    *error = MD_ERR_NONE;
    char url[URL_LENGTH];
    snprintf(url, URL_LENGTH, "%s?token=%s", file->url, client->token);
    http_request_to_file(url, stream, error);
}