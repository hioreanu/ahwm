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

#include <X11/Xlib.h>
#include <stdio.h>

#include "client.h"
#include "stacking.h"
#include "malloc.h"
#include "workspace.h"
#include "debug.h"
#include "ewmh.h"

/*
 * We use parallel arrays to keep track of the stacking order.  I
 * tried to do this with linked lists, but I was not happy with the
 * performance - we have to convert to arrays of windows anyway to use
 * XRestackWindows and ewmh requires arrays of windows.  In addition,
 * the list implementation was very ugly and this is more elegant IMHO.
 * 
 * "frames" contains the frame windows, which we use for XRestackWindows()
 * 
 * "clients" contains the client structures, which we need to manage.
 *
 * "windows" contains the client windows, which are needed for EWMH.
 * 
 * "windows" and "frames" always contain two more items than "clients"
 * - we have the desktop window and the hiding window, which may not
 * be clients (and are kept respectively on top or bottom of all
 * clients).  "nused" indicates the size of the "clients" array.
 * 
 * client->stacking is an index into the "clients" array.
 * If client->stacking == -1, client is not in arrays.
 * 
 * An additional problem arises in that XRestackWindows wants windows
 * in top-to-bottom order and EWMH wants windows in bottom-to-top
 * order.  In my way of thinking, XRestackWindows works backwards,
 * and I've been screwed up a number of times by this.  I have this
 * feeling that the whole idea behind the EWMH stacking list stuff
 * is because XRestackWindows works backwards.  Thus, the "frames"
 * array is actually not parallel with "clients" and "windows" but
 * is maintained in backwards order.
 * 
 * invariants:
 * 
 * top-most client is:
 * clients[nused - 1]
 * windows[nused]
 * frames[1]
 * 
 * bottom-most client is:
 * clients[0]
 * windows[1]
 * frames[nused]
 * 
 * clients[i]->window == windows[i + 1]
 * clients[i]->frame == frames[nused - i]
 */

static Window *frames = NULL;
static client_t **clients = NULL;
static Window *windows = NULL;

static int nused = 0;

Window stacking_hiding_window = None;
Window stacking_desktop_window = None;
Window stacking_desktop_frame = None;

static Bool grow();
static int order(client_t *client1, client_t *client2);
static void restack(client_t *client, Bool move_up);
static void commit();
static void raise_tree(client_t *client, client_t *ignore, Bool go_up);

#if 0
static void dump(); /* defined out to get rid of warning */
#endif

/* easy way to maintain invariants */
static void set(client_t *client, int index)
{
    clients[index] = client;
    clients[index]->stacking = index;
    windows[index + 1] = client->window;
    frames[nused - index] = client->frame;
}

void stacking_add(client_t *client)
{
    int i;
    
    if (grow() == False) {
        perror("AHWM: stacking_add: grow");
        client->stacking = -1;
    }

    /* add to backwards "frames" array */
    for (i = 0; i < nused; i++) {
        if (order(clients[i], client) > 0)
            break;
        frames[nused - i + 1] = frames[nused - i];
    }
    frames[nused - i + 1] = client->frame;
    
    /* add to "clients" and "windows" */
    for (i = nused - 1; i >= 0; i--) {
        if (order(clients[i], client) == 0)
            break;
        clients[i + 1] = clients[i];
        clients[i + 1]->stacking = i + 1;
        windows[i + 2] = windows[i + 2];
    }
    clients[i + 1] = client;
    clients[i + 1]->stacking = i + 1;

    nused++;
    commit();
}

/*
 * simply move the client on top of all other clients
 * and decrement the number of elements in the arrays
 */

void stacking_remove(client_t *client)
{
    int i;

    for (i = client->stacking + 1; i < nused; i++) {
        clients[i - 1] = clients[i];
        clients[i - 1]->stacking = i - 1;
        windows[i] = windows[i + 1];
    }
    for (i = nused - client->stacking; i <= nused; i++) {
        frames[i] = frames[i + 1];
    }
    client->stacking = -1;
    nused--;
    commit();
}

client_t *stacking_top()
{
    if (nused == 0) return NULL;
    else return clients[nused - 1];
}

/* FIXME:  remove NULL checks here, do once, further up call tree */
client_t *stacking_prev(client_t *client)
{
    if (client == NULL)
        return NULL;
    if (client->stacking <= 0) return NULL;
    else return clients[client->stacking - 1];
}

client_t *stacking_next(client_t *client)
{
    if (client == NULL)
        return NULL;
    if (client->stacking >= nused - 1) return NULL;
    else return clients[client->stacking + 1];
}

void stacking_raise(client_t *client)
{
    if (client == NULL)
        return;
    raise_tree(client, NULL, True);
    commit();
}

void stacking_restack(client_t *client)
{
    if (client == NULL)
        return;
    restack(client, False);
    commit();
}

#if 0
/* defined out to get rid of warning */
static void dump()
{
    int i;
    client_t *client;

    fprintf(stderr, "----\n");
    fprintf(stderr, "Clients:\n");
    for (i = 0; i < nused; i++) {
        fprintf(stderr, "% 2d. %#lx %s\n",
                i, clients[i]->frame, clients[i]->name);
    }
    fprintf(stderr, "Frames:\n");
    for (i = nused; i > 0; i--) {
        client = client_find(frames[i]);
        fprintf(stderr, "% 2d. %#lx ", i, frames[i]);
        if (client != NULL)
            fprintf(stderr, "%s", client->name);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "----\n");
}
#endif

/*
 * Ensures arrays have room enough for one more member.  Returns True
 * if have enough room.  Does not touch "nused".
 */

static Bool grow()
{
    static int nallocated = 0;
    Window *win_tmp;
    client_t **client_tmp;
    Window *frame_tmp;
    
    if (nallocated == 0) {
        frames = Malloc(sizeof(Window) * 3);
        windows = Malloc(sizeof(Window) * 3);
        clients = Malloc(sizeof(client_t *));
        if (frames == NULL || clients == NULL || windows == NULL) {
            if (frames != NULL) Free(frames);
            if (clients != NULL) Free(clients);
            if (windows != NULL) Free(windows);
            return False;
        }
        nallocated = 1;
    } else if (nallocated == nused) {
        win_tmp = Realloc(windows, sizeof(Window) * (nallocated * 2 + 2));
        frame_tmp = Realloc(frames, sizeof(Window) * (nallocated * 2 + 2));
        client_tmp = Realloc(clients, sizeof(client_t *) * nallocated * 2);

        if (win_tmp == NULL || client_tmp == NULL || frame_tmp == NULL) {
            return False;
        }
        windows = win_tmp;
        clients = client_tmp;
        frames = frame_tmp;
        nallocated *= 2;
    }
    return True;
}

/*
 * commits stacking order to X server and EWMH stacking list
 */

static void commit()
{
    int start, nitems, e_start, e_nitems;

    e_nitems = nitems = nused;
    if (stacking_hiding_window != None) {
        frames[0] = stacking_hiding_window;
        start = 0;
        nitems++;
    } else {
        start = 1;
    }
    if (stacking_desktop_frame != None) {
        frames[start + nitems] = stacking_desktop_frame;
        nitems++;
    }
    if (stacking_desktop_window != None) {
        windows[0] = stacking_desktop_window;
        e_start = 0;
        e_nitems++;
    } else {
        e_start = 1;
    }
    /* we ignore stacking_hiding_window for EWMH */
    
    XRestackWindows(dpy, &frames[start], nitems);
    ewmh_stacking_list_update(&windows[e_start], e_nitems);
}

/* defines partial ordering on clients
 * returns 1 if client1 should be on top of client2
 * returns 0 if can't determine which should be on top
 * returns -1 if client1 should be below client2 */
static int order(client_t *client1, client_t *client2)
{
    if (client1->always_on_top) {
        if (client2->always_on_top)
            return 0;
        return 1;
    } else if (client1->always_on_bottom) {
        if (client2->always_on_bottom)
            return 0;
        return -1;
    } else {
        if (client2->always_on_top)
            return -1;
        else if (client2->always_on_bottom)
            return 1;
        else
            return 0;
    }
}

/*
 * Ensures client is in correct place in arrays
 * Additionally moves client to top of peers if MOVE_UP is true
 */

static void restack(client_t *client, Bool move_up)
{
    int i;
    client_t *ctmp;
    int c;

    i = client->stacking;

    if (i < 0) {
        return;
    }
    /* I want a bug report if you see either of these */
    if (i > nused - 1) {
        fprintf(stderr, "AHWM: restack: assertion failed: i=%d, nused=%d\n",
                i, nused);
        fprintf(stderr, "name = %s\n", client->name);
        return;
    }
    if (client != clients[i]) {
        fprintf(stderr,
                "AHWM: restack: assertion failed: clients != clients[i]\n");
    }

    if (move_up) c = 0;
    else c = 1;

    /* move up if absolutely needed -OR-
     * if possible to move up and "move_up" = True */
    while (i + 1 < nused && order(clients[i], clients[i + 1]) >= c) {
        /* swap client[i] and client[i + 1] */
        ctmp = clients[i];
        set(clients[i + 1], i);
        set(ctmp, i + 1);
        i++;
    }

    i = client->stacking;
    
    /* move down if absolutely needed */
    while (i > 0 && order(clients[i], clients[i - 1]) < 0) {
        /* swap client[i] and client[i - 1] */
        ctmp = clients[i];
        set(clients[i - 1], i);
        set(ctmp, i - 1);
        i--;
    }
}

/*
 * Raising a window presents a somewhat interesting problem if we want
 * to hold the following invariant:
 * 
 * A client's transient windows are always on top of the client.
 *
 * This calls for a subtle combination of mathematics and extreme violence.
 * 
 * Consider the following tree, with the root node being 'A', and the
 * node we want raised being 'D' (with 'B' and 'C' being the path from
 * the requested window to the root window):
 * 
 * A
 * |- 1
 * |  `- 2
 * |- B
 * |  |- 10
 * |  |- C
 * |  |  |- 14
 * |  |  |- D
 * |  |  |  |- 16
 * |  |  |  |  `- 17
 * |  |  |  `- 18
 * |  |  `- 15
 * |  `- 11
 * |     |- 12
 * |     `- 13
 * `- 3
 *    |- 4
 *    |  `- 5
 *    |     |- 6
 *    |     `- 7
 *    `- 8
 *       `- 9
 * 
 * The nodes will be raised in the following order:
 * A 1 2 3 4 5 6 7 8 9 B 10 11 12 13 C 14 15 D 16 17 18
 * This holds the invariant, and ensures that each of A, B, C, D is
 * raised among its siblings.
 * 
 * The algorithm goes as follows:
 * 
 * raise(D) =
 * 
 * go up to C, ignoring D
 *     go up to B, ignoring C
 *         go up to A, ignoring B
 *             can't go any further up
 *             raise A
 *             raise all subtrees of A except B
 *         raise B
 *         raise all subtrees of B except C
 *     raise C
 *     raise all subtrees of C except D
 * raise D
 * raise all subtrees of D
 * 
 */

static void raise_tree(client_t *node, client_t *ignore, Bool go_up)
{
    client_t *parent, *c;

    if (node->transient_for == None) {
        parent = NULL;
    } else {
        parent = client_find(node->transient_for);
    }
    
    /* go up if needed */
    if (go_up && parent != NULL && parent->keep_transients_on_top) {
        raise_tree(parent, node, True);
    }

    /* visit node */
    if (node->workspace == workspace_current && node->state == NormalState) {
        XMapWindow(dpy, node->frame);
    }
    if (parent == NULL || order(node, parent) >= 0) {
        restack(node, True);
        debug(("\tRaising client %#lx ('%.10s')\n", node, node->name));
    }

    if (node->keep_transients_on_top == 0)
        return;
    
    /* go down */
    for (c = node->transients; c != NULL; c = c->next_transient) {
        if (c != ignore)
            raise_tree(c, NULL, False);
    }
}
