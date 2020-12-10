#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rlutil.h"
#include "utf8.c"
#include "wcwidth.h"
#include <curl/curl.h>

#include "client.h"
#include "util.h"

/* Reads single line up to n bytes from file with \n removed. s should be at
   least n + 1 in length. 1 is returned if the line is too long and was cut off. */
int fread_line(FILE *file, char *s, int n) {
    s[0] = '\0';
    int len = 0, cutOff = 0;
    char c = 0;
    do {
        c = fgetc(file);
        if (c != '\n' && !feof(file)) {
            if (len == n) cutOff = 1;

            if (len < n) s[len++] = c;
        }
    } while (!feof(file) && c != '\n');
    s[len] = '\0';
    return cutOff;
}

int *getWidthOfColumns(int sepLength) {
    int *widthOfColumns = malloc(sizeof(int) * 3);
    int availableColumns = tcols();
    widthOfColumns[0] = (availableColumns / 6) - sepLength;
    widthOfColumns[1] = (availableColumns / 3) - sepLength;
    widthOfColumns[2] = (availableColumns / 2);
    return widthOfColumns;
}

void printSpaces(int count) {
    printf("%*s", count, "");
}

void printOption(char *optionName, int width) {
    Rune u;
    int printedChWidth = 0, printedChSize = 0;
    int optionLength = strlen(optionName);

    while (1) {
        int charSize = utf8decode(optionName + printedChSize, &u, optionLength);
        int charWidth = wcwidth(u);
        printedChWidth += charWidth;

        if (printedChWidth > width - 1 && optionName[printedChSize + charSize]) {
            printf("~");
            printSpaces(printedChWidth - width);
            break;
        }
        if (charWidth == 0) {
            printSpaces(width - printedChWidth);
            break;
        }

        fwrite(optionName + printedChSize, charWidth, charSize, stdout);
        printedChSize += charSize;
    }

}

void printHighlightedOption(char *optionName, int width) {
    saveDefaultColor();
    setBackgroundColor(7);
    setColor(0);
    printOption(optionName, width);
    resetColor();
}

void goDown(int *highlightedOption, int nrOfOptions) {
    if (*highlightedOption == nrOfOptions - 1)
        *highlightedOption = 0;
    else
        ++*highlightedOption;
}

void goUp(int *highlightedOption, int nrOfOptions) {
    if (*highlightedOption == 0)
        *highlightedOption = nrOfOptions - 1;
    else
        --*highlightedOption;
}

void processNavigationRequest(int *highlightedOption, int nrOfOptions, _Bool *isOptionChosen) {
        int key = getkey();
        switch(key) {
            case 9: // tab
            case 106: // j
                goDown(highlightedOption, nrOfOptions);
                break;
            case 107: // k
                goUp(highlightedOption, nrOfOptions);
                break;
            case 0:
                if (getkey() == 91) {
                    key = getkey();
                    switch (key) {
                        case 66: // arrow down
                            goDown(highlightedOption, nrOfOptions);
                            break;
                        case 90: // shift tab
                        case 65: // arrow up
                            goUp(highlightedOption, nrOfOptions);
                    }
                }
                break;
            case 108: // l
            case 10: // enter
                *isOptionChosen = 1;
                break;
        }
}

void menuLoop (int depth, char *seperator, Courses courses, int *highlightedOptions) {
    int *widthOfColumns = getWidthOfColumns(strlen(seperator));
    _Bool isOptionChosen = 0;
    while (!isOptionChosen) {
        cls();
        for (int i = 0; 1; ++i) {
            int emptyColumns = 0;
            if (courses.len > i)
                printOption(courses.data[i].name, widthOfColumns[highlightedOptions[0]]);
            else {
                printSpaces(widthOfColumns[0]);
                ++emptyColumns;
            }
            printf("%s", seperator);

            if (courses.data[highlightedOptions[0]].topics.len > i)
                printOption(courses.data[highlightedOptions[0]].topics.data[i].name, widthOfColumns[1]);
            else {
                printSpaces(widthOfColumns[1]);
                ++emptyColumns;
            }
            printf("%s", seperator);

            if (courses.data[highlightedOptions[0]].topics.data[highlightedOptions[1]].modules.len > i)
                printOption(courses.data[highlightedOptions[0]].topics.data[highlightedOptions[1]].modules.data[i].name, widthOfColumns[2]);
            else
                ++emptyColumns;
            
            if (emptyColumns == 3)
                break;

            printf("\n");
        }
        getch();
        //processNavigationRequest(&highlightedOptions[1], 2, &isOptionChosen);
    }
}

int main () {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen("../.token", "r");
    char token[100];
    fread_line(f, token, 99);
    Client *client = mt_new_client(token, "https://emokymai.vu.lt");

    ErrorCode err = ERR_NONE;
    if ((err = mt_init_client(client))) {
        printf("%d %s\n", err, getError(err));
        return 0;
    }
    Courses courses = mt_get_courses(client, &err);

    char seperator[] = "  ";
    int highlightedOptions[3] = {0};
    int emptyColumns;
    _Bool isOptionChosen = 0;
    menuLoop(1, seperator, courses, highlightedOptions);

    mt_free_courses(courses);

    mt_destroy_client(client);
    fclose(f);
    curl_global_cleanup();
}
