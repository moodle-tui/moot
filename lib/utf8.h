#ifndef __UTF8_H
#define __UTF8_H

#include <stdint.h>
#include <stdlib.h>

typedef uint_least32_t Rune;

size_t utf8decode(const char*, Rune*, size_t);
size_t utf8decodeNullTerm(const char* c, Rune* u);
size_t utf8encode(Rune, char*);

#endif
