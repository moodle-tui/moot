#ifndef __MT_ERROR_H
#define __MT_ERROR_H

typedef enum {
    ERR_NONE = 0,
    ERR_ALLOC,
    ERR_JSON,
    ERR_MOODLE_EXCEPTION,
    ERR_MISSING_JSON_KEY,
    ERR_INVALID_JSON_VALUE,
    ERR_INVALID_JSON,
    ERR_HTTP_REQUEST_FAIL,
    ERR_CURL_FAIL,
    ERR_FILE_OPERATION,
    ERR_MISUSED_MOODLE_API,
} ErrorCode;

const char *getError(ErrorCode);
void setErrorMessage(const char *message);

typedef struct Error {
    ErrorCode code;
    const char *file;
    int line;
} Error;


Error newError(ErrorCode code, const char *file, int line);

#endif