/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "mouse.h"
#include "client.h"
#include "cursor.h"
#include "event.h"
#include <stdio.h>

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

/*
 * These are the possible directions in which one can resize a window,
 * depending on which direction the mouse moves at first
 * 
 * UNKNOWN is obviously a marker to say the direction hasn't yet been
 * identified
 * 
 * NB:  we will never actually constrain the resizing to only one
 * dimension (ie, simply North or South, I find that annoying); the
 * NORTH, SOUTH, EAST, WEST directions are simply to indicate that
 * only one component of the resizing direction has been figured out
 */

typedef enum {
    NW = 0, NE, SE, SW, NORTH, SOUTH, EAST, WEST, UNKNOWN
} resize_direction_t;

/*
 * One of the four quadrants of a window like in plane geometry
 */

typedef enum {
    I = 1, II, III, IV
} quadrant_t;

static XEvent *compress_motion(XEvent *xevent);
static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           quadrant_t quadrant,
                           int *old_x, int *old_y);
static quadrant_t get_quadrant(client_t *client, int x, int y);

/*
 * WindowMaker grabs the server while processing the motion events for
 * moving a window; that seems a bit antisocial, so we won't do that.
 * Window movement is opaque.
 * 
 * mouse_move:
 * select only one window, the one under the pointer
 * Grab the pointer
 * Grab the keyboard
 * Grab the server
 * 
 * use XCheckMaskEvent to compress motion events
 * ButtonMotionMask, ButtonReleaseMask, ButtonPressMask
 */

void mouse_grab_buttons(Window w)
{
    XGrabButton(dpy, Button1, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_moving);
    XGrabButton(dpy, Button3, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_normal);
}

/*
 * This takes over the event loop and grabs the pointer until the move
 * is complete
 */
/* FIXME:  need to send a synthetic event according to ICCCM */
void mouse_move_client(XEvent *xevent)
{
    static client_t *client = NULL;
    static int x_start, y_start;
    XEvent event1;

    do {
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
                /* we have a separate event loop here because we want
                 * to gobble up all EnterNotify and LeaveNotify events
                 * while moving or resizing the window */
                break;
            case ButtonPress:
                if (xevent->xbutton.button == Button1) {
                    if (client != NULL) {
                        /* Received button1 down while moving a window */
                        fprintf(stderr, "Unexpected Button1 down\n");
                        goto reset;
                    } else if (xevent->xbutton.state & Mod1Mask) {
                        /* received normal Mod1 + Button1 press */
                        client = client_find(xevent->xbutton.window);
                        if (client == NULL) {
                            /* error - we should never do XGrabButton() on
                             * an override_redirect window */
                            fprintf(stderr, "Not moving a non-client\n");
                            return;
                        }
                        /* I kind of like being able to move a window
                         * without giving it focus, and this doesn't
                         * cause any problems: */
                        /* focus_set(client); */
                        /* focus_ensure(); */
                        x_start = xevent->xbutton.x_root;
                        y_start = xevent->xbutton.y_root;
#ifdef DEBUG
                        printf("\tGrabbing the mouse for moving\n");
#endif /* DEBUG */
                        XGrabPointer(dpy, root_window, True,
                                     PointerMotionMask | ButtonPressMask |
                                     ButtonReleaseMask,
                                     GrabModeAsync, GrabModeAsync, None,
                                     cursor_moving, CurrentTime);
                    } else {
                        /* error - we were called from somewhere else
                         * to deal with a button press that's not ours */
                        fprintf(stderr,
                                "XWM: Received an unknown button press\n");
                    }
                } else {
                    /* not an error - user may have clicked another button
                     * while resizing a window */
#ifdef DEBUG
                    printf("\tIgnoring stray button press\n");
#endif /* DEBUG */
                }
                break;
            
            case MotionNotify:
                xevent = compress_motion(xevent);

                /* this can happen with the motion compressing */
                if (!(xevent->xmotion.state & Button1Mask)) {
                    fprintf(stderr,
                            "XWM: Motion event without correct button\n");
                    goto reset;
                }
                if (client == NULL) {
                    fprintf(stderr,
                            "XWM: Error, null client while moving\n");
                    goto reset;
                }
                /* just move the window */
                client->x += xevent->xbutton.x_root - x_start;
                client->y += xevent->xbutton.y_root - y_start;

                /* take out these two lines for a fun effect :) */
                x_start = xevent->xbutton.x_root;
                y_start = xevent->xbutton.y_root;
                XMoveWindow(dpy, client->frame, client->x, client->y);

                break;
            case ButtonRelease:
                if (client) {
                    client->x += xevent->xbutton.x_root - x_start;
                    client->y += xevent->xbutton.y_root - y_start;
                }
                goto reset;
            
                break;
            default:
#ifdef DEBUG
                printf("\tStart recursive event processing\n");
#endif /* DEBUG */
                event_dispatch(xevent);
#ifdef DEBUG
                printf("\tEnd recursive event processing\n");
#endif /* DEBUG */
                break;
        }

        if (client != NULL) {
            XNextEvent(dpy, &event1);
            xevent = &event1;
        }

        /* static client is set to NULL in reset code below (this
         * function may be nested arbitrarily */
    } while (client != NULL);
        
    return;
    
 reset:

    if (client != NULL) {
        XMoveWindow(dpy, client->frame, client->x, client->y);
        /* must send a synthetic ConfigureNotify to the client
         * according to ICCCM 4.1.5 */
        /* FIXME:  make this a function in client.c */
        event1.type = ConfigureNotify;
        event1.xconfigure.display = dpy;
        event1.xconfigure.event = client->window;
        event1.xconfigure.window = client->window;
        event1.xconfigure.x = client->x;
        event1.xconfigure.y = client->y - TITLE_HEIGHT;
        event1.xconfigure.width = client->width;
        event1.xconfigure.height = client->height - TITLE_HEIGHT;
        event1.xconfigure.above = None; /* FIXME */
        event1.xconfigure.override_redirect = False;
        XSendEvent(dpy, client->window, False, StructureNotifyMask, &event1);
        XFlush(dpy);
    }

#ifdef DEBUG
    printf("\tUngrabbing the mouse for move\n");
#endif /* DEBUG */

    client = NULL;
    x_start = y_start = -1;
    XUngrabPointer(dpy, CurrentTime);
}
    

/*
 * This gets a bit hairy...similar to the move window code above but a
 * bit more complex
 */

/*
 * Escape ends the resize, does not resize client window
 * Shift constrains resize to y->x->x+y->y->....
 * Shift with button press constrains to x or y
 */

void mouse_resize_client(XEvent *xevent)
{
    static client_t *client = NULL;
    static int x_start, y_start;
    static resize_direction_t resize_direction = UNKNOWN;
    static quadrant_t quadrant = IV;
    XEvent event1;
    
    do {
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
                break;
            case ButtonPress:
                if (xevent->xbutton.button == Button3) {
                    if (client != NULL) {
                        /* Received button3 down while sizing a window */
                        fprintf(stderr, "XWM: Unexpected Button3 down\n");
                        goto reset;
                    } else if (xevent->xbutton.state & Mod1Mask) {
                        client = client_find(xevent->xbutton.window);
                        if (client == NULL) {
#ifdef DEBUG
                            printf("\tNot resizing a non-client\n");
#endif /* DEBUG */
                            return;
                        }
                        x_start = xevent->xbutton.x_root;
                        y_start = xevent->xbutton.y_root;
                        quadrant = get_quadrant(client, xevent->xbutton.x_root,
                                                xevent->xbutton.y_root);
                        switch (quadrant) {
                            case I:
                                resize_direction = NE;
                                break;
                            case II:
                                resize_direction = NW;
                                break;
                            case III:
                                resize_direction = SW;
                                break;
                            case IV:
                                resize_direction = SE;
                                break;
                        }
#ifdef DEBUG
                        printf("\tGrabbing the mouse for resizing\n");
#endif /* DEBUG */
                        XGrabPointer(dpy, root_window, True,
                                     PointerMotionMask | ButtonPressMask |
                                     ButtonReleaseMask,
                                     GrabModeAsync, GrabModeAsync, None,
                                     cursor_direction_map[resize_direction],
                                     CurrentTime);
                        process_resize(client, x_start, y_start,
                                       resize_direction, quadrant,
                                       &x_start, &y_start);
                        
                    } else {
                        fprintf(stderr, "Received an unexpected button press\n");
                    }
                } else {
#ifdef DEBUG
                    printf("\tIgnoring stray button press\n");
#endif /* DEBUG */
                }
                break;
            
            case MotionNotify:
                xevent = compress_motion(xevent);

                /* this can happen with motion compression */
                if (!(xevent->xmotion.state & Button3Mask)) {
                    fprintf(stderr,
                            "XWM: Motion event w/o button while moving\n");
                    goto reset;
                }
                if (client == NULL) {
                    fprintf(stderr, "XWM: Error, null client in resize\n");
                    goto reset;
                }

#ifdef DEBUG
                printf("Sizing from (%d,%d) to (%d,%d), direction ",
                       x_start, y_start, xevent->xbutton.x_root,
                       xevent->xbutton.y_root);
                switch (resize_direction) {
                    case UNKNOWN:
                        printf("UNKNOWN\n");
                        break;
                    case NW:
                        printf("NW\n");
                        break;
                    case NE:
                        printf("NE\n");
                        break;
                    case SE:
                        printf("SE\n");
                        break;
                    case SW:
                        printf("SW\n");
                        break;
                    case NORTH:
                        printf("NORTH\n");
                        break;
                    case SOUTH:
                        printf("SOUTH\n");
                        break;
                    case WEST:
                        printf("WEST\n");
                        break;
                    case EAST:
                        printf("EAST\n");
                        break;
                }
#endif /* DEBUG */
                process_resize(client, xevent->xbutton.x_root,
                               xevent->xbutton.y_root, resize_direction,
                               quadrant, &x_start, &y_start);
                break;
            case ButtonRelease:
                if (client) {
                    process_resize(client, xevent->xmotion.x_root,
                                   xevent->xmotion.y_root, resize_direction,
                                   quadrant, &x_start, &y_start);
                }
                goto reset;
            
                break;
            default:
#ifdef DEBUG
                printf("\tStart recursive event processing\n");
#endif /* DEBUG */
                event_dispatch(xevent);
#ifdef DEBUG
                printf("\tEnd recursive event processing\n");
#endif /* DEBUG */
                break;
        }
        XNextEvent(dpy, &event1);
        xevent = &event1;
    } while (client != NULL);
        
    return;
    
 reset:

    if (client != NULL) {
        XMoveResizeWindow(dpy, client->frame, client->x, client->y,
                          client->width, client->height);
        XResizeWindow(dpy, client->window, client->width,
                      client->height - TITLE_HEIGHT);
    }

#ifdef DEBUG
    printf("\tUngrabbing the mouse\n");
#endif /* DEBUG */

    client = NULL;
    x_start = y_start = -1;
    resize_direction = UNKNOWN;
    XUngrabPointer(dpy, CurrentTime);
}

void mouse_handle_event(XEvent *xevent)
{
    if (xevent->type != ButtonPress) {
        fprintf(stderr, "XWM: Error, mouse_handle_event called incorrectly\n");
        return;
    }
    if (xevent->xbutton.button == Button1
        && (xevent->xbutton.state & Mod1Mask)) {
        mouse_move_client(xevent);
    } else if (xevent->xbutton.button == Button3
               && (xevent->xbutton.state & Mod1Mask)) {
        mouse_resize_client(xevent);
    } else {
#ifdef DEBUG
        printf("\tIgnoring unknown mouse event\n");
#endif /* DEBUG */
    }
}

/* compress motion events, idea taken from WindowMaker */
/* returns most recent event to deal with */
/* FIXME:  deal with this more elegantly, process events
 * which do not have right modifiers separately */
static XEvent *compress_motion(XEvent *xevent)
{
    static XEvent newer;

    while (XCheckMaskEvent(dpy, ButtonMotionMask, &newer)) {
        if (newer.type == MotionNotify
            && newer.xmotion.window == xevent->xmotion.window
            && newer.xmotion.state == xevent->xmotion.state) {
            xevent = &newer;
#ifdef DEBUG
            printf("\tMotion event compressed (%d,%d)\n",
                   xevent->xmotion.x_root, xevent->xmotion.y_root);
#endif /* DEBUG */
        } else {
            /* Can't happen */
            fprintf(stderr,
                    "XWM: Accidentally ate up an event!\n");
        }
    }
    return xevent;
}

static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           quadrant_t quadrant,
                           int *old_x, int *old_y)
{
    int x_diff, y_diff;
    int x, y, w, h;

    x = client->x;
    y = client->y;
    w = client->width;
    h = client->height;
    y_diff = new_y - *old_y;
    x_diff = new_x - *old_x;
    if (direction == WEST || direction == EAST)
        y_diff = 0;
    if (direction == NORTH || direction == SOUTH)
        x_diff = 0;
#ifdef DEBUG
    printf("\tx_diff = %d, y_diff = %d\n", x_diff, y_diff);
#endif /* DEBUG */
    if (client->xsh != NULL && (client->xsh->flags & PResizeInc)) {
#ifdef DEBUG
        printf("\txsh->width_inc = %d, xsh->height_inc = %d\n",
               client->xsh->width_inc, client->xsh->height_inc);
#endif /* DEBUG */
        if (y_diff > client->xsh->height_inc
            || (-y_diff) > client->xsh->height_inc) {
            y_diff -= (y_diff % client->xsh->height_inc);
            if (y_diff < 0) y_diff += client->xsh->height_inc;
        } else {
            y_diff = 0;
        }
        if (x_diff > client->xsh->width_inc
            || (-x_diff) > client->xsh->width_inc) {
            x_diff -= (x_diff % client->xsh->width_inc);
            if (x_diff < 0) x_diff += client->xsh->width_inc;
        } else {
            x_diff = 0;
        }
#ifdef DEBUG
        printf("\t(after) x_diff = %d, y_diff = %d\n", x_diff, y_diff);
#endif /* DEBUG */
    }

    if (x_diff != 0) {
        
        switch (direction) {
            case WEST:
            case SW:
            case NW:
                client->x += x_diff;
                client->width -= x_diff;
                *old_x += x_diff;
                break;
            case EAST:
            case SE:
            case NE:
                client->width += x_diff;
                *old_x += x_diff;
                break;
            default:
        }
    }
    if (y_diff != 0) {
        
        switch (direction) {
            case NORTH:
            case NW:
            case NE:
                client->y += y_diff;
                client->height -= y_diff;
                *old_y += y_diff;
                break;
            case SOUTH:
            case SE:
            case SW:
                client->height += y_diff;
                *old_y += y_diff;
                break;
            default:
        }
    }
}

static quadrant_t get_quadrant(client_t *client, int x, int y)
{
    if (x < client->x + (client->width / 2)) {
        if (y < client->y + (client->height / 2)) {
            return II;
        } else {
            return III;
        }
    } else {
        if (y < client->y + (client->height / 2)) {
            return I;
        } else {
            return IV;
        }
    }
}
