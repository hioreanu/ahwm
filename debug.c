/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

#ifdef DEBUG
# ifndef HAVE_VPRINTF
#  error "XWM needs porting to your system (no vprintf); please contact the author."
# endif
#else
# ifndef HAVE_VPRINTF
void _debug(char *fmt, ...) { }
# else
void _debug(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
/*    printf("\017"); */ /* fixes stupid xterm "alternate-charset" escape */
}
# endif
#endif
