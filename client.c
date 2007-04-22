/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xresource.h>      /* needed for XUniqueContext in Xutil.h */

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
#include "prefs.h"
#include "mwm.h"
#include "colormap.h"
#include "compat.h"

int TITLE_HEIGHT = 15;

/*
 * we store the data associated with each window using Xlib's XContext
 * mechanism (which has nothing to do with X itself, it's just a hash
 * mechanism built into Xlib as far as I can tell).
 * window_context associates clients with their main windows
 * frame_context associates clients with their frame windows
 * title_context associates clients with their titlebar windows
 */

XContext window_context;
XContext frame_context;
XContext title_context;

static void client_create_frame(client_t *client, position_size *win_position);
static void remove_transient_from_leader(client_t *client);
static void client_add_titlebar_internal(client_t *client);
static void update_move_offset(client_t *client);
    
void client_init()
{
    window_context = XUniqueContext();
    frame_context = XUniqueContext();
    title_context = XUniqueContext();
}

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
        fprintf(stderr, "AHWM: Malloc failed, unable to allocate client\n");
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
    client->stacking = -1;
    client->reparented = 0;
    client->ignore_unmapnotify = 0;
    client->color_index = 0;
    client->colormap = xwa.colormap;
    client->colormap_windows = NULL;
    client->ncolormap_windows = 0;
    client->pass_focus_click = 1;
    client->focus_policy = SloppyFocus;
    client->cycle_behaviour = RaiseImmediately;
    client->sticky = 0;
    client->dont_bind_keys = 0;
    client->dont_bind_mouse = 0;
    client->keep_transients_on_top = 1;
    client->raise_delay = 0;
    client->use_net_wm_pid = 0;
    client->shaded = 0;

    client->workspace_set = UnSet;
    client->focus_policy_set = UnSet;
    client->map_policy_set = UnSet;
    client->cycle_behaviour_set = UnSet;
    client->has_titlebar_set = UnSet;
    client->is_shaped_set = UnSet;
    client->pass_focus_click_set = UnSet;
    client->always_on_top_set = UnSet;
    client->always_on_bottom_set = UnSet;
    client->omnipresent_set = UnSet;
    client->sticky_set = UnSet;
    client->dont_bind_keys_set = UnSet;
    client->dont_bind_mouse_set = UnSet;
    client->keep_transients_on_top_set = UnSet;
    client->raise_delay_set = UnSet;
    client->use_net_wm_pid_set = UnSet;
    
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
    /* see if window is shaped */
    if (shape_supported) {
        int tmp;
        unsigned int tmp2;

        XShapeSelectInput(dpy, client->window, ShapeNotifyMask);
        XShapeQueryExtents(dpy, client->window, &shaped, &tmp, &tmp,
                           &tmp2, &tmp2, &tmp, &tmp, &tmp, &tmp2, &tmp2);
        client->is_shaped = (shaped ? 1 : 0);
        if (shaped) debug(("\tIs a shaped window\n"));
        else debug(("\tnot shaped\n"));
    }
#endif /* SHAPE */

    client_set_name(client);
    client_set_instance_class(client);
    client_set_xwmh(client);
    client_set_xsh(client);
    client_set_protocols(client);
    colormap_update_windows_property(client);

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

    prefs_apply(client);
    mwm_apply(client);
    ewmh_wm_state_apply(client);
    ewmh_wm_strut_apply(client);
    ewmh_window_type_apply(client);
    ewmh_wm_desktop_apply(client);
    
    XSelectInput(dpy, client->window,
                 xwa.your_event_mask
                 | StructureNotifyMask
                 | PropertyChangeMask
                 | ColormapChangeMask);

    requested_geometry.x = xwa.x;
    requested_geometry.y = xwa.y;
    requested_geometry.width = xwa.width;
    requested_geometry.height = xwa.height;
    client_create_frame(client, &requested_geometry);
    if (client->frame == None) {
        fprintf(stderr, "AHWM: Could not create frame\n");
        Free(client);
        return NULL;
    }
    if (client->has_titlebar) client_add_titlebar_internal(client);

    if (XSaveContext(dpy, w, window_context, (void *)client) != 0) {
        fprintf(stderr, "AHWM: XSaveContext failed, could not save window\n");
        Free(client);
        return NULL;
    }
    
    client_set_transient_for(client);

#ifdef SHAPE
    if (shaped && shape_supported) {
        if (client->has_titlebar) {
            XShapeCombineShape(dpy, client->frame, ShapeBounding, 0,
                               TITLE_HEIGHT, client->window, ShapeBounding,
                               ShapeSet);
            XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                               client->titlebar, ShapeBounding, ShapeUnion);
        } else {
            XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                               client->window, ShapeBounding, ShapeSet);
        }
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
        if (client->workspace == 0)
            client->workspace = workspace_current;
        ewmh_desktop_update(client);
        if (client->workspace == workspace_current)
            XMapWindow(dpy, client->frame);
        if (client->titlebar != None)
            XMapWindow(dpy, client->titlebar);
        if (client->dont_bind_keys == 0)
            keyboard_grab_keys(client->frame);
        mouse_grab_buttons(client);
        focus_add(client, event_timestamp);
        client_inform_state(client);
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
    xswa.override_redirect = True;
    xswa.event_mask =  SubstructureRedirectMask | EnterWindowMask
        | LeaveWindowMask | FocusChangeMask | StructureNotifyMask;

    /* client_get_position_size_hints(client, &ps); */
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
        debug(("\tClient %#lx now has frame %#lx\n", client, client->frame));
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

    /* XClearWindow(dpy, client->frame); */ /* FIXME:  ??? */

    if (XSaveContext(dpy, client->frame,
                     frame_context, (void *)client) != 0) {
        fprintf(stderr, "AHWM: XSaveContext failed, could not save frame\n");
    }
}

static void update_move_offset(client_t *client)
{
    position_size ps;
    int offset;

    client_position_noframe(client, &ps);
    if (client->titlebar == None) {
        offset = 0;
    } else {
        offset = ps.y + TITLE_HEIGHT - client->y;
    }
    XChangeProperty(dpy, client->window, _AHWM_MOVE_OFFSET, XA_INTEGER,
                    32, PropModeReplace, (unsigned char *)&offset, 1);
}

void client_reparent(client_t *client)
{
    /* reparent the window and map window and frame */
    debug(("\tReparenting %s\n", client_dbg(client)));
    XAddToSaveSet(dpy, client->window);
    XSetWindowBorderWidth(dpy, client->window, 0);
    if (client->titlebar != None) {
        XReparentWindow(dpy, client->window,
                        client->frame, 0, TITLE_HEIGHT);
        update_move_offset(client);
    } else {
        XReparentWindow(dpy, client->window, client->frame, 0, 0);
        XDeleteProperty(dpy, client->window, _AHWM_MOVE_OFFSET);
    }
    /* XMapWindow(dpy, client->window); */ /* FIXME: remove, b0rked */
    client->reparented = 1;
}

void client_unreparent(client_t *client)
{
    client->reparented = 0;
    XRemoveFromSaveSet(dpy, client->window);
    XReparentWindow(dpy, client->window, root_window, client->x, client->y);
    XSetWindowBorderWidth(dpy, client->window, client->orig_border_width);
}

static void client_add_titlebar_internal(client_t *client)
{
    XSetWindowAttributes xswa;
    int mask;

    if (client->titlebar != None) {
        debug(("\tClient already has titlebar, not touching it"));
        return;
    }
    debug(("\tAdding titlebar\n"));
    mask = CWBackPixmap | CWBackPixel | CWCursor | CWEventMask
           | CWOverrideRedirect | CWWinGravity;
    xswa.cursor = cursor_normal;
    xswa.background_pixmap = None;
    xswa.background_pixel = black;
    xswa.event_mask = ExposureMask;
    xswa.override_redirect = True;
    xswa.win_gravity = NorthWestGravity;

    debug(("\tClient width is %d\n", client->width));
    
    client->titlebar = XCreateWindow(dpy, client->frame, 0, 0, client->width,
                                     TITLE_HEIGHT, 0, DefaultDepth(dpy, scr),
                                     CopyFromParent, DefaultVisual(dpy, scr),
                                     mask, &xswa);
    /* raise title above client window so shading works properly */
    XRaiseWindow(dpy, client->titlebar);
    if (client->titlebar != None) {
        if (XSaveContext(dpy, client->titlebar,
                         title_context, (void *)client) != 0) {
            fprintf(stderr,
                    "AHWM: XSaveContext failed, could not save titlebar\n");
        }
    }

    client->has_titlebar = 1;

#ifdef SHAPE
    if (shape_supported && client->is_shaped) {
        XShapeCombineShape(dpy, client->frame, ShapeBounding, 0,
                           TITLE_HEIGHT, client->window,
                           ShapeBounding, ShapeSet);
        XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                           client->titlebar, ShapeBounding, ShapeUnion);
    }
#endif
}

void client_add_titlebar(client_t *client)
{
    position_size ps;
    
    client_add_titlebar_internal(client);
    ps.x = client->x;
    ps.y = client->y;
    ps.width = client->width;
    ps.height = client->height;
    /* client_get_position_size_hints(client, &ps); */
    client_create_frame(client, &ps);
    XMoveWindow(dpy, client->window, 0, TITLE_HEIGHT);
    update_move_offset(client);
    XMapWindow(dpy, client->titlebar);
    mouse_grab_buttons(client);
}

void client_remove_titlebar(client_t *client)
{
    position_size ps;
    Bool reparented;
    Window junk1, *junk2;
    unsigned int n;

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

    client_create_frame(client, &ps); /* just resets frame's position */
    XMoveWindow(dpy, client->window, 0, 0);
    XDeleteProperty(dpy, client->window, _AHWM_MOVE_OFFSET);
#ifdef SHAPE
    if (shape_supported && client->is_shaped) {
        XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                           client->window, ShapeBounding, ShapeSet);
    }
#endif
    debug(("\tDone removing titlebar\n"));
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

    ewmh_client_list_remove(client);
    
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
            client->name = Strdup((char *)xtp.value);
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
        fprintf(stderr, "AHWM: Couldn't allocate Size Hints structure\n");
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
                    leader->transients = client->next_transient;
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
            fprintf(stderr, "AHWM: The application '%s' has a "
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
 * ICCCM 4.1.2.3
 */

void client_frame_position(client_t *client, position_size *ps)
{
#ifdef DEBUG
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
#endif
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

char *client_dbg(client_t *client)
{
    static char buf[40];

    if (client == NULL)
        return "(NULL)";
    snprintf(buf, 40, "(%.10s,%lx,%lx,%lx)", client->name,
             client->window, client->frame, (unsigned long)client);
    return buf;
}
