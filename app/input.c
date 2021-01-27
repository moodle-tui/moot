/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#include <stdio.h>
#include <errno.h>
#include "rlutil.h"
#include "app.h"

void readLine(char *line, Message *msg);

char *getInput(char *inputMsg, Message *msg) {
    locate (0, trows());
    printSpaces(tcols());
    locate (0, trows());
    printf("%s ", inputMsg);
    showcursor();
    char *line = xmalloc(sizeof(char) * 2, msg);
    readLine(line, msg);
    hidecursor();
    return line;
}

void readLine(char *line, Message *msg) {
    int lineLen = 2;
    errno = 0;
    for (int i = 0; scanf("%c", &line[i]); ++i) {
        if (errno) {
            createMsg(msg, strerror(errno), NULL, MSG_TYPE_ERROR);
            break;
        }
        if (line[i] == '\n') {
            line[i] = 0;
            break;
        }
        ++lineLen;
        line = xrealloc(line, lineLen * sizeof(char), msg);
        if (msg->type == MSG_TYPE_ERROR)
            break;
    }
}

