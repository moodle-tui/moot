#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"
#include "moodle.h"

#define MESSAGE_SIZE 4096

char msg[MESSAGE_SIZE] = "";
char errBuff[MESSAGE_SIZE] = "";
bool errorHandlingWarningSet = false;

struct errorMsg {
    MDError code;
    cchar *message;
} errorMessages[] = {
    {MD_ERR_NONE, "No error"},
    {MD_ERR_ALLOC, "File not found"},
    {MD_ERR_JSON, "Invalid input"},
    {MD_ERR_MOODLE_EXCEPTION, "Moodle returned exception: %s"},
    {MD_ERR_MISSING_JSON_KEY, "Missing json key: %s"},
    {MD_ERR_INVALID_JSON_VALUE, "Invalid json value for key %s"},
    {MD_ERR_INVALID_JSON, "Invalid json stream"},
    {MD_ERR_HTTP_REQUEST_FAIL, "Http request failed: %s"},
    {MD_ERR_CURL_FAIL, "Curl library error"},
    {MD_ERR_FILE_OPERATION, "Unable to perform operation on file %s"},
    {MD_ERR_MISUSED_MOODLE_API, "Moodle api c function was used incorrectly"},
    {MD_ERR_MISMACHING_MOODLE_DATA, "Moodle server returned different data from different calls"},
    {MD_ERR_FAILED_PLUGIN_LOGIN, "Failed to login using plugin"}, 
    {MD_ERR_NO_MATCHING_PLUGIN_FOUND, "No matching plugin found for website %s"}, 
    {MD_ERR_FAILED_TO_LOAD_PLUGIN, "Failed to load plugin: %s"}, 
    {MD_ERR_MISSING_PLUGIN_VAR, "Missing plugin variable required for a plugin: %s"}, 
    {MD_ERR_INVALID_PLUGIN, "Plugin %s is invalid"}, 
};

void md_set_error_handling_warning() {
    errorHandlingWarningSet = true;
}

void md_error_set_message(cchar *message) {
    snprintf(msg, MESSAGE_SIZE, "%s", message);
}

cchar *md_error_get_message(MDError code) {
    errBuff[0] = 0;
    int size = sizeof(errorMessages) / sizeof(struct errorMsg);
    for (int i = 0; i < size; ++i) {
        if (errorMessages[i].code == code) {
            snprintf(errBuff, MESSAGE_SIZE, errorMessages[i].message, msg);
            int len = strlen(errBuff);
            if (errorHandlingWarningSet) {
                snprintf(errBuff + len, MESSAGE_SIZE - len, "\nWARNING: failed error handling!");
            }
            break;
        }
    }
    return errBuff;
}