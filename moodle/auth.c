/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * Authentication plugin system for moodle sites. Auth plugins are loaded on
 * runtime and can be made outside of this tool. The interface of such plugins
 * is described in internal.h.
 */

#include "dlib.h"
#include "internal.h"
#define DO_STR(x) #x
#define STR(x) DO_STR(x)

// List of loaded plugins.
static MDArray plugins = MD_ARRAY_INITIALIZER;

char *md_auth_login(cchar *website, cchar *username, cchar *password, MDError *error) {
    for (int i = 0; i < plugins.len; ++i) {
        MDLoadedPlugin plugin = MD_ARR(plugins, MDLoadedPlugin)[i];
        if (plugin.plugin.isSupported(website)) {
            char *token = plugin.plugin.getToken(website, username, password);
            if (!token) {
                *error = MD_ERR_FAILED_PLUGIN_LOGIN;
            }
            return token;
        }
    }
    *error = MD_ERR_NO_MATCHING_PLUGIN_FOUND;
    md_error_set_message(website);
    return NULL;
}

void md_auth_load_plugin(cchar *filename, MDError *error) {
    *error = MD_ERR_NONE;
    void *lib = dl_open(filename);
    if (lib) {
        MDPlugin *plugin = dl_get_symbol(lib, STR(MD_PLUGIN_NAME));
        if (!plugin) {
            *error = MD_ERR_MISSING_PLUGIN_VAR;
            md_error_set_message(STR(MD_PLUGIN_NAME));
        } else if (!plugin->isSupported || !plugin->getToken) {
            *error = MD_ERR_INVALID_PLUGIN;
            md_error_set_message(filename);
            dl_close(lib);
        } else {
            MDLoadedPlugin loadedPlugin = {
                .plugin = *plugin,
                .handle = lib,
            };
            md_array_append(&plugins, &loadedPlugin, sizeof(loadedPlugin), error);
        }
    } else {
        *error = MD_ERR_FAILED_TO_LOAD_PLUGIN;
        md_error_set_message(dl_get_error());
    }
}

void md_auth_plugin_cleanup(MDLoadedPlugin *plugin) {
    dl_close(plugin->handle);
}

void md_auth_cleanup_plugins() {
    md_array_cleanup(&plugins, sizeof(MDLoadedPlugin), (MDCleanupFunc) md_auth_plugin_cleanup);
}