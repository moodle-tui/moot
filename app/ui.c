#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"
#include "config.h"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    ConfigValues configValues;
    Error error;
    readConfigFile(&configValues, &error);
    if (error) {
        msgErr(app_error_get_message(error));
        return 0;
    }
    MDError mdError;
    MDClient *client = md_client_new(configValues.token, "https://emokymai.vu.lt", &mdError);
    if (mdError) {
        msgErr(md_error_get_message(mdError));
        return 0;
    }

    md_client_init(client, &mdError);
    if (mdError) {
        msgErr(md_error_get_message(mdError));
        return 0;
    }
    MDArray courseArr = md_client_fetch_courses(client, &mdError);
    if (mdError) {
        msgErr(md_error_get_message(mdError));
        return 0;
    }

    hidecursor();
    cls();
    mainLoop(courseArr, client, configValues.uploadCommand);
    cls();
    showcursor();

    md_courses_cleanup(courseArr);
    md_client_cleanup(client);
    curl_global_cleanup();
}

void mainLoop (MDArray courses, MDClient *client, char *uploadCommand) {
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

        doAction(action, courses, client, highlightedOptions, &depth, scrollOffsets, uploadCommand);
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
        msgErr("Invalid option name. Expected string, got NULL");
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
