#include <curl/curl.h>
#include <stdio.h>
#include <time.h>

#include "moodle.h"
// #include "client.h"

/* Reads single line up to n bytes from file with \n removed. s should be at
   least n + 1 in length. 1 is returned if the line is too long and was cut off. */
int fread_line(FILE *file, char *s, int n) {
    s[0] = '\0';
    int len = 0, cutOff = 0;
    char c = 0;
    do {
        c = fgetc(file);
        if (c != '\n' && !feof(file)) {
            if (len == n)
                cutOff = 1;

            if (len < n)
                s[len++] = c;
        }
    } while (!feof(file) && c != '\n');
    s[len] = '\0';
    return cutOff;
}

void test_upload() {
    // https://school.moodledemo.net/login/token.php?username=markellis267&password=moodle&service=moodle_mobile_app
    MDClient *client = mt_new_client("00aa0cd99c8a55577ee8d2987641d4d4", "https://school.moodledemo.net");
    MDError err = 0;
    if (!(err = md_client_init(client))) {
        printf("Site: %s\nName: %s\n", client->siteName, client->fullName);

        // Module mod = {665, 101, MODULE_ASSIGNMENT, "name"};
        MDModule wk = {90, 1, MD_MODULE_WORKSHOP, "name"};

        mt_client_mod_workshop_submit(client, &wk, MD_MAKE_ARR(const char *, "README.md"), "t", &err);
        printf("%s\n", md_get_error_message(err));
    } else {
        printf("%d %s\n", err, md_get_error_message(err));
    }
    mt_destroy_client(client);
}

int main() {
    time(NULL);

    curl_global_init(CURL_GLOBAL_ALL);

    // test_upload();

    // curl_global_cleanup();
    // return 0;

    MDError err = MD_ERR_NONE;
    FILE *f = fopen(".token", "r");
    char token[100];
    fread_line(f, token, 99);
    MDClient *client = mt_new_client(token, "https://emokymai.vu.lt");
    if (!(err = md_client_init(client))) {
        printf("Site: %s\nName: %s\n", client->siteName, client->fullName);
        printf("%d\n", err);
        // return 0;
    } else {
        printf("%d %s\n", err, md_get_error_message(err));
        return 0;
    }
    MDArray courseArr = mt_get_courses(client, &err);
    MDCourse *courses = MD_ARR(courseArr, MDCourse);

    printf("%d\n", err);
    if (!err) {
        for (int i = 0; i < courseArr.len; ++i) {
            printf("%s %d\n", courses[i].name, courses[i].id);
            for (int j = 0; j < courses[i].topics.len; ++j) {
                MDArray modules = MD_TOPICS(courses[i].topics)[j].modules;
                printf(" - %s\n", MD_TOPICS(courses[i].topics)[j].name);
                for (int k = 0; k < modules.len; ++k) {
                    printf("  - %s\n", MD_MODULES(modules)[k].name);
                    if (MD_MODULES(modules)[k].type == MD_MODULE_RESOURCE) {
                        for (int a = 0; a < MD_MODULES(modules)[k].contents.resource.files.len; ++a) {
                            printf("[%s]\n", MD_FILES(MD_MODULES(modules)[k].contents.resource.files)[a].filename);
                            FILE *f = fopen(MD_FILES(MD_MODULES(modules)[k].contents.resource.files)[a].filename, "w");
                            if (!f)
                                perror("noppe");
                            mt_client_download_file(client, &MD_ARR(MD_ARR(modules, MDModule)[k].contents.resource.files, MDFile)[a], f,
                                                          &err);
                            fclose(f);
                            return 0;
                        }

                        // printf("[%s]\n", modules.data[k].contents.assignment.description.text);
                        // printf("[%s]\n", modules.data[k].contents.workshop.description.text);
                    }
                }
            }
        }

    } else {
        printf("%s\n", md_get_error_message(err));
    }
    // return 0;
    md_courses_cleanup(courseArr);

    mt_destroy_client(client);
    fclose(f);
    curl_global_cleanup();
}