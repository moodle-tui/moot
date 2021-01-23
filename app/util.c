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
    void *data = malloc(size);
    if (!data && size)
        createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
    return data;
}

int getNrOfRecurringMessages(Message msg, Message *prevMsg, Action action) {
    static Action originalAction = -1;
    static int nrOfRecurringMessages = 0;
    if (originalAction == -1)
        originalAction = action;
    if (msgCompare(*prevMsg, msg) == -1) {
        originalAction = action;
        createMsg(prevMsg, msg.msg, NULL, msg.type);
        nrOfRecurringMessages = 0;
    }
    else if (originalAction == action)
        ++nrOfRecurringMessages;
    return nrOfRecurringMessages;
}

