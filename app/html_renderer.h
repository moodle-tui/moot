#ifndef __HTML_RENDERER_H
#define __HTML_RENDERER_H

typedef unsigned int uint;

typedef struct HtmlRender {
    uint lineCount;
    char **lines;
} HtmlRender;

typedef struct Line {
    const char *text;
    int length;
} Line;

typedef struct WrappedLines {
    uint count;
    Line *lines;
} WrappedLines;

HtmlRender renderHtml(const char *html);
WrappedLines wrapHtmlRender(HtmlRender render, uint width);
WrappedLines wordWrapHtmlRender(HtmlRender render, uint width);
void freeHtmlRender(HtmlRender render);
void freeWrappedLines(WrappedLines lines);

#endif