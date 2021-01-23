#include "html_renderer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "gumbo/gumbo.h"
#include "utf8.h"
#include "wcwidth.h"

static GumboTag inlineTags[] = {
    GUMBO_TAG_A,   GUMBO_TAG_ABBR, GUMBO_TAG_ACRONYM, GUMBO_TAG_B, GUMBO_TAG_BDO,   GUMBO_TAG_BIG,  GUMBO_TAG_CITE,   GUMBO_TAG_CODE,   GUMBO_TAG_DFN, GUMBO_TAG_EM,  GUMBO_TAG_FONT, GUMBO_TAG_I,
    GUMBO_TAG_IMG, GUMBO_TAG_KBD,  GUMBO_TAG_NOBR,    GUMBO_TAG_S, GUMBO_TAG_SMALL, GUMBO_TAG_SPAN, GUMBO_TAG_STRIKE, GUMBO_TAG_STRONG, GUMBO_TAG_SUB, GUMBO_TAG_SUP, GUMBO_TAG_TT,
};

typedef struct RenderState {
    HtmlRender *render;
    ArrayWrap lines;
    bool trailingNewline;
    Message *msg;
} RenderState;

bool isOk(Message *message);
void addLine(RenderState *state, char *line);
bool hasTag(GumboNode *node, GumboTag tag);
bool isTagInline(GumboTag tag);
bool canSkipNode(GumboNode *node);
char *trimWhitespace(const char *text, Message *message);
bool isContextInline(GumboNode *node);
void renderNodeInline(GumboNode *node, char **line, int *length, RenderState *state);
void renderNodeBlock(GumboNode *node, RenderState *state);

bool isOk(Message *message) {
    return message->type != MSG_TYPE_ERROR;
}

ArrayWrap wrapArray(void *dataPtr, int *lenPtr, int size) {
    ArrayWrap array = {
        .data = dataPtr,
        .len = lenPtr,
        .size = size,
        .cap = *lenPtr,
    };
    return array;
}

void arrayAppend(ArrayWrap *array, const void *elem, Message *message) {
    if (array->cap <= *array->len) {
        array->cap = array->cap ? array->cap * 2 : 1;
        *array->data = xrealloc(*array->data, array->cap * array->size, message);
    }
    if (*array->data) {
        memcpy((char *)*array->data + *array->len * array->size, elem, array->size);
        ++(*array->len);
    } else {
        *array->len = array->cap = 0;
    }
}

void arrayShrink(ArrayWrap *array, Message *message) {
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
            .lines = wrapArray(&render.lines, &render.lineCount, sizeof(char *)),
        };
        renderNodeBlock(out->root, &state);
    }
    gumbo_destroy_output(&opt, out);
    return render;
}

void addLine(RenderState *state, char *line) {
    if (state->render->lineCount && state->trailingNewline) {
        char *empty = xcalloc(1, 1, state->msg);
        if (isOk(state->msg)) {
            arrayAppend(&state->lines, &empty, state->msg);
            state->trailingNewline = false;
        }
    }
    if (isOk(state->msg)) {
        arrayAppend(&state->lines, &line, state->msg);
    }
}

bool hasTag(GumboNode *node, GumboTag tag) {
    return node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == tag;
}

bool isTagInline(GumboTag tag) {
    int count = sizeof(inlineTags) / sizeof(GumboTag);
    for (int i = 0; i < count; ++i) {
        if (inlineTags[i] == tag)
            return true;
    }
    return false;
}

bool canSkipNode(GumboNode *node) {
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
    return xrealloc(buffer, strlen(buffer), message);
}

bool isContextInline(GumboNode *node) {
    if (node->type != GUMBO_NODE_ELEMENT || isTagInline(node->v.element.tag)) {
        return true;
    }
    bool isInline = true;
    for (int i = 0; i < node->v.element.children.length && isInline; ++i) {
        GumboNode *child = ((GumboNode **)node->v.element.children.data)[i];
        isInline &= child->type != GUMBO_NODE_ELEMENT || isTagInline(child->v.element.tag);
    }
    return isInline;
}

void renderNodeInline(GumboNode *node, char **line, int *length, RenderState *state) {
    // TODO: add inline node clustering
    if (node->type == GUMBO_NODE_TEXT) {
        int prevLength = *length;
        *length = *length + strlen(node->v.text.text);
        *line = xrealloc(*line, *length + 1, state->msg);
        if (*line) {
            strcpy(*line + prevLength, node->v.text.text);
        }
    }
    if (node->type == GUMBO_NODE_ELEMENT) {
        for (int i = 0; i < node->v.element.children.length && isOk(state->msg); ++i) {
            GumboNode *child = ((GumboNode **)node->v.element.children.data)[i];
            renderNodeInline(child, line, length, state);
        }
    }
}

void renderNodeBlock(GumboNode *node, RenderState *state) {
    for (int i = 0; i < node->v.element.children.length && isOk(state->msg); ++i) {
        GumboNode *child = ((GumboNode **)node->v.element.children.data)[i];
        if (canSkipNode(child)) {
            continue;
        }
        if (isContextInline(child)) {
            state->trailingNewline |= hasTag(child, GUMBO_TAG_P);
            char *line = NULL;
            renderNodeInline(child, &line, &(int){0}, state);
            if (line) {
                char *trimmed = trimWhitespace(line, state->msg);
                free(line);
                if (trimmed && *trimmed) {
                    addLine(state, trimmed);
                }
            }
            state->trailingNewline |= hasTag(child, GUMBO_TAG_P);
        } else {
            renderNodeBlock(child, state);
        }
    }
}

WrappedLines wrapHtmlRender(HtmlRender render, int width, Message *message) {
    // This implementation relies on the html rendering algorithm, which leaves
    // only spaces as whitespace.
    WrappedLines result = {.count = 0, .lines = NULL};
    ArrayWrap resultWrap = wrapArray(&result.lines, &result.count, sizeof(Line));
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
                if (strchr(BREAK_ON_CHARS, *it) && it - begin > 0) {
                    lastBreakPos = it;
                }
                Rune ch;
                it += utf8decodeNullTerm(it, &ch);
                sliceWidth += wcwidth(ch);
            }
            const char *end = it;
            if (sliceWidth >= width && lastBreakPos && !strchr(BREAK_ON_CHARS, *it)) {
                end = lastBreakPos + 1;
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

// int main() {
//     char *html;
//     scanf("%m[^#]", &html);
//     Message message = {.msg = NULL, .type = MSG_TYPE_NONE};
//     HtmlRender render = renderHtml(html, &message);
//     WrappedLines lines = wrapHtmlRender(render, 45, &message);
//     for (int i = 0; i < lines.count; ++i) {
//         fwrite(lines.lines[i].text, 1, lines.lines[i].length, stdout);
//         fputc('\n', stdout);
//     }
//     freeHtmlRender(render);
//     freeWrappedLines(lines);
//     free(html);
// }