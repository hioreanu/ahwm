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

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include "client.h"
#include "workspace.h"
#include "keyboard.h"
#include "mouse.h"
#include "cursor.h"
#include "focus.h"
#include "event.h"
#include "malloc.h"
#include "debug.h"

XContext window_context;
XContext frame_context;
XContext title_context;

/* we only keep around clients as a list to allow iteration over all clients */
client_t *client_list = NULL;

client_t *client_create(Window w)
{
    client_t *client;
    XWindowAttributes xwa;
    Bool has_titlebar = True;
    position_size requested_geometry;
    int shaped = 0;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return NULL;
    if (xwa.override_redirect) {
        debug(("\tWindow has override_redirect, not creating client\n"));
        /* FIXME:  may still need to reparent to fake root */
        return NULL;
    }

    client = Malloc(sizeof(client_t));
    if (client == NULL) {
        fprintf(stderr, "XWM: Malloc failed, unable to allocate client\n");
        return NULL;
    }
    memset(client, 0, sizeof(client_t));
    
    client->window = w;
    client->transient_for = None;
    client->group_leader = NULL;
    client->workspace = workspace_current;
    client->state = WithdrawnState;
    client->frame = None;
    client->titlebar = None;
    client->prev_x = client->prev_y = -1;
    client->prev_height = client->prev_width = -1;
    client->ignore_enternotify = 0;
    client->orig_border_width = xwa.border_width;
    XSetWindowBorderWidth(dpy, w, 0);

    client_set_name(client);
    client_set_instance_class(client);
    client_set_xwmh(client);
    client_set_xsh(client);
    client_set_protocols(client);
    client_set_transient_for(client);

    /* see if in window group (ICCCM, 4.1.11) */
    if (client->xwmh != NULL
        && client->xwmh->flags & WindowGroupHint) {
        client->group_leader = client_find((Window)client->xwmh->window_group);
        if (client->group_leader != NULL
            && client->group_leader->xwmh != NULL) {
            XFree(client->xwmh);
            client->xwmh = client->group_leader->xwmh;
        }
    }

    client->frame_event_mask = SubstructureRedirectMask | EnterWindowMask
                              | FocusChangeMask | StructureNotifyMask;
    client->window_event_mask = xwa.your_event_mask | StructureNotifyMask
                                | PropertyChangeMask;
    XSelectInput(dpy, client->window, client->window_event_mask);

#ifdef SHAPE
    /* if we have a shaped window, don't add a titlebar */
    if (shape_supported) {
        int tmp;
        unsigned int tmp2;

        XShapeSelectInput(dpy, client->window, ShapeNotifyMask);
        XShapeQueryExtents(dpy, client->window, &shaped, &tmp, &tmp,
                           &tmp2, &tmp2, &tmp, &tmp, &tmp, &tmp2, &tmp2);
        has_titlebar = !shaped;
        if (shaped) debug(("\tSHAPED\n"));
        else debug(("\tNOT SHAPED\n"));
    }
#endif /* SHAPE */

    requested_geometry.x = xwa.x;
    requested_geometry.y = xwa.y;
    requested_geometry.width = xwa.width;
    requested_geometry.height = xwa.height;
    client_reparent(client, has_titlebar, &requested_geometry);
    if (client->frame == None) {
        fprintf(stderr, "XWM: Could not reparent window\n");
        Free(client);
        return NULL;
    }
    if (has_titlebar) client_add_titlebar(client);

    if (XSaveContext(dpy, w, window_context, (void *)client) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save window\n");
        Free(client);
        return NULL;
    }

#ifdef SHAPE
    if (shaped && shape_supported) {
        XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                           client->window, ShapeBounding, ShapeSet);

    }
#endif /* SHAPE */

    client->next = client_list;
    client->prev = NULL;
    if (client_list != NULL) client_list->prev = client;
    client_list = client;

    if (xwa.map_state != IsUnmapped) {
        debug(("\tclient_create:  client is already mapped\n"));
        client->state = NormalState;
        client_inform_state(client);
        XMapWindow(dpy, client->frame);
        if (client->titlebar != None)
            XMapWindow(dpy, client->titlebar);
        keyboard_grab_keys(client);
        mouse_grab_buttons(client);
        focus_add(client, event_timestamp);
    }
    
    return client;
}

void client_reparent(client_t *client, Bool has_titlebar,
                     position_size *win_position)
{
    XSetWindowAttributes xswa;
    XWindowChanges xwc;
    int mask;
    Window w;
    position_size ps;

    w = client->window;
    mask = CWBackPixmap | CWBackPixel | CWCursor
           | CWEventMask | CWOverrideRedirect;
    xswa.cursor = cursor_normal;
    xswa.background_pixmap = None;
    xswa.background_pixel = black;
    xswa.event_mask = client->frame_event_mask;
    xswa.override_redirect = True;

    client_get_position_size_hints(client, &ps);
    /* client_frame_position checks for the titlebar which
     * has not yet been created - I'd rather not change the api */
    memcpy(&ps, win_position, sizeof(position_size));
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

    XClearWindow(dpy, client->frame);
    /* ignore the map and unmap events caused by the reparenting: */
    XSelectInput(dpy, w, client->window_event_mask & ~StructureNotifyMask);
    if (has_titlebar) {
        XReparentWindow(dpy, w, client->frame, 0, TITLE_HEIGHT);
    } else {
        XReparentWindow(dpy, w, client->frame, 0, 0);
    }
    XSelectInput(dpy, w, client->window_event_mask);

    if (XSaveContext(dpy, client->frame,
                     frame_context, (void *)client) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save frame\n");
    }
}

void client_add_titlebar(client_t *client)
{
    XSetWindowAttributes xswa;
    int mask;
    
    mask = CWBackPixmap | CWBackPixel | CWCursor | CWEventMask
           | CWOverrideRedirect | CWWinGravity;
    xswa.cursor = cursor_normal;
    xswa.background_pixmap = None;
    xswa.background_pixel = workspace_darkest_highlight[client->workspace - 1];
    xswa.event_mask = ExposureMask;
    xswa.override_redirect = True;
    xswa.win_gravity = NorthWestGravity;

    client->titlebar = XCreateWindow(dpy, client->frame, 0, 0, client->width,
                                     TITLE_HEIGHT, 0, DefaultDepth(dpy, scr),
                                     CopyFromParent, DefaultVisual(dpy, scr),
                                     mask, &xswa);
    if (client->titlebar != None) {
        if (XSaveContext(dpy, client->titlebar,
                         title_context, (void *)client) != 0) {
            fprintf(stderr,
                    "XWM: XSaveContext failed, could not save titlebar\n");
        }
    }
}

void client_remove_titlebar(client_t *client)
{
    position_size ps;
    
    if (client->titlebar == None)
        return;
    client_position_noframe(client, &ps);
    XUnmapWindow(dpy, client->titlebar);
    XDestroyWindow(dpy, client->titlebar);
    XDeleteContext(dpy, client->titlebar, title_context);
    client->titlebar = None;
    XReparentWindow(dpy, client->window, root_window, ps.x, ps.y);
    XUnmapWindow(dpy, client->frame);
    XDestroyWindow(dpy, client->frame);
    XDeleteContext(dpy, client->frame, frame_context);
    client->frame = None;
    client->x = ps.x;
    client->y = ps.y;
    client->width = ps.width;
    client->height = ps.height;
    client_reparent(client, False, &ps);
}

client_t *client_find(Window w)
{
    client_t *client;

    if (XFindContext(dpy, w, window_context, (void *)&client) != 0)
        if (XFindContext(dpy, w, frame_context, (void *)&client) != 0)
            if (XFindContext(dpy, w, title_context, (void *)&client) != 0)
                client = NULL;

    if (client == NULL)
        debug(("\tCould not find client\n"));
    else if (client->window == w)
        debug(("\tFound client from window\n"));
    else if (client->titlebar == w)
        debug(("\tFound client from titlebar\n"));
    else
        debug(("\tFound client from frame\n"));
    
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
    if (client->xwmh != NULL
        && client->group_leader == NULL)
        XFree(client->xwmh);
    Free(client->name);         /* should never be NULL */
    if (client->instance != NULL) XFree(client->instance);
    if (client->class != NULL) XFree(client->class);
    
    if (client->next != NULL) client->next->prev = client->prev;
    if (client->prev != NULL) client->prev->next = client->next;
    if (client_list == client) client_list = client->next;
    Free(client);
}

int client_foreach(client_foreach_function fn, void *arg)
{
    client_t *c;

    for (c = client_list; c != NULL; c = c->next) {
        if ((*fn)(c, arg) == 0) return 0;
    }
    return 1;
}

/* snarfed mostly from ctwm and WindowMaker */
/* FIXME:  use XFetchName */
void client_set_name(client_t *client)
{
    XTextProperty xtp;
    char **list;
    int n;

    if (XGetWMName(dpy, client->window, &xtp) == 0) {
        client->name = Strdup("");      /* client did not set a window name */
        return;
    }
    if (xtp.value == NULL || xtp.nitems <= 0) {
        /* client set window name to NULL */
        client->name = Strdup("");
    } else {
        if (xtp.encoding == XA_STRING) {
            /* usual case */
            client->name = Strdup(xtp.value);
        } else {
            /* client is using UTF-16 or something equally stupid */
            /* haven't seen this block actually run yet */
            xtp.nitems = strlen((char *)xtp.value);
            if (XmbTextPropertyToTextList(dpy, &xtp, &list, &n) == Success
                && n > 0 && *list != NULL) {
                client->name = Strdup(*list);
                XFreeStringList(list);
            } else {
                client->name = Strdup("");
            }
        }
    }
    if (xtp.value != NULL) XFree(xtp.value);

    debug(("\tClient 0x%08X is %s\n", (unsigned int)client, client->name));
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

void client_set_transient_for(client_t *client)
{
    Window w;

    if (XGetTransientForHint(dpy, client->window, &w))
        client->transient_for = w;
    else
        client->transient_for = None;
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

/*
 * I think this is the only window manager I've seen that
 * actually follows ICCCM 4.1.2.3 to the letter, specifically
 * with EastGravity and WestGravity.
 */

void client_frame_position(client_t *client, position_size *ps)
{
    int gravity;
    int y_win_ref, y_frame_ref; /* "reference" points */

    debug(("\twindow wants to be positioned at %dx%d+%d+%d\n",
           ps->width, ps->height, ps->x, ps->y));

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
            debug(("\tGravity is like static\n"));
            ps->y -= TITLE_HEIGHT;
            break;
        case CenterGravity:
        case EastGravity:
        case WestGravity:
            y_win_ref = (ps->height - TITLE_HEIGHT) / 2;
            y_frame_ref = ps->height / 2;
            ps->y -= (y_frame_ref - y_win_ref);
            debug(("\tGravity is like center\n"));
            break;
        case NorthGravity:
        case NorthEastGravity:
        case NorthWestGravity:
        default:                /* assume NorthWestGravity */
            debug(("\tGravity is like NorthWest\n"));
            break;
    }
    debug(("\tframe positioned at %dx%d+%d+%d\n",
           ps->width, ps->height, ps->x, ps->y));
}

void client_position_noframe(client_t *client, position_size *ps)
{
    int gravity;
    int y_win_ref, y_frame_ref;

    ps->x = client->x;
    ps->y = client->y;
    ps->width = client->width;
    ps->height = client->height;
    
    if (client->titlebar == None) return;
    gravity = NorthWestGravity;
    
    if (client->xsh != NULL) {
        if (client->xsh->flags & PWinGravity)
            gravity = client->xsh->win_gravity;
    }

    ps->height -= TITLE_HEIGHT;
    switch (gravity) {
        case StaticGravity:
        case SouthEastGravity:
        case SouthGravity:
        case SouthWestGravity:
            ps->y += TITLE_HEIGHT;
            break;
        case CenterGravity:
        case EastGravity:
        case WestGravity:
            y_win_ref = ps->height / 2;
            y_frame_ref = (ps->height + TITLE_HEIGHT) / 2;
            ps->y += (y_frame_ref - y_win_ref);
            break;
        case NorthGravity:
        case NorthEastGravity:
        case NorthWestGravity:
        default:                /* assume NorthWestGravity */
            break;
    }
}

void _client_print(char *s, client_t *client)
{
    if (client == NULL) {
        debug(("%-19s null client\n", s));
        return;
    }
    debug(("%-19s client = 0x%08X, window = 0x%08X, frame = 0x%08X\n",
           s, (unsigned int)client, (unsigned int)client->window,
           (unsigned int)client->frame));
    debug(("%-19s name = %s, instance = %s, class = %s\n",
           s, client->name, client->instance, client->class));
}

void client_paint_titlebar(client_t *client)
{
    XGCValues xgcv;

    if (client == NULL || client->titlebar == None) return;
    
    XClearWindow(dpy, client->titlebar);
    if (client == focus_current) {
        xgcv.foreground = workspace_pixels[client->workspace - 1];
        XChangeGC(dpy, extra_gc, GCForeground, &xgcv);
        XFillRectangle(dpy, client->titlebar, extra_gc,
                       0, 0, client->width, TITLE_HEIGHT);
        
        xgcv.foreground = workspace_highlight[client->workspace - 1];
        XChangeGC(dpy, extra_gc, GCForeground, &xgcv);
        XDrawLine(dpy, client->titlebar, extra_gc, 0, 0,
                  client->width, 0);
        XDrawLine(dpy, client->titlebar, extra_gc, 1, 1,
                  client->width - 2, 1);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  0, 0, 0, TITLE_HEIGHT);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  1, 1, 1, TITLE_HEIGHT - 2);

        xgcv.foreground = workspace_dark_highlight[client->workspace - 1];
        XChangeGC(dpy, extra_gc, GCForeground, &xgcv);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  1, TITLE_HEIGHT - 1, client->width - 1, TITLE_HEIGHT - 1);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  2, TITLE_HEIGHT - 2, client->width - 3, TITLE_HEIGHT - 2);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  client->width - 1, 1, client->width - 1, TITLE_HEIGHT - 2);
        XDrawLine(dpy, client->titlebar, extra_gc,
                  client->width - 2, 2, client->width - 2, TITLE_HEIGHT - 2);
    }
    /* we don't draw a border for non-focused windows as that would
     * require two additional colors (which is bloat) */
    
    XDrawString(dpy, client->titlebar, root_white_fg_gc, 2, TITLE_HEIGHT - 4,
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
    if (atoms != NULL) XFree(atoms);
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

static int raise_transients(client_t *client, void *v)
{
    Window w = (Window)v;

    /* might be in different workspace, but oh well */
    if (client->transient_for == w && client->state == NormalState)
        XMapRaised(dpy, client->frame);
    return 1;
}

void client_raise(client_t *client)
{
    XMapRaised(dpy, client->frame);
    client_foreach(raise_transients, (void *)client->window);
}
