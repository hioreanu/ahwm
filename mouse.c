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
#include <stdlib.h>

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif /* MIN */

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif /* MAX */

#ifndef ABS
#define ABS(x) ((x) < 0 ? (-(x)) : (x))
#endif /* ABS */

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

typedef enum {
    FIRST, MIDDLE, LAST
} resize_ordinal_t;

static XEvent *compress_motion(XEvent *xevent);
static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           quadrant_t quadrant,
                           int *old_x, int *old_y,
                           position_size *orig,
                           resize_ordinal_t ordinal);
static quadrant_t get_quadrant(client_t *client, int x, int y);
static void display_geometry(char *s, client_t *client);
static void get_display_width_height(client_t *client, int *w, int *h);
static void drafting_lines(client_t *client, resize_direction_t direction,
                           int x1, int y1, int x2, int y2);

/*
 * windowmaker grabs the server while processing the motion events for
 * moving a window; that seems a bit antisocial, so we won't do that.
 * window movement is opaque.
 * 
 * mouse_move:
 * select only one window, the one under the pointer
 * grab the pointer
 * grab the keyboard
 * grab the server
 * 
 * use xcheckmaskevent to compress motion events
 * buttonmotionmask, buttonreleasemask, buttonpressmask
 */

void mouse_grab_buttons(Window w)
{
    XGrabButton(dpy, Button1, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_moving);
    XGrabButton(dpy, Button3, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_normal);
}

/*
 * this takes over the event loop and grabs the pointer until the move
 * is complete
 */
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
                 * to gobble up all enternotify and leavenotify events
                 * while moving or resizing the window */
                break;
            case ButtonPress:
                if (xevent->xbutton.button == Button1) {
                    if (client != NULL) {
                        /* received button1 down while moving a window */
                        fprintf(stderr, "unexpected button1 down\n");
                        goto reset;
                    } else if (xevent->xbutton.state & Mod1Mask) {
                        /* received normal mod1 + button1 press */
                        client = client_find(xevent->xbutton.window);
                        if (client == NULL) {
                            /* error - we should never do xgrabbutton() on
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
                        printf("\tgrabbing the mouse for moving\n");
#endif /* DEBUG */
                        XGrabPointer(dpy, root_window, True,
                                     PointerMotionMask | ButtonPressMask
                                     | ButtonReleaseMask,
                                     GrabModeAsync, GrabModeAsync, None,
                                     cursor_moving, CurrentTime);
                        display_geometry("Moving", client);
                    } else {
                        /* error - we were called from somewhere else
                         * to deal with a button press that's not ours */
                        fprintf(stderr,
                                "XWM: received an unknown button press\n");
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
                            "XWM: motion event without correct button\n");
                    goto reset;
                }
                if (client == NULL) {
                    fprintf(stderr,
                            "XWM: error, null client while moving\n");
                    goto reset;
                }
                /* just move the window */
                client->x += xevent->xbutton.x_root - x_start;
                client->y += xevent->xbutton.y_root - y_start;

                /* take out these two lines for a fun effect :) */
                x_start = xevent->xbutton.x_root;
                y_start = xevent->xbutton.y_root;
                XMoveWindow(dpy, client->frame, client->x, client->y);
                display_geometry("Moving", client);

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

        /* static client is set to null in reset code below (this
         * function may be nested arbitrarily */
    } while (client != NULL);
        
    return;
    
 reset:

    if (client != NULL) {
        XMoveWindow(dpy, client->frame, client->x, client->y);
        if (client->name != NULL) free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
        /* must send a synthetic ConfigureNotify to the client
         * according to ICCCM 4.1.5 */
        /* FIXME:  make this a function in client.c */
        /* FIXME:  breaks with netscape */
        event1.type = ConfigureNotify;
        event1.xconfigure.display = dpy;
        event1.xconfigure.event = client->window;
        event1.xconfigure.window = client->window;
        event1.xconfigure.x = client->x;
        event1.xconfigure.y = client->y - TITLE_HEIGHT;
        event1.xconfigure.width = client->width;
        event1.xconfigure.height = client->height - TITLE_HEIGHT;
        event1.xconfigure.border_width = 0;
        event1.xconfigure.above = client->frame; /* fixme */
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
 * this gets a bit hairy...similar to the move window code above but a
 * bit more complex
 */

/*
 * escape ends the resize, does not resize client window
 * shift constrains resize to y->x->x+y->y->....
 * shift with button press constrains to x or y
 */

void mouse_resize_client(XEvent *xevent)
{
    static client_t *client = NULL;
    static int x_start, y_start;
    static resize_direction_t resize_direction = UNKNOWN;
    static quadrant_t quadrant = IV;
    static position_size orig;
    XEvent event1;
    
    do {
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
                break;
            case ButtonPress:
                if (xevent->xbutton.button == Button3) {
                    if (client != NULL) {
                        /* received button3 down while sizing a window */
                        fprintf(stderr, "XWM: unexpected button3 down\n");
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
                        orig.x = client->x;
                        orig.y = client->y;
                        orig.width = client->width;
                        orig.height = client->height;
#ifdef DEBUG
                        printf("\tGrabbing the mouse for resizing\n");
#endif /* DEBUG */
                        XGrabPointer(dpy, root_window, True,
                                     PointerMotionMask | ButtonPressMask
                                     | ButtonReleaseMask,
                                     GrabModeAsync, GrabModeAsync, None,
                                     cursor_direction_map[resize_direction],
                                     CurrentTime);
                        process_resize(client, x_start, y_start,
                                       resize_direction, quadrant,
                                       &x_start, &y_start, &orig, FIRST);
                        
                    } else {
                        fprintf(stderr,
                                "XWM: received an unexpected button press\n");
                    }
                } else {
#ifdef DEBUG
                    printf("\tignoring stray button press\n");
#endif /* DEBUG */
                }
                break;
            
            case MotionNotify:
                xevent = compress_motion(xevent);

                /* this can happen with motion compression */
                if (!(xevent->xmotion.state & Button3Mask)) {
                    fprintf(stderr,
                            "XWM: motion event w/o button while moving\n");
                    goto reset;
                }
                if (client == NULL) {
                    fprintf(stderr, "XWM: error, null client in resize\n");
                    goto reset;
                }

#ifdef DEBUG
                printf("sizing from (%d,%d) to (%d,%d)\n",
                       x_start, y_start, xevent->xbutton.x_root,
                       xevent->xbutton.y_root);
#endif /* DEBUG */
                process_resize(client, xevent->xbutton.x_root,
                               xevent->xbutton.y_root, resize_direction,
                               quadrant, &x_start, &y_start, &orig, MIDDLE);
                break;
            case ButtonRelease:
                if (client == NULL) {
                    fprintf(stderr, "XWM: error, null client in resize\n");
                    goto reset;
                }
                process_resize(client, xevent->xmotion.x_root,
                               xevent->xmotion.y_root, resize_direction,
                               quadrant, &x_start, &y_start, &orig, LAST);
                goto done;
            
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
    XClearWindow(dpy, root_window);

 done:

    if (client != NULL) {
        XMoveResizeWindow(dpy, client->frame, client->x, client->y,
                          client->width, client->height);
        XResizeWindow(dpy, client->window, client->width,
                      client->height - TITLE_HEIGHT);
        if (client->name != NULL) free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
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
        fprintf(stderr, "XWM: error, mouse_handle_event called incorrectly\n");
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

/* compress motion events, idea taken from windowmaker */
/* returns most recent event to deal with */
/* fixme:  deal with this more elegantly, process events
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
            /* can't happen */
            fprintf(stderr,
                    "XWM: accidentally ate up an event!\n");
        }
    }
    return xevent;
}

static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           quadrant_t quadrant,
                           int *old_x, int *old_y,
                           position_size *orig, resize_ordinal_t ordinal)
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
        printf("\t(After) x_diff = %d, y_diff = %d\n", x_diff, y_diff);
#endif /* DEBUG */
    }

    if (x_diff != 0) {
        switch (direction) {
            case WEST:
            case SW:
            case NW:
                if (client->width - x_diff > 1
                    && new_x < orig->x + orig->width) {
                    if (client->xsh->flags & PMinSize
                        && client->width - x_diff < client->xsh->min_width)
                        break;
                    if (client->xsh->flags & PMaxSize
                        && client->width - x_diff > client->xsh->max_width)
                        break;
                    client->x += x_diff;
                    client->width -= x_diff;
                    *old_x += x_diff;
                }
                break;
            case EAST:
            case SE:
            case NE:
                if (client->width + x_diff > 1) {
                    if (client->xsh->flags & PMinSize
                        && client->width + x_diff < client->xsh->min_width)
                        break;
                    if (client->xsh->flags & PMaxSize
                        && client->width + x_diff > client->xsh->max_width)
                        break;
                    client->width += x_diff;
                    *old_x += x_diff;
                }
                break;
            default:
        }
    }
    
    if (y_diff != 0) {
        switch (direction) {
            case NORTH:
            case NW:
            case NE:
                if (client->height - y_diff > TITLE_HEIGHT + 1
                    && new_y < orig->y + orig->height) {
                    if (client->xsh->flags & PMinSize
                        && client->height - y_diff <
                           client->xsh->min_height + TITLE_HEIGHT)
                        break;
                    if (client->xsh->flags & PMaxSize
                        && client->height - y_diff >
                           client->xsh->max_height + TITLE_HEIGHT)
                        break;
                    client->y += y_diff;
                    client->height -= y_diff;
                    *old_y += y_diff;
                }
                break;
            case SOUTH:
            case SE:
            case SW:
                if (client->height + y_diff > TITLE_HEIGHT + 1) {
                    if (client->xsh->flags & PMinSize
                        && client->height + y_diff <
                           client->xsh->min_height + TITLE_HEIGHT)
                        break;
                    if (client->xsh->flags & PMaxSize
                        && client->height + y_diff >
                           client->xsh->max_height + TITLE_HEIGHT)
                        break;
                    client->height += y_diff;
                    *old_y += y_diff;
                }
                break;
            default:
        }
    }
    
    /* now we draw the window rectangle
     * 
     * first, we erase the previous rectangle (the gc has an xor function
     * selected, so we simply draw on it to erase).  we don't want any
     * flicker, so we only erase and redraw if we've made any changes
     * to the line segment */

    if (ordinal != MIDDLE || x != client->x
        || w != client->width || y != client->y) {
        /* redraw top bar */
        if (ordinal != FIRST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      x, y, x + w, y);
            if (direction == NORTH || direction == NW || direction == NE)
                drafting_lines(client, NORTH, x, y, x + w, y);
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc, client->x,
                      client->y, client->x + client->width, client->y);
            if (direction == NORTH || direction == NW || direction == NE)
                drafting_lines(client, NORTH, client->x, client->y,
                               client->x + client->width, client->y);
        }
    }
    if (ordinal != MIDDLE || x != client->x || w != client->width
        || y + h != client->y + client->height) {
        /* redraw bottom bar */
        if (ordinal != FIRST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      x, y + h, x + w, y + h);
            if (direction == SOUTH || direction == SW || direction == SE)
                drafting_lines(client, SOUTH, x, y + h, x + w, y + h);
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x, client->y + client->height,
                      client->x + client->width,
                      client->y + client->height);
            if (direction == SOUTH || direction == SW || direction == SE)
                drafting_lines(client, SOUTH, client->x,
                               client->y + client->height,
                               client->x + client->width,
                               client->y + client->height);
        }
    }
    if (ordinal != MIDDLE || y != client->y ||
        h != client->height || x != client->x) {
        /* redraw left bar */
        if (ordinal != FIRST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      x, y, x, y + h);
            if (direction == WEST || direction == NW || direction == SW)
                drafting_lines(client, WEST, x, y, x, y + h);
                
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x, client->y, client->x,
                      client->y + client->height);
            if (direction == WEST || direction == NW || direction == SW)
                drafting_lines(client, WEST, client->x, client->y,
                               client->x, client->y + client->height);
        }
    }
    if (ordinal != MIDDLE || y != client->y || h != client->height
        || x + w != client->x + client->width) {
        /* redraw right bar */
        if (ordinal != FIRST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      x + w, y, x + w, y + h);
            if (direction == EAST || direction == NE || direction == SE)
                drafting_lines(client, EAST, x + w, y, x + w, y + h);
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x + client->width,
                      client->y,
                      client->x + client->width,
                      client->y + client->height);
            if (direction == EAST || direction == NE || direction == SE)
                drafting_lines(client, EAST, client->x + client->width,
                      client->y, client->x + client->width,
                      client->y + client->height);
        }
    }

    if (ordinal == FIRST || x != client->x || y != client->y
        || w != client->width || h != client->height) {
        display_geometry("Resizing", client);
    }

}

static void get_display_width_height(client_t *client, int *w, int *h)
{
    int w_inc, h_inc;
    
    if (client->xsh != NULL && (client->xsh->flags & PResizeInc)) {
        w_inc = client->xsh->width_inc;
        h_inc = client->xsh->height_inc;
    } else {
        w_inc = h_inc = 1;
    }
    *w = client->width / w_inc;
    *h = (client->height - TITLE_HEIGHT) / h_inc;
}

#define DRAFTING_OFFSET 16

static void drafting_lines(client_t *client, resize_direction_t direction,
                           int x1, int y1, int x2, int y2)
{
    int h_inc, w_inc, tmp;
    int font_width, font_height;
    int x_room, y_room;
    char label[16];
    
    if (client->xsh != NULL && (client->xsh->flags & PResizeInc)) {
        w_inc = client->xsh->width_inc;
        h_inc = client->xsh->height_inc;
    } else {
        w_inc = h_inc = 1;
    }
    font_width = (fontstruct->max_bounds.rbearing
                  - fontstruct->min_bounds.lbearing + 1);
    font_height = (fontstruct->max_bounds.ascent
                   + fontstruct->max_bounds.descent + 1);

    if (x1 > x2) {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }
    if (y1 > y2) {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    if (direction == WEST || direction == EAST) {
        tmp = (y2 - y1 - TITLE_HEIGHT) / h_inc;
    } else {
        tmp = (x2 - x1) / w_inc;
    }
    snprintf(label, 16, "%d", tmp);
    y_room = font_height / 2;
    x_room = XTextWidth(fontstruct, label, strlen(label)) / 2;
    
    if (direction == WEST) {
        x1 -= x_room + 3;
        x2 -= x_room + 3;
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1, x2,
                  ((y2 + y1) / 2) - (y_room + 1));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y2, x2,
                  ((y2 + y1) / 2) + (y_room + 1));
        /* FIXME:  working on this statement */
        XDrawString(dpy, root_window, root_invert_gc,
                    x2 - x_room,
                    ((y2 + y1) / 2) + y_room,
                    label, strlen(label));
    } else if (direction == EAST) {
        x1 += x_room;
        x2 += x_room;
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1, x2, y1 + ((y2 - y1) / 2) - ((font_height / 2) + 1));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y2, x2, y2 - ((y2 - y1) / 2) + ((font_height / 2) + 1));
        XDrawString(dpy, root_window, root_invert_gc,
                    x2 - 2 * font_width,
                    y2 - ((y2 - y1) / 2) + (font_height / 2),
                    label, strlen(label));
    } else if (direction == NORTH) {
        y1 -= y_room;
        y2 -= y_room;
        XDrawLine(dpy, root_window, root_invert_gc, x1, y1,
                  x1 + ((x2 - x1) / 2) - (2 * font_width + 1), y2);
        XDrawLine(dpy, root_window, root_invert_gc, x2, y1,
                  x2 - ((x2 - x1) / 2) + (2 * font_width + 1), y2);
        XDrawString(dpy, root_window, root_invert_gc,
                    x1 + ((x2 - x1) / 2) - (2 * font_width),
                    y2 + (font_height / 2), label, strlen(label));
    } else if (direction == SOUTH) {
        y1 += y_room;
        y2 += y_room;
        XDrawLine(dpy, root_window, root_invert_gc, x1, y1,
                  x1 + ((x2 - x1) / 2) - (2 * font_width + 1), y2);
        XDrawLine(dpy, root_window, root_invert_gc, x2, y1,
                  x2 - ((x2 - x1) / 2) + (2 * font_width + 1), y2);
        XDrawString(dpy, root_window, root_invert_gc,
                    x1 + ((x2 - x1) / 2) - (2 * font_width),
                    y2 + (font_height / 2), label, strlen(label));
    }
}

static void display_geometry(char *s, client_t *client)
{
    static char *titlebar_display = NULL;
    int width, height;

    get_display_width_height(client, &width, &height);
    if (client->name != titlebar_display) {
        titlebar_display = malloc(256); /* arbitrary, whatever */
        if (titlebar_display == NULL) return;
        free(client->name);
        client->name = titlebar_display;
    }
    snprintf(client->name, 256, "%dx%d+%d+%d [%s %s]",
             client->x, client->y, width, height, s, client->instance);
    client_paint_titlebar(client);
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
