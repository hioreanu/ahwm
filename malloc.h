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

/*
 * Had a free-something-twice bug, needed this
 */

#ifndef MALLOC_H
#define MALLOC_H

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "compat.h"
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
