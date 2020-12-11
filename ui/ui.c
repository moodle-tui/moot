#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rlutil.h"
#include "utf8.c"
#include "wcwidth.h"
#include <curl/curl.h>

#include "moodle.h"
// #include "util.h"

typedef struct menuInfo {
    int depth, *widths, HLOptions[3];
    char *seperator;
} MenuInfo;

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

int getBigger(int a, int b) {
    return a > b? a: b;
}

int *getWidthsOfOptions(int sepLength) {
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

void printPlainOption(char *optionName, int width) {
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
    printPlainOption(optionName, width);
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

void printOption(char *name, int width, _Bool isHighlighted) {
    if (isHighlighted)
        printHighlightedOption(name, width);
    else
        printPlainOption(name, width);
}

void fillOptionRow(MDArray mdArray, MenuInfo *menuInfo, int rowIndex, int optionsIndex) {
    int widthIndex = optionsIndex - menuInfo->depth;
    if (widthIndex < 0)
        return;
    
    if (mdArray.len > rowIndex) {
        switch (optionsIndex) {
        case 0:
            printOption(MD_COURSES(mdArray)[rowIndex].name,
                        menuInfo->widths[widthIndex],
                        menuInfo->HLOptions[optionsIndex] == rowIndex);
            break;
        case 1:
            printOption(MD_TOPICS(mdArray)[rowIndex].name,
                        menuInfo->widths[widthIndex],
                        menuInfo->HLOptions[optionsIndex] == rowIndex);
            break;
        case 2:
            printOption(MD_MODULES(mdArray)[rowIndex].name,
                        menuInfo->widths[widthIndex],
                        menuInfo->HLOptions[optionsIndex] == rowIndex);
            break;
        default:
            printf("Wrong option index\n");
        }
    }
    else
        printSpaces(menuInfo->widths[widthIndex]);
}

void printMenu(MDArray courses, MenuInfo *menuInfo, int height) {
    int *hl = menuInfo->HLOptions;

    for (int i = 0; i < height; ++i) {
        fillOptionRow(courses, menuInfo, i, 0);
        printf("%s", menuInfo->seperator);

        fillOptionRow(MD_COURSES(courses)[hl[0]].topics, menuInfo, i, 1);
        printf("%s", menuInfo->seperator);

        fillOptionRow(MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules,
                      menuInfo, i, 2);
        printf("\n");
    }
}

void menuLoop (MDArray courses, MenuInfo *menuInfo) {
    printf("In the menu loop\n");
    menuInfo->widths = getWidthsOfOptions(strlen(menuInfo->seperator));

    int *hl = menuInfo->HLOptions;
    int heights[3] = {
        courses.len,
        MD_COURSES(courses)[hl[0]].topics.len,
        MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules.len
    };
    int maxHeight = getBigger(heights[0], getBigger(heights[1], heights[2]));

    _Bool isOptionChosen = 0;
    while (!isOptionChosen) {
        cls();
        printMenu(courses, menuInfo, maxHeight);
        getch();
        //processNavigationRequest(&HLOptions[1], 2, &isOptionChosen);
    }
}

int main () {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen("../.token", "r");
    char token[100];
    fread_line(f, token, 99);
    MDError err;
    MDClient *client = md_client_new(token, "https://emokymai.vu.lt", &err);

    md_client_init(client, &err);
    if (err) {
        printf("%d %s\n", err, md_error_get_message(err));
        return 0;
    }
    MDArray courses = md_client_fetch_courses(client, &err);

    MenuInfo menuInfo = {
        .HLOptions = {0},
        .depth = 0,
        .seperator = "  "
    };
    menuLoop(courses, &menuInfo);

    md_courses_cleanup(courses);

    md_client_cleanup(client);
    fclose(f);
    curl_global_cleanup();
}
