/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "kill.h"
#include "event.h"
#include "debug.h"

void kill_nicely(XEvent *xevent, void *v)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "XWM: Unable to kill client, client not found\n");
    }
    if (client->protocols & PROTO_DELETE_WINDOW) {
        debug(("\tPolitely requesting window to die\n"));
        client_sendmessage(client, WM_DELETE_WINDOW, event_timestamp,
                           0, 0, 0);
    } else {
        debug(("\tWindow isn't civilized, exterminating it\n"));
        kill_with_extreme_prejudice(xevent, v);
    }
}
    
void kill_with_extreme_prejudice(XEvent *xevent, void *v)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "XWM: Unable to kill client, client not found\n");
    }
    debug(("Commiting windicide\n"));
    XKillClient(dpy, client->window);
}
