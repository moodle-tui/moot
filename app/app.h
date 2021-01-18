#ifndef __APP_H
#define __APP_H

#include "moodle.h"

typedef const char cchar;

// ui.c

#define EMPTY_OPTION_NAME "[empty]"
#define OPTION_CUT_STR "~"
#define SEPERATOR "  "
#define NR_OF_WIDTHS 3
#define SCROLLOFF 5

typedef enum Depth {
    INIT_DEPTH = -1,
    COURSES_DEPTH = 0,
    TOPICS_DEPTH = 1,
    MODULES_DEPTH = 2,
    MODULE_CONTENTS_DEPTH1 = 3,
    MODULE_CONTENTS_DEPTH2 = 4,
    LAST_DEPTH = 5,
} Depth;

typedef struct optionCoordinates {
    int height;
    Depth depth;
} OptionCoordinates;

typedef struct Layout {
    int heights[LAST_DEPTH], *widths;
} Layout;

// mainLoop prints all the information and reacts to user input, until user decides to quit
void mainLoop (MDArray courses, MDClient *client, char *uploadCommand);

// printMenu prints menu, saves height of current depth and returns menu size,
// depth in it being the last visible depth
OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets);

char *getName(MDArray courses, OptionCoordinates printPos, int *highlightedOptions, int *scrollOffsets);

_Bool checkIfHighlighted(char *name, int *highlightedOptions, OptionCoordinates printPos);

// getMDArrayName returns mdArray option name or EMPTY_OPTION_NAME
char *getMDArrayName(MDArray mdArray, int height, int depthIndex);

char *getModuleContentName(MDArray modules, OptionCoordinates printPos, int *highlightedOptions);

int filterDepth(int optionDepth, int depth);

// add option checks if option is visible and if it is, prints it as highlighted option or just
// option, depending on whether isHighlighted equals true
void addOption(char *name, int optionDepth, _Bool isHighlighted, int widthIndex, int width);

// printHighlightedOption changes colors, calls printOption, and resets the colors
void printHighlightedOption(char *optionName, int width);

// printOption prints option name and fills left space with spaces until width is reached.
// If there'is any utf8 characters this function gets their true width and size, and fills
// the available space respectively
void printOption(char *optionName, int width);

// fills specified width and height with spaces
void clean (int width, int height);

// getWidths get gets three different widths for menu layout, depending on how
// much space is currently available in the terminal
int *getWidths(int sepLength);

void printSpaces(int count);

// getMax gets max value from array
int getMax(int *array, int size);

// input.c

typedef enum KeyDef {
    KD_RIGHT,
    KD_DOWN,
    KD_LEFT,
    KD_UP,
    KD_DOWNLOAD,
    KD_QUIT,
} KeyDef;

// getKeyDef returns KeyDef equivalent of a pressed key, so that vi keys are also supported.
// KEY_* macros are from rlutil.h lib and are not working for me, but I left them just in case.
// TODO: figure out if KEY_* macros do something on other systems
KeyDef getKeyDef(int key);

// action.c

typedef enum Action {
    ACTION_INVALID = -1,
    ACTION_GO_RIGHT,
    ACTION_GO_DOWN,
    ACTION_GO_LEFT,
    ACTION_GO_UP,
    ACTION_DOWNLOAD,
    ACTION_QUIT,
} Action;

Action getAction(MDArray courses, KeyDef keyDef, int depth, int currentMaxDepth, int depthHeight);

int getDepthHeight(int depth, MDArray courses, int *highlightedOptions);

void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions, int *depth, int *scrollOffsets, char *uploadCommand);

// following functions change some values, to navigate the menu
void goRight(int *depth);
void goDown(int *highlightedOption, int nrOfOptions, int terminalHeight, int *scrollOffset);
void goLeft(int *depth, int *highlightedOptions);
void goUp(int *highlightedOption, int nrOfOptions, int terminalHeight, int *scrollOffset);

void resetNextDepth(int *highlightedOptions, int depth, int *scrollOffsets);

// getMDFile returns currently highlighted mdFile
MDFile getMDFile(MDArray courses, int *highlightedOptions);

void downloadFile(MDFile mdFile, MDClient *client);

// msg.c

#define DOWNLOAD_STARTED_MSG_COLOR BLUE
#define DOWNLOAD_FINISHED_MSG_COLOR GREEN
#define NO_FILE_TO_DOWNLOAD_MSG_COLOR RED

// notifyBig prints box with msg in the middle of the screen, waits for keypress,
// then disappears. Box borders are printed in the color that is passed.
void msgBig(cchar *msg, int color);

// notifySmall prints msg in bottom left corner. msg text is printed in the color
// that is passed.
void msgSmall(cchar *msg, int color);

// printErr prints msg in red background, waits for key press, then disappears
void msgErr(cchar *msg);

// config.c

#define CFG_ERR_MSG_SIZE 2048

typedef enum Error {
    CFG_ERR_NONE = 0,
    CFG_ERR_GET_ENV,
    CFG_ERR_OPEN_FILE,
    CFG_ERR_EMPTY_PROPERTY,
    CFG_ERR_NO_VALUE,
    CFG_ERR_WRONG_PROPERTY,
    CFG_ERR_NO_TOKEN,
    CFG_ERR_ALLOCATE,
} Error;

typedef struct ConfigValues {
    char *token;
    char *uploadCommand;
} ConfigValues;

cchar *app_error_get_message(Error code);

void readConfigFile(ConfigValues *configValues, Error *error);

#endif // __APP_H