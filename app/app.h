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

#define DEFAULT_UPLOAD_COMMAND "tempFile=`mktemp` && lf -selection-path $tempFile && cat $tempFile && rm $tempFile"
#define UPLOAD_FILE_LENGTH 2048

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
void validateAction(Action *action, MDArray courses, int *highlightedOptions, int depth, int currentMaxDepth);

int getDepthHeight(int depth, MDArray courses, int *highlightedOptions);

void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions,
        int *depth, int *scrollOffsets, char *uploadCommand, Message *msg);

// following functions change some values, to navigate the menu
void goRight(int *depth);
void goDown(int *highlightedOption, int nrOfOptions, int terminalHeight, int *scrollOffset);
void goLeft(int *depth, int *highlightedOptions);
void goUp(int *highlightedOption, int nrOfOptions, int terminalHeight, int *scrollOffset);

void resetNextDepth(int *highlightedOptions, int depth, int *scrollOffsets);

void downloadFile(MDClient *client, MDArray modules, int *highlightedOptions, int depth, Message *msg);

// getMDFile returns currently highlighted mdFile
void getMDFile(MDFile *mdFile, MDArray modules, int *highlightedOptions, int depth, Message *msg);

void uploadFiles(MDClient *client, int depth, MDArray modules, int *highlightedOptions, char *uploadCommand, Message *msg);
void checkIfAssignmentOrWorkshop(MDArray modules, int *highlightedOptions, int depth, Message *msg);
void getFileNames(MDArray *fileNames, char *uploadCommand, Message *msg);
FILE *openFileSelectionProcess(char *uploadCommand, Message *msg);
void readFileNames(FILE *uploadPathsPipe, MDArray *fileNames, Message *msg);
void removeNewline(char *string);
void startUpload(MDClient *client, MDModule module, MDArray fileNames, Message *msg);
void setUploadSuccessMsg(int nrOfFilesUploaded, Message *msg);

// ui.c

#define FIRST_WIDTH_DIVISOR 6
#define SECOND_WIDTH_DIVISOR 3
#define THIRD_WIDTH_DIVISOR 2

#define EMPTY_OPTION_NAME "[empty]"
#define OPTION_CUT_STR "~"
#define SEPERATOR "  "
#define NR_OF_WIDTHS 3
#define SCROLLOFF 5

#define DESCRIPTION_NAME "Description"
#define FILES_NAME "Files"
#define UNSUPPORTED_DESCRIPTION_FORMAT "[Unsupported description format]"

typedef enum Depth {
    INIT_DEPTH = -1,
    COURSES_DEPTH,
    TOPICS_DEPTH,
    MODULES_DEPTH,
    MODULE_DEPTH1,
    MODULE_DEPTH2,
    LAST_DEPTH,
} Depth;

typedef enum OptionType {
    OPTION_TYPE_NONE,
    OPTION_TYPE_EMPTY,
    OPTION_TYPE_OPTION,
    OPTION_TYPE_LINE,
} OptionType;

typedef enum ModuleContentHeight {
    DESCRIPTION_HEIGHT,
    FILES_HEIGHT,
} ModuleContentHeight;

typedef struct optionCoordinates {
    int height;
    Depth depth;
} OptionCoordinates;

typedef struct Layout {
    int heights[LAST_DEPTH], *widths;
} Layout;

typedef union OptionContent {
    char *option;
    Line line;
} OptionContent;

typedef struct Option {
    OptionType type;
    OptionContent content;
} Option;

// mainLoop prints all the information and reacts to user input, until user decides to quit
void mainLoop(MDArray courses, MDClient *client, char *uploadCommand, Message *msg, Message *prevMsg);
void savePrevMessage(Message *msg, Message *prevMsg);
void restorePrevMessage(Message *msg, Message *prevMsg);
int getNrOfRecurringMessages(Message msg, Message *prevMsg, Action action);

// printMenu prints menu, saves height of current depth and returns menu size,
// depth in it being the last visible depth
OptionCoordinates printMenu(MDArray courses, int *highlightedOptions, int depth, int *scrollOffsets, Message *msg);

void getDescriptionLines(WrappedLines *lines, MDArray courses, int *highlightedOptions, int width, Message *msg);

void getOption(Option *option, MDArray courses, WrappedLines descriptionLines, OptionCoordinates printPos,
        int *highlightedOptions, int *scrollOffsets, int width, Message *msg);

// getMDArrayName returns mdArray option name or EMPTY_OPTION_NAME
void getMDArrayOption(Option *option, MDArray mdArray, int height, int depthIndex, Message *msg);

void getModuleContentOption(Option *option, MDArray modules, WrappedLines descriptionLines,
        OptionCoordinates printPos, int *highlightedOptions, int width, Message *msg);

void getModuleDepth1Option(Option *option, MDArray modules, int *highlightedOptions, OptionCoordinates printPos);

void getModuleDepth2Option(Option *option, MDArray modules, WrappedLines descriptionLines,
        int *highlightedOptions, OptionCoordinates printPos);

bool checkIfHighlighted(Option option, int *highlightedOptions, OptionCoordinates printPos);

// addOption checks if option is visible and if it is, prints it as highlighted option or just
// option, depending on whether isHighlighted equals true
void addOption(Option option, _Bool isHighlighted, int widthIndex, int width);

// printHighlightedOption changes colors, calls printOption, and resets the colors
void printHighlightedOption(Option option, int width);

void printOption(Option option, int width);

// printCharArray prints option name and fills left space with spaces until width is reached.
// If there'is any utf8 characters this function gets their true width and size, and fills
// the available space respectively
void printCharArray(char *name, int width);

// fills specified width and height with spaces
void clean(int width, int height);

// getWidths get gets three different widths for menu layout, depending on how
// much space is currently available in the terminal
int *getWidths();

void printSpaces(int count);

// getMax gets max value from array
int getMax(int *array, int size);

// util.c

void *xmalloc(size_t size, Message *msg);
void *xrealloc(void *data, size_t size, Message *msg);
void *xcalloc(size_t n, size_t size, Message *msg);
char *getStr(int n);
int getNrOfDigits(int number);
void setHtmlRenders(MDArray *courses, Message *msg);
MDRichText *getModuleDescription(MDModule *module);
void setHtmlRender(MDRichText *description, Message *msg);

// config.c

typedef struct ConfigValues {
    char *token;
    char *uploadCommand;
} ConfigValues;

void readConfigFile(ConfigValues *configValues, Message *msg);

// input.c

char *getInput(char *inputMsg, Message *msg);
char *readLine(char *line, Message *msg);

// main.c

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, Message *msg);
void terminate(MDClient *client, MDArray courses, Message *msg, Message *prevMsg);

#endif // __APP_H
