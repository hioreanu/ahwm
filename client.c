/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#include <stdlib.h>

#include "client.h"
#include "workspace.h"

XContext window_context;

client_t *client_create(Window w)
{
    client_t *client;
    XWMHints *xwmh;
    XWindowAttributes xwa;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return NULL;
    if (xwa.override_redirect) return NULL;

    client = malloc(sizeof(client_t));
    if (client == NULL) return NULL;
    memset(client, 0, sizeof(client_t));
    
    client->window = w;
    client->transient_for = None;
    client->name = "???";
    client->workspace = workspace_current;
    client->xwmh = XGetWMHints(dpy, w);
    client->state = xwa.map_state == IsViewable ? MAPPED : UNMAPPED;
    client->x = xwa.x;
    client->y = xwa.y;
    client->width = xwa.width;
    client->height = xwa.height;

    if (focus_canfocus(client)) focus_add(client);

    if (XSaveContext(dpy, w, window_context, (void *)client) != 0) {
        free(client);
        return NULL;
    }
    printf("Created an entry for window 0x%08X at 0x%08X\n", w, client);
    return client;
}

client_t *client_find(Window w)
{
    client_t *client;

    if (XFindContext(dpy, w, window_context, (void *)&client) != 0)
        return NULL;
    return client;
}

void client_destroy(client_t *client)
{
    if (client->xwmh != NULL) XFree(client->xwmh);
    XDeleteContext(dpy, client->window, window_context);
    free(client);
}

