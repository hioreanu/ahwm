/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "focus.h"
#include "client.h"
#include "workspace.h"
#include "debug.h"

client_t *focus_current = NULL;

client_t *focus_stacks[NO_WORKSPACES] = { NULL };

static void focus_change_current(client_t *, Time);

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
        debug(("\tsetting focus stack of workspace %d to %s\n",
               client->workspace, client->name));
        client->next_focus = client;
        client->prev_focus = client;
        focus_stacks[client->workspace - 1] = client;
    } else {
        client->next_focus = old;
        client->prev_focus = old->prev_focus;
        old->prev_focus->next_focus = client;
        old->prev_focus = client;
    }
    if (client->workspace == workspace_current) {
        focus_change_current(client, timestamp);
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
            client->prev_focus->next_focus = client->next_focus;
            client->next_focus->prev_focus = client->prev_focus;
            /* if was focused for workspace, update workspace pointer */
            if (focus_stacks[client->workspace - 1] == client) {
                debug(("\tsetting focus stack of workspace %d to %s\n",
                       client->workspace, client->prev_focus->name));
                focus_stacks[client->workspace - 1] = client->prev_focus;
            }
            /* if only client left on workspace, set to NULL */
            if (client->next_focus == client) {
                debug(("setting focus stack of workspace %d to null\n",
                       client->workspace));
                focus_stacks[client->workspace - 1] = NULL;
                client->next_focus = NULL;
                client->prev_focus = NULL;
            }
            /* if removed was focused window, refocus now */
            if (client == focus_current) {
                focus_change_current(client->prev_focus, timestamp);
            }
            return;
        }
        stack = stack->next_focus;
    } while (stack != orig);
}

void focus_set(client_t *client, Time timestamp)
{
    client_t *p;

    if (client == focus_current) return;

    if (client == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }
    
    p = focus_stacks[client->workspace - 1];
    if (p == NULL) {
        fprintf(stderr, "XWM: current focus list is empty, shouldn't be\n");
        return;
    }
    do {
        if (p == client) {
            debug(("setting focus stack of workspace %d to %s\n",
                   client->workspace, client->name));
            focus_stacks[client->workspace - 1] = client;
            if (client->workspace == workspace_current) {
                focus_change_current(client, timestamp);
            }
            return;
        }
        p = p->next_focus;
    } while (p != focus_stacks[client->workspace - 1]);
    fprintf(stderr, "XWM: client not found on focus list, shouldn't happen\n");
}

void focus_next(Time timestamp)
{
    if (focus_current != NULL)
        focus_set(focus_current->next_focus, timestamp);
}

void focus_prev(Time timestamp)
{
    if (focus_current != NULL)
        focus_set(focus_current->prev_focus, timestamp);
}

void focus_ensure(Time timestamp)
{
    if (focus_current == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }

    debug(("\tSetting focus to 0x%08X (%s)...\n",
           (unsigned int)focus_current->window, focus_current->name));

    /* see ICCCM 4.1.7 */
    if (focus_current->xwmh != NULL &&
        focus_current->xwmh->flags & InputHint &&
        focus_current->xwmh->input == False) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        debug(("\tdoesn't want focus\n"));
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
            debug(("\tglobally active focus\n"));
        }
    } else {
        debug(("\twants focus\n"));
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            debug(("\twill forcibly take focus\n"));
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
        }
        XSetInputFocus(dpy, focus_current->window,
                       RevertToPointerRoot, timestamp);
    }
    XSync(dpy, False);
    XFlush(dpy);

    client_raise(focus_current);
}

static void focus_change_current(client_t *new, Time timestamp)
{
    client_t *old;

    old = focus_current;
    focus_current = new;
    client_paint_titlebar(old);
    client_paint_titlebar(new);
    focus_ensure(timestamp);
}

