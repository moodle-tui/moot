#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "string.h"

static char *getSysConfigHome();
static char *joinPaths(char *first, char *second);
static char *sreadUntil(char *line, char mark, int *readPos);
static Attribute getAttribute(char *attributeStr);

int readConfigFile(char **token) {
    char *sysConfigHome = getSysConfigHome();
    if (!sysConfigHome) {
        printf("couldn't find required environment variables for your system\n");
        return -1;
    }
    char *configFolderPath = joinPaths(sysConfigHome, CONFIG_FOLDER);
    struct stat st = {0};
    if (stat(configFolderPath, &st) == -1) {
        if (mkdir(configFolderPath, 0700) == -1) {
            printf("couldn't create moot config folder\n");
            return -1;
        }
    }
    char *configPath = joinPaths(configFolderPath, CONFIG_FILE);
    errno = 0;
    FILE *configFile = fopen(configPath, "a+");

    if (!configFile) {
        printf("couldn't open config file: %s, error: %s\n", configPath, strerror(errno));
        return -1;
    }

    char configLine[100] = {0};
    fgets(configLine, 100, configFile);
    if (!configLine[0]) {
        printf("no attribute\n");
        return -1;
    }

    int readPos = 0;
    char *attributeStr = sreadUntil(configLine, '=', &readPos);
    ++readPos;
    if (!attributeStr) {
        printf("no value found for: %s\n", configLine);
        return -1;
    }

    Attribute attribute = getAttribute(attributeStr);

    switch (attribute) {
        case ATTRIBUTE_TOKEN:
            *token = sreadUntil(configLine, '\n', &readPos);
            break;
        case ATTRIBUTE_INVALID:
            printf("no attribute named %s", attributeStr);
            return -1;
            break;
        default:
            printf("attribute type error");
            return -1;
    }

    if (!*token) {
        printf("no token found in config file\n");
        return -1;
    }

    return 0;
}

char *getSysConfigHome() {
    #if defined (PLATFORM_UNIX)
    char *sysConfigHome = getenv("XDG_CONFIG_HOME");
    #elif defined (PLATFORM_WIN)
    char *sysConfigHome = getenv("LOCALAPPDATA");
    #endif
    return sysConfigHome;
}

char *joinPaths(char *first, char *second) {
    int resultLength = strlen(first) + strlen(second);
    char *result = malloc(resultLength * sizeof(char));
    sprintf(result, "%s/%s", first, second);
    return result;
}

char *sreadUntil(char *line, char mark, int *readPos) {
    char *str = malloc(sizeof(char) * 1024);
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

Attribute getAttribute(char *attributeStr) {
    for (Attribute i = 0; i < NR_OF_ATTRIBUTES; ++i) {
        if (!strcmp(attributeStr, attributes[i]))
            return i;
    }
    return ATTRIBUTE_INVALID;
}

