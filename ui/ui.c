#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "rlutil.h"
#include "utf8.c"
#include "wcwidth.h"
#include <curl/curl.h>
#include "moodle.h"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen(".token", "r");
    char token[100];
    fread_line(f, token, 99);
    printf("%s", token);
    MDError err;
    MDClient *client = md_client_new(token, "https://emokymai.vu.lt", &err);
    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        printErr(msg);
        return 0;
    }

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
    Action action;

    while (action != ACTION_QUIT) {
        menuInfo->widths = getWidthsOfOptions(strlen(SEPERATOR));
        int *heights = getHeights(courses, menuInfo->HLOptions);
        int maxHeight = getMax(heights, 3);
        int terminalHeight = trows() - 1;
        if (maxHeight >= terminalHeight) {
            maxHeight = terminalHeight - 1;
        }

        locate(0, 0);
        menuInfo->currentMaxDepth = 0;
        printMenu(courses, menuInfo, maxHeight);

        do {
            int key = getkey();
            KeyDef keyDef = getKeyDef(key);
            action = getAction(courses, menuInfo, keyDef);
        } while (action == ACTION_INVALID);

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

void printMenu(MDArray courses, MenuInfo *menuInfo, int maxHeight) {
    MDArray topics = MD_COURSES(courses)[menuInfo->HLOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[menuInfo->HLOptions[1]].modules;
    MDModResource resource;
    int height;

    for (height = 0; height < maxHeight; ++height) {
        int depthIndex = -2;
        addOption(" ", menuInfo, ++depthIndex, 0); // first empty options column
        addMDArrayOption(courses, menuInfo, height, ++depthIndex);
        addMDArrayOption(topics, menuInfo, height, ++depthIndex);
        addMDArrayOption(modules, menuInfo, height, ++depthIndex);

        if (modules.len > 0 && MD_MODULES(modules)[menuInfo->HLOptions[2]].type == MD_MOD_RESOURCE) {
            if (height == 0)
                addOption("Files", menuInfo, ++depthIndex, menuInfo->HLOptions[3] == height);
            else
                addOption("", menuInfo, ++depthIndex, 0);

            resource = MD_MODULES(modules)[menuInfo->HLOptions[2]].contents.resource;
            if (resource.files.len > height) {
                addOption(MD_FILES(resource.files)[height].filename, menuInfo, ++depthIndex,
                          menuInfo->HLOptions[4] == height);
                if (menuInfo->depth == depthIndex) {
                    menuInfo->isCurrentFile = 1;
                }
                else
                    menuInfo->isCurrentFile = 0;
            }
            else
                addOption("", menuInfo, ++depthIndex, 0);
        }
        else {
            addOption("", menuInfo, ++depthIndex, 0);
            addOption("", menuInfo, ++depthIndex, 0);
        }
        addOption("", menuInfo, ++depthIndex, 0);
        printf("\n");
    }

    clean(tcols(), trows() - height - 1);
}

void addMDArrayOption(MDArray mdArray, MenuInfo *menuInfo, int heightIndex, int depthIndex) {
    char *name;
    if (mdArray.len > heightIndex) {
        name = getMDArrayName(mdArray, heightIndex, depthIndex);
    }
    else {
        if (heightIndex == 0)
            name = EMPTY_OPTION_NAME;
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
        case 108: // l
        case KEY_RIGHT:
        case 10: // enter
            keyDef = KD_RIGHT;
            break;
        case 106: // j
        case KEY_DOWN:
            keyDef = KD_DOWN;
            break;
        case 104: // h
        case KEY_LEFT:
            keyDef = KD_LEFT;
            break;
        case 107: // k
        case KEY_UP:
            keyDef = KD_UP;
            break;
        case 0:
            if (getkey() == 91) {
                key = getkey();
                switch (key) {
                    case 67: // arrow key right
                        keyDef = KD_RIGHT;
                        break;
                    case 66: // arrow key down
                        keyDef = KD_DOWN;
                        break;
                    case 65: // arrow key up
                        keyDef = KD_UP;
                        break;
                    case 68: // arrow key left
                        keyDef = KD_LEFT;
                }
            }
            break;
        case 113: // q
            keyDef = KD_QUIT;
            break;
        case 115: // s
            keyDef = KD_DOWNLOAD;
    }
    return keyDef;
}

Action getAction(MDArray courses, MenuInfo *menuInfo, KeyDef keyDef) {
    Action action;
    switch (keyDef) {
        case KD_RIGHT:
            if (menuInfo->depth < menuInfo->currentMaxDepth)
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
        case KD_DOWNLOAD:
            action = ACTION_DOWNLOAD;
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
            break;
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
    char msg[1000];
    if (!file) {
        sprintf(msg, "Couldn't open %s for writing", mdFile.filename);
        printErr(msg);
    }
    MDError err;
    sprintf(msg, "Downloading: %s [%ldK]", mdFile.filename, mdFile.filesize / 1000);
    notifySmall(msg, BLUE);
    md_client_download_file(client, &mdFile, file, &err);
    sprintf(msg, "Downloaded: %s [%ldK]", mdFile.filename, mdFile.filesize / 1000);
    notifySmall(msg, GREEN);
    fclose(file);

    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        printErr(msg);
    }
}

int *getWidthsOfOptions(int sepLength) {
    int *widthOfColumns = malloc(sizeof(int) * 3);
    int availableColumns = tcols();
    widthOfColumns[0] = (availableColumns / 6) - sepLength;
    widthOfColumns[1] = (availableColumns / 3) - sepLength;
    widthOfColumns[2] = (availableColumns / 2);
    return widthOfColumns;
}

void clean (int width, int height) {
    for (int i = 0; i < height; ++i) {
        printSpaces(width);
        printf("\n");
    }
}

void printSpaces(int count) {
    printf("%*s", count, "");
}

int getMax(int *array, int size) {
    int max = 0;
    for (int i = 0; i < size; ++i) {
        if (max < array[i])
            max = array[i];
    }
    return max;
}

void notifyBig(char *msg, int color) {
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

void notifySmall(char *msg, int color) {
    locate (0, trows());
    printSpaces(tcols());
    locate (0, trows());
    saveDefaultColor();
    setColor(color);
    printf(" %s ",  msg);
    resetColor();
}

void printErr(char *msg) {
    showcursor();
    locate(0, 0);
    saveDefaultColor();
    setBackgroundColor(RED);
    printf("Error: %s ", msg);
    resetColor();
    getch();
}

int fread_line(FILE *f, char *s, int n) {
    s[0] = '\0';
    int len = 0, cutOff = 0;
    char c = 0;
    do {
        c = fgetc(f);
        if (c != '\n' && !feof(f)) {
            if (len == n) cutOff = 1;

            if (len < n) s[len++] = c;
        }
    } while (!feof(f) && c != '\n');
    s[len] = '\0';
    return cutOff;
}
