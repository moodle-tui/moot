#ifndef __CLIENT_H
#define __CLIENT_H
#include "error.h"
#include <stdbool.h>
#include <time.h>

#define DATE_NONE 0

typedef struct Client {
    char *fullName, *siteName;
    int userid;
    long uploadLimit;
    // private
    char *token, *website;
} Client;

typedef enum {
    MOD_UNSUPPORTED,
    MODULE_ASSIGNMENT = 1,
    MODULE_WORKSHOP = 2,
    MODULE_RESOURCE = 3,
} ModuleType;

typedef enum {
    FORMAT_MOODLE = 0,
    FORMAT_HTML = 1, // Most often used format
    FORMAT_PLAIN = 2,
    FORMAT_MARKDOWN = 4,
} TextFormat;

typedef struct RichText {
    char *text;
    TextFormat format;
} RichText;

typedef struct Attachment {
    char *filename;
    long filesize;
    char *url;
} Attachment;

typedef struct Attachments {
    int len;
    Attachment *data;
} Attachments;

typedef enum SubmissionStatus {
    SUBMISSION_DISABLED = 0,
    SUBMISSION_ENABLED = 1,
    SUBMISSION_REQUIRED = 2,
} SubmissionStatus;

#define NO_FILE_LIMIT -1
typedef struct FileSubmission {
    SubmissionStatus status;
    int maxUploadedFiles;
    long maxSubmissionSize;
    char *acceptedFileTypes;
} FileSubmission;

typedef struct ModAssignment {
    time_t fromDate, dueDate, cutOffDate;
    RichText description;
    Attachments attachments;
    
    FileSubmission fileSubmission;
} ModAssignment;

typedef struct ModWorkshop {
    time_t fromDate, dueDate;
    bool lateSubmissions; // Allows submitting after due date if true.
    RichText description, instructions;

    FileSubmission fileSubmission;
} ModWorkshop;

typedef struct ModResource {
    
} ModResource;

typedef struct Module {
    int id, instance;
    ModuleType type;
    char *name;

    union {
        ModAssignment assignment;
        ModWorkshop workshop;
        ModResource resource;
    } contents;
} Module;

typedef struct Modules {
    int len;
    Module *data;
} Modules;

typedef struct Topic {
    int id;
    char *name;
    RichText summary;
    Modules modules;
} Topic;

typedef struct Topics {
    int len;
    Topic *data;
} Topics;

typedef struct Course {
    int id;
    char *name;
    Topics topics;
} Course;

typedef struct Courses {
    int len;
    Course *data;
} Courses;

Client *mt_new_client(char *token, char *website);
ErrorCode mt_init_client(Client *client);
void mt_destroy_client(Client *client);
Courses mt_get_courses(Client *client, ErrorCode *error);
void mt_free_courses(Courses courses);

void mt_client_mod_assign_submit(Client *client, Module assignment, const char *filenames[], int len, ErrorCode *error);
// title may not be empty
void mt_client_mod_workshop_submit(Client *client, Module workshop, const char *filenames[], int len, const char *title,
                                   ErrorCode *error);

#endif