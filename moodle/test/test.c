#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "internal.h"
#include "json.h"
#include "moodle.h"

// printf with a newline
#define println(format, ...) printf(format "\n", ##__VA_ARGS__)

// similar to assert, but instead of terminating makes a goto to end;
#define assertend(expr)                                         \
    if (!(expr)) {                                              \
        println("Assert failed: %s\nline:%d", #expr, __LINE__); \
        goto end;                                               \
    }

// Checks if a string is valid.
#define teststr(str) assertend(str != NULL)

#define DEMO_SITE "https://school.moodledemo.net"
#define SAVE_FILE "file.bin"

// Gets token from
char *get_token(MDError *error) {
    char *data = http_get_request(DEMO_SITE
                                  "/login/token.php"
                                  "?username=markellis267&password=moodle&service=moodle_mobile_app",
                                  error);
    char *token = NULL;
    if (!*error) {
        json_value *json = md_parse_moodle_json(data, error);
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
    bool cleanStatus = false, cleanClient = false, cleanCourses = false;
    MDError error = MD_ERR_NONE;
    char *token = get_token(&error);
    if (error)
        goto end;

    println("Creating client");
    MDClient *client = md_client_new(token, DEMO_SITE, &error);
    cleanClient = true;
    if (error)
        goto end;

    println("Initializing client");
    md_client_init(client, &error);
    if (error)
        goto end;

    md_client_save_to_file(client, SAVE_FILE, &error);
    if (!error) {
        md_client_cleanup(client);
        client = md_client_load_from_file(SAVE_FILE, &error);
    }
    assertend(error == MD_ERR_NONE);
    println("Client information:");
    println("Site: %s", client->siteName);
    println("Name: %s", client->fullName);
    assertend(!strcmp(client->siteName, "Mount Orange School"));
    assertend(!strcmp(client->fullName, "Mark Ellis"));
    assertend(client->uploadLimit > 0);

    println("Accepting policy");
    json_value_free(md_client_do_http_json_request(client, &error, "core_user_agree_site_policy", ""));
    if (error)
        goto end;


    println("Fetching courses");
    MDArray courses = md_client_fetch_courses(client, &error);
    cleanCourses = true;
    if (error)
        goto end;

    println("Checking specific modules");
    MDModule *assignment1 = md_courses_locate_module(courses, 66, 788, 119, &error);
    MDModule *assignment2 = md_courses_locate_module(courses, 56, 573, 95, &error);
    if (error)
        goto end;
    assertend(assignment1->type == MD_MOD_ASSIGNMENT);
    assertend(assignment2->type == MD_MOD_ASSIGNMENT);
    assertend(strcmp(assignment1->name, "Assignment 2 (Upload)") == 0);
    assertend(strcmp(assignment2->name, "Assignment: Causes of the October 1917 Revolution") == 0);

    MDModule *workshop1 = md_courses_locate_module(courses, 6, 90, 1, &error);
    if (error)
        goto end;
    assertend(workshop1->type == MD_MOD_WORKSHOP);
    assertend(strcmp(workshop1->name, "Simulation -  Remake the film!") == 0);

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
                switch (module->type) {
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
                        assertend("Unexpected MOD TYPE" == false);
                }
            }
        }
    }

    println("Testing module assignment submission");
    md_client_mod_assign_submit(client, assignment1, &MD_MAKE_ARR(cchar *, "moodle/test/test_file.txt"),
                                &(MDRichText){"- -", MD_FORMAT_MARKDOWN}, &error);
    md_client_mod_assign_submit(client, assignment2, &MD_MAKE_ARR(cchar *, "moodle/test/test_file.txt"), NULL, &error);
    if (error)
        goto end;
    println("Ok. Check for success on " DEMO_SITE "/mod/assign/view.php?id=%d", assignment1->id);
    println("Ok. Check for success on " DEMO_SITE "/mod/assign/view.php?id=%d", assignment2->id);

    println("Testing module workshop submission");
    md_client_mod_workshop_submit(client, workshop1, &MD_MAKE_ARR(cchar *, "moodle/test/test_file.txt"), NULL, "Tit le", &error);
    if (error) {
        println("Error: %s", md_error_get_message(error));
        if (error == MD_ERR_MOODLE_EXCEPTION) {
            println("This may be caused by the workshop being already submitted.");
        } else {
            goto end;
        }
    }
    println("Ok. Check for success on " DEMO_SITE "/mod/workshop/view.php?id=%d", workshop1->id);

    println("Loading status of modules");
    MDLoadedStatus status = md_courses_load_status(client, courses, &error);
    cleanStatus = true;
    if (error)
        goto end;
    md_loaded_status_apply(status);
    println("Checking the correctness of loaded status");

    // existing assignment
    MDModule *assignment3 = md_courses_locate_module(courses, 62, 724, 111, &error);
    if (error)
        goto end;
    assertend(assignment3->contents.assignment.status.state == MD_MOD_ASSIGNMENT_STATE_SUBMITTED);
    assertend(assignment3->contents.assignment.status.submitDate == 1407676072);
    assertend(strcmp(assignment3->contents.assignment.status.grade, "55.00 / 100.00") == 0);
    assertend(assignment3->contents.assignment.status.submittedFiles.len > 0);
    assertend(strcmp(MD_FILES(assignment3->contents.assignment.status.submittedFiles)[0].filename,
                     "CinemaSpec_ME267.pdf") == 0);

    // existing workshop
    MDModule *workshop2 = md_courses_locate_module(courses, 56, 575, 10, &error);
    if (error)
        goto end;
    assertend(workshop2->contents.workshop.status.submitted == false);

    // previously submitted workshop.
    assertend(workshop1->contents.workshop.status.submitted == true);
    assertend(!strcmp(workshop1->contents.workshop.status.title, "Tit le"));
    assertend(workshop1->contents.workshop.status.submittedFiles.len == 1);
    assertend(!strcmp(MD_FILES(workshop1->contents.workshop.status.submittedFiles)[0].filename, "test_file.txt"));

end:
    if (error)
        printf("Error: %s\n", md_error_get_message(error));
    else
        println("Done");

    // Cleaning up.
    free(token);

    if (cleanStatus)
        md_loaded_status_cleanup(status);
    if (cleanClient)
        md_client_cleanup(client);
    if (cleanCourses)
        md_courses_cleanup(courses);
    curl_global_cleanup();
    return 0;
}