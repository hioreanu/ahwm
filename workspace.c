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

#include <X11/Xlib.h>
#include <stdio.h>

#include "workspace.h"
#include "focus.h"
#include "event.h"
#include "debug.h"
#include "ewmh.h"
#include "stacking.h"

unsigned int nworkspaces = 1;
unsigned int workspace_current = 1;

void workspace_goto_bindable(XEvent *e, arglist *args)
{
    if (args != NULL && args->arglist_arg->type_type == INTEGER) {
        workspace_goto(args->arglist_arg->type_value.intval);
    } else {
        fprintf(stderr, "AHWM: type error\n"); /* FIXME */
    }
}

static Bool unmap(client_t *client, void *v)
{
    XSetWindowAttributes xswa;
    
    if (client->omnipresent) {
        client->workspace = (unsigned int)v;
    } else {
        XUnmapWindow(dpy, client->frame);
        debug(("\tUnmapping frame %#lx in workspace_goto\n",
               client->frame));
        ewmh_client_list_remove(client);
    }
    return True;
}

static Bool map(client_t *client, void *v)
{
    debug(("\tRemapping %#lx ('%.10s')\n", client, client->name));
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

    if (new_workspace < 1 || new_workspace > nworkspaces) {
        fprintf(stderr, "AHWM:  attempt to go to invalid workspace %d\n",
                new_workspace);
        return;
    }

    debug(("\tGoing to workspace %d\n", new_workspace));

    /* When we unmap the windows in order to change workspaces,
     * sometimes it is possible to see the actual unmappings as
     * they happen, especially when the server is stressed or
     * the windows have contrasting colors; therefore we map a
     * temporary window to cover up our actions. */
    xswa.override_redirect = True;
    xswa.background_pixmap = ParentRelative;
    stacking_hiding_window = XCreateWindow(dpy, root_window, 0, 0,
                                           scr_width, scr_height,
                                           0, DefaultDepth(dpy, scr),
                                           InputOutput,
                                           DefaultVisual(dpy, scr),
                                           CWBackPixmap | CWOverrideRedirect,
                                           &xswa);
    XMapRaised(dpy, stacking_hiding_window);
    XClearWindow(dpy, stacking_hiding_window);
    
    /* unmap windows in current workspace */
    focus_forall(unmap, (void *)new_workspace);
    
    workspace_current = new_workspace;

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

    if (client->keep_transients_on_top) {
        for (transient = client->transients;
             transient != NULL;
             transient = transient->next_transient) {
            if (transient->workspace == client->workspace) {
                move_with_transients(transient, ws);
            }
        }
    }
    debug(("\tMoving window %#lx ('%.10s') to workspace %d\n",
           client->window, client->name, ws));
    
    focus_remove(client, event_timestamp);
    debug(("\tUnmapping frame %#lx in workspace move\n", client->frame));
    XUnmapWindow(dpy, client->frame);
    client->workspace = ws;
    ewmh_desktop_update(client);
    prefs_apply(client);
    focus_add(client, event_timestamp);
}

void workspace_client_moveto_bindable(XEvent *xevent, arglist *args)
{
    unsigned int ws;
    client_t *client;

    client = client_find(event_window(xevent));
    if (args != NULL && args->arglist_arg->type_type == INTEGER) {
        ws = args->arglist_arg->type_value.intval;
    } else {
        fprintf(stderr, "AHWM: type error\n"); /* FIXME */
        return;
    }
    if (client == NULL) {
        fprintf(stderr,
                "AHWM: can't move client to workspace %d, can't find client\n",
                ws);
        return;
    }
    workspace_client_moveto(client, ws);
}

void workspace_client_moveto(client_t *client, unsigned int ws)
{
    if (ws < 1 || ws > nworkspaces) {
        fprintf(stderr, "AHWM:  attempt to move to invalid workspace %d\n", ws);
        return;
    }
    
    if (client->workspace == ws) return;

    /* we now move the client's transients which are in the same
     * workspace as the client to the new workspace */

    move_with_transients(client, ws);
}
