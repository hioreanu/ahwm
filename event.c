/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "xwm.h"
#include "event.h"
#include "client.h"
#include "workspace.h"

static void event_key(XKeyEvent *);
static void event_button(XButtonEvent *);
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
    /* check the event number, jump to appropriate function */
    switch(event->type) {
        case KeyPress:
        /* case KeyRelease: */
            event_key(&event->xkey);
            break;
        case ButtonPress:
        case ButtonRelease:
            event_button(&event->xbutton);
            break;
        case EnterNotify:
        case LeaveNotify:
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
        /* MappingNotify */
    }
}

static void event_key(XKeyEvent *xevent)
{
    printf("got a key event..\n");
}

static void event_button(XButtonEvent *xevent)
{
    printf("Got a button event...\n");
}

static void event_enter_leave(XCrossingEvent *xevent)
{
    client_t *client;
    
    printf("Got a crossing event....\n");
    /* stolen from 9wm: */
//    if (xevent->mode != NotifyGrab || xevent->detail != NotifyNonlinearVirtual)
//        return;
    client = client_find(xevent->window);
    if (client != NULL && focus_canfocus(client)) {
        focus_set(client);      /* focus.c */
        focus_ensure();
    }
}

static void event_create(XCreateWindowEvent *xevent)
{
    client_t *client;

    printf("Got a create window event....\n");
    if (xevent->override_redirect) return;

    client = client_create(xevent->window);
    if (client == NULL) return;
    client->x = xevent->x;
    client->y = xevent->y;
    client->width = xevent->width;
    client->height = xevent->height;
    client->parent = root_window;
}

static void event_destroy(XDestroyWindowEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
    printf("Got a destroy event for client 0x%08X...\n", client);
    if (client == NULL) {
        printf("unable to find client, shouldn't happen\n");
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
    printf("Got an unmap event for client 0x%08X...\n", client);
    if (client == NULL) {
        printf("unable to find client, shouldn't happen\n");
        return;
    }
    client->state = UNMAPPED;
    focus_remove(client);
    focus_ensure();
}

static void event_maprequest(XMapRequestEvent *xevent)
{
    client_t *client;
    
    client = client_find(xevent->window);
    printf("Got a map request for client 0x%08X...\n", client);
    if (client == NULL) {
        printf("unable to find client, shouldn't happen\n");
        return;
    }
    if (client->state == MAPPED) {
        focus_remove(client);
    }
    client->state = MAPPED;
    client->workspace = workspace_current;

    XMapWindow(xevent->display, client->window);
    keyboard_grab_keys(client->window);
    focus_set(client);
    focus_ensure();
}

static void event_configurerequest(XConfigureRequestEvent *xevent)
{
    client_t *client;
    XWindowChanges xwc;
    
    client = client_find(xevent->window);
    printf("Got a configure request for client 0x%08X....\n", client);
    if (xevent->value_mask & CWX)
        client->x = xevent->x;
    if (xevent->value_mask & CWY)
        client->y = xevent->y;
    if (xevent->value_mask & CWWidth)
        client->width = xevent->width;
    if (xevent->value_mask & CWHeight)
        client->height = xevent->height;

    xwc.x = xevent->x;
    xwc.y = xevent->y;
    xwc.height = xevent->height;
    xwc.width = xevent->width;
    xwc.x = xevent->x;
    xwc.x = xevent->x;
    xwc.border_width = xevent->border_width;
    xwc.sibling = xevent->above;
    xwc.stack_mode = xevent->detail;

    XConfigureWindow(xevent->display, xevent->window,
                     xevent->value_mask, &xwc);

/*
 * file:/home/ach/xlib/events/structure-control/configure.html
 * file:/home/ach/xlib/window/configure.html#XWindowChanges
 */
    
}

static void event_property(XPropertyEvent *xevent)
{
    /* probably don't need to do anything here */
    printf("Got a property event...\n");
}

static void event_colormap(XColormapEvent *xevent)
{
    /* deal with this later */
    printf("Got a colormap event..\n");
}

static void event_clientmessage(XClientMessageEvent *xevent)
{
    /* used to iconify or hide windows in 9wm */
    printf("Got a client message event...\n");
}

static void event_circulaterequest(XCirculateRequestEvent *xevent)
{
    printf("WTF?  Nobody uses this...\n");
}

