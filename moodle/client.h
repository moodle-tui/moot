#ifndef __CLIENT_H
#define __CLIENT_H

enum {
    ERR_NONE,
    ERR_ALLOC,
    ERR_JSON,
    ERR_MOODLE,
    ERR_RESULT,
};

typedef struct Client {
    char *fullName, *siteName;
    char *token, *website;
} Client;

Client *mt_new_client(char *token, char *website);
int mt_init_client(Client *client);
void mt_destroy_client(Client *client);

#endif