#ifndef __CLIENT_H
#define __CLIENT_H

enum {
    ERR_NONE,
    ERR_ALLOC,
    ERR_JSON,
    ERR_MOODLE,
    ERR_RESULT,
} ErrorCode;

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
} moduleType;

typedef struct Module {
    int id, instance;
    moduleType type;
    char *name;
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
int mt_init_client(Client *client);
void mt_destroy_client(Client *client);
Courses mt_get_courses(Client *client);
void mt_free_courses(Courses courses);

#endif