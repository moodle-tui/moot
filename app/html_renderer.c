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
    bool *ok;
} RenderState;

uint renderNode(GumboNode *node, uint thisLineWidth, RenderState state);
void renderNodeBlock(GumboNode *node, RenderState state);

void *mallocOk(size_t size, bool *ok) {
    void *data = malloc(size);
    if (!data)
        *ok = false;
    return data;
}

void *callocOk(size_t n, size_t size, bool *ok) {
    void *data = calloc(n, size);
    if (!data)
        *ok = false;
    return data;
}

void *reallocOk(void *data, size_t size, bool *ok) {
    data = realloc(data, size);
    if (!data)
        *ok = false;
    return data;
}

HtmlRender renderHtml(const char *html) {
    HtmlRender render = {.lineCount = 0, .lines = NULL};
    GumboOptions opt = kGumboDefaultOptions;
    // opt.fragment_context = GUMBO_TAG_BODY;
    opt.fragment_context = GUMBO_TAG_HTML;
    GumboOutput *out = gumbo_parse_with_options(&opt, html, strlen(html));
    if (out) {
        RenderState state = {
            .render = &render,
            .ok = &(bool){true},
        };
        renderNodeBlock(out->root, state);
    }
    gumbo_destroy_output(&opt, out);
    return render;
}

const char *sliceText(const char *text, uint width) {
    int len = strlen(text), wcWidth = 0;
    uint sliceWidth = 0;
    while (sliceWidth + wcWidth < width && *text) {
        Rune ch;
        uint chWidth = utf8decode(text, &ch, len);
        if (ch == '\n') {
            text += chWidth;
            break;
        }
        wcWidth = wcwidth(ch);
        if (sliceWidth + wcWidth <= width) {
            text += chWidth;
            sliceWidth += wcWidth;
            len -= chWidth;
        }
    }
    return text;
}

void addLine(HtmlRender *render, char *line, bool *ok) {
    render->lines = realloc(render->lines, (render->lineCount + 1) * sizeof(char *));
    if (render->lines) {
        render->lines[render->lineCount++] = line;
    } else {
        render->lineCount = 0;
        *ok = false;
    }
}

bool isTagInline(GumboTag tag) {
    int count = sizeof(inlineTags) / sizeof(GumboTag);
    for (int i = 0; i < count; ++i) {
        if (inlineTags[i] == tag)
            return true;
    }
    return false;
}

char *trimWhitespace(const char *text, bool *ok) {
    char *buffer = callocOk(strlen(text) + 1, 1, ok);
    if (*ok) {
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
    return buffer;
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

void renderNodeInline(GumboNode *node, char **line, int *length, RenderState state) {
    // TODO: add inline node clustering
    if (node->type == GUMBO_NODE_TEXT) {
        int prevLength = *length;
        *length = *length + strlen(node->v.text.text);
        *line = reallocOk(*line, *length + 1, state.ok);
        if (*line) {
            strcpy(*line + prevLength, node->v.text.text);
        }
    }
    if (node->type == GUMBO_NODE_ELEMENT) {
        for (int i = 0; i < node->v.element.children.length && *state.ok; ++i) {
            GumboNode *child = ((GumboNode **)node->v.element.children.data)[i];
            renderNodeInline(child, line, length, state);
        }
    }
}

void renderNodeBlock(GumboNode *node, RenderState state) {
    for (int i = 0; i < node->v.element.children.length && *state.ok; ++i) {
        GumboNode *child = ((GumboNode **)node->v.element.children.data)[i];
        if (isContextInline(child)) {
            char *line = NULL;
            renderNodeInline(child, &line, &(int){0}, state);
            if (line) {
                char *trimmed = trimWhitespace(line, state.ok);
                free(line);
                if (trimmed && *trimmed) {
                    addLine(state.render, trimmed, state.ok);
                    if (!*state.ok) {
                        state.render->lineCount = 0;
                    }
                }
            }
        } else {
            renderNodeBlock(child, state);
        }
    }
}

WrappedLines wrapHtmlRender(HtmlRender render, uint width) {
    WrappedLines result = {.count = 0, .lines = NULL};
    if (width < 2) {
        return result;
    }
    size_t capacity = 0;
    bool ok = true;
    for (int i = 0; i < render.lineCount && ok; ++i) {
        const char *begin = render.lines[i];
        do {
            const char *end = sliceText(begin, width);
            if (result.count >= capacity) {
                capacity = capacity ? capacity * 2 : 1;
                result.lines = reallocOk(result.lines, capacity * sizeof(Line), &ok);
            }
            if (result.lines) {
                result.lines[result.count].text = begin; 
                result.lines[result.count].length = end - begin;
                ++result.count;
            } else {
                result.count = 0;
            }
            begin = end;
        } while (*begin && ok);
    }
    result.lines = realloc(result.lines, result.count * sizeof(Line));
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