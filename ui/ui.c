#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "rlutil.h"
#include "utf8.c"
#include "wcwidth.h"
#include <curl/curl.h>

#include "moodle.h"
// #include "util.h"

int main () {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen(".token", "r");
    char token[100];
    fread_line(f, token, 99);
    MDError err;
    MDClient *client = md_client_new(token, "https://emokymai.vu.lt", &err);

    md_client_init(client, &err);
    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        printErr(msg);
        return 0;
    }
    MDArray courseArr = md_client_fetch_courses(client, &err);
    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        printErr(msg);
        return 0;
    }

    MenuInfo menuInfo = {
        .HLOptions = {0},
        .depth = 0,
        .isCurrentFile = 0,
        .client = client,
    };
    MDCourse* courses = MD_ARR(courseArr, MDCourse);
    hidecursor();
    mainLoop(courseArr, &menuInfo);
    cls();
    showcursor();

    md_courses_cleanup(courseArr);
    md_client_cleanup(client);
    fclose(f);
    curl_global_cleanup();
}

void mainLoop (MDArray courses, MenuInfo *menuInfo) {
    _Bool isOptionChosen = 0;

    while (!isOptionChosen) {
        menuInfo->widths = getWidthsOfOptions(strlen(SEPERATOR));
        int *heights = getHeights(courses, menuInfo->HLOptions);
        int maxHeight = getMax(heights, 3);
        int terminalHeight = trows() - 1;
        if (maxHeight >= terminalHeight) {
            maxHeight = terminalHeight - 1;
        }

        cls();
        menuInfo->currentMaxDepth = 0;
        printMenu(courses, menuInfo, maxHeight);

        Action action;
        do {
            int key = getkey();
            KeyDef keyDef = getKeyDef(key);
            action = getAction(courses, menuInfo, keyDef);
        } while (action == ACTION_INVALID);

        if (action == ACTION_QUIT)
            break;
        doAction(menuInfo, courses, heights[menuInfo->depth], action);
    }
}

int *getHeights(MDArray courses, int *hl) {
    int *heights = malloc(sizeof(int) * 3);
    heights[0] = courses.len;
    heights[1] = MD_COURSES(courses)[hl[0]].topics.len,
    heights[2] = MD_TOPICS(MD_COURSES(courses)[hl[0]].topics)[hl[1]].modules.len;
    return heights;
}

void printMenu(MDArray courses, MenuInfo *menuInfo, int height) {
    MDArray topics = MD_COURSES(courses)[menuInfo->HLOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[menuInfo->HLOptions[1]].modules;
    MDModResource resource;

    for (int i = 0; i < height; ++i) {
        int depthIndex = -2;
        addOption(" ", menuInfo, ++depthIndex, 0); // first empty options column
        addMDArrayOption(courses, menuInfo, i, ++depthIndex);
        addMDArrayOption(topics, menuInfo, i, ++depthIndex);
        addMDArrayOption(modules, menuInfo, i, ++depthIndex);

        if (modules.len > 0 && MD_MODULES(modules)[menuInfo->HLOptions[2]].type == MD_MOD_RESOURCE) {
            if (i == 0)
                addOption("Files", menuInfo, ++depthIndex, menuInfo->HLOptions[3] == i);
            resource = MD_MODULES(modules)[menuInfo->HLOptions[2]].contents.resource;
            if (resource.files.len > i) {
                addOption(MD_FILES(resource.files)[i].filename, menuInfo, ++depthIndex,
                          menuInfo->HLOptions[4] == i);
                if (menuInfo->depth == depthIndex)
                    menuInfo->isCurrentFile = 1;
                else
                    menuInfo->isCurrentFile = 0;
            }
        }
        printf("\n");
    }
}

void addMDArrayOption(MDArray mdArray, MenuInfo *menuInfo, int heightIndex, int depthIndex) {
    char *name;
    if (mdArray.len > heightIndex) {
        name = getMDArrayName(mdArray, heightIndex, depthIndex);
    }
    else {
        if (heightIndex == 0)
            name = EMPTY_OPTION_NAME;
        else if (depthIndex == 2)
            return;
        else
            name = " ";
    }

    addOption(name, menuInfo, depthIndex,
              menuInfo->HLOptions[depthIndex] == heightIndex);
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

void addOption(char *name, MenuInfo *menuInfo, int optionDepth, _Bool isHighlighted) {
    int widthIndex = optionDepth - menuInfo->depth + 1;
    if (widthIndex < 0 || widthIndex > 2)
        return;

    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", SEPERATOR);

    if (name == NULL) {
        printErr("Invalid option name. Expected string, got NULL");
    }
    if (isHighlighted && strcmp(name, EMPTY_OPTION_NAME)) {
        printHighlightedOption(name, menuInfo->widths[widthIndex]);
        if (menuInfo->currentMaxDepth < optionDepth)
            menuInfo->currentMaxDepth = optionDepth;
    }
    else
        printOption(name, menuInfo->widths[widthIndex]);
}

void printHighlightedOption(char *optionName, int width) {
    saveDefaultColor();
    setBackgroundColor(7);
    setColor(0);
    printOption(optionName, width);
    resetColor();
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

KeyDef getKeyDef(int key) {
    KeyDef keyDef;
    switch (key) {
        case 106: // j
        case KEY_DOWN:
            keyDef = KD_DOWN;
            break;
        case 107: // k
        case KEY_UP:
            keyDef = KD_UP;
            break;
        case 0:
            if (getkey() == 91) {
                key = getkey();
                switch (key) {
                case 66: // key down
                    keyDef = KD_DOWN;
                    break;
                case 65: // key up
                    keyDef = KD_UP;
                }
            }
            break;
        case 104: // h
        case KEY_LEFT:
            keyDef = KD_LEFT;
            break;
        case 108: // l
        case KEY_RIGHT:
        case 10: // enter
            keyDef = KD_RIGHT;
            break;
        case 113: // q
            keyDef = KD_QUIT;
    }
    return keyDef;
}

Action getAction(MDArray courses, MenuInfo *menuInfo, KeyDef keyDef) {
    Action action;
    switch (keyDef) {
        case KD_RIGHT:
            if (menuInfo->isCurrentFile)
                action = ACTION_DOWNLOAD;
            else if (menuInfo->depth < menuInfo->currentMaxDepth)
                action = ACTION_GO_RIGHT;
            else
                action = ACTION_INVALID;
            break;
        case KD_DOWN:
            action = ACTION_GO_DOWN;
            break;
        case KD_LEFT:
            if (menuInfo->depth != 0)
                action = ACTION_GO_LEFT;
            else
                action = ACTION_INVALID;
            break;
        case KD_UP:
            action = ACTION_GO_UP;
            break;
        case KD_QUIT:
            action = ACTION_QUIT;
            break;
    }
    return action;
};

void doAction(MenuInfo *menuInfo, MDArray courses, int nrOfOptions, Action action) {
    int *highlightedOption = &menuInfo->HLOptions[menuInfo->depth];
    MDFile mdFile;
    switch (action) {
        case ACTION_GO_RIGHT:
            goRight(menuInfo);
            break;
        case ACTION_GO_DOWN:
            goDown(highlightedOption, nrOfOptions);
            break;
        case ACTION_GO_LEFT:
            goLeft(menuInfo);
            break;
        case ACTION_GO_UP:
            goUp(highlightedOption, nrOfOptions);
            break;
        case ACTION_DOWNLOAD:
            mdFile = getMDFile(courses, menuInfo);
            downloadFile(mdFile, menuInfo->client);
            break;
        default:
            printErr("Invalid action");
    }
}

void goRight(MenuInfo *menuInfo) {
    ++menuInfo->depth;
}

void goDown(int *highlightedOption, int nrOfOptions) {
    if (*highlightedOption == nrOfOptions - 1)
        *highlightedOption = 0;
    else
        ++*highlightedOption;
}

void goLeft(MenuInfo *menuInfo) {
    menuInfo->HLOptions[menuInfo->depth] = 0;
    --menuInfo->depth;
}

void goUp(int *highlightedOption, int nrOfOptions) {
    if (*highlightedOption == 0)
        *highlightedOption = nrOfOptions - 1;
    else
        --*highlightedOption;
}

MDFile getMDFile(MDArray courses, MenuInfo *menuInfo) {
    MDArray topics = MD_COURSES(courses)[menuInfo->HLOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[menuInfo->HLOptions[1]].modules;
    MDFile mdFile;
    if (modules.len > 0 && MD_MODULES(modules)[menuInfo->HLOptions[2]].type == MD_MOD_RESOURCE) {
        MDModResource resource;
        resource = MD_MODULES(modules)[menuInfo->HLOptions[2]].contents.resource;
        mdFile = MD_FILES(resource.files)[menuInfo->HLOptions[4]];
    }
    else
        printErr("Couldn't get moodle file");

    return mdFile;
}

void downloadFile(MDFile mdFile, MDClient *client) {
    FILE* file = fopen(mdFile.filename, "w");
    if (!file)
        printErr("Couldn't open file for writing");
    MDError err;
    md_client_download_file(client, &mdFile, file, &err);
    fclose(file);

    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        printErr(msg);
    }
}

void printSpaces(int count) {
    printf("%*s", count, "");
}

int *getWidthsOfOptions(int sepLength) {
    int *widthOfColumns = malloc(sizeof(int) * 3);
    int availableColumns = tcols();
    widthOfColumns[0] = (availableColumns / 6) - sepLength;
    widthOfColumns[1] = (availableColumns / 3) - sepLength;
    widthOfColumns[2] = (availableColumns / 2);
    return widthOfColumns;
}

int getMax(int *array, int size) {
    int max = 0;
    for (int i = 0; i < size; ++i) {
        if (max < array[i])
            max = array[i];
    }
    return max;
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
