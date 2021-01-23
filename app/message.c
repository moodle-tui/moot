#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "app.h"
#include "rlutil.h"

void createMsg(Message *msg, cchar *content, cchar *details, MsgType type) {
    if (!msg->msg)
        msgInit(msg);
    snprintf(msg->msg, MSG_LEN, content, details);
    msg->type = type;
}

void msgInit(Message *msg) {
    *msg = (Message) {
        .msg = calloc(MSG_LEN, sizeof(char)),
        .type = MSG_TYPE_NONE,
    };
}

bool checkIfMsgBad(Message msg) {
    return (msg.type == MSG_TYPE_BAD_ACTION || msg.type == MSG_TYPE_ERROR || msg.type == MSG_TYPE_WARNING);
}

void printMsg(Message msg, int nrOfRecurringMessages) {
    char initStr[MSG_LEN] = {0};
    locate (0, trows());
    printSpaces(tcols());
    locate (0, trows());
    saveDefaultColor();
    switch(msg.type) {
        case MSG_TYPE_SUCCESS:
            setColor(MSG_COLOR_SUCCESS);
            break;
        case MSG_TYPE_INFO:
            setColor(MSG_COLOR_INFO);
            break;
        case MSG_TYPE_BAD_ACTION:
            setColor(MSG_COLOR_BAD_ACTION);
            break;
        case MSG_TYPE_WARNING:
            setColor(MSG_COLOR_WARNING);
            break;
        case MSG_TYPE_ERROR:
            strcpy(initStr, ERROR_MSG_INIT_STRING);
            setColor(MSG_COLOR_ERROR);
            break;
        default:
            break;
    }
    printf(" %s%s", initStr, msg.msg);
    if (nrOfRecurringMessages)
        printf(" (%d)", nrOfRecurringMessages + 1);
    resetColor();
}

int msgCompare(Message msg1, Message msg2) {
    if (msg1.type == MSG_TYPE_NONE || msg2.type == MSG_TYPE_NONE)
        return -1;
    if (msg1.type != msg2.type || strcmp(msg1.msg, msg2.msg))
        return -1;
    return 0;
}

void printMsgNoUI(Message msg) {
    char initStr[MSG_LEN] = {0};
    if (msg.type == MSG_TYPE_ERROR)
        strcpy(initStr, ERROR_MSG_INIT_STRING);
    printf("%s%s\n", initStr, msg.msg);
}

