#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"
#include "config.h"

int main() {
    Error error = ERR_NONE;
    MDError mdError = MD_ERR_NONE;
    ConfigValues configValues;
    readConfigFile(&configValues, &error);
    printErrIfErr(error, mdError);
    if (error)
        return 0;
    MDArray courses;
    MDClient *client = NULL;
    initialize(&client, &courses, &configValues, &error, &mdError);
    if (error || mdError) {
        printErrIfErr(error, mdError);
        terminate(client, courses);
        return 0;
    }

    hidecursor();
    cls();
    mainLoop(courses, client, configValues.uploadCommand, &error, &mdError);
    cls();
    showcursor();
    printErrIfErr(error, mdError);

    terminate(client, courses);
    return 0;
}

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, Error *error, MDError *mdError) {
    md_init();
    *client = md_client_new(configValues->token, "https://emokymai.vu.lt", mdError);
    if (*mdError)
        return;

    md_client_init(*client, mdError);
    if (*mdError)
        return;

    *courses = md_client_fetch_courses(*client, 0, mdError);
}

void printErrIfErr(Error error, MDError mdError) {
    if (error)
        msgErr(app_error_get_message(error));
    else if (mdError)
        msgErr(md_error_get_message(mdError));
}

void terminate(MDClient *client, MDArray courses) {
    md_courses_cleanup(courses);
    md_client_cleanup(client);
    md_cleanup();
}
