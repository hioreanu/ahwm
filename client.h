/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef CLIENT_H
#define CLIENT_H

#include "xwm.h"

/*
 * we store the data associated with each window using Xlib's XContext
 * mechanism (which has nothing to do with X itself, it's just a hash
 * mechanism built into Xlib as far as I can tell).  This is
 * initialized in xwm.c and defined in client.c.
 */

extern XContext window_context;

/* we only care about whether a window is mapped or not */

typedef enum _window_states_t { MAPPED, UNMAPPED } window_states_t;

/*
 * this is the information we store with each top-level window
 */

typedef struct _client_t {
    Window window;              /* the actual window */
    Window parent;              /* FIXME */
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
 */

client_t *client_find(Window);

/*
 * Deallocate and forget about a client structure.
 */

void client_delete(client_t *);

#endif /* CLIENT_H */
