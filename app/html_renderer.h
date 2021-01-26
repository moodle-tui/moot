/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com Licensed
 * as with https://github.com/moodle-tui/moot
 *
 * Hacky and very limited html renderer to text.
 *
 * Currently, it only extracts text and tries to handle whitespace and line
 * breaks close to normal renderers. It also handles tags like links, images by
 * displaying their urls, also trying to mimic formatting tags like b, em, h1-h6
 * using only text. Ordered and unordered lists are displayed as well.
 */

#ifndef __HTML_RENDERER_H
#define __HTML_RENDERER_H
#include "message.h"
#include "util.h"

// ZERO_WIDTH_SPACE is used to insert a c character with no with and a possible
// word breaking location. Encoded in UTF-8.
#define ZERO_WIDTH_SPACE "\xe2\x80\x8b"

// BREAK_ON_CHARS contains all characters that are considered good positions to
// break up text when wrapping.
#define BREAK_ON_CHARS " " ZERO_WIDTH_SPACE

// HtmlRender is the html rendered to text. It should not be used directly, but
// wrapped first.
typedef struct HtmlRender {
    int lineCount;
    char **lines;
} HtmlRender;

// Line is NOT null terminated piece of text, limited to certain width.
typedef struct Line {
    const char *text;
    int length;
} Line;

// WrappedLines is the wrapped output of rendered html.
typedef struct WrappedLines {
    int count;
    Line *lines;
} WrappedLines;

// renderHtml renders html to text. Html is expected to be encoded in UTF-8 and
// output should be wrapped before using.
HtmlRender renderHtml(const char *html, Message *message);

// wrapHtmlRender wraps rendered html output, resulting in each line no longer
// than given width when printed in terminal. The lines are pointing to the
// original HtmlRender output, therefore lines are not zero terminated and
// the HtmlRender should not be freed while WrappedLines generated from it are
// in use.
WrappedLines wrapHtmlRender(HtmlRender render, int width, Message *message);

void freeHtmlRender(HtmlRender render);
void freeWrappedLines(WrappedLines lines);

// ArrayWrapper is a wrapper around a dinamic array, allowing to append elements
// easily and efficiently. No more that one wrapper for single array must be
// used.
typedef struct ArrayWrapper {
    int *len, cap, size;
    void **data;
} ArrayWrapper;

// wrapArray wraps given dynamic array for appending.
// @param dataPtr pointer to the array (not to the first element). Must point to
// NULL value or valid array.
// @param lenPtr pointer to current array length, modified when appending.
// @param size the size of a single element.
ArrayWrapper wrapArray(void *dataPtr, int *lenPtr, int size);

// arrayAppend appends element to given array, modifying underlying properties.
void arrayAppend(ArrayWrapper *array, const void *elem, Message *message);

// arrayAppendMulti acts like arrayAppend, but appends count elements at once.
void arrayAppendMulti(ArrayWrapper *array, int count, const void *elems, Message *message);

// arrayShrink reduces wrapped array capacity to its length. Inefficient and
// usually should be called when done appending.
void arrayShrink(ArrayWrapper *array, Message *message);

#endif