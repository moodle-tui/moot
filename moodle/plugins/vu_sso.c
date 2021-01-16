/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * Hacky authentication system for Vilnius University Single-Oauth-System. Meant
 * to be used to log in into https://emokymai.vu.lt
 * 
 * Complying to moot auth plugin structure described in moodle.h
 */

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"
#include "auth.h"

#define URL_LENGTH 4096
#define WEBSITE "emokymai.vu.lt"
#define LAUNCH_URL "https://emokymai.vu.lt/admin/tool/mobile/launch.php?service=moodle_mobile_app&passport=208465373836651&urlscheme=moodlemobile"
#define PICK_STUDENTS "&src-dnVfbGRhcC1zdHVkZW50YWk%3D=VU+students"
#define SAML_PATTERN "name=\"SAMLResponse\" value=\""
#define SAML_RESPONSE "RelayState=https%3A%2F%2Femokymai.vu.lt%2Fadmin%2Ftool%2Fmobile%2Flaunch.php%3Fservice%3Dmoodle_mobile_app%26passport%3D208465373836651%26urlscheme%3Dmoodlemobile&SAMLResponse="
#define SAML_SUBMIT_URL "https://emokymai.vu.lt/auth/saml2/sp/saml2-acs.php/emokymai.vu.lt"
#define TOKEN_PREFIX "://token="
#define TOKEN_SEPARATOR ":::"

int is_supported(const char *url) {
    return strstr(url, WEBSITE) != NULL;
}

typedef const char cchar;

// struct to temporarily hold data while performing http request.
typedef struct Memblock {
    char *memory;
    size_t size;
} Memblock;

static size_t write_memblock_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memblock *mem = (struct Memblock *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static size_t simulate_write(void *contents, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}

char *get_token_initial(CURL *handle, cchar *user, cchar *pass);
char *get_token_source_pick_request(CURL *handle, cchar *redirectUrl, cchar *user, cchar *pass);
char *get_token_credentials_request(CURL *handle, cchar *redirectUrl, cchar *user, cchar *pass);
char *get_token_submit_saml(CURL *handle, cchar *saml);
char *decrypt_token(char *token);

char *get_token_initial(CURL *handle, cchar *user, cchar *pass) {
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, simulate_write);
    curl_easy_setopt(handle, CURLOPT_URL, LAUNCH_URL);
    if (curl_easy_perform(handle) == CURLE_OK) {
        char *redirect;
        curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &redirect);
        return get_token_source_pick_request(handle, redirect, user, pass);
    }
    return NULL;
}

char *get_token_source_pick_request(CURL *handle, cchar *redirectUrl, cchar *user, cchar *pass) {
    char url[URL_LENGTH];
    strncpy(url, redirectUrl, URL_LENGTH);
    strncat(url, PICK_STUDENTS, URL_LENGTH - strlen(url));

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, simulate_write);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    if (curl_easy_perform(handle) == CURLE_OK) {
        char *redirect;
        curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &redirect);
        return get_token_credentials_request(handle, redirect, user, pass);
    }
    return NULL;
}

char *get_token_credentials_request(CURL *handle, cchar *redirectUrl, cchar *user, cchar *pass) {
    user = curl_easy_escape(handle, user, 0);
    pass = curl_easy_escape(handle, pass, 0);
    char postFields[URL_LENGTH];
    snprintf(postFields, URL_LENGTH, "username=%s&password=%s", user, pass);
    curl_free((char *)user);
    curl_free((char *)pass);

    Memblock chunk = {
        .size = 0,
        .memory = calloc(1, 1),
    };
    if (!chunk.memory) {
        return NULL;
    }

    curl_easy_setopt(handle, CURLOPT_POST, 1);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postFields);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_memblock_callback);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &chunk);
    char url[strlen(redirectUrl) + 1];
    strcpy(url, redirectUrl);
    curl_easy_setopt(handle, CURLOPT_URL, url);
    char *token = NULL;
    if (curl_easy_perform(handle) == CURLE_OK && chunk.memory) {
        char *saml = strstr(chunk.memory, SAML_PATTERN);
        if (saml) {
            saml += sizeof(SAML_PATTERN) - 1;
            char *end = strchr(saml, '"');
            if (end) {
                saml = curl_easy_escape(handle, saml, end - saml);
                token = get_token_submit_saml(handle, saml);
                curl_free(saml);
            }
        }
    }
    free(chunk.memory);
    return token;
}

char *get_token_submit_saml(CURL *handle, cchar *saml) {
    char postFields[strlen(saml) + sizeof(SAML_RESPONSE)];
    strcpy(postFields, SAML_RESPONSE);
    strcat(postFields, saml);

    curl_easy_setopt(handle, CURLOPT_POST, 1);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, postFields);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, simulate_write);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, NULL);
    curl_easy_setopt(handle, CURLOPT_URL, SAML_SUBMIT_URL);
    curl_easy_perform(handle);

    char *redirect;
    curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &redirect);
    char *token = strstr(redirect, TOKEN_PREFIX);
    if (token) {
        token = decrypt_token(token + sizeof(TOKEN_PREFIX) - 1);
    }
    return token;
}

char *decrypt_token(char *token) {
    char *decoded = (char *)base64_decode((unsigned char *)token, strlen(token), &(size_t){0});
    if (decoded) {
        token = strstr(decoded, TOKEN_SEPARATOR);
        if (token) {
            token += sizeof(TOKEN_SEPARATOR) - 1;
            char *end = strstr(token, TOKEN_SEPARATOR);
            if (end) {
                *end = '\0';
            }
            char *buffer = malloc(strlen(token) + 1);
            if (buffer) {
                strcpy(buffer, token);
            }
            token = buffer;
        }
        free(decoded);
    }
    return token;
}

char *get_token(const char *url, const char *user, const char *pass) {
    CURL *handle = curl_easy_init();
    if (!handle)
        return NULL;

    curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_COOKIEFILE, "");

    char *token = get_token_initial(handle, user, pass);
    curl_easy_cleanup(handle);
    return token;
}

// Exposing the functions of this plugin.
MDPlugin MD_PLUGIN_NAME = {
    .isSupported = is_supported,
    .getToken = get_token,
};