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

// List of loaded plugins.
static MDArray plugins = MD_ARRAY_INITIALIZER;

char *md_auth_login(cchar *website, cchar *username, cchar *password, MDError *error) {
    for (int i = 0; i < plugins.len; ++i) {
        MDPlugin plugin = MD_ARR(plugins, MDPlugin)[i];
        if (plugin.isSupported(website)) {
            char *token = plugin.getToken(website, username, password);
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
        MDPlugin plugin = {
            .isSupported = dl_get_symbol(lib, IS_SUPPORTED_NAME),
            .getToken = dl_get_symbol(lib, GET_TOKEN_NAME),
            .handle = lib,
        };
        if (!plugin.isSupported) {
            md_error_set_message(IS_SUPPORTED_NAME);
        }
        if (!plugin.getToken) {
            md_error_set_message(GET_TOKEN_NAME);
        }
        if (!plugin.isSupported || !plugin.getToken) {
            *error = MD_ERR_MISSING_PLUGIN_FUNCTION;
            md_auth_plugin_cleanup(&plugin);
        } else {
            md_array_append(&plugins, &plugin, sizeof(plugin), error);
        }
    } else {
        *error = MD_ERR_FAILED_TO_LOAD_PLUGIN;
        md_error_set_message(dl_get_error());
    }
}

void md_auth_plugin_cleanup(MDPlugin *plugin) {
    dl_close(plugin->handle);
}

void md_auth_cleanup_plugins() {
    md_array_cleanup(&plugins, sizeof(MDPlugin), (MDCleanupFunc) md_auth_plugin_cleanup);
}