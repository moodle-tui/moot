#ifndef __AUTH_H
#define __AUTH_H
#define MD_PLUGIN_NAME plugin
// Plugins and authentication

// While normal client creation and authentication requires a token, some moodle
// sites have a third party login system, which makes the process of getting the
// moodle token harder. Therefore custom plugins may be made for specific sites
// and loaded dinamically on runtime. They are basically shared libraries with
// exported variable named using MD_PLUGIN_NAME macro (type of MDPlugin);

// Plugin for authentication must have these two functions

// IsSupportedFunc should return non 0 when the module can be used to login
// to moodle system located on the given url.
typedef int (*IsSupportedFunc)(const char *url);

// GetTokenFunc must should try to login to given (supported) url using username
// and password, returning allocated moodle token or NULL.

typedef char *(*GetTokenFunc)(const char *url, const char *user, const char *pass);
// The plugin must expose the functions mentioned above using global MDPlugin
// variable named using MD_PLUGIN_NAME macro.
typedef struct MDPlugin {
    IsSupportedFunc isSupported;
    GetTokenFunc getToken;
} MDPlugin;

#endif