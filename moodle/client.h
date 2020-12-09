#ifndef __CLIENT_H
#define __CLIENT_H
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

#include "error.h"
#include "moodle.h"
#include "json.h"

MDClient* mt_new_client(char* token, char* website);
void mt_client_write_url(MDClient* client, char* url, char* wsfunction, const char* format, ...);
MDError md_client_init(MDClient* client);
MDArray mt_get_courses(MDClient* client, MDError* error);
void md_array_init(MDArray* array);
void md_course_init(MDCourse* course);
void md_course_cleanup(MDCourse* course);
void md_topic_init(MDTopic* topic);
void md_topic_cleanup(MDTopic* topic);
void md_module_init(MDModule* module);
void md_module_cleanup(MDModule* module);
MDModType mt_get_mod_type(const char* module);
MDArray mt_create_modules(json_value* json, MDError* error);
MDArray mt_parse_topics(json_value* json, MDError* error);
void mt_load_courses_topics(MDClient* client, MDArray courses, MDError* error);
void mt_destroy_client(MDClient* client);
json_value* mt_parse_moodle_json(char* data, MDError* error);
long mt_client_upload_file(MDClient* client, const char* filename, long itemId, MDError* error);
long mt_client_upload_files(MDClient* client, MDArray filenames, MDError* error);
const char* mt_find_moodle_warnings(json_value* json);
void mt_client_mod_assign_submit(MDClient* client, MDModule* assignment, MDArray filenames, MDError* error);
MDModule* mt_locate_courses_module(MDCourse course, int instance, MDError* error);
MDArray mt_parse_files(json_value* jsonFiles, MDError* error);
void mt_free_rich_text(MDRichText text);
void mt_free_file_submission(MDFileSubmission submission);
void mt_free_mod_assignment(MDModAssignment assignment);
void mt_free_mod_workshop(MDModWorkshop* workshop);
void mt_free_mod_resource(MDModResource* resource);
void md_mod_assignment_init(MDModAssignment* assignment);
void md_mod_assignment_cleanup(MDModAssignment* assignment);
void md_mod_workshop_init(MDModWorkshop* workshop);
void md_mod_workshop_cleanup(MDModWorkshop* workshop);
void md_mod_resource_init(MDModResource* resource);
void md_mod_resource_cleanup(MDModResource* resource);
void md_rich_text_init(MDRichText* richText);
void md_rich_text_cleanup(MDRichText* richText);
void md_file_submission_init(MDFileSubmission* submission);
void md_file_submission_cleanup(MDFileSubmission* submission);
void md_file_init(MDFile* file);
void md_file_cleanup(MDFile* file);
void mt_parse_assignment_plugins(json_value* configs, MDModAssignment* assignment, MDError* error);
void mt_client_courses_set_mod_assign_data(MDClient* client, MDArray courses, char* data, MDError* error);
void mt_client_courses_set_mod_workshop_data(MDClient* client, MDArray courses, char* data, MDError* error);
void mt_client_courses_set_mod_resource_data(MDClient* client, MDArray courses, char* data, MDError* error);
void mt_client_download_file(MDClient* client, MDFile* file, FILE* stream, MDError* error);

#endif