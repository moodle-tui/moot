#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "rlutil.h"
#include "app.h"
#include "moodle.h"

Action getAction(MDArray courses, KeyDef keyDef, int depth, int currentMaxDepth, int depthHeight) {
    Action action;
    switch (keyDef) {
        case KD_RIGHT:
            if (depth < currentMaxDepth)
                action = ACTION_GO_RIGHT;
            else
                action = ACTION_INVALID;
            break;
        case KD_DOWN:
            if (depthHeight == 1)
                action = ACTION_INVALID;
            else 
                action = ACTION_GO_DOWN;
            break;
        case KD_LEFT:
            if (depth != 0)
                action = ACTION_GO_LEFT;
            else
                action = ACTION_INVALID;
            break;
        case KD_UP:
            if (depthHeight == 1)
                action = ACTION_INVALID;
            else
                action = ACTION_GO_UP;
            break;
        case KD_DOWNLOAD:
            action = ACTION_DOWNLOAD;
            break;
        case ACTION_UPLOAD:
            action = ACTION_UPLOAD;
            break;
        case KD_QUIT:
            action = ACTION_QUIT;
            break;
    }
    return action;
}

void doAction(Action action, MDArray courses, MDClient *client, int *highlightedOptions,
        int *depth, int *scrollOffsets, char *uploadCommand, Error *error, MDError *mdError) {
    int depthHeight = getDepthHeight(*depth, courses, highlightedOptions) - 1;
    MDArray topics = MD_COURSES(courses)[highlightedOptions[COURSES_DEPTH]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[TOPICS_DEPTH]].modules;
    MDFile mdFile;
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
        case ACTION_DOWNLOAD:
            mdFile = getMDFile(courses, highlightedOptions);
            downloadFile(mdFile, client);
            break;
        case ACTION_UPLOAD:
            uploadFiles(client, MD_MODULES(modules)[highlightedOptions[MODULES_DEPTH]], uploadCommand, error, mdError);
            break;
        default:
            break;
    }
}

void goRight(int *depth) {
    ++*depth;
}

void goDown(int *highlightedOption, int depthHeight, int maxHeight, int *scrollOffset) {
    if (*highlightedOption + *scrollOffset == depthHeight) {
        *highlightedOption = 0;
        *scrollOffset = 0;
    }
    else {
        if (*highlightedOption >= maxHeight - SCROLLOFF && depthHeight - *scrollOffset > maxHeight)
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
        if (depthHeight > maxHeight) {
            *scrollOffset = depthHeight - maxHeight;
            *highlightedOption = maxHeight;
        }
        else
            *highlightedOption = depthHeight;
    }
    else {
        if (*scrollOffset != 0 && *highlightedOption <= SCROLLOFF)
            --*scrollOffset;
        else
            --*highlightedOption;
    }
}

void resetNextDepth(int *highlightedOptions, int depth, int *scrollOffsets) {
    if (depth < LAST_DEPTH - 1) {
        highlightedOptions[depth + 1] = 0;
        scrollOffsets[depth + 1] = 0;
    }
}

MDFile getMDFile(MDArray courses, int *highlightedOptions) {
    MDArray topics = MD_COURSES(courses)[highlightedOptions[0]].topics;
    MDArray modules = MD_TOPICS(topics)[highlightedOptions[1]].modules;
    MDFile mdFile;
    if (modules.len > 0 && MD_MODULES(modules)[highlightedOptions[2]].type == MD_MOD_RESOURCE) {
        MDModResource resource;
        resource = MD_MODULES(modules)[highlightedOptions[2]].contents.resource;
        mdFile = MD_FILES(resource.files)[highlightedOptions[4]];
    }
    else
        msgSmall("Couldn't get file", RED);

    return mdFile;
}

void downloadFile(MDFile mdFile, MDClient *client) {
    FILE* file = fopen(mdFile.filename, "w");
    char msg[1000];
    if (!file) {
        sprintf(msg, "Couldn't open %s for writing", mdFile.filename);
        msgErr(msg);
    }
    MDError err;
    sprintf(msg, "Downloading: %s [%ldK]", mdFile.filename, mdFile.filesize / 1000);
    msgSmall(msg, BLUE);
    md_client_download_file(client, &mdFile, file, &err);
    sprintf(msg, "Downloaded: %s [%ldK]", mdFile.filename, mdFile.filesize / 1000);
    msgSmall(msg, GREEN);
    fclose(file);

    if (err) {
        char msg[100];
        sprintf(msg, "%s", md_error_get_message(err));
        msgErr(msg);
    }
}

void uploadFiles(MDClient *client, MDModule module, char *uploadCommand, Error *error, MDError *mdError) {
    if (!uploadCommand) {
        uploadCommand = UPLOAD_COMMAND;
    }
    FILE *uploadPathsPipe = openFileSelectionProcess(uploadCommand, error);
    if (*error)
        return;
    MDArray fileNames = MD_ARRAY_INITIALIZER;
    for (int i = 0;; ++i) {
        char *fileName = xmalloc(sizeof(char) * UPLOAD_FILE_LENGTH, error);
        if (*error)
            return;
        if (!fgets(fileName, UPLOAD_FILE_LENGTH, uploadPathsPipe))
            break;

        removeNewline(fileName);
        MDError mdError = MD_ERR_NONE;
        md_array_append(&fileNames, &fileName, sizeof(char *), &mdError);
    }
    pclose(uploadPathsPipe);
    hidecursor();
    if (fileNames.len == 0) {
        msgSmall("No files chosen", RED);
        return;
    }
    if (!*mdError && !*error)
        startUpload(client, module, fileNames, mdError);
    if (!*mdError)
        msgSmall(UPLOAD_SUCCESFUL_MSG, UPLOAD_SUCCESFUL_MSG_COLOR);
    md_array_free(&fileNames);
}

FILE *openFileSelectionProcess(char *uploadCommand, Error *error) {
    errno = 0;
    FILE *uploadPathsPipe = popen(uploadCommand, "r");
    if (errno) {
        app_error_set_message(strerror(errno));
        *error = UPLOAD_ERR_EXEC;
    }
    return uploadPathsPipe;
}

void removeNewline(char *string) {
    int fileNameLen = strlen(string);
    if (string[fileNameLen - 1] == '\n')
        string[fileNameLen - 1] = 0;
}

void startUpload(MDClient *client, MDModule module, MDArray fileNames, MDError *mdError) {
    if (module.type == MD_MOD_ASSIGNMENT) {
        md_client_mod_assign_submit(client, &module, &fileNames, NULL, mdError);
    }
    else if (module.type == MD_MOD_WORKSHOP) {
        msgSmall("Workshop upload is not supported yet", RED);
        //md_client_mod_workshop_submit(client, &module, &MD_MAKE_ARR(cchar *, filePath), NULL, "hmmm", &error);
    }
    else {
        msgSmall("This is not a workshop or assignment", RED);
    }
    if (*mdError)
        msgErr(md_error_get_message(*mdError));
}


