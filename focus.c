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
 * OK, the stuff in here gets a little bit confusing because we have
 * to do things *just so* or client applications will misbehave.  Each
 * function has a distinct purpose.  There are a number of separate
 * things we can do in any given function:
 * 
 * 1.  update some pointers, like focus_current or focus_stacks[]
 * 2.  update the focus stack (focus order)
 * 3.  update clients' titlebars to show input focus
 * 4.  call XSetInputFocus()
 * 5.  raise the active client
 */

/* 
 * invariant:
 * 
 * focus_current == focus_stacks[workspace_current - 1]
 * 
 * This should hold whenever we enter or leave the window manager.  In
 * a few fringe cases, focus_current is not the real input focus (ie,
 * what XGetInputFocus() would return), but represents what what the
 * real input focus will be once some condition has been satisfied.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <stdio.h>
#include "focus.h"
#include "client.h"
#include "workspace.h"
#include "debug.h"
#include "event.h"
#include "ewmh.h"
#include "keyboard-mouse.h"
#include "stacking.h"
#include "malloc.h"
#include "paint.h"

typedef struct _focus_node {
    struct _focus_node *next;
    struct _focus_node *prev;
    client_t *client;
} focus_node;

client_t *focus_current = NULL;

static focus_node **focus_stacks;

static XContext *focus_contexts;

static Bool in_alt_tab = False; /* see focus_alt_tab, focus_ensure */

static Window revert_to = None;

static focus_node *find_node(client_t *);
static void focus_change_current(client_t *, Time, Bool);
static void focus_set_internal(focus_node *, Time, Bool);
static void permute(focus_node *, focus_node *);
static focus_node *get_prev(focus_node *);
static focus_node *get_next(focus_node *);
static void focus_add_internal(focus_node *, int ws, Time timestamp);
static void focus_remove_internal(focus_node *, int ws, Time timestamp);

void focus_init()
{
    int i;
    XSetWindowAttributes xswa;

    focus_contexts = malloc(nworkspaces * sizeof(XContext));
    focus_stacks = malloc(nworkspaces * sizeof(focus_node *));
    if (focus_contexts == NULL || focus_stacks == NULL) {
        perror("XWM: focus_init: malloc");
        fprintf(stderr, "XWM: this is a fatal error, quitting.\n");
        exit(1);
    }
    for (i = 0; i < nworkspaces; i++) {
        focus_contexts[i] = XUniqueContext();
        focus_stacks[i] = NULL;
    }
    xswa.override_redirect = True;
    revert_to = XCreateWindow(dpy, root_window, 0, 0, 1, 1, 0, 0,
                              InputOnly, DefaultVisual(dpy, scr),
                              CWOverrideRedirect, &xswa);
    XMapWindow(dpy, revert_to);
    XSync(dpy, False);
    keyboard_grab_keys(revert_to);
}

static focus_node *find_node(client_t *client)
{
    focus_node *node;

    if (client == NULL) return NULL;
    if (XFindContext(dpy, client->window,
                     focus_contexts[client->workspace - 1],
                     (void *)&node) != 0) {
        return NULL;
    }
    return node;
}

Bool focus_forall(forall_fn fn, void *v)
{
    focus_node *node, *tmp;

    tmp = node = focus_stacks[workspace_current - 1];
    if (node == NULL) return True;
    do {
        if (fn(node->client, v) == False) return False;
        node = node->next;
    } while (node != tmp);
    return True;
}

void focus_workspace_changed(Time timestamp)
{
    focus_node *n;
    
    n = focus_stacks[workspace_current - 1];
    if (n == NULL)
        focus_current = NULL;
    else
        focus_current = n->client;
    focus_ensure(timestamp);
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_add(client_t *client, Time timestamp)
{
    int i;
    focus_node *node;

    if ( (node = find_node(client)) != NULL)
        focus_remove(client, CurrentTime);
    if (client->omnipresent) {
        node = Malloc(nworkspaces * sizeof(focus_node));
        if (node == NULL) {
            fprintf(stderr, "XWM: out of memory while focusing client\n");
            return;
        }
        debug(("\tOmnipresent node = %#lx\n", node));
        for (i = 0; i < nworkspaces; i++) {
            node[i].client = client;
            focus_add_internal(&node[i], i + 1, timestamp);
        }
    } else {
        node = Malloc(sizeof(focus_node));
        if (node == NULL) {
            fprintf(stderr, "XWM: out of memory while focusing client\n");
            return;
        }
        node->client = client;
        focus_add_internal(node, client->workspace, timestamp);
    }
}

static void focus_add_internal(focus_node *node, int ws, Time timestamp)
{
    focus_node *old;

    old = focus_stacks[ws - 1];
    if (old == NULL) {
        node->next = node;
        node->prev = node;
    } else {
        node->next = old;
        node->prev = old->prev;
        old->prev->next = node;
        old->prev = node;
    }
    if (node->client->focus_policy == DontFocus) {
        if (old != NULL)
            permute(node, old);
        if (node->next == node)
            focus_stacks[ws - 1] = node;
    } else {
        debug(("\tSetting focus stack of workspace %d to %#lx ('%.10s')\n",
               ws, node->client->window, node->client->name));
        focus_stacks[ws - 1] = node;
        if (ws == workspace_current) {
            focus_change_current(node->client, timestamp, True);
        }
    }
    if (XSaveContext(dpy, node->client->window,
                     focus_contexts[ws - 1], (void *)node) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save window\n");
    }
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_remove(client_t *client, Time timestamp)
{
    int i;
    focus_node *node = NULL;

    if (client->omnipresent) {
        for (i = nworkspaces - 1; i >= 0; i--) {
            if (XFindContext(dpy, client->window, focus_contexts[i],
                             (void *)&node) != 0) {
                node = NULL;
                continue;
            }
            debug(("\tOmnipresent, i = %d, node = 0x%lx\n\n", i, node));
            focus_remove_internal(node, i + 1, timestamp);
        }
    } else {
        node = find_node(client);
        if (node == NULL) return;
        focus_remove_internal(node, client->workspace, timestamp);
    }
    if (node != NULL) Free(node);
}

static void focus_remove_internal(focus_node *node, int ws, Time timestamp)
{
    focus_node *new;

    new = get_next(node);
    
    /* remove from list */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    /* if was focused for workspace, update workspace pointer */
    if (focus_stacks[ws - 1] == node) {
        debug(("\tSetting focus stack of workspace "
               "%d to %#lx ('%.10s')\n",
               ws, new->client->window,
               new->client->name));
        focus_stacks[ws - 1] = new;
    }
    /* if only client left on workspace, set to NULL */
    if (node->next == node) {
        debug(("\tSetting focus stack of workspace %d to null\n", ws));
        focus_stacks[ws - 1] = NULL;
        node->next = NULL;
        node->prev = NULL;
    }
    /* if removed focused window, refocus now */
    if (node->client == focus_current && ws == workspace_current) {
        if (focus_stacks[ws - 1] == NULL) {
            focus_change_current(NULL, timestamp, True);
        } else {
            focus_change_current(focus_stacks[ws - 1]->client,
                                 timestamp, True);
        }
    }
    XDeleteContext(dpy, node->client->window,
                   focus_contexts[ws - 1]);
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_set(client_t *client, Time timestamp)
{
    focus_node *node, *old;

    node = find_node(client);
    old = find_node(focus_current);
    focus_set_internal(node, timestamp, True);
    if (old != NULL && node != NULL)
        permute(old, node);
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus iff (call_focus_ensure == True)
 */

static void focus_set_internal(focus_node *node, Time timestamp,
                               Bool call_focus_ensure)
{
    focus_node *p;

    if (node != NULL && node->client == focus_current) return;

    if (node == NULL) {
        XSetInputFocus(dpy, revert_to, RevertToPointerRoot, CurrentTime);
        return;
    }
    
    p = focus_stacks[node->client->workspace - 1];
    if (p == NULL) {
        fprintf(stderr, "XWM: current focus list is empty, shouldn't be\n");
        return;
    }
    do {
        if (p == node) {
            debug(("\tSetting focus stack of workspace %d "
                   "to %#lx ('%.10s')\n",
                   node->client->workspace, node->client->window,
                   node->client->name));
            focus_stacks[node->client->workspace - 1] = node;
            if (node->client->workspace == workspace_current) {
                focus_change_current(node->client, timestamp,
                                     call_focus_ensure);
            }
            return;
        }
        p = p->next;
    } while (p != focus_stacks[node->client->workspace - 1]);
    fprintf(stderr, "XWM: client not found on focus list, shouldn't happen\n");
}

/*
 * This will:
 * call XSetInputFocus
 * raise active client
 * 
 * This is the only function in the module that will call
 * XSetInputFocus; all other functions call this function.
 */

void focus_ensure(Time timestamp)
{
    if (focus_current == NULL || focus_current->focus_policy == DontFocus) {
        XSetInputFocus(dpy, revert_to, RevertToPointerRoot, CurrentTime);
        return;
    }

    debug(("\tCalling XSetInputFocus(%#lx) ('%.10s')\n",
           (unsigned int)focus_current->window, focus_current->name));

    ewmh_active_window_update();
    
    if (in_alt_tab) {
        stacking_raise(focus_current);
        return;
    }
    
    /* see ICCCM 4.1.7 */
    if (focus_current->xwmh != NULL &&
        focus_current->xwmh->flags & InputHint &&
        focus_current->xwmh->input == False) {
        /* FIXME:  we shouldn't call XSetInputFocus here */
        debug(("\tInput hint is False\n"));
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
    } else {
        debug(("\tInput hint is True\n"));
        XSetInputFocus(dpy, focus_current->window,
                       RevertToPointerRoot, timestamp);
    }
    if (focus_current->protocols & PROTO_TAKE_FOCUS) {
        debug(("\tUses TAKE_FOCUS protocol\n"));
        client_sendmessage(focus_current, WM_TAKE_FOCUS,
                           timestamp, 0, 0, 0);
    } else {
        debug(("\tDoesn't use TAKE_FOCUS protocol\n"));
    }

    stacking_raise(focus_current);
}

/*
 * This will:
 * update pointers
 * update titlebars
 * call XSetInputFocus iff (call_focus_ensure == True)
 */

static void focus_change_current(client_t *new, Time timestamp,
                                 Bool call_focus_ensure)
{
    client_t *old;

    old = focus_current;
    focus_current = new;
    paint_titlebar(old);
    paint_titlebar(new);
    if (new != NULL && new->focus_policy == DontFocus) return;
    if (call_focus_ensure) focus_ensure(timestamp);
    if (old != new) {
        if (old != NULL && old->focus_policy == ClickToFocus) {
            debug(("\tGrabbing Button 1 of 0x%08x\n", old));
            XGrabButton(dpy, Button1, 0, old->frame,
                        True, ButtonPressMask, GrabModeSync,
                        GrabModeAsync, None, None);
            keyboard_grab_keys(old->frame); /* FIXME */
            mouse_grab_buttons(old);
        }
        if (new != NULL && new->focus_policy == ClickToFocus) {
            XUngrabButton(dpy, Button1, 0, new->frame);
            mouse_grab_buttons(new);
        }
    }
    XFlush(dpy);
}

/* FIXME:  these grabs/ungrabs should go in mouse_grab_buttons() */

void focus_policy_to_click(client_t *client)
{
    if (client != focus_current) {
        debug(("\tGrabbing Button 1 of 0x%08x\n", client));
        XGrabButton(dpy, Button1, 0, client->frame,
                    True, ButtonPressMask, GrabModeSync,
                    GrabModeAsync, None, None);
        keyboard_grab_keys(client->frame); /* FIXME */
        mouse_grab_buttons(client); /* FIXME */
    }
}

void focus_policy_from_click(client_t *client)
{
    XUngrabButton(dpy, Button1, 0, client->frame);
    mouse_grab_buttons(client); /* FIXME */
}

/*
 * After extensive experimentation, I determined that this is the best
 * way for this function to behave.
 * 
 * We want clients to receive a Focus{In,Out} event when the focus is
 * transferred, and will only transfer the focus when the alt-tab
 * action is completed (when user lets go of alt).  Some clients
 * (notably all applications which use the QT toolkit) will only think
 * that they've received the input focus when they receive a FocusIn,
 * NotfyNormal event.  The problem is that if we transfer the input
 * focus while we have the keyboard grabbed, this will generate
 * FocusIn, NotfyGrab events, which some applications completely
 * ignore (whether or not this is the correct behaviour, I don't care,
 * that's how it is), and then any XSetInputFocus call to the current
 * focus window made after ungrabbing the keyboard will be a no-op;
 * thus QT applications will not believe they have the input focus
 * (and the way QT is set up, QT applications won't take keyboard
 * input in this state).  Needless to say, this is quite annoying.
 * The solution is to delay all calls to XSetInputFocus until we've
 * ungrabbed the keyboard.
 * 
 * This will behave correctly if clients are added or removed while
 * this function is active.  That's the purpose of the in_alt_tab
 * boolean - it ensures we don't call XSetInputFocus while this
 * function is active.  This will return immediately if all the
 * clients disappear from under our nose.
 * 
 * This is still sub-optimal as some clients may receive FocusIn,
 * NotifyPointer events while we are in this function and will
 * incorrectly think they have the input focus and update themselves
 * accordingly.  Xterm does this.  This only lasts until the function
 * exits, but is a bit annoying.  There is no way to fix this other
 * than fixing the bug in xterm (other window managers have the same
 * behaviour).
 */

void focus_alt_tab(XEvent *xevent, arglist *ignored)
{
    focus_node *orig_focus;
    focus_node *node;
    unsigned int action_keycode;
    KeyCode keycode_Alt_L, keycode_Alt_R;
    enum { CONTINUE, DONE, REPLAY_KEYBOARD, QUIT } state;

    if (focus_current == NULL) {
        debug(("\tNo clients in alt-tab, returning\n"));
        return;
    }
    debug(("\tEntering alt-tab\n"));
    in_alt_tab = True;
    orig_focus = focus_stacks[workspace_current - 1];
    debug(("\torig_focus = '%s'\n",
           orig_focus ? orig_focus->client->name : "NULL"));
    action_keycode = xevent->xkey.keycode;
    keycode_Alt_L = XKeysymToKeycode(dpy, XK_Alt_L);
    keycode_Alt_R = XKeysymToKeycode(dpy, XK_Alt_R);

    XGrabKeyboard(dpy, root_window, True, GrabModeAsync,
                  GrabModeAsync, CurrentTime);
    state = CONTINUE;
    while (state == CONTINUE) {
        switch (xevent->type) {
            case KeyPress:
                if (xevent->xkey.keycode == action_keycode) {
                    node = focus_stacks[workspace_current - 1];
                    if (xevent->xkey.state & ShiftMask) {
                        node = get_prev(node);
                    } else {
                        node = get_next(node);
                    }
                    /* FIXME:  aren't we checking this twice? */
                    if (!(node->client->cycle_behaviour == SkipCycle
                          || node->client->focus_policy == DontFocus)) {
                        focus_set_internal(node, event_timestamp, False);
                        if (node->client->cycle_behaviour == RaiseImmediately) {
                            focus_ensure(CurrentTime);
                        }
                    }
                } else {
                    state = REPLAY_KEYBOARD;
                }
                break;
            case KeyRelease:
                if (xevent->xkey.keycode == keycode_Alt_L
                    || xevent->xkey.keycode == keycode_Alt_R) {
                    /* user let go of all modifiers */
                    state = DONE;
                }
                break;
            default:
                event_dispatch(xevent);
                if (focus_current == NULL) {
                    debug(("\tAll clients gone, returning from alt-tab\n"));
                    state = QUIT;
                }
        }
        if (state == CONTINUE) event_get(ConnectionNumber(dpy), xevent);
    }

    node = focus_stacks[workspace_current - 1];
    if (state != QUIT && node != NULL && orig_focus != NULL) {
        permute(orig_focus, node);
    }
    
    XUngrabKeyboard(dpy, CurrentTime);
    /* we want to make sure server knows we've
     * ungrabbed keyboard before calling
     * XSetInputFocus: */
    XSync(dpy, False);
    in_alt_tab = False;
    focus_ensure(CurrentTime);
                    
    if (state == REPLAY_KEYBOARD) {
        if (!keyboard_handle_event(&xevent->xkey))
            keyboard_replay(&xevent->xkey);
    }
    
    debug(("\tfocus_current = '%.10s'\n",
           focus_current ? focus_current->name : "NULL"));
    debug(("\tLeaving alt-tab\n"));
}

static focus_node *get_prev(focus_node *node)
{
    focus_node *p;

    for (p = node->prev; p != node; p = p->prev) {
        if (!(p->client->cycle_behaviour == SkipCycle
              || p->client->focus_policy == DontFocus))
            return p;
    }
    return node->prev;
}

static focus_node *get_next(focus_node *node)
{
    focus_node *p;

    for (p = node->next; p != node; p = p->next) {
        if (!(p->client->cycle_behaviour == SkipCycle
              || p->client->focus_policy == DontFocus))
            return p;
    }
    return node->next;
}

/*
 * This will swap two elements in the focus stack, moving one to the
 * top of the stack.
 * 
 * ring looks like this:
 * A-B-...-C-D-E-...-F-A
 * and we want it to look like this:
 * D-A-B-...-C-E-...-F-D
 * 
 * special cases:
 * A-A-...  ->  A-A-...  (no change)
 * A-B-A-...  ->  B-A-B-... (no change, just update focus_current)
 * A-D-E-...-F-A  ->  D-A-E-...-F-D  (just swap A & D)
 * A-B-...-C-D-A  ->  D-A-B-...-C-D  (no change, just update focus_current)
 * We actually don't change B in any way, so it's left out.
 */

static void permute(focus_node *A, focus_node *D)
{
    focus_node *C, *E, *F;

    /* if have only one or two elements, or not moving, done */
    if (A == D || (A->next == D && D->next == A))
        return;

    F = A->prev;
    C = D->prev;
    E = D->next;

    if (A->next == D) {
        /* no B or C, just swap A & D */
        A->prev = D;
        D->next = A;
        
        A->next = E;
        E->prev = A;
        
        F->next = D;
        D->prev = F;
        
    } else if (A->prev == D) {
        /* no E or F, no need for change */
    } else {
        /* B and D are separated by at least one node on each side */
        C->next = E;
        E->prev = C;

        F->next = D;
        D->prev = F;

        D->next = A;
        A->prev = D;
    }
}
