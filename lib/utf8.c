/* code taken from https://st.suckless.org/
 *
 * MIT/X Consortium License
 * 
 * © 2014-2020 Hiltjo Posthuma <hiltjo at codemadness dot org>
 * © 2018 Devin J. Pohly <djpohly at gmail dot com>
 * © 2014-2017 Quentin Rameau <quinq at fifth dot space>
 * © 2009-2012 Aurélien APTEL <aurelien dot aptel at gmail dot com>
 * © 2008-2017 Anselm R Garbe <garbeam at gmail dot com>
 * © 2012-2017 Roberto E. Vargas Caballero <k0ga at shike2 dot com>
 * © 2012-2016 Christoph Lohmann <20h at r-36 dot net>
 * © 2013 Eon S. Jeon <esjeon at hyunmu dot am>
 * © 2013 Alexander Sedov <alex0player at gmail dot com>
 * © 2013 Mark Edgar <medgar123 at gmail dot com>
 * © 2013-2014 Eric Pruitt <eric.pruitt at gmail dot com>
 * © 2013 Michael Forney <mforney at mforney dot org>
 * © 2013-2014 Markus Teich <markus dot teich at stusta dot mhn dot de>
 * © 2014-2015 Laslo Hunhold <dev at frign dot de>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utf8.h"

typedef uint_least32_t Rune;

static Rune utf8decodebyte(char, size_t*);
static size_t utf8validate(Rune*, size_t);
static char utf8encodebyte(Rune, size_t);

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

static uchar utfbyte[UTF_SIZ + 1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static uchar utfmask[UTF_SIZ + 1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static Rune utfmin[UTF_SIZ + 1] = {0, 0, 0x80, 0x800, 0x10000};
static Rune utfmax[UTF_SIZ + 1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

/* macros */
#define LEN(a) (sizeof(a) / sizeof(a)[0])
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))

size_t utf8decode(const char* c, Rune* u, size_t clen) {
    size_t i, j, len, type;
    Rune udecoded;

    *u = UTF_INVALID;
    if (!clen)
        return 0;
    udecoded = utf8decodebyte(c[0], &len);
    if (!BETWEEN(len, 1, UTF_SIZ))
        return 1;
    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf8validate(u, len);
    return len;
}

size_t utf8decodeNullTerm(const char* c, Rune* u) {
    size_t i, j, len, type;
    Rune udecoded;

    *u = UTF_INVALID;
    if (!*c)
        return 0;
    udecoded = utf8decodebyte(c[0], &len);
    if (!BETWEEN(len, 1, UTF_SIZ))
        return 1;
    for (i = 1, j = 1; c[i] && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf8decodebyte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf8validate(u, len);
    return len;
}

Rune utf8decodebyte(char c, size_t* i) {
    for (*i = 0; *i < LEN(utfmask); ++(*i))
        if (((uchar)c & utfmask[*i]) == utfbyte[*i])
            return (uchar)c & ~utfmask[*i];

    return 0;
}

size_t utf8validate(Rune* u, size_t i) {
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) || BETWEEN(*u, 0xD800, 0xDFFF))
        *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i)
        ;

    return i;
}

size_t utf8encode(Rune u, char* c) {
    size_t len, i;

    len = utf8validate(&u, 0);
    if (len > UTF_SIZ)
        return 0;

    for (i = len - 1; i != 0; --i) {
        c[i] = utf8encodebyte(u, 0);
        u >>= 6;
    }
    c[0] = utf8encodebyte(u, len);

    return len;
}

char utf8encodebyte(Rune u, size_t i) {
    return utfbyte[i] | (u & ~utfmask[i]);
}
