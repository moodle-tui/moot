#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#define MESSAGE_SIZE 4096

char msg[MESSAGE_SIZE] = "";

struct errorMsg{
    ErrorCode code;
    const char *message;
} errorMessages[] = {
    { ERR_NONE, "No error" },
    { ERR_JSON, "Invalid input" },
    { ERR_ALLOC, "File not found" },
};

void setErrorMessage(const char *message) {
    printf("%s\n", message);
    snprintf(msg, MESSAGE_SIZE, "%s", message);
}


const char *getError(ErrorCode code) {
    int size = sizeof(errorMessages) / sizeof(struct errorMsg);
    for (int i = 0; i < size; ++i) {
        if (errorMessages[i].code == code)
            return errorMessages[i].message;
    }
    return NULL;
}