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
// Not null expected
Topics mt_create_topics(json_value *json, ErrorCode *error);
void mt_load_courses_topics(Client *client, Courses courses, ErrorCode *error);
void mt_destroy_client(Client *client);
json_value *mt_parse_moodle_json(char *data, ErrorCode *error);
#define ITEM_ID_NONE 0
long mt_client_upload_file(Client *client, const char *filename, long itemId, ErrorCode *error);
long mt_client_upload_files(Client *client, const char *filenames[], int len, ErrorCode *error);
const char *mt_find_moodle_warnings(json_value *json);
// locates module by instance and sets error if not found;
Module *mt_locate_courses_module(Course course, int instance, ErrorCode *error);
// Parses attachments from json. Json must be type of json_array.
Attachments mt_parse_attachments(json_value *jsonAttachments, ErrorCode *error);
void mt_client_courses_set_mod_assign_data(Client *client, Courses courses, char *data, ErrorCode *error);
void mt_client_courses_set_mod_workshop_data(Client *client, Courses courses, char *data, ErrorCode *error);
// configs must be type of json_aray.
void mt_parse_assignment_plugins(json_value *configs, ModAssignment *assignment, ErrorCode *error);
void mt_free_attachments(Attachments attachments);
void mt_free_rich_text(RichText text);
void mt_free_file_submission(FileSubmission submission);
void mt_free_mod_assignment(ModAssignment assignment);
void mt_free_module(Module module);

ModAssignment mt_create_mod_assignment();
RichText mt_create_rich_text();
Attachments mt_create_attachments();
FileSubmission mt_create_file_submission();

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
        client->uploadLimit = jsonGetInteger(json, "usermaxuploadfilesize", &err);
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
        mt_free_module(modules.data[i]);
    }
    free(modules.data);
}

void mt_free_topics(Topics topics) {
    for (int i = 0; i < topics.len; ++i) {
        free(topics.data[i].name);
        free(topics.data[i].summary.text);
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
        modules.data = (Module *)malloc(json->u.array.length * sizeof(Module));
        for (int i = 0; i < json->u.array.length; ++i) {
            json_value *module = json->u.array.values[i];
            json_value *type = get_by_key(module, "modname");
            ModuleType modtype;
            modules.data[i - skip].type = MOD_UNSUPPORTED;
            if (!type || type->type != json_string ||
                (modtype = mt_get_mod_type(type->u.string.ptr)) == MOD_UNSUPPORTED) {
                ++skip;
                continue;
            }
            modules.data[i - skip].type = modtype;
            if (modtype == MODULE_ASSIGNMENT)
                modules.data[i - skip].contents.assignment = mt_create_mod_assignment();

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

Topics mt_create_topics(json_value *json, ErrorCode *error) {
    Topics topics = {0, NULL};
    if (json->type == json_array) {
        topics.data = (Topic *)malloc(json->u.array.length * sizeof(Topic));
        if (!topics.data) {
            *error = ERR_ALLOC;
        }
        for (int i = 0; i < json->u.array.length && (!*error); ++i) {
            Topic *topic = &topics.data[i];
            topic->modules.len = 0;
            topic->id = jsonGetInteger(json->u.array.values[i], "id", error);
            topic->name = jsonGetString(json->u.array.values[i], "name", error);
            topic->summary.text = jsonGetString(json->u.array.values[i], "summary", error);
            topic->summary.format = jsonGetInteger(json->u.array.values[i], "summaryformat", error);
            json_value *modules = jsonGetArray(json->u.array.values[i], "modules", error);

            ++topics.len;
            if (*error)
                break;
            topics.data[i].modules = mt_create_modules(modules);
        }
    } else {
        *error = ERR_INVALID_JSON_VALUE;
    }
    return topics;
}

void mt_load_courses_topics(Client *client, Courses courses, ErrorCode *error) {
    // TODO: add other mod loading.
    int count = courses.len + 2;
    char urls[count][URL_LENGTH];
    for (int i = 0; i < courses.len; ++i) {
        mt_client_write_url(client, urls[i], "core_course_get_contents", "&courseid=%d", courses.data[i].id);
    }
    mt_client_write_url(client, urls[count - MODULE_ASSIGNMENT], "mod_assign_get_assignments", "");
    mt_client_write_url(client, urls[count - MODULE_WORKSHOP], "mod_workshop_get_workshops_by_courses", "");

    char *urlArray[count];
    for (int i = 0; i < count; ++i)
        urlArray[i] = urls[i];

    char **results = httpMultiRequest(urlArray, count, error);

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
        char *assignmentsData = results[count - MODULE_ASSIGNMENT];
        if (assignmentsData) {
            mt_client_courses_set_mod_assign_data(client, courses, assignmentsData, error);
        }
        char *workshopsData = results[count - MODULE_WORKSHOP];
        if (workshopsData) {
            mt_client_courses_set_mod_workshop_data(client, courses, workshopsData, error);
        }
        for (int i = 0; i < count; ++i)
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

// Parses json and looks for moodle exeption. on success json value needs to be freed.
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
                resultId = jsonGetInteger(json->u.array.values[0], "itemid", error);
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

Module *mt_locate_courses_module(Course course, int instance, ErrorCode *error) {
    // TODO: oprimize for log n lookup
    for (int i = 0; i < course.topics.len; ++i) {
        Topic topic = course.topics.data[i];
        for (int j = 0; j < topic.modules.len; ++j) {
            if (topic.modules.data[j].instance == instance) {
                return &topic.modules.data[j];
            }
        }
    }
    *error = ERR_MISMACHING_MOODLE_DATA;
    return NULL;
}

Attachments mt_parse_attachments(json_value *jsonAttachments, ErrorCode *error) {
    Attachments attachments;
    attachments.data = (Attachment *)malloc(jsonAttachments->u.array.length * sizeof(Attachment));
    attachments.len = 0;
    if (!attachments.data)
        *error = ERR_ALLOC;
    for (int i = 0; i < jsonAttachments->u.array.length && (!*error); ++i) {
        json_value *jsonAttachment = jsonAttachments->u.array.values[i];
        attachments.data[i].filename = jsonGetString(jsonAttachment, "filename", error);
        attachments.data[i].filesize = jsonGetInteger(jsonAttachment, "filesize", error);
        attachments.data[i].url = jsonGetString(jsonAttachment, "fileurl", error);
    }
    return attachments;
}

void mt_free_attachments(Attachments attachments) {
    for (int i = 0; i < attachments.len; ++i) {
        free(attachments.data[i].filename);
        free(attachments.data[i].url);
    }
    free(attachments.data);
    attachments.data = NULL;
    attachments.len = 0;
}

void mt_free_rich_text(RichText text) {
    free(text.text);
    text.text = NULL;
}

void mt_free_file_submission(FileSubmission submission) { free(submission.acceptedFileTypes); }

void mt_free_mod_assignment(ModAssignment assignment) {
    mt_free_rich_text(assignment.description);
    mt_free_file_submission(assignment.fileSubmission);
}

ModAssignment mt_create_mod_assignment() {
    ModAssignment assignment;
    assignment.fromDate = assignment.dueDate = assignment.cutOffDate = DATE_NONE;
    assignment.description = mt_create_rich_text();
    assignment.fileSubmission = mt_create_file_submission();
    return assignment;
}

ModWorkshop mt_create_mod_workshop() {
    ModWorkshop workshop;
    workshop.fromDate = workshop.dueDate = DATE_NONE;
    workshop.description = mt_create_rich_text();
    workshop.instructions = mt_create_rich_text();
    workshop.lateSubmissions = false;
    workshop.fileSubmission = mt_create_file_submission();
    return workshop;
}

RichText mt_create_rich_text() {
    RichText text;
    text.text = NULL;
    text.format = FORMAT_PLAIN;
    return text;
}

Attachments mt_create_attachments() {
    Attachments attachments;
    attachments.len = 0;
    attachments.data = NULL;
    return attachments;
}

FileSubmission mt_create_file_submission() {
    FileSubmission submission;
    submission.status = SUBMISSION_DISABLED;
    submission.maxUploadedFiles = NO_FILE_LIMIT;
    submission.maxSubmissionSize = 0;
    submission.acceptedFileTypes = NULL;
    return submission;
}

void mt_free_module(Module module) {
    free(module.name);
    module.name = NULL;
    switch (module.type) {
    case MODULE_ASSIGNMENT:
        mt_free_mod_assignment(module.contents.assignment);
        break;
    }
}

void mt_parse_assignment_plugins(json_value *configs, ModAssignment *assignment, ErrorCode *error) {
    for (int i = 0; i < configs->u.array.length && (!*error); ++i) {
        json_value *config = configs->u.array.values[i];
        const char *plugin = jsonGetStringNoAlloc(config, "plugin", error);
        if (*error)
            break;
        if (strcmp(plugin, "file") == 0) {
            const char *name = jsonGetStringNoAlloc(config, "name", error);
            const char *value = jsonGetStringNoAlloc(config, "value", error);
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

void mt_client_courses_set_mod_assign_data(Client *client, Courses courses, char *data, ErrorCode *error) {
    json_value *json = mt_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonCourses = jsonGetArray(json, "courses", error);
    if (!*error) {
        for (int i = 0; i < jsonCourses->u.array.length; ++i) {
            json_value *jsonCourse = jsonCourses->u.array.values[i];
            int courseId = jsonGetInteger(jsonCourse, "id", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and assignment courses is the same
                if (courses.data[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                *error = ERR_MISMACHING_MOODLE_DATA;
                break;
            }
            Course course = courses.data[index];
            json_value *jsonAssignments = jsonGetArray(jsonCourse, "assignments", error);
            if (*error)
                break;

            // Parse the actual assignment details.
            for (int j = 0; j < jsonAssignments->u.array.length && (!*error); ++j) {
                json_value *jsonAssignment = jsonAssignments->u.array.values[j];
                int instance = jsonGetInteger(jsonAssignment, "id", error);
                if (*error)
                    break;
                Module *module = mt_locate_courses_module(course, instance, error);
                if (*error)
                    break;
                module->type = MODULE_ASSIGNMENT;
                ModAssignment *assignment = &module->contents.assignment;
                assignment->fromDate = jsonGetInteger(jsonAssignment, "allowsubmissionsfromdate", error);
                assignment->dueDate = jsonGetInteger(jsonAssignment, "duedate", error);
                assignment->cutOffDate = jsonGetInteger(jsonAssignment, "cutoffdate", error);
                assignment->description.text = jsonGetString(jsonAssignment, "intro", error);
                assignment->description.format = jsonGetInteger(jsonAssignment, "introformat", error);
                json_value *jsonAttachments = jsonGetArray(jsonAssignment, "introattachments", error);
                if (jsonAttachments)
                    assignment->attachments = mt_parse_attachments(jsonAttachments, error);
                json_value *configs = jsonGetArray(jsonAssignment, "configs", error);
                if (configs)
                    mt_parse_assignment_plugins(configs, assignment, error);
            }
        }
    }
    json_value_free(json);
}

void mt_client_courses_set_mod_workshop_data(Client *client, Courses courses, char *data, ErrorCode *error) {
    json_value *json = mt_parse_moodle_json(data, error);
    if (*error)
        return;
    json_value *jsonWorkshops = jsonGetArray(json, "workshops", error);
    if (!*error) {
        for (int i = 0; i < jsonWorkshops->u.array.length; ++i) {
            json_value *jsonWorkshop = jsonWorkshops->u.array.values[i];
            int courseId = jsonGetInteger(jsonWorkshop, "course", error);
            if (*error)
                break;
            int index = -1;
            for (int j = 0; j < courses.len && index < 0; ++j) {
                // Hoping that the order of preloaded courses and workshop courses is the same
                if (courses.data[(i + j) % courses.len].id == courseId) {
                    index = (i + j) % courses.len;
                }
            }
            if (index < 0) {
                *error = ERR_MISMACHING_MOODLE_DATA;
                break;
            }
            Course course = courses.data[index];

            int instance = jsonGetInteger(jsonWorkshop, "id", error);
            if (*error)
                break;
            Module *module = mt_locate_courses_module(course, instance, error);
            if (*error)
                break;
            module->type = MODULE_WORKSHOP;
            ModWorkshop *workshop = &module->contents.workshop;
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