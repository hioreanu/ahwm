/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#include <stdlib.h>

#include "client.h"
#include "workspace.h"
#include "keyboard.h"

XContext window_context;
XContext frame_context;

client_t *client_create(Window w)
{
    client_t *client;
    XWMHints *xwmh;
    XWindowAttributes xwa;
    XSetWindowAttributes xswa;
    int mask;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return NULL;
    if (xwa.override_redirect) {
#ifdef DEBUG
        printf("\tWindow has override_redirect, not creating client\n");
#endif /* DEBUG */
        return NULL;
    }

    client = malloc(sizeof(client_t));
    if (client == NULL) {
        fprintf(stderr, "Malloc failed, unable to allocate client\n");
        return NULL;
    }
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
        fprintf(stderr, "XSaveContext failed, could not save window\n");
        free(client);
        return NULL;
    }
#ifdef DEBUG
    printf("\tCreated an entry for window 0x%08X at 0x%08X\n", w, client);
#endif /* DEBUG */

    /* create frame and reparent */
    client->frame = None;
    if (1) {
        mask = CWBackPixmap | CWBackPixel | CWBorderPixel
               | CWCursor | CWEventMask | CWOverrideRedirect;
        xswa.cursor = cursor_normal;
        xswa.background_pixmap = None;
        xswa.background_pixel = black;
        xswa.event_mask = SubstructureRedirectMask | ExposureMask |
                          EnterWindowMask | LeaveWindowMask;
        xswa.override_redirect = True;
        client->frame = XCreateWindow(dpy, root_window, client->x, client->y,
                                      client->width, client->height, 0,
                                      DefaultDepth(dpy, scr), CopyFromParent,
                                      DefaultVisual(dpy, scr),
                                      mask, &xswa);
#ifdef DEBUG
        printf("\tReparenting client 0x%08X (window 0x%08X) to 0x%08X\n",
               client, w, client->frame);
#endif /* DEBUG */
        XClearWindow(dpy, client->frame);
        /* ignore the map and unmap events caused by the reparenting: */
        XSelectInput(dpy, w, xwa.your_event_mask & ~StructureNotifyMask);
        XReparentWindow(dpy, w, client->frame, 0, TITLE_HEIGHT);
        XSelectInput(dpy, w,
                     xwa.your_event_mask // | EnterWindowMask
                     | StructureNotifyMask);
    }
    if (client->frame != None) {
        if (XSaveContext(dpy, client->frame,
                         frame_context, (void *)client) != 0) {
            XDeleteContext(dpy, client->window, window_context);
            free(client);
            fprintf(stderr, "XSaveContext failed, could not save frame\n");
            return NULL;
        }
    } else {
        XSelectInput(dpy, w, EnterWindowMask);
    }

    return client;
}

client_t *client_find(Window w)
{
    client_t *client;

    if (XFindContext(dpy, w, window_context, (void *)&client) != 0)
        if (XFindContext(dpy, w, frame_context, (void *)&client) != 0)
            client = NULL;

#ifdef DEBUG
    if (client == NULL)
        printf("\tCould not find client\n");
    else if (client->window == w)
        printf("\tFound client from window\n");
    else
        printf("\tFound client from frame\n");
#endif /* DEBUG */
    
    return client;
}

void client_destroy(client_t *client)
{
    if (client->frame != None)
        XUnmapWindow(dpy, client->frame);
    if (client->xwmh != NULL) XFree(client->xwmh);
    XDeleteContext(dpy, client->window, window_context);
    XDeleteContext(dpy, client->frame, frame_context);
    free(client);
}

void client_print(char *s, client_t *client)
{
    if (client == NULL) {
        printf("%-19s null client\n", s);
        return;
    }
    printf("%-19s client = 0x%08X, window = 0x%08X, frame = 0x%08X\n",
           s, client, client->window, client->frame);
}

