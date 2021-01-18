/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * Yet another JSON (https://json.org) parser. While it is usable, this parser
 * was made to exercise coding and there are better JSON parsers around. The
 * main flaw of this parser is that it is recursively implemented and thus will
 * probably crash if the level of nested objects/arrays reaches very high
 * number. 
 *
 * Besides this, currently it does not validate utf-8 at all, and no number
 * overflow is checked. Standart C library funcions are used to parse numbers,
 * thus it is locale-dependent.
 *
 * Because not specified in the standard and due to the current implementation
 * duplicate keys in objects are allowed.
 *
 * The api is not stable and may change in the future. Other than flaws
 * mentioned above, this is a fully working and standard-compliant JSON parser.
 */

#ifndef __JSON_H
#define __JSON_H
#include <limits.h>

// JsonType holds the possible types of Json value.
typedef enum JsonType {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOLEAN,
    JSON_NULL,
} JsonType;

typedef struct Json Json;

// JsonLength is the length type used in various JSON values.
typedef unsigned int JsonLength;

// JsonString represents json string.
typedef struct {
    char *str;
    JsonLength len;
} JsonString;

typedef struct JsonObjectEntry JsonObjectEntry;

// JsonObject represents JSON object type.
typedef struct {
    JsonObjectEntry *entries;
    JsonLength len;
} JsonObject;

// JsonArray represents JSON array type.
typedef struct {
    Json *values;
    JsonLength len;
} JsonArray;

// JsonNumber is the type of a JSON number.
typedef long double JsonNumber;

// JsonBoolean is the type of a JSON boolean.
typedef int JsonBoolean;

// Json is general JSON value which has specific type. It is returned by
// json_parse and may be contained in JSON objects and arrays.
struct Json {
    JsonType type;
    union {
        JsonObject object;
        JsonArray array;
        JsonString string;
        JsonNumber number;
        JsonBoolean boolean;
    };
};

// JsonObjectEntry is the type of entries in a JsonObject.
struct JsonObjectEntry {
    JsonString key;
    Json value;
};

// JsonParseError holds the possible errors returned by the parser.
typedef enum JsonParseError {
    JSON_ERR_OK = 0,
    JSON_ERR_FAILED_ALLOCATION = INT_MIN,
    JSON_ERR_INVALID_VALUE,
    JSON_ERR_STRING_INVALID_ESCAPE,
    JSON_ERR_STRING_INVALID_ESCAPE_DIGITS,
    JSON_ERR_STRING_END_NOT_FOUND,
    JSON_ERR_OBJECT_UNEXPECTED_END,
    JSON_ERR_OBJECT_UNEXPECTED_VALUE,
    JSON_ERR_OBJECT_MISSING_COLLON,
    JSON_ERR_OBJECT_END_NOT_FOUND,
    JSON_ERR_ARRAY_UNEXPECTED_END,
    JSON_ERR_ARRAY_UNEXPECTED_VALUE,
    JSON_ERR_ARRAY_END_NOT_FOUND,
    JSON_ERR_NUMBER_INVALID,
    JSON_ERR_NUMBER_INVALID_DECIMAL,
    JSON_ERR_NUMBER_INVALID_EXPONENT,    
    JSON_ERR_TRAILING_DATA,
} JsonParseError;

// json_parse parses given data to given Json object, returning parse error.
// Pointers should be valid and non-NULL. Resources allocated during parsing can
// be freed using json_cleanup.
JsonParseError json_parse(Json *json, const char *data);

// Releases resources held by JSON object.
void json_cleanup(Json *json);

#endif