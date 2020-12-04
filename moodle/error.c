#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#define MESSAGE_SIZE 4096

char msg[MESSAGE_SIZE] = "";
char errBuff[MESSAGE_SIZE] = "";

struct errorMsg{
    ErrorCode code;
    const char *message;
} errorMessages[] = {
    { ERR_NONE, "No error" },
    { ERR_ALLOC, "File not found" },
    { ERR_JSON, "Invalid input" },
    { ERR_MOODLE_EXCEPTION, "Moodle returned exception: %s" },
    { ERR_MISSING_JSON_KEY, "Missing json key" },
    { ERR_INVALID_JSON_VALUE, "Invalid json value" },
    { ERR_INVALID_JSON, "Invalid json stream" },
    { ERR_HTTP_REQUEST_FAIL, "Http request failed: %s" },
    { ERR_CURL_FAIL, "Curl library error" },
    { ERR_FILE_OPERATION, "Unable to perform operation on file %s" },
    { ERR_MISSING_JSON_KEY, "Moodle api c function was used incorretly"}
};

void setErrorMessage(const char *message) {
    snprintf(msg, MESSAGE_SIZE, "%s", message);
}


const char *getError(ErrorCode code) {
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