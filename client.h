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

/*
 * this is the information we store with each top-level window
 */

typedef struct _client_t {
    Window window;              /* the actual window */
    Window frame;               /* contains titlebar, parent of above */
    Window transient_for;       /* FIXME */
    Window next_transient;      /* FIXME */
    Window group_leader;        /* FIXME */
    XWMHints *xwmh;             /* Hints or NULL (ICCCM, 4.1.2.4) */
    XSizeHints *xsh;            /* Size hints or NULL (ICCCM, 4.1.2.3) */
    int x;                      /* frame's actual position when mapped */
    int y;                      /* frame's actual position when mapped */
    int width;                  /* frame's actual size when mapped */
    int height;                 /* frame's actual size when mapped */
    int workspace;              /* client's workspace  */
    int window_event_mask;      /* event mask of client->window */
    int frame_event_mask;       /* event mask of client->frame */
    char *name;
    /* window's name (ICCCM, 4.1.2.1) */
    /* will not be NULL; use free() */
    char *instance;             /* window's instance (ICCCM, 4.1.2.5) */
    char *class;                /* window's class (ICCCM, 4.1.2.5) */
    /* both of the above may be NULL; use XFree() on them */

    int state;
    /* The state is 'Withdrawn' when the window is created but is
     * not yet mapped and when the window has been unmapped but
     * not yet destroyed.
     * The state is 'Iconic' when the window has been unmapped
     * because it is not in the current workspace.  We do
     * absolutely nothing with icons, but this is how other
     * window managers deal with workspaces, so we shouldn't
     * confuse the client.
     * The state is 'NormalState' whenever the window is mapped.
     */

    /* clients are managed as doubly linked lists in focus.c: */
    struct _client_t *next;
    struct _client_t *prev;
} client_t;
/* FIXME:  prolly need to keep a list of client's transient clients */

/*
 * Create and store a newly-allocated client_t structure for a given
 * window.  Returns NULL on error or if we shouldn't be touching this
 * window in any way.  This will also do a number of miscellaneous X
 * -related things with the window, such as reparenting it and setting
 * the input mask.
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
 * Figure out the name of a client and set it to a newly-malloced
 * string.  The name is found using the WM_NAME property, and this
 * function will always set the 'name' member of the client
 * argument to a newly-malloced string.  This will NOT free
 * the previous 'name' member.
 */

void client_set_name(client_t *);

/*
 * Get the client's 'class' and 'instance' using the WM_CLASS
 * property and set the corresponding members in the client
 * structure to newly-allocated strings which contain this
 * information.  The 'class' and 'instance' members may be set
 * to NULL (unlike client_set_name()) if the application does
 * not supply this information; this function does NOT free
 * the member data at any time.  Use 'XFree()' to free the data,
 * NOT 'free()'
 */

void client_set_instance_class(client_t *);

/*
 * Get a client's XWMHints and set the xwmh member of the client
 * structure.  The 'xwmh' member may be NULL after this.  Use XFree()
 * to release the memory for the member; this function will NOT
 * free any members.
 */

void client_set_xwmh(client_t *);

/*
 * Same as above except for the XWMSizeHints structure
 */

void client_set_xsh(client_t *);

/*
 * Ensure that a client's WM_STATE property reflects what we think it
 * should be (the 'state' member, either NormalState, Iconic,
 * Withdrawn, etc.).
 */

void client_inform_state(client_t *);

/*
 * reparent a client
 * FIXME: more
 */

void client_reparent(client_t *);

typedef struct _position_size {
    int x, y, width, height;
} position_size;

/*
 * Sets the position_size argument to the position and size that a
 * frame should take around this client in strict accordance to ICCCM,
 * 4.1.2.3; FIXME: more
 */

void client_frame_position(client_t *, position_size *);

void client_get_position_size_hints(client_t *client, position_size *ps);

/*
 * print out some debugging information about a client
 */

void client_print(char *, client_t *);

#endif /* CLIENT_H */
