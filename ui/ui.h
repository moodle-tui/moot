#ifndef UI_H
#define UI_H

#include <stdio.h>
#include "moodle.h"

#define EMPTY_OPTION_NAME "[empty]"
#define OPTION_CUT_STR "~"
#define SEPERATOR "  "
#define DOWNLOAD_STARTED_MSG_COLOR BLUE
#define DOWNLOAD_FINISHED_MSG_COLOR GREEN
#define NO_FILE_TO_DOWNLOAD_MSG_COLOR RED
#define NR_OF_WIDTHS 3

typedef enum Depth {
    INIT_DEPTH = -1,
    COURSES_DEPTH = 0,
    TOPICS_DEPTH = 1,
    MODULES_DEPTH = 2,
    MODULE_CONTENTS_DEPTH1 = 3,
    MODULE_CONTENTS_DEPTH2 = 4,
    LAST_DEPTH = 5,
} Depth;

typedef enum Action {
    ACTION_INVALID = 0,
    ACTION_DOWNLOAD = 1,
    ACTION_QUIT = 2,
    ACTION_GO_RIGHT = 3,
    ACTION_GO_DOWN = 4,
    ACTION_GO_LEFT = 5,
    ACTION_GO_UP = 6,
} Action;

typedef enum KeyDef {
    KD_RIGHT,
    KD_DOWN,
    KD_LEFT,
    KD_UP,
    KD_DOWNLOAD,
    KD_QUIT,
} KeyDef;

//typedef struct menuInfo {
//    int depth, *widths, HLOptions[MAX_DEPTH], currentMaxDepth, scrollOffset[MAX_DEPTH];
//    _Bool isCurrentFile;
//    MDClient *client;
//} MenuInfo;

typedef struct optionCoordinates {
    int height;
    Depth depth;
} OptionCoordinates;

typedef struct Layout {
    int heights[LAST_DEPTH], *widths;
} Layout;

// mainLoop prints all the information and reacts to user input, until user decides to quit
void mainLoop (MDArray courses, MDClient *client);

// printMenu prints menu, saves height of current depth and returns menu size,
// depth in it being the last visible depth
OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int *depthHeight, int depth);

char *getName(MDArray courses, OptionCoordinates printPos, int *highlightedOptions);

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

// getKeyDef returns KeyDef equivalent of a pressed key, so that vi keys are also supported.
// KEY_* macros are from rlutil.h lib, and they are not working for me but, I left them just
// in case.
KeyDef getKeyDef(int key);

Action getAction(MDArray courses, KeyDef keyDef, int depth, int currentMaxDepth);

void doAction(Action action, MDArray courses, MDClient *client, int nrOfOptions, int *highlightedOptions, int *depth);

// following functions change some values, to navigate the menu
void goRight(int *depth);
void goDown(int *highlightedOption, int nrOfOptions);
void goLeft(int *depth, int *highlightedOptions);
void goUp(int *highlightedOption, int nrOfOptions);

// getMDFile returns currently highlighted mdFile
MDFile getMDFile(MDArray courses, int *highlightedOptions);

void downloadFile(MDFile mdFile, MDClient *client);

// fills specified width and height with spaces
void clean (int width, int height);

void printSpaces(int count);

// getWidths get gets three different widths for menu layout, depending on how
// much space is currently available in the terminal
int *getWidths(int sepLength);

// getMax gets max value from array
int getMax(int *array, int size);

// notifyBig prints box with msg in the middle of the screen, waits for keypress,
// then disappears. Box borders are printed in the color that is passed.
void notifyBig(char *msg, int color);

// notifySmall prints msg in bottom left corner. msg text is printed in the color
// that is passed.
void notifySmall(char *msg, int color);

// printErr prints msg in red background, waits for key press, then disappears
void printErr(char *msg);

// fread_line reads single line up to n bytes from file with \n removed. s should be at
// least n + 1 in length. 1 is returned if the line is too long and was cut off.
int fread_line(FILE *file, char *s, int n);

#endif
