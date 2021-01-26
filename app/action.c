/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "rlutil.h"
#include "app.h"
#include "moodle.h"

Action getAction(int key) {
    Action action;
    switch (key) {
        case 108: // l
        case KEY_RIGHT:
        case 10: // enter
            action = ACTION_GO_RIGHT;
            break;
        case 106: // j
        case KEY_DOWN:
            action = ACTION_GO_DOWN;
            break;
        case 104: // h
        case KEY_LEFT:
            action = ACTION_GO_LEFT;
            break;
        case 107: // k
        case KEY_UP:
            action = ACTION_GO_UP;
            break;
        case KEY_ESCAPE:
            action = ACTION_DISMISS_MSG;
            break;
        case 117: // u
            action = ACTION_UPLOAD;
            break;
        case 115: // s
            action = ACTION_DOWNLOAD;
            break;
        case 113: // q
            action = ACTION_QUIT;
            break;
    }
    return action;
}

void validateAction(Action *action, MDArray courses, int *highlightedOptions, int depth, int currentMaxDepth) {
    int depthHeight = getDepthHeight(depth, courses, highlightedOptions);
    switch (*action) {
        case ACTION_GO_RIGHT:
            if (depth == currentMaxDepth)
                *action = ACTION_INVALID;
            break;
        case ACTION_GO_DOWN:
            if (depthHeight == 1)
                *action = ACTION_INVALID;
            break;
        case ACTION_GO_LEFT:
            if (depth == 0)
                *action = ACTION_INVALID;
            break;
        case ACTION_GO_UP:
            if (depthHeight == 1)
                *action = ACTION_INVALID;
            break;
        default:
            break;
    }
}

int getDepthHeight(int depth, MDArray courses, int *highlightedOptions) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    int height;
    bool isResource = (modules.len > 0
            && MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].type == MD_MOD_RESOURCE);
    switch (depth) {
        case COURSES_DEPTH:
            height = courses.len;
            break;
        case TOPICS_DEPTH:
            height = topics.len;
            break;
        case MODULES_DEPTH:
            height = modules.len;
            break;
        case MODULE_DEPTH1:
            height = 1 + isResource;
            break;
        case MODULE_DEPTH2:
            if (isResource)
                height = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]].contents.resource.files.len;
            else
                height = -1;
            break;
        default:
            height = -1;
    }
    return height;
}

void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions,
        int *depth, int *scrollOffsets, char *uploadCommand, Message *msg) {
    int depthHeight = getDepthHeight(*depth, courses, highlightedOptions);
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    int maxHeight = trows() - 2;
    switch (action) {
        case ACTION_GO_RIGHT:
            goRight(depth);
            break;
        case ACTION_GO_DOWN:
            goDown(&highlightedOptions[*depth], depthHeight, maxHeight, &scrollOffsets[*depth]);
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            break;
        case ACTION_GO_LEFT:
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            goLeft(depth, highlightedOptions);
            break;
        case ACTION_GO_UP:
            goUp(&highlightedOptions[*depth], depthHeight, maxHeight, &scrollOffsets[*depth]);
            resetNextDepth(highlightedOptions, *depth, scrollOffsets);
            break;
        case ACTION_DISMISS_MSG:
            createMsg(msg, NULL, NULL, MSG_TYPE_DISMISSED);
            break;
        case ACTION_DOWNLOAD:
            downloadFile(client, modules, highlightedOptions, *depth, msg);
            break;
        case ACTION_UPLOAD:
            uploadFiles(client, *depth, modules, highlightedOptions, uploadCommand, msg);
            break;
        default:
            break;
    }
}

void goRight(int *depth) {
    ++*depth;
}

void goDown(int *highlightedOption, int depthHeight, int maxHeight, int *scrollOffset) {
    if (*highlightedOption + *scrollOffset == depthHeight - 1) {
        *highlightedOption = 0;
        *scrollOffset = 0;
    }
    else {
        if (*highlightedOption >= maxHeight - SCROLLOFF && depthHeight - *scrollOffset - 1 > maxHeight)
            ++*scrollOffset;
        else
            ++*highlightedOption;
    }
}

void goLeft(int *depth, int *highlightedOptions) {
    --*depth;
}

void goUp(int *highlightedOption, int depthHeight, int maxHeight, int *scrollOffset) {
    if (*highlightedOption + *scrollOffset == 0) {
        if (depthHeight - 1 > maxHeight) {
            *scrollOffset = depthHeight - maxHeight - 1;
            *highlightedOption = maxHeight;
        }
        else
            *highlightedOption = depthHeight - 1;
    }
    else {
        if (*scrollOffset != 0 && *highlightedOption <= SCROLLOFF)
            --*scrollOffset;
        else
            --*highlightedOption;
    }
}

void resetNextDepth(int *highlightedOption, int depth, int *scrollOffsets) {
    if (depth < LAST_DEPTH - 1) {
        highlightedOption[depth + 1] = 0;
        scrollOffsets[depth + 1] = 0;
    }
}

void downloadFile(MDClient *client, MDArray modules, int *highlightedOptions, int depth, Message *msg) {
    MDFile mdFile;
    getMDFile(&mdFile, modules, highlightedOptions, depth, msg);
    if (checkIfAbort(*msg))
        return;
    FILE* file = fopen(mdFile.filename, "w");
    if (!file) {
        createMsg(msg, MSG_CANNOT_OPEN_DOWNLOAD_FILE, NULL, MSG_TYPE_ERROR);
        return;
    }

    MDError mdError;
    md_client_download_file(client, &mdFile, file, &mdError);
    fclose(file);
    if (mdError) {
        createMsg(msg, md_error_get_message(mdError), NULL, MSG_TYPE_ERROR);
        return;
    }
    createMsg(msg, MSG_DOWNLOADED, mdFile.filename, MSG_TYPE_SUCCESS);
}

void getMDFile(MDFile *mdFile, MDArray modules, int *highlightedOptions, int depth, Message *msg) {
    if (depth == MODULE_DEPTH2) {
        MDModule module = MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]];
        if (module.type == MD_MOD_RESOURCE) {
            MDModResource resource;
            resource = module.contents.resource;
            *mdFile = MD_FILES(resource.files)[highlightedOptions[MODULES_DEPTH]];
            return;
        }
    }
    createMsg(msg, MSG_NOT_FILE, NULL, MSG_TYPE_BAD_ACTION);
}

void uploadFiles(MDClient *client, int depth, MDArray modules, int *highlightedOptions, char *uploadCommand, Message *msg) {
    checkIfAssignmentOrWorkshop(modules, highlightedOptions, depth, msg);
    if (checkIfAbort(*msg))
        return;
    MDModule module = MD_MODULES(modules)[highlightedOptions[depth]];
    if (!uploadCommand[0]) {
        uploadCommand = DEFAULT_UPLOAD_COMMAND;
    }

    MDArray fileNames = MD_ARRAY_INITIALIZER;
    getFileNames(&fileNames, uploadCommand, msg);
    hidecursor();
    if (checkIfAbort(*msg))
        return;
    if (fileNames.len == 0) {
        createMsg(msg, MSG_NO_FILE_CHOSEN, NULL, MSG_TYPE_BAD_ACTION);
        return;
    }

    startUpload(client, module, fileNames, msg);
    if (checkIfAbort(*msg))
        return;
    setUploadSuccessMsg(fileNames.len, msg);
    md_array_free(&fileNames);
}

void checkIfAssignmentOrWorkshop(MDArray modules, int *highlightedOptions, int depth, Message *msg) {
    if (depth != MODULES_DEPTH) {
        createMsg(msg, MSG_NOT_ASSIGNMENT_OR_WORKSHOP, NULL, MSG_TYPE_BAD_ACTION);
        return;
    }
    MDModule module = MD_MODULES(modules)[highlightedOptions[depth]];
    if (module.type != MD_MOD_ASSIGNMENT && module.type != MD_MOD_WORKSHOP) {
        createMsg(msg, MSG_NOT_ASSIGNMENT_OR_WORKSHOP, NULL, MSG_TYPE_BAD_ACTION);
    }
}

void getFileNames(MDArray *fileNames, char *uploadCommand, Message *msg) {
    FILE *uploadPathsPipe = openFileSelectionProcess(uploadCommand, msg);
    if (checkIfAbort(*msg))
        return;
    readFileNames(uploadPathsPipe, fileNames, msg);
    pclose(uploadPathsPipe);
}

FILE *openFileSelectionProcess(char *uploadCommand, Message *msg) {
    errno = 0;
    FILE *uploadPathsPipe = popen(uploadCommand, "r");
    if (errno) {
        createMsg(msg, MSG_CANNOT_EXEC_UPLOAD_CMD, strerror(errno), MSG_TYPE_ERROR);
    }
    return uploadPathsPipe;
}

void readFileNames(FILE *uploadPathsPipe, MDArray *fileNames, Message *msg) {
    MDError mdError = MD_ERR_NONE;
    while(1) {
        char *fileName = malloc(sizeof(char) * UPLOAD_FILE_LENGTH);
        if (!fileName) {
            createMsg(msg, MSG_CANNOT_ALLOCATE, NULL, MSG_TYPE_ERROR);
            return;
        }
        if (fgets(fileName, UPLOAD_FILE_LENGTH, uploadPathsPipe)) {
            removeNewline(fileName);
            md_array_append(fileNames, &fileName, sizeof(char *), &mdError);
            if (mdError) {
                createMsg(msg, md_error_get_message(mdError), NULL, MSG_TYPE_ERROR);
                return;
            }
        }
        else
            return;
    }
}

void removeNewline(char *string) {
    int fileNameLen = strlen(string);
    if (string[fileNameLen - 1] == '\n')
        string[fileNameLen - 1] = 0;
}

void startUpload(MDClient *client, MDModule module, MDArray fileNames, Message *msg) {
    MDError mdError = MD_ERR_NONE;
    if (module.type == MD_MOD_ASSIGNMENT) {
        md_client_mod_assign_submit(client, &module, &fileNames, NULL, &mdError);
    }
    else if (module.type == MD_MOD_WORKSHOP) {
        createMsg(msg, "Workshop upload is not supported yet", NULL, MSG_TYPE_INFO);
        //md_client_mod_workshop_submit(client, &module, &MD_MAKE_ARR(cchar *, filePath), NULL, "hmmm", &error);
    }
    if (mdError)
        createMsg(msg, md_error_get_message(mdError), NULL, MSG_TYPE_ERROR);
}

void setUploadSuccessMsg(int nrOfFilesUploaded, Message *msg) {
    char *nrOfFilesStr = getStr(nrOfFilesUploaded);
    createMsg(msg, MSG_UPLOADED, nrOfFilesStr, MSG_TYPE_SUCCESS);
    free(nrOfFilesStr);
}
