/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include "xwm.h"
#include "icccm.h"
#include "debug.h"

static Window icccm_window = None;
static Atom WM_Sn, MESSAGE, VERSION;

static void failure(char *);

void icccm_init()
{
    XSetWindowAttributes xswa;
    XEvent event;
    Time timestamp;
    char buf[16];

    snprintf(buf, 16, "WM_S%d", scr);
    
    WM_Sn = XInternAtom(dpy, buf, False);
    MESSAGE = XInternAtom(dpy, "MESSAGE", False);
    VERSION = XInternAtom(dpy, "VERSION", False);

    /* no need to grab the server using mechanisms in ICCCM: */
/*    XGrabServer(dpy); */
    /* this selection is supposed to be owned by a dedicated window,
     * so if the window manager crashes, selection becomes owner-less */
    if (XGetSelectionOwner(dpy, WM_Sn) != None) {
        /* don't try to steal it, just bomb */
        failure(buf);
    }

    xswa.override_redirect = True;
    icccm_window = XCreateWindow(dpy, root_window, 0, 0, 1, 1, 0,
                                 CopyFromParent, InputOutput,
                                 DefaultVisual(dpy, scr), CWOverrideRedirect,
                                 &xswa);

    XSetSelectionOwner(dpy, WM_Sn, icccm_window, CurrentTime);
    XSync(dpy, False);
/*    XUngrabServer(dpy); */
    if (XGetSelectionOwner(dpy, WM_Sn) != icccm_window) {
        failure(buf);
    }

    /* obtain a timestamp using a zero-length append as per ICCCM */
    XSelectInput(dpy, icccm_window, PropertyChangeMask);
    XSync(dpy, False);
    XChangeProperty(dpy, icccm_window, WM_Sn, XA_INTEGER, 32,
                    PropModeAppend, (void *)failure, 0);
    for (;;) {
        XNextEvent(dpy, &event);
        if (event.type == PropertyNotify
            && event.xproperty.window == icccm_window) {
            timestamp = event.xproperty.time;
            break;
        }
        /* otherwise discard the event */
    }
    /* SelectionRequest events require no event mask */
    XSelectInput(dpy, icccm_window, NoEventMask);

    event.type = ClientMessage;
    event.xclient.message_type = MESSAGE;
    event.xclient.format = 32;
    event.xclient.window = root_window;
    event.xclient.data.l[0] = timestamp;
    event.xclient.data.l[1] = WM_Sn;
    event.xclient.data.l[2] = icccm_window;
    XSendEvent(dpy, root_window, False, 0, &event);
}

/*
 * Nobody, but NOBODY, uses this.  This is supposed to specify window
 * manager ICCCM 2.0 compliance (section 4.3), but I've yet to see an
 * application which checks for this.  I have no idea if I'm doing
 * this correctly, as no window manager I know of does this and I have
 * no client programs to test this against.  I could be interpreting
 * ICCCM completely incorrectly.
 */
/*
 * ICCCM is very obtuse about this stuff:
 * 
 * xevent->owner is my created window
 * 
 * xevent->requestor is a window created by the requestor
 * 
 * xevent->selection is the selection name property
 * 
 * xevent->target is an atom which designates the desired form of
 * information (not a type like XA_INTEGER, but an atom like VERSION
 * which has a type of XA_INTEGER)
 * 
 * xevent->property is a property on xevent->requestor
 * 
 * if xevent->property is NULL, use xevent->target instead
 * 
 * if xevent->property exists, it contains parameters for the
 * conversion request; if the request does not have any parameters,
 * the property should not exist on xevent->requestor
 * 
 * owner should then attempt to set xevent->property on
 * xevent->requestor to the data requested
 * 
 * owner should then send a SelectionNotify with the selection,
 * target, time and property attributes the same as those in the
 * received request.  if the request fails, owner should set the
 * property argument to None in the SelectionNotify event.
 */

void icccm_selection_request(XSelectionRequestEvent *xevent)
{
    Atom their_property;
    XSelectionEvent event;
    long data[2] = {2, 0};

    debug(("*** RECEIVED SELECTION REQUEST from window %#lx\n",
           xevent->requestor));

    event.type = SelectionNotify;
    event.selection = xevent->selection;
    event.target = xevent->target;
    event.time = xevent->time;
    
    if (xevent->owner != icccm_window
        || xevent->selection != WM_Sn
        || xevent->target != VERSION) {
        /* send failure */
        event.property = None;
        XSendEvent(dpy, xevent->requestor, False,
                   NoEventMask, (XEvent *)&event);
    }
    /* FIXME: look at timestamp */

    if (xevent->property != None) {
        their_property = xevent->property;
    } else if (xevent->target != None) {
        their_property = xevent->target;
    }

    XChangeProperty(dpy, xevent->requestor, their_property,
                    XA_INTEGER, 32, PropModeReplace,
                    (unsigned char *)data, 2);
    /* we assume that the ChangeProperty request succeeded */

    event.property = xevent->property;
    XSendEvent(dpy, xevent->requestor, False,
               NoEventMask, (XEvent *)&event);
}


static void failure(char *selection)
{
    fprintf(stderr, "XWM: "
            "It appears that some other window is the owner of the '%s'\n"
            "selection on this screen.  This usually means that another\n"
            "window manager is running.\n"
            "Quitting, ciao!\n", selection);
    exit(1);
}
