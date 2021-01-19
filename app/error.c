#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static char msg[ERR_MSG_SIZE] = "";
static char errBuff[ERR_MSG_SIZE] = "";
static bool errorHandlingWarningSet = false;

static struct errorMessage {
    Error code;
    cchar *message;
} errorMessages[] = {
    {ERR_NONE, "No error"},
    {ERR_ALLOCATE, "Couldn't allocate memory"},
    {CFG_ERR_GET_ENV, "Couldn't find required environment variables for your system"},
    {CFG_ERR_OPEN_FILE, "Couldn't open config file: %s"},
    {CFG_ERR_NO_VALUE, "No value found for: %s"},
    {CFG_ERR_WRONG_PROPERTY, "No property named %s"},
    {CFG_ERR_NO_TOKEN, "No token found in config file"},
    {UPLOAD_ERR_EXEC, "Couldn't execute upload command: %s"},
    {UPLOAD_ERR_NO_FILES_CHOSEN, "No files files chosen"},
};

void app_error_set_message(cchar *message) {
    snprintf(msg, ERR_MSG_SIZE, "%s", message);
}

cchar *app_error_get_message(Error code) {
    errBuff[0] = 0;
    int size = sizeof(errorMessages) / sizeof(struct errorMessage);
    for (int i = 0; i < size; ++i) {
        if (errorMessages[i].code == code) {
            snprintf(errBuff, ERR_MSG_SIZE, errorMessages[i].message, msg);
            int len = strlen(errBuff);
            if (errorHandlingWarningSet) {
                snprintf(errBuff + len, ERR_MSG_SIZE - len, "\nWARNING: failed error handling!");
            }
            break;
        }
    }
    return errBuff;
}
