/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <stdio.h>

#include "workspace.h"
#include "focus.h"
#include "event.h"
#include "debug.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

char *workspace_colors[NO_WORKSPACES] = {
    "#404040",
    "#2F4F4F",
    "#000050",
    "#500000",
    "#500050",
    "#A08000",
    "#101010"
};

unsigned long workspace_pixels[NO_WORKSPACES] = { 0 };
unsigned long workspace_dark_highlight[NO_WORKSPACES];
unsigned long workspace_darkest_highlight[NO_WORKSPACES];
unsigned long workspace_highlight[NO_WORKSPACES];

int workspace_current = 1;

static void must_focus_this_client(client_t *client);
static void alloc_workspace_colors();

void workspace_goto(XEvent *xevent, void *v)
{
    int new_workspace = (int)v; /* this is always safe */
    client_t *client, *tmp;

    if (new_workspace < 1 || new_workspace > NO_WORKSPACES) {
        fprintf(stderr, "XWM:  attempt to go to invalid workspace %d\n",
                new_workspace);
        return;
    }

    debug(("\tgoing to workspace %d\n", new_workspace));
    
    client = focus_stacks[workspace_current - 1];
    if (client != NULL) {
        XUnmapWindow(dpy, client->frame);
        tmp = client;
        for (client = tmp->next_focus;
             client != tmp;
             client = client->next_focus) {
            XUnmapWindow(dpy, client->frame);
        }
    }
    workspace_current = new_workspace;
    workspace_update_color();
    
    client = focus_stacks[workspace_current - 1];
    if (client != NULL) {
        tmp = client;
        for (client = tmp->prev_focus;
             client != tmp;
             client = client->prev_focus) {
            client_raise(client); /* FIXME:  optimize */
        }
        client_raise(client);
    }
    must_focus_this_client(focus_stacks[workspace_current - 1]);
}

void workspace_client_moveto(XEvent *xevent, void *v)
{
    int ws = (int)v;
    client_t *client;

    client = client_find(event_window(xevent));
    if (client == NULL) {
        fprintf(stderr,
                "XWM: can't move client to workspace %d, can't find client\n",
                ws);
        return;
    }
    if (ws < 1 || ws > NO_WORKSPACES) {
        fprintf(stderr, "XWM:  attempt to move to invalid workspace %d\n", ws);
        return;
    }
    
    if (client->workspace == ws) return;

#ifdef DEBUG
    debug(("\tMoving client 0x%08X (%s) to workspace %d\n",
           (unsigned int)client, client->name, ws));
#endif /* DEBUG */
    
    focus_remove(client, event_timestamp);
    XUnmapWindow(dpy, client->frame);
    client->workspace = ws;
    focus_add(client, event_timestamp);
}

/*
 * OK, so we have a problem:
 * 
 * When we change workspaces, we map all the windows in the workspace,
 * then we try to set the input focus to the window which should be
 * focused for that workspace.  HOWEVER, in most cases, the window
 * which we need to focus has NOT been mapped by the server (we just
 * made the request, server still has to process it).  The window is
 * not yet viewable and we get a BadMatch on XSetInputFocus (see the
 * manpage).  This screws up everything since we won't have a focused
 * window when we change workspaces (and this is not superfluous, this
 * is absolutely CRITICAL as it interrupts my work).  I've not seen a
 * window manager that correctly deals with this, they'll just force
 * you to refocus (or they'll map the window to be focused before
 * anything else, hoping to avoid the X error, which doesn't always
 * work).
 * 
 * So we postpone the XSetInputFocus call until our window is actually
 * mapped.
 */

static void must_focus_this_client(client_t *client)
{
    XEvent event;
    Bool changed_mask;

    if (client == NULL) {
        focus_set(NULL, CurrentTime);
        return;
    }
    if (!(client->frame_event_mask & StructureNotifyMask)) {
        changed_mask = True;
        client->frame_event_mask |= StructureNotifyMask;
        XSelectInput(dpy, client->frame, client->frame_event_mask);
    } else {
        changed_mask = False;
    }

    for (;;) {
        event_get(ConnectionNumber(dpy), &event);
        if (event.type == EnterNotify
            || event.type == FocusIn
            || event.type == FocusOut)
            continue;
        if (event.type == MapNotify && event.xmap.window == client->frame) {
            debug(("ATTEMPTING TO FOCUS %s\n", client->name));
            focus_set(client, CurrentTime); /* XMapEvent has no time stamp */
            focus_ensure(CurrentTime);
            break;
        } else {
            event_dispatch(&event);
        }
    }

    if (changed_mask) {
        client->frame_event_mask &= ~StructureNotifyMask;
        XSelectInput(dpy, client->frame, client->frame_event_mask);
    }
}

/* addition and subtraction without overflow */
#define SUB(x,y) ((x) < (y) ? 0 : ((x) - (y)))
#define ADD(x,y) (((((x) + (y)) > 0xFFFF) || ((x) + (y) < (x))) ? \
                  0xFFFF : ((x) + (y)))

static void alloc_workspace_colors()
{
    int i;
    XColor usable, exact;

    for (i = 0; i < NO_WORKSPACES; i++) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), workspace_colors[i],
                             &usable, &exact) == 0) {
            fprintf(stderr, "XWM: Could not get color \"%s\"\n",
                    workspace_colors[i]);
        }
        workspace_pixels[i] = usable.pixel;
        usable.flags = DoRed | DoGreen | DoBlue;
        usable.red = SUB(exact.red, 4096);
        usable.green = SUB(exact.green, 4096);
        usable.blue = SUB(exact.blue, 4096);
        if (XAllocColor(dpy, DefaultColormap(dpy, scr), &usable) == 0) {
            fprintf(stderr,
                    "XWM: Could not allocate dark highlight of color \"%s\"\n",
                    workspace_colors[i]);
        }
        workspace_dark_highlight[i] = usable.pixel;
        usable.red = SUB(exact.red, 8192);
        usable.green = SUB(exact.green, 8192);
        usable.blue = SUB(exact.blue, 8192);
        if (XAllocColor(dpy, DefaultColormap(dpy, scr), &usable) == 0) {
            fprintf(stderr,
                    "XWM: Could not allocate dark highlight of color \"%s\"\n",
                    workspace_colors[i]);
        }
        workspace_darkest_highlight[i] = usable.pixel;
        usable.red = ADD(exact.red, 4096);
        usable.green = ADD(exact.green, 4096);
        usable.blue = ADD(exact.blue, 4096);
        if (XAllocColor(dpy, DefaultColormap(dpy, scr), &usable) == 0) {
            fprintf(stderr,
                    "XWM: Could not allocate highlight of color \"%s\"\n",
                    workspace_colors[i]);
        }
        workspace_highlight[i] = usable.pixel;
    }
}

void workspace_update_color()
{
    static Bool initialized = False;
    XSetWindowAttributes xswa;

    if (!initialized) {
        alloc_workspace_colors();
        initialized = True;
    }
    xswa.background_pixel = workspace_pixels[workspace_current - 1];
    XChangeWindowAttributes(dpy, root_window, CWBackPixel, &xswa);
    XClearWindow(dpy, root_window);
}
