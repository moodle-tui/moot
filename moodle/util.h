#ifndef __UTIL_H
#define __UTIL_H

#include "json.h"

json_value *get_by_key(json_value *json, const char *key);
char *cloneStr(const char *s);
char *httpRequest(char *url);
char **httpMultiRequest(char *urls[], unsigned int size);

#endif
