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
 * The stacking order is kept as a list of clients; whenever we change
 * the stacking order, we build an array from the list and call
 * XRestackWindows().  We never ask the server for the current
 * stacking order via XQueryTree() as we are always in control of the
 * stacking order (even on startup when we manage already-mapped
 * windows, we add them to the list in stacking order
 * (xwm.c:scan_windows)).
 */

#include "config.h"

#include <X11/Xlib.h>
#include <stdio.h>

#include "client.h"
#include "stacking.h"
#include "malloc.h"
#include "workspace.h"
#include "debug.h"

Window stacking_hiding_window = None;
Window stacking_desktop_window = None;

/* We keep around some extra pointers to avoid walking lists too often */
static client_t *normal_list, *lowered_list, *raised_list;
static client_t *normal_list_end, *lowered_list_end, *raised_list_end;

static int nstacking_clients = 0;

static void stacking_add_internal(client_t *client);
static void stacking_remove_internal(client_t *client);
static void commit();
static void raise_tree(client_t *client, client_t *ignore, Bool go_up);

/* hopefully compiler can figure out how to inline this given the
 * static variables and whatnot */
client_t *stacking_top()
{
    if (raised_list_end != NULL) return raised_list_end;
    if (normal_list_end != NULL) return normal_list_end;
    if (lowered_list_end != NULL) return lowered_list_end;
    return NULL;
}

void stacking_add(client_t *client)
{
    stacking_add_internal(client);
    commit();
}

void stacking_remove(client_t *client)
{
    stacking_remove_internal(client);
    commit();
}

void stacking_raise(client_t *client)
{
    raise_tree(client, NULL, True);
    commit();
}

/* builds stacking array from list and calls XRestackWindows() */
static void commit()
{
    client_t *start, *client;
    Window *tmp;
    int i;
    static Window *buffer = NULL;
    static int nallocated = 0;

    /* ensure buffer big enough */
    if (buffer == NULL) {
        buffer = Malloc((nstacking_clients + 2) * sizeof(Window));
        if (buffer == NULL) {
            fprintf(stderr, "XWM: out of memory while raising client\n");
            return;
        }
        nallocated = nstacking_clients;
    } else if (nallocated < nstacking_clients) {
        tmp = Realloc(buffer, (nstacking_clients + 2) * sizeof(Window));
        if (tmp == NULL) {
            fprintf(stderr, "XWM: out of memory while raising client\n");
            return;
        }
        buffer = tmp;
        nallocated = nstacking_clients;
    }

    /* build the stacking array */
    start = stacking_top();
    if (start == NULL) return;

    i = 0;
    if (stacking_hiding_window != None)
        buffer[i++] = stacking_hiding_window;
    for (client = start; client != NULL; client = client->prev_stacking) {
        buffer[i++] = client->frame;
    }
    if (stacking_desktop_window != None)
        buffer[i++] = stacking_desktop_window;
    
    debug(("\tCommitting stacking order\n"));
    XRestackWindows(dpy, buffer, i);

    /* update _NET_CLIENT_LIST_STACKING */
    if (stacking_hiding_window == None) {
        tmp = buffer;
    } else {
        tmp = buffer + 1;
        i--;
    }
    if (stacking_desktop_window != None) i--;
    ewmh_stacking_list_update(tmp, i);
}

/* all of the functions below just update pointers */
/* these two get a bit ugly since we need to update the six pointers
 * into the list, but we don't have to walk the list */
static void stacking_add_internal(client_t *client)
{
    client_t *next, *prev;

    nstacking_clients++;

    if (client->always_on_bottom) {
        if (lowered_list == NULL) {
            lowered_list = client;
            prev = NULL;
        } else {
            prev = lowered_list_end;
        }
        lowered_list_end = client;
        next = (normal_list != NULL ? normal_list : raised_list);
        prev = NULL;
    } else if (client->always_on_top) {
        if (raised_list == NULL) {
            raised_list = client;
            prev = (normal_list_end != NULL ?
                    normal_list_end : lowered_list_end);
        } else {
            prev = raised_list_end;
        }
        raised_list_end = client;
        next = NULL;
    } else {
        if (normal_list == NULL) {
            normal_list = client;
            prev = lowered_list_end;
        } else {
            prev = normal_list_end;
        }
        normal_list_end = client;
        next = raised_list;
    }
    
    if (next != NULL) next->prev_stacking = client;
    client->next_stacking = next;
    if (prev != NULL) prev->next_stacking = client;
    client->prev_stacking = prev;
}

static void stacking_remove_internal(client_t *client)
{
    nstacking_clients--;
    if (client->next_stacking != NULL)
        client->next_stacking->prev_stacking = client->prev_stacking;
    if (client->prev_stacking != NULL)
        client->prev_stacking->next_stacking = client->next_stacking;
    
    if (client->always_on_bottom) {
        if (client == lowered_list_end)
            lowered_list_end = client->prev_stacking;
        if (client == lowered_list)
            lowered_list = client->next_stacking;
        if (lowered_list != NULL && lowered_list == normal_list)
            lowered_list = NULL;
    } else if (client->always_on_top) {
        if (client == raised_list_end)
            raised_list_end = client->prev_stacking;
        if (client == raised_list)
            raised_list = client->next_stacking;
        if (raised_list_end != NULL && raised_list_end == normal_list_end)
            raised_list_end = NULL;
    } else {
        if (client == normal_list_end)
            normal_list_end = client->prev_stacking;
        if (client == normal_list)
            normal_list = client->next_stacking;
        if (normal_list != NULL && normal_list == raised_list)
            normal_list = NULL;
        if (normal_list_end != NULL && normal_list_end == lowered_list_end)
            normal_list_end = NULL;
    }
}

/*
 * Raising a window presents a somewhat interesting problem because we
 * want to hold the following invariant:
 * 
 * A client's transient windows are always on top of the client.
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

    /* go up if needed */
    if (go_up && node->transient_for != None) {
        parent = client_find(node->transient_for);
        if (parent != NULL) {
            raise_tree(parent, node, True);
        }
    }

    /* visit node */
    if (node->workspace == workspace_current
        && node->state == NormalState) {
        XMapWindow(dpy, node->frame);
        stacking_remove_internal(node);
        stacking_add_internal(node);
        debug(("\tRaising client %#lx ('%.10s')\n", node, node->name));
    }

    /* go down */
    for (c = node->transients; c != NULL; c = c->next_transient) {
        if (c != ignore)
            raise_tree(c, NULL, False);
    }
}
