/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "focus.h"
#include "workspace.h"

client_t *focus_current = None;

typedef struct _focus_stack_t {
    client_t *client;
    struct _focus_stack_t *next;
} focus_stack_t;

focus_stack_t *focus_stacks[NO_WORKSPACES];

void focus_lost()
{
    /* walk the focus list or stack or something else clever */
}

void focus_none()
{
    focus_current = None;
    XSetInputFocus(dpy, None, RevertToNone, CurrentTime);
}

void focus_set(client_t *client)
{
    client->prevfocus = focus_current;
    focus_current = client;
    XSetInputFocus(dpy, client->window, RevertToNone, CurrentTime);
}

int focus_settoplevel(client_t *client)
{
    focus_stack_t *oldstack, *newstack;

    oldstack = focus_stacks[client->workspace - 1];
    newstack = malloc(sizeof(focus_stack_t));
    if (newstack == NULL) return -1;
    newstack->next = oldstack;
    newstack->client = client;
    focus_stacks[client->workspace - 1] = newstack;
    return 0;
}

int focus_canfocus(client_t *client)
{
    Window w = client->window;
    /* XQueryTree() */

    if (client->state != NormalState) return 0;
    if (client->workspace != workspace_current) return 0;
    if (client->xwmh == NULL) return 1;
    if (client->xwmh.flags & InputHint && client->input == False)
        return 0;
    return 1;

    /* Window Maker does a bunch of stuff with KDE here */
    /* ICC/client-to-windowmanager/wm-hints.html */
}
