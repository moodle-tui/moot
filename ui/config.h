#ifndef __CONFIG_H
#define __CONFIG_H

#if defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#define PLATFORM_UNIX
#define CONFIG_FOLDER "moot"
#define CONFIG_FILE "config"
#elif defined(_WIN32)
#define PLATFORM_WIN
#else
#define PLATFORM_UNKNOWN
#endif

#define ATTRIBUTE_MAX 20

static const char * const attributes[] = {
    "token",
};

typedef enum Attribute {
    ATTRIBUTE_INVALID = -1,
    ATTRIBUTE_TOKEN,
    NR_OF_ATTRIBUTES,
} Attribute;


int readConfigFile(char **token);

#endif // __CONFIG_H
