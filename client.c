/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include "client.h"
#include "workspace.h"
#include "keyboard.h"
#include "cursor.h"
#include "focus.h"

XContext window_context;
XContext frame_context;
XContext title_context;

static void client_add_titlebar(client_t *client);

client_t *client_create(Window w)
{
    client_t *client;
    XWindowAttributes xwa;
    Bool has_titlebar = True;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return NULL;
    if (xwa.override_redirect) {
#ifdef DEBUG
        printf("\tWindow has override_redirect, not creating client\n");
#endif /* DEBUG */
        /* FIXME:  may still need to reparent to fake root */
        return NULL;
    }

    client = malloc(sizeof(client_t));
    if (client == NULL) {
        fprintf(stderr, "XWM: Malloc failed, unable to allocate client\n");
        return NULL;
    }
    memset(client, 0, sizeof(client_t));
    
    client->window = w;
    client->transient_for = None;
    client->workspace = workspace_current;
    client->state = WithdrawnState;
    client->frame = None;
    client->titlebar = None;
    client->prev_x = client->prev_y = -1;
    client->prev_height = client->prev_width = -1;

    client_set_name(client);
    client_set_instance_class(client);
    client_set_xwmh(client);
    client_set_xsh(client);
    client_set_protocols(client);

    client->frame_event_mask = SubstructureRedirectMask | EnterWindowMask
                              | FocusChangeMask;
    client->window_event_mask = xwa.your_event_mask | StructureNotifyMask
                                | PropertyChangeMask;
    XSelectInput(dpy, client->window, client->window_event_mask);

#ifdef SHAPE
    if (shape_supported) {
        int tmp;
        unsigned int tmp2;
        int shaped;

        XShapeSelectInput(dpy, client->window, ShapeNotifyMask);
        XShapeQueryExtents(dpy, client->window, &shaped, tmp, tmp,
                           tmp2, tmp, tmp, tmp, tmp2, tmp2);
        has_titlebar = !shaped;
    }
#endif /* SHAPE */
    
    client_reparent(client, has_titlebar);
    if (client->frame == None) {
        fprintf(stderr, "Could not reparent window\n");
        free(client);
        return NULL;
    }
    if (has_titlebar) client_add_titlebar(client);
    
    if (focus_canfocus(client)) focus_add(client);

    if (XSaveContext(dpy, w, window_context, (void *)client) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save window\n");
        free(client);
        return NULL;
    }
#ifdef DEBUG
    printf("\tCreated an entry for window 0x%08X at 0x%08X\n",
           (unsigned int)w, (unsigned int)client);
#endif /* DEBUG */

    if (XSaveContext(dpy, client->frame,
                     frame_context, (void *)client) != 0) {
        XDeleteContext(dpy, client->window, window_context);
        free(client);
        fprintf(stderr, "XWM: XSaveContext failed, could not save frame\n");
        return NULL;
    }
    if (client->titlebar != None) {
        if (XSaveContext(dpy, client->titlebar,
                         title_context, (void *)client) != 0) {
            XDeleteContext(dpy, client->window, window_context);
            XDeleteContext(dpy, client->frame, frame_context);
            free(client);
            fprintf(stderr,
                    "XWM: XSaveContext failed, could not save titlebar\n");
            return NULL;
        }
    }
    
    return client;
}

/*
 * FIXME:  should figure a way around calling XGetWindowAttributes
 * twice when creating a client and remapping window
 */

void client_reparent(client_t *client, Bool has_titlebar)
{
    XSetWindowAttributes xswa;
    XWindowAttributes xwa;
    XWindowChanges xwc;
    int mask;
    Window w;
    position_size ps;

    w = client->window;
    if (XGetWindowAttributes(dpy, w, &xwa) == 0) {
        fprintf(stderr,
                "XWM: XGetWindowAttributes failed in a very inconvenient place\n");
        return;
    }
    mask = CWBackPixmap | CWBackPixel | CWCursor
           | CWEventMask | CWOverrideRedirect;
    xswa.cursor = cursor_normal;
    xswa.background_pixmap = None;
    xswa.background_pixel = black;
    xswa.event_mask = client->frame_event_mask;
    xswa.override_redirect = True;

    ps.x = xwa.x;
    ps.y = xwa.y;
    ps.width = xwa.width;
    ps.height = xwa.height;
    client_get_position_size_hints(client, &ps);
    /* client_frame_position checks for the titlebar which
     * has not yet been created - I'd rather not change the api */
    if (has_titlebar) {
        client->titlebar = client->window;
        client_frame_position(client, &ps);
        client->titlebar = None;
    } else {
        client_frame_position(client, &ps);
    }
    client->x = ps.x;
    client->y = ps.y;
    client->width = ps.width;
    client->height = ps.height;
    
    if (client->frame == None) {
        client->frame = XCreateWindow(dpy, root_window, ps.x, ps.y,
                                      ps.width, ps.height, 0,
                                      DefaultDepth(dpy, scr), CopyFromParent,
                                      DefaultVisual(dpy, scr),
                                      mask, &xswa);
    } else {
        /* if the client already has a frame, assume that we need to
         * update the frame as if it were the first time we are creating
         * it; however, deleting the frame and creating a new window is
         * more expensive than just resetting the frame to the initial
         * state */
        XSelectInput(dpy, w, client->window_event_mask & ~StructureNotifyMask);
        XReparentWindow(dpy, w, root_window, 0, 0);
        XChangeWindowAttributes(dpy, client->frame, mask, &xswa);

        mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
        xwc.x = ps.x;
        xwc.y = ps.y;
        xwc.width = ps.width;
        xwc.height = ps.height;
        xwc.border_width = 0;
        XConfigureWindow(dpy, client->frame, mask, &xwc);
    }

#ifdef DEBUG
    printf("\tReparenting client 0x%08X (window 0x%08X) to 0x%08X\n",
           (unsigned int)client, (unsigned int)w, (unsigned int)client->frame);
#endif /* DEBUG */

    XClearWindow(dpy, client->frame);
    /* ignore the map and unmap events caused by the reparenting: */
    XSelectInput(dpy, w, client->window_event_mask & ~StructureNotifyMask);
    if (has_titlebar) {
        XReparentWindow(dpy, w, client->frame, 0, TITLE_HEIGHT);
    } else {
        XReparentWindow(dpy, w, client->frame, 0, 0);
    }
    XSelectInput(dpy, w, client->window_event_mask);
}

static void client_add_titlebar(client_t *client)
{
    XSetWindowAttributes xswa;
    int mask;
    
    mask = CWBackPixmap | CWBackPixel | CWCursor
           | CWEventMask | CWOverrideRedirect | CWWinGravity;
    xswa.cursor = cursor_normal;
    xswa.background_pixmap = None;
    xswa.background_pixel = black;
    xswa.event_mask = ExposureMask;
    xswa.override_redirect = True;
    xswa.win_gravity = NorthWestGravity;

    client->titlebar = XCreateWindow(dpy, client->frame, 0, 0, client->width,
                                     TITLE_HEIGHT, 0, DefaultDepth(dpy, scr),
                                     CopyFromParent, DefaultVisual(dpy, scr),
                                     mask, &xswa);
}

client_t *client_find(Window w)
{
    client_t *client;

    if (XFindContext(dpy, w, window_context, (void *)&client) != 0)
        if (XFindContext(dpy, w, frame_context, (void *)&client) != 0)
            if (XFindContext(dpy, w, title_context, (void *)&client) != 0)
                client = NULL;

#ifdef DEBUG
    if (client == NULL)
        printf("\tCould not find client\n");
    else if (client->window == w)
        printf("\tFound client from window\n");
    else if (client->titlebar == w)
        printf("\tFound client from titlebar\n");
    else
        printf("\tFound client from frame\n");
#endif /* DEBUG */
    
    return client;
}

void client_destroy(client_t *client)
{
    XDeleteContext(dpy, client->window, window_context);
    XDeleteContext(dpy, client->frame, frame_context);
    XUnmapWindow(dpy, client->frame);
    XDestroyWindow(dpy, client->frame);
    if (client->titlebar != None) {
        XDeleteContext(dpy, client->titlebar, title_context);
        XUnmapWindow(dpy, client->titlebar);
        XDestroyWindow(dpy, client->titlebar);
    }
    if (client->xwmh != NULL) XFree(client->xwmh);
    free(client->name);         /* should never be NULL */
    if (client->instance != NULL) XFree(client->instance);
    if (client->class != NULL) XFree(client->class);
    free(client);
}

/* snarfed mostly from ctwm and WindowMaker */
/* FIXME:  use XFetchName */
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
    printf("\tClient 0x%08X is %s\n", (unsigned int)client, client->name);
#endif /* DEBUG */
}

void client_set_instance_class(client_t *client)
{
    XClassHint xch;

    client->class = NULL;
    client->instance = NULL;

    if (XGetClassHint(dpy, client->window, &xch) != 0) {
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
    long set_fields;            /* ignored */

    client->xsh = XAllocSizeHints();
    if (client->xsh == NULL) {
        fprintf(stderr, "XWM: Couldn't allocate Size Hints structure\n");
        return;
    }
    if (XGetWMNormalHints(dpy, client->window,
                          client->xsh, &set_fields) == 0) {
        XFree(client->xsh);
        client->xsh = NULL;
    }
    /* WTF am I supposed to do with an increment of zero? */
    if (client->xsh != NULL && (client->xsh->flags & PResizeInc)) {
        if (client->xsh->height_inc <= 0)
            client->xsh->height_inc = 1;
        if (client->xsh->width_inc <= 0)
            client->xsh->width_inc = 1;
    }
}

void client_get_position_size_hints(client_t *client, position_size *ps)
{
    if (client->xsh != NULL) {
        if (client->xsh->flags & (PPosition | USPosition)) {
            ps->x = client->xsh->x;
            ps->y = client->xsh->y;
        }
        if (client->xsh->flags & (PSize | USSize)) {
            ps->width = client->xsh->width;
            ps->height = client->xsh->height;
        }
    }
}

void client_frame_position(client_t *client, position_size *ps)
{
    int gravity;
    int y_win_ref, y_frame_ref; /* "reference" points */

#ifdef DEBUG
    printf("\twindow wants to be positioned at %dx%d+%d+%d\n",
           ps->x, ps->y, ps->height, ps->width);
#endif /* DEBUG */

    if (client->titlebar == None) return;
    gravity = NorthWestGravity;
    
    if (client->xsh != NULL) {
        if (client->xsh->flags & PWinGravity)
            gravity = client->xsh->win_gravity;
    }

    /* this is a bit simpler since we only add a titlebar at the top */
    ps->height += TITLE_HEIGHT;
    switch (gravity) {
        case StaticGravity:
        case SouthEastGravity:
        case SouthGravity:
        case SouthWestGravity:
#ifdef DEBUG
            printf("\tGravity is like static\n");
#endif /* DEBUG */
            ps->y -= TITLE_HEIGHT;
            break;
        case CenterGravity:
        case EastGravity:
        case WestGravity:
            y_win_ref = ps->height / 2;
            y_frame_ref = (ps->height + TITLE_HEIGHT) / 2;
            ps->y += (y_frame_ref - y_win_ref);
#ifdef DEBUG
            printf("\tGravity is like center\n");
#endif /* DEBUG */
            break;
        case NorthGravity:
        case NorthEastGravity:
        case NorthWestGravity:
        default:                /* assume NorthWestGravity */
#ifdef DEBUG
            printf("\tGravity is like NorthWest\n");
#endif /* DEBUG */
            break;
    }
#ifdef DEBUG
    printf("\tframe positioned at %dx%d+%d+%d\n",
           ps->x, ps->y, ps->height, ps->width);
#endif /* DEBUG */
}

void client_print(char *s, client_t *client)
{
#ifndef DEBUG
    /* this may be in the middle of a tight loop where one cannot
     * afford even a null function call */
    fprintf(stderr, "XWM: Crazy author forgot to remove debugging output\n");
#endif
    if (client == NULL) {
        printf("%-19s null client\n", s);
        return;
    }
    printf("%-19s client = 0x%08X, window = 0x%08X, frame = 0x%08X\n",
           s, (unsigned int)client, (unsigned int)client->window,
           (unsigned int)client->frame);
    printf("%-19s name = %s, instance = %s, class = %s\n",
           s, client->name, client->instance, client->class);
}

void client_paint_titlebar(client_t *client)
{
    if (client->titlebar == None) return;
    XClearWindow(dpy, client->titlebar);
    XDrawString(dpy, client->titlebar, root_invert_gc, 2, TITLE_HEIGHT - 4,
                client->name, strlen(client->name));
}

/* ICCCM, 4.1.3.1 */
void client_inform_state(client_t *client)
{
    CARD32 data[2];

    data[0] = (CARD32)client->state;
    data[1] = (CARD32)None;
    XChangeProperty(dpy, client->window, WM_STATE, WM_STATE, 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

void client_set_protocols(client_t *client)
{
    Atom *atoms;
    int n, i;

    client->protocols = PROTO_NONE;
    if (XGetWMProtocols(dpy, client->window, &atoms, &n) == 0) {
        return;
    } else {
        for (i = 0; i < n; i++) {
            if (atoms[i] == WM_TAKE_FOCUS) {
                client->protocols |= PROTO_TAKE_FOCUS;
            } else if (atoms[i] == WM_SAVE_YOURSELF) {
                client->protocols |= PROTO_SAVE_YOURSELF;
            } else if (atoms[i] == WM_DELETE_WINDOW) {
                client->protocols |= PROTO_DELETE_WINDOW;
            }
        }
    }
    XFree(atoms);
}

void client_sendmessage(client_t *client, Atom data0, Time timestamp,
                        long data2, long data3, long data4)
{
    XClientMessageEvent xcme;
    
    xcme.type = ClientMessage;
    xcme.window = client->window;
    xcme.message_type = WM_PROTOCOLS;
    xcme.format = 32;
    xcme.data.l[0] = (long)data0;
    xcme.data.l[1] = (long)timestamp;
    xcme.data.l[2] = data2;
    xcme.data.l[3] = data3;
    xcme.data.l[4] = data4;
    XSendEvent(dpy, client->window, False, 0, (XEvent *)&xcme);
}
