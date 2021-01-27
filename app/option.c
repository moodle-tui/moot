#include <stdio.h>
#include "app.h"
#include "utf8.h"
#include "wcwidth.h"

#define OPTION_CUT_STR "~"
#define EMPTY_OPTION_NAME "[empty]"
#define DESCRIPTION_NAME "Description"
#define FILES_NAME "Files"
#define UNSUPPORTED_DESCRIPTION_FORMAT "[Unsupported description format]"

void getMDArrayOption(Option *option, MDArray mdArray, int height, int depthIndex, Message *msg);
void getModuleContentOption(Option *option, MDArray modules, WrappedLines descriptionLines,
        OptionCoordinates printPos, int *highlightedOptions, int width, Message *msg);
void getModuleDepth1Option(Option *option, MDArray modules, int *highlightedOptions, OptionCoordinates printPos);
void getModuleDepth2Option(Option *option, MDArray modules, WrappedLines descriptionLines,
        int *highlightedOptions, OptionCoordinates printPos);
void printHighlightedOption(Option option, int width);
void printOption(Option option, int width);
void printOptionOption(char *name, int width);
void printOptionLine(Line line);

void getOption(Option *option, MDArray courses, WrappedLines descriptionLines, OptionCoordinates printPos,
        int *highlightedOptions, int *scrollOffsets, int width, Message *msg) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    printPos.height += scrollOffsets[printPos.depth];
    switch (printPos.depth) {
        case COURSES_DEPTH:
            getMDArrayOption(option, courses, printPos.height, printPos.depth, msg);
            break;
        case TOPICS_DEPTH:
            getMDArrayOption(option, topics, printPos.height, printPos.depth, msg);
            break;
        case MODULES_DEPTH:
            getMDArrayOption(option, modules, printPos.height, printPos.depth, msg);
            break;
        case MODULE_DEPTH1:
        case MODULE_DEPTH2:
            getModuleContentOption(option, modules, descriptionLines, printPos, highlightedOptions, width, msg);
            break;
        default:
            break;
    }
    if (option->type == OPTION_TYPE_NONE && printPos.height == 0 && printPos.depth != INIT_DEPTH
            && printPos.depth != LAST_DEPTH) {
        option->type = OPTION_TYPE_EMPTY;
        option->content.option = EMPTY_OPTION_NAME;
    }
}

void getMDArrayOption(Option *option, MDArray mdArray, int height, int depthIndex, Message *msg) {
    if (mdArray.len > height) {
        option->type = OPTION_TYPE_OPTION;
        switch (depthIndex) {
            case 0:
                option->content.option = MD_COURSES(mdArray)[height].name;
                break;
            case 1:
                option->content.option = MD_TOPICS(mdArray)[height].name;
                break;
            case 2:
                option->content.option = MD_MODULES(mdArray)[height].name;
                break;
        }
    }
}

void getModuleContentOption(Option *option, MDArray modules, WrappedLines descriptionLines,
        OptionCoordinates printPos, int *highlightedOptions, int width, Message *msg) {
    switch (printPos.depth) {
        case MODULE_DEPTH1:
            getModuleDepth1Option(option, modules, highlightedOptions, printPos);
            break;
        case MODULE_DEPTH2:
            getModuleDepth2Option(option, modules, descriptionLines, highlightedOptions, printPos);
            break;
        default:
            break;
    }
}

void getModuleDepth1Option(Option *option, MDArray modules, int *highlightedOptions, OptionCoordinates printPos) {
    MDModule module = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]];
    switch (printPos.height) {
        case DESCRIPTION_HEIGHT:
            option->content.option = DESCRIPTION_NAME;
            option->type = OPTION_TYPE_OPTION;
            break;
        case FILES_HEIGHT:
            if (modules.len > 0 && module.type == MD_MOD_RESOURCE) {
                option->content.option = FILES_NAME;
                option->type = OPTION_TYPE_OPTION;
            }
            break;
        default:
            break;
    }
}

void getModuleDepth2Option(Option *option, MDArray modules, WrappedLines descriptionLines,
        int *highlightedOptions, OptionCoordinates printPos) {
    MDModule module = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]];
    MDModResource resource;
    switch(highlightedOptions[MODULE_DEPTH1]) {
        case DESCRIPTION_HEIGHT:
            option->type = OPTION_TYPE_LINE;
            MDRichText *description = getModuleDescription(&module);
            if (description->format == MD_FORMAT_HTML) {
                if (descriptionLines.count > printPos.height)
                    option->content.line = descriptionLines.lines[printPos.height];
            }
            else if (printPos.height == 0)
                option->content.line = (Line) {UNSUPPORTED_DESCRIPTION_FORMAT,
                    strlen(UNSUPPORTED_DESCRIPTION_FORMAT)};
            break;
        case FILES_HEIGHT:
            resource = module.contents.resource;
            if (resource.files.len > printPos.height) {
                option->content.option = MD_FILES(resource.files)[printPos.height].filename;
                option->type = OPTION_TYPE_OPTION;
            }
            break;
    }
}

bool checkIfHighlighted(Option option, int *highlightedOptions, OptionCoordinates printPos) {
    if (option.type == OPTION_TYPE_NONE || option.type == OPTION_TYPE_LINE
            || option.type == OPTION_TYPE_EMPTY)
        return 0;
    else
        return highlightedOptions[printPos.depth] == printPos.height;
}

void addOption(Option option, _Bool isHighlighted, int widthIndex, int width) {
    if (widthIndex == 1 || widthIndex == 2)
        printf("%s", SEPERATOR);

    if (isHighlighted)
        printHighlightedOption(option, width);
    else
        printOption(option, width);
}

void printHighlightedOption(Option option, int width) {
    saveDefaultColor();
    setBackgroundColor(7);
    setColor(0);
    printOption(option, width);
    resetColor();
}

void printOption(Option option, int width) {
    switch(option.type) {
        case OPTION_TYPE_OPTION:
        case OPTION_TYPE_EMPTY:
            printOptionOption(option.content.option, width);
            break;
        case OPTION_TYPE_NONE:
            printSpaces(width);
            break;
        case OPTION_TYPE_LINE:
            printOptionLine(option.content.line);
            break;
        default:
            break;
    }
}

void printOptionOption(char *name, int width) {
    int printedChWidth = 0, printedChSize = 0;
    int optionLength = strlen(name);

    printf(" ");
    width -= 2;
    while (1) {
        Rune u;
        size_t charSize = utf8decode(name + printedChSize, &u, optionLength);
        int charWidth = wcwidth(u);

        if (!name[printedChSize]) {
            printSpaces(width - printedChWidth);
            break;
        }
        else if (printedChWidth + charWidth > width - 1 && name[printedChSize + charSize]) {
            printSpaces(width - printedChWidth - 1);
            printf("~");
            ++printedChWidth;
            break;
        }

        fwrite(name + printedChSize, charWidth, charSize, stdout);
        printedChWidth += charWidth;
        printedChSize += charSize;
    }
    printf(" ");
}

void printOptionLine(Line line) {
    fwrite(line.text, sizeof(char), line.length, stdout);
}

