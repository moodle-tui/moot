/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#ifndef __APP_H
#define __APP_H

#include "moodle.h"
#include "rlutil.h"
#include "message.h"
#include "html_renderer.h"

typedef const char cchar;

// message.c

void msgInit(Message *msg);
bool checkIfAbort(Message msg);
void printMsg(Message msg, int nrOfRecurringMessages);
int msgCompare(Message msg1, Message msg2);
void printMsgNoUI(Message msg);

// action.c

typedef enum Action {
    ACTION_INVALID = -1,
    ACTION_GO_RIGHT,
    ACTION_GO_DOWN,
    ACTION_GO_LEFT,
    ACTION_GO_UP,
    ACTION_DISMISS_MSG,
    ACTION_UPLOAD,
    ACTION_DOWNLOAD,
    ACTION_QUIT,
} Action;

Action getAction(int key);
// TODO: move validation for upload and download from their functions to this
void validateAction(Action *action, MDArray courses, int *highlightedOptions, int depth, int currentMaxDepth);
void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions,
        int *depth, int *scrollOffsets, char *uploadCommand, Message *msg);

// ui.c

#define SEPERATOR "  "
#define SCROLLOFF 5

typedef enum Depth {
    INIT_DEPTH = -1,
    COURSES_DEPTH,
    TOPICS_DEPTH,
    MODULES_DEPTH,
    MODULE_DEPTH1,
    MODULE_DEPTH2,
    LAST_DEPTH,
} Depth;

typedef enum ModuleContentHeight {
    DESCRIPTION_HEIGHT,
    FILES_HEIGHT,
} ModuleContentHeight;

typedef struct optionCoordinates {
    int height;
    Depth depth;
} OptionCoordinates;

void mainLoop(MDArray courses, MDClient *client, char *uploadCommand, Message *msg, Message *prevMsg);
int getMax(int *array, int size);

// option.c

typedef enum OptionType {
    OPTION_TYPE_NONE,
    OPTION_TYPE_EMPTY,
    OPTION_TYPE_OPTION,
    OPTION_TYPE_LINE,
} OptionType;

typedef union OptionContent {
    char *option;
    Line line;
} OptionContent;

typedef struct Option {
    OptionType type;
    OptionContent content;
} Option;

bool checkIfHighlighted(Option option, int *highlightedOptions, OptionCoordinates printPos);
void getOption(Option *option, MDArray courses, WrappedLines descriptionLines, OptionCoordinates printPos,
        int *highlightedOptions, int *scrollOffsets, int width, Message *msg);
void addOption(Option option, _Bool isHighlighted, int widthIndex, int width);

// util.c

void *xmalloc(size_t size, Message *msg);
void *xrealloc(void *data, size_t size, Message *msg);
void *xcalloc(size_t n, size_t size, Message *msg);
char *getStr(int n);
int getNrOfDigits(int number);
void printSpaces(int count);
void setHtmlRenders(MDArray *courses, Message *msg);
MDRichText *getModuleDescription(MDModule *module);
void setHtmlRender(MDRichText *description, Message *msg);

// config.c

typedef struct ConfigValues {
    char *site;
    char *token;
    char *uploadCommand;
} ConfigValues;

void readConfigFile(ConfigValues *configValues, Message *msg);

// input.c

#define INPUT_ENTER_TITLE "Enter title:"

char *getInput(char *inputMsg, Message *msg);

// main.c

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, Message *msg);
void terminate(MDClient *client, MDArray courses, Message *msg, Message *prevMsg);

#endif // __APP_H

