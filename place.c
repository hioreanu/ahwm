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

#include "place.h"
#include "workspace.h"
#include "debug.h"
#include "focus.h"
#include "stacking.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

struct xyclient {
    int x;
    int y;
    client_t *client;
};

static Bool place_corner_helper(client_t *client, void *v)
{
    struct xyclient *xyclient = (struct xyclient *)v;
    
    if (client != xyclient->client
        && client->x <= xyclient->x
        && client->y <= xyclient->y
        && client->x + client->width >= xyclient->x
        && client->y + client->height >= xyclient->y)
        return False;
    return True;
}

/* try to place in specified corner if possible */
static Bool place_corner(client_t *client, int x, int y)
{
    struct xyclient xyclient;

    xyclient.x = x;
    xyclient.y = y;
    xyclient.client = client;
    if (focus_forall(place_corner_helper, (void *)&xyclient) == False)
        return False;

    debug(("\tplacing in corner %d,%d\n", x, y));
    client->x = x;
    client->y = y;
    if (x == scr_width)
        client->x -= client->width;
    if (y == scr_height)
        client->y -= client->height;
    return True;
}

/*
 * Finds the overlap between two clients.
 * 
 * The overlap between a newly-mapped window and all existing clients
 * is found by simply summing the result of this function across the
 * new window crossed with all existing windows.  This is not the
 * correct overlap as it counts some overlaps twice if the existing
 * windows were already overlapping under the new position.  However,
 * finding the "correct" overlap takes too much time, and in practice,
 * this actually "feels" better.
 */

static int find_overlap(client_t *one, client_t *two)
{
    int x1, x2, y1, y2;

    x1 = MAX(one->x, two->x);
    x2 = MIN(one->x + one->width, two->x + two->width);
    if (x2 - x1 <= 0) return 0;
    y1 = MAX(one->y, two->y);
    y2 = MIN(one->y + one->height, two->y + two->height);
    if (y2 - y1 <= 0) return 0;
    return (y2 - y1) * (x2 - x1);
}

/*
 * Algorithm works as follows:
 * 
 * For each client A
 *     y = A.top
 *     For each client B
 *         x = B.left
 *         Try to place at x, y
 *         x = B.right
 *         Try to place at x, y
 *     y = A.bottom
 *     For each client B
 *         x = B.left
 *         Try to place at x, y
 *         x = B.right
 *         Try to place at x, y
 * 
 * The position that wins is the position that creates the least
 * amount of overlap in the windows.  Ties are settled by least y,
 * then by least x.  The actual code looks a bit different because the
 * 'Try to place' step includes calculating the overlap, which is
 * expensive.  This is only called when each of the four corners has a
 * window in it, so we examine the sides of the screen just as we
 * examine the sides of each window.
 * 
 * This algorithm is "correct" in that it will choose the position
 * with the least amount of overlap out of all the possible positions.
 * Proof is long and tedious; contact me if you're pedantic and you
 * want the proof.
 * 
 * Other window managers use a similar algorithm (perhaps with some
 * additional steps to shortcut if a zero-overlap position is found)
 * or they use some heuristic that's a bit faster but doesn't satisfy
 * the least overlap condition.  The triple-loop looks ugly, but it
 * runs in favorable time on every slow machine that I've tried (there
 * are no function calls in any of this, this is just straight
 * calculation).
 * 
 * FIXME:  need to test this on a slower machine (like a 486 or
 * pentium 1), works fine on all my machines, but I still don't feel
 * OK with it
 */

void place_least_overlap(client_t *client)
{
    client_t *A, *B, *C;
    int overlap_final, x_final, y_final, overlap_test, x_test, y_test;
    int max_x, max_y;
    int state;

/* flags for the state variable */
#define SEEN_TOP    1
#define SEEN_BOTTOM 2
#define SEEN_LEFT   4
#define SEEN_RIGHT  8

    max_x = scr_width - client->width;
    max_y = scr_height - client->height;
    overlap_final = scr_height * scr_width + 1;
    x_final = client->x;
    y_final = client->y;

    state = 0;
    
    A = stacking_top();
    for (;;) {
        /* find next interesting y position */
        y_test = -1;
        while (y_test < 0 || y_test > max_y
               || A->workspace != workspace_current
               || A->state != NormalState) {
            if (! (state & SEEN_TOP)) {
                /* haven't seen top */
                y_test = A->y;
                state |= SEEN_TOP;
            } else if (! (state & SEEN_BOTTOM)) {
                /* haven't seen bottom */
                y_test = A->y + A->height;
                state |= SEEN_BOTTOM;
            } else {
                /* seen both top and bottom */
                A = A->prev_stacking;
                state &= ~(SEEN_TOP | SEEN_BOTTOM);
                if (A == NULL) break;
            }
        }
        if (A == NULL) break;
        
        B = stacking_top();
        for (;;) {
            /* find next interesting x position */
            x_test = -1;
            while (x_test < 0 || x_test > max_x
                   || B->workspace != workspace_current
                   || B->state != NormalState) {
                if (! (state & SEEN_LEFT)) {
                    x_test = B->x;
                    state |= SEEN_LEFT;
                } else if (! (state & SEEN_RIGHT)) {
                    x_test = B->x + B->width;
                    state |= SEEN_RIGHT;
                } else {
                    B = B->prev_stacking;
                    state &= ~(SEEN_LEFT | SEEN_RIGHT);
                    if (B == NULL) break;
                }
            }
            if (B == NULL) break;

            /* calculate the overlap */
            overlap_test = 0;
            client->x = x_test;
            client->y = y_test;
            for (C = stacking_top(); C != NULL; C = C->prev_stacking) {
                if (C->workspace == workspace_current
                    && C->state == NormalState) {
                    overlap_test += find_overlap(C, client);
                }
            }
            /* set the final x, y to the x, y which have the smallest
             * overlap, sorted by y then by x */
            if (overlap_test < overlap_final
                || (overlap_test == overlap_final
                    && (y_test < y_final
                        || (y_test == y_final
                            && x_test < x_final)))) {
                overlap_final = overlap_test;
                x_final = x_test;
                y_final = y_test;
            }
        }
    }
    debug(("\tplacing at %d,%d\n", x_final, y_final));
    client->x = x_final;
    client->y = y_final;
}

void place(client_t *client)
{
    int orig_x, orig_y;

    orig_x = client->x;
    orig_y = client->y;
    
    if (!place_corner(client, 0, 0)
        && !place_corner(client, scr_width, 0)
        && !place_corner(client, 0, scr_height)
        && !place_corner(client, scr_width, scr_height)) {
        place_least_overlap(client);
    }
    if (orig_x != client->x || orig_y != client->y)
        XMoveWindow(dpy, client->frame, client->x, client->y);
}
