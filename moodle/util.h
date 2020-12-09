#ifndef __UTIL_H
#define __UTIL_H

#include "json.h"
#include "error.h"
#include "moodle.h"
#include <stdio.h>


#define MD_NEW_ARRAY() (MDArray){.len = 0, ._data = NULL}

// MDInitFunc is the callback called by md_array_init_new for each created element.
typedef void (*MDInitFunc)(void *, MDError *);

// MDInitFunc is the callback called by md_array_cleanup for each created element.
typedef void (*MDCleanupFunc)(void *);

// md_array_init_new initializes given array, optionaly calling callback for each created element
// (if callback isn't NULL). The array must be cleaned later up using md_array_cleanup.
void md_array_init_new(MDArray *array, size_t size, int length, MDInitFunc callback, MDError *error);

// md_array_cleanup frees and resets given array, optionaly calling callback for each array element
// (if callback isn't NULL).
void md_array_cleanup(MDArray *array, size_t size, MDCleanupFunc callback);

char *clone_str(const char *s);
char *cloneStrErr(const char *s, MDError *error);

void http_request_to_file(char *url, FILE *stream, MDError *error);
char *httpRequest(char *url, MDError *error);
char **httpMultiRequest(char *urls[], unsigned int size, MDError *error);
char *httpPostFile(const char *url, const char *filename, const char *name, MDError *error);

json_value *json_get_by_key(json_value *json, const char *key);
MDError assingJsonValues(json_value *json, const char *format, ...);

long jsonGetInteger(json_value *json, const char *key, MDError *error);
int jsonGetBool(json_value *json, const char *key, MDError *error);
char *jsonGetString(json_value *json, const char *key, MDError *error);
const char *jsonGetStringNoAlloc(json_value *json, const char *key, MDError *error);
json_value *jsonGetArray(json_value *json, const char *key, MDError *error);

#endif
