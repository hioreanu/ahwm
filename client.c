/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "config.h"

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
#include "keyboard-mouse.h"
#include "cursor.h"
#include "focus.h"
#include "event.h"
#include "malloc.h"
#include "debug.h"
#include "ewmh.h"
#include "move-resize.h"
#include "stacking.h"

XContext window_context;
XContext frame_context;
XContext title_context;

static void client_create_frame(client_t *client, position_size *win_position);
static void remove_transient_from_leader(client_t *client);

client_t *client_create(Window w)
{
    client_t *client;
    XWindowAttributes xwa;
    position_size requested_geometry;
    int shaped = 0;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return NULL;
    if (xwa.override_redirect) {
        debug(("\tWindow has override_redirect, not creating client\n"));
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
    client->workspace = 0;
    client->state = WithdrawnState;
    client->frame = None;
    client->titlebar = None;
    client->prev_x = client->prev_y = -1;
    client->prev_height = client->prev_width = -1;
    client->orig_border_width = xwa.border_width;
    client->reparented = 0;
    client->ignore_unmapnotify = 0;
    client->pass_focus_click = 1;
    client->focus_policy = SloppyFocus;
    
    /* God, this sucks.  I want the border width to be zero on all
     * clients, so I need to change the client's border width at some
     * point.  Apparently, I can't do that here, because xterm listens
     * for ConfigureNotify events, and this generates a
     * ConfigureNotify event.  The problem is that if xterm receives
     * this ConfigureNotify event it will segfault (xterm bug, not
     * really my bug).  It took me three days of combing through the
     * xterm source code to figure this out - it only manifests itself
     * when xterm is started with a scroll bar 'xterm -sb', which I
     * never use.  Anyway, reading through xterm source code sucks. */

/*    XSetWindowBorderWidth(dpy, w, 0); */

#ifdef SHAPE
    /* if we have a shaped window, don't add a titlebar */
    if (shape_supported) {
        int tmp;
        unsigned int tmp2;

        XShapeSelectInput(dpy, client->window, ShapeNotifyMask);
        XShapeQueryExtents(dpy, client->window, &shaped, &tmp, &tmp,
                           &tmp2, &tmp2, &tmp, &tmp, &tmp, &tmp2, &tmp2);
        client->has_titlebar = (shaped ? 0 : 1);
        if (shaped) debug(("\tIs a shaped window\n"));
        else debug(("\tnot shaped\n"));
    }
#endif /* SHAPE */

    client_set_name(client);
    client_set_instance_class(client);
    client_set_xwmh(client);
    client_set_xsh(client);
    client_set_protocols(client);

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
        | LeaveWindowMask | FocusChangeMask | StructureNotifyMask;
    client->window_event_mask = xwa.your_event_mask | StructureNotifyMask
        | PropertyChangeMask;
    XSelectInput(dpy, client->window, client->window_event_mask);

    requested_geometry.x = xwa.x;
    requested_geometry.y = xwa.y;
    requested_geometry.width = xwa.width;
    requested_geometry.height = xwa.height;
    client_create_frame(client, &requested_geometry);
    if (client->frame == None) {
        fprintf(stderr, "XWM: Could not create frame\n");
        Free(client);
        return NULL;
    }
    if (client->has_titlebar) client_add_titlebar(client);

    if (XSaveContext(dpy, w, window_context, (void *)client) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save window\n");
        Free(client);
        return NULL;
    }
    
    client_set_transient_for(client);

#ifdef SHAPE
    if (shaped && shape_supported) {
        XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                           client->window, ShapeBounding, ShapeSet);

    }
#endif /* SHAPE */

    stacking_add(client);
    
    if (xwa.map_state != IsUnmapped) {
        /* The only time when this can happen is when we just started
         * the windowmanager and there are already some windows on
         * the screen.  FIXME:  we should check the window's properties
         * (like workspace, etc) to see if a previous windowmanager was
         * doing something interesting with the window */
        debug(("\tclient_create:  client is already mapped\n"));
        client->ignore_unmapnotify = 1;
        client->state = NormalState;
        client_inform_state(client);
        client_reparent(client);
        stacking_raise(client);
        XMapWindow(dpy, client->frame);
        if (client->titlebar != None)
            XMapWindow(dpy, client->titlebar);
        keyboard_grab_keys(client->frame);
        mouse_grab_buttons(client);
        focus_add(client, event_timestamp);
        ewmh_client_list_add(client);
    }
    
    return client;
}

static void client_create_frame(client_t *client, position_size *win_position)
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
    memcpy(&ps, win_position, sizeof(position_size));
    client_frame_position(client, &ps);
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

        mask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
        xwc.x = ps.x;
        xwc.y = ps.y;
        xwc.width = ps.width;
        xwc.height = ps.height;
        xwc.border_width = 0;
        XConfigureWindow(dpy, client->frame, mask, &xwc);
    }

    XClearWindow(dpy, client->frame); /* FIXME:  ??? */

    if (XSaveContext(dpy, client->frame,
                     frame_context, (void *)client) != 0) {
        fprintf(stderr, "XWM: XSaveContext failed, could not save frame\n");
    }
}

void client_reparent(client_t *client)
{
    /* reparent the window and map window and frame */
    debug(("\tReparenting window %#lx ('%.10s')\n",
           client->window, client->name));
    XAddToSaveSet(dpy, client->window);
    XSetWindowBorderWidth(dpy, client->window, 0);
    if (client->titlebar != None) {
        XReparentWindow(dpy, client->window,
                        client->frame, 0, TITLE_HEIGHT);
    } else {
        XReparentWindow(dpy, client->window, client->frame, 0, 0);
    }
    XMapWindow(dpy, client->window);
    client->reparented = 1;
}

void client_unreparent(client_t *client)
{
    client->reparented = 0;
    XRemoveFromSaveSet(dpy, client->window);
    XReparentWindow(dpy, client->window, root_window, client->x, client->y);
    XSetWindowBorderWidth(dpy, client->window, client->orig_border_width);
}

void client_add_titlebar(client_t *client)
{
    XSetWindowAttributes xswa;
    int mask;

    debug(("\tAdding titlebar\n"));
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
    Bool reparented;
    Window junk1, *junk2;
    int n;

    client->has_titlebar = 0;
    if (client->titlebar == None)
        return;
    
    debug(("\tRemoving titlebar\n"));

    if (XQueryTree(dpy, client->frame, &junk1, &junk1, &junk2, &n) == 0) {
        reparented = False;
    } else if (n == 2) {
        reparented = True;
    } else {
        reparented = False;
    }
    
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
    client_create_frame(client, &ps);
    if (reparented) client_reparent(client);
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
    client_t *c, *tmp;

    stacking_remove(client);
    /* apparently we need this here */
    remove_transient_from_leader(client);
    for (c = client->transients; c != NULL; c = tmp) {
        tmp = c->next_transient;
        c->next_transient = NULL;
        c->transient_for = None;
    }
    
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

    Free(client);
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

    debug(("\tClient %#lx is %s\n", (unsigned int)client, client->name));
    if (strcmp(client->name, "kicker") == 0
        || strcmp(client->name, "lt-kicker") == 0) {
        client->focus_policy = DontFocus;
        client->skip_alt_tab = 1;
        client->always_on_top = 1;
        client->sticky = 1;
        client->omnipresent = 1;
        client_remove_titlebar(client);
    }
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

static void remove_transient_from_leader(client_t *client)
{
    client_t *leader, *c, *tmp;
    
    if (client->transient_for != None
        && ( (leader = client_find(client->transient_for)) != NULL)) {

        c = leader->transients;
        tmp = NULL;
        while (c != NULL) {
            if (c == client) {
                if (tmp == NULL) {
                    leader->transients = NULL;
                } else {
                    tmp->next_transient = c->next_transient;
                }
                break;
            }
            tmp = c;
            c = c->next_transient;
        }
    }
}

/*
 * transient for root or nothing means transient for group leader
 */

void client_set_transient_for(client_t *client)
{
    Window new_transient_for;
    client_t *leader, *c;

    if (XGetTransientForHint(dpy, client->window, &new_transient_for) == 0)
        new_transient_for = None;

    if (new_transient_for == None || new_transient_for == root_window) {
        if (client->group_leader == NULL) {
            remove_transient_from_leader(client);
            client->transient_for = None;
            return;
        } else {
            client->transient_for = client->group_leader->window;
        }
    }
        
    if (client->transient_for == new_transient_for) return;

    c = leader = client_find(new_transient_for);
    while (c != NULL) {
        if (c == client) {
            fprintf(stderr, "XWM: The application '%s' has a "
                    "TRANSIENT_FOR cycle.\n"
                    "This is a serious bug, and you should contact "
                    "the author of this program.\n", client->name);
            client->transient_for = None;
            return;
        }
        c = client_find(c->transient_for);
    }
    
    /* remove client from old leader's 'transients' list if needed */
    remove_transient_from_leader(client);
    
    /* add client to new leader's 'transients' list if possible; if
     * that's not possible, no big deal, just means client's
     * WM_TRANSIENT_FOR points to yet-uncreated window or the root
     * window or something (in which case we won't even treat it as a
     * transient) */
    
    client->transient_for = new_transient_for;
    if (leader != NULL) {
        client->next_transient = leader->transients;
        leader->transients = client;
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

/*
 * I think this is the only window manager I've seen that
 * actually follows ICCCM 4.1.2.3 to the letter, specifically
 * with EastGravity and WestGravity.
 */

void client_frame_position(client_t *client, position_size *ps)
{
    static char *gravity_strings[] = {
        "ForgetGravity",
        "NorthWestGravity",
        "NorthGravity",
        "NorthEastGravity",
        "WestGravity",
        "CenterGravity",
        "EastGravity",
        "SouthWestGravity",
        "SouthGravity",
        "SouthEastGravity",
        "StaticGravity"
    };
    int gravity;
    int y_win_ref, y_frame_ref; /* "reference" points */

    debug(("\twindow wants to be positioned at %dx%d+%d+%d\n",
           ps->width, ps->height, ps->x, ps->y));

    if (!client->has_titlebar) return;
    gravity = NorthWestGravity;
    
    if (client->xsh != NULL) {
        if (client->xsh->flags & PWinGravity)
            gravity = client->xsh->win_gravity;
    }

    debug(("\tGravity is %s\n", gravity_strings[gravity]));
    
    /* this is a bit simpler since we only add a titlebar at the top */
    ps->height += TITLE_HEIGHT;
    switch (gravity) {
        case StaticGravity:
        case SouthEastGravity:
        case SouthGravity:
        case SouthWestGravity:
            ps->y -= TITLE_HEIGHT;
            break;
        case CenterGravity:
        case EastGravity:
        case WestGravity:
            y_win_ref = (ps->height - TITLE_HEIGHT) / 2;
            y_frame_ref = ps->height / 2;
            ps->y -= (y_frame_ref - y_win_ref);
            break;
        case NorthGravity:
        case NorthEastGravity:
        case NorthWestGravity:
        default:                /* assume NorthWestGravity */
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
    debug(("%-19s client = %#lx, window = %#lx, frame = %#lx\n",
           s, (unsigned int)client, (unsigned int)client->window,
           (unsigned int)client->frame));
    debug(("%-19s name = '%.10s', instance = '%.10s', class = '%.10s'\n",
           s, client->name,
           client->instance == NULL ? "NULL" : client->instance,
           client->class == NULL ? "NULL" : client->class));
}

void client_paint_titlebar(client_t *client)
{
    XGCValues xgcv;

    if (client == NULL || client->titlebar == None) return;
    
    if (client == focus_current && client->focus_policy != DontFocus) {
        /* using three different GCs instead of using one and
         * continually changing its values may or may not be faster
         * (depending on hardware) according to the Xlib docs, but
         * this seems to reduce flicker when moving windows on my
         * hardware */
        xgcv.foreground = workspace_pixels[client->workspace - 1];
        XChangeGC(dpy, extra_gc1, GCForeground, &xgcv);
        xgcv.foreground = workspace_highlight[client->workspace - 1];
        XChangeGC(dpy, extra_gc2, GCForeground, &xgcv);
        xgcv.foreground = workspace_dark_highlight[client->workspace - 1];
        XChangeGC(dpy, extra_gc3, GCForeground, &xgcv);
        
        XFillRectangle(dpy, client->titlebar, extra_gc1,
                       0, 0, client->width, TITLE_HEIGHT);
        
        XDrawLine(dpy, client->titlebar, extra_gc2, 0, 0,
                  client->width, 0);
        XDrawLine(dpy, client->titlebar, extra_gc2, 1, 1,
                  client->width - 2, 1);
        XDrawLine(dpy, client->titlebar, extra_gc2,
                  0, 0, 0, TITLE_HEIGHT);
        XDrawLine(dpy, client->titlebar, extra_gc2,
                  1, 1, 1, TITLE_HEIGHT - 2);

        XDrawLine(dpy, client->titlebar, extra_gc3,
                  1, TITLE_HEIGHT - 1, client->width - 1, TITLE_HEIGHT - 1);
        XDrawLine(dpy, client->titlebar, extra_gc3,
                  2, TITLE_HEIGHT - 2, client->width - 3, TITLE_HEIGHT - 2);
        XDrawLine(dpy, client->titlebar, extra_gc3,
                  client->width - 1, 1, client->width - 1, TITLE_HEIGHT - 2);
        XDrawLine(dpy, client->titlebar, extra_gc3,
                  client->width - 2, 2, client->width - 2, TITLE_HEIGHT - 2);
    } else {
        /* we don't draw a border for non-focused windows as that would
         * require two additional colors (which is bloat) */
        XClearWindow(dpy, client->titlebar);
    }
    
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
