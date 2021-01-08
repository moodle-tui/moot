#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"
#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include <curl/curl.h>
#include "moodle.h"
#include "config.h"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen("/home/ramojus/studijos/.token", "r");
    char *token;
    if (readConfigFile(&token) == -1)
        getchar();
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

    hidecursor();
    cls();
    mainLoop(courseArr, client);
    cls();
    showcursor();

    md_courses_cleanup(courseArr);
    md_client_cleanup(client);
    fclose(f);
    curl_global_cleanup();
}

void mainLoop (MDArray courses, MDClient *client) {
    Action action;
    int depth = 0, highlightedOptions[LAST_DEPTH] = {0};
    int prevHeight = 0;
    int scrollOffsets[LAST_DEPTH] = {0};

    while (action != ACTION_QUIT) {
        OptionCoordinates menuSize;
        locate(0, 0);
        menuSize = printMenu(courses, highlightedOptions, depth, scrollOffsets);

        if (prevHeight > menuSize.height)
            clean(tcols(), prevHeight - menuSize.height);
        prevHeight = menuSize.height;

        int depthHeight = getDepthHeight(depth, courses, highlightedOptions);
        do {
            int key = getkey();
            KeyDef keyDef = getKeyDef(key);
            action = getAction(courses, keyDef, depth, menuSize.depth, depthHeight);
        } while (action == ACTION_INVALID);

        doAction(action, courses, client, highlightedOptions, &depth, scrollOffsets);
    }
}

OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets) {
    OptionCoordinates menuSize;
    OptionCoordinates printPos;
    menuSize.depth = 0;
    int *widths = getWidths(strlen(SEPERATOR));
    char *name;
    int emptyRowOptions = 0;
    int terminalHeight = trows() - 1;

    for (printPos.height = 0; emptyRowOptions < NR_OF_WIDTHS && printPos.height < terminalHeight; ++printPos.height) {
        emptyRowOptions = 0;
        for (printPos.depth = INIT_DEPTH + depth; printPos.depth < NR_OF_WIDTHS + depth - 1; ++printPos.depth) {
            name = getName(courses, printPos, highlightedOptions, scrollOffsets);

            _Bool isHighlighted = checkIfHighlighted(name, highlightedOptions, printPos);
            int widthIndex = printPos.depth - depth + 1;
            int width = widths[widthIndex];
            addOption(name, printPos.depth, isHighlighted, widthIndex, width);

            if (!strcmp(name, ""))
                ++emptyRowOptions;

            if (isHighlighted && menuSize.depth < printPos.depth)
                menuSize.depth = printPos.depth;
        }
        printf("\n");
    }
    menuSize.height = printPos.height;
    return menuSize;
}

char *getName(MDArray courses, OptionCoordinates printPos, int *highlightedOptions, int *scrollOffsets) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    printPos.height += scrollOffsets[printPos.depth];
    char *name;
    switch (printPos.depth) {
        case COURSES_DEPTH:
            name = getMDArrayName(courses, printPos.height, printPos.depth);
            break;
        case TOPICS_DEPTH:
            name = getMDArrayName(topics, printPos.height, printPos.depth);
            break;
        case MODULES_DEPTH:
            name = getMDArrayName(modules, printPos.height, printPos.depth);
            break;
        case MODULE_CONTENTS_DEPTH1:
        case MODULE_CONTENTS_DEPTH2:
            name = getModuleContentName(modules, printPos, highlightedOptions);
            break;
        default:
            name = "";
            break;
    }
    if (!strcmp(name, "") && printPos.height == 0 && printPos.depth != INIT_DEPTH
            && printPos.depth != LAST_DEPTH)
        name = EMPTY_OPTION_NAME;

    return name;
}

_Bool checkIfHighlighted(char *name, int *highlightedOptions, OptionCoordinates printPos) {
    if (!strcmp(name, "") || !strcmp(name, EMPTY_OPTION_NAME))
        return 0;
    else
        return highlightedOptions[printPos.depth] == printPos.height;
}

char *getMDArrayName(MDArray mdArray, int height, int depthIndex) {
    char *name;
    if (mdArray.len > height) {
        switch (depthIndex) {
            case 0:
                name = MD_COURSES(mdArray)[height].name;
                break;
            case 1:
                name = MD_TOPICS(mdArray)[height].name;
                break;
            case 2:
                name = MD_MODULES(mdArray)[height].name;
                break;
            default:
                printErr("Wrong depth index\n");
        }
    }
    else
        name = "";

    return name;
}

char *getModuleContentName(MDArray modules, OptionCoordinates printPos, int *highlightedOptions) {
    MDModResource resource;
    MDModType moduleType = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].type;

    if (modules.len > 0 && moduleType == MD_MOD_RESOURCE) {
        resource = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].contents.resource;
        switch (printPos.depth) {
            case MODULE_CONTENTS_DEPTH1:
                switch (printPos.height) {
                    case 0:
                        return "files";
                        break;
                    default:
                        return "";
                }
                break;
            case MODULE_CONTENTS_DEPTH2:
                if (resource.files.len > printPos.height)
                    return MD_FILES(resource.files)[printPos.height].filename;
                break;
            default:
                break;
        }
    }
    return "";
}

void addOption(char *name, int optionDepth, _Bool isHighlighted, int widthIndex, int width) {

    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", SEPERATOR);

    if (name == NULL) {
        printErr("Invalid option name. Expected string, got NULL");
    }
    if (isHighlighted) {
        printHighlightedOption(name, width);
    }
    else
        printOption(name, width);
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

int getDepthHeight(int depth, MDArray courses, int *highlightedOptions) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    int height;
    switch (depth) {
        case COURSES_DEPTH:
            height = courses.len;
            break;
        case TOPICS_DEPTH:
            height = topics.len;
            break;
        case MODULES_DEPTH:
            height = modules.len;
            break;
        case MODULE_CONTENTS_DEPTH1:
            height = 1;
        case MODULE_CONTENTS_DEPTH2:
            if (modules.len > 0 && MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].type == MD_MOD_RESOURCE)
                height = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].contents.resource.files.len;
            else
                height = -1;
            break;
        default:
            height = -1;
    }
    return height;
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

Action getAction(MDArray courses, KeyDef keyDef, int depth, int currentMaxDepth, int depthHeight) {
    Action action;
    switch (keyDef) {
        case KD_RIGHT:
            if (depth < currentMaxDepth)
                action = ACTION_GO_RIGHT;
            else
                action = ACTION_INVALID;
            break;
        case KD_DOWN:
            if (depthHeight == 1)
                action = ACTION_INVALID;
            else 
                action = ACTION_GO_DOWN;
            break;
        case KD_LEFT:
            if (depth != 0)
                action = ACTION_GO_LEFT;
            else
                action = ACTION_INVALID;
            break;
        case KD_UP:
            if (depthHeight == 1)
                action = ACTION_INVALID;
            else
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

void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions, int *depth, int *scrollOffsets) {
    int depthHeight = getDepthHeight(*depth, courses, highlightedOptions) - 1;
    MDFile mdFile;
    int maxHeight = trows() - 2;
    switch (action) {
        case ACTION_GO_RIGHT:
            goRight(depth);
            break;
        case ACTION_GO_DOWN:
            goDown(&highlightedOptions[*depth], depthHeight, maxHeight, &scrollOffsets[*depth]);
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            break;
        case ACTION_GO_LEFT:
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            goLeft(depth, highlightedOptions);
            break;
        case ACTION_GO_UP:
            goUp(&highlightedOptions[*depth], depthHeight, maxHeight, &scrollOffsets[*depth]);
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            break;
        case ACTION_DOWNLOAD:
            mdFile = getMDFile(courses, highlightedOptions);
            downloadFile(mdFile, client);
            break;
        default:
            break;
    }
}

void goRight(int *depth) {
    ++*depth;
}

void goDown(int *highlightedOption, int depthHeight, int maxHeight, int *scrollOffset) {
    if (*highlightedOption + *scrollOffset == depthHeight) {
        *highlightedOption = 0;
        *scrollOffset = 0;
    }
    else {
        if (*highlightedOption >= maxHeight - SCROLLOFF && depthHeight - *scrollOffset > maxHeight)
            ++*scrollOffset;
        else
            ++*highlightedOption;
    }
}

void goLeft(int *depth, int *highlightedOptions) {
    --*depth;
}

void goUp(int *highlightedOption, int depthHeight, int maxHeight, int *scrollOffset) {
    if (*highlightedOption + *scrollOffset == 0) {
        if (depthHeight > maxHeight) {
            *scrollOffset = depthHeight - maxHeight;
            *highlightedOption = maxHeight;
        }
        else
            *highlightedOption = depthHeight;
    }
    else {
        if (*scrollOffset != 0 && *highlightedOption <= SCROLLOFF)
            --*scrollOffset;
        else
            --*highlightedOption;
    }
}

void resetNextDepth(int *highlightedOptions, int depth, int *scrollOffsets) {
    if (depth < LAST_DEPTH - 1) {
        highlightedOptions[depth + 1] = 0;
        scrollOffsets[depth + 1] = 0;
    }
}

MDFile getMDFile(MDArray courses, int *highlightedOptions) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[1]].modules;
    MDFile mdFile;
    if (modules.len > 0 && MD_MODULES(modules)[highlightedOptions[2]].type == MD_MOD_RESOURCE) {
        MDModResource resource;
        resource = MD_MODULES(modules)[highlightedOptions[2]].contents.resource;
        mdFile = MD_FILES(resource.files)[highlightedOptions[4]];
    }
    else
        notifySmall("Couldn't get file", RED);

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

int *getWidths(int sepLength) {
    int *widthOfColumns = malloc(sizeof(int) * NR_OF_WIDTHS);
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
