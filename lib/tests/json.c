/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com Licensed
 * as with https://github.com/moodle-tui/moot
 *
 * Custom JSON parser (see json.h) unit tests. Test by running main.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "json.h"

typedef struct TestCase {
    const char *inputData;
    Json *expectedJson;
    JsonParseError expectedError;
} TestCase;

bool approxEquals(double value, double other, double epsilon);
bool jsonEquals(Json *a, Json *b) ;

bool test(TestCase testCase, int number);

int main() {
    TestCase testCases[] = {
        {
            .inputData = "1.2",
            .expectedJson = &(Json){.type = JSON_NUMBER, .number = 1.2,},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "\"Hello world!\"",
            .expectedJson = &(Json){.type = JSON_STRING, .string = {.str = "Hello world!", .len = 12,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "{\"a\":{\"b\":{\"c\":{\"1\":10.456e-10, \"2\":\"text\", \"3\":true, \"4\":false, \"5\":null, \"6\":{}, \"7\":[]}}}}",
            .expectedJson = &(Json){.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "a", .len = 1,},.value = {.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "b", .len = 1,},.value = {.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "c", .len = 1,},.value = {.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "5", .len = 1,},.value = {.type = JSON_NULL,},},{.key = {.str = "6", .len = 1,},.value = {.type = JSON_OBJECT, .object = { .len = 0,},},},{.key = {.str = "7", .len = 1,},.value = {.type = JSON_ARRAY, .array = { .len = 0,},},},{.key = {.str = "1", .len = 1,},.value = {.type = JSON_NUMBER, .number = 1.0456e-09,},},{.key = {.str = "2", .len = 1,},.value = {.type = JSON_STRING, .string = {.str = "text", .len = 4,},},},{.key = {.str = "3", .len = 1,},.value = {.type = JSON_BOOLEAN, .boolean = true,},},{.key = {.str = "4", .len = 1,},.value = {.type = JSON_BOOLEAN, .boolean = false,},},}, .len = 7,},},},}, .len = 1,},},},}, .len = 1,},},},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]",
            .expectedJson = &(Json){.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_ARRAY, .array = { .len = 0,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "[\"\\uDADA\"]",
            .expectedJson = &(Json){.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_STRING, .string = {.str = "�", .len = 3,},},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "\n\t\t{\n\t\t\t\n\t\t\t\"\\taa\\t\\uDADA\"\n\t\t\t\n\t\t\t:\n\t\t\t\n\t\t\t\n\t\t\t{\n\t\t\t\t\n\t\t\t\t\"\\uBCAA\"\n\t\t\t\t\n\t\t\t\t:\n\t\t\t\t\n\t\t\t\t[\n\t\t\t\t\t]\n\t\t\t}\n\t\t}",
            .expectedJson = &(Json){.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "\taa\t�", .len = 7,},.value = {.type = JSON_OBJECT, .object = { .entries = (JsonObjectEntry[]){{.key = {.str = "벪", .len = 3,},.value = {.type = JSON_ARRAY, .array = { .len = 0,},},},}, .len = 1,},},},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "{\"id\":0,}",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_OBJECT_UNEXPECTED_END,
        },
        {
            .inputData = "{\"a\":\"b\"}#",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_TRAILING_DATA,
        },
        {
            .inputData = "[True]",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_INVALID_VALUE,
        },
        {
            .inputData = "[true]",
            .expectedJson = &(Json){.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_BOOLEAN, .boolean = true,},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "[-0]",
            .expectedJson = &(Json){.type = JSON_ARRAY, .array = { .values = (Json[]){{.type = JSON_NUMBER, .number = -0,},}, .len = 1,},},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "[- 0]",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_NUMBER_INVALID,
        },
        {
            .inputData = "[+0]",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_INVALID_VALUE,
        },
        {
            .inputData = "-0e",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_NUMBER_INVALID_EXPONENT,
        },
        {
            .inputData = "-0e0",
            .expectedJson = &(Json){.type = JSON_NUMBER, .number = -0,},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "-0e-0",
            .expectedJson = &(Json){.type = JSON_NUMBER, .number = -0,},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "-0e+0",
            .expectedJson = &(Json){.type = JSON_NUMBER, .number = -0,},
            .expectedError = JSON_ERR_OK,
        },
        {
            .inputData = "\"hell\\o\"",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_STRING_INVALID_ESCAPE,
        },
        {
            .inputData = "\"hell\\u123g\"",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_STRING_INVALID_ESCAPE_DIGITS,
        },
        {
            .inputData = "\"no end",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_STRING_END_NOT_FOUND,
        },
        {
            .inputData = "{\"key\"}",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_OBJECT_MISSING_COLLON,
        },
        {
            .inputData = "{\"key\":\t\"value\",   }",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_OBJECT_UNEXPECTED_END,
        },
        {
            .inputData = "{",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_OBJECT_END_NOT_FOUND,
        },
        {
            .inputData = "[1, ]",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_ARRAY_UNEXPECTED_END,
        },
        {
            .inputData = "[{}{}]",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_ARRAY_UNEXPECTED_VALUE,
        },
        {
            .inputData = "[1, 2",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_ARRAY_END_NOT_FOUND,
        },
        {
            .inputData = "123.a",
            .expectedJson = NULL,
            .expectedError = JSON_ERR_NUMBER_INVALID_DECIMAL,
        },
    };
    int len = sizeof(testCases) / sizeof(testCases[0]);
    int passed = 0;
    for (int i = 0; i < len; ++i) {
        passed += test(testCases[i], i + 1);
    }
    printf("Done. %d/%d tests have passed\n", passed, len);
}

bool test(TestCase testCase, int number) {
    printf("Test #%d: ", number);
    Json json;
    JsonParseError error = json_parse(&json, testCase.inputData);
    bool success = false;
    if (!jsonEquals(error == JSON_ERR_OK ? &json : NULL, testCase.expectedJson)) {
        printf("Fail: Json not as expected\n");
    } else if (error != testCase.expectedError) {
        printf("Fail: Wrong error! expected %d, got %d\n", testCase.expectedError, error);
    } else {
        printf("OK\n");
        success = true;
    }
    json_cleanup(&json);
    return success;
}

bool approxEquals(double value, double other, double epsilon) {
    return fabs(value - other) < epsilon;
}

bool jsonStringEquals(JsonString *s1, JsonString *s2) {
    return s1->len == s2->len && memcmp(s1->str, s2->str, s1->len) == 0;
}

bool jsonEquals(Json *a, Json *b) {
    if (a == b)
        return true;
    if (!a || !b || a->type != b->type)
        return false;
    switch (a->type) {
        case JSON_OBJECT:
            if (a->object.len != b->object.len) {
                return false;
            }
            for (JsonLength i = 0; i < a->object.len; ++i) {
                bool found = false;
                for (JsonLength j = 0; j < b->object.len && !found; ++j) {
                    if (jsonStringEquals(&a->object.entries[i].key, &b->object.entries[j].key)) {
                        if (jsonEquals(&a->object.entries[i].value, &b->object.entries[j].value)) {
                            found = true;
                        }
                    }
                }
                if (!found)
                    return false;
            }
            return true;
        case JSON_ARRAY:
            if (a->array.len != b->array.len) {
                return false;
            }
            for (JsonLength i = 0; i < a->array.len; ++i) {
                if (!jsonEquals(&a->array.values[i], &b->array.values[i])) {
                    return false;
                }
            }
            return true;
        case JSON_STRING:
            return jsonStringEquals(&a->string, &b->string);
        case JSON_NUMBER:
            return approxEquals(a->number, b->number, 1e-18);
        case JSON_BOOLEAN:
            return a->boolean == b->boolean;
        case JSON_NULL:
            return true;
        default:
            return true;
    }
}