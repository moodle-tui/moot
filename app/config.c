#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"

void readConfigFile(ConfigValues *configValues, Error *error) {
    *error = ERR_NONE;

    char *configPath = getConfigPath(error);
    if (*error)
        return;

    FILE *configFile = openConfigFile(configPath, error);
    if (*error)
        return;

    char *line = xmalloc(LINE_LIMIT * sizeof(char), error);
    if (!*error) {
        while (fgets(line, LINE_LIMIT, configFile)) {
            if (line[0] != '\n')
                processLine(line, configValues, error);
        }
        free(line);
    }
    fclose(configFile);
}

char *getConfigPath(Error *error) {
    char *sysConfigHome = getenv(CONFIG_HOME_ENV);
    if (!sysConfigHome) {
        *error = CFG_ERR_GET_ENV;
        return NULL;
    }
    char *configFolderPath = joinPaths(sysConfigHome, CONFIG_FOLDER);
    char *configPath = joinPaths(configFolderPath, CONFIG_FILE);
    return configPath;
}

FILE *openConfigFile(char *configPath, Error *error) {
    errno = 0;
    FILE *configFile = fopen(configPath, "a+");
    if (!configFile) {
        *error = CFG_ERR_OPEN_FILE;
        app_error_set_message(strerror(errno));
    }
    return configFile;
}

char *joinPaths(char *string1, char *string2) {
    int resultLength = strlen(string1) + strlen(string2);
    char *result = malloc(resultLength * sizeof(char));
    sprintf(result, "%s%c%s", string1, PATH_SEPERATOR, string2);
    return result;
}

void processLine(char *line, ConfigValues *configValues, Error *error) {
    int readPos = 0;
    char *propertyStr = sreadProperty(line, &readPos, error);
    if (*error)
        return;

    Property property = getProperty(propertyStr, error);
    if (*error)
        return;

    skipSeperator(&readPos);
    sreadValue(configValues, property, line, &readPos, error);
}

char *sreadProperty(char *line, int *readPos, Error *error) {
    char *propertyStr = sreadUntil(line, CFG_SEPERATOR, LINE_LIMIT, readPos, 1);
    ++readPos;
    if (!propertyStr) {
        *error = CFG_ERR_NO_VALUE;
        app_error_set_message(line);
    }
    return propertyStr;
}

Property getProperty(char *propertyStr, Error *error) {
    for (Property i = 0; i < NR_OF_PROPERTIES; ++i) {
        if (!strcmp(propertyStr, properties[i]))
            return i;
    }
    *error = CFG_ERR_WRONG_PROPERTY;
    app_error_set_message(propertyStr);
    return -1;
}

void skipSeperator(int *readPos) {
    ++*readPos;
}

void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, Error *error) {
    switch (property) {
        case PROPERTY_TOKEN:
            configValues->token = sreadUntil(line, '\n', LINE_LIMIT, readPos, 1);
            if (!configValues->token)
                *error = CFG_ERR_NO_TOKEN;
            break;
        case PROPERTY_UPLOAD_COMMAND:
            configValues->uploadCommand = sreadUntil(line, '\n', LINE_LIMIT, readPos, 0);
        default:
            break;
    }
}

char *sreadUntil(char *line, char mark, int limit, int *readPos, bool ignoreBlank) {
    char *str = malloc(sizeof(char) * limit);
    int len = -1;
    for (; line[*readPos]; ++*readPos) {
        if (line[*readPos] == mark) {
            str[++len] = 0;
            return str;
        }
        if (((ignoreBlank && !isblank(line[*readPos])) || !ignoreBlank) && line[*readPos])
            str[++len] = line[*readPos];
    }
    return NULL;
}


