#ifndef __MT_INTERNAL_H
#define __MT_INTERNAL_H

#include "json.h"
#include "moodle.h"

typedef const char cchar;
#define ENSURE_EMPTY_ERROR(error)            \
    {                                        \
        if (*error != MD_ERR_NONE) {         \
            md_set_error_handling_warning(); \
        }                                    \
    }

// error.c

void md_set_error_message(cchar *message);
void md_set_error_handling_warning();
// util.c

void *md_malloc(size_t size, MDError *error);
void *md_realloc(void *data, size_t size, MDError *error);
char *clone_str(cchar *s, MDError *error);
void http_get_request_to_file(char *url, FILE *stream, MDError *error);
char *http_get_request(char *url, MDError *error);
char **http_get_multi_request(char *urls[], unsigned int size, MDError *error);
char *http_post_file(cchar *url, cchar *filename, cchar *name, MDError *error);
json_value *json_get_property_silent(json_value *json, cchar *key);
MDError json_asign_values(json_value *json, cchar *format, ...);
json_value *json_get_property(json_value *json, cchar *key, json_type type, MDError *error);
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

void md_client_write_url(MDClient *client, char *url, cchar *wsfunction, cchar *format, ...);
void md_client_write_url_varg(MDClient *client, char *url, cchar *wsfunction, cchar *format, va_list args);
json_value *md_client_do_http_json_request(MDClient *client, MDError *error, char *wsfunction, cchar *format, ...);
MDModType md_get_mod_type(cchar *module);
json_value *md_parse_moodle_json(char *data, MDError *error);
long md_client_upload_file(MDClient *client, cchar *filename, long itemId, MDError *error);
long md_client_upload_files(MDClient *client, MDArray filenames, MDError *error);
cchar *md_find_moodle_warnings(json_value *json);

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

void md_load_courses_topics(MDClient *client, MDArray courses, MDError *error);
MDArray md_parse_modules(json_value *json, MDError *error);
MDArray md_parse_topics(json_value *json, MDError *error);
MDArray md_parse_files(json_value *json, MDError *error);
void md_parse_assignment_plugins(json_value *configs, MDModAssignment *assignment, MDError *error);
MDModule *md_locate_courses_module(MDCourse course, int instance, MDError *error);

void md_client_courses_set_mod_assignment_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_workshop_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_resource_data(MDClient *client, MDArray courses, char *data, MDError *error);
void md_client_courses_set_mod_url_data(MDClient *client, MDArray courses, char *data, MDError *error);

#endif