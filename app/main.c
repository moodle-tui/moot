#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"
#include "config.h"

int main() {
    MDError mdError = MD_ERR_NONE;
    ConfigValues configValues;
    Message msg;
    msgInit(&msg);
    readConfigFile(&configValues, &msg);
    if (msg.msg[0]) {
        printMsgNoUI(msg);
        if (msg.type == MSG_TYPE_ERROR)
            return 0;
    }
    MDArray courses;
    MDClient *client = NULL;
    initialize(&client, &courses, &configValues, &mdError);
    if (mdError) {
        createMsg(&msg, md_error_get_message(mdError), NULL, MSG_TYPE_ERROR);
        printMsgNoUI(msg);
        terminate(client, courses, &msg);
        return 0;
    }

    hidecursor();
    cls();
    mainLoop(courses, client, configValues.uploadCommand);
    cls();
    showcursor();

    terminate(client, courses, &msg);
    return 0;
}

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, MDError *mdError) {
    md_init();
    *client = md_client_new(configValues->token, "https://emokymai.vu.lt", mdError);
    if (*mdError)
        return;

    md_client_init(*client, mdError);
    if (*mdError)
        return;

    *courses = md_client_fetch_courses(*client, 0, mdError);
}

void terminate(MDClient *client, MDArray courses, Message *msg) {
    free(msg->msg);
    md_courses_cleanup(courses);
    md_client_cleanup(client);
    md_cleanup();
}
