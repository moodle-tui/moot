/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * https://github.com/moodle-tui/moot
 *
 * Moodle SDK. This is a heavily simplified and minimized Moodle SDK interacting
 * through Moodle webservice api. Built for and tested with moodle 3.1.
 * https://docs.moodle.org/310/en/Web_services
 *
 * An example of use can be found in subfolder test. However, the example there
 * is more focused on testing the library, rather than displaying it's
 * capabilities (e. g. header internal.h should not be included by a user of
 * this library).
 *
 * Another example is the code in folder ui, which is a more complicated example
 * of how this library can be used to make an user interface, developed by
 * Ramojus Lapinskas.
 *
 * This library depends on LIBCURL. Therefore the user of this library is
 * responsible to call curl_global_init(CURL_GLOBAL_ALL) at the start and (more
 * importantly) curl_global_cleanup().
 */

#ifndef __MOODLE_H
#define __MOODLE_H

#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include "defines.h"

// Before using any of the library functions, one must call md_init to initialize it.
void md_init();

// When done using the library, user should free the resources using md_cleanup.
void md_cleanup();

// Error handling:
// Functions of this library that may fail has a pointer to MDError parameter, which will be used
// to indicate success or failure. It will be set to MD_ERR_NONE = 0 on success, or some error
// otherwise. Error message may be obtained using md_error_get_message.
// The address of the error should always be a valid pointer.

// MDError are the possible errors.
typedef enum MDError {
    MD_ERR_NONE = 0,
    MD_ERR_ALLOC,
    MD_ERR_JSON,
    MD_ERR_MOODLE_EXCEPTION,
    MD_ERR_MISSING_JSON_KEY,
    MD_ERR_INVALID_JSON_VALUE,
    MD_ERR_INVALID_JSON,
    MD_ERR_HTTP_REQUEST_FAIL,
    MD_ERR_CURL_FAIL,
    MD_ERR_FILE_OPERATION,
    MD_ERR_MISUSED_MOODLE_API,
    MD_ERR_MISMACHING_MOODLE_DATA,
    MD_ERR_FAILED_PLUGIN_LOGIN,
    MD_ERR_NO_MATCHING_PLUGIN_FOUND,
    MD_ERR_FAILED_TO_LOAD_PLUGIN,
    MD_ERR_MISSING_PLUGIN_VAR,
    MD_ERR_INVALID_PLUGIN,
} MDError;

// Dynamic arrays:
// MDArray is generic array. When accessing elements, it should be casted using
// macro MD_ARR; E. g.: MDArray numbers = MD_MAKE_ARR(int, 1, 2, 3);
// MD_ARR(numbers, int)[0] == 1; MDArray may be created by client using macros
// MD_MAKE_ARR and MD_MAKE_ARR_LEN. Arrays from these macros can be used to pass
// values to moodle functions, but may not be returned from a function or be
// cleaned up, as these arrays are tied to the current scope.
typedef struct MDArray {
    int len;
    void *_data;
} MDArray;

// MD_ARR casts generic array to specific type array (pointer).
#define MD_ARR(array, type) ((type*)array._data)

// MD_COURSES casts a generic array to MDCourse*.
#define MD_COURSES(array) MD_ARR(array, MDCourse)

// MD_TOPICS casts a generic array to MDTopic*.
#define MD_TOPICS(array) MD_ARR(array, MDTopic)

// MD_MODULES casts a generic array to MDModule*.
#define MD_MODULES(array) MD_ARR(array, MDModule)

// MD_FILES casts a generic array to MDFile*.
#define MD_FILES(array) MD_ARR(array, MDFile)

// MD_MAKE_ARR returns typed array with given elements, tied to the current scope.
#define MD_MAKE_ARR(type, ...) \
    ((MDArray){.len = sizeof((type[]){__VA_ARGS__}) / sizeof(type), ._data = (void *)((type[]){__VA_ARGS__})})

// MD_MAKE_ARR returns typed array with given length and elements, tied to the current scope.
#define MD_MAKE_ARR_LEN(type, length, ...) ((MDArray){.len = length, ._data = (void *)((type[length]){__VA_ARGS__})})

// Macro for zero initializer of a MDArray
#define MD_ARRAY_INITIALIZER {.len = 0, ._data = NULL}

// md_array_append appends an element of given size go give MDArray. Size must
// match the sizes of previous elements, or behaviour is undefined. This
// function will not work with arrays made with MD_MAKE_ARR... macros, because
// the memory there is static.
void md_array_append(MDArray *array, const void *ptr, size_t size, MDError *error);

// md_array_free frees and resets the dinamically allocated array (using
// function md_array_append). Keep in mind that elements are not be freed.
void md_array_free(MDArray *array);

// MD_DATE_NONE is a non existing date.
#define MD_DATE_NONE 0

// MD_NO_FILE_LIMIT means that file count is not limited.
#define MD_NO_FILE_LIMIT -1

// MD_NO_WORD_LIMIT means that word count is not limited.
#define MD_NO_WORD_LIMIT 0

// MDClient represents a single user on moodle. All of the functions are based
// on it. MDClient should not be created manually, instead use md_client_new or
// md_client_load_from_file.
typedef struct MDClient {
    char *fullName, *siteName;
    int userid;
    long uploadLimit;
    char *token, *website;  // private
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_CLIENT    
} MDClient;

// MDModType is the type of a moodle module. MD_MOD_UNSUPPORTED will likely
// never be returned, as unsupported modules are simply skipped when fetching
// courses.
typedef enum MDModType {
    MD_MOD_ASSIGNMENT,
    MD_MOD_WORKSHOP,
    MD_MOD_RESOURCE,
    MD_MOD_URL,
    MD_MOD_UNSUPPORTED,  // Must be the last entry.
} MDModType;

#define MD_MOD_COUNT MD_MOD_UNSUPPORTED

// MDTextFormat contains avaiable moodle text formats. HTML is used ussualy though.
typedef enum MDTextFormat {
    MD_FORMAT_MOODLE = 0,
    MD_FORMAT_HTML = 1,  // Most often used format
    MD_FORMAT_PLAIN = 2,
    MD_FORMAT_MARKDOWN = 4,
} MDTextFormat;

// MDRichText represents the rich text found on moodle pages.
typedef struct MDRichText {
    char *text;
    MDTextFormat format;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_RICH_TEXT    
} MDRichText;

// MDFile represents a file on moodle system that can be downloaded using md_client_download_file.
typedef struct MDFile {
    char *filename;
    long filesize;
    char *url;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_FILE    
} MDFile;

// MDSubmissionStatus denotes the status of a submission, as it may be changed by teachers.
typedef enum MDSubmissionStatus {
    MD_SUBMISSION_DISABLED = 0,
    MD_SUBMISSION_ENABLED = 1,
    MD_SUBMISSION_REQUIRED = 2,
} MDSubmissionStatus;

// MDFileSubmission, if enabled, allows to submit single or multiple files.
typedef struct MDFileSubmission {
    MDSubmissionStatus status;
    int maxUploadedFiles;  // may be equal to MD_NO_FILE_LIMIT.
    long maxSubmissionSize;
    char *acceptedFileTypes;  // may be NULL if file types are not limited.
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_FILE_SUBMISSION    
} MDFileSubmission;

// MDTextSubmission, if enabled, allows to submit single or multiple files.
typedef struct MDTextSubmission {
    MDSubmissionStatus status;
    int wordLimit;  // Equal to MD_NO_WORD_LIMIT if not limited.
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_TEXT_SUBMISSION    
} MDTextSubmission;

// MDModAssignmentState notes the state of an assignment.
typedef enum MDModAssignmentState {
    MD_MOD_ASSIGNMENT_STATE_NEW,
    MD_MOD_ASSIGNMENT_STATE_SUBMITTED,
} MDModAssignmentState;

// MDModAssignmentStatus holds additional information about an assignment, such
// as if the current user has submitted and what grade he received.
typedef struct MDModAssignmentStatus {
    MDModAssignmentState state;
    time_t submitDate, gradeDate;  // May be equal to MD_DATE_NEVER.
    MDArray submittedFiles;        // Array with elements of type MDFile.
    MDRichText submittedText;
    bool graded;
    char *grade;
    // TODO: add or move feedback?
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_ASSIGNMENT_STATUS    
} MDModAssignmentStatus;

// MDModWorkshopStatus holds additional information about a workshop, currently
// the user's submission if one exists.
typedef struct MDModWorkshopStatus {
    bool submitted;
    char *title;
    time_t submitDate;
    MDArray submittedFiles;  // Array with elements of type MDFile.
    MDRichText submittedText;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_WORKSHOP_STATUS    
} MDModWorkshopStatus;

// MDModAssignment represents moodle assignment. Currently it only supports file submission, which
// may be disabled.
typedef struct MDModAssignment {
    time_t fromDate, dueDate, cutOffDate;  // May be equal to MD_DATE_NEVER
    MDRichText description;
    MDArray files;  // Array with elements of type MDFile.

    MDFileSubmission fileSubmission;
    MDTextSubmission textSubmission;

    MDModAssignmentStatus status; // needs to be loaded separately.
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_ASSIGNMENT    
} MDModAssignment;

// MDModWorkshop represents moodle workshop. Currently it only supports file submission, which
// may be disabled.
typedef struct MDModWorkshop {
    time_t fromDate, dueDate;  // May be equal to MD_DATE_NEVER
    bool lateSubmissions;      // Allows submitting after due date if true.
    MDRichText description, instructions;

    MDFileSubmission fileSubmission;
    MDTextSubmission textSubmission;
    MDModWorkshopStatus status; // needs to be loaded separately.
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_WORKSHOP    
} MDModWorkshop;

// MDModResource represents moodle resource. Files from it may be downloaded using md_client_download_file.
typedef struct MDModResource {
    MDRichText description;
    // Array with elements of type MDFile.
    MDArray files;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_RESOURCE    
} MDModResource;

// MDModUrl represents moodle url module.
typedef struct MDModUrl {
    MDRichText description;
    char *name, *url;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MOD_URL    
} MDModUrl;

// MDModule represents a general moodle module that specific module under contents. Modules are contained
// in topics.
typedef struct MDModule {
    int id, instance;
    MDModType type;
    char *name;
    union {
        MDModAssignment assignment;
        MDModWorkshop workshop;
        MDModResource resource;
        MDModUrl url;
    } contents;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_MODULE    
} MDModule;

// MDTopic represents a moodle topic into which courses are divided,
typedef struct MDTopic {
    int id;
    char *name;
    MDRichText summary;
    MDArray modules;  // Array with elements of type MDModule.
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_TOPIC    
} MDTopic;

// MDCourse represents a moodle course which contains topics;
typedef struct MDCourse {
    int id;
    char *name;
    // Array with elements of type MDTopic.
    MDArray topics;
    MD_EXTRA_FIELD
    MD_EXTRA_FIELD_COURSE    
} MDCourse;

// MDLoadedStatus is a type to hold preloaded module statuses using
// md_courses_load_status. The information can be applied to courses array using
// md_loaded_status_apply. This allows to perform status loading in the
// background and then applying it thread-safe way.
typedef struct MDLoadedStatus {
    MDArray internalReferences;
} MDLoadedStatus;

// Functions
//
// Bellow are the functions of this library. All of them are documented, but it
// should be mentioned that every pointer passed to a function bellow must be
// valid and non-NULL pointer, unless stated otherwise. No NULL checks are made
// by the library and this will not be repeated in the decriptions of following
// functions.

// md_loaded_status_apply applies loaded changes.
void md_loaded_status_apply(MDLoadedStatus status);

// md_loaded_status_apply releases resources held by loaded changes.
void md_loaded_status_cleanup(MDLoadedStatus status);

// md_error_get_message returns the error message for an error;
const char *md_error_get_message(MDError error);

// md_client_new creates and returns new MDClient.
MDClient *md_client_new(const char *token, const char *website, MDError *error);

// md_client_init initializes the client, which is required for all further operations.
void md_client_init(MDClient *client, MDError *error);

// md_client_save_to_file can be used to save client to given binary file. Later
// a client can be loaded using md_client_load_from_file. 
void md_client_save_to_file(MDClient *client, const char *filename, MDError *error);

// md_client_load_from_file loads and returns a client from file, previously
// saved using md_client_save_to_file. If the client was initialized before
// saving, this function can be used to bypass the md_client_init function.
MDClient *md_client_load_from_file(const char *filename, MDError *error);

// md_client_cleanup releases all the resources owned by the client.
void md_client_cleanup(MDClient *client);

// md_client_fetch_courses gets all the courses from the moodle server. Returned courses should
// be cleaned up later using md_courses_cleanup.
MDArray md_client_fetch_courses(MDClient *client, bool sortByName, MDError *error);

// md_courses_cleanup releases all the resources owned by the list of courses.
// @param courses MDArray with elements of type MDCourse.
void md_courses_cleanup(MDArray courses);

// md_client_mod_assign_submit submits an assignment module with given files and
// text. text or filename array may be NULL, in which case that part will not be
// submitted.
// @param filenames pointer to MDArray with elements of type const char *.
void md_client_mod_assign_submit(MDClient *client, MDModule *assignment, MDArray *filenames, MDRichText *text, MDError *error);

// md_client_mod_workshop_submit submits a workshop module with given files and
// text. text or filename array may be NULL, in which case that part will not be
// submitted.
// @param filenames MDArray with elements of type const char *
// @param title A string that may not be empty
void md_client_mod_workshop_submit(MDClient *client,
                                   MDModule *workshop,
                                   MDArray *filenames,
                                   MDRichText *text,
                                   const char *title,
                                   MDError *error);

// md_client_download_file downloads the given file to the stream.
void md_client_download_file(MDClient *client, MDFile *file, FILE *stream, MDError *error);

// md_courses_load_status loads statuses of modules in given courses (whether
// it's submitted, graded, etc). The changes needs to be applied to courses
// array using function md_loaded_status_apply. The result of a call to this
// function can be freed using md_loaded_status_cleanup.
MDLoadedStatus md_courses_load_status(MDClient *client, MDArray courses, MDError *error);

// See auth.h for implementing a custom plugin.

// md_auth_load_plugin tries to load a new auth plugin specified by the
// filename. Once loaded, it will be used (if it accepts the website) when
// calling md_auth_login. The caller is responsible to free the resources using
// md_auth_cleanup_plugins.
void md_auth_load_plugin(const char *filename, MDError *error);

// md_auth_cleanup_plugins releases the resources held by all the plugins loaded
// using md_auth_load_plugin.
void md_auth_cleanup_plugins();

// md_auth_login tries to log in with each loaded plugin that supports the given
// website. On success token is returned that the caller is responsible to free.
char *md_auth_login(const char *website, const char *username, const char *password, MDError *error);

#endif
