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
#include "stacking.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

unsigned long workspace_pixels[NO_WORKSPACES] = { 0 };
unsigned long workspace_dark_highlight[NO_WORKSPACES];
unsigned long workspace_darkest_highlight[NO_WORKSPACES];
unsigned long workspace_highlight[NO_WORKSPACES];

unsigned int workspace_current = 1;

static char *workspace_colors[NO_WORKSPACES] = {
    "#404040",                  /* dark gray */
    "#2F4F4F",                  /* blue-greenish */
    "#000050",                  /* blue */
    "#500000",                  /* red */
    "#500050",                  /* violet */
    "#005000",                  /* green */
    "#101010"                   /* black */
};

static void alloc_workspace_colors();

void workspace_goto_bindable(XEvent *e, void *v)
{
    unsigned int new_workspace = (int)v; /* this is always safe */

    workspace_goto(new_workspace);
}

static Bool unmap(client_t *client, void *v)
{
    XSetWindowAttributes xswa;
    
    if (client->prefs.omnipresent) {
        client->workspace = (unsigned int)v;
        if (client->titlebar != None) {
            xswa.background_pixel =
                workspace_darkest_highlight[client->workspace - 1];
            XChangeWindowAttributes(dpy, client->titlebar, CWBackPixel, &xswa);
        }
    } else {
        XUnmapWindow(dpy, client->frame);
        debug(("\tUnmapping frame 0x%08X in workspace_goto\n",
               client->frame));
        ewmh_client_list_remove(client);
    }
    return True;
}

static Bool map(client_t *client, void *v)
{
    debug(("\tRemapping 0x%08X ('%.10s')\n", client, client->name));
    XMapWindow(dpy, client->frame);
    return True;
}

/*
 * we allow changing to the current workspace, basically has same
 * effect as an 'xrefresh'
 */

void workspace_goto(unsigned int new_workspace)
{
    XSetWindowAttributes xswa;

    if (new_workspace < 1 || new_workspace > NO_WORKSPACES) {
        fprintf(stderr, "XWM:  attempt to go to invalid workspace %d\n",
                new_workspace);
        return;
    }

    debug(("\tGoing to workspace %d\n", new_workspace));

    /* When we unmap the windows in order to change workspaces,
     * sometimes it is possible to see the actual unmappings as
     * they happen, especially when the server is stressed or
     * the windows have contrasting colors; therefore we map a
     * temporary window to cover up our actions.
     * Little things like this make a big difference. */
    xswa.background_pixel = workspace_pixels[workspace_current - 1];
    xswa.override_redirect = True;
    stacking_hiding_window = XCreateWindow(dpy, root_window, 0, 0,
                                           scr_width, scr_height,
                                           0, DefaultDepth(dpy, scr),
                                           InputOutput,
                                           DefaultVisual(dpy, scr),
                                           CWBackPixel | CWOverrideRedirect,
                                           &xswa);
    XMapRaised(dpy, stacking_hiding_window);
    XClearWindow(dpy, stacking_hiding_window);
    
    /* unmap windows in current workspace */
    focus_forall(unmap, (void *)new_workspace);
    
    workspace_current = new_workspace;
    workspace_update_color();

    /* map windows in new workspace */
    focus_forall(map, NULL);
    
    XUnmapWindow(dpy, stacking_hiding_window);
    XDestroyWindow(dpy, stacking_hiding_window);
    stacking_hiding_window = None;

    focus_workspace_changed(event_timestamp);
    ewmh_current_desktop_update();
}

void move_with_transients(client_t *client, unsigned int ws)
{
    client_t *transient;
    XSetWindowAttributes xswa;
    
    for (transient = client->transients;
         transient != NULL;
         transient = transient->next_transient) {
        if (transient->workspace == client->workspace) {
            move_with_transients(transient, ws);
        }
    }
    debug(("\tMoving window 0x%08X ('%.10s') to workspace %d\n",
           client->window, client->name, ws));
    xswa.background_pixel =
        workspace_darkest_highlight[ws - 1];
    XChangeWindowAttributes(dpy, client->titlebar, CWBackPixel, &xswa);
    focus_remove(client, event_timestamp);
    ewmh_client_list_remove(client);
    debug(("\tUnmapping frame 0x%08X in workspace move\n", client->frame));
    XUnmapWindow(dpy, client->frame);
    client->workspace = ws;
    focus_add(client, event_timestamp);
}

void workspace_client_moveto_bindable(XEvent *xevent, void *v)
{
    unsigned int ws = (int)v;
    client_t *client;

    client = client_find(event_window(xevent));
    if (client == NULL) {
        fprintf(stderr,
                "XWM: can't move client to workspace %d, can't find client\n",
                ws);
        return;
    }
    workspace_client_moveto(client, ws);
}

void workspace_client_moveto(client_t *client, unsigned int ws)
{
    if (ws < 1 || ws > NO_WORKSPACES) {
        fprintf(stderr, "XWM:  attempt to move to invalid workspace %d\n", ws);
        return;
    }
    
    if (client->workspace == ws) return;

    /* we now move the client's transients which are in the same
     * workspace as the client to the new workspace
     * FIXME:  try to ensure the focus list remains somewhat
     * intact as we move the transients */

    move_with_transients(client, ws);
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
