/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "focus.h"
#include "client.h"
#include "workspace.h"

client_t *focus_current = NULL;

static client_t *focus_stacks[NO_WORKSPACES] = { NULL };

/* 
 * invariant:
 * 
 * focus_current == focus_stacks[workspace_current - 1]
 * 
 * This should hold whenever we enter or leave the window manager.
 * Call focus_ensure() whenever leaving the window manager (and the
 * invariant may not hold) to update 'focus_current'.
 */

void focus_add(client_t *client, Time timestamp)
{
    client_t *old;

    focus_remove(client, timestamp);
    old = focus_stacks[client->workspace - 1];
    if (old == NULL) {
        client->next = client;
        client->prev = client;
        focus_stacks[client->workspace - 1] = client;
    } else {
        client->next = old;
        client->prev = old->prev;
        old->prev->next = client;
        old->prev = client;
    }
    if (client->workspace == workspace_current) {
        focus_current = client;
        focus_ensure(timestamp);
    }
}

void focus_remove(client_t *client, Time timestamp)
{
    client_t *stack, *orig;

    stack = orig = focus_stacks[client->workspace - 1];
    if (stack == NULL) return;
    do {
        if (stack == client) {
            /* remove from list */
            client->prev->next = client->next;
            client->next->prev = client->prev;
            /* if was focused for workspace, update workspace pointer */
            if (focus_stacks[client->workspace - 1] == client) {
                focus_stacks[client->workspace - 1] = client->next;
            }
            /* if only client left on workspace, set to NULL */
            if (client->next == client) {
                focus_stacks[client->workspace - 1] = NULL;
                client->next = NULL;
                client->prev = NULL;
            }
            /* if removed from current workspace, refocus now */
            if (client->workspace == workspace_current) {
                focus_current = client->next;
                focus_ensure(timestamp);
            }
            return;
        }
        stack = stack->next;
    } while (stack != orig);
}

void focus_set(client_t *client, Time timestamp)
{
    client_t *p;

    p = focus_stacks[client->workspace - 1];
    if (p == NULL) {
        fprintf(stderr, "XWM: current focus list is empty, shouldn't be\n");
        return;
    }
    do {
        if (p == client) {
            focus_stacks[client->workspace - 1] = client;
            if (client->workspace == workspace_current) {
                focus_current = client;
                focus_ensure(timestamp);
            }
            return;
        }
        p = p->next;
    } while (p != focus_stacks[client->workspace - 1]);
    fprintf(stderr, "XWM: client not found on focus list, shouldn't happen\n");
}

void focus_next(Time timestamp)
{
    if (focus_current != NULL)
        focus_set(focus_current->next, timestamp);
}

void focus_prev(Time timestamp)
{
    if (focus_current != NULL)
        focus_set(focus_current->prev, timestamp);
}

void focus_ensure(Time timestamp)
{
//    if (focus_current == focus_stacks[workspace_current - 1])
//        return;
//    focus_current = focus_stacks[workspace_current - 1];
    if (focus_current == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }

#ifdef DEBUG
    printf("\tSetting focus to 0x%08X...\n",
           (unsigned int)focus_current->window);
#endif /* DEBUG */

    /* see ICCCM 4.1.7 */
    if (focus_current->xwmh != NULL &&
        focus_current->xwmh->flags & InputHint &&
        focus_current->xwmh->input == False) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        printf("DOESN'T WANT FOCUS\n");
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
            printf("GLOBALLY ACTIVE FOCUS\n");
        }
    } else {
        printf("WANTS FOCUS\n");
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            printf("WILL TAKE FOCUS\n");
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
        }
        XSetInputFocus(dpy, focus_current->window,
                       RevertToPointerRoot, timestamp);
    }
    XSync(dpy, False);
    XFlush(dpy);
    XMapRaised(dpy, focus_current->frame);
}
