/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "move-resize.h"
#include "client.h"
#include "cursor.h"
#include "event.h"
#include "xwm.h"
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
 * We only constrain the window resizing to one dimension by using a
 * keypress; window movement is never constrained to one direction.
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
static int keycode_Control_L, keycode_Control_R;

static int moving = 0;
static int sizing = 0;

static XEvent *compress_motion(XEvent *xevent);
static void process_resize(client_t *client, int new_x, int new_y,
                           resize_direction_t direction,
                           resize_direction_t old_direction,
                           int *old_x, int *old_y,
                           position_size *orig,
                           resize_ordinal_t ordinal);
static resize_direction_t get_direction(client_t *client, int x, int y);
static void display_geometry(char *s, client_t *client);
static void drafting_lines(client_t *client, resize_direction_t direction,
                           int x1, int y1, int x2, int y2);
static void draw_arrowhead(int x, int y, resize_direction_t direction);
static void xrefresh();
static void cycle_resize_direction_mouse(resize_direction_t *current,
                                         resize_direction_t *old);
static void cycle_resize_direction_keyboard(resize_direction_t *current);
static void set_keys();
static void move_constrain(client_t *client);
static int get_width_resize_inc(client_t *client);
static int get_height_resize_inc(client_t *client);

/*
 * This takes over the event loop and grabs the pointer until the move
 * is complete; we keep most of the data 'static' as this may get
 * called recursively and all simultaneously-running invocations need
 * to keep some of the same state (the resize code is much worse).
 */
void move_client(XEvent *xevent)
{
    client_t *client = NULL;
    int x_start, y_start;       /* only used for mouse move */
    int orig_x, orig_y;
    int have_mouse;
    unsigned int init_button;
    XEvent event1;
    enum { CONTINUE, DONE, RESET, RESIZE } action;

    if (moving || sizing) return;
    if (keycode_Escape == 0) set_keys();
    if (xevent->type == ButtonPress) {
        client = client_find(xevent->xbutton.window);
        x_start = xevent->xbutton.x_root;
        y_start = xevent->xbutton.y_root;
        init_button = xevent->xbutton.button;
        have_mouse = 1;
    } else if (xevent->type == KeyPress) {
        client = client_find(xevent->xkey.window);
        have_mouse = 0;
    } else {
        fprintf(stderr, "XWM: Error, move_client called incorrectly\n");
        return;
    }
    if (client == NULL) {
        fprintf(stderr, "XWM: Not moving a non-client\n");
        return;
    }
    moving = 1;
    orig_x = client->x;
    orig_y = client->y;
    
    /* I kind of like being able to move a window
     * without giving it focus, and this doesn't
     * cause any problems: */
    /* focus_set(client); */
    /* focus_ensure(); */
    if (have_mouse)
        XGrabPointer(dpy, root_window, True,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None,
                     cursor_moving, CurrentTime);
    XGrabKeyboard(dpy, root_window, True, GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    display_geometry("Moving", client);

    action = CONTINUE;
    while (action == CONTINUE) {
        XNextEvent(dpy, &event1);
        xevent = &event1;
        
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
            case ButtonPress:
                /* we have a separate event loop here because we want
                 * to gobble up all EnterNotify and LeaveNotify events
                 * while moving or resizing the window */
                /* we also don't care about button presses */
                break;
            
            case MotionNotify:
                if (!have_mouse) break;
                xevent = compress_motion(xevent);
                if (client == NULL) {
                    fprintf(stderr,
                            "XWM: error, null client while moving\n");
                    action = DONE;
                    break;
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
                    if (have_mouse) {
                        XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, -1);
                    } else {
                        client->y--;
                        move_constrain(client);
                    }
                } else if (xevent->xkey.keycode == keycode_Down ||
                           xevent->xkey.keycode == keycode_j) {
                    if (have_mouse) {
                        XWarpPointer(dpy, None, None, 0, 0, 0, 0, 0, 1);
                    } else {
                        client->y++;
                        move_constrain(client);
                    }
                } else if (xevent->xkey.keycode == keycode_Left ||
                           xevent->xkey.keycode == keycode_h) {
                    if (have_mouse) {
                        XWarpPointer(dpy, None, None, 0, 0, 0, 0, -1, 0);
                    } else {
                        client->x--;
                        move_constrain(client);
                    }
                } else if (xevent->xkey.keycode == keycode_Right ||
                           xevent->xkey.keycode == keycode_l) {
                    if (have_mouse) {
                        XWarpPointer(dpy, None, None, 0, 0, 0, 0, 1, 0);
                    } else {
                        client->x++;
                        move_constrain(client);
                    }
                } else if (xevent->xkey.keycode == keycode_Return) {
                    action = DONE;
                    break;
                } else if (xevent->xkey.keycode == keycode_Escape) {
                    client->x = orig_x;
                    client->y = orig_y;
                    action = DONE;
                    break;
                } else if (xevent->xkey.keycode == keycode_Control_L ||
                           xevent->xkey.keycode == keycode_Control_R) {
                    action = RESIZE;
                    break;
                } else {
                    /* can still use alt-tab, etc. while moving */
                    event_dispatch(xevent);
                }
                break;

            case ButtonRelease:
                if (have_mouse && xevent->xbutton.button == init_button) {
                    if (client) {
                        client->x += xevent->xbutton.x_root - x_start;
                        client->y += xevent->xbutton.y_root - y_start;
                    }
                    action = DONE;
                    break;
                }
            
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

    }
        
    if (client != NULL) {
        XMoveWindow(dpy, client->frame, client->x, client->y);
        if (client->name != NULL) free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
        /* must send a synthetic ConfigureNotify to the client
         * according to ICCCM 4.1.5 */
        /* FIXME:  make this a function in client.c */
        if (action != RESET) {
            event1.type = ConfigureNotify;
            event1.xconfigure.display = dpy;
            event1.xconfigure.event = client->window;
            event1.xconfigure.window = client->window;
            event1.xconfigure.x = client->x;
            event1.xconfigure.y = client->y + TITLE_HEIGHT;
            event1.xconfigure.width = client->width;
            event1.xconfigure.height = client->height - TITLE_HEIGHT;
            event1.xconfigure.border_width = 0;
            event1.xconfigure.above = client->frame; /* fixme */
            event1.xconfigure.override_redirect = False;
            XSendEvent(dpy, client->window, False,
                       StructureNotifyMask, &event1);
            XFlush(dpy);
        }
    }

    if (have_mouse) XUngrabPointer(dpy, CurrentTime);
    XUngrabKeyboard(dpy, CurrentTime);
    moving = 0;
    if (action == RESIZE) {
        /* if we want to toggle from a move to a resize, we create
         * a fake XEvent, filling out the fields that the resize
         * code looks at.  The event isn't sent (no X requests are
         * made - it is just used to call the resize code. */
        if (have_mouse) {
            event1.type = ButtonPress;
            event1.xbutton.window = client->window;
            event1.xbutton.x_root = x_start;
            event1.xbutton.y_root = y_start;
            event1.xbutton.button = init_button;
        } else {
            event1.type = KeyPress;
            event1.xkey.window = client->window;
        }
        resize_client(&event1);
    }
}

/* helper for move function, don't let user move window off-screen */
static void move_constrain(client_t *client)
{
    while (client->x + client->width < 0) client->x++;
    while (client->y + client->height < 0) client->y++;
    while (client->x > DisplayWidth(dpy, scr)) client->x--;
    while (client->y > DisplayHeight(dpy, scr)) client->y--;
    XMoveWindow(dpy, client->frame, client->x, client->y);
    display_geometry("Moving", client);
}

/*
 * WARNING:  this gets really, really ugly from here on
 */

void resize_client(XEvent *xevent)
{
    client_t *client = NULL;
    int x_start, y_start, have_mouse, delta;
    resize_direction_t resize_direction = UNKNOWN;
    resize_direction_t old_resize_direction = UNKNOWN;
    position_size orig;
    XEvent event1;
    unsigned int init_button;
    enum { CONTINUE, RESET, DONE, MOVE } action;

    if (moving || sizing) return;
    if (keycode_Escape == 0) set_keys();
    
    if (xevent->type == ButtonPress) {
        have_mouse = 1;
        client = client_find(xevent->xbutton.window);
        x_start = xevent->xbutton.x_root;
        y_start = xevent->xbutton.y_root;
        init_button = xevent->xbutton.button;
        resize_direction =
            get_direction(client, xevent->xbutton.x_root,
                          xevent->xbutton.y_root);
    } else if (xevent->type == KeyPress) {
        have_mouse = 0;
        client = client_find(xevent->xkey.window);
        if (client != NULL) {
            x_start = client->x + client->width;
            y_start = client->y + client->height;
        }
        resize_direction = SE;
    }
    if (client == NULL) {
#ifdef DEBUG
        printf("\tNot resizing a non-client\n");
#endif /* DEBUG */
        return;
    }
    
    sizing = 1;
    orig.x = client->x;
    orig.y = client->y;
    orig.width = client->width;
    orig.height = client->height;
    if (have_mouse) {
        XGrabPointer(dpy, root_window, True,
                     PointerMotionMask | ButtonPressMask
                     | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None,
                     cursor_direction_map[resize_direction],
                     CurrentTime);
    }
    XGrabKeyboard(dpy, root_window, True,
                  GrabModeAsync, GrabModeAsync,
                  CurrentTime);
    process_resize(client, x_start, y_start,
                   resize_direction, old_resize_direction,
                   &x_start, &y_start, &orig, FIRST);

    action = CONTINUE;
    while (action == CONTINUE) {
        XNextEvent(dpy, &event1);
        xevent = &event1;
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
            case ButtonPress:
                break;

            case KeyPress:
                if (xevent->xkey.keycode == keycode_Escape) {
                    action = RESET;
                    break;
                } else if (xevent->xkey.keycode == keycode_Return) {
                    if (have_mouse) {
                        process_resize(client, xevent->xkey.x_root,
                                       xevent->xkey.y_root, resize_direction,
                                       old_resize_direction, &x_start,
                                       &y_start, &orig, LAST);
                    } else {
                        process_resize(client, x_start, y_start,
                                       resize_direction, old_resize_direction,
                                       &x_start, &y_start, &orig, LAST);
                    }
                    action = DONE;
                    break;
                } else if (xevent->xkey.keycode == keycode_Shift_L ||
                           xevent->xkey.keycode == keycode_Shift_R) {
                    if (have_mouse) {
                        cycle_resize_direction_mouse(&resize_direction,
                                                     &old_resize_direction);
                        XChangeActivePointerGrab(dpy,
                                    PointerMotionMask
                                    | ButtonPressMask | ButtonReleaseMask,
                                    cursor_direction_map[resize_direction],
                                    CurrentTime);
                    } else {
                        cycle_resize_direction_keyboard(&resize_direction);
                        switch (resize_direction) {
                            case NE:
                                x_start = client->x + client->width;
                                y_start = client->y;
                                break;
                            case NW:
                                x_start = client->x;
                                y_start = client->y;
                                break;
                            case SW:
                                x_start = client->x;
                                y_start = client->y + client->height;
                                break;
                            case SE:
                                x_start = client->x + client->width;
                                y_start = client->y + client->height;
                                break;
                        }
                    }
                    xrefresh();
                    process_resize(client, x_start, y_start, resize_direction,
                                   old_resize_direction, &x_start, &y_start,
                                   &orig, FIRST);
                } else if (xevent->xkey.keycode == keycode_Up ||
                           xevent->xkey.keycode == keycode_k) {
                    if (resize_direction != EAST && resize_direction != WEST) {
                        delta = get_height_resize_inc(client);
                        if (have_mouse) {
                            XWarpPointer(dpy, None, None, 0, 0, 0, 0,
                                         0, -delta);
                        } else {
                            process_resize(client, x_start, y_start - delta,
                                           resize_direction,
                                           old_resize_direction,
                                           &x_start, &y_start, &orig, MIDDLE);
                        }
                    }
                } else if (xevent->xkey.keycode == keycode_Down ||
                           xevent->xkey.keycode == keycode_j) {
                    if (resize_direction != EAST && resize_direction != WEST) {
                        delta = get_height_resize_inc(client);
                        if (have_mouse) {
                            XWarpPointer(dpy, None, None, 0, 0, 0, 0,
                                         0, delta);
                        } else {
                            process_resize(client, x_start, y_start + delta,
                                           resize_direction,
                                           old_resize_direction,
                                           &x_start, &y_start, &orig, MIDDLE);
                        }
                    }
                } else if (xevent->xkey.keycode == keycode_Left ||
                           xevent->xkey.keycode == keycode_h) {
                    if (resize_direction != NORTH && resize_direction != SOUTH) {
                        delta = get_width_resize_inc(client);
                        if (have_mouse) {
                            XWarpPointer(dpy, None, None, 0, 0, 0, 0,
                                         -delta, 0);
                        } else {
                            process_resize(client, x_start - delta, y_start,
                                           resize_direction,
                                           old_resize_direction,
                                           &x_start, &y_start, &orig, MIDDLE);
                        }
                    }
                } else if (xevent->xkey.keycode == keycode_Right ||
                           xevent->xkey.keycode == keycode_l) {
                    if (resize_direction != NORTH && resize_direction != SOUTH) {
                        delta = get_width_resize_inc(client);
                        if (have_mouse) {
                            XWarpPointer(dpy, None, None, 0, 0, 0, 0,
                                         delta, 0);
                        } else {
                            process_resize(client, x_start + delta, y_start,
                                           resize_direction,
                                           old_resize_direction,
                                           &x_start, &y_start, &orig, MIDDLE);
                        }
                    }
                } else if (xevent->xkey.keycode == keycode_Control_L ||
                           xevent->xkey.keycode == keycode_Control_R) {
                    process_resize(client, x_start, y_start,
                                   resize_direction, old_resize_direction,
                                   &x_start, &y_start, &orig, LAST);
                    action = MOVE;
                    break;
                } else {
                    /* can still use alt-tab while resizing */
                    event_dispatch(xevent);
                }
                break;
                
            case MotionNotify:
                if (!have_mouse) break;
                
                xevent = compress_motion(xevent);

                if (client == NULL) {
                    fprintf(stderr, "XWM: error, null client in resize\n");
                    action = RESET;
                    break;
                }
                process_resize(client, xevent->xbutton.x_root,
                               xevent->xbutton.y_root, resize_direction,
                               old_resize_direction, &x_start, &y_start,
                               &orig, MIDDLE);
                break;
            case ButtonRelease:
                if (have_mouse && xevent->xbutton.button == init_button) {
                    if (client == NULL) {
                        fprintf(stderr, "XWM: error, null client in resize\n");
                        action = RESET;
                        break;
                    }
                    process_resize(client, xevent->xmotion.x_root,
                                   xevent->xmotion.y_root, resize_direction,
                                   old_resize_direction, &x_start, &y_start,
                                   &orig, LAST);
                    action = DONE;
                    break;
                }
            
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
    }
    
    if (action == RESET) {
        xrefresh();
        client->x = orig.x;
        client->y = orig.y;
        client->width = orig.width;
        client->height = orig.height;
    }

    if (client != NULL) {
        XMoveResizeWindow(dpy, client->frame, client->x, client->y,
                          client->width, client->height);
        XResizeWindow(dpy, client->window, client->width,
                      client->height - TITLE_HEIGHT);
        if (client->name != NULL) free(client->name); /* FIXME */
        client_set_name(client);
        client_paint_titlebar(client);
    }

    if (have_mouse) XUngrabPointer(dpy, CurrentTime);
    XUngrabKeyboard(dpy, CurrentTime);
    sizing = 0;
    if (action == MOVE) {
        if (have_mouse) {
            event1.type = ButtonPress;
            event1.xbutton.window = client->window;
            event1.xbutton.x_root = x_start;
            event1.xbutton.y_root = y_start;
            event1.xbutton.button = init_button;
        } else {
            event1.type = KeyPress;
            event1.xkey.window = client->window;
        }
        move_client(&event1);
    }
}

static void cycle_resize_direction_keyboard(resize_direction_t *d)
{
    switch (*d) {
        case SE:
            *d = NE;
            break;
        case NE:
            *d = NW;
            break;
        case NW:
            *d = SW;
            break;
        default:
            *d = SE;
    }
}

/*
 * SW -> SOUTH -> WEST -> SW -> ....
 * SE -> SOUTH -> EAST -> SW -> ....
 * NE -> NORTH -> EAST -> NE -> ....
 * NW -> NORTH -> WEST -> NW -> ....
 */

static void cycle_resize_direction_mouse(resize_direction_t *current,
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
    XSync(dpy, 0);
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
    if (client->xsh != NULL && (client->xsh->flags & PResizeInc)) {
        if (y_diff >= client->xsh->height_inc
            || (-y_diff) >= client->xsh->height_inc) {
            y_diff -= (y_diff % client->xsh->height_inc);
            if (y_diff < 0 && y_diff % client->xsh->height_inc != 0)
                y_diff += client->xsh->height_inc;
        } else {
            y_diff = 0;
        }
        if (x_diff >= client->xsh->width_inc
            || (-x_diff) >= client->xsh->width_inc) {
            x_diff -= (x_diff % client->xsh->width_inc);
            if (x_diff < 0 && x_diff % client->xsh->width_inc != 0)
                x_diff += client->xsh->width_inc;
        } else {
            x_diff = 0;
        }
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

    w_inc = get_width_resize_inc(client);
    h_inc = get_height_resize_inc(client);
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

    width = client->width / get_width_resize_inc(client);
    height = (client->height - TITLE_HEIGHT) / get_height_resize_inc(client);
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
    keycode_Control_L = XKeysymToKeycode(dpy, XK_Control_L);
    keycode_Control_R = XKeysymToKeycode(dpy, XK_Control_R);
}

static int get_width_resize_inc(client_t *client)
{
    if (client->xsh != NULL
        && client->xsh->flags & PResizeInc) {
        return client->xsh->width_inc;
    } else {
        return 1;
    }
}

static int get_height_resize_inc(client_t *client)
{
    if (client->xsh != NULL
        && client->xsh->flags & PResizeInc) {
        return client->xsh->height_inc;
    } else {
        return 1;
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
