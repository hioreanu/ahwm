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

/*
 * Nothing here is particularly difficult, but I'm a bit unsure if
 * this is the correct behaviour.  All my machines usually run at 24-
 * or 32-bit depth, so this doesn't get much testing.  In addition,
 * I've seen no application that uses the ICCCM 2.0 colormap
 * installation stuff.
 * 
 * I also don't trust the ICCCM colormap installation stuff, so we
 * reset the colormap installation policy whenever a different client
 * is to get a colormap.
 */

#include "config.h"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "colormap.h"
#include "client.h"
#include "focus.h"

Atom WM_COLORMAP_WINDOWS = None;

static Atom WM_COLORMAP_NOTIFY = None;
static Window icccm_colormap_policy_stealer = None;

/*
 * In this function, we should also
 * 
 * "...ensure that the root window's colormap field contains a
 * colormap that is suitable for clients to inherit.  In particular,
 * the colormap will provide distinguishable colors for BlackPixel and
 * WhitePixel."   (ICCCM 2.0, 4.1.8)
 * 
 * Yeah, right.
 */

void colormap_init()
{
    WM_COLORMAP_WINDOWS = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
    WM_COLORMAP_NOTIFY = XInternAtom(dpy, "WM_COLORMAP_NOTIFY", False);
}

/*
 * We keep track of the client's WM_COLORMAP_WINDOWS property.  We
 * could instead keep track of the colormaps of those windows and
 * select for colormap change events on those windows to avoid the
 * (potentially slow) XGetWindowAttributes loop.  However, nobody uses
 * this.
 */

void colormap_install(client_t *client)
{
    XWindowAttributes xwa;
    Bool installed_main_colormap;
    int i;

    if (client->window == icccm_colormap_policy_stealer)
        return;
    icccm_colormap_policy_stealer = None;
    if (client->colormap_windows == NULL) {
        if (client->colormap != None)
            XInstallColormap(dpy, client->colormap);
        return;
    }
    installed_main_colormap == False;
    for (i = 0; i < client->ncolormap_windows; i++) {
        if (XGetWindowAttributes(dpy, client->colormap_windows[i], &xwa) == 0)
            continue;
        if (xwa.colormap == client->colormap)
            installed_main_colormap = True;
        if (xwa.colormap != None)
            XInstallColormap(dpy, xwa.colormap);
    }
    if (installed_main_colormap == False) {
        if (client->colormap != None)
            XInstallColormap(dpy, client->colormap);
    }
}

void colormap_update_windows_property(client_t *client)
{
    Atom actual;
    int fmt;
    long bytes_after_return;
    
    if (client == NULL)
        return;
    if (client->colormap_windows != NULL)
        XFree(client->colormap_windows);
    if (XGetWindowProperty(dpy, client->window, WM_COLORMAP_WINDOWS, 0,
                           sizeof(Window), False, XA_WINDOW,
                           &actual, &fmt, &client->ncolormap_windows,
                           &bytes_after_return,
                           (unsigned char **)&client->colormap_windows) == 0) {
        if (client->colormap_windows != NULL)
            XFree(client->colormap_windows);
        client->ncolormap_windows = 0;
        return;
    }
    if (fmt != 32 || actual != XA_ATOM) {
        if (client->colormap_windows != NULL)
            XFree(client->colormap_windows);
        client->ncolormap_windows = 0;
    }
    fprintf(stderr, "Client %s is using WM_COLORMAP_WINDOWS\n",
            client->name);
    if (client == focus_current)
        colormap_install(client);
}

Bool colormap_handle_clientmessage(XClientMessageEvent *xevent)
{
    client_t *client;
    
    if (xevent->message_type == WM_COLORMAP_NOTIFY &&
        xevent->format == 32) {

        client = client_find(xevent->window);
        /* I've never seen this code running, so I want to hear
         * about it if someone uses this. */
        fprintf(stderr,
                "XWM: Client '%s' is using ICCCM 2.0 colormap "
                "installation procedures.\n"
                "Please contact hioreanu+ahwm@uchicago.edu\n",
                client == NULL ? "unknown" : client->name);
        if (xevent->data.l[1] == 0) {
            icccm_colormap_policy_stealer = None;
        } else {
            icccm_colormap_policy_stealer = xevent->window;
        }
        return True;
    } else {
        return False;
    }
}
