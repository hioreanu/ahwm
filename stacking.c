/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include "client.h"
#include "stacking.h"
#include "malloc.h"
#include "workspace.h"
#include "debug.h"

Window stacking_hiding_window = None;
Window stacking_desktop_window = None;

/* We keep around some extra pointers to avoid walking any lists when
 * adding a client */
static client_t *normal_list, *lowered_list, *raised_list;
static client_t *normal_list_end, *lowered_list_end, *raised_list_end;

static int nstacking_clients = 0;

static void stacking_add_internal(client_t *client);
static void stacking_remove_internal(client_t *client);
static void commit();
static void raise_tree(client_t *client, client_t *ignore, Bool go_up);

/* hopefully compiler can figure out how to inline this given the
 * static variables and whatnot - kinda hard */
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

static void commit()
{
    client_t *start, *client;
    Window *tmp;
    int i;
    static Window *buffer = NULL;
    static int nallocated = 0;

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
    /* FIXME:  also do _NET_CLIENT_LIST_STACKING now */
}

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

#if 0

/*
 * Raising a window presents a somewhat interesting problem because we
 * want to hold the following invariant:
 * 
 * A client's transient windows are always on top of the client.
 * 
 * The problem arises because one can have arbitrarily complex trees
 * of transient windows (a window may have any number of transients
 * but it will only have one or zero windows for which it is
 * transient).  In order to raise a window, we must raise the entire
 * tree, ensuring that each node along the path up from the window
 * requested to the root must be raised among all nodes at the same
 * height in the tree.
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
 * We don't want to generate a RaiseWindow request every time we visit
 * a node, so we do it all at once with an XRaiseWindow and an
 * XRestackWindows.  We also have to deal with windows that should be
 * kept on bottom and windows that should be kept on top - we hold the
 * invariants that for any windows A, B:
 * 
 * if (A.keep_on_bottom and not B.keep_on_bottom)
 *     A < B
 * if (A.keep_on_top and not B.keep_on_top)
 *     A > B
 * 
 * keep_on_top and keep_on_bottom are mutually exclusive.  We do raise
 * a keep_on_bottom window to the top of all lowered windows when we
 * receive this request, however (and the same for keep_on_top
 * windows).
 */

typedef struct _window_buffer {
    Window *w;
    int nused;
    int nallocated;
} window_buffer;

static void add_to_buffer(window_buffer *wb, Window w)
{
    Window *tmp;
    
    if (wb->nallocated == wb->nused) {
        tmp = Realloc(wb->w, 2 * wb->nallocated * sizeof(Window));
        if (tmp == NULL) {
            fprintf(stderr,
                    "XWM: Out of memory while raising window\n");
            return;
        }
        wb->nallocated *= 2;
        wb->w = tmp;
    }
    wb->w[wb->nused++] = node->frame;
}

static void raise_tree(client_t *node, client_t *ignore, Bool go_up,
                       window_buffer *normal, window_buffer *lowered,
                       window_buffer *raised)
{
    client_t *parent, *c;

    /* go up if needed */
    if (go_up && node->transient_for != None) {
        parent = client_find(node->transient_for);
        if (parent != NULL) {
            raise_tree(parent, node, True, wb);
        }
    }

    /* visit node */
    if (node->workspace == workspace_current
        && node->state == NormalState) {
        XMapWindow(dpy, node->frame);
        if (node->always_on_bottom) {
            add_to_buffer(lower, node->frame);
        } else if (node->always_on_top) {
            add_to_buffer(raised, node->frame);
        } else {
            add_to_buffer(normal, node->frame);
        }
    }

    /* go down */
    for (c = node->transients; c != NULL; c = c->next_transient) {
        if (c != ignore)
            raise_tree(c, NULL, False, wb);
    }
}

void client_raise(client_t *client)
{
    static window_buffer normal = {NULL, 0, 0};
    static window_buffer lowered, raised;
    Window lowest_normal, lowest_raised;
    client_t *c1, *c2;
    Window tmp;
    int i;

    if (normal.w == NULL) {
        normal.w = Malloc(sizeof(Window));
        raised.w = Malloc(sizeof(Window));
        lowered.w = Malloc(sizeof(Window));
        if (normal.w == NULL || raised.w == NULL || lowered.w == NULL) {
            fprintf(stderr, "XWM: Out of memory while raising window\n");
            if (normal.w != NULL) free(normal.w);
            if (raised.w != NULL) free(raised.w);
            if (lowered.w != NULL) free(lowered.w);
            normal.w = raised.w = lowered.w = NULL;
            XMapWindow(dpy, client->frame);
            if (!client->always_on_bottom)
                XRaiseWindow(dpy, client->frame);
            return;
        }
        normal.nallocated = raised.nallocated = lowered.nallocated = 1;
    }
    normal.nused = raised.nused = lowered.nused = 0;

    raise_tree(client, NULL, True, &normal, &lowered, &raised);

    add_to_buffer(lowered, lowest_normal);
    add_to_buffer(normal, lowest_raised);
    
    /* must reverse the arrays because XRestackWindows works backwards */
    for (i = 0; 2 * i < normal.nused; i++) {
        tmp = normal.w[i];
        normal.w[i] = normal.w[normal.nused - i - 1];
        normal.w[normal.nused - i - 1] = tmp;
    }
    for (i = 0; 2 * i < raised.nused; i++) {
        tmp = raised.w[i];
        raised.w[i] = raised.w[raised.nused - i - 1];
        raised.w[raised.nused - i - 1] = tmp;
    }
    for (i = 0; 2 * i < lowered.nused; i++) {
        tmp = lowered.w[i];
        lowered.w[i] = lowered.w[lowered.nused - i - 1];
        lowered.w[lowered.nused - i - 1] = tmp;
    }

    if (lowered.nused != 0)
        XRestackWindows(dpy, lowered.w, lowered.nused);
    if (normal.nused != 0)
        XRestackWindows(dpy, normal.w, normal.nused);
    if (raised.nused != 0) {
        XRaiseWindow(dpy, raised.w[0]);
        XRestackWindows(dpy, raised.w, raised.nused);
    }
}

#endif
