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
#include "ewmh.h"

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
        ewmh_client_list_remove(client);
        tmp = client;
        for (client = tmp->next_focus;
             client != tmp;
             client = client->next_focus) {
            XUnmapWindow(dpy, client->frame);
            ewmh_client_list_remove(client);
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
            client_raise(client);
//            ewmh_client_list_add(client);
        }
        client_raise(client);
//        ewmh_client_list_add(client);
    }
    focus_current = focus_stacks[workspace_current - 1];
    focus_ensure(event_timestamp);
    ewmh_current_desktop_update();
//    must_focus_this_client(focus_stacks[workspace_current - 1]);
}

void workspace_client_moveto(XEvent *xevent, void *v)
{
    int ws = (int)v;
    client_t *client, *transient;

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

    /* we now move the client's transients which are in the same
     * workspace as the client to the new workspace
     * FIXME:  try to ensure the focus list remains somewhat
     * intact as we move the transients */

    for (transient = client->transients;
         transient != NULL;
         transient = transient->next_transient) {
        if (transient->workspace == client->workspace) {
            focus_remove(transient, event_timestamp);
            ewmh_client_list_remove(transient);
            XUnmapWindow(dpy, transient->frame);
            transient->workspace = ws;
            focus_add(transient, event_timestamp);
        }
    }

    focus_remove(client, event_timestamp);
    ewmh_client_list_remove(client);
    XUnmapWindow(dpy, client->frame);
    client->workspace = ws;
    focus_add(client, event_timestamp);
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
