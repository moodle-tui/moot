#ifndef __MT_INTERNAL_H
#define __MT_INTERNAL_H

/*
 * Copyright (C) 2020 Nojus Gudinavičius
 * nojus.gudinavicius@gmail.com
 * https://github.com/moodle-tui/moot
 */

#include <stdarg.h>
#include "json.h"
#include "moodle.h"

typedef const char cchar;

// This macros sets faulted error flag if the error is not NONE. It's used to
// make sure that actions are terminated in time and errors aren't overriden.
#define ENSURE_EMPTY_ERROR(error)            \
    {                                        \
        if (*error != MD_ERR_NONE) {         \
            md_set_error_handling_warning(); \
        }                                    \
    }

// error.c

// md_error_set_message sets the additional information for message obtained via
// md_error_get_message.
void md_error_set_message(cchar *message);

// md_set_error_handling_warning sets a warning about faulty error handling (see
// ENSURE_EMPTY_ERROR).
void md_set_error_handling_warning();

// util.c

// struct to temporarily hold data while performing http request.
struct Memblock {
    char *memory;
    size_t size;
    MDError *error;
};

// md_malloc allocates allocates memory and sets error on fail.
void *md_malloc(size_t size, MDError *error);

// md_realloc reallocates allocates memory and sets error on fail.
void *md_realloc(void *data, size_t size, MDError *error);

// clone_str returns allocated a copy of provided string.
char *clone_str(cchar *s, MDError *error);

// http_get_request_to_file makes http get request to stream and writes data to
// given stream.
void http_get_request_to_file(char *url, FILE *stream, MDError *error);

// http_get_request makes get request and returns received data, for which
// caller is responsible to free.
char *http_get_request(char *url, MDError *error);

// http_get_multi_request makes multiple http requests at once. Each element of
// the returned 2D array and the array itself needs to be freed by the caller. 
char **http_get_multi_request(char *urls[], unsigned int size, MDError *error);

// http_post_file posts a file specified by the filename to the given url. 
// @param name multipart field name of the field with file contents
// @return response data, that the caller is responsible to free.
char *http_post_file(cchar *url, cchar *filename, cchar *name, MDError *error);

// json_get_property_silent finds and returns a property of json object if
// found, NULL otherwise.
json_value *json_get_property_silent(json_value *json, cchar *key);

// json_get_property finds and returns a property of json object with specific
// type.
json_value *json_get_property(json_value *json, cchar *key, json_type type, MDError *error);


// json getters for specific types:

long json_get_integer(json_value *json, cchar *key, MDError *error);
int json_get_bool(json_value *json, cchar *key, MDError *error);
char *json_get_string(json_value *json, cchar *key, MDError *error);
cchar *json_get_string_no_alloc(json_value *json, cchar *key, MDError *error);
json_value *json_get_array(json_value *json, cchar *key, MDError *error);

// client.c

// MDInitFunc is the callback called by md_array_init_new for each created element.
typedef void (*MDInitFunc)(void *, MDError *);

// MDInitFunc is the callback called by md_array_cleanup for each created element.
typedef void (*MDCleanupFunc)(void *);

// md_array_init_new initializes given array, optionaly calling callback for each created element
// (if callback isn't NULL). The array must be cleaned later up using md_array_cleanup.
void md_array_init_new(MDArray *array, size_t size, int length, MDInitFunc callback, MDError *error);

// md_array_cleanup frees and resets given array, optionaly calling callback for each array element
// (if callback isn't NULL).
void md_array_cleanup(MDArray *array, size_t size, MDCleanupFunc callback);

// md_client_write_url formats url for Moodle webservice request. 
void md_client_write_url(MDClient *client, char *url, cchar *wsfunction, cchar *format, ...);

// same as md_client_write_url, but with va_list.
void md_client_write_url_varg(MDClient *client, char *url, cchar *wsfunction, cchar *format, va_list args);

// md_client_do_http_json_request makes a request to Moodle webservice to
// specific function, caching Moodle exceptions.
// @return json result, must be freed by the caller.
json_value *md_client_do_http_json_request(MDClient *client, MDError *error, char *wsfunction, cchar *format, ...);

// md_get_mod_type returns module type from Moodle module name.
MDModType md_get_mod_type(cchar *module);

// md_parse_moodle_json parses json, looking for Moodle exceptions.
// @return json result, must be freed by the caller.
json_value *md_parse_moodle_json(char *data, MDError *error);

// md_client_upload_file uploads a file to Moodle server. If itemId is not
// MD_NO_ITEM_ID, file is uploaded to the same pool as the file specified with
// itemId.
// @return itemId on success, which later can be passed to this function.
long md_client_upload_file(MDClient *client, cchar *filename, long itemId, MDError *error);

// md_client_upload_files uploads multiple files to the same pool.
// @param filenames MSArray with elements of type const char *.
// @return itemId on success, pointing to uploaded files.
long md_client_upload_files(MDClient *client, MDArray filenames, MDError *error);

// md_find_moodle_warning returns the first found warning in the json or NULL; 
cchar *md_find_moodle_warning(json_value *json);


// initialization to default values and cleanup functions for various datatypes:

void md_array_init(MDArray *array);
void md_course_init(MDCourse *course);
void md_course_cleanup(MDCourse *course);
void md_topic_init(MDTopic *topic);
void md_topic_cleanup(MDTopic *topic);
void md_module_init(MDModule *module);
void md_module_cleanup(MDModule *module);
void md_mod_assignment_init(MDModAssignment *assignment);
void md_mod_assignment_cleanup(MDModAssignment *assignment);
void md_mod_workshop_init(MDModWorkshop *workshop);
void md_mod_workshop_cleanup(MDModWorkshop *workshop);
void md_mod_resource_init(MDModResource *resource);
void md_mod_resource_cleanup(MDModResource *resource);
void md_mod_url_init(MDModUrl *url);
void md_mod_url_cleanup(MDModUrl *url);
void md_rich_text_init(MDRichText *richText);
void md_rich_text_cleanup(MDRichText *richText);
void md_file_submission_init(MDFileSubmission *submission);
void md_file_submission_cleanup(MDFileSubmission *submission);
void md_file_init(MDFile *file);
void md_file_cleanup(MDFile *file);

// md_courses_fetch_topic_contents fetches all the data from Moodle server for
// the topics of given courses.
void md_courses_fetch_topic_contents(MDClient *client, MDArray courses, MDError *error);

// md_parse_modules parses and returns modules from given json.
// @return MDArray with elements of type MDModule.
MDArray md_parse_modules(json_value *json, MDError *error);

// md_parse_topics parses and returns topics from given json.
// @return MDArray with elements of type MDTopic.
MDArray md_parse_topics(json_value *json, MDError *error);

// md_parse_files parses and returns files from given json.
// @return MDArray with elements of type MDFile.
MDArray md_parse_files(json_value *json, MDError *error);

// md_parse_mod_assignment_plugins parses settings for mod assignment out of given
// json, as they have different syntax from the rest. 
void md_parse_mod_assignment_plugins(json_value *configs, MDModAssignment *assignment, MDError *error);

// md_course_locate_module locates and returns pointer to a module from its
// topics with maching instance and id.
MDModule *md_course_locate_module(MDCourse course, int instance, int id, MDError *error);


// for efficiency, data for modules is fetched at once and then applied to
// courses using following functions:

void md_client_courses_set_mod_assignment_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_workshop_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_resource_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_url_data(MDClient *client, MDArray courses, char *data, MDError *error);

#endif