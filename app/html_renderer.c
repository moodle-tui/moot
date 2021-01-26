/*
 * Copyright (C) 2020 Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 * see html_renderer.h
 */
#include "html_renderer.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "gumbo/gumbo.h"
#include "utf8.h"
#include "wcwidth.h"

#define MAX_HEADING_N 6
#define SRC_BASE64 "Base64"
#define IMAGE_PREFIX "Image:"
#define BOLD_MARK "*"
#define ITALICS_MARK "/"
#define HEADING_MARK "#"
#define UNORDERED_LIST_PREFIX "\xE2\x80\xA2 "
#define MAX_INT_WIDTH 13
#define HR_LINE "------"

static GumboTag inlineTags[] = {
    GUMBO_TAG_A,   GUMBO_TAG_ABBR, GUMBO_TAG_ACRONYM, GUMBO_TAG_B, GUMBO_TAG_BDO,   GUMBO_TAG_BIG,  GUMBO_TAG_CITE,   GUMBO_TAG_CODE,   GUMBO_TAG_DFN, GUMBO_TAG_EM,  GUMBO_TAG_FONT, GUMBO_TAG_I,
    GUMBO_TAG_IMG, GUMBO_TAG_KBD,  GUMBO_TAG_NOBR,    GUMBO_TAG_S, GUMBO_TAG_SMALL, GUMBO_TAG_SPAN, GUMBO_TAG_STRIKE, GUMBO_TAG_STRONG, GUMBO_TAG_SUB, GUMBO_TAG_SUP, GUMBO_TAG_TT,
};

typedef struct RenderState {
    HtmlRender *render;
    ArrayWrapper renderWrap;
    ArrayWrapper currentLine;
    bool trailingNewline;
    Message *msg;
} RenderState;

typedef struct Nodes {
    GumboNode **begin, **end;
} Nodes;

bool isOk(Message *message);
ArrayWrapper wrapArray(void *dataPtr, int *lenPtr, int size);
void arrayAppendMulti(ArrayWrapper *array, int count, const void *elems, Message *message);
void arrayAppend(ArrayWrapper *array, const void *elem, Message *message);
void arrayShrink(ArrayWrapper *array, Message *message);
HtmlRender renderHtml(const char *html, Message *message);

// addLine adds line to render, also adding empty one above if
// state::trailingNewline is true.
void addLine(RenderState *state, char *line);

bool hasTag(GumboNode *node, GumboTag tag);
bool isNodeInline(GumboNode *node);
Nodes getNodeChildren(GumboNode *node);
GumboAttribute *getAttribute(GumboNode *node, const char *name);

// shouldSkipNode returns whether the contents of the node should not be rendered,
// e. g. style or script nodes.
bool shouldSkipNode(GumboNode *node);

// canBreakWord returns whether current position in string is suitable for
// breaking to next line. Depends on BREAK_ON_CHARS.
bool canBreakWord(const char *s, int length);

// trimWhitespace converts all whitespace of the text to single spaces,
// consecutive space to single spaces and trims both leading and trailing
// whitespace. Returned string is newly allocated.
char *trimWhitespace(const char *text, Message *message);

// surroundNodes returns new node range, surrounding given nodes with prefix and
// suffix and writing everything to given array. Prefix or suffix may be NULL,
// in which case they are not added. array must be big enough to fill the old
// node range, prefix and suffix (if not NULL).
Nodes surroundNodes(GumboNode *prefix, GumboNode *suffix, Nodes nodes, GumboNode **array);

// renderNodeSurrounded renders given children of given node with newly created
// leading and trailing text nodes, containing the prefix and suffix
// accordingly. Prefix or suffix may be NULL, in which case coresponding text
// node(s) are not created.
void renderNodeSurrounded(const char *prefix, const char *suffix, GumboNode *node, bool isInline, RenderState *state);

// Rendering functions for specific html elements.

void renderNodeP(GumboNode *node, bool isInline, RenderState *state);
void renderNodeA(GumboNode *node, bool isInline, RenderState *state);
void renderNodeImg(GumboNode *node, bool isInline, RenderState *state);
void renderNodeI(GumboNode *node, bool isInline, RenderState *state);
void renderNodeB(GumboNode *node, bool isInline, RenderState *state);
void renderNodeUl(GumboNode *node, bool isInline, RenderState *state);
void renderNodeOl(GumboNode *node, bool isInline, RenderState *state);
void renderNodeHr(GumboNode *node, bool isInline, RenderState *state);

// renderNodeHN renders headings (h1-h6) with the number specified as n.
void renderNodeHN(GumboNode *node, int n, bool isInline, RenderState *state);

void renderNode(GumboNode *node, bool isInline, RenderState *state);
void renderNodes(Nodes nodes, bool isInline, RenderState *state);

bool isOk(Message *message) {
    return message->type != MSG_TYPE_ERROR;
}

ArrayWrapper wrapArray(void *dataPtr, int *lenPtr, int size) {
    ArrayWrapper array = {
        .data = dataPtr,
        .len = lenPtr,
        .size = size,
        .cap = *lenPtr,
    };
    return array;
}

void arrayAppendMulti(ArrayWrapper *array, int count, const void *elems, Message *message) {
    if (array->cap < *array->len + count) {
        while (array->cap < *array->len + count) {
            array->cap = array->cap ? array->cap * 2 : 1;
        }
        *array->data = xrealloc(*array->data, array->cap * array->size, message);
    }
    if (*(array->data)) {
        memcpy((char *)*array->data + *array->len * array->size, elems, array->size * count);
        *array->len += count;
    } else {
        *array->len = array->cap = 0;
    }
}

void arrayAppend(ArrayWrapper *array, const void *elem, Message *message) {
    arrayAppendMulti(array, 1, elem, message);
}

void arrayShrink(ArrayWrapper *array, Message *message) {
    if (array->cap != *array->len) {
        *array->data = xrealloc(*array->data, *array->len * array->size, message);
        array->cap = *array->len;
    }
}

HtmlRender renderHtml(const char *html, Message *message) {
    HtmlRender render = {.lineCount = 0, .lines = NULL};
    GumboOptions opt = kGumboDefaultOptions;
    opt.fragment_context = GUMBO_TAG_HTML;
    GumboOutput *out = gumbo_parse_with_options(&opt, html, strlen(html));
    if (out) {
        RenderState state = {
            .render = &render,
            .msg = message,
            .trailingNewline = false,
            .renderWrap = wrapArray(&render.lines, &render.lineCount, sizeof(char *)),
        };
        renderNodes(getNodeChildren(out->root), false, &state);
    }
    gumbo_destroy_output(&opt, out);
    return render;
}

void addLine(RenderState *state, char *line) {
    if (state->render->lineCount && state->trailingNewline) {
        char *empty = xcalloc(1, 1, state->msg);
        if (isOk(state->msg)) {
            arrayAppend(&state->renderWrap, &empty, state->msg);
            state->trailingNewline = false;
        }
    }
    if (isOk(state->msg)) {
        arrayAppend(&state->renderWrap, &line, state->msg);
    }
}

bool hasTag(GumboNode *node, GumboTag tag) {
    return node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == tag;
}

bool shouldSkipNode(GumboNode *node) {
    return hasTag(node, GUMBO_TAG_STYLE) || hasTag(node, GUMBO_TAG_SCRIPT);
}

char *trimWhitespace(const char *text, Message *message) {
    char *buffer = xcalloc(strlen(text) + 1, 1, message);
    if (isOk(message)) {
        int bufferLength = 0;
        // Trim leading whitespace.
        bool ignoreWhitespace = true;
        while (*text) {
            // Convert inner whitespace to single spaces.
            if (isspace(*text) && !ignoreWhitespace) {
                buffer[bufferLength++] = ' ';
                ignoreWhitespace = true;
            } else if (!isspace(*text)) {
                buffer[bufferLength++] = *text;
                ignoreWhitespace = false;
            }
            ++text;
        }
        // Trim trailing whitespace.
        for (int i = bufferLength - 1; i >= 0 && isspace(buffer[i]); --i) {
            buffer[i] = 0;
        }
    }
    return xrealloc(buffer, strlen(buffer) + 1, message);
}

bool isNodeInline(GumboNode *node) {
    if (node->type != GUMBO_NODE_ELEMENT) {
        return true;
    }
    int count = sizeof(inlineTags) / sizeof(GumboTag);
    for (int i = 0; i < count; ++i) {
        if (inlineTags[i] == node->v.element.tag)
            return true;
    }
    return false;
}

Nodes getNodeChildren(GumboNode *node) {
    assert(node->type == GUMBO_NODE_ELEMENT);
    return (Nodes){
        .begin = (GumboNode **)node->v.element.children.data,
        .end = (GumboNode **)node->v.element.children.data + node->v.element.children.length,
    };
}

Nodes surroundNodes(GumboNode *prefix, GumboNode *suffix, Nodes nodes, GumboNode **array) {
    int offset = prefix != NULL;
    if (offset) {
        array[0] = prefix;
    }
    memcpy(array + offset, nodes.begin, (nodes.end - nodes.begin) * sizeof(GumboNode *));
    if (suffix) {
        array[offset + nodes.end - nodes.begin] = suffix;
    }
    return (Nodes){
        .begin = array,
        .end = array + offset + (nodes.end - nodes.begin) + (suffix != NULL),
    };
}

GumboAttribute *getAttribute(GumboNode *node, const char *name) {
    for (int i = 0; i < node->v.element.attributes.length; ++i) {
        GumboAttribute *attr = ((GumboAttribute **)node->v.element.attributes.data)[i];
        if (strcmp(attr->name, name) == 0) {
            return attr;
        }
    }
    return NULL;
}

void renderNodeP(GumboNode *node, bool isInline, RenderState *state) {
    state->trailingNewline = true;
    renderNodes(getNodeChildren(node), isInline, state);
    state->trailingNewline = true;
}

// #define SURROUND_NODES(prefix, suffix, nodes)
#define MAKE_TEXT_NODE(str) ((GumboNode){.type = GUMBO_NODE_TEXT, .v.text.text = str})

void renderNodeSurrounded(const char *prefix, const char *suffix, GumboNode *node, bool isInline, RenderState *state) {
    GumboNode *nodeArray[node->v.element.children.length + 2];
    Nodes nodes = getNodeChildren(node);
    GumboNode left = {.type = GUMBO_NODE_TEXT, .v.text.text = prefix};
    GumboNode right = {.type = GUMBO_NODE_TEXT, .v.text.text = suffix};

    nodes = surroundNodes(prefix ? &left : NULL, suffix ? &right : NULL, nodes, nodeArray);
    renderNodes(nodes, isInline, state);
}

void renderNodeA(GumboNode *node, bool isInline, RenderState *state) {
    GumboAttribute *href = getAttribute(node, "href");
    const char *url = href ? href->value : "";
    char suffix[strlen(url) + sizeof(">" ZERO_WIDTH_SPACE "[]") + 1];
    sprintf(suffix, ">" ZERO_WIDTH_SPACE "[%s]", url);

    renderNodeSurrounded("<", suffix, node, isInline, state);
}

void renderNodeImg(GumboNode *node, bool isInline, RenderState *state) {
    GumboAttribute *src = getAttribute(node, "src");
    const char *url = src ? src->value : "";
    if (strstr(url, ";base64,")) {
        url = SRC_BASE64;
    }
    char suffix[strlen(url) + sizeof(ZERO_WIDTH_SPACE "[]") + 1];
    sprintf(suffix, ZERO_WIDTH_SPACE "[%s]", url);

    renderNodeSurrounded(IMAGE_PREFIX, suffix, node, isInline, state);
}

void renderNodeI(GumboNode *node, bool isInline, RenderState *state) {
    renderNodeSurrounded(ITALICS_MARK, ITALICS_MARK, node, isInline, state);
}

void renderNodeB(GumboNode *node, bool isInline, RenderState *state) {
    renderNodeSurrounded(BOLD_MARK, BOLD_MARK, node, isInline, state);
}

void renderNodeUl(GumboNode *node, bool isInline, RenderState *state) {
    Nodes nodes = getNodeChildren(node);
    for (GumboNode **it = nodes.begin; it < nodes.end; ++it) {
        if (hasTag(*it, GUMBO_TAG_LI)) {
            renderNodeSurrounded(UNORDERED_LIST_PREFIX, NULL, *it, isInline, state);
        }
    }
}

void renderNodeOl(GumboNode *node, bool isInline, RenderState *state) {
    Nodes nodes = getNodeChildren(node);
    int index = 1;
    for (GumboNode **it = nodes.begin; it < nodes.end; ++it) {
        if (hasTag(*it, GUMBO_TAG_LI)) {
            char indent[MAX_INT_WIDTH + 2];
            sprintf(indent, "%d. ", index);
            renderNodeSurrounded(indent, NULL, *it, isInline, state);
            ++index;
        }
    }
}

void renderNodeHN(GumboNode *node, int n, bool isInline, RenderState *state) {
    char title[MAX_HEADING_N + 3] = " ";
    for (int i = 0; i < n; ++i) {
        strcat(title, HEADING_MARK);
    }
    strcat(title, " ");
    renderNodeSurrounded(title, title, node, isInline, state);
}

void renderNodeHr(GumboNode *node, bool isInline, RenderState *state) {
    renderNodeSurrounded(HR_LINE, NULL, node, isInline, state);
}

void renderNode(GumboNode *node, bool isInline, RenderState *state) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        switch (node->v.element.tag) {
            case GUMBO_TAG_P:
                renderNodeP(node, isInline, state);
                break;
            case GUMBO_TAG_A:
                renderNodeA(node, isInline, state);
                break;
            case GUMBO_TAG_IMG:
                renderNodeImg(node, isInline, state);
                break;
            case GUMBO_TAG_I:
            case GUMBO_TAG_EM:
                renderNodeI(node, isInline, state);
                break;
            case GUMBO_TAG_B:
            case GUMBO_TAG_STRONG:
                renderNodeB(node, isInline, state);
                break;
            case GUMBO_TAG_UL:
                renderNodeUl(node, isInline, state);
                break;
            case GUMBO_TAG_OL:
                renderNodeOl(node, isInline, state);
                break;
            case GUMBO_TAG_H1:
                renderNodeHN(node, 1, isInline, state);
                break;
            case GUMBO_TAG_H2:
                renderNodeHN(node, 2, isInline, state);
                break;
            case GUMBO_TAG_H3:
                renderNodeHN(node, 3, isInline, state);
                break;
            case GUMBO_TAG_H4:
                renderNodeHN(node, 4, isInline, state);
                break;
            case GUMBO_TAG_H5:
                renderNodeHN(node, 5, isInline, state);
                break;
            case GUMBO_TAG_H6:
                renderNodeHN(node, 6, isInline, state);
                break;
            case GUMBO_TAG_HR:
                renderNodeHr(node, isInline, state);
                break;
            default:
                renderNodes(getNodeChildren(node), isInline, state);
                break;
        }
    }
    if (node->type == GUMBO_NODE_TEXT) {
        // Remove the zero terminator.
        if (*state->currentLine.len) {
            --(*state->currentLine.len);
        }
        arrayAppendMulti(&state->currentLine, strlen(node->v.text.text) + 1, node->v.text.text, state->msg);
    }
}

void renderNodes(Nodes nodes, bool isInline, RenderState *state) {
    for (GumboNode **it = nodes.begin; it < nodes.end && isOk(state->msg); ++it) {
        GumboNode *node = *it;
        if (shouldSkipNode(node)) {
            continue;
        }
        if (!isInline && isNodeInline(node)) {
            GumboNode **end = it + 1;
            while (end < nodes.end && isNodeInline(*end)) {
                ++end;
            }
            char *line = NULL;
            state->currentLine = wrapArray(&line, &(int){0}, sizeof(char));
            renderNodes((Nodes){.begin = it, .end = end}, true, state);
            if (line) {
                char *trimmed = trimWhitespace(line, state->msg);
                free(line);
                if (trimmed && *trimmed) {
                    addLine(state, trimmed);
                }
            }
            it = end - 1;
        } else {
            renderNode(node, isInline, state);
        }
    }
}

bool canBreakWord(const char *s, int length) {
    if (!length) {
        Rune ch;
        length = utf8decodeNullTerm(s, &ch);
    }
    char c[length + 1];
    strncpy(c, s, length);
    c[length] = 0;
    return length && strstr(BREAK_ON_CHARS, c) != NULL;
}

WrappedLines wrapHtmlRender(HtmlRender render, int width, Message *message) {
    // This implementation relies on the html rendering algorithm, which leaves
    // only spaces as whitespace.
    WrappedLines result = {.count = 0, .lines = NULL};
    ArrayWrapper resultWrap = wrapArray(&result.lines, &result.count, sizeof(Line));
    if (width < 2) {
        return result;
    }
    for (int i = 0; i < render.lineCount && isOk(message); ++i) {
        const char *it = render.lines[i];
        do {
            // Skip leading space.
            if (*it == ' ') {
                ++it;
            }
            const char *begin = it, *lastBreakPos = NULL;
            int sliceWidth = 0;
            while (sliceWidth < width && *it) {
                Rune ch;
                int chLength = utf8decodeNullTerm(it, &ch);
                if (canBreakWord(it, chLength) && it - begin > 0) {
                    lastBreakPos = it;
                }
                it += chLength;
                sliceWidth += wcwidth(ch);
            }
            const char *end = it;
            if (sliceWidth >= width && lastBreakPos && !canBreakWord(it, 0)) {
                end = lastBreakPos + utf8decodeNullTerm(lastBreakPos, &(Rune){0});
            }
            arrayAppend(&resultWrap, &(Line){.text = begin, .length = end - begin}, message);
            it = end;
        } while (*it && isOk(message));
    }
    arrayShrink(&resultWrap, message);
    return result;
}

void freeHtmlRender(HtmlRender render) {
    for (unsigned int i = 0; i < render.lineCount; ++i) {
        free(render.lines[i]);
    }
    free(render.lines);
}

void freeWrappedLines(WrappedLines lines) {
    free(lines.lines);
}
