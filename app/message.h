/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#ifndef __MESSAGE_H
#define __MESSAGE_H

#include "rlutil.h"
#include "stdbool.h"

typedef const char cchar;

#define MSG_LEN 4096
#define ERROR_MSG_INIT_STRING "Error: "

// success messages
#define MSG_DOWNLOADED "Succesfully downloaded %s"
#define MSG_UPLOADED "Succesfully uploaded %s files"

// bad action messages
#define MSG_NO_FILE_CHOSEN "No file chosen"
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
    MSG_TYPE_DISMISSED,
    MSG_TYPE_SUCCESS,
    MSG_TYPE_INFO,
    MSG_TYPE_BAD_ACTION,
    MSG_TYPE_WARNING,
    MSG_TYPE_ERROR,
} MsgType;

typedef enum MsgColors {
    MSG_COLOR_SUCCESS = GREEN,
    MSG_COLOR_INFO = BLUE,
    MSG_COLOR_BAD_ACTION = MAGENTA, 
    MSG_COLOR_WARNING = LIGHTRED,
    MSG_COLOR_ERROR = RED,
} MsgColors;

typedef struct Message {
    char *msg;
    MsgType type;
} Message;

void createMsg(Message *msg, cchar *content, cchar *details, MsgType type);

#endif // __MESSAGE_H
