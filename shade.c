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

#include "shade.h"
#include "animation.h"
#include "client.h"

/*
 * Resize or maxmimize shaded window, "unshades"
 *   - perhaps change client->height while shaded
 *   - bunch of other stuff also will cause "unshading"
 * keep clicking and invoking action, should go smoothly
 *   - should have an animation_t, animation_cancel
 * should also move client window within frame
 * shaded clients shouldn't get keyboard input
 *   - either focus.c knows about client->shaded or ...
 * test on really slow machines
 * option to turn off animation
 * option to change animation interval
 */

static void shade_callback(float m, void *v)
{
    client_t *client = (client_t *)v;

    if (client->shaded) {
        XResizeWindow(dpy, client->frame, client->width,
                      client->height - (client->height - TITLE_HEIGHT) * m);
    } else {
        XResizeWindow(dpy, client->frame, client->width,
                      TITLE_HEIGHT + (client->height - TITLE_HEIGHT) * m);
    }
}

void shade(XEvent *e, arglist *ignored)
{
    client_t *client;

    client = client_find(e->xbutton.window);
    if (client == NULL) return;
    client->shaded = client->shaded ? 0 : 1;
    animate(shade_callback, NULL, (void *)client);
}
