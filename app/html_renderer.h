#ifndef __HTML_RENDERER_H
#define __HTML_RENDERER_H
#include "message.h"
#include "util.h"

// BREAK_ON_CHARS contains all characters that are considered good positions to
// break up text when wrapping.
#define BREAK_ON_CHARS " -/"

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

// renderHtml renders html to text. Should be wrapped before using.
HtmlRender renderHtml(const char *html, Message *message);

// wrapHtmlRender wraps rendered html output, resulting in each line no longer
// than given width when printed in terminal.
WrappedLines wrapHtmlRender(HtmlRender render, int width, Message *message);

void freeHtmlRender(HtmlRender render);
void freeWrappedLines(WrappedLines lines);

// ArrayWrap is a wrapper around a dinamic array, allowing to append elements
// easily and efficiently. No more that one wrapper for single array must be
// used.
typedef struct ArrayWrap {
    int *len, cap, size;
    void **data;
} ArrayWrap;

// wrapArray wraps given dynamic array for appending.
// @param dataPtr pointer to the array (not to the first element). Must point to
// NULL value or valid array.
// @param lenPtr pointer to current array length, modified when appending.
// @param size the size of a single element.
ArrayWrap wrapArray(void *dataPtr, int *lenPtr, int size);

// arrayAppend appends element to given array, modifying underlying properties.
void arrayAppend(ArrayWrap *array, const void *elem, Message *message);

// arrayShrink reduces wrapped array capacity to its length. Inefficient,
// usually should be called when done appending.
void arrayShrink(ArrayWrap *array, Message *message);


#endif