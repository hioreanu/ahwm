/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xatom.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include "xwm.h"
#include "event.h"
#include "client.h"
#include "focus.h"
#include "workspace.h"
#include "keyboard.h"
#include "mouse.h"
#include "xev.h"
#include "error.h"
#include "malloc.h"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

Time event_timestamp;

static void event_enter(XCrossingEvent *);
static void event_leave(XCrossingEvent *);
static void event_create(XCreateWindowEvent *);
static void event_destroy(XDestroyWindowEvent *);
static void event_unmap(XUnmapEvent *);
static void event_maprequest(XMapRequestEvent *);
static void event_configurerequest(XConfigureRequestEvent *);
static void event_property(XPropertyEvent *);
static void event_colormap(XColormapEvent *);
static void event_clientmessage(XClientMessageEvent *);
static void event_circulaterequest(XCirculateRequestEvent *);
static void event_expose(XExposeEvent *);
static void event_focus(XFocusChangeEvent *);
static void event_map(XMapEvent *);

#ifdef SHAPE
static void event_shape(XShapeEvent *);
#endif /* SHAPE */

static Time figure_timestamp(XEvent *event);
static client_t *query_stacking_order(Window unmapped);

/*
 * One could also put a timeout handler here (WindowMaker does this),
 * and pass a different timeout value to select()
 */

void event_get(int xfd, XEvent *event)
{
    fd_set fds;

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        if (select(xfd + 1, &fds, &fds, &fds, NULL) > 0) {
            if (XPending(dpy) > 0) {
                XNextEvent(dpy, event);
                return;
            }
        } else if (errno != EINTR) {
            perror("XWM: select:");
        }
    }
}

void event_dispatch(XEvent *event)
{
#ifdef DEBUG
    static char *xevent_names[] = {
        "Zero",                 /* NO */
        "One",                  /* NO */
        "KeyPress",             /* YES */
        "KeyRelease",           /* YES */
        "ButtonPress",          /* YES */
        "ButtonRelease",        /* YES */
        "MotionNotify",         /* special case */
        "EnterNotify",          /* YES */
        "LeaveNotify",          /* YES */
        "FocusIn",              /* TODO */
        "FocusOut",             /* TODO */
        "KeymapNotify",         /* TODO */
        "Expose",               /* YES */
        "GraphicsExpose",       /* NO? */
        "NoExpose",             /* NO? */
        "VisibilityNotify",     /* TODO */
        "CreateNotify",         /* YES */
        "DestroyNotify",        /* YES */
        "UnmapNotify",          /* YES */
        "MapNotify",            /* YES */
        "MapRequest",           /* YES */
        "ReparentNotify",       /* TODO */
        "ConfigureNotify",      /* TODO */
        "ConfigureRequest",     /* YES */
        "GravityNotify",        /* TODO */
        "ResizeRequest",        /* TODO */
        "CirculateNotify",      /* TODO */
        "CirculateRequest",     /* YES */
        "PropertyNotify",       /* YES */
        "SelectionClear",       /* NO */
        "SelectionRequest",     /* NO */
        "SelectionNotify",      /* NO */
        "ColormapNotify",       /* TODO */
        "ClientMessage",        /* YES */
        "MappingNotify",        /* TODO */
    };

    printf("----------------------------------------");
    printf("----------------------------------------\n");
    if (event->type > MappingNotify)
        printf("%-19s unknown (%d)\n", "received event:", event->type);
    else
        printf("%-19s %s (%d)\n", "received event:",
               xevent_names[event->type], event->type);
    xev_print(event);
#endif /* DEBUG */

    event_timestamp = figure_timestamp(event);
    
    /* check the event number, jump to appropriate function */
    switch(event->type) {
        case 0:                 /* can't happen */
        case 1:
            fprintf(stderr, "XWM: received unusual event type %d\n",
                    event->type);
            break;
            
        case KeyPress:          /* XGrabKeys in keyboard.c */
        case KeyRelease:        /* XGrabKeys in keyboard.c */
            keyboard_process(&event->xkey);
            break;
            
        case ButtonPress:       /* XGrabButton in mouse.c */
        case ButtonRelease:     /* XGrabButton in mouse.c */
            mouse_handle_event(event);
            break;
            
        case MotionNotify:      /* can't happen */
            fprintf(stderr, "XWM: received MotionNotify in wrong place\n");
            break;
            
        case EnterNotify:       /* frame EnterWindowMask, client.c */
            event_enter(&event->xcrossing);
            break;
            
        case LeaveNotify:       /* frame LeaveWindowMask, see event_unmap() */
            event_leave(&event->xcrossing);
            break;
            
        case FocusIn:           /* frame FocusChangeMask, client.c */
            event_focus(&event->xfocus);
            break;
            
/*        case FocusOut: */     /* TODO */
/*        case KeymapNotify: */ /* TODO */
            
        case Expose:            /* frame ExposureMask, client.c */
            event_expose(&event->xexpose);
            break;
            
/*        case GraphicsExpose: */ /* ignored */
/*        case NoExpose: */     /* ignored */
/*        case VisibilityNotify: */ /* TODO */
            
        case CreateNotify:      /* root SubstructureNotifyMask, xwm.c */
            event_create(&event->xcreatewindow);
            break;
            
        case DestroyNotify:     /* client StructureNotifyMask, client.c */
            event_destroy(&event->xdestroywindow);
            break;
            
        case UnmapNotify:       /* client StructureNotifyMask, client.c */
            event_unmap(&event->xunmap);
            break;
            
        case MapNotify:         /* client StructureNotifyMask, client.c */
            event_map(&event->xmap);
            break;
            
        case MapRequest:        /* frame SubstructureRedirectMask, client.c */
            event_maprequest(&event->xmaprequest);
            break;
            
/*        case ReparentNotify: */ /* TODO */
/*        case ConfigureNotify: */ /* TODO */
            
        case ConfigureRequest:  /* frame SubstructureRedirectmask, client.c */
            event_configurerequest(&event->xconfigurerequest);
            break;
            
/*        case GravityNotify: */ /* TOOD */
/*        case ResizeRequest: */ /* TODO */
/*        case CirculateNotify: */ /* TODO */
            
        case CirculateRequest:  /* frame SubstructureRedirectMask, client.c */
            event_circulaterequest(&event->xcirculaterequest);
            break;
            
        case PropertyNotify:    /* client PropertyChangeMask, client.c */
            event_property(&event->xproperty);
            break;

/*        case SelectionClear: */ /* ignored */
/*        case SelectionRequest: */ /* ignored */
/*        case SelectionNotify: */ /* ignored */

#if 0
        case ColormapNotify:    /* TODO */
            event_colormap(&event->xcolormap);
            break;
#endif
            
        case ClientMessage:     /* client's in charge of this */
            event_clientmessage(&event->xclient);
            break;
            
/*        case MappingNotify: */ /* TODO */
            
        default:

#ifdef SHAPE
            if (shape_supported &&
                event->type == shape_event_base + ShapeNotify) {
                event_shape((XShapeEvent *)event);
            }
#endif /* SHAPE */
#ifdef DEBUG
            printf("\tIgnoring event\n");
#endif /* DEBUG */
            break;
    }
}

/*
 * Focus and raise if possible
 * 
 * The problem is that sometimes we'll get EnterNotify events that and
 * we don't want to focus the window on those events - for example,
 * when the window under the pointer unmaps, it changes the window the
 * pointer is in and causes an EnterNotify on the new
 * window-under-pointer.  We only want to respond to this event when
 * the user moves the pointer, so we set a flag on clients that may
 * soon get an EnterNotify and ignore the event if the flag is set.
 * 
 * NB:  all of this is needed because we allow a
 * focus-follows-mouse-style processing without the assumption that
 * the window under the pointer gets the focus.
 * 
 * I've seen no other window manager that does this correctly -
 * they'll just force you to choose another window to focus or focus
 * the wrong window, which is clearly unacceptable.
 */

static void event_enter(XCrossingEvent *xevent)
{
    client_t *client;
    
    if (xevent->mode != NotifyNormal)
        return;
    
    /* If the mouse is NOT in the focus window but in some other
     * window and it moves to the titlebar of the window, it will
     * generate this event, and I don't want this to do anything
     */
    if (xevent->detail == NotifyInferior) return;

    client = client_find(xevent->window);
    if (client != NULL && client->state == NormalState) {
        if (client->ignore_enternotify == 1) {
#ifdef DEBUG
            printf("\tclient has ignore_enternotify\n");
#endif /* DEBUG */
            client->ignore_enternotify = 0;
        } else {
            focus_set(client, CurrentTime);
        }
    }
}

/*
 * We set the ignore_enternotify flag on the client under the mouse on
 * any unmap.  Unfortunately, we cannot know if the EnterNotify event
 * will actually be created by the server.  For example, open two
 * xterms, move them to different parts of the screen and then set the
 * focus (using alt-tab) to the xterm which is not under the pointer.
 * Then type in "xrefresh."  "Xrefresh" maps and unmaps a window with
 * override_redirect, so we don't keep track of its position/size, so
 * we can't tell whether or not it was under the pointer (so we can't
 * tell if an EnterNotify event will be sent).  There is no way to
 * tell the difference between an EnterNotify generated by the server
 * on a window unmap and an EnterNotify generated because the user
 * actually moved the pointer.
 * 
 * If we get a LeaveNotify, however, we know we won't be getting a
 * stray EnterNotify, so we reset the flag.  That's the only reason we
 * would even listen for LeaveNotify events.
 */

static void event_leave(XCrossingEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
    if (client == NULL) return;
    if (client->ignore_enternotify == 1) {
#ifdef DEBUG
        printf("\tResetting client->ignore_enternotify\n");
#endif /* DEBUG */
        client->ignore_enternotify = 0;
    }
    client->frame_event_mask &= ~LeaveWindowMask;
    XSelectInput(dpy, client->frame, client->frame_event_mask);
}

/*
 * create client structure, basically manage the window
 */

static void event_create(XCreateWindowEvent *xevent)
{
    client_t *client;

    if (xevent->override_redirect) {
#ifdef DEBUG
        printf("\tWindow 0x%08X has override_redirect, ignoring\n",
               (unsigned int)xevent->window);
#endif /* DEBUG */
        return;
    }

    client = client_create(xevent->window);
#ifdef DEBUG
    client_print("Create:", client);
#endif /* DEBUG */
    if (client == NULL) return;
}

/*
 * remove client structure
 */

static void event_destroy(XDestroyWindowEvent *xevent)
{
    client_t *client, *under_mouse;
    
    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Destroy:", client);
#endif /* DEBUG */
    if (client == NULL) {
        return;
    }
    if (client->titlebar == xevent->window) return;
    if (client->state == NormalState) {
        /* received a DestroyNotify before or without an UnmapNotify
         * 
         * the Xlib docs don't say whether UnmapNotify will always
         * be generated before DestroyNotify, but I've always seen
         * it happen that way (so I haven't seen this code run)
         */
        focus_remove(client, event_timestamp);
        if (client->workspace == workspace_current) {
            under_mouse = query_stacking_order(client->frame);
            if (under_mouse != NULL) under_mouse->ignore_enternotify = 1;
        }
    }
    client_destroy(client);
}

/*
 * ICCCM 4.1.4:
 * 
 * "For compatibility with obsolete clients, window managers should
 * trigger the transition to the Withdrawn state on the real
 * UnmapNotify rather than waiting for the synthetic one. They should
 * also trigger the transition if they receive a synthetic UnmapNotify
 * on a window for which they have not yet received a real UnmapNotify."
 * 
 * Which means that we simply set the state to Withdrawn if we receive
 * any kind of unmap request.  Most other window managers also
 * reparent the window to the root window when it's unmapped so that
 * it doesn't get mapped again if the window manager exits.  I'm not
 * going to deal with that since I don't use X that way (window
 * manager crashes = you're screwed).
 * 
 * Also, this is where we see if we are going to get an EnterNotify
 * which we don't want to see (happens for both client windows and
 * override_redirect windows (eg, xrefresh).
 */

static void event_unmap(XUnmapEvent *xevent)
{
    client_t *client, *under_mouse;
    
    client = client_find(xevent->window);

#ifdef DEBUG
    client_print("Unmap:", client);
#endif /* DEBUG */
    if (client == NULL) {
        under_mouse = query_stacking_order(None);
        if (under_mouse != NULL) {
            under_mouse->ignore_enternotify = 1;
            under_mouse->frame_event_mask |= LeaveWindowMask;
            XSelectInput(dpy, under_mouse->frame,
                         under_mouse->frame_event_mask);
        }
        return;
    }

    /* if we unmapped it ourselves, no need to do anything */
    if (xevent->window != client->window) return;

    /* well, at this point, we need to do some things to the window
     * (such as setting the WM_STATE property on the window to Withdrawn
     * as per ICCCM), but the problem is that the client may have
     * already destroyed the window, and the server may have already
     * processed the destroy request, which makes the window invalid.
     * I can't think of any way of figuring out if the window is still
     * valid other than grabbing the server and seeing if some request
     * on that window fails.  In any case, we have to catch some sort
     * of error, and since we don't care if the requests we are about
     * to make succeed or fail, we just ignore the errors they can
     * cause. */
    /* FIXME:  this will break with synchronous behaviour turned off */

    focus_remove(client, event_timestamp);
    
    if (client->state == NormalState) {
        XUnmapWindow(dpy, client->frame);

        error_ignore(BadWindow, X_UnmapWindow);
        XUnmapWindow(dpy, client->window);
        error_ignore(BadWindow, X_UnmapWindow); /* FIXME */
        if (client->workspace == workspace_current) {
            under_mouse = query_stacking_order(client->frame);
            if (under_mouse != NULL) under_mouse->ignore_enternotify = 1;
        }
    }
    client->state = WithdrawnState;
    
    error_ignore(BadWindow, X_ChangeProperty);
    client_inform_state(client);
    error_unignore(BadWindow, X_ChangeProperty);
}

/*
 * Map the client and the frame, update WM_STATE on the client window
 */

static void event_maprequest(XMapRequestEvent *xevent)
{
    client_t *client;
    Bool addfocus;

    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Map Request:", client);
#endif /* DEBUG */
    if (client == NULL) {
        fprintf(stderr, "XWM: unable to find client, shouldn't happen\n");
        return;
    }
    if (client->state == NormalState) {
        /* This should never happen as XMapWindow on an already-mapped
         * window should never generate an X request, but better be
         * safe (don't want to raise a window randomly) */
        addfocus = False;
    } else {
        addfocus = True;
    }

    if (client->xwmh != NULL && client->state == WithdrawnState) {
        client->state = client->xwmh->initial_state;
        if (client->state != IconicState)
            client->state = NormalState;
    } else {
        client->state = NormalState;
    }
    client->workspace = workspace_current;

    if (client->state == NormalState) {
        XMapWindow(xevent->display, client->window);
        XMapWindow(xevent->display, client->frame);
        if (client->titlebar != None) {
            XMapWindow(xevent->display, client->titlebar);
        }
        keyboard_grab_keys(client);
        mouse_grab_buttons(client);
        if (addfocus) focus_add(client, event_timestamp);
    } else {
#ifdef DEBUG
        printf("\tsigh...client->state = %d\n", client->state);
#endif /* DEBUG */
    }
    client_inform_state(client);
}

/*
 * update our idea of client's geometry, massage request
 * according to client's gravity if it has a title bar
 * 
 * This may also generate one of those "Bad" EnterNotify events, but I
 * haven't seen it be a problem yet (hasn't interrupted my work), so
 * I'm not dealing with that possibility.
 */

static void event_configurerequest(XConfigureRequestEvent *xevent)
{
    client_t       *client;
    XWindowChanges  xwc;
    position_size   ps;
    
    client = client_find(xevent->window);

    if (client == NULL) {
        fprintf(stderr,
                "XWM: Got ConfigureRequest from unknown client, "
                "shouldn't happen\n");
        return;
    }

#ifdef DEBUG
    client_print("Configure Request:", client);
    printf("\tBefore: %dx%d+%d+%d\n", client->width, client->height,
           client->x, client->y);
#endif /* DEBUG */

    if (xevent->value_mask & CWBorderWidth) {
        client->orig_border_width = xevent->border_width;
    }
    
    ps.x = client->x;
    ps.y = client->y;
    ps.width = client->width;
    ps.height = client->height;

#ifdef DEBUG
    printf("\tClient is changing: ");
#endif /* DEBUG */
    if (xevent->value_mask & CWX) {
        client->prev_x = client->prev_width = -1;
        ps.x = xevent->x;
#ifdef DEBUG
        printf("x ");
#endif /* DEBUG */
    }
    if (xevent->value_mask & CWY) {
        client->prev_y = client->prev_height = -1;
        ps.y = xevent->y;
#ifdef DEBUG
        printf("y ");
#endif /* DEBUG */
    }
    if (xevent->value_mask & CWWidth) {
        client->prev_x = client->prev_width = -1;
        ps.width = xevent->width;
#ifdef DEBUG
        printf("width ");
#endif /* DEBUG */
    }
    if (xevent->value_mask & CWHeight) {
        client->prev_y = client->prev_height = -1;
        ps.height = xevent->height;
#ifdef DEBUG
        printf("height ");
#endif /* DEBUG */
    }
    if (xevent->value_mask & CWBorderWidth) {
        client->orig_border_width = xevent->border_width;
#ifdef DEBUG
        printf("border_width ");
#endif /* DEBUG */
    }
#ifdef DEBUG
    if (xevent->value_mask & CWSibling) {
        printf("sibling ");
    }
    if (xevent->value_mask & CWStackMode) {
        printf("stack_mode ");
    }
    printf("\n");
#endif /* DEBUG */

    client_frame_position(client, &ps);
    client->x = ps.x;
    client->y = ps.y;
    client->width = ps.width;
    client->height = ps.height;
    
    xwc.x            = client->x;
    xwc.y            = client->y;
    xwc.width        = client->width;
    xwc.height       = client->height;
    xwc.border_width = 0;
    xwc.sibling      = xevent->above;
    xwc.stack_mode   = xevent->detail;

    XConfigureWindow(dpy, client->frame,
                     xevent->value_mask | CWX | CWY | CWWidth | CWHeight,
                     &xwc);
    if (client->titlebar != None)
        XConfigureWindow(dpy, client->titlebar, CWWidth, &xwc);

    if (client->titlebar != None) {
        xwc.y       = TITLE_HEIGHT;
        xwc.height -= TITLE_HEIGHT;
    } else {
        xwc.y = 0;
    }
    xwc.x       = 0;
    XConfigureWindow(xevent->display, client->window,
                     xevent->value_mask | CWX | CWY | CWWidth | CWHeight,
                     &xwc);

#ifdef DEBUG
    printf("\tAfter: %dx%d+%d+%d\n", client->width, client->height,
           client->x, client->y);
#endif /* DEBUG */
}

/*
 * window property changed, update client structure
 */

static void event_property(XPropertyEvent *xevent)
{
    client_t *client;

    client = client_find(xevent->window);
    if (client == NULL) return;
    if (xevent->atom == XA_WM_NAME) {
#ifdef DEBUG
        printf("\tWM_NAME, changing client->name\n");
#endif /* DEBUG */
        Free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
    } else if (xevent->atom == XA_WM_CLASS) {
#ifdef DEBUG
        printf("\tWM_CLASS, changing client->[class, instance]\n");
#endif /* DEBUG */
        if (client->class != None) XFree(client->class);
        if (client->instance != None) XFree(client->instance);
        client_set_instance_class(client);
    } else if (xevent->atom == XA_WM_HINTS) {
#ifdef DEBUG
        printf("\tWM_HINTS, changing client->xwmh\n");
#endif /* DEBUG */
        if (client->xwmh != NULL && client->group_leader == NULL)
            XFree(client->xwmh);
        client_set_xwmh(client);
    } else if (xevent->atom == XA_WM_NORMAL_HINTS) {
#ifdef DEBUG
        printf("\tWM_NORMAL_HINTS, changing client->xsh\n");
#endif /* DEBUG */
        if (client->xsh != None) XFree(client->xsh);
        client_set_xsh(client);
    } else if (xevent->atom == WM_PROTOCOLS) {
#ifdef DEBUG
        printf("\tWM_PROTOCOLS, changing client->protocols\n");
#endif /* DEBUG */
        client_set_protocols(client);
    } else if (xevent->atom == XA_WM_TRANSIENT_FOR) {
        client_set_transient_for(client);
    }
}

static void event_colormap(XColormapEvent *xevent)
{
    /* FIXME: deal with this later */
}

/*
 * The only thing we look for here is if the window wishes to iconize
 * itself as per ICCCM 4.1.4
 */

static void event_clientmessage(XClientMessageEvent *xevent)
{
    client_t *client;
    
    if (xevent->message_type == WM_CHANGE_STATE) {
        /* ICCCM 4.1.4 */
        client = client_find(xevent->window);
        if (xevent->format == 32 && xevent->data.l[0] == IconicState) {
#ifdef DEBUG
            printf("\tClient insists on iconifying itself, placating it\n");
#endif /* DEBUG */
            XUnmapWindow(dpy, client->frame);
            XUnmapWindow(dpy, client->window);
            client->state = IconicState;
            client_inform_state(client);
            focus_remove(client, event_timestamp);
        }
    }
}

static void event_circulaterequest(XCirculateRequestEvent *xevent)
{
    /* nobody uses this */
    fprintf(stderr, "XWM: Received CirculateRequest, ignoring it\n");
}

/*
 * Should only get this if titlebar needs repainting; we simply
 * repaint the whole thing at once.
 */

static void event_expose(XExposeEvent *xevent)
{
    client_t *client;

    /* simple, stupid */
    if (xevent->count != 0) return;
    
    client = client_find(xevent->window);
    if (client != NULL)
        client_paint_titlebar(client);
}

/*
 * Only does anthing if client is using Globally Active focus (see
 * ICCCM) or if client stole the focus.
 */

static void event_focus(XFocusChangeEvent *xevent)
{
    client_t *client;

    if (xevent->type == FocusIn
        && xevent->mode == NotifyNormal
        && xevent->detail == NotifyNonlinearVirtual) {
        /* someone stole our focus without asking for permission
         * or is using funky input focus model (eg, Globally Active) */
        client = client_find(xevent->window);
        if (client == NULL) {
            fprintf(stderr, "XWM: Could not find client on FocusIn event\n");
            return;
        }
#ifdef DEBUG
        printf("\tSetting focus\n");
#endif /* DEBUG */
        focus_set(client, event_timestamp);
    }
}

/*
 * Occasionally we map a window and then immediately give it the focus
 * - however, we get a BadMatch error if the window hasn't yet been
 * made visible by the server.  This function ensures we always focus
 * what we want to focus.  workspace.c does something similar, but it
 * doesn't hurt to have some redundancy (having the wrong window
 * focused is a CATASTROPHIC FAILURE for a window manager)
 */

static void event_map(XMapEvent *xevent)
{
    client_t *client;

    client = client_find(xevent->window);
    if (client != NULL && client == focus_current) {
#ifdef DEBUG
        printf("\tCalling XSetInputFocus\n");
#endif /* DEBUG */
        XSetInputFocus(dpy, client->window, RevertToPointerRoot, CurrentTime);
    }
}

/*
 * If we get this, we update our frame to match the client's shape and
 * get rid of the titlebar if the client has one.
 */

#ifdef SHAPE
static void event_shape(XShapeEvent *xevent)
{
    XRectangle *rectangles;
    int n_rects, junk;
    client_t *client;

    client = client_find(xevent->window);
    if (client == NULL) return;
    if ( (rectangles = XShapeGetRectangles(dpy, xevent->window,
                                           ShapeBounding, &n_rects,
                                           &junk)) == NULL)
        return;
    if (rectangles != NULL) XFree(rectangles);
    if (n_rects <= 1)
        return;
    if (client->titlebar != None) {
        client_remove_titlebar(client);
    }
    XShapeCombineShape(dpy, client->frame, ShapeBounding, 0, 0,
                       client->window, ShapeBounding, ShapeSet);
}
#endif /* SHAPE */

/* 
 * all events defined by Xlib have common first few members, so just
 * use any arbitrary event structure to get the window if the event is
 * defined by Xlib
 */
Window event_window(XEvent *event)
{
    if (event->type > 1 && event->type < LASTEvent) {
        return event->xbutton.window;
    } else {
        return None;
    }
}

/* not all events have a timestamp, some have it in the same place */
static Time figure_timestamp(XEvent *event)
{
    switch (event->type) {
        case ButtonPress:
        case ButtonRelease:
        case KeyPress:
        case KeyRelease:
        case MotionNotify:
        case EnterNotify:
        case LeaveNotify:
            return event->xbutton.time;
        case PropertyNotify:
        case SelectionClear:
            return event->xproperty.time;
        case SelectionRequest:
            return event->xselectionrequest.time;
        case SelectionNotify:
            return event->xselection.time;
        default:
#ifdef SHAPE
            if (event->type == shape_event_base + ShapeNotify)
                return ((XShapeEvent *)event)->time;
#endif /* SHAPE */
            return CurrentTime;
    }
}

/*
 * This looks at the windows which are actually mapped (mapped on the
 * server, not our idea of mapped) and returns the client under the
 * pointer.  It will ignore the parameter window if it is found as a
 * frame of a client.  This is used to detect unwanted EnterNotify
 * events.
 */

static client_t *query_stacking_order(Window unmapped)
{
    Window junk1, *children;
    int i, nchildren, x, y, junk2;
    unsigned int junk3;
    client_t *client;

    if (XQueryTree(dpy, root_window, &junk1, &junk1,
                   &children, &nchildren) == 0) {
#ifdef DEBUG
        printf("\tstacking: xquerytree failed\n");
#endif /* DEBUG */
        return NULL;
    }
    if (XQueryPointer(dpy, root_window, &junk1, &junk1, &x, &y,
                      &junk2, &junk2, &junk3) == 0) {
#ifdef DEBUG
        printf("\tstacking: xquerypointer failed\n");
#endif /* DEBUG */
        return NULL;
    }

    if (children == NULL) {
#ifdef DEBUG
        printf("\tstacking: no children\n");
#endif /* DEBUG */
        return NULL;
    }

    for (i = nchildren - 1; i >= 0; i--) {
#ifdef DEBUG
        printf("\tstacking: --\n");
#endif /* DEBUG */
        if (children[i] == unmapped) {
#ifdef DEBUG
            printf("\tstacking: unmapped window\n");
#endif /* DEBUG */
            continue;
        }
        client = client_find(children[i]);
        if (client == NULL) {
#ifdef DEBUG
            printf("\tstacking: window 0x%08X is not client\n",
                   (unsigned int)children[i]);
#endif /* DEBUG */
            continue;
        }
#ifdef DEBUG
        printf("\tstacking: client is %s\n", client->name);
#endif /* DEBUG */
        if (x >= client->x
            && y >= client->y
            && x <= client->x + client->width
            && y <= client->y + client->height) {
#ifdef DEBUG
            printf("\tstacking: client is below pointer\n");
#endif /* DEBUG */
            XFree(children);
            return client;
        }
    }
    XFree(children);
    return NULL;
}
