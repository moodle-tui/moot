/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#ifndef __CFG_LOCAL_H
#define __CFG_LOCAL_H

#include "app.h"

#if defined(_WIN32)
#define PLATFORM_WIN
#define PATH_SEPERATOR '\\'
#define ENV_CONFIG_HOME "LOCALAPPDATA"
#else
#define PLATFORM_UNIX
#define PATH_SEPERATOR '/'
#define ENV_CONFIG_HOME "XDG_CONFIG_HOME"
#define ENV_HOME "HOME"
#define CONFIG_HOME_FOLDER ".config"
#endif

#define CONFIG_FOLDER "moot"
#define CONFIG_FILE "config"
#define CFG_SEPERATOR '='
#define MAX_CONFIG_PATH_LENGTH 4096

#define LINE_LIMIT 4096

static cchar * const properties[] = {
    "token",
    "upload_command",
};

typedef enum Property {
    PROPERTY_INVALID = -1,
    PROPERTY_TOKEN,
    PROPERTY_UPLOAD_COMMAND,
    NR_OF_PROPERTIES,
} Property;

void initConfigValues(ConfigValues *configValues, Message *msg);
char *getConfigPath(Message *msg);
FILE *openConfigFile(char *configPath, Message *msg);
char *joinPaths(char *string1, char *string2);
char *joinPaths(char *string1, char *string2);
void processLine(char *line, ConfigValues *configValues, Message *msg);
Property getProperty(char *propertyStr, Message *msg);
void skipSeperator(int *readPos);
void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, Message *msg);
char *sreadUntil(char *line, char mark, int limit, int *readPos, bool ignoreBlank);

#endif // __CFG_LOCAL_H

