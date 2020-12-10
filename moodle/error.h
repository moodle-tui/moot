#ifndef __MT_ERROR_H
#define __MT_ERROR_H

typedef enum {
    MD_ERR_NONE = 0,
    MD_ERR_ALLOC,
    MD_ERR_JSON,
    MD_ERR_MOODLE_EXCEPTION,
    MD_ERR_MISSING_JSON_KEY,
    MD_ERR_INVALID_JSON_VALUE,
    MD_ERR_INVALID_JSON,
    MD_ERR_HTTP_REQUEST_FAIL,
    MD_ERR_CURL_FAIL,
    MD_ERR_FILE_OPERATION,
    MD_ERR_MISUSED_MOODLE_API,
    MD_ERR_MISMACHING_MOODLE_DATA,
} MDError;

const char *md_error_get_message(MDError);
// privat
void md_set_error_message(const char *message);

#endif