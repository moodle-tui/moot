/*
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * Licensed as with https://github.com/moodle-tui/moot
 *
 * A wrapper library for dynamic shared library loading using POSIX
 * and Win32 api.
 */

#ifndef __DLIB_H
#define __DLIB_H

// dl_open opens a shared library, returning handle or NULL. Handle should be
// freed by the caller using dl_close.
void *dl_open(const char *filename);

// dl_close closes a shared library opened using dl_open.
void dl_close(void *handle);

// dl_get_symbol finds and returns a symbol from a handle returned by dl_open,
// or NULL on failure.
void *dl_get_symbol(void *handle, const char *symbol);

// dl_get_error returns last error if any occurred.
char *dl_get_error();

#endif