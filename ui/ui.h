#include "moodle.h"
#ifndef UI_H
#define UI_H

#define EMPTY_OPTION_NAME "[empty]"
#define OPTION_CUT_STR "~"
#define SEPERATOR "  "
#define MAX_DEPTH 5

typedef enum Action {
    ACTION_INVALID,
    ACTION_QUIT,
    ACTION_GO_RIGHT,
    ACTION_GO_DOWN,
    ACTION_GO_LEFT,
    ACTION_GO_UP,
    ACTION_DOWNLOAD,
} Action;

typedef enum KeyDef {
    KD_RIGHT,
    KD_DOWN,
    KD_LEFT,
    KD_UP,
    KD_QUIT,
} KeyDef;

typedef struct menuInfo {
    int depth, *widths, HLOptions[MAX_DEPTH], currentMaxDepth, scrollOffset[MAX_DEPTH];
    _Bool isCurrentFile;
    MDClient *client;
} MenuInfo;

void mainLoop (MDArray courses, MenuInfo *menuInfo);
int *getHeights(MDArray courses, int *hl);
void printMenu(MDArray courses, MenuInfo *menuInfo, int height);
void addMDArrayOption(MDArray mdArray, MenuInfo *menuInfo, int heightIndex, int depthIndex);
char *getMDArrayName(MDArray mdArray, int heightIndex, int depthIndex);
void addOption(char *name, MenuInfo *menuInfo, int optionDepth, _Bool isHighlighted);
void printHighlightedOption(char *optionName, int width);
void printOption(char *optionName, int width);
KeyDef getKeyDef(int key);
Action getAction(MDArray courses, MenuInfo *menuInfo, KeyDef keyDef);
void doAction(MenuInfo *menuInfo, MDArray courses, int nrOfOptions, Action action);
void goRight(MenuInfo *menuInfo);
void goDown(int *highlightedOption, int nrOfOptions);
void goLeft(MenuInfo *menuInfo);
void goUp(int *highlightedOption, int nrOfOptions);
MDFile getMDFile(MDArray courses, MenuInfo *menuInfo);
void downloadFile(MDFile mdFile, MDClient *client);
void printSpaces(int count);
int *getWidthsOfOptions(int sepLength);
int getMax(int *array, int size);
void printErr(char *msg);
int fread_line(FILE *file, char *s, int n);

#endif