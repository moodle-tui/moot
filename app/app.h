#ifndef __APP_H
#define __APP_H

#include "moodle.h"
#include "rlutil.h"

typedef const char cchar;

// message.c

#define MSG_LEN 4096
#define ERROR_MSG_INIT_STRING "Error: "

// success messages
#define MSG_DOWNLOADED "Succesfully downloaded %s"
#define MSG_UPLOADED "Succesfully uploaded %s files"

// bad action messages
#define MSG_NO_FILES_CHOSEN "No files chosen"
#define MSG_NOT_ASSIGNMENT_OR_WORKSHOP "This is not an assignment or a workshop"
#define MSG_NOT_FILE "This is not a file"

// warning messages
#define MSG_NO_CFG_VALUE "No value found for: %s"
#define MSG_WRONG_CFG_PROPERTY "No property named %s"

// error messages
#define MSG_CANNOT_GET_ENV "Couldn't find required environment variables for your system"
#define MSG_CANNOT_OPEN_CONFIG_FILE "Couldn't open config file: %s"
#define MSG_NO_TOKEN "No token found in config file"
#define MSG_CANNOT_ALLOCATE "Cannot allocate memory"
#define MSG_CANNOT_OPEN_DOWNLOAD_FILE "Cannot open file for download: %s"
#define MSG_CANNOT_EXEC_UPLOAD_CMD "Couldn't execute upload command: %s"

typedef enum MsgType {
    MSG_TYPE_NONE,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_BAD_ACTION,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
} MsgType;

typedef enum MsgColors {
    MSG_COLOR_SUCCESS = GREEN,
    MSG_COLOR_INFO = BLUE,
    MSG_COLOR_BAD_ACTION = GREY, 
    MSG_COLOR_WARNING = LIGHTRED,
    MSG_COLOR_ERROR = RED,
} MsgColors;

typedef struct Message {
    char *msg;
    MsgType type;
} Message;

void createMsg(Message *msg, cchar *content, cchar *details, MsgType type);
void msgInit(Message *msg);
bool checkIfMsgBad(Message msg);
void printMsg(Message msg, int nrOfRecurringMessages);
int msgCompare(Message msg1, Message msg2);
void printMsgNoUI(Message msg);

// error.c

#define ERR_MSG_SIZE 2048

typedef enum Error {
    ERR_NONE = 0,
    ERR_ALLOCATE,
    CFG_ERR_GET_ENV,
    CFG_ERR_OPEN_FILE,
    CFG_ERR_EMPTY_PROPERTY,
    CFG_ERR_NO_VALUE,
    CFG_ERR_WRONG_PROPERTY,
    CFG_ERR_NO_TOKEN,
    UPLOAD_ERR_EXEC,
    UPLOAD_ERR_NO_FILES_CHOSEN,
    UPLOAD_ERR_WRONG_MODULE,
} Error;

void setErrorMsg(cchar *message);

cchar *getErrorMsg(Error error);
cchar *getAnyErrorMsg(Error error, MDError mdError);
void printErr(cchar *msg);

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
void mainLoop(MDArray courses, MDClient *client, char *uploadCommand);

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
int *getWidths();

void printSpaces(int count);

// getMax gets max value from array
int getMax(int *array, int size);

// action.c

#define UPLOAD_COMMAND "tempFile=`mktemp` && lf -selection-path $tempFile && cat $tempFile && rm $tempFile"
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

// getMDFile returns currently highlighted mdFile
MDFile getMDFile(MDArray modules, int *highlightedOptions, int depth, Message *msg);

void downloadFile(MDFile mdFile, MDClient *client, Message *msg);

void uploadFiles(MDClient *client, int depth, MDArray modules, int *highlightedOptions, char *uploadCommand, Message *msg);
FILE *openFileSelectionProcess(char *uploadCommand, Message *msg);
void removeNewline(char *string);
void startUpload(MDClient *client, MDModule module, MDArray fileNames, Message *msg);

// util.c

char *getStr(int n);
int getNrOfDigits(int number);
void *xmalloc(size_t size, Message *msg);
int getNrOfRecurringMessages(Message msg, Message *prevMsg, Action action);

// config.c

typedef struct ConfigValues {
    char *token;
    char *uploadCommand;
} ConfigValues;

void readConfigFile(ConfigValues *configValues, Message *msg);

// main.c

void initialize(MDClient **client, MDArray *courses, ConfigValues *configValues, MDError *mdError);
void terminate(MDClient *client, MDArray courses, Message *msg);

#endif // __APP_H
