/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"

void readConfigFile(ConfigValues *configValues, Message *msg) {
    initConfigValues(configValues, msg);
    if (msg->type == MSG_TYPE_WARNING || msg->type == MSG_TYPE_ERROR)
        return;
    char *configPath = getConfigPath(msg);
    if (msg->type == MSG_TYPE_WARNING || msg->type == MSG_TYPE_ERROR)
        return;

    FILE *configFile = openConfigFile(configPath, msg);
    if (msg->type == MSG_TYPE_WARNING || msg->type == MSG_TYPE_ERROR)
        return;

    char *line = xmalloc(LINE_LIMIT * sizeof(char), msg);
    if (msg->type != MSG_TYPE_WARNING && msg->type != MSG_TYPE_ERROR) {
        while (fgets(line, LINE_LIMIT, configFile)) {
            if (line[0] != '\n')
                processLine(line, configValues, msg);
        }
        free(line);
    }
    fclose(configFile);
}

void initConfigValues(ConfigValues *configValues, Message *msg) {
    configValues->uploadCommand = xcalloc(LINE_LIMIT, sizeof(char), msg);
    if (msg->type != MSG_TYPE_ERROR)
        configValues->token = xcalloc(LINE_LIMIT, sizeof(char), msg);
    if (msg->type != MSG_TYPE_ERROR)
        configValues->site = xcalloc(LINE_LIMIT, sizeof(char), msg);
}

char *getConfigPath(Message *msg) {
    char *sysConfigHome;
    sysConfigHome = getenv(ENV_CONFIG_HOME);
    if (!sysConfigHome) {
#ifdef PLATFORM_UNIX
        char *sysHome = getenv(ENV_HOME);
        if (!sysHome) {
            createMsg(msg, MSG_CANNOT_GET_ENV, NULL, MSG_TYPE_ERROR);
            return NULL;
        }
        sysConfigHome = joinPaths(sysHome, CONFIG_HOME_FOLDER);
#else
        sysConfigHome = joinPaths(sysHome, CONFIG_HOME_FOLDER);
        createMsg(msg, MSG_CANNOT_GET_ENV, NULL, MSG_TYPE_ERROR);
        return NULL;
#endif
    }
    char *configFolderPath = joinPaths(sysConfigHome, CONFIG_FOLDER);
    char *configPath = joinPaths(configFolderPath, CONFIG_FILE);
    return configPath;
}

FILE *openConfigFile(char *configPath, Message *msg) {
    errno = 0;
    FILE *configFile = fopen(configPath, "a+");
    if (errno) {
        createMsg(msg, MSG_CANNOT_OPEN_CONFIG_FILE, strerror(errno), MSG_TYPE_ERROR);
    }
    return configFile;
}

char *joinPaths(char *string1, char *string2) {
    int resultLength = strlen(string1) + strlen(string2);
    char *result = malloc(resultLength * sizeof(char));
    sprintf(result, "%s%c%s", string1, PATH_SEPERATOR, string2);
    return result;
}

void processLine(char *line, ConfigValues *configValues, Message *msg) {
    int readPos = 0;
    char *propertyStr = sreadUntil(line, CFG_SEPERATOR, LINE_LIMIT, &readPos, 1);

    Property property = getProperty(propertyStr, msg);
    if (msg->type == MSG_TYPE_WARNING || msg->type == MSG_TYPE_ERROR)
        return;

    skipSeperator(&readPos);
    sreadValue(configValues, property, line, &readPos, msg);
}

Property getProperty(char *propertyStr, Message *msg) {
    for (Property i = 0; i < NR_OF_PROPERTIES; ++i) {
        if (!strcmp(propertyStr, properties[i]))
            return i;
    }
    createMsg(msg, MSG_WRONG_CFG_PROPERTY, propertyStr, MSG_TYPE_WARNING);
    return -1;
}

void skipSeperator(int *readPos) {
    ++*readPos;
}

void sreadValue(ConfigValues *configValues, Property property, char *line, int *readPos, Message *msg) {
    switch (property) {
        case PROPERTY_SITE:
            configValues->site = sreadUntil(line, '\n', LINE_LIMIT, readPos, 1);
            if (!configValues->site[0])
                createMsg(msg, MSG_NO_SITE, NULL, MSG_TYPE_ERROR);
            break;
        case PROPERTY_FILE_SELECTION_COMMAND:
            configValues->token = sreadUntil(line, '\n', LINE_LIMIT, readPos, 1);
            if (!configValues->token[0])
                createMsg(msg, MSG_NO_TOKEN, NULL, MSG_TYPE_ERROR);
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

