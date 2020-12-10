#include <stdint.h>
#include <stdlib.h>

#ifndef UTF8_H
#define UTF8_H

/* Arbitrary sizes */
#define UTF_INVALID 0xFFFD
#define UTF_SIZ 4
#define ESC_BUF_SIZ (128 * UTF_SIZ)
#define ESC_ARG_SIZ 16
#define STR_BUF_SIZ ESC_BUF_SIZ
#define STR_ARG_SIZ ESC_ARG_SIZ
#define HISTSIZE 2000

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef uint_least32_t Rune;
static uchar utfbyte[UTF_SIZ + 1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static uchar utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static Rune utfmin[UTF_SIZ + 1] = {0, 0, 0x80, 0x800, 0x10000};
static Rune utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static size_t utf8decode(const char*, Rune*, size_t);
static Rune utf8decodebyte(char, size_t*);
static char utf8encodebyte(Rune, size_t);
static size_t utf8validate(Rune*, size_t);
static size_t utf8encode(Rune, char*);

/* macros */
#define LEN(a) (sizeof(a) / sizeof(a)[0])
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))

#endif
