/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rlutil.h"
#include "utf8.h"
#include "wcwidth.h"
#include "app.h"
#include "html_renderer.h"

void mainLoop(MDArray courses, MDClient *client, char *uploadCommand, Message *msg, Message *prevMsg) {
    Action action;
    int depth = 0, highlightedOptions[LAST_DEPTH] = {0};
    int scrollOffsets[LAST_DEPTH] = {0};
    OptionCoordinates menuSize;
    menuSize = printMenu(courses, highlightedOptions, depth, scrollOffsets, msg);

    while (action != ACTION_QUIT) {
        if (kbhit()) {
            savePrevMessage(msg, prevMsg);
            msg->type = MSG_TYPE_NONE;
            int key = getkey();
            action = getAction(key);
            validateAction(&action, courses, highlightedOptions, depth, menuSize.depth);
            if (action != ACTION_INVALID) {
                doAction(action, courses, client, highlightedOptions, &depth, scrollOffsets, uploadCommand, msg);
            }

            cls();
            if (msg->type == MSG_TYPE_NONE)
                restorePrevMessage(msg, prevMsg);
            int nrOfRecurringMessages = getNrOfRecurringMessages(*msg, prevMsg, action);
            printMsg(*msg, nrOfRecurringMessages);

            locate(0, 0);
            savePrevMessage(msg, prevMsg);
            msg->type = MSG_TYPE_NONE;
            menuSize = printMenu(courses, highlightedOptions, depth, scrollOffsets, msg);
            if (msg->type == MSG_TYPE_ERROR)
                return;
            restorePrevMessage(msg, prevMsg);
        }
    }
}

void savePrevMessage(Message *msg, Message *prevMsg) {
    createMsg(prevMsg, msg->msg, NULL, msg->type);
}

void restorePrevMessage(Message *msg, Message *prevMsg) {
    createMsg(msg, prevMsg->msg, NULL, prevMsg->type);
}

int getNrOfRecurringMessages(Message msg, Message *prevMsg, Action action) {
    static Action originalAction = -1;
    static int nrOfRecurringMessages = 0;
    if (originalAction == -1 && msg.type != MSG_TYPE_NONE)
        originalAction = action;
    if (msgCompare(*prevMsg, msg) == -1) {
        originalAction = action;
        createMsg(prevMsg, msg.msg, NULL, msg.type);
        nrOfRecurringMessages = 0;
    }
    else if (originalAction == action && msg.type != MSG_TYPE_NONE && msg.type != MSG_TYPE_DISMISSED)
        ++nrOfRecurringMessages;
    return nrOfRecurringMessages;
}

OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets, Message *msg) {
    OptionCoordinates menuSize;
    OptionCoordinates printPos;
    menuSize.depth = 0;
    int *widths = getWidths();
    int emptyOptions = 0;
    int terminalHeight = trows() - 1;
    WrappedLines descriptionLines = {
        .count = 0,
        .lines = malloc(sizeof(Line *)),
    };

    if (depth == MODULE_DEPTH1 && highlightedOptions[depth] == DESCRIPTION_HEIGHT) {
        getDescriptionLines(&descriptionLines, courses, highlightedOptions, widths[NR_OF_WIDTHS - 1], msg);
        if (msg->type == MSG_TYPE_ERROR)
            return menuSize;
    }
    for (printPos.height = 0; emptyOptions < NR_OF_WIDTHS && printPos.height < terminalHeight; ++printPos.height) {
        emptyOptions = 0;
        for (printPos.depth = INIT_DEPTH + depth; printPos.depth < NR_OF_WIDTHS + depth - 1; ++printPos.depth) {
            int widthIndex = printPos.depth - depth + 1;
            int width = widths[widthIndex];
            Option option = {.type = OPTION_TYPE_NONE, .content.option = NULL};

            getOption(&option, courses, descriptionLines, printPos, highlightedOptions, scrollOffsets, width, msg);
            if (msg->type == MSG_TYPE_ERROR)
                return menuSize;
            bool isHighlighted = checkIfHighlighted(option, highlightedOptions, printPos);
            addOption(option, isHighlighted, widthIndex, width);
            if (option.type == OPTION_TYPE_NONE)
                ++emptyOptions;

            if (isHighlighted && menuSize.depth < printPos.depth)
                menuSize.depth = printPos.depth;
        }
        printf("\n");
    }
    menuSize.height = printPos.height;

    if (descriptionLines.count)
        freeWrappedLines(descriptionLines);
    free(widths);

    return menuSize;
}

void getDescriptionLines(WrappedLines *lines, MDArray courses, int *highlightedOptions, int width, Message *msg) {
        MDModule *module = &MD_MODULES(MD_TOPICS(MD_COURSES(courses)
                    [highlightedOptions[COURSES_DEPTH]].topics)
                    [highlightedOptions[TOPICS_DEPTH]].modules)
                    [highlightedOptions[MODULES_DEPTH]];

        MDRichText *description = getModuleDescription(module);
        HtmlRender *render = description->html_render;
        *lines = wrapHtmlRender(*render, width, msg);
}

void getOption(Option *option, MDArray courses, WrappedLines descriptionLines, OptionCoordinates printPos,
        int *highlightedOptions, int *scrollOffsets, int width, Message *msg) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    printPos.height += scrollOffsets[printPos.depth];
    switch (printPos.depth) {
        case COURSES_DEPTH:
            getMDArrayOption(option, courses, printPos.height, printPos.depth, msg);
            break;
        case TOPICS_DEPTH:
            getMDArrayOption(option, topics, printPos.height, printPos.depth, msg);
            break;
        case MODULES_DEPTH:
            getMDArrayOption(option, modules, printPos.height, printPos.depth, msg);
            break;
        case MODULE_DEPTH1:
        case MODULE_DEPTH2:
            getModuleContentOption(option, modules, descriptionLines, printPos, highlightedOptions, width, msg);
            break;
        default:
            break;
    }
    if (option->type == OPTION_TYPE_NONE && printPos.height == 0 && printPos.depth != INIT_DEPTH
            && printPos.depth != LAST_DEPTH) {
        option->type = OPTION_TYPE_EMPTY;
        option->content.option = EMPTY_OPTION_NAME;
    }
}

void getMDArrayOption(Option *option, MDArray mdArray, int height, int depthIndex, Message *msg) {
    if (mdArray.len > height) {
        option->type = OPTION_TYPE_OPTION;
        switch (depthIndex) {
            case 0:
                option->content.option = MD_COURSES(mdArray)[height].name;
                break;
            case 1:
                option->content.option = MD_TOPICS(mdArray)[height].name;
                break;
            case 2:
                option->content.option = MD_MODULES(mdArray)[height].name;
                break;
        }
    }
}

void getModuleContentOption(Option *option, MDArray modules, WrappedLines descriptionLines,
        OptionCoordinates printPos, int *highlightedOptions, int width, Message *msg) {
    switch (printPos.depth) {
        case MODULE_DEPTH1:
            getModuleDepth1Option(option, modules, highlightedOptions, printPos);
            break;
        case MODULE_DEPTH2:
            getModuleDepth2Option(option, modules, descriptionLines, highlightedOptions, printPos);
            break;
        default:
            break;
    }
}

void getModuleDepth1Option(Option *option, MDArray modules, int *highlightedOptions, OptionCoordinates printPos) {
    MDModule module = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]];
    switch (printPos.height) {
        case DESCRIPTION_HEIGHT:
            option->content.option = DESCRIPTION_NAME;
            option->type = OPTION_TYPE_OPTION;
            break;
        case FILES_HEIGHT:
            if (modules.len > 0 && module.type == MD_MOD_RESOURCE) {
                option->content.option = FILES_NAME;
                option->type = OPTION_TYPE_OPTION;
            }
            break;
        default:
            break;
    }
}

void getModuleDepth2Option(Option *option, MDArray modules, WrappedLines descriptionLines,
        int *highlightedOptions, OptionCoordinates printPos) {
    MDModule module = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]];
    MDModResource resource;
    switch(highlightedOptions[MODULE_DEPTH1]) {
        case DESCRIPTION_HEIGHT:
            option->type = OPTION_TYPE_LINE;
            MDRichText *description = getModuleDescription(&module);
            if (description->format == MD_FORMAT_HTML) {
                if (descriptionLines.count > printPos.height)
                    option->content.line = descriptionLines.lines[printPos.height];
            }
            else if (printPos.height == 0)
                option->content.line = (Line) {UNSUPPORTED_DESCRIPTION_FORMAT,
                    strlen(UNSUPPORTED_DESCRIPTION_FORMAT)};
            break;
        case FILES_HEIGHT:
            resource = module.contents.resource;
            if (resource.files.len > printPos.height) {
                option->content.option = MD_FILES(resource.files)[printPos.height].filename;
                option->type = OPTION_TYPE_OPTION;
            }
            break;
    }
}

bool checkIfHighlighted(Option option, int *highlightedOptions, OptionCoordinates printPos) {
    if (option.type == OPTION_TYPE_NONE || option.type == OPTION_TYPE_LINE
            || option.type == OPTION_TYPE_EMPTY)
        return 0;
    else
        return highlightedOptions[printPos.depth] == printPos.height;
}

void addOption(Option option, _Bool isHighlighted, int widthIndex, int width) {
    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", SEPERATOR);

    if (isHighlighted)
        printHighlightedOption(option, width);
    else
        printOption(option, width);
}

void printHighlightedOption(Option option, int width) {
    saveDefaultColor();
    setBackgroundColor(7);
    setColor(0);
    printOption(option, width);
    resetColor();
}

void printOption(Option option, int width) {
    switch(option.type) {
        case OPTION_TYPE_OPTION:
        case OPTION_TYPE_EMPTY:
            printCharArray(option.content.option, width);
            break;
        case OPTION_TYPE_NONE:
            printSpaces(width);
            break;
        case OPTION_TYPE_LINE:
            fwrite(option.content.line.text, sizeof(char), option.content.line.length, stdout);
            break;
        default:
            break;
    }
}

void printCharArray(char *name, int width) {
    int printedChWidth = 0, printedChSize = 0;
    int optionLength = strlen(name);

    printf(" ");
    width -= 2;
    while (1) {
        Rune u;
        size_t charSize = utf8decode(name + printedChSize, &u, optionLength);
        int charWidth = wcwidth(u);

        if (!name[printedChSize]) {
            printSpaces(width - printedChWidth);
            break;
        }
        else if (printedChWidth + charWidth > width - 1 && name[printedChSize + charSize]) {
            printSpaces(width - printedChWidth - 1);
            printf("~");
            ++printedChWidth;
            break;
        }

        fwrite(name + printedChSize, charWidth, charSize, stdout);
        printedChWidth += charWidth;
        printedChSize += charSize;
    }
    printf(" ");
}

int *getWidths() {
    int sepLength = strlen(SEPERATOR);
    int *widthOfColumns = malloc(sizeof(int) * NR_OF_WIDTHS);
    int availableColumns = tcols();
    float leftOvers = (float)availableColumns / (float)FIRST_WIDTH_DIVISOR - availableColumns / FIRST_WIDTH_DIVISOR
        + (float)availableColumns / (float)SECOND_WIDTH_DIVISOR - availableColumns / SECOND_WIDTH_DIVISOR
        + (float)availableColumns / (float)THIRD_WIDTH_DIVISOR - availableColumns / THIRD_WIDTH_DIVISOR;

    widthOfColumns[0] = (availableColumns / FIRST_WIDTH_DIVISOR) - sepLength;
    widthOfColumns[1] = (availableColumns / SECOND_WIDTH_DIVISOR) - sepLength;
    widthOfColumns[2] = (availableColumns / THIRD_WIDTH_DIVISOR) + leftOvers + 0.1;
    return widthOfColumns;
}

void clean(int width, int height) {
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

