/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "focus.h"

client_t *focus_current = None;

void focus_lost()
{
    /* walk the focus list or stack or something else clever */
}

void focus_set(client_t *client)
{
    client->prevfocus = focus_current;
    focus_current = client;
    XSetInputFocus(dpy, client->window, RevertToNone, CurrentTime);
}

int focus_wantsfocus(client_t *client)
{
    Window w = client->window;
    /* XQueryTree() */
    
}
