#include <stdio.h>

#include "msg.h"
#include "rlutil.h"

static void printSpaces(int count);

void msgBig(cchar *msg, int color) {
    int msgLen = strlen(msg);
    int x = (tcols() / 2) - (msgLen / 2);
    int y = trows() / 2 - 2;
    locate(x, y);
    saveDefaultColor();
    setColor(color);
    
    printf(" ┌");
    for (int i = 0; i < msgLen + 2; ++i)
        printf("─");
    printf("┐ \n");

    locate(x, ++y);
    printf(" │ ");
    resetColor();
    printf("%s", msg);
    setColor(color);
    printf(" │ \n");

    locate(x, ++y);
    printf(" └");
    for (int i = 0; i < msgLen + 2; ++i)
        printf("─");
    printf("┘ ");

    resetColor();
    getch();
}

void msgSmall(cchar *msg, int color) {
    locate (0, trows());
    printSpaces(tcols());
    locate (0, trows());
    saveDefaultColor();
    setColor(color);
    printf(" %s ",  msg);
    resetColor();
}

void msgErr(cchar *msg) {
    showcursor();
    saveDefaultColor();
    setColor(RED);
    printf("Error: %s",  msg);
    resetColor();
    getch();
}

void printSpaces(int count) {
    printf("%*s", count, "");
}
