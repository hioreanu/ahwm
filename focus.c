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

void focus_add(client_t *client)
{
    client_t *old;

    focus_remove(client);
    old = focus_stacks[client->workspace - 1];
    if (old == NULL) {
        client->next = client;
        client->prev = client;
    } else {
        client->next = old;
        client->prev = old->prev;
        old->prev->next = client;
        old->prev = client;
    }
    focus_stacks[client->workspace - 1] = client;
}

void focus_remove(client_t *client)
{
    client_t *stack, *orig;

    orig = focus_stacks[client->workspace - 1];
    stack = orig;
    if (stack == NULL) return;
    do {
        if (stack == client) {
            focus_stacks[client->workspace - 1] = client->next;
            focus_stacks[client->workspace - 1]->prev = client->prev;
            client->prev->next = focus_stacks[client->workspace - 1];
            if (focus_stacks[client->workspace - 1] == client)
                focus_stacks[client->workspace - 1] = NULL;
            return;
        }
        stack = stack->next;
    } while (stack != orig);
}

void focus_set(client_t *client)
{
/*    focus_remove(client); */
    focus_add(client);
}

void focus_next()
{
    if (focus_current != NULL)
        focus_set(focus_current->next);
}

void focus_prev()
{
    if (focus_current != NULL)
        focus_set(focus_current->prev);
}

int focus_canfocus(client_t *client)
{
    /* Window Maker does a bunch of stuff with KDE here */
    /* ICC/client-to-windowmanager/wm-hints.html */

    if (client->state != NormalState) return 0;
    if (client->workspace != workspace_current) return 0;
    if (client->xwmh == NULL) return 1;
    if (client->xwmh->flags & InputHint && client->xwmh->input == False)
        return 0;
    return 1;
}

void focus_ensure(Time timestamp)
{
    if (focus_current == focus_stacks[workspace_current - 1])
        return;
    focus_current = focus_stacks[workspace_current - 1];
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
    /* We raise the window when we get back the FocusIn event, not here: */
/*    XMapRaised(dpy, focus_current->frame); */
}
