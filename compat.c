/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "compat.h"

#ifdef HOMEGROWN_STRERROR

extern int   sys_nerr;
extern char *sys_errlist[];

char *strerror(int errnum)
{
    static char *errtext = "unknown error";

    if (errnum < sys_nerr)
        return sys_errlist[errnum];
    return errtext;
}

#endif /* HOMEGROWN_STRERROR */

#ifdef HOMEGROWN_SNPRINTF

/* this was actually written a couple of years ago for a kernel I'm writing */

/* should be long enough to accomodate any "printf" number */
#define BUFSZ 81

#define OPT_HASH  1
#define OPT_ZERO  2
#define OPT_NEG   4
#define OPT_PLUS  8
#define OPT_SPACE 16

static void format_number(char *buffer, int orig_len, unsigned int num, char fmt,
                          int modifier, int orig_width, int precision,
                          int options) 
{
    unsigned base;
    char lowernibs[] = "0123456789abcdef";
    char uppernibs[] = "0123456789ABCDEF";
    int is_signed, i = 0;
    char *nibs;
    int len = orig_len;
    int inum = (int) num;
    char *s = buffer;
    char *p;
    char c;
    char tmp[BUFSZ];
    int width = orig_width;

/* we build up a backwards string in tmp[] from right to left
 * and then copy it into buffer[] and reverse it
 * this gets pretty ugly
 */

    /* process the options et al. */
    nibs = lowernibs;

    switch (fmt) {
        case 'o':
            base = 8;
            is_signed = 0;
            break;
        case 'X':
            nibs = uppernibs;
            /* fall through */
        case 'x':
            base = 16;
            is_signed = 0;
            break;
        case 'u':
            is_signed = 0;
            base = 10;
            break;
        default:
            base = 10;
            is_signed = 1;
    }

    /* per ANSI C3.159-1989 ("C89") */
    if (precision >= 0) options &= ~OPT_ZERO;
    if (options & OPT_NEG) options &= ~OPT_ZERO;

    if (is_signed && inum < 0) num = -inum;

    /* generate the actual number backwards */
    for (;;) {
        if (len-- <= 0) goto out;
        c = nibs[num % base];
        num /= base;
        tmp[i++] = c;
        precision--;
        if (num == 0) break;
    }
    while (precision-- > 0) {
        if (--len <= 0) goto out;
        tmp[i++] = '0';
    }

    /* the following have to go in this order */

    /* if padding, the sign space has to be accounted for
     * before it is actually added */
    if (is_signed && ((options & (OPT_PLUS | OPT_SPACE)) || inum < 0))
        orig_width--;

    /* if padding with zeros, that goes before the '#' option, but
     * the space the '#' option take up must be accounted for */
    if (options & OPT_ZERO) {
        if ((options & OPT_HASH) && (fmt == 'X' || fmt == 'x'))
            orig_width -= 2;
        if ((options & OPT_HASH) && (fmt == 'o'))
            orig_width--;
    }

    /* pad with zeros */
    width = orig_width - i;
    if (options & OPT_ZERO)
        while (width-- > 0) {
            if (len-- <= 0) goto out;
            if (options & OPT_NEG)
                *s++ = '0';
            else
                tmp[i++] = '0';
        }

    /* add in the '#' option */
    if (options & OPT_HASH) {
        if (fmt == 'X' || fmt == 'x') {
            if ( (len -= 2) <= 0) goto out;
            tmp[i++] = (fmt == 'x' ? 'x' : 'X');
            tmp[i++] = '0';
        } else if (fmt == 'o') {
            if (--len <= 0) goto out;
            tmp[i++] = '0';
        }
    }
        
    /* pad with spaces */
    width = orig_width - i;
    while (width-- > 0) {
        if (len-- <= 0) goto out;
        if (options & OPT_NEG)
            *s++ = ' ';
        else
            tmp[i++] = ' ';
    }

    /* add in sign stuff */
    if (is_signed && (options & OPT_PLUS) && inum > 0)
        if (len-- <= 0) goto out;
        else tmp[i++] = '+';
    if (is_signed && (options & OPT_SPACE) && inum > 0)
        if (len-- <= 0) goto out;
        else tmp[i++] = ' ';
    if (is_signed && inum < 0)
        if (len-- <= 0) goto out;
        else tmp[i++] = '-';
    
    /* done -- reverse and copy it back out */
    while (i) *s++ = tmp[--i];

    *s = '\0';

 out:
    buffer[orig_len] = '\0';
    return;
}

int vsnprintf(char *dst, int orig_len, const char *fmt, va_list args)
{
    int options, width, precision, modifier, i;
    unsigned int num;
    int inum;
    const char *s;
    int done;
    char buffer[BUFSZ];
    int len = orig_len;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            if (len-- > 0) *dst++ = *fmt;
            continue;
        }

        /* process "options" */
        options = done = 0;
        while (!done) {
            fmt++;
            switch (*fmt) {
                case '#':
                    options |= OPT_HASH;
                    break;
                case ' ':
                    options |= OPT_SPACE;
                    break;
                case '0':
                    options |= OPT_ZERO;
                    break;
                case '-':
                    options |= OPT_NEG;
                    break;
                case '+':
                    options |= OPT_PLUS;
                    break;
                default:
                    done = 1;
            }
        }

        if (options & OPT_NEG) options &= ~OPT_ZERO;

        /* process "width" */
        width = -1;
        while (isdigit(*fmt)) {
            if (width == -1) width = 0;
            width *= 10;
            width += (*fmt) - '0';
            fmt++;
        }

        /* process "precision" */
        precision = -1;
        if (*fmt == '.') {
            precision = 0;
            fmt++;
            options &= ~OPT_ZERO;
            while (isdigit(*fmt)) {
                precision *= 10;
                precision += (*fmt) - '0';
                fmt++;
            }
        }

        /* process "modifiers" */
        modifier = 0;
        if (*fmt == 'b') {
            modifier = 'b';
            fmt++;
        } else if (*fmt == 'h') {
            modifier = 'h';
            fmt++;
        } else if (*fmt == 'l') {
            modifier = 'l';
            fmt++;
        }

        /* process the "format" and perhaps output something */
        switch (*fmt) {
            case '%':
                /* things like "%-0 #99%" are perfectly admissible */
                if (len-- > 0) *dst++ = '%';
                break;
            case 'd':
                if (modifier == 'b')
                    inum = (i32) va_arg(args, char);
                else if (modifier == 'h')
                    inum = (i32) va_arg(args, short);
                else
                    inum = (i32) va_arg(args, int);
                format_number(buffer, BUFSZ, inum, *fmt,
                              modifier, width, precision, options);
                s = buffer;
                while (*s) {
                    if (len-- > 0) *dst++ = *s;
                    s++;
                }
                break;
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                if (modifier == 'b')
                    num = (u32) va_arg(args, unsigned char);
                else if (modifier == 'h')
                    num = (u32) va_arg(args, unsigned short);
                else
                    num = (u32) va_arg(args, unsigned int);
                format_number(buffer, BUFSZ, num, *fmt,
                              modifier, width, precision, options);
                s = buffer;
                while (*s) {
                    if (len-- > 0) *dst++ = *s;
                    s++;
                }
                break;
            case 'c':
                if (modifier == 'b')
                    num = (unsigned int) va_arg(args, unsigned char);
                else if (modifier == 'h')
                    num = (unsigned int) va_arg(args, unsigned short);
                else
                    num = (unsigned int) va_arg(args, unsigned int);
                if (len-- > 0) *dst++ = (char) num;
                break;
            case 's':
                s = va_arg(args, char *);
                if (!s) s = "(NULL)";
                i = strlen(s);
                if (precision >= 0) i = MIN(i, precision);
                
                if (!(options & OPT_NEG)) {
                    while (i < width--)
                        if (len-- > 0) *dst++ = ' ';
                }
                if (width < 0) {
                    while (*s) {
                        if (len-- > 0) *dst++ = *s;
                        s++;
                    }
                    break;
                } else {
                    done = i;
                    while (done--)
                        if (len-- > 0) *dst++ = *s++;
                }
                while (i++ < width)
                    if (len-- > 0) *dst++ = ' ';
                break;
            case 'p':
                if (modifier == 'b')
                    my_snprintf(buffer, sizeof(buffer),
                                "0x%02X", va_arg(args, unsigned char));
                else if (modifier == 'h')
                    my_snprintf(buffer, sizeof(buffer),
                                "0x%04X", va_arg(args, unsigned short));
                else
                    my_snprintf(buffer, sizeof(buffer),
                                "0x%08X", va_arg(args, unsigned int));
                s = buffer;
                while (*s) {
                    if (len-- > 0) *dst++ = *s;
                    s++;
                }
                break;
        }
    }

    if (len > 0) *dst = '\0';
    else *(dst-1) = '\0';
    if (len <= 0) return orig_len + (- len);
    else return orig_len - len;
}

int snprintf(char *dst, int len, const char *fmt, ...)
{
    va_list args;
    int i;

    va_start(args, fmt);
    i = vsnprintf(dst, len, fmt, args);
    va_end(args);
    return i;
}

#endif /* HOMEGROWN_SNPRINTF */
