#ifndef __CLIENT_H
#define __CLIENT_H
#include "error.h"

typedef struct Client {
    char *fullName, *siteName;
    int userid;
    // private
    char *token, *website;
} Client;

typedef enum {
    MOD_UNSUPPORTED,
    MODULE_ASSIGNMENT,
    MODULE_WORKSHOP,
    MODULE_RESOURCE,
} ModuleType;

typedef struct ModAssignment {

} ModAssignment;

typedef struct ModWorkshop {
    /* data */
} ModWorkshop;

typedef struct ModResource {
    /* data */
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
    char *name, *summaryHtml;
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

void mt_client_mod_assign_submit(Client *client, Module assignment, const char *filenames[], int len,
                                 ErrorCode *error);
// title may not be empty
void mt_client_mod_workshop_submit(Client *client, Module workshop, const char *filenames[], int len, const char *title,
                                   ErrorCode *error);

#endif