#ifndef __MOODLE_H
#define __MOODLE_H

#include <stdio.h>
#include <stdlib.h>
#include "stdbool.h"
#include "error.h"

// MDArray is generic array. When accessing elements, it should be casted using macro MD_ARR;
// E. g.: Array numbers = MD_MAKE_ARR(int, 1, 2, 3); MD_ARR(numbers, int)[0] == 1;
// MDArray may be created by client using macros MD_MAKE_ARR and MD_MAKE_ARR_LEN. Arrays from
// these macros can be used to pass values to moodle functions, but may not be returned
// from a function or be cleaned up, as these arrays are tied to the current scope.
typedef struct MDArray {
    int len;
    void *_data;
} MDArray;

// MD_ARR casts generic array to specific type array (pointer).
#define MD_ARR(array, type) ((type *)array._data)
#define MD_COURSES(array) MD_ARR(array, MDCourse)
#define MD_TOPICS(array) MD_ARR(array, MDTopic)
#define MD_MODULES(array) MD_ARR(array, MDModule)
#define MD_FILES(array) MD_ARR(array, MDFile)

// MD_MAKE_ARR returns typed array with given elements, tied to the current scoope. 
#define MD_MAKE_ARR(type, ...) ((MDArray){.len = sizeof((type[]){__VA_ARGS__}) / sizeof(type), ._data = (void *)((type[]){__VA_ARGS__})})

// MD_MAKE_ARR returns typed array with given length and elements, tied to the current scoope. 
#define MD_MAKE_ARR_LEN(type, length, ...) ((MDArray){.len = length, ._data = (void *)((type[length]){__VA_ARGS__})})

#define DATE_NONE 0

typedef struct MDClient {
    char *fullName, *siteName;
    int userid;
    long uploadLimit;
    // private
    char *token, *website;
} MDClient;

typedef enum {
    MD_MOD_UNSUPPORTED,
    MD_MOD_ASSIGNMENT = 1,
    MD_MODULE_WORKSHOP = 2,
    MD_MODULE_RESOURCE = 3,
} MDModType;

typedef enum {
    MD_FORMAT_MOODLE = 0,
    MD_FORMAT_HTML = 1, // Most often used format
    MD_FORMAT_PLAIN = 2,
    MD_FORMAT_MARKDOWN = 4,
} MDTextFormat;

typedef struct MDRichText {
    char *text;
    MDTextFormat format;
} MDRichText;

typedef struct MDFile {
    char *filename;
    long filesize;
    char *url;
} MDFile;

typedef enum MDSubmissionStatus {
    SUBMISSION_DISABLED = 0,
    SUBMISSION_ENABLED = 1,
    SUBMISSION_REQUIRED = 2,
} MDSubmissionStatus;

#define NO_FILE_LIMIT -1
typedef struct MDFileSubmission {
    MDSubmissionStatus status;
    int maxUploadedFiles;
    long maxSubmissionSize;
    char *acceptedFileTypes;
} MDFileSubmission;

typedef struct MDModAssignment {
    time_t fromDate, dueDate, cutOffDate;
    MDRichText description;
    // Array with elements of type MDFile.
    MDArray files;

    MDFileSubmission fileSubmission;
} MDModAssignment;

typedef struct MDModWorkshop {
    time_t fromDate, dueDate;
    bool lateSubmissions; // Allows submitting after due date if true.
    MDRichText description, instructions;

    MDFileSubmission fileSubmission;
} MDModWorkshop;

typedef struct MDModResource {
    MDRichText description;
    // Array with elements of type MDFile.
    MDArray files;
} MDModResource;

typedef struct MDModule {
    int id, instance;
    MDModType type;
    char *name;

    union {
        MDModAssignment assignment;
        MDModWorkshop workshop;
        MDModResource resource;
    } contents;
} MDModule;

typedef struct MDTopic {
    int id;
    char *name;
    MDRichText summary;
    // Array with elements of type MDModule.
    MDArray modules;
} MDTopic;

typedef struct MDCourse {
    int id;
    char *name;
    // Array with elements of type MDTopic.
    MDArray topics;
} MDCourse;

MDClient *mt_new_client(char *token, char *website);
MDError md_client_init(MDClient *client);
void mt_destroy_client(MDClient *client);
MDArray mt_get_courses(MDClient *client, MDError *error);
void md_courses_cleanup(MDArray courses);


void mt_client_mod_assign_submit(MDClient *client, MDModule *assignment, MDArray filenames, MDError *error);
// title may not be empty
void mt_client_mod_workshop_submit(MDClient *client, MDModule *workshop, MDArray filenames, const char *title,
                                   MDError *error);
void mt_client_download_file(MDClient *client, MDFile *file, FILE *stream, MDError *error);


#endif
