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
#include "ahwm.h"
#include "client.h"
#include "debug.h"
#include "malloc.h"
#include "focus.h"
#include "compat.h"

#include <stdio.h>

#include "box.xbm"
#include "down.xbm"
#include "topbar.xbm"
#include "up.xbm"
#include "wins.xbm"
#include "x.xbm"

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

typedef struct _button {
    Pixmap pixmap;
    int width;
    int height;
    struct _button *next;
} button;

#define OFFSET 4096

/* 16-bit unsigned addition and subtraction without overflow */
/* don't want hilight of pure white to be black :) */
#define SUB(x,y) ((x) < (y) ? 0 : ((x) - (y)))
#define ADD(x,y) (((((x) + (y)) > 0xFFFF) || ((x) + (y) < (x))) ? \
                  0xFFFF : ((x) + (y)))

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif /* MAX */

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif /* MIN */



static int find(unsigned long normal, unsigned long focused,
                unsigned long text, unsigned long focused_text);
static unsigned long calc(unsigned long orig, XColor exact, long offset,
                          char *color_text);

int paint_ascent;

static unsigned long *colors = NULL;  /* treated as a 2-D array */
static int nallocated = 0;   /* dimension of "colors" is "nallocated" by 8 */

static button *left_buttons;
static button *right_buttons;

/*
 * Allocates default colors
 */

void paint_init() 
{
    colors = Malloc(sizeof(unsigned long) * NCOLORS);
    if (colors == NULL) {
        perror("AHWM: Failed to allocate default colors: malloc:");
        fprintf(stderr, "AHWM: This is a fatal error, quitting.\n");
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

void paint_add_button(char *image, Bool left)
{
    button *b, *tmp;
    char *bits;

    b = malloc(sizeof(button));
    if (b == NULL) {
        perror("AHWM: add_button: malloc");
        return;
    }
    b->next = NULL;

    if (strcasecmp(image, "box") == 0) {
        bits = box_bits;
        b->width = box_width;
        b->height = box_height;
    } else if (strcasecmp(image, "down") == 0) {
        bits = down_bits;
        b->width = down_width;
        b->height = down_height;
    } else if (strcasecmp(image, "topbar") == 0) {
        bits = topbar_bits;
        b->width = topbar_width;
        b->height = topbar_height;
    } else if (strcasecmp(image, "up") == 0) {
        bits = up_bits;
        b->width = up_width;
        b->height = up_height;
    } else if (strcasecmp(image, "wins") == 0) {
        bits = wins_bits;
        b->width = wins_width;
        b->height = wins_height;
    } else if (strcasecmp(image, "x") == 0) {
        bits = x_bits;
        b->width = x_width;
        b->height = x_height;
    } else {
        return;                 /* FIXME */
    }
        
    b->pixmap = XCreateBitmapFromData(dpy, root_window, bits,
                                      b->width, b->height);

    if (left) {
        if (left_buttons == NULL) {
            left_buttons = b;
            return;
        } else {
            tmp = left_buttons;
        }
    } else {
        if (right_buttons == NULL) {
            right_buttons = b;
            return;
        } else {
            tmp = right_buttons;
        }
    }
    for (;;) {
        if (tmp->next == NULL) {
            tmp->next = b;
            return;
        }
        tmp = tmp->next;
    }
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
                "AHWM: Could not allocate %shighlight of color \"%s\"\n",
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

    debug(("\tColors:  %s, %s, %s, %s\n", normal, focused, text, focused_text));
    
    /* parse four given colors, save "exact" representation */
    if (normal != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), normal,
                             &usable, &exact) == 0) {
            fprintf(stderr, "AHWM: Could not get color \"%s\"\n", normal);
        } else {
            new_dquad[NORMAL] = usable.pixel;
            xc_normal = exact;
        }
    }
    if (focused != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), focused,
                             &usable, &exact) == 0) {
            fprintf(stderr, "AHWM: Could not get color \"%s\"\n", focused);
        } else {
            new_dquad[FOCUSED] = usable.pixel;
            xc_focused = exact;
        }
    }
    if (text != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), text,
                             &usable, &exact) == 0) {
            fprintf(stderr, "AHWM: Could not get color \"%s\"\n", text);
        } else {
            new_dquad[TEXT] = usable.pixel;
        }
    }
    if (focused_text != NULL) {
        if (XAllocNamedColor(dpy, DefaultColormap(dpy, scr), focused_text,
                             &usable, &exact) == 0) {
            fprintf(stderr, "AHWM: Could not get color \"%s\"\n", focused_text);
        } else {
            new_dquad[FOCUSED_TEXT] = usable.pixel;
        }
    }

    /* see if already know combination */
    i = find(new_dquad[NORMAL], new_dquad[FOCUSED],
             new_dquad[TEXT], new_dquad[FOCUSED_TEXT]);
    if (i != -1) {
        client->color_index = i;
        debug(("\tColor index found: %d\n", i));
        return;
    }

    /* allocate another entry */
    tmp = Realloc(colors, (nallocated + 1) * NCOLORS * sizeof(unsigned long));
    if (tmp == NULL) {
        perror("AHWM:  realloc:");
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
    debug(("\tAllocated new color entry: %d\n", nallocated-1));
    /* FIXME:  should investigate changing window's background
     * color in case X decides to show that at some point */
    /* FIXME:  might also need to repaint right now */
}

/* this is one of the few functions that is speed-critical, so it looks
 * a bit ugly because it's somewhat optimized
 * we use line segments because we want to reduce the number of calls
 * to xlib (xlib calls can be very expensive) */
void paint_titlebar(client_t *client)
{
    unsigned long middle, hilight, lowlight, text;
    XGCValues xgcv;
    int ndx, title_position, room_left, room_right, tmp;
    button *b;
    static XSegment *button_hilights = NULL, *button_lolights = NULL;
    static int nbutton_hilights = 0;
    int nhilights_used;
    static XSegment main_hilight[4] = {
        { 0, 0, 0, 0 }, { 1, 1, 1, 1 },
        { 0, 0, 0, 0 }, { 1, 1, 1, 1 } };
    static XSegment main_lolight[4] = {
        { 1, 1, 1, 1 }, { 2, 2, 2, 2 },
        { 1, 1, 1, 1 }, { 2, 2, 2, 2 } };
    XSegment *tmp_segptr;
    
    if (client == NULL || client->titlebar == None) return;

    ndx = client->color_index;
    debug(("\tColor index = %d, nallocated = %d\n", ndx, nallocated));
    if (ndx >= nallocated) {
        fprintf(stderr,
                "AHWM:  Assertion failed: client->color_index >= nallocated\n");
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

    /* using four different GCs instead of using one and
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
    xgcv.background = middle;
    XChangeGC(dpy, extra_gc4, GCForeground | GCBackground, &xgcv);
    
    XFillRectangle(dpy, client->titlebar, extra_gc1,
                   0, 0, client->width, TITLE_HEIGHT);

    /* the commented-out assignments happen only once,
     * in the static initializer */
/*     main_hilight[0].x1 = 0; */
/*     main_hilight[0].y1 = 0; */
    main_hilight[0].x2 = client->width;
/*     main_hilight[0].y2 = 0; */
    
/*     main_hilight[1].x1 = 1; */
/*     main_hilight[1].y1 = 1; */
    main_hilight[1].x2 = client->width - 2;
/*     main_hilight[1].y2 = 1; */
    
/*     main_hilight[2].x1 = 0; */
/*     main_hilight[2].y1 = 0; */
/*     main_hilight[2].x2 = 0; */
    main_hilight[2].y2 = TITLE_HEIGHT;
    
/*     main_hilight[3].x1 = 1; */
/*     main_hilight[3].y1 = 1; */
/*     main_hilight[3].x2 = 1; */
    main_hilight[3].y2 = TITLE_HEIGHT - 2;
    
/*     main_lolight[0].x1 = 1; */
    main_lolight[0].y1 = TITLE_HEIGHT - 1;
    main_lolight[0].x2 = client->width - 1;
    main_lolight[0].y2 = TITLE_HEIGHT - 1;

/*     main_lolight[1].x1 = 2; */
    main_lolight[1].y1 = TITLE_HEIGHT - 2;
    main_lolight[1].x2 = client->width - 3;
    main_lolight[1].y2 = TITLE_HEIGHT - 2;

    main_lolight[2].x1 = client->width - 1;
/*     main_lolight[2].y1 = 1; */
    main_lolight[2].x2 = client->width - 1;
    main_lolight[2].y2 = TITLE_HEIGHT - 1;

    main_lolight[3].x1 = client->width - 2;
/*     main_lolight[3].y1 = 2; */
    main_lolight[3].x2 = client->width - 2;
    main_lolight[3].y2 = TITLE_HEIGHT - 2;

    /* looks ugly, but goes fast for common case */
    room_left = room_right = 2;
    nhilights_used = 0;
    for (b = left_buttons; b != NULL; b = b->next) {
        XCopyPlane(dpy, b->pixmap, client->titlebar, extra_gc4, 0, 0,
                   b->width, MIN(b->height, TITLE_HEIGHT - 4),
                   room_left, 2, 1);
        if (nbutton_hilights <= nhilights_used) {
            tmp_segptr = Realloc(button_hilights,
                                 (nbutton_hilights + 2) * sizeof(XSegment));
            if (tmp_segptr == NULL) {
                perror("AHWM: paint_titlebar: realloc");
                continue;
            } else {
                button_hilights = tmp_segptr;
            }
            tmp_segptr = Realloc(button_lolights,
                                 (nbutton_hilights + 2) * sizeof(XSegment));
            if (tmp_segptr == NULL) {
                perror("AHWM: paint_titlebar: realloc");
                continue;
            } else {
                button_lolights = tmp_segptr;
            }
            nbutton_hilights += 2;
        }
        nhilights_used += 2;

        tmp = room_left + b->width;
        room_left += b->width + 4;

        button_lolights[nbutton_hilights - 2].x1 = tmp;
        button_lolights[nbutton_hilights - 2].y1 = 2;
        button_lolights[nbutton_hilights - 2].x2 = tmp;
        button_lolights[nbutton_hilights - 2].y2 = TITLE_HEIGHT - 2;
        tmp++;
        button_lolights[nbutton_hilights - 1].x1 = tmp;
        button_lolights[nbutton_hilights - 1].y1 = 1;
        button_lolights[nbutton_hilights - 1].x2 = tmp;
        button_lolights[nbutton_hilights - 1].y2 = TITLE_HEIGHT - 2;
        tmp++;
        button_hilights[nbutton_hilights - 2].x1 = tmp;
        button_hilights[nbutton_hilights - 2].y1 = 0;
        button_hilights[nbutton_hilights - 2].x2 = tmp;
        button_hilights[nbutton_hilights - 2].y2 = TITLE_HEIGHT;
        tmp++;
        button_hilights[nbutton_hilights - 1].x1 = tmp;
        button_hilights[nbutton_hilights - 1].y1 = 1;
        button_hilights[nbutton_hilights - 1].x2 = tmp;
        button_hilights[nbutton_hilights - 1].y2 = TITLE_HEIGHT - 2;
    }
    for (b = right_buttons; b != NULL; b = b->next) {
        XCopyPlane(dpy, b->pixmap, client->titlebar, extra_gc4, 0, 0,
                   b->width, MIN(b->height, TITLE_HEIGHT - 4),
                   client->width - room_right - b->width, 2, 1);
        if (nbutton_hilights <= nhilights_used) {
            tmp_segptr = Realloc(button_hilights,
                                 (nbutton_hilights + 2) * sizeof(XSegment));
            if (tmp_segptr == NULL) {
                perror("AHWM: paint_titlebar: realloc");
                continue;
            } else {
                button_hilights = tmp_segptr;
            }
            tmp_segptr = Realloc(button_lolights,
                                 (nbutton_hilights + 2) * sizeof(XSegment));
            if (tmp_segptr == NULL) {
                perror("AHWM: paint_titlebar: realloc");
                continue;
            } else {
                button_lolights = tmp_segptr;
            }
            nbutton_hilights += 2;
        }
        nhilights_used += 2;
        
        room_right += b->width;
        tmp = client->width - room_right - 1;
        room_right += 4;

        button_hilights[nhilights_used - 2].x1 = tmp;
        button_hilights[nhilights_used - 2].y1 = 1;
        button_hilights[nhilights_used - 2].x2 = tmp;
        button_hilights[nhilights_used - 2].y2 = TITLE_HEIGHT - 2;
        tmp--;
        button_hilights[nhilights_used - 1].x1 = tmp;
        button_hilights[nhilights_used - 1].y1 = 0;
        button_hilights[nhilights_used - 1].x2 = tmp;
        button_hilights[nhilights_used - 1].y2 = TITLE_HEIGHT;
        tmp--;
        button_lolights[nhilights_used - 2].x1 = tmp;
        button_lolights[nhilights_used - 2].y1 = 1;
        button_lolights[nhilights_used - 2].x2 = tmp;
        button_lolights[nhilights_used - 2].y2 = TITLE_HEIGHT - 2;
        tmp--;
        button_lolights[nhilights_used - 1].x1 = tmp;
        button_lolights[nhilights_used - 1].y1 = 2;
        button_lolights[nhilights_used - 1].x2 = tmp;
        button_lolights[nhilights_used - 1].y2 = TITLE_HEIGHT - 2;
    }

    room_left += 2;
    room_right += 2;
    
    if (client->title_position == DisplayLeft) {
        title_position = room_left;
    } else if (client->title_position == DisplayCentered) {
        int i = XTextWidth(fontstruct, client->name, strlen(client->name));
        title_position = (client->width - room_left - room_right) / 2 - i / 2;
        title_position += room_left;
        if (title_position + i > client->width - room_right) {
            /* don't want title to obscure buttons */
            title_position -= title_position + i - (client->width - room_right);
        }
    } else if (client->title_position == DisplayRight) {
        int i = XTextWidth(fontstruct, client->name, strlen(client->name));
        title_position = client->width - i - room_right;
    } else {
        fprintf(stderr, "AHWM: Unkown client title position %d on client %s\n",
                client->title_position, client_dbg(client));
        /* possible memory corruption, attempt to continue */
        title_position = room_left;
    }
    XDrawSegments(dpy, client->titlebar, extra_gc2,
                  main_hilight, 4);
    XDrawSegments(dpy, client->titlebar, extra_gc3,
                  main_lolight, 4);

    if (client->title_position != DontDisplay) {
        XDrawString(dpy, client->titlebar, extra_gc4, title_position,
                    paint_ascent, client->name, strlen(client->name));
    }
    XDrawSegments(dpy, client->titlebar, extra_gc2,
                  button_hilights, nhilights_used);
    XDrawSegments(dpy, client->titlebar, extra_gc3,
                  button_lolights, nhilights_used);
}
