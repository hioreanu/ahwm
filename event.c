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
#include "xwm.h"
#include "event.h"
#include "client.h"
#include "focus.h"
#include "workspace.h"
#include "keyboard.h"
#include "mouse.h"
#include "xev.h"

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
/* check out line 966 of Xlib.h for a very cool example of a good use
 * for the 'union' in C */
/* windowmaker: src/event.c:237 */
void event_dispatch(XEvent *event)
{
#ifdef DEBUG
    static char *xevent_names[] = {
        "Zero",
        "One",
        "KeyPress",
        "KeyRelease",
        "ButtonPress",
        "ButtonRelease",
        "MotionNotify",
        "EnterNotify",
        "LeaveNotify",
        "FocusIn",
        "FocusOut",
        "KeymapNotify",
        "Expose",
        "GraphicsExpose",
        "NoExpose",
        "VisibilityNotify",
        "CreateNotify",
        "DestroyNotify",
        "UnmapNotify",
        "MapNotify",
        "MapRequest",
        "ReparentNotify",
        "ConfigureNotify",
        "ConfigureRequest",
        "GravityNotify",
        "ResizeRequest",
        "CirculateNotify",
        "CirculateRequest",
        "PropertyNotify",
        "SelectionClear",
        "SelectionRequest",
        "SelectionNotify",
        "ColormapNotify",
        "ClientMessage",
        "MappingNotify",
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
        case KeyPress:
        /* case KeyRelease: */
            keyboard_process(&event->xkey); /* keyboard.c */
            break;
        case ButtonPress:
        case ButtonRelease:
        case MotionNotify:
            mouse_handle_event(event); /* mouse.c */
            break;
        case EnterNotify:
/*        case LeaveNotify: */
            event_enter_leave(&event->xcrossing);
            break;
        case CreateNotify:
            event_create(&event->xcreatewindow);
            break;
        case DestroyNotify:
            event_destroy(&event->xdestroywindow);
            break;
        case UnmapNotify:
            event_unmap(&event->xunmap);
            break;
        /* MapNotify */
        case MapRequest:
            event_maprequest(&event->xmaprequest);
            break;
        /* ReparentNotify */
        /* ConfigureNotify */
        case ConfigureRequest:
            event_configurerequest(&event->xconfigurerequest);
            break;
        case PropertyNotify:
            event_property(&event->xproperty);
            break;
#if 0
        case ColormapNotify:
            event_colormap(&event->xcolormap);
            break;
#endif
        case ClientMessage:
            event_clientmessage(&event->xclient);
            break;
        case CirculateRequest:
            event_circulaterequest(&event->xcirculaterequest);
            break;
        case Expose:
            event_expose(&event->xexpose);
            break;
        /* MappingNotify */
        default:
#ifdef DEBUG
            printf("\tIgnoring event\n");
#endif /* DEBUG */
            break;
    }
}
/* FIXME:  must catch UnmapNotify, DestroyNotify */

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

static void event_unmap(XUnmapEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
#ifdef DEBUG
    client_print("Unmap:", client);
#endif /* DEBUG */
    if (client == NULL) return;
    client->state = NormalState; /* FIXME: right? */
    if (client->frame != None)
        XUnmapWindow(dpy, client->frame);
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
        focus_remove(client);
    }
    client->state = NormalState;
    client->workspace = workspace_current;

    XMapWindow(xevent->display, client->window);
    if (client->frame != None) {
        XMapWindow(xevent->display, client->frame);
        keyboard_grab_keys(client->frame);
        mouse_grab_buttons(client->frame);
    } else {
        keyboard_grab_keys(client->window);
        mouse_grab_buttons(client->window);
    }
    
    focus_set(client);
    focus_ensure();
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
    /* used to iconify or hide windows in 9wm */
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

