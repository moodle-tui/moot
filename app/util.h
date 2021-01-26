/*
 * Ramojus Lapinskas ramojus.lap@gmail.com
 * Nojus Gudinavičius nojus.gudinavicius@gmail.com
 * licensed as with https://github.com/moodle-tui/moot
 */
 
#ifndef __UTIL_H
#define __UTIL_H
#include "message.h"

void *xmalloc(size_t size, Message *msg);
void *xrealloc(void *data, size_t size, Message *msg);
void *xcalloc(size_t n, size_t size, Message *msg);

#endif
