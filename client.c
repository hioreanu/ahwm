/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "client.h"
#include "workspace.h"
#include "keyboard.h"

XContext window_context;
XContext frame_context;

client_t *client_create(Window w)
{
    client_t *client;
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
    client->workspace = workspace_current;
    client->state = Withdrawn;
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

    client_set_name(client);
    client_set_instance_class(client);
    client_set_xwmh(client);
    client_set_xsh(client);
        
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
    free(client->name);         /* should never be NULL */
    if (client->instance != NULL) XFree(client->instance);
    if (client->class != NULL) XFree(client->class);
    free(client);
}

/* snarfed mostly from ctwm and WindowMaker */
void client_set_name(client_t *client)
{
    XTextProperty xtp;
    char **list;
    int n;

    if (XGetWMName(dpy, client->window, &xtp) == 0) {
        client->name = strdup("");      /* client did not set a window name */
        return;
    }
    if (xtp.value == NULL || xtp.nitems <= 0) {
        /* client set window name to NULL */
        client->name = strdup("");
    } else {
        if (xtp.encoding == XA_STRING) {
            /* usual case */
            client->name = strdup(xtp.value);
        } else {
            /* client is using UTF-16 or something equally stupid */
            /* haven't seen this block actually run yet */
            xtp.nitems = strlen((char *)xtp.value);
            if (XmbTextPropertyToTextList(dpy, &xtp, &list, &n) == Success
                && n > 0 && *list != NULL) {
                client->name = strdup(*list);
                XFreeStringList(list);
            } else {
                client->name = strdup("");
            }
        }
    }
    XFree(xtp.value);

#ifdef DEBUG
    printf("\tClient 0x%08X is %s\n", client, client->name);
#endif /* DEBUG */
}

void client_set_instance_class(client_t *client)
{
    XClassHint xch;

    client->class = NULL;
    client->instance = NULL;

    if (XGetClassHint(dpy, w, &xch) != 0) {
        client->instance = xch.res_name;
        client->class = xch.res_class;
    }
}

void client_set_xwmh(client_t *client)
{
    client->xwmh = XGetWMHints(dpy, client->window);
}

void client_set_xsh(client_t *client)
{
    long set_fields;

    client->xsh = XAllocSizeHints();
    if (client->xsh == NULL) {
        fprintf(stderr, "Couldn't allocate Size Hints structure\n");
        return;
    }
    if (XGetWMSizeHints(dpy, client->window, client->xsh,
                        &set_fields, XA_WM_SIZE_HINTS) == 0) {
        XFree(client->xsh);
        client->xsh = NULL;
    }
}

/* ICCCM, 4.1.3.1 */
void client_inform_state(client_t *client)
{
    /* FIXME */
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
