#ifndef __CFG_LOCAL_H
#define __CFG_LOCAL_H

#include "app.h"

#if defined(_WIN32)
#define PLATFORM_WIN
#define PATH_SEPERATOR '\\'
#define CONFIG_HOME_ENV "LOCALAPPDATA"
#else
#define PLATFORM_UNIX
#define PATH_SEPERATOR '/'
#define CONFIG_HOME_ENV "XDG_CONFIG_HOME"
#endif

#define CONFIG_FOLDER "moot"
#define CONFIG_FILE "config"
#define CFG_SEPERATOR '='

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

void initConfigValues(ConfigValues *configValues);
char *getConfigPath(Message *msg);
FILE *openConfigFile(char *configPath, Message *msg);
char *joinPaths(char *string1, char *string2);
char *joinPaths(char *string1, char *string2);
void processLine(char *line, ConfigValues *configValues, Message *msg);
Property getProperty(char *propertyStr, Message *msg);
void skipSeperator(int *readPos);
void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, Message *msg);
char *sreadToken(char *line, int *readPos, Error *error);
char *sreadUntil(char *line, char mark, int limit, int *readPos, bool ignoreBlank);

#endif // __CFG_LOCAL_H

