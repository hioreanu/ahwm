/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "malloc.h"

#ifdef DEBUG

static FILE *malloc_file = NULL;

void malloc_openfile()
{
    if (malloc_file == NULL) {
        malloc_file = fopen("./malloc-out", "w");
        if (malloc_file == NULL) {
            perror("XWM: fopen \"./malloc-out\" for writing");
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
