/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include "xwm.h"

/* height of the titlebar */
#define TITLE_HEIGHT 15

/*
 * we store the data associated with each window using Xlib's XContext
 * mechanism (which has nothing to do with X itself, it's just a hash
 * mechanism built into Xlib as far as I can tell).  These are
 * initialized in xwm.c and defined in client.c.
 * window_context associates clients with their main windows
 * frame_context associates clients with their frame windows
 */

extern XContext window_context;
extern XContext frame_context;

/* we only care about whether a window is mapped or not */

typedef enum _window_states_t { MAPPED, UNMAPPED } window_states_t;

/*
 * this is the information we store with each top-level window
 */

typedef struct _client_t {
    Window window;              /* the actual window */
    Window frame;               /* contains titlebar, parent of above */
    Window transient_for;       /* FIXME */
    XWMHints *xwmh;             /* Hints or NULL */
    char *name;                 /* FIXME */
    int x;                      /* position */
    int y;                      /* position */
    int width;                  /* size */
    int height;                 /* size */
    int workspace;              /* client's workspace  */
    window_states_t state;      /* mapped or not */

    /* clients are managed as doubly linked lists in focus.c: */
    struct _client_t *next;
    struct _client_t *prev;
} client_t;

/*
 * Create and store a newly-allocated client_t structure for a given
 * window.  Returns NULL on error or if we shouldn't be touching this
 * window in any way.  This will also do a number of miscellaneous X
 * input-related things with the window.
 */

client_t *client_create(Window);

/*
 * Find the client structure for a given window.
 * The window is either the client window you passed to client_create
 * or the frame which that function creates.
 * Returns NULL on error.
 */

client_t *client_find(Window);

/*
 * Deallocate and forget about a client structure.
 */

void client_delete(client_t *);

/*
 * print out some information about a client
 */

void client_print(char *, client_t *);

#endif /* CLIENT_H */
