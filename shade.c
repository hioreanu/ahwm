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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

/*
 * Resize or maximize shaded window, "unshades"
 *   - perhaps change client->height while shaded
 *   - bunch of other stuff also will cause "unshading"
 * shaded clients shouldn't get keyboard input
 *   - either focus.c knows about client->shaded or ...
 * test on really slow machines
 * option to turn off animation
 * option to change animation interval
 * EWMH probably needs to know about this
 */

typedef struct _shade_t {
    animation *anim;
    client_t *client;
    struct _shade_t *next;
} shade_t;

shade_t *shades = NULL;

static void shade_callback(float m, void *v)
{
    client_t *client = (client_t *)v;

    XResizeWindow(dpy, client->frame, client->width,
                  client->height - (client->height - TITLE_HEIGHT) * m);
    XMoveWindow(dpy, client->window, 0,
                TITLE_HEIGHT - (client->height - TITLE_HEIGHT) * m);
    if (client->titlebar != None)
        XRaiseWindow(dpy, client->titlebar);
}

static void shade_finalize(void *v)
{
    client_t *client = (client_t *)v;
    shade_t *shade, *prev;

#if 0
    /* useful for debugging animation params */
    if (client->shaded) {
        XResizeWindow(dpy, client->frame, client->width, TITLE_HEIGHT);
        XMoveWindow(dpy, client->window, 0, - client->height);
    } else {
        XResizeWindow(dpy, client->frame, client->width, client->height);
        XMoveWindow(dpy, client->window, 0, TITLE_HEIGHT);
    }
    if (client->titlebar != None)
        XRaiseWindow(dpy, client->titlebar);
#endif

    prev = NULL;
    for (shade = shades; shade != NULL; shade = shade->next) {
        if (shade->client == client) {
            if (prev) {
                prev->next = shade->next;
            }
            if (shades == shade) {
                shades = shade->next;
            }
            free(shade);
            return;
        }
        prev = shade;
    }
}

void shade(XEvent *e, arglist *ignored)
{
    client_t *client;
    shade_t *shade;

    client = client_find(e->xbutton.window);
    if (client == NULL) return;

    client->shaded = client->shaded ? 0 : 1;
    
    for (shade = shades; shade != NULL; shade = shade->next) {
        if (shade->client == client) {
            /* client is already in some sort of shading action */
            animation_reverse(shade->anim);
            return;
        }
    }

    shade = malloc(sizeof(shade_t));
    if (!shade) {
        perror("AHWM: Cannot shade window: malloc");
        return;
    }
    shade->anim = animate(shade_callback, shade_finalize, (void *)client);
    if (client->shaded == 0) {
        animation_start_backwards(shade->anim);
    }
    shade->client = client;
    shade->next = shades;
    shades = shade;
}
