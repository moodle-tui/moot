#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_ERR_MSG_SIZE 2048

typedef const char cchar;

typedef enum CFGError {
    CFG_ERR_NONE = 0,
    CFG_ERR_GET_ENV,
    CFG_ERR_OPEN_FILE,
    CFG_ERR_EMPTY_PROPERTY,
    CFG_ERR_NO_VALUE,
    CFG_ERR_WRONG_PROPERTY,
    CFG_ERR_NO_TOKEN,
    CFG_ERR_ALLOCATE,
} CFGError;

typedef struct ConfigValues {
    char *token;
} ConfigValues;

cchar *cfg_error_get_message(CFGError code);

void readConfigFile(ConfigValues *configValues, CFGError *error);

#endif // __CONFIG_H
