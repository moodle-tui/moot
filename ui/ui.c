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


int goLeft(MenuInfo *menuInfo) {
    if (menuInfo->depth != 0) {
        menuInfo->HLOptions[menuInfo->depth] = 0;
        --menuInfo->depth;
        return 0;
    }
    return -1;
}

int goRight(MenuInfo *menuInfo) {
    if (menuInfo->depth != 2) {
        ++menuInfo->depth;
        return 0;
    }
    return -1;
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

// returns 0 on succes, -1 on invalid action, 1 on quit
int processNavigationRequest(MenuInfo *menuInfo, int nrOfOptions) {
    int key = getkey();
    int returnCode = 0;
    int *highlightedOption = &menuInfo->HLOptions[menuInfo->depth];
    switch (key) {
        case 106: // j
        case KEY_DOWN:
            goDown(highlightedOption, nrOfOptions);
            break;
        case 107: // k
        case KEY_UP:
            goUp(highlightedOption, nrOfOptions);
            break;
        case 0:
            if (getkey() == 91) {
                key = getkey();
                switch (key) {
                    case 66: // arrow down
                        goDown(highlightedOption, nrOfOptions);
                        break;
                    case 65: // arrow up
                        goUp(highlightedOption, nrOfOptions);
                }
            }
            break;
        case 104: // h
        case KEY_LEFT:
            returnCode = goLeft(menuInfo);
            break;
        case 108: // l
        case KEY_RIGHT:
        case 10: // enter
            returnCode = goRight(menuInfo);
            break;
        case 113: // q
            returnCode = 1;
    }
    return returnCode;
}

void printPlainOption(char *optionName, int width) {
    Rune u;
    int printedChWidth = 0, printedChSize = 0;
    int optionLength = strlen(optionName);

    printf(" ");
    --width;
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

void printOption(char *name, int width, _Bool isHighlighted) {
    if (isHighlighted)
        printHighlightedOption(name, width);
    else
        printPlainOption(name, width);
}

void addOption(MDArray mdArray, MenuInfo *menuInfo, int heightIndex, int optionsIndex) {
    int widthIndex = optionsIndex - menuInfo->depth + 1;
    if (widthIndex < 0 || widthIndex > 2)
        return;

    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", menuInfo->seperator);

    if (mdArray.len > heightIndex) {
        switch (optionsIndex) {
            case -1:
                printSpaces(menuInfo->widths[0]);
                break;
            case 0:
                printOption(MD_COURSES(mdArray)[heightIndex].name,
                            menuInfo->widths[widthIndex],
                            menuInfo->HLOptions[optionsIndex] == heightIndex);
                break;
            case 1:
                printOption(MD_TOPICS(mdArray)[heightIndex].name,
                            menuInfo->widths[widthIndex],
                            menuInfo->HLOptions[optionsIndex] == heightIndex);
                break;
            case 2:
                printOption(MD_MODULES(mdArray)[heightIndex].name,
                            menuInfo->widths[widthIndex],
                            menuInfo->HLOptions[optionsIndex] == heightIndex);
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
        addOption(courses, menuInfo, i, -1); // first empty row
        addOption(courses, menuInfo, i, 0);
        addOption(MD_COURSES(courses)[hl[0]].topics, menuInfo, i, 1);
        addOption(MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules,
                      menuInfo, i, 2);
        printf("\n");
    }
}

int getMax(int *array, int size) {
    int max = 0;
    for (int i = 0; i < size; ++i) {
        if (max < array[i])
            max = array[i];
    }
    return max;
}

int *getHeights(MDArray courses, int *hl) {
    int *heights = malloc(sizeof(int) * 4);
    heights[0] = courses.len;
    heights[1] = MD_COURSES(courses)[hl[0]].topics.len,
    heights[2] = MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules.len;
    return heights;
}

void menuLoop (MDArray courses, MenuInfo *menuInfo) {
    _Bool isOptionChosen = 0;

    while (!isOptionChosen) {
        menuInfo->widths = getWidthsOfOptions(strlen(menuInfo->seperator));
        int *heights = getHeights(courses, menuInfo->HLOptions);
        int maxHeight = getMax(heights, 3);
        int availableHeight = trows() - 1;
        if (maxHeight > availableHeight)
            maxHeight = availableHeight;

        cls();
        printMenu(courses, menuInfo, maxHeight);

        int navReturnCode;
        do {
            navReturnCode = processNavigationRequest(menuInfo, heights[menuInfo->depth]);
        } while (navReturnCode == -1);

        if (navReturnCode == 1)
            break;
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
    hidecursor();
    menuLoop(courses, &menuInfo);
    cls();
    showcursor();

    md_courses_cleanup(courses);

    md_client_cleanup(client);
    fclose(f);
    curl_global_cleanup();
}
