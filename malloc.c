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

#include "config.h"

#include <stdio.h>

#include "ahwm.h"
#include "malloc.h"

#ifdef DEBUG

static FILE *malloc_file = NULL;

void malloc_openfile()
{
    if (malloc_file == NULL) {
        malloc_file = fopen("./malloc-out", "w");
        if (malloc_file == NULL) {
            perror("AHWM: fopen \"./malloc-out\" for writing");
            exit(1);
        }
        /* set line buffered */
        setvbuf(malloc_file, NULL, _IOLBF, 0);
    }
}

void *my_malloc(size_t size, int line, char *file)
{
    void *retval;
    
    malloc_openfile();
    retval = malloc(size);
    /* "%p" format is not portable, but DEBUG builds are meant
     * only for me */
    fprintf(malloc_file, "MALLOC %d FROM %s:%d RETURNS %p\n",
            size, file, line, retval);
    return retval;
}

void *my_realloc(void *ptr, size_t size, int line, char *file)
{
    void *retval;

    malloc_openfile();
    retval = realloc(ptr, size);
    fprintf(malloc_file, "REALLOC %p , %d FROM %s:%d RETURNS %p\n",
            ptr, size, file, line, retval);
    return retval;
}

char *my_strdup(char *ptr, int line, char *file)
{
    char *retval;

    malloc_openfile();
    retval = strdup(ptr);
    fprintf(malloc_file, "STRDUP '%s' FROM %s:%d RETURNS %p\n",
            ptr, file, line, retval);
    return retval;
}

void my_free(void *ptr, int line, char *file)
{
    malloc_openfile();
    fprintf(malloc_file, "FREE %p FROM %s:%d\n",
            ptr, file, line);
    free(ptr);
}

#endif /* DEBUG */
