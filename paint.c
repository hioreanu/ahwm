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

#include "paint.h"
#include "xwm.h"
#include "client.h"
#include "debug.h"
#include "malloc.h"
#include "focus.h"

#include <stdio.h>

/*
 * Color allocation only happens on startup, so we don't have to be
 * too clever here.  Just use a simple array, realloc() when needed
 * and do linear search to look up entries.  When it matters, each
 * color is only a couple pointer indirections away.  Hilights are
 * calculated using a fixed offset.
 */

enum {
    NORMAL = 0,
    HILIGHT,
    LOLIGHT,
    FOCUSED,
    FOCUSED_HILIGHT,
    FOCUSED_LOLIGHT,
    TEXT,
    FOCUSED_TEXT
};

#define NCOLORS 8

#define OFFSET 4096

/* 16-bit unsigned addition and subtraction without overflow */
/* don't want hilight of pure white to be black :) */
#define SUB(x,y) ((x) < (y) ? 0 : ((x) - (y)))
#define ADD(x,y) (((((x) + (y)) > 0xFFFF) || ((x) + (y) < (x))) ? \
                  0xFFFF : ((x) + (y)))

static int find(unsigned long normal, unsigned long focused,
                unsigned long text, unsigned long focused_text);
static unsigned long calc(unsigned long orig, XColor exact, long offset,
                          char *color_text);

unsigned long *colors = NULL;  /* treated as a 2-D array */
int nallocated = 0;            /* dimension of "colors" is "nallocated" by 8 */

/*
 * Allocates default colors
 */

void paint_init() 
{
    colors = Malloc(sizeof(unsigned long) * NCOLORS);
    if (colors == NULL) {
        perror("XWM: Failed to allocate default colors: malloc:");
        fprintf(stderr, "XWM: This is a fatal error, quitting.\n");
        exit(1);
    }

    nallocated = 1;
    
    /* default colors are black and white, no 3-D look */
    /* (this is index zero) */
    colors[NORMAL] = black;
    colors[HILIGHT] = black;
    colors[LOLIGHT] = black;
    colors[TEXT] = white;
    colors[FOCUSED] = white;
    colors[FOCUSED_HILIGHT] = white;
    colors[FOCUSED_LOLIGHT] = white;
    colors[FOCUSED_TEXT] = black;
}

/* see if values already seen */
static int find(unsigned long normal, unsigned long focused,
                unsigned long text, unsigned long focused_text)
{
    int i;

    for (i = 0; i < nallocated; i++) {
        if (normal == colors[i*NCOLORS + NORMAL] &&
            focused == colors[i*NCOLORS + FOCUSED] &&
            text == colors[i*NCOLORS + TEXT] &&
            focused_text == colors[i*NCOLORS + FOCUSED_TEXT]) {
            return i;
        }
    }
    return -1;
}

/*
 * calculates hilights given original color pixel value, an "exact"
 * XColor value, etc.  This is kind of ugly, but the behaviour is
 * correct.
 */
static unsigned long calc(unsigned long orig, XColor exact, long offset,
                          char *color_text)
{
    XColor usable;

    usable.flags = DoRed | DoBlue | DoGreen;
    if (offset < 0) {
        usable.red = SUB(exact.red, OFFSET);
        usable.green = SUB(exact.green, OFFSET);
        usable.blue = SUB(exact.blue, OFFSET);
    } else {
        usable.red = ADD(exact.red, OFFSET);
        usable.green = ADD(exact.green, OFFSET);
        usable.blue = ADD(exact.blue, OFFSET);
    }
    if (XAllocColor(dpy, DefaultColormap(dpy, scr), &usable) == 0) {
        fprintf(stderr,
                "XWM: Could not allocate %shighlight of color \"%s\"\n",
                (offset < 0 ? "dark " : ""),
                color_text);
        return orig;
    }
    return usable.pixel;
}

void paint_calculate_colors(client_t *client, char *normal,
                            char *focused, char *text, char *focused_text)
{
    int i;
    unsigned long new_dquad[NCOLORS];
    unsigned long *tmp;
    XColor xc_normal, xc_focused;
    XColor usable, exact;

    /* set defaults (using index zero of colors array) */
    memcpy(&new_dquad, colors, NCOLORS * sizeof(unsigned long));

    xc_normal.red = 0xFFFF;
    xc_normal.blue = 0xFFFF;
    xc_normal.green = 0xFFFF;
    xc_focused.red = 0;
    xc_focused.blue = 0;
    xc_focused.green = 0;

    debug(("%s, %s, %s, %s\n", normal, focused, text, focused_text));
    
    /* parse four given colors, save "exact" representation */
    if (normal != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), normal,
                             &usable, &exact) == 0) {
            fprintf(stderr, "XWM: Could not get color \"%s\"\n", normal);
        } else {
            new_dquad[NORMAL] = usable.pixel;
            xc_normal = exact;
        }
    }
    if (focused != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), focused,
                             &usable, &exact) == 0) {
            fprintf(stderr, "XWM: Could not get color \"%s\"\n", focused);
        } else {
            new_dquad[FOCUSED] = usable.pixel;
            xc_focused = exact;
        }
    }
    if (text != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), text,
                             &usable, &exact) == 0) {
            fprintf(stderr, "XWM: Could not get color \"%s\"\n", text);
        } else {
            new_dquad[TEXT] = usable.pixel;
        }
    }
    if (focused_text != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), focused_text,
                             &usable, &exact) == 0) {
            fprintf(stderr, "XWM: Could not get color \"%s\"\n", focused_text);
        } else {
            new_dquad[FOCUSED_TEXT] = usable.pixel;
        }
    }

    /* see if already know combination */
    i = find(new_dquad[NORMAL], new_dquad[FOCUSED],
             new_dquad[TEXT], new_dquad[FOCUSED_TEXT]);
    if (i != -1) {
        client->color_index = i;
        debug(("Found it: %d\n", i));
        return;
    }

    /* allocate another entry */
    tmp = Realloc(colors, (nallocated + 1) * NCOLORS * sizeof(unsigned long));
    if (tmp == NULL) {
        perror("XWM:  realloc:");
        client->color_index = 0;
        return;
    }
    colors = tmp;

    /* calculate the hilights */
    new_dquad[HILIGHT] = calc(new_dquad[NORMAL], xc_normal, OFFSET, normal);
    new_dquad[LOLIGHT] = calc(new_dquad[NORMAL], xc_normal, -OFFSET, normal);
    new_dquad[FOCUSED_HILIGHT] = calc(new_dquad[FOCUSED], xc_focused,
                                      OFFSET, focused);
    new_dquad[FOCUSED_LOLIGHT] = calc(new_dquad[FOCUSED], xc_focused,
                                      -OFFSET, focused);

    memcpy(&colors[nallocated * NCOLORS],
           &new_dquad,
           NCOLORS * sizeof(unsigned long));
    client->color_index = nallocated++;
    debug(("Allocated new entry: %d\n", nallocated-1));
    /* FIXME:  should investigate changing window's background
     * color in case X decides to show that at some point */
    /* FIXME:  might also need to repaint right now */
}

void paint_titlebar(client_t *client)
{
    unsigned long middle, hilight, lowlight, text;
    XGCValues xgcv;
    int ndx;

    if (client == NULL || client->titlebar == None) return;

    ndx = client->color_index;
    debug(("index = %d, nallocated = %d\n", ndx, nallocated));
    if (ndx >= nallocated) {
        fprintf(stderr,
                "XWM:  Assertion failed: client->color_index >= nallocated\n");
        /* *client most likely corrupted, but attempt to continue anyway */
        ndx = 0;
    }
    
    if (client == focus_current && client->focus_policy != DontFocus) {
        middle = colors[ndx * NCOLORS + FOCUSED];
        hilight = colors[ndx * NCOLORS + FOCUSED_HILIGHT];
        lowlight = colors[ndx * NCOLORS + FOCUSED_LOLIGHT];
        text = colors[ndx * NCOLORS + FOCUSED_TEXT];
    } else {
        middle = colors[ndx * NCOLORS + NORMAL];
        hilight = colors[ndx * NCOLORS + HILIGHT];
        lowlight = colors[ndx * NCOLORS + LOLIGHT];
        text = colors[ndx * NCOLORS + TEXT];
    }

    /* using three different GCs instead of using one and
     * continually changing its values may or may not be faster
     * (depending on hardware) according to the Xlib docs, but
     * this seems to reduce flicker when moving windows on my
     * hardware */

    xgcv.foreground = middle;
    XChangeGC(dpy, extra_gc1, GCForeground, &xgcv);
    xgcv.foreground = hilight;
    XChangeGC(dpy, extra_gc2, GCForeground, &xgcv);
    xgcv.foreground = lowlight;
    XChangeGC(dpy, extra_gc3, GCForeground, &xgcv);
    xgcv.foreground = text;
    XChangeGC(dpy, extra_gc4, GCForeground, &xgcv);
    
    XFillRectangle(dpy, client->titlebar, extra_gc1,
                   0, 0, client->width, TITLE_HEIGHT);
        
    XDrawLine(dpy, client->titlebar, extra_gc2, 0, 0,
              client->width, 0);
    XDrawLine(dpy, client->titlebar, extra_gc2, 1, 1,
              client->width - 2, 1);
    XDrawLine(dpy, client->titlebar, extra_gc2,
              0, 0, 0, TITLE_HEIGHT);
    XDrawLine(dpy, client->titlebar, extra_gc2,
              1, 1, 1, TITLE_HEIGHT - 2);

    XDrawLine(dpy, client->titlebar, extra_gc3,
              1, TITLE_HEIGHT - 1, client->width - 1, TITLE_HEIGHT - 1);
    XDrawLine(dpy, client->titlebar, extra_gc3,
              2, TITLE_HEIGHT - 2, client->width - 3, TITLE_HEIGHT - 2);
    XDrawLine(dpy, client->titlebar, extra_gc3,
              client->width - 1, 1, client->width - 1, TITLE_HEIGHT - 2);
    XDrawLine(dpy, client->titlebar, extra_gc3,
              client->width - 2, 2, client->width - 2, TITLE_HEIGHT - 2);

    XDrawString(dpy, client->titlebar, extra_gc4, 2, TITLE_HEIGHT - 4,
                client->name, strlen(client->name));
}
