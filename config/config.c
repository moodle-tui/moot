#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "config.h"
#include "cfgLocal.h"

void readConfigFile(ConfigValues *configValues, CFGError *error) {
    *error = CFG_ERR_NONE;

    char *configPath = getConfigPath(error);
    if (*error)
        return;

    FILE *configFile = openConfigFile(configPath, error);
    if (*error)
        return;

    char *line;
    xmalloc((void **) &line, LINE_LIMIT * sizeof(char), error);
    if (!*error) {
        while (fgets(line, LINE_LIMIT, configFile)) {
            if (line[0] != '\n')
                processLine(line, configValues, error);
        }
        free(line);
    }
    fclose(configFile);
}

char *getConfigPath(CFGError *error) {
    char *sysConfigHome = getenv(CONFIG_HOME_ENV);
    if (!sysConfigHome) {
        *error = CFG_ERR_GET_ENV;
        return NULL;
    }
    char *configFolderPath = joinPaths(sysConfigHome, CONFIG_FOLDER);
    char *configPath = joinPaths(configFolderPath, CONFIG_FILE);
    return configPath;
}

FILE *openConfigFile(char *configPath, CFGError *error) {
    errno = 0;
    FILE *configFile = fopen(configPath, "a+");
    if (!configFile) {
        *error = CFG_ERR_OPEN_FILE;
        cfg_error_set_message(strerror(errno));
    }
    return configFile;
}

char *joinPaths(char *string1, char *string2) {
    int resultLength = strlen(string1) + strlen(string2);
    char *result = malloc(resultLength * sizeof(char));
    sprintf(result, "%s%c%s", string1, PATH_SEPERATOR, string2);
    return result;
}

void processLine(char *line, ConfigValues *configValues, CFGError *error) {
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

void xmalloc(void **var, size_t size, CFGError *error) {
    *var = malloc(size);
    if (!*var)
        *error = CFG_ERR_ALLOCATE;
}

char *sreadProperty(char *line, int *readPos, CFGError *error) {
    char *propertyStr = sreadUntil(line, SEPERATOR, LINE_LIMIT, readPos);
    ++readPos;
    if (!propertyStr) {
        *error = CFG_ERR_NO_VALUE;
        cfg_error_set_message(line);
    }
    return propertyStr;
}

Property getProperty(char *propertyStr, CFGError *error) {
    for (Property i = 0; i < NR_OF_PROPERTIES; ++i) {
        if (!strcmp(propertyStr, properties[i]))
            return i;
    }
    *error = CFG_ERR_WRONG_PROPERTY;
    cfg_error_set_message(propertyStr);
    return -1;
}

void skipSeperator(int *readPos) {
    ++*readPos;
}

void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, CFGError *error) {
    switch (property) {
        case PROPERTY_TOKEN:
            configValues->token = sreadUntil(line, '\n', LINE_LIMIT, readPos);
            if (!configValues->token)
                *error = CFG_ERR_NO_TOKEN;
            break;
        default:
            break;
    }
}

char *sreadUntil(char *line, char mark, int limit, int *readPos) {
    char *str = malloc(sizeof(char) * limit);
    int len = -1;
    for (; line[*readPos]; ++*readPos) {
        if (line[*readPos] == mark) {
            str[++len] = 0;
            return str;
        }
        if (!isblank(line[*readPos]))
            str[++len] = line[*readPos];
    }
    return NULL;
}


