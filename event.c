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

static void event_enter_leave(XCrossingEvent *);
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

/*
 * FIXME:  every window manager I've seen does this by sitting a
 * select() loop, but why?  XNextEvent() blocks, there's no need to
 * cook up one's own event blocking thing...besides that, XNextEvent()
 * uses select() internally in all the X implementations I've seen....
 */

/*
 * One could also put a timeout handler here (WindowMaker does this),
 * and pass a different timeout value to select()
 */

void event_get(int xfd, XEvent *event)
{
    fd_set fds;

    XNextEvent(dpy, event);
    return;
    for (;;) {
        FD_ZERO(&fds);
        FD_SET(xfd, &fds);
        if (select(xfd + 1, &fds, &fds, &fds, NULL) > 0) {
            if (QLength(dpy) > 0) {
                XNextEvent(dpy, event);
                return;
            }
        } else if (errno != EINTR) {
            perror("xwm: select:");
            /* just continue, no need to die */
            /* although this means that if the display context
             * becomes invalid for some reason, we lose */
        }
    }
}

/* Line 571 of my Xlib.h */
/* file:/home/ach/xlib/events/processing-overview.html */
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
        "MapNotify",            /* TODO */
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
            event_enter_leave(&event->xcrossing);
            break;
            
/*        case LeaveNotify: */  /* ignored */
/*        case FocusIn: */      /* TODO */
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
            
/*        case MapNotify: */    /* TODO */
            
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
            
        case ClientMessage:     /* client in charge of this */
            event_clientmessage(&event->xclient);
            break;
            
/*        case MappingNotify: */ /* TODO */
            
        default:
#ifdef DEBUG
            printf("\tIgnoring event\n");
#endif /* DEBUG */
            break;
    }
}

/*
 * see the manual page for each individual event (for instance,
 * XKeyEvent(3))
 */

static void event_enter_leave(XCrossingEvent *xevent)
{
    client_t *client;
    
    /* have no idea when other 'modes' will be generated, just be safe */
    if (xevent->mode != NotifyNormal)
        return;

    /* If the mouse is NOT in the focus window but in some other
     * window and it moves to the frame of the window, it will
     * generate this event, and I don't want this to do anything
     */
    if (xevent->detail == NotifyInferior) return;

    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Enter/Leave:", client);
    printf("\tmode = %d, detail = %d\n", xevent->mode, xevent->detail);
#endif /* DEBUG */
    if (client != NULL && focus_canfocus(client)) {
        focus_set(client);      /* focus.c */
        focus_ensure();
    }
}

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

static void event_destroy(XDestroyWindowEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Destroy:", client);
#endif /* DEBUG */
    if (client == NULL) {
        return;
    }
    focus_remove(client);
    focus_ensure();
    client_destroy(client);
}

/*
 * ICCCM 4.1.4:
 * 
 * For compatibility with obsolete clients, window managers should
 * trigger the transition to the Withdrawn state on the real
 * UnmapNotify rather than waiting for the synthetic one. They should
 * also trigger the transition if they receive a synthetic UnmapNotify
 * on a window for which they have not yet received a real UnmapNotify.
 * 
 * Which means that we simply set the state to Withdrawn if we receive
 * any kind of unmap request.  Most other window managers also reparent
 * the window to the root window when it's unmapped so that it doesn't
 * get mapped again if the window manager exits.  I'm not going to deal
 * with that since I don't use X that way.
 */

static void event_unmap(XUnmapEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Unmap:", client);
#endif /* DEBUG */
    if (client == NULL) return;

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
    if (client->state == NormalState) {
        if (client->frame != None)
            XUnmapWindow(dpy, client->frame);

        error_ignore(BadWindow, X_UnmapWindow);
        XUnmapWindow(dpy, client->window);
        error_ignore(BadWindow, X_UnmapWindow);
    }
    client->state = WithdrawnState;

    error_ignore(BadWindow, X_ChangeProperty);
    client_inform_state(client);
    error_unignore(BadWindow, X_ChangeProperty);

    focus_remove(client);
    focus_ensure();
}

static void event_maprequest(XMapRequestEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Map Request:", client);
#endif /* DEBUG */
    if (client == NULL) {
        fprintf(stderr, "unable to find client, shouldn't happen\n");
        return;
    }
    if (client->state == NormalState) {
        /* already mapped, remove from focus list since it will
         * be added again at end of function */
        focus_remove(client);
    }

    if (client->xwmh != NULL && client->state == WithdrawnState) {
        client->state = client->xwmh->initial_state;
    } else {
        client->state = NormalState;
    }
    client->workspace = workspace_current;

    if (client->state == NormalState) {
        XMapWindow(xevent->display, client->window);
        if (client->frame != None) {
            XMapWindow(xevent->display, client->frame);
        }
        keyboard_grab_keys(client);
        mouse_grab_buttons(client);
        focus_set(client);
        focus_ensure();
    }
    client_inform_state(client);
}

static void event_configurerequest(XConfigureRequestEvent *xevent)
{
    client_t       *client;
    XWindowChanges  xwc;
    
    client = client_find(xevent->window);

#ifdef DEBUG
    client_print("Configure Request:", client);
    printf("\tBefore: %dx%d+%d+%d\n", client->x, client->y,
           client->width, client->height);
#endif /* DEBUG */
    
    if (xevent->value_mask & CWX)
        client->x      = xevent->x;
    if (xevent->value_mask & CWY)
        client->y      = xevent->y;
    if (xevent->value_mask & CWWidth)
        client->width  = xevent->width;
    if (xevent->value_mask & CWHeight)
        client->height = xevent->height + TITLE_HEIGHT;

    xwc.x            = client->x;
    xwc.y            = client->y;
    xwc.width        = client->width;
    xwc.height       = client->height;
    xwc.border_width = xevent->border_width;
    xwc.sibling      = xevent->above;
    xwc.stack_mode   = xevent->detail;

    XConfigureWindow(dpy, client->frame,
                     xevent->value_mask | CWX | CWY | CWWidth | CWHeight,
                     &xwc);

    xwc.y       = TITLE_HEIGHT;
    xwc.x       = 0;
    xwc.height -= TITLE_HEIGHT;
    XConfigureWindow(xevent->display, client->window,
                     xevent->value_mask | CWX | CWY | CWWidth | CWHeight,
                     &xwc);

#ifdef DEBUG
    printf("\tBefore: %dx%d+%d+%d\n", client->x, client->y,
           client->width, client->height);
#endif /* DEBUG */
}

static void event_property(XPropertyEvent *xevent)
{
    client_t *client;

    client = client_find(xevent->window);
    if (client == NULL) return;
    switch (xevent->atom) {
        case XA_WM_NAME:
#ifdef DEBUG
            printf("\tWM_NAME, changing client->name\n");
#endif /* DEBUG */
            free(client->name);
            client_set_name(client);
            client_paint_titlebar(client);
            break;
        case XA_WM_CLASS:
#ifdef DEBUG
            printf("\tWM_CLASS, changing client->[class, instance]\n");
#endif /* DEBUG */
            if (client->class != None) XFree(client->class);
            if (client->instance != None) XFree(client->instance);
            client_set_instance_class(client);
            break;
        case XA_WM_HINTS:
#ifdef DEBUG
            printf("\tWM_HINTS, changing client->xwmh\n");
#endif /* DEBUG */
            if (client->xwmh != None) XFree(client->xwmh);
            client_set_xwmh(client);
            break;
        case XA_WM_NORMAL_HINTS:
#ifdef DEBUG
            printf("\tWM_NORMAL_HINTS, changing client->xsh\n");
#endif /* DEBUG */
            if (client->xsh != None) XFree(client->xsh);
            client_set_xsh(client);
    }
}

static void event_colormap(XColormapEvent *xevent)
{
    /* deal with this later */
}

static void event_clientmessage(XClientMessageEvent *xevent)
{
    client_t *client;
    
    if (xevent->message_type == WM_CHANGE_STATE) {
        client = client_find(xevent->window);
        if (xevent->format == 32 && xevent->data.l[0] == IconicState) {
            if (client->frame != None) {
                XUnmapWindow(dpy, client->frame);
            }
            XUnmapWindow(dpy, client->window);
            client->state = IconicState;
            client_inform_state(client);
            focus_remove(client);
            focus_ensure();
        }
    }
}

static void event_circulaterequest(XCirculateRequestEvent *xevent)
{
    /* nobody uses this */
}

static void event_expose(XExposeEvent *xevent)
{
    client_t *client;

    /* simple, stupid */
    if (xevent->count != 0) return;
    
    client = client_find(xevent->window);
    if (client != NULL)
        client_paint_titlebar(client);
}

