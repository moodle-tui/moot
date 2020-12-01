#include <stdio.h>
#include <curl/curl.h>

#include "client.h"
#include "util.h"

/* Reads single line up to n bytes from file with \n removed. s should be at
   least n + 1 in length. 1 is returned if the line is too long and was cut off. */
int fread_line(FILE *file, char *s, int n) {
    s[0] = '\0';
    int len = 0, cutOff = 0;
    char c = 0;
    do {
        c = fgetc(file);
        if (c != '\n' && !feof(file)) {
            if (len == n) cutOff = 1;

            if (len < n) s[len++] = c;
        }
    } while (!feof(file) && c != '\n');
    s[len] = '\0';
    return cutOff;
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    FILE *f = fopen("../.token", "r");
    char token[100];
    fread_line(f, token, 99);
    Client *client = mt_new_client(token, "https://emokymai.vu.lt");
    if (mt_init_client(client) == ERR_NONE) {
        printf("Site: %s\nName: %s\n", client->siteName, client->fullName);
    }
    Courses courses = mt_get_courses(client);
    printf("%d\n", courses.len);
    for (int i = 0; i < courses.len; ++i) {
        printf("%s\n", courses.data[i].name);
    }
    mt_free_courses(courses);

    mt_destroy_client(client);
    fclose(f);
    curl_global_cleanup();
}