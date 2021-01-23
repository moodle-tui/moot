#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"

void mainLoop(MDArray courses, MDClient *client, char *uploadCommand) {
    Action action;
    int depth = 0, highlightedOptions[LAST_DEPTH] = {0};
    int scrollOffsets[LAST_DEPTH] = {0};
    Message msg, prevMsg;
    msgInit(&msg);
    msgInit(&prevMsg);
    OptionCoordinates menuSize;
    menuSize = printMenu(courses, highlightedOptions, depth, scrollOffsets);

    while (action != ACTION_QUIT) {
        if (kbhit()) {
            int key = getkey();
            action = getAction(key);
            validateAction(&action, courses, highlightedOptions, depth, menuSize.depth);
            if (action != ACTION_INVALID) {
                doAction(action, courses, client, highlightedOptions, &depth, scrollOffsets, uploadCommand, &msg);
            }
            cls();
            int nrOfRecurringMessages = getNrOfRecurringMessages(msg, &prevMsg, action);
            printMsg(msg, nrOfRecurringMessages);
            locate(0, 0);
            menuSize = printMenu(courses, highlightedOptions, depth, scrollOffsets);
        }
    }
    free(msg.msg);
    free(prevMsg.msg);
}

OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets) {
    OptionCoordinates menuSize;
    OptionCoordinates printPos;
    menuSize.depth = 0;
    int *widths = getWidths();
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

int *getWidths() {
    int sepLength = strlen(SEPERATOR);
    int *widthOfColumns = malloc(sizeof(int) * NR_OF_WIDTHS);
    int availableColumns = tcols();
    widthOfColumns[0] = (availableColumns / 6) - sepLength;
    widthOfColumns[1] = (availableColumns / 3) - sepLength;
    widthOfColumns[2] = (availableColumns / 2) + 1;
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

