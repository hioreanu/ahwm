/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * Had a free-something-twice bug, needed this
 */

#ifndef MALLOC_H
#define MALLOC_H

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "xwm.h"

#ifdef DEBUG

#define Malloc(size) my_malloc(size, __LINE__, __FILE__)
#define Free(ptr) my_free(ptr, __LINE__, __FILE__)
#define Realloc(ptr,size) my_realloc(ptr, size, __LINE__, __FILE__)
#define Strdup(ptr) my_strdup(ptr, __LINE__, __FILE__)

void *my_malloc(size_t, int, char *);
void my_free(void *, int, char *);
void *my_realloc(void *, size_t, int, char *);
char *my_strdup(char *, int, char *);

#else

#define Malloc(size) malloc(size)
#define Free(ptr) free(ptr)
#define Realloc(ptr,size) realloc(ptr, size)
#define Strdup(ptr) strdup(ptr)

#endif /* DEBUG */

#endif /* MALLOC_H */
