#ifndef __UTIL_H
#define __UTIL_H

#include "json.h"
#include "error.h"

char *cloneStr(const char *s);
char *httpRequest(char *url, ErrorCode *error);
char **httpMultiRequest(char *urls[], unsigned int size, ErrorCode *error);
char *httpPostFile(const char *url, const char *filename, const char *name, ErrorCode *error);

json_value *get_by_key(json_value *json, const char *key);
ErrorCode assingJsonValues(json_value *json, const char *format, ...);

#endif
