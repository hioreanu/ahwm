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
#include <X11/keysym.h>

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

typedef enum {
    FIRST, MIDDLE, LAST
} resize_ordinal_t;

static int keycode_Escape = 0, keycode_Shift_L, keycode_Shift_R;
static int keycode_Return, keycode_Up, keycode_Down, keycode_Left;
static int keycode_Right, keycode_j, keycode_k, keycode_l, keycode_h;

static XEvent *compress_motion(XEvent *xevent);
static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           resize_direction_t old_direction,
                           int *old_x, int *old_y,
                           position_size *orig,
                           resize_ordinal_t ordinal);
static resize_direction_t get_direction(client_t *client, int x, int y);
static void display_geometry(char *s, client_t *client);
static void get_display_width_height(client_t *client, int *w, int *h);
static void drafting_lines(client_t *client, resize_direction_t direction,
                           int x1, int y1, int x2, int y2);
static void draw_arrowhead(int x, int y, resize_direction_t direction);
static void xrefresh();
static void cycle_resize_direction(resize_direction_t *current,
                                   resize_direction_t *old);
static void warp_pointer(client_t *client, resize_direction_t direction);
static void set_keys();

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
 * This takes over the event loop and grabs the pointer until the move
 * is complete; we keep most of the data 'static' as this may get
 * called recursively and all simultaneously-running invocations need
 * to keep some of the same state (the resize code is much worse).
 */
void mouse_move_client(XEvent *xevent)
{
    static client_t *client = NULL;
    static int x_start, y_start;
    XEvent event1;

    if (keycode_Escape == 0) set_keys();
    
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
                        XGrabKeyboard(dpy, root_window, True,
                                      GrabModeAsync, GrabModeAsync,
                                      CurrentTime);
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
                
            case KeyPress:
                if (xevent->xkey.keycode == keycode_Up ||
                           xevent->xkey.keycode == keycode_k) {
                    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -1);
                } else if (xevent->xkey.keycode == keycode_Down ||
                           xevent->xkey.keycode == keycode_j) {
                    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, 1);
                } else if (xevent->xkey.keycode == keycode_Left ||
                           xevent->xkey.keycode == keycode_h) {
                    XWarpPointer(dpy, None, None, 0, 0, 0, 0, -1, 0);
                } else if (xevent->xkey.keycode == keycode_Right ||
                           xevent->xkey.keycode == keycode_l) {
                    XWarpPointer(dpy, None, None, 0, 0, 0, 0, 1, 0);
                } else if (xevent->xkey.keycode == keycode_Return) {
                    goto reset;
                /* ignore Escape, can't think of a good reason one would
                 * want to quit a move operation */
                } else {
                    /* can still use alt-tab while moving */
                    event_dispatch(xevent);
                }
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
    XUngrabKeyboard(dpy, CurrentTime);
}
    

/*
 * WARNING:  this gets really, really ugly from here on
 */

/*
 * escape ends the resize, does not resize client window
 * enter ends the resize, resizes client window
 * shift constrains resize to y->x->x+y->y->....
 * shift with button press constrains to x or y
 * left, right, up, down, h, j, k and l work as expected
 */

void mouse_resize_client(XEvent *xevent)
{
    static client_t *client = NULL;
    static int x_start, y_start;
    static resize_direction_t resize_direction = UNKNOWN;
    static resize_direction_t old_resize_direction = UNKNOWN;
    static position_size orig;
    XEvent event1;

    if (keycode_Escape == 0) set_keys();

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
                        resize_direction =
                            get_direction(client, xevent->xbutton.x_root,
                                          xevent->xbutton.y_root);
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
                        XGrabKeyboard(dpy, root_window, True,
                                      GrabModeAsync, GrabModeAsync,
                                      CurrentTime);
                        process_resize(client, x_start, y_start,
                                       resize_direction, old_resize_direction,
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

            case KeyPress:
                if (xevent->xkey.keycode == keycode_Escape) {
                    goto reset;
                } else if (xevent->xkey.keycode == keycode_Return) {
                    process_resize(client, xevent->xkey.x_root,
                                   xevent->xkey.y_root, resize_direction,
                                   old_resize_direction, &x_start, &y_start,
                                   &orig, LAST);
                    goto done;
                } else if (xevent->xkey.keycode == keycode_Shift_L ||
                           xevent->xkey.keycode == keycode_Shift_R) {
                    cycle_resize_direction(&resize_direction,
                                           &old_resize_direction);
                    XChangeActivePointerGrab(dpy,
                       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                       cursor_direction_map[resize_direction], CurrentTime);
                    xrefresh();
                    process_resize(client, x_start, y_start, resize_direction,
                                   old_resize_direction, &x_start, &y_start,
                                   &orig, FIRST);
                } else if (xevent->xkey.keycode == keycode_Up ||
                           xevent->xkey.keycode == keycode_k) {
                    if (resize_direction != EAST && resize_direction != WEST)
                        warp_pointer(client, NORTH);
                } else if (xevent->xkey.keycode == keycode_Down ||
                           xevent->xkey.keycode == keycode_j) {
                    if (resize_direction != EAST && resize_direction != WEST)
                        warp_pointer(client, SOUTH);
                } else if (xevent->xkey.keycode == keycode_Left ||
                           xevent->xkey.keycode == keycode_h) {
                    if (resize_direction != NORTH && resize_direction != SOUTH)
                        warp_pointer(client, WEST);
                } else if (xevent->xkey.keycode == keycode_Right ||
                           xevent->xkey.keycode == keycode_l) {
                    if (resize_direction != NORTH && resize_direction != SOUTH)
                        warp_pointer(client, EAST);
                } else {
                    /* can still use alt-tab while resizing */
                    event_dispatch(xevent);
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
                               old_resize_direction, &x_start, &y_start,
                               &orig, MIDDLE);
                break;
            case ButtonRelease:
                if (client == NULL) {
                    fprintf(stderr, "XWM: error, null client in resize\n");
                    goto reset;
                }
                process_resize(client, xevent->xmotion.x_root,
                               xevent->xmotion.y_root, resize_direction,
                               old_resize_direction, &x_start, &y_start,
                               &orig, LAST);
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
    xrefresh();
    client->x = orig.x;
    client->y = orig.y;
    client->width = orig.width;
    client->height = orig.height;

 done:

    if (client != NULL) {
        XMoveResizeWindow(dpy, client->frame, client->x, client->y,
                          client->width, client->height);
        XResizeWindow(dpy, client->window, client->width,
                      client->height - TITLE_HEIGHT);
        if (client->name != NULL) free(client->name); /* FIXME */
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
    XUngrabKeyboard(dpy, CurrentTime);
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

/*
 * SW -> SOUTH -> WEST -> SW -> ....
 * SE -> SOUTH -> EAST -> SW -> ....
 * NE -> NORTH -> EAST -> NE -> ....
 * NW -> NORTH -> WEST -> NW -> ....
 */

static void cycle_resize_direction(resize_direction_t *current,
                                   resize_direction_t *old)
{
    resize_direction_t tmp;
    
    switch (*current) {
        case SW:
            *old = SW;
            *current = SOUTH;
            break;
        case NW:
            *old = NW;
            *current = NORTH;
            break;
        case NE:
            *old = NE;
            *current = NORTH;
            break;
        case SE:
            *old = SE;
            *current = SOUTH;
            break;
        case SOUTH:
        case NORTH:
            tmp = *current;
            switch (*old) {
                case NW:
                case SW:
                    *current = WEST;
                    break;
                case NE:
                case SE:
                default:
                    *current = EAST;
            }
            *old = tmp;
            break;
        case EAST:
            if (*old == NORTH)
                *current = NE;
            else
                *current = SE;
            *old = EAST;
            break;
        case WEST:
            if (*old == NORTH)
                *current = NW;
            else
                *current = SW;
            *old = WEST;
            break;
        default:
    }
}

/*
 * works just like the program of the same name; we use this instead
 * of an XClearWindow() as we may have drawn on a number of windows
 * and this seems faster than walking the window tree and generating
 * expose events
 * 
 * FIXME:  test speed compared to XQueryTree()
 */

static void xrefresh()
{
    XSetWindowAttributes xswa;
    Window w;

    xswa.override_redirect = True;
    xswa.backing_store = NotUseful;
    xswa.save_under = False;
    w = XCreateWindow(dpy, root_window, 0, 0,
                      DisplayWidth(dpy, scr), DisplayHeight(dpy, scr),
                      0, DefaultDepth(dpy, scr), InputOutput,
                      DefaultVisual(dpy, scr),
                      CWOverrideRedirect | CWBackingStore | CWSaveUnder,
                      &xswa);
    XMapWindow(dpy, w);
    XUnmapWindow(dpy, w);
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

/*
 * Does two things:
 * 1. Examines the previous configuration and sees if we can resize
 * now or if we need a few more points to resize because of the
 * client's width and height increment hints
 * 2. Visually displays information about the client's size
 * 
 * The ordinal argument must be FIRST if this is the first time the
 * function is called for a particular resize, or LAST if this will be
 * the last time the function is called for a resize.
 */

static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           resize_direction_t old_direction,
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
            if ((direction == NW || direction == NE)
                || ((old_direction == NORTH)
                    && (direction == EAST || direction == WEST)))
                drafting_lines(client, NORTH, x, y, x + w, y);
            
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc, client->x,
                      client->y, client->x + client->width, client->y);
            if ((direction == NW || direction == NE)
                || ((old_direction == NORTH)
                    && (direction == EAST || direction == WEST)))
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
            if ((direction == SW || direction == SE)
                || ((old_direction == SOUTH)
                    && (direction == WEST || direction == EAST)))
                drafting_lines(client, SOUTH, x, y + h, x + w, y + h);
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x, client->y + client->height,
                      client->x + client->width,
                      client->y + client->height);
            if ((direction == SW || direction == SE)
                || ((old_direction == SOUTH)
                    && (direction == WEST || direction == EAST)))
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
            if ((direction == NW || direction == SW)
                || ((old_direction == NW || old_direction == SW)
                    && (direction == NORTH || direction == SOUTH)))
                drafting_lines(client, WEST, x, y, x, y + h);
                
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x, client->y, client->x,
                      client->y + client->height);
            if ((direction == NW || direction == SW)
                || ((old_direction == NW || old_direction == SW)
                    && (direction == NORTH || direction == SOUTH)))
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
            if ((direction == NE || direction == SE)
                || ((old_direction == NE || old_direction == SE)
                    && (direction == NORTH || direction == SOUTH)))
                drafting_lines(client, EAST, x + w, y, x + w, y + h);
        }
        if (ordinal != LAST) {
            XDrawLine(dpy, root_window, root_invert_gc,
                      client->x + client->width,
                      client->y,
                      client->x + client->width,
                      client->y + client->height);
            if ((direction == NE || direction == SE)
                || ((old_direction == NE || old_direction == SE)
                    && (direction == NORTH || direction == SOUTH)))
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

/*
 * Get two integers which should be displayed to the user to indicate
 * the width and height (think xterm)
 */

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

/*
 * Utility function for resize.  Draws some pretty little lines and
 * stuff that I find really help when resizing.  The direction
 * indicates for which side of the client this is to be done and the
 * coordinates define a line which is the side we are to draw.  NB:
 * the line defined by the coordinates need not be the same as the
 * geometry in the client structure - we also call this function to
 * draw over a previous display since we use the inverting GC.
 */

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

    /* this puts too much room on bottom of the numbers; in most
     * fonts, the characters 1 through 9 don't have a 'descent.'  If
     * this is used with a font that has, for example, 4s or 9s that
     * have a descent, it probably won't look right.
     * font_height = (fontstruct->max_bounds.ascent
     *                + fontstruct->max_bounds.descent + 1);
     */
    font_height = fontstruct->max_bounds.ascent + 1;

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
        x1 -= x_room + 5;
        x2 -= x_room + 5;
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1, x2,
                  ((y2 + y1) / 2) - (y_room + 1));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y2, x2,
                  ((y2 + y1) / 2) + (y_room + 1));
        XDrawString(dpy, root_window, root_invert_gc,
                    x2 - x_room,
                    ((y2 + y1) / 2) + (y_room - 1),
                    label, strlen(label));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1 + x_room, y1, x1 - x_room, y1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1 + x_room, y2, x1 - x_room, y2);
        draw_arrowhead(x1, y1, NORTH);
        draw_arrowhead(x2, y2, SOUTH);
    } else if (direction == EAST) {
        x1 += x_room + 5;
        x2 += x_room + 5;
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1, x2,
                  ((y2 + y1) / 2) - (y_room + 1));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y2, x2,
                  ((y2 + y1) / 2) + (y_room + 1));
        XDrawString(dpy, root_window, root_invert_gc,
                    x2 - x_room,
                    ((y2 + y1) / 2) + (y_room - 1),
                    label, strlen(label));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1 + x_room, y1, x1 - x_room, y1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1 + x_room, y2, x1 - x_room, y2);
        draw_arrowhead(x1, y1, NORTH);
        draw_arrowhead(x2, y2, SOUTH);
    } else if (direction == NORTH) {
        y1 -= y_room + 5;
        y2 -= y_room + 5;
        XDrawLine(dpy, root_window, root_invert_gc, x1, y1,
                  ((x2 + x1) / 2) - (x_room + 1), y2);
        /* we give a bit more room here - just makes it look
         * better in most fonts */
        XDrawLine(dpy, root_window, root_invert_gc, x2, y1,
                  ((x2 + x1) / 2) + (x_room + 3), y2);
        XDrawString(dpy, root_window, root_invert_gc,
                    ((x2 + x1) / 2) - (x_room - 1),
                    y2 + y_room, label, strlen(label));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1 + y_room, x1, y1 - y_room);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x2, y2 + y_room, x2, y2 - y_room);
        draw_arrowhead(x1, y1, WEST);
        draw_arrowhead(x2, y2, EAST);
    } else if (direction == SOUTH) {
        y1 += y_room + 5;
        y2 += y_room + 5;
        XDrawLine(dpy, root_window, root_invert_gc, x1, y1,
                  ((x2 + x1) / 2) - (x_room + 1), y2);
        XDrawLine(dpy, root_window, root_invert_gc, x2, y1,
                  ((x2 + x1) / 2) + (x_room + 3), y2);
        XDrawString(dpy, root_window, root_invert_gc,
                    ((x2 + x1) / 2) - (x_room - 1),
                    y2 + y_room, label, strlen(label));
        XDrawLine(dpy, root_window, root_invert_gc,
                  x1, y1 + y_room, x1, y1 - y_room);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x2, y2 + y_room, x2, y2 - y_room);
        draw_arrowhead(x1, y1, WEST);
        draw_arrowhead(x2, y2, EAST);
    }
}

/*
 * This draws an arrowhead pointing in the direction specified, and
 * does not draw on top of the line already there.  I took a technical
 * drawing class in high school - the arrowheads are supposed to be
 * drawn at a 30 degree angle and are filled.  This also fills the
 * given point since we just drew a line there (remember, all the
 * drawing operations invert).
 */
/* we just draw a bunch of lines instead of using a pixmap, this is a
 * very small image; using lines instead of specifying each point saves
 * some time in this program, but it may put some more stress on the X
 * server if it has to do the line computations in software. */
static void draw_arrowhead(int x, int y, resize_direction_t direction)
{
    XDrawPoint(dpy, root_window, root_invert_gc, x, y);
    
    if (direction == WEST) {
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 2, y + 1, x + 8, y + 1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 4, y + 2, x + 8, y + 2);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 6, y + 3, x + 8, y + 3);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x + 8, y + 4);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 2, y - 1, x + 8, y - 1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 4, y - 2, x + 8, y - 2);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 6, y - 3, x + 8, y - 3);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x + 8, y - 4);
    } else if (direction == EAST) {
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 2, y + 1, x - 8, y + 1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 4, y + 2, x - 8, y + 2);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 6, y + 3, x - 8, y + 3);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x - 8, y + 4);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 2, y - 1, x - 8, y - 1);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 4, y - 2, x - 8, y - 2);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 6, y - 3, x - 8, y - 3);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x - 8, y - 4);
    } else if (direction == NORTH) {
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 1, y + 2, x + 1, y + 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 2, y + 4, x + 2, y + 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 3, y + 6, x + 3, y + 8);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x + 4, y + 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 1, y + 2, x - 1, y + 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 2, y + 4, x - 2, y + 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 3, y + 6, x - 3, y + 8);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x - 4, y + 8);
    } else if (direction == SOUTH) {
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 1, y - 2, x + 1, y - 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 2, y - 4, x + 2, y - 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x + 3, y - 6, x + 3, y - 8);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x + 4, y - 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 1, y - 2, x - 1, y - 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 2, y - 4, x - 2, y - 8);
        XDrawLine(dpy, root_window, root_invert_gc,
                  x - 3, y - 6, x - 3, y - 8);
        XDrawPoint(dpy, root_window, root_invert_gc,
                   x - 4, y - 8);
    }
}

/*
 * changes the client's titlebar display to the geometry prefixed by a
 * given string
 */

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

static void set_keys()
{
    keycode_Escape = XKeysymToKeycode(dpy, XK_Escape);
    keycode_Shift_L = XKeysymToKeycode(dpy, XK_Shift_L);
    keycode_Shift_R = XKeysymToKeycode(dpy, XK_Shift_R);
    keycode_Return = XKeysymToKeycode(dpy, XK_Return);
    keycode_Up = XKeysymToKeycode(dpy, XK_Up);
    keycode_Down = XKeysymToKeycode(dpy, XK_Down);
    keycode_Left = XKeysymToKeycode(dpy, XK_Left);
    keycode_Right = XKeysymToKeycode(dpy, XK_Right);
    keycode_j = XKeysymToKeycode(dpy, XK_j);
    keycode_k = XKeysymToKeycode(dpy, XK_k);
    keycode_l = XKeysymToKeycode(dpy, XK_l);
    keycode_h = XKeysymToKeycode(dpy, XK_h);
}

/*
 * warps the pointer in the direction specified (one of {NORTH, SOUTH,
 * EAST, WEST}) by an amount that should cause a resize
 */

static void warp_pointer(client_t *client, resize_direction_t direction)
{
    int multiplier;

    if (direction == NORTH || direction == WEST)
        multiplier = -1;
    else
        multiplier = 1;
    if (direction == NORTH || direction == SOUTH) {
        if (client->xsh != NULL &&
            client->xsh->flags & PResizeInc)
            XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0,
                         multiplier * (client->xsh->height_inc));
        else
            XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, multiplier);
    } else {
        if (client->xsh != NULL &&
            client->xsh->flags & PResizeInc)
            XWarpPointer(dpy, None, None, 0, 0, 0, 0,
                         multiplier * (client->xsh->width_inc), 0);
        else
            XWarpPointer(dpy, None, None, 0, 0, 0, 0, multiplier, 0);
    }
}

/*
 * returns the direction in which to resize client from initial x and
 * y of mouse click
 */

static resize_direction_t get_direction(client_t *client, int x, int y)
{
    if (x < client->x + (client->width / 2)) {
        if (y < client->y + (client->height / 2)) {
            return NW;
        } else {
            return SW;
        }
    } else {
        if (y < client->y + (client->height / 2)) {
            return NE;
        } else {
            return SE;
        }
    }
}
