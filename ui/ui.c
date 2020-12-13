#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rlutil.h"
#include "utf8.c"
#include "wcwidth.h"
#include <curl/curl.h>

#include "moodle.h"
// #include "util.h"

#define EMPTY_OPTIONS_NAME "[empty]"
#define OPTION_CUT_STR "~"
#define SEPERATOR "  "

typedef struct menuInfo {
    int depth, *widths, HLOptions[3];
    _Bool canGoRight;
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

void printErr(char *msg) {
    showcursor();
    locate(0, 0);
    saveDefaultColor();
    setBackgroundColor(4);
    printf("Error: %s\n", msg);
    resetColor();
    getch();
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

char *getNextHighlightedOption(MDArray courses, MenuInfo *menuInfo) {
    MDArray topics = MD_COURSES(courses)[menuInfo->HLOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[menuInfo->HLOptions[1]].modules;
    char *name = "";
    
    switch (menuInfo->depth) {
    case 0:
        if (topics.len != 0)
            name = MD_TOPICS(topics)[0].name;
        else
            name = NULL;
        break;
    case 1:
        if(modules.len != 0)
            name = MD_MODULES(modules)[0].name;
        else
            name = NULL;
    }
    return name;
}

_Bool checkIfRightActionPossible(MDArray courses, MenuInfo *menuInfo) {
    if (getNextHighlightedOption(courses, menuInfo) != NULL)
        return 1;

    return 0;
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
int processNavigationRequest(MenuInfo *menuInfo, int nrOfOptions, _Bool canGoRight) {
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
        if (canGoRight)
            returnCode = goRight(menuInfo);
        else
            returnCode = -1;
        break;
    case 113: // q
        returnCode = 1;
    }
    return returnCode;
}

void printOption(char *optionName, int width) {
    Rune u;
    int printedChWidth = 0, printedChSize = 0;
    int optionLength = strlen(optionName);

    printf(" ");
    --width;
    while (1) {
        int charSize = utf8decode(optionName + printedChSize, &u, optionLength);
        int charWidth = wcwidth(u);

        if (!optionName[printedChSize]) {
            printSpaces(width - printedChWidth);
            break;
        }
        else if (printedChWidth + charWidth > width - 1 && optionName[printedChSize + charSize]) {
            printSpaces(width - printedChWidth - 1);
            printf("~");
            ++printedChWidth;
            break;
        }

        fwrite(optionName + printedChSize, charWidth, charSize, stdout);
        printedChWidth += charWidth;
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

void addOption(char *name, MenuInfo *menuInfo, int optionDepth, _Bool isHighlighted) {
    int widthIndex = optionDepth - menuInfo->depth + 1;
    if (widthIndex < 0 || widthIndex > 2)
        return;

    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", SEPERATOR);

    if (name == NULL) {
        printErr("Invalid option name. Expected string, got NULL");
    }
    if (isHighlighted && strcmp(name, EMPTY_OPTIONS_NAME))
        printHighlightedOption(name, menuInfo->widths[widthIndex]);
    else
        printOption(name, menuInfo->widths[widthIndex]);
}

char *getMDArrayName(MDArray mdArray, int heightIndex, int depthIndex) {
    char *name;
    switch (depthIndex) {
    case 0:
        name = MD_COURSES(mdArray)[heightIndex].name;
        break;
    case 1:
        name = MD_TOPICS(mdArray)[heightIndex].name;
        break;
    case 2:
        name = MD_MODULES(mdArray)[heightIndex].name;
        break;
    default:
        printErr("Wrong option index\n");
    }
    return name;
}

void addMDArrayOption(MDArray mdArray, MenuInfo *menuInfo, int heightIndex, int depthIndex) {
    char *name;
    if (mdArray.len > heightIndex) {
        name = getMDArrayName(mdArray, heightIndex, depthIndex);
    }
    else {
        if (heightIndex == 0)
            name = EMPTY_OPTIONS_NAME;
        else if (depthIndex == 2)
            return;
        else
            name = " ";
    }

    addOption(name, menuInfo, depthIndex,
              menuInfo->HLOptions[depthIndex] == heightIndex);
}

void printMenu(MDArray courses, MenuInfo *menuInfo, int height) {
    MDArray topics = MD_COURSES(courses)[menuInfo->HLOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[menuInfo->HLOptions[1]].modules;

    for (int i = 0; i < height; ++i) {
        addOption(" ", menuInfo, -1, 0); // first empty options column
        addMDArrayOption(courses, menuInfo, i, 0);
        addMDArrayOption(topics, menuInfo, i, 1);
        addMDArrayOption(modules, menuInfo, i, 2);
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
    int *heights = malloc(sizeof(int) * 3);
    heights[0] = courses.len;
    heights[1] = MD_COURSES(courses)[hl[0]].topics.len,
    heights[2] = MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules.len;
    return heights;
}

void menuLoop (MDArray courses, MenuInfo *menuInfo) {
    _Bool isOptionChosen = 0;

    while (!isOptionChosen) {
        menuInfo->widths = getWidthsOfOptions(strlen(SEPERATOR));
        int *heights = getHeights(courses, menuInfo->HLOptions);
        int maxHeight = getMax(heights, 3);
        int availableHeight = trows() - 1;
        if (maxHeight > availableHeight)
            maxHeight = availableHeight;

        cls();
        printMenu(courses, menuInfo, maxHeight);

        int navReturnCode;
        _Bool canGoRight = checkIfRightActionPossible(courses, menuInfo);
        do {
            navReturnCode = processNavigationRequest(menuInfo, heights[menuInfo->depth],
                                                     canGoRight);
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
    MDArray mdArray = md_client_fetch_courses(client, &err);

    MenuInfo menuInfo = {
        .HLOptions = {0},
        .depth = 0,
    };
    hidecursor();
    menuLoop(mdArray, &menuInfo);
    cls();
    showcursor();

    md_courses_cleanup(mdArray);
    md_client_cleanup(client);
    fclose(f);
    curl_global_cleanup();
}
