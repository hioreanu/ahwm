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

typedef struct _client_t {
    Window    window;
    Window    parent;
    Window    transient_for;
    Bool      managed;          /* FIXME */
    char     *name;
    int       x;
    int       y;
    int       width;
    int       height;
    int       workspace;
    int       state;
    /* state is one of {WithdrawnState, NormalState, IconicState} */
    /* we ignore this since we don't deal with icons */
    struct _client_t *prevfocus;
} client_t;

client_t *client_create(Window);
client_t *client_find(Window);

void client_release(client_t *);
void client_deactivate(client_t *);
void client_activate(client_t *);
void client_kill(client_t *);
void client_hide(client_t *);

#if 0
void client_move_to_workspace(client_t *, int);
void client_move(client_t *, XButtonEvent *);
void client_resize(client_t *, XButtonEvent *);
void client_move_or_resize(client_t *, XButtonEvent *);
#endif

#endif /* CLIENT_H */
