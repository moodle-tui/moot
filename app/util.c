#include <stdlib.h>
#include <math.h>
#include "app.h"

char *getStr(int n) {
    int len = getNrOfDigits(n) + 1;
    char *str = malloc(sizeof(char) * len);
    sprintf(str, "%d", n);
    return str;
}

int getNrOfDigits(int number) {
    return snprintf(NULL, 0, "%d", number);
}

void *xmalloc(size_t size, Message *msg) {
    return xrealloc(NULL, size, msg);
}

void *xrealloc(void *data, size_t size, Message *msg) {
    data = realloc(data, size);
    if (!data && size)
        createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
    return data;
}

void *xcalloc(size_t n, size_t size, Message *msg) {
    void *data = calloc(n, size);
    if (!data && size)
        createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
    return data;
}

