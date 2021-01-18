/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 * see json.h
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"
#include "utf8.h"

#define CONST_STRLEN(str) (sizeof(str) - 1)
#define TRUE_VALUE "true"
#define FALSE_VALUE "false"
#define NULL_VALUE "null"

// Triple is a helper enum when more then a boolean is needed.
typedef enum Triple {
    Yes,
    Maybe,
    No,
} Triple;

// JsonRet is the type returned by parsing functions which can hold both the
// error codes (< 0) and parsed byte count.
typedef long JsonRet;

// ignore_whitespace skips all the whitespace (according to is_whitespace) and
// returns pointer to the first non-whitespace byte.
const char *ignore_whitespace(const char *data);
bool is_whitespace(char token);
bool is_digit(char c);
bool is_positive_digit(char c);

// Functions to parse specific json types. Returns parsed byte count or error
// code (< 0). First byte should be the start if value, no whitespace is skiped
// at the beginning.

JsonRet json_parse_value(Json *json, const char *data);
JsonRet json_parse_object(JsonObject *object, const char *data);
JsonRet json_parse_array(JsonArray *array, const char *data);
JsonRet json_parse_string(JsonString *string, const char *data);
JsonRet json_parse_number(JsonNumber *number, const char *data);
JsonRet json_parse_boolean(JsonBoolean *boolean, const char *data);
JsonRet json_parse_null(const char *data);

// Cleanup functions for specific Json types. Objects themselves are not freed.

void json_value_cleanup(Json *json);
void json_object_cleanup(JsonObject *object);
void json_array_cleanup(JsonArray *array);
void json_string_cleanup(JsonString *string);

// Dynamic array management functions.

// array_grow reallocates given array to fit current length.
// @param array array to grow, valid pointer
// @param len pointer to wanted array length, set to 0 on failure
// @param cap pointer to current capacity, adjusted when reallocating / set to 0 on fail
// @param size size in bytes of single array element
// @return new pointer to array, NULL if failed
void *array_grow(void *array, JsonLength *len, JsonLength *cap, JsonLength size);

// array_append appends single value at the end of given array.
// @param array array to append, valid pointer
// @param len pointer to current array length, adjusted and set to 0 on failure
// @param cap pointer to current capacity, adjusted when reallocating / set to 0 on fail
// @param size size in bytes of single array element
// @param value valid pointer to the element to append
// @return new pointer to array, NULL if failed
void *array_append(void *array, JsonLength *len, JsonLength *cap, JsonLength size, const void *value);

bool is_whitespace(char token) {
    return token == ' ' || token == '\n' || token == '\r' || token == '\t';
}

const char *ignore_whitespace(const char *data) {
    while (*data && is_whitespace(*data)) {
        ++data;
    }
    return data;
}

JsonRet json_parse_boolean(JsonBoolean *boolean, const char *data) {
    if (strncmp(data, TRUE_VALUE, CONST_STRLEN(TRUE_VALUE)) == 0) {
        *boolean = true;
        return CONST_STRLEN(TRUE_VALUE);
    }
    if (strncmp(data, FALSE_VALUE, CONST_STRLEN(FALSE_VALUE)) == 0) {
        *boolean = false;
        return CONST_STRLEN(FALSE_VALUE);
    }
    return JSON_ERR_INVALID_VALUE;
}

JsonRet json_parse_null(const char *data) {
    return strncmp(data, NULL_VALUE, 4) == 0 ? CONST_STRLEN(NULL_VALUE) : JSON_ERR_INVALID_VALUE;
}

JsonRet json_parse_string(JsonString *string, const char *data) {
    const char *begin = data;
    ++data;
    bool escaped = false, unicode = false;
    JsonLength cap = 1;
    string->len = 0;
    string->str = malloc(cap);
    while (*data) {
        if (escaped) {
            char token = 0;
            switch (*data) {
                case '\\':
                    token = '\\';
                    break;
                case '\"':
                    token = '\"';
                    break;
                case '/':
                    token = '/';
                    break;
                case 'b':
                    token = '\b';
                    break;
                case 'f':
                    token = '\f';
                    break;
                case 'n':
                    token = '\n';
                    break;
                case 'r':
                    token = '\r';
                    break;
                case 't':
                    token = '\t';
                    break;
                case 'u':
                    unicode = true;
                    break;
                default:
                    return JSON_ERR_STRING_INVALID_ESCAPE;
            }
            if (token) {
                string->str = array_append(string->str, &string->len, &cap, sizeof(char), &token);
                if (!string->str)
                    return JSON_ERR_FAILED_ALLOCATION;
            }
            escaped = false;
            ++data;
        } else if (unicode) {
            char tmp[5];
            int len = 0;
            // TODO: check if correct.
            if (sscanf(data, "%4[0-9a-fA-F]%n", tmp, &len) != 1 || len != 4) {
                return JSON_ERR_STRING_INVALID_ESCAPE_DIGITS;
            }
            unsigned long long rune = strtoull(tmp, NULL, 16);
            char buff[6];
            int size = utf8encode(rune, buff);
            for (int i = 0; i < size; ++i) {
                string->str = array_append(string->str, &string->len, &cap, sizeof(char), buff + i);
                if (!string->str)
                    return JSON_ERR_FAILED_ALLOCATION;
            }
            // TODO: convert to utf-8?
            data += 4;
            unicode = false;
        } else {
            switch (*data) {
                case '\\':
                    escaped = true;
                    break;
                case '"':
                    string->str = realloc(string->str, string->len + 1);
                    if (string->str)
                        string->str[string->len] = 0;
                    return !string->len || string->str ? data - begin + 1 : JSON_ERR_FAILED_ALLOCATION;
                default:
                    string->str = array_append(string->str, &string->len, &cap, sizeof(char), data);
                    if (!string->str)
                        return JSON_ERR_FAILED_ALLOCATION;
            }
            ++data;
        }
    }
    return JSON_ERR_STRING_END_NOT_FOUND;
}

JsonRet json_parse_object(JsonObject *object, const char *data) {
    const char *begin = data;
    ++data;
    Triple needEntry = Maybe;
    JsonLength cap = 1;
    object->len = 0;
    object->entries = malloc(cap * sizeof(object->entries[0]));
    while (*data) {
        data = ignore_whitespace(data);
        if (*data == '}') {
            if (needEntry == Yes) {
                return JSON_ERR_OBJECT_UNEXPECTED_END;
            }
            object->entries = realloc(object->entries, sizeof(object->entries[0]) * object->len);
            return !object->len || object->entries ? data - begin + 1 : JSON_ERR_FAILED_ALLOCATION;
        }
        if (needEntry == No) {
            return JSON_ERR_OBJECT_UNEXPECTED_VALUE;
        }
        ++(object->len);
        object->entries = array_grow(object->entries, &object->len, &cap, sizeof(object->entries[0]));
        if (!object->entries) {
            return JSON_ERR_FAILED_ALLOCATION;
        }
        // In case the value does not get parsed and initialized.
        object->entries[object->len - 1].value.type = JSON_NULL;
        JsonRet offset = json_parse_string(&object->entries[object->len - 1].key, data);
        if (offset < 0) {
            return offset;
        }
        data = ignore_whitespace(data + offset);
        if (*(data++) != ':') {
            return JSON_ERR_OBJECT_MISSING_COLLON;
        }
        data = ignore_whitespace(data);
        offset = json_parse_value(&object->entries[object->len - 1].value, data);
        if (offset < 0) {
            return offset;
        }
        data = ignore_whitespace(data + offset);
        if (*data == ',') {
            ++data;
            needEntry = Yes;
        } else {
            needEntry = No;
        }
    }
    return JSON_ERR_OBJECT_END_NOT_FOUND;
}

void *array_grow(void *array, JsonLength *len, JsonLength *cap, JsonLength size) {
    if (*len >= *cap) {
        *cap *= 2;
        if (!*cap)
            *cap = 1;
        array = realloc(array, (*cap) * size);
    }
    if (!array) {
        *cap = *len = 0;
    }
    return array;
}

void *array_append(void *array, JsonLength *len, JsonLength *cap, JsonLength size, const void *value) {
    ++(*len);
    array = array_grow(array, len, cap, size);
    if (array) {
        memcpy((char *)array + ((*len) - 1) * size, value, size);
    }
    return array;
}

JsonRet json_parse_array(JsonArray *array, const char *data) {
    const char *begin = data;
    ++data;
    Triple needValue = Maybe;
    JsonLength cap = 1;
    array->len = 0;
    array->values = malloc(cap * sizeof(Json));

    while (*data) {
        data = ignore_whitespace(data);
        if (*data == ']') {
            if (needValue == Yes) {
                return JSON_ERR_ARRAY_UNEXPECTED_END;
            }
            array->values = realloc(array->values, sizeof(Json) * array->len);
            return !array->len || array->values ? data - begin + 1 : 0;
        }
        if (needValue == No) {
            return JSON_ERR_ARRAY_UNEXPECTED_VALUE;
        }
        ++(array->len);
        array->values = array_grow(array->values, &array->len, &cap, sizeof(array->values[0]));
        if (!array->values) {
            return JSON_ERR_FAILED_ALLOCATION;
        }
        // In case the value does not get parsed and initialized.
        array->values[array->len - 1].type = JSON_NULL;
        JsonRet offset = json_parse_value(&array->values[array->len - 1], data);
        if (offset < 0) {
            return offset;
        }
        data = ignore_whitespace(data + offset);
        if (*data == ',') {
            ++data;
            needValue = Yes;
        } else {
            needValue = No;
        }
    }
    return JSON_ERR_ARRAY_END_NOT_FOUND;
}

inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

inline bool is_positive_digit(char c) {
    return c >= '1' && c <= '9';
}

JsonRet json_parse_number(JsonNumber *number, const char *data) {
    const char *begin = data;
    if (*data == '-') {
        ++data;
    }
    if (*data == '0') {
        ++data;
    } else if (is_positive_digit(*data)) {
        ++data;
        while (is_digit(*data)) {
            ++data;
        }
    } else {
        return JSON_ERR_NUMBER_INVALID;
    }
    if (*data == '.') {
        ++data;
        if (!is_digit(*data)) {
            return JSON_ERR_NUMBER_INVALID_DECIMAL;
        }
        while (is_digit(*data)) {
            ++data;
        }
    }
    if (*data == 'e' || *data == 'E') {
        ++data;
        if (*data == '+' || *data == '-') {
            ++data;
        }
        if (!is_digit(*data)) {
            return JSON_ERR_NUMBER_INVALID_EXPONENT;
        }
        while (is_digit(*data)) {
            ++data;
        }
    }
    char *endPtr;
    // TODO: use locale-independent way and check for overflows.
    *number = strtold(begin, &endPtr);
    if (endPtr != data) {
        return JSON_ERR_NUMBER_INVALID;
    }
    return endPtr - begin;
}

JsonRet json_parse_value(Json *json, const char *data) {
    // In case the value does not get parsed and initialized.
    json->type = JSON_NULL;
    switch (*data) {
        case '{':
            json->type = JSON_OBJECT;
            return json_parse_object(&json->object, data);
        case '[':
            json->type = JSON_ARRAY;
            return json_parse_array(&json->array, data);
        case '"':
            json->type = JSON_STRING;
            return json_parse_string(&json->string, data);
        case 't':
        case 'f':
            json->type = JSON_BOOLEAN;
            return json_parse_boolean(&json->boolean, data);
        case 'n':
            json->type = JSON_NULL;
            return json_parse_null(data);
        default:
            if (*data == '-' || is_digit(*data)) {
                json->type = JSON_NUMBER;
                return json_parse_number(&json->number, data);
            }
            return JSON_ERR_INVALID_VALUE;
    }
}

JsonParseError json_parse(Json *json, const char *data) {
    data = ignore_whitespace(data);
    JsonRet offset = json_parse_value(json, data);
    if (offset < 0 || *ignore_whitespace(data + offset) != 0) {
        json_cleanup(json);
        return offset < 0 ? offset : JSON_ERR_TRAILING_DATA;
    }
    return JSON_ERR_OK;
}

void json_string_cleanup(JsonString *string) {
    free(string->str);
}

void json_object_cleanup(JsonObject *object) {
    if (object && object->entries) {
        for (JsonLength i = 0; i < object->len; ++i) {
            json_string_cleanup(&object->entries[i].key);
            json_value_cleanup(&object->entries[i].value);
        }
    }
    free(object->entries);
}

void json_array_cleanup(JsonArray *array) {
    if (array && array->values) {
        for (JsonLength i = 0; i < array->len; ++i) {
            json_value_cleanup(&array->values[i]);
        }
    }
    free(array->values);
}

void json_value_cleanup(Json *json) {
    switch (json->type) {
        case JSON_OBJECT:
            json_object_cleanup(&json->object);
            break;
        case JSON_ARRAY:
            json_array_cleanup(&json->array);
            break;
        case JSON_STRING:
            json_string_cleanup(&json->string);
            break;
        default:
            // Other types don't contain anything required to free.
            break;
    }
}

void json_cleanup(Json *json) {
    if (!json)
        return;
    json_value_cleanup(json);
    json->type = JSON_NULL;
}
