#ifndef __CFG_LOCAL_H
#define __CFG_LOCAL_H

#include "config.h"

#if defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define PLATFORM_UNIX
#define PATH_SEPERATOR '/'
#define CONFIG_HOME_ENV "XDG_CONFIG_HOME"
#elif defined(_WIN32)
#define PLATFORM_WIN
#define PATH_SEPERATOR '\\'
#define CONFIG_HOME_ENV "LOCALAPPDATA"
#else
#define PLATFORM_UNKNOWN
#endif

#define CONFIG_FOLDER "moot"
#define CONFIG_FILE "config"
#define SEPERATOR '='

#define LINE_LIMIT 4096

static cchar * const properties[] = {
    "token",
};

typedef enum Property {
    PROPERTY_INVALID = -1,
    PROPERTY_TOKEN,
    NR_OF_PROPERTIES,
} Property;

char *getConfigPath(CFGError *error);
char *getSysConfigHome();
FILE *openConfigFile(char *configPath, CFGError *error);
char *joinPaths(char *string1, char *string2);
void processLine(char *line, ConfigValues *configValues, CFGError *error);
void xmalloc(void **var, size_t size, CFGError *error);
char *sreadProperty(char *line, int *readPos, CFGError *error);
Property getProperty(char *propertyStr, CFGError *error);
void skipSeperator(int *readPos);
void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, CFGError *error);
char *sreadToken(char *line, int *readPos, CFGError *error);
char *sreadUntil(char *line, char mark, int limit, int *readPos);

// cfgError.c
void cfg_error_set_message(cchar *message);

#endif // __CFG_LOCAL_H

