#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#define MESSAGE_SIZE 4096

char msg[MESSAGE_SIZE] = "";
char errBuff[MESSAGE_SIZE] = "";

struct errorMsg{
    MDError code;
    const char *message;
} errorMessages[] = {
    { MD_ERR_NONE, "No error" },
    { MD_ERR_ALLOC, "File not found" },
    { MD_ERR_JSON, "Invalid input" },
    { MD_ERR_MOODLE_EXCEPTION, "Moodle returned exception: %s" },
    { MD_ERR_MISSING_JSON_KEY, "Missing json key: %s" },
    { MD_ERR_INVALID_JSON_VALUE, "Invalid json value" },
    { MD_ERR_INVALID_JSON, "Invalid json stream" },
    { MD_ERR_HTTP_REQUEST_FAIL, "Http request failed: %s" },
    { MD_ERR_CURL_FAIL, "Curl library error" },
    { MD_ERR_FILE_OPERATION, "Unable to perform operation on file %s" },
    { MD_ERR_MISUSED_MOODLE_API, "Moodle api c function was used incorretly"}
};

void md_set_error_message(const char *message) {
    snprintf(msg, MESSAGE_SIZE, "%s", message);
}


const char *md_error_get_message(MDError code) {
    errBuff[0] = 0;
    int size = sizeof(errorMessages) / sizeof(struct errorMsg);
    for (int i = 0; i < size; ++i) {
        if (errorMessages[i].code == code) {
            snprintf(errBuff, MESSAGE_SIZE, errorMessages[i].message, msg);
            break;
        }
    }
    return errBuff;
}