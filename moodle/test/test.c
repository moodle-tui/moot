#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "internal.h"
#include "json.h"
#include "moodle.h"
#define println(format, ...) printf(format "\n", ##__VA_ARGS__)
#define assertend(expr) if (!(expr)) {println("Assert failed: %s\nline:%d", #expr, __LINE__);goto end;}
#define teststr(str) assertend(str != NULL)

char* get_token(MDError* error) {
    char* data = http_get_request(
        "https://school.moodledemo.net/login/token.php"
        "?username=markellis267&password=moodle&service=moodle_mobile_app",
        error);
    char* token = NULL;
    if (!*error) {
        json_value* json = md_parse_moodle_json(data, error);
        if (!*error) {
            token = json_get_string(json, "token", error);
        }
        json_value_free(json);
    }
    free(data);
    return token;
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    MDError error = MD_ERR_NONE;
    char* token = get_token(&error);
    if (error)
        goto end;
    println("Creating client");
    MDClient* client = md_client_new(token, "https://school.moodledemo.net", &error);
    if (error)
        goto end;

    println("Initializing client");
    md_client_init(client, &error);
    if (error)
        goto end;

    println("Acceptint policy");
    md_client_do_http_json_request(client, &error, "core_user_agree_site_policy", "");
    if (error)
        goto end;


    println("Client information:");
    println("Site: %s", client->siteName);
    println("Name: %s", client->fullName);
    assertend(!strcmp(client->siteName, "Mount Orange School"));
    assertend(!strcmp(client->fullName, "Mark Ellis"));
    assertend(client->uploadLimit > 0);

    println("Fetching courses");
    MDArray courses = md_client_fetch_courses(client, &error);
    if (error)
        goto end;
    
    println("Validating data");
    for (int i = 0; i < courses.len; ++i) {
        MDCourse *course = &MD_COURSES(courses)[i];
        teststr(course->name);
        for (int j = 0; j < course->topics.len; ++j) {
            MDTopic *topic = &MD_TOPICS(course->topics)[j];
            teststr(topic->name);
            teststr(topic->summary.text);
            for (int k = 0; k < topic->modules.len; ++k) {
                MDModule *module = &MD_MODULES(topic->modules)[k];
                teststr(module->name);
                switch (module->type)
                {
                case MD_MOD_ASSIGNMENT:
                    println("%s %d %d", course->name, course->id, module->instance);
                    teststr(module->contents.assignment.description.text);
                    for (int l = 0; l < module->contents.assignment.files.len; ++l) {
                        teststr(MD_FILES(module->contents.assignment.files)[l].filename);
                        teststr(MD_FILES(module->contents.assignment.files)[l].url);
                        assertend(MD_FILES(module->contents.assignment.files)[l].filesize > 0);
                    }
                    break;
                case MD_MOD_WORKSHOP:
                    teststr(module->contents.workshop.description.text);
                    teststr(module->contents.workshop.instructions.text);
                    break;
                case MD_MOD_RESOURCE:
                    teststr(module->contents.resource.description.text);
                    for (int l = 0; l < module->contents.resource.files.len; ++l) {
                        teststr(MD_FILES(module->contents.resource.files)[l].filename);
                        teststr(MD_FILES(module->contents.resource.files)[l].url);
                        assertend(MD_FILES(module->contents.resource.files)[l].filesize > 0);
                    }
                    break;
                case MD_MOD_URL:
                    teststr(module->contents.url.description.text);
                    teststr(module->contents.url.name);
                    teststr(module->contents.url.url);
                    break;
                default:
                    assertend(("Unexpected MOD TYPE", false));
                }
            }
            
        }
    }

    // md_client_init(client, &err);
end:
    if (error)
        printf("Error: %s\n", md_error_get_message(error));
    else 
        println("Done");
    free(token);
    curl_global_cleanup();
    return 0;
    // test_upload();

    // curl_global_cleanup();
    // return 0;

    // MDError err = MD_ERR_NONE;
    // FILE* f = fopen(".token", "r");
    // char token[100];
    // fread_line(f, token, 99);
    // MDClient* client = md_client_new(token, "https://emokymai.vu.lt", &err);
    // md_client_init(client, &err);
    // if (!err) {
    //     printf("Site: %s\nName: %s\n", client->siteName, client->fullName);
    //     printf("%d\n", err);
    //     // return 0;
    // } else {
    //     printf("%d %s\n", err, md_error_get_message(err));
    //     return 0;
    // }
    // MDArray courseArr = md_client_fetch_courses(client, &err);
    // MDCourse* courses = MD_ARR(courseArr, MDCourse);

    // printf("%d\n", err);
    // if (!err) {
    //     for (int i = 0; i < courseArr.len; ++i) {
    //         printf("%s %d\n", courses[i].name, courses[i].id);
    //         for (int j = 0; j < courses[i].topics.len; ++j) {
    //             MDArray modules = MD_TOPICS(courses[i].topics)[j].modules;
    //             printf(" - %s\n", MD_TOPICS(courses[i].topics)[j].name);
    //             for (int k = 0; k < modules.len; ++k) {
    //                 printf("  - %s\n", MD_MODULES(modules)[k].name);
    //                 if (MD_MODULES(modules)[k].type == MD_MOD_RESOURCE) {
    //                     for (int a = 0; a < MD_MODULES(modules)[k].contents.resource.files.len; ++a) {
    //                         printf("[%s]\n", MD_FILES(MD_MODULES(modules)[k].contents.resource.files)[a].filename);
    //                         FILE* f = fopen(MD_FILES(MD_MODULES(modules)[k].contents.resource.files)[a].filename,
    //                         "w"); if (!f)
    //                             perror("noppe");
    //                         md_client_download_file(
    //                             client, &MD_ARR(MD_ARR(modules, MDModule)[k].contents.resource.files, MDFile)[a], f,
    //                             &err);
    //                         fclose(f);
    //                         return 0;
    //                     }

    //                     // printf("[%s]\n", modules.data[k].contents.assignment.description.text);
    //                     // printf("[%s]\n", modules.data[k].contents.workshop.description.text);
    //                 }
    //             }
    //         }
    //     }

    // } else {
    //     printf("%s\n", md_error_get_message(err));
    // }
    // // return 0;
    // md_courses_cleanup(courseArr);

    // md_client_cleanup(client);
    // fclose(f);
    // curl_global_cleanup();
}