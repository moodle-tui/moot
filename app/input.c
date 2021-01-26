#include <stdio.h>
#include <errno.h>
#include "rlutil.h"
#include "app.h"

char *getInput(char *inputMsg, Message *msg) {
    locate (0, trows());
    printSpaces(tcols());
    locate (0, trows());
    printf("%s ", inputMsg);
    showcursor();
    char *line = xmalloc(sizeof(char), msg);
    readLine(line, msg);
    return line;
}

char *readLine(char *line, Message *msg) {
    int lineLen = 1;
    errno = 0;
    for (int i = 0; scanf("%c", &line[i]); ++i) {
        if (errno) {
            createMsg(msg, strerror(errno), NULL, MSG_TYPE_ERROR);
            return line;
        }
        //if (line[i] == '\n') {
        //    line[i] = 0;
        //    break;
        //}
        //if (!line[i])
        //    break;

        ++lineLen;
        line = xrealloc(line, lineLen * sizeof(char), msg);
        if (msg->type == MSG_TYPE_ERROR)
            return line;
    }
    return line;
}

