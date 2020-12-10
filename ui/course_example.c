#include "ui.h"

void initExCourse(Courses *courses) {
    courses->len = 1;
    courses->data[0] = (Course){
        .id = 0,
        .name = "Chemistry",
        .topics = {.len = 2}
    };
    courses->data[0].topics.data[0] = (Topic){
        .id = 0,
        .name = "chemistry topic 1",
        .modules = {.len = 3}
    };
    courses->data[0].topics.data[0].modules.data[0] = (Module){
        .id = 0,
        .type = MODULE_ASSIGNMENT,
        .name = "Do this"
    };
    courses->data[0].topics.data[0].modules.data[1] = (Module){
        .id = 0,
        .type = MODULE_ASSIGNMENT,
        .name = "NO! Do this"
    };
    courses->data[0].topics.data[0].modules.data[2] = (Module){
        .id = 0,
        .type = MOD_UNSUPPORTED,
        .name = "Do whatever"
    };

    courses->data[0].topics.data[1] = (Topic){
        .id = 0,
        .name = "chemistry topic 2",
        .modules = {.len = 2}
    };
    courses->data[0].topics.data[1].modules.data[0] = (Module){
        .id = 0,
        .type = MODULE_WORKSHOP,
        .name = "Workshop example"
    };
    courses->data[0].topics.data[1].modules.data[1] = (Module){
        .id = 0,
        .type = MODULE_RESOURCE,
        .name = "Very useful recourse"
    };
}
