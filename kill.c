/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "kill.h"
#include "event.h"

void kill_nicely(XEvent *xevent)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "XWM: Unable to kill client, client not found\n");
    }
    if (client->protocols & PROTO_DELETE_WINDOW) {
#ifdef DEBUG
        printf("\tPolitely requesting window to die\n");
#endif /* DEBUG */
        client_sendmessage(client, WM_DELETE_WINDOW, event_timestamp,
                           0, 0, 0);
    } else {
#ifdef DEBUG
        printf("\tWindow isn't civilized, exterminating it\n");
#endif /* DEBUG */
        kill_with_extreme_prejudice(xevent);
    }
}
    
void kill_with_extreme_prejudice(XEvent *xevent)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "XWM: Unable to kill client, client not found\n");
    }
#ifdef DEBUG
    printf("Commiting windicide\n");
#endif /* DEBUG */
    XKillClient(dpy, client->window);
}
