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

MDModule *locate_module(MDArray courses, int courseId, int moduleId, int instance, MDError * error) {
    for (int i = 0; i < courses.len; ++i) {
        if (MD_COURSES(courses)[i].id == courseId)
            return md_course_locate_module(MD_COURSES(courses)[i], instance, moduleId, error);
    }
    return NULL;
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

    println("Accepting policy");
    json_value_free(md_client_do_http_json_request(client, &error, "core_user_agree_site_policy", ""));
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
    
    println("Checking specific modules");
    MDModule *assignment = locate_module(courses, 66, 788, 119, &error);
    if (error)
        goto end;
    assertend(assignment && assignment->type == MD_MOD_ASSIGNMENT);
    assertend(strcmp(assignment->name, "Assignment 2 (Upload)") == 0);


    println("Validating data");
    for (int i = 0; i < courses.len; ++i) {
        MDCourse *course = &MD_COURSES(courses)[i];
        teststr(course->name);
        println("%s", course->name);
        for (int j = 0; j < course->topics.len; ++j) {
            MDTopic *topic = &MD_TOPICS(course->topics)[j];
            println(" - %s", topic->name);
            teststr(topic->name);
            teststr(topic->summary.text);
            for (int k = 0; k < topic->modules.len; ++k) {
                MDModule *module = &MD_MODULES(topic->modules)[k];
                println("    - %s %d", module->name, module->instance);
                teststr(module->name);
                switch (module->type)
                {
                case MD_MOD_ASSIGNMENT:
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

    println("Testing module assignment submission");
    md_client_mod_assign_submit(client, assignment, MD_MAKE_ARR(cchar *, "moodle/test/test_file.txt"), &error);
    if (error)
        goto end;
    println("Check for success on https://school.moodledemo.net/mod/assign/view.php?id=%d", assignment->id);
end:
    if (error)
        printf("Error: %s\n", md_error_get_message(error));
    else 
        println("Done");
    free(token);
    md_client_cleanup(client);
    md_courses_cleanup(courses);
    curl_global_cleanup();
    return 0;
}