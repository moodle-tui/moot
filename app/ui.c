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

#define FIRST_WIDTH_DIVISOR 6
#define SECOND_WIDTH_DIVISOR 3
#define THIRD_WIDTH_DIVISOR 2

#define NR_OF_WIDTHS 3

typedef struct Layout {
    int heights[LAST_DEPTH], *widths;
} Layout;

void savePrevMessage(Message *msg, Message *prevMsg);
void restorePrevMessage(Message *msg, Message *prevMsg);
int getNrOfRecurringMessages(Message msg, Message *prevMsg, Action action);
OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets, Message *msg);
void getDescriptionLines(WrappedLines *lines, MDArray courses, int *highlightedOptions, int width, Message *msg);
// fills specified width and height with spaces
void clean(int width, int height);
// getWidths get gets three different widths for menu layout, depending on how
// much space is currently available in the terminal
int *getWidths();

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

int getMax(int *array, int size) {
    int max = 0;
    for (int i = 0; i < size; ++i) {
        if (max < array[i])
            max = array[i];
    }
    return max;
}

