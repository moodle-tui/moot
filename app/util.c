/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */

#include <stdlib.h>
#include <math.h>
#include "app.h"
#include "html_renderer.h"

void *xmalloc(size_t size, Message *msg) {
    return xrealloc(NULL, size, msg);
}

void *xrealloc(void *data, size_t size, Message *msg) {
    data = realloc(data, size);
    if (!data && size)
        createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
    return data;
}

void *xcalloc(size_t n, size_t size, Message *msg) {
    void *data = calloc(n, size);
    if (!data && size)
        createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
    return data;
}

char *getStr(int n) {
    int len = getNrOfDigits(n) + 1;
    char *str = malloc(sizeof(char) * len);
    sprintf(str, "%d", n);
    return str;
}

int getNrOfDigits(int number) {
    return snprintf(NULL, 0, "%d", number);
}

void setHtmlRenders(MDArray *courses, Message *msg) {
    for (int coursesIndex = 0; coursesIndex < courses->len; ++coursesIndex) {
        MDArray topics = MD_COURSES(*courses)[coursesIndex].topics;
        for (int topicsIndex = 0; topicsIndex < topics.len; ++topicsIndex) {
            MDArray modules = MD_TOPICS(topics)[topicsIndex].modules;
            for (int modulesIndex = 0; modulesIndex < modules.len; ++modulesIndex) {
                MDModule *module = &MD_MODULES(modules)[modulesIndex];
                MDRichText *description = getModuleDescription(module);
                if (description->format == MD_FORMAT_HTML) {
                    setHtmlRender(description, msg);
                }
            }
        }
    }
}

MDRichText *getModuleDescription(MDModule *module) {
    switch(module->type) {
        case MD_MOD_ASSIGNMENT:
            return &module->contents.assignment.description;
        case MD_MOD_RESOURCE:
            return &module->contents.resource.description;
        case MD_MOD_WORKSHOP:
            return &module->contents.workshop.description;
        default:
            return &module->contents.url.description;
    }
}

void setHtmlRender(MDRichText *description, Message *msg) {
    HtmlRender *render = malloc(sizeof(HtmlRender));
    *render = renderHtml(description->text, msg);
    if (msg->type == MSG_TYPE_ERROR)
        return;
    description->html_render = render;
}


