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
#ifndef CLIENT_H
#define CLIENT_H

#include "config.h"

#include "xwm.h"

/* height of the titlebar, should probably be configurable */
#define TITLE_HEIGHT 15

/*
 * this is the information we store with each top-level window EXCEPT
 * for those windows which have override_redirect set (the ONLY thing
 * we do with override_redirect windows is an XGrabKeys since we want
 * our global keybindings to work globally - we don't event listen for
 * events on override_redirect windows and we will never give them the
 * focus (if they want the focus, they can take it themselves).  If
 * you haven't played around with this, the most common kind of
 * top-level override_redirect window would be a popup menu (it can't
 * be a child of a top-level window because it shouldn't be clipped).
 * 
 * Regular top-level windows are reparented to a frame window we
 * create which may or may not have an area for a titlebar.  We
 * reparent windows even if they don't have a titlebar since a lot of
 * X apps ASSUME that they will be reparented by a windowmanager upon
 * creation and it's a bit easier to listen for some events on our
 * frame rather than their app window.  We also reset the client's
 * border width to zero since I hate borders on windows.  The old
 * border width must be saved, however.
 */

typedef struct _client_t {
    Window window;              /* their application window */
    Window frame;               /* contains titlebar, parent of above */
    Window titlebar;            /* our titlebar, subwindow of frame */
    Window transient_for;       /* WM_TRANSIENT_FOR hint */
    struct _client_t *group_leader; /* group leader, ICCCM 4.1.11 */
    XWMHints *xwmh;             /* Hints or NULL (ICCCM, 4.1.2.4) */
    XSizeHints *xsh;            /* Size hints or NULL (ICCCM, 4.1.2.3) */
    int x;                      /* frame's actual position when mapped */
    int y;                      /* frame's actual position when mapped */
    int width;                  /* frame's actual size when mapped */
    int height;                 /* frame's actual size when mapped */
    int prev_x;                 /* previous position/size for maximization */
    int prev_y;                 /* previous position/size for maximization */
    int prev_width;             /* previous position/size for maximization */
    int prev_height;            /* previous position/size for maximization */
    int orig_border_width;      /* client's requested border width */
    unsigned int workspace;     /* client's workspace, see workspace.h */
    unsigned int protocols;     /* WM_PROTOCOLS, see below (ICCCM, 4.1.2.7) */
    char *name;                 /* window's name (ICCCM, 4.1.2.1) */
    /* will not be NULL; use free() */
    char *instance;             /* window's instance (ICCCM, 4.1.2.5) */
    char *class;                /* window's class (ICCCM, 4.1.2.5) */
    /* both of the above may be NULL; use XFree() on them */

    /* The state is 'Withdrawn' when the window is created but is
     * not yet mapped and when the window has been unmapped but
     * not yet destroyed.
     * The state is never 'Iconic', not even when the window
     * asks to be iconized (which is perfectly OK by ICCCM)
     * The state is 'NormalState' whenever the window is mapped
     * or unmapped because it is not in the current workspace.
     */
    int state;

    /* If some client has this client as the transient_for hint, then
     * this client is a 'leader' (my nomenclature, nothing to do with
     * window groups).  A leader has the 'transients' attribute set to
     * one of its transient windows.  A transient window has the
     * 'next_transient' attribute set the the next transient window.
     * Note that a transient window may also be a leader.  These two
     * are used to represent the tree of transient windows as a binary
     * tree. */

    struct _client_t *transients;
    struct _client_t *next_transient;
    
    /* All clients are managed as regular doubly linked lists in
     * stacking.c; these can be used to iterate over all clients. */
    struct _client_t *next_stacking;
    struct _client_t *prev_stacking;

    /* hacks, see client.c and event.c */
    unsigned int reparented : 1;
    unsigned int ignore_unmapnotify : 1;

    /* user preferences */
    /* FIXME: 2-bit fields not defined in C, test with autoconf */
    enum { ClickToFocus, SloppyFocus, DontFocus } focus_policy : 2;
    enum { Fixed, Smart, Cascade, Mouse } map_policy : 2;
    enum { SkipCycle, RaiseImmediately,
           RaiseOnCycleFinish, DontRaise } cycle_behaviour : 2;
    unsigned int has_titlebar : 1;
    unsigned int is_shaped : 1;
    unsigned int pass_focus_click : 1;
    unsigned int always_on_top : 1;
    unsigned int always_on_bottom : 1;
    unsigned int omnipresent : 1;
    /* unsigned int sticky : 1; */
} client_t;                     /* 116 bytes on ILP-32 machines */

/* the values for client->protocols, can be ORed together */

#define PROTO_NONE          00
#define PROTO_TAKE_FOCUS    01
#define PROTO_SAVE_YOURSELF 02
#define PROTO_DELETE_WINDOW 04

typedef struct _position_size {
    int x, y, width, height;
} position_size;

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
extern XContext title_context;

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
 * The window argument is either the client window you passed to
 * client_create or the frame which that function creates.
 * Returns NULL on error.
 */

client_t *client_find(Window);

/*
 * Deallocate and forget about a client structure.
 */

void client_destroy(client_t *);

/*
 * Figure out the name of a client and set it to a newly-malloced
 * string.  The name is found using the WM_NAME property, and this
 * function will always set the 'name' member of the client
 * argument to a newly-malloced string.  This will NOT free
 * the previous 'name' member.
 */

void client_set_name(client_t *);

/*
 * paint the client's titlebar
 */

void client_paint_titlebar(client_t *);

/*
 * Get the client's 'class' and 'instance' using the WM_CLASS
 * property and set the corresponding members in the client
 * structure to newly-allocated strings which contain this
 * information.  The 'class' and 'instance' members may be set
 * to NULL (unlike client_set_name()) if the application does
 * not supply this information; this function does NOT free
 * the member data at any time.  Use 'XFree()' to free the data,
 * NOT 'free()'.
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
 * Examine the client's WM_PROTOCOLS property and set the appropriate
 * members of client->protocols
 */

void client_set_protocols(client_t *);

/*
 * Examine client's WM_TRANSIENT_FOR property and set client->transient_for
 */

void client_set_transient_for(client_t *);

/*
 * Ensure that a client's WM_STATE property reflects what we think it
 * should be (the 'state' member, either NormalState, Iconic,
 * Withdrawn, etc.).  We set this attribute on the client's
 * application window according to ICCCM 4.1.3.1 and 4.1.4.
 */

void client_inform_state(client_t *);

/*
 * Create a frame window for a client and reparent the client.  If the
 * client has already been reparented, ensure the frame window
 * properly expresses the client's size and position wishes.  Frame is
 * stored in client->frame.
 */

void client_reparent(client_t *client);
void client_unreparent(client_t *client);

void client_add_titlebar(client_t *);
void client_remove_titlebar(client_t *);

/*
 * Sets the position_size argument to the position and size that a
 * frame should take around this client in strict accordance to ICCCM,
 * 4.1.2.3; the position_size is an in-and-out argument.  Pass in the
 * place where the application window wants to be placed and out comes
 * the place where the frame should be placed, taking gravity,
 * etc. into account.
 */

void client_frame_position(client_t *, position_size *);

/*
 * Sets the position_size argument to the position and size
 * that the application asked for and would have been given
 * had it not been reparented.  This is the exact inverse
 * of client_frame_position().  Anything passed in to the
 * position_size argument will be ignored and overwritten
 * since we already have the frame's position in the client
 * structure.
 */

void client_position_noframe(client_t *, position_size *);

/*
 * Set the postion_size argument to the client's desired position and
 * size based upon the client's hints.
 */

void client_get_position_size_hints(client_t *client, position_size *ps);

/*
 * print out some debugging information about a client, prepended by a
 * given string
 */

void _client_print(char *, client_t *);
#ifdef DEBUG
#define client_print _client_print
#else
#define client_print(x,y) /* */
#endif /* DEBUG */

/*
 * Send a ClientMessage to a client window, according the conventions
 * described in ICCCM, 4.2.8.  The DATA[234] members are stuffed into
 * the XClientMessageEvent structure, the timestamp is put in data[1]
 * and the atom is put into data[0].
 */

void client_sendmessage(client_t *client, Atom data0, Time timestamp,
                        long data2, long data3, long data4);

#endif /* CLIENT_H */
