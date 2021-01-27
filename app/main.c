/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"
#include "config.h"
#include "internal.h"

int main() {
    ConfigValues configValues;
    Message msg, prevMsg;
    msgInit(&msg);
    msgInit(&prevMsg);
    if (!msg.msg || !prevMsg.msg) {
        printMsgNoUI((Message) {MSG_CANNOT_ALLOCATE, MSG_TYPE_ERROR});
        return 0;
    }
    readConfigFile(&configValues, &msg);
    if (msg.msg[0]) {
        printMsgNoUI(msg);
        if (msg.type == MSG_TYPE_ERROR)
            return 0;
    }
    MDArray courses;
    MDClient *client = NULL;
    initialize(&client, &courses, &configValues, &msg);
    if (msg.type == MSG_TYPE_ERROR) {
        printMsgNoUI(msg);
        terminate(client, courses, &msg, &prevMsg);
        return 0;
    }

    hidecursor();
    cls();
    mainLoop(courses, client, configValues.uploadCommand, &msg, &prevMsg);
    cls();
    showcursor();

    terminate(client, courses, &msg, &prevMsg);
    return 0;
}

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, Message *msg) {
    MDError mdError = MD_ERR_NONE;
    md_init();
    *client = md_client_new(configValues->token, "emokymai.vu.lt", &mdError);
    if (!mdError)
        md_client_init(*client, &mdError);
    if (!mdError)
        md_cleanup_json(md_client_do_http_json_request(*client, &mdError, "core_user_agree_site_policy", ""));
    if (!mdError)
        *courses = md_client_fetch_courses(*client, 0, &mdError);
    if (mdError) {
        createMsg(msg, md_error_get_message(mdError), NULL, MSG_TYPE_ERROR);
        return;
    }
    setHtmlRenders(courses, msg);
}

void terminate(MDClient *client, MDArray courses, Message *msg, Message *prevMsg) {
    free(msg->msg);
    free(prevMsg->msg);
    md_courses_cleanup(courses);
    md_client_cleanup(client);
    md_cleanup();
}
