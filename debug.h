/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "config.h"

/*
 * This is the only way to conditionally define a variable length
 * function in current C implementations (C99 has a feature for
 * variable-length macros, but no one has implemented C99 as of June
 * 2001 and gcc also has a similar non-standard feature).  The idea is
 * that in code with DEBUG turned off, debug statements don't even
 * generate a function call.
 */

#ifndef DEBUG

#define debug(x) /* */

#else

void _debug(char *fmt, ...);
#define debug(x) _debug x

#endif /* DEBUG */
#endif /* DEBUG_H */
