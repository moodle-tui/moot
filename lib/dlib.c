/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * See dlib.h
 */

#include "dlib.h"

#if defined(_WIN32)
#define PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

void *dl_open(const char *filename) {
#ifdef PLATFORM_WINDOWS
    return LoadLibrary(filename);
#else
    return dlopen(filename, RTLD_NOW);
#endif
}

void dl_close(void *handle) {
#ifdef PLATFORM_WINDOWS
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

void *dl_get_symbol(void *handle, const char *symbol) {
#ifdef PLATFORM_WINDOWS
    return GetProcAddress(handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}

char *dl_get_error() {
#ifdef PLATFORM_WINDOWS
    return "get error not implemented on windows";
#else
    return dlerror();
#endif
}
