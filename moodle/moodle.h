#ifndef __MOODLE_H
#define __MOODLE_H

/*
 * Copyright (C) 2020 Nojus Gudinavičius
 * nojus.gudinavicius@gmail.com
 * https://github.com/moodle-tui/moot
 * 
 * Moodle SDK
 * This is a heavily simplified and minimized Moodle SDK interacting through
 * Moodle webservice api. Built for and tested with moodle 3.1.
 * https://docs.moodle.org/310/en/Web_services
 *
*/


#include <time.h>
#include "stdbool.h"

#define DEBUG(var) \
    printf("DBG: %s = ", #var); \
    printf(_Generic( \
        var, \
        int: "%d", \
        long: "%ld", \
        char *: "%s", \
        const char *: "%s", \
        default: "?" \
    ), var); \
    printf("\n"); \

// MDArray is generic array. When accessing elements, it should be casted using
// macro MD_ARR; E. g.: Array numbers = MD_MAKE_ARR(int, 1, 2, 3);
// MD_ARR(numbers, int)[0] == 1; MDArray may be created by client using macros
// MD_MAKE_ARR and MD_MAKE_ARR_LEN. Arrays from these macros can be used to pass
// values to moodle functions, but may not be returned from a function or be
// cleaned up, as these arrays are tied to the current scope.
typedef struct MDArray {
    int len;
    void *_data;
} MDArray;

// MD_ARR casts generic array to specific type array (pointer).
#define MD_ARR(array, type) ((type *)array._data)

// MD_COURSES casts a generic array to MDCourse*.
#define MD_COURSES(array) MD_ARR(array, MDCourse)

// MD_TOPICS casts a generic array to MDTopic*.
#define MD_TOPICS(array) MD_ARR(array, MDTopic)

// MD_MODULES casts a generic array to MDModule*.
#define MD_MODULES(array) MD_ARR(array, MDModule)

// MD_FILES casts a generic array to MDFile*.
#define MD_FILES(array) MD_ARR(array, MDFile)

// MD_MAKE_ARR returns typed array with given elements, tied to the current scoope.
#define MD_MAKE_ARR(type, ...) \
    ((MDArray){.len = sizeof((type[]){__VA_ARGS__}) / sizeof(type), ._data = (void *)((type[]){__VA_ARGS__})})

// MD_MAKE_ARR returns typed array with given length and elements, tied to the current scoope.
#define MD_MAKE_ARR_LEN(type, length, ...) ((MDArray){.len = length, ._data = (void *)((type[length]){__VA_ARGS__})})

// MD_DATE_NONE is a non existing date.
#define MD_DATE_NONE 0

// MD_NO_FILE_LIMIT means that file count is not limited.
#define MD_NO_FILE_LIMIT -1

// MDClient represents a single user on moodle. All of the functions are based on it.
// MDClient should not be created manually, instead use md_client_new.
typedef struct MDClient {
    char *fullName, *siteName;
    int userid;
    long uploadLimit;
    char *token, *website;  // private
} MDClient;

// MDModType is the type of a moodle module. MD_MOD_UNSUPPORTED will likely
// never be returned, as unsuported modules are simply skipped when fetching
// courses.
typedef enum MDModType {
    MD_MOD_UNSUPPORTED,
    MD_MOD_ASSIGNMENT = 1,
    MD_MOD_WORKSHOP = 2,
    MD_MOD_RESOURCE = 3,
    MD_MOD_URL = 4,
} MDModType;

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
} MDRichText;

// MDFile represents a file on moodle system that can be downloaded using md_client_download_file.
typedef struct MDFile {
    char *filename;
    long filesize;
    char *url;
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
} MDFileSubmission;

// MDModAssignment represents moodle assignment. Currently it only supports file submission, which
// may be disabled.
typedef struct MDModAssignment {
    time_t fromDate, dueDate, cutOffDate;  // May be equal to MD_DATE_NEVER
    MDRichText description;
    MDArray files;  // Array with elements of type MDFile.

    MDFileSubmission fileSubmission;
} MDModAssignment;

// MDModWorkshop represents moodle workshop. Currently it only supports file submission, which
// may be disabled.
typedef struct MDModWorkshop {
    time_t fromDate, dueDate;  // May be equal to MD_DATE_NEVER
    bool lateSubmissions;      // Allows submitting after due date if true.
    MDRichText description, instructions;

    MDFileSubmission fileSubmission;
} MDModWorkshop;

// MDModResource represents moodle resource. Files from it may be downloaded using md_client_download_file.
typedef struct MDModResource {
    MDRichText description;
    // Array with elements of type MDFile.
    MDArray files;
} MDModResource;

// MDModUrl represents moodle url module.
typedef struct MDModUrl {
    MDRichText description;
    char *name, *url;
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
} MDModule;

// MDTopic represents a moodle topic into which courses are divided,
typedef struct MDTopic {
    int id;
    char *name;
    MDRichText summary;
    MDArray modules;  // Array with elements of type MDModule.
} MDTopic;

// MDCourse represents a moodle course which contains topics;
typedef struct MDCourse {
    int id;
    char *name;
    // Array with elements of type MDTopic.
    MDArray topics;
} MDCourse;

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
} MDError;

// md_error_get_message returns the error message for an error;
const char *md_error_get_message(MDError error);

// md_client_new creates and returns new MDClient.
MDClient *md_client_new(char *token, char *website, MDError *error);

// md_client_init initializes the client, which is requered for all further operations.
void md_client_init(MDClient *client, MDError *error);

// md_client_cleanup releases all the resources owned by the client.
void md_client_cleanup(MDClient *client);

// md_client_fetch_courses gets all the courses from the moodle server. Returned courses should
// be cleaned up later using md_courses_cleanup.
MDArray md_client_fetch_courses(MDClient *client, MDError *error);

// md_courses_cleanup releases all the resources owned by the list of courses.
// @param courses MDArray with elements of type MDCourse.
void md_courses_cleanup(MDArray courses);

// md_client_mod_assign_submit submits an assignment module with given files.
// @param filenames MDArray with elements of type const char *
void md_client_mod_assign_submit(MDClient *client, MDModule *assignment, MDArray filenames, MDError *error);

// md_client_mod_workshop_submit submits a workshop module with given files.
// @param filenames MDArray with elements of type const char *
// @param title A string that may not be empty
void md_client_mod_workshop_submit(MDClient *client,
                                   MDModule *workshop,
                                   MDArray filenames,
                                   const char *title,
                                   MDError *error);

// md_client_download_file downloads the given file to the stream.
void md_client_download_file(MDClient *client, MDFile *file, FILE *stream, MDError *error);

#endif
