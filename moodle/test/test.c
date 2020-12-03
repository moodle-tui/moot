#include <curl/curl.h>
#include <stdio.h>
#include <threads.h>

#include "client.h"

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

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    ErrorCode err = 0;

    // printf("%lld\n",
    //        httpPostFile("https://emokymai.vu.lt/webservice/upload.php?token=216ddc5f39111d27148473a63f80bab4",
    //                     "./out.txt", "file_box", &err)
    //             )
    //             ;
    // printf("%d\n", err);

    // return 0;

    FILE *f = fopen(".token", "r");
    char token[100];
    fread_line(f, token, 99);
    Client *client = mt_new_client(token, "https://emokymai.vu.lt");
    if (!(err = mt_init_client(client))) {
        printf("Site: %s\nName: %s\n", client->siteName, client->fullName);

        printf("%d\n", mt_client_upload_file(client, "out.txt", ITEM_ID_NONE, &err));
        printf("%d\n", err);
        return 0;
    } else {
        printf("%d %s\n", err, getError(err));
        return 0;
    }
    Courses courses = mt_get_courses(client, &err);
    printf("%d\n", courses.len);
    for (int i = 0; i < courses.len; ++i) {
        printf("%s %d\n", courses.data[i].name, courses.data[i].id);
        for (int j = 0; j < courses.data[i].topics.len; ++j) {
            Modules modules = courses.data[i].topics.data[j].modules;
            printf(" - %s\n", courses.data[i].topics.data[j].name);
            for (int k = 0; k < modules.len; ++k) {
                printf("  - %s\n", modules.data[k].name);
            }
        }
    }
    mt_free_courses(courses);

    mt_destroy_client(client);
    fclose(f);
    curl_global_cleanup();
}