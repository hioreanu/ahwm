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

void mouse_grab_buttons(window w)
{
    xgrabbutton(dpy, button1, mod1mask, w, true, buttonpressmask,
                grabmodesync, grabmodeasync, none, cursor_moving);
    xgrabbutton(dpy, button3, mod1mask, w, true, buttonpressmask,
                grabmodesync, grabmodeasync, none, cursor_normal);
}

/*
 * this takes over the event loop and grabs the pointer until the move
 * is complete
 */
void mouse_move_client(xevent *xevent)
{
    static client_t *client = null;
    static int x_start, y_start;
    xevent event1;

    do {
        switch (xevent->type) {
            case enternotify:
            case leavenotify:
                /* we have a separate event loop here because we want
                 * to gobble up all enternotify and leavenotify events
                 * while moving or resizing the window */
                break;
            case buttonpress:
                if (xevent->xbutton.button == button1) {
                    if (client != null) {
                        /* received button1 down while moving a window */
                        fprintf(stderr, "unexpected button1 down\n");
                        goto reset;
                    } else if (xevent->xbutton.state & mod1mask) {
                        /* received normal mod1 + button1 press */
                        client = client_find(xevent->xbutton.window);
                        if (client == null) {
                            /* error - we should never do xgrabbutton() on
                             * an override_redirect window */
                            fprintf(stderr, "not moving a non-client\n");
                            return;
                        }
                        /* i kind of like being able to move a window
                         * without giving it focus, and this doesn't
                         * cause any problems: */
                        /* focus_set(client); */
                        /* focus_ensure(); */
                        x_start = xevent->xbutton.x_root;
                        y_start = xevent->xbutton.y_root;
#ifdef debug
                        printf("\tgrabbing the mouse for moving\n");
#endif /* debug */
                        xgrabpointer(dpy, root_window, true,
                                     pointermotionmask | buttonpressmask
                                     | buttonreleasemask,
                                     grabmodeasync, grabmodeasync, none,
                                     cursor_moving, currenttime);
                        display_geometry("moving", client);
                    } else {
                        /* error - we were called from somewhere else
                         * to deal with a button press that's not ours */
                        fprintf(stderr,
                                "xwm: received an unknown button press\n");
                    }
                } else {
                    /* not an error - user may have clicked another button
                     * while resizing a window */
#ifdef debug
                    printf("\tignoring stray button press\n");
#endif /* debug */
                }
                break;
            
            case motionnotify:
                xevent = compress_motion(xevent);

                /* this can happen with the motion compressing */
                if (!(xevent->xmotion.state & button1mask)) {
                    fprintf(stderr,
                            "xwm: motion event without correct button\n");
                    goto reset;
                }
                if (client == null) {
                    fprintf(stderr,
                            "xwm: error, null client while moving\n");
                    goto reset;
                }
                /* just move the window */
                client->x += xevent->xbutton.x_root - x_start;
                client->y += xevent->xbutton.y_root - y_start;

                /* take out these two lines for a fun effect :) */
                x_start = xevent->xbutton.x_root;
                y_start = xevent->xbutton.y_root;
                xmovewindow(dpy, client->frame, client->x, client->y);
                display_geometry("moving", client);

                break;
            case buttonrelease:
                if (client) {
                    client->x += xevent->xbutton.x_root - x_start;
                    client->y += xevent->xbutton.y_root - y_start;
                }
                goto reset;
            
                break;
            default:
#ifdef debug
                printf("\tstart recursive event processing\n");
#endif /* debug */
                event_dispatch(xevent);
#ifdef debug
                printf("\tend recursive event processing\n");
#endif /* debug */
                break;
        }

        if (client != null) {
            xnextevent(dpy, &event1);
            xevent = &event1;
        }

        /* static client is set to null in reset code below (this
         * function may be nested arbitrarily */
    } while (client != null);
        
    return;
    
 reset:

    if (client != null) {
        xmovewindow(dpy, client->frame, client->x, client->y);
        if (client->name != null) free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
        /* must send a synthetic configurenotify to the client
         * according to icccm 4.1.5 */
        /* fixme:  make this a function in client.c */
        /* fixme:  breaks with netscape */
        event1.type = configurenotify;
        event1.xconfigure.display = dpy;
        event1.xconfigure.event = client->window;
        event1.xconfigure.window = client->window;
        event1.xconfigure.x = client->x;
        event1.xconfigure.y = client->y - title_height;
        event1.xconfigure.width = client->width;
        event1.xconfigure.height = client->height - title_height;
        event1.xconfigure.border_width = 0;
        event1.xconfigure.above = client->frame; /* fixme */
        event1.xconfigure.override_redirect = false;
        xsendevent(dpy, client->window, false, structurenotifymask, &event1);
        xflush(dpy);
    }

#ifdef debug
    printf("\tungrabbing the mouse for move\n");
#endif /* debug */

    client = null;
    x_start = y_start = -1;
    xungrabpointer(dpy, currenttime);
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

void mouse_resize_client(xevent *xevent)
{
    static client_t *client = null;
    static int x_start, y_start;
    static resize_direction_t resize_direction = unknown;
    static quadrant_t quadrant = iv;
    static position_size orig;
    xevent event1;
    
    do {
        switch (xevent->type) {
            case enternotify:
            case leavenotify:
                break;
            case buttonpress:
                if (xevent->xbutton.button == button3) {
                    if (client != null) {
                        /* received button3 down while sizing a window */
                        fprintf(stderr, "xwm: unexpected button3 down\n");
                        goto reset;
                    } else if (xevent->xbutton.state & mod1mask) {
                        client = client_find(xevent->xbutton.window);
                        if (client == null) {
#ifdef debug
                            printf("\tnot resizing a non-client\n");
#endif /* debug */
                            return;
                        }
                        x_start = xevent->xbutton.x_root;
                        y_start = xevent->xbutton.y_root;
                        quadrant = get_quadrant(client, xevent->xbutton.x_root,
                                                xevent->xbutton.y_root);
                        switch (quadrant) {
                            case i:
                                resize_direction = ne;
                                break;
                            case ii:
                                resize_direction = nw;
                                break;
                            case iii:
                                resize_direction = sw;
                                break;
                            case iv:
                                resize_direction = se;
                                break;
                        }
                        orig.x = client->x;
                        orig.y = client->y;
                        orig.width = client->width;
                        orig.height = client->height;
#ifdef debug
                        printf("\tgrabbing the mouse for resizing\n");
#endif /* debug */
                        xgrabpointer(dpy, root_window, true,
                                     pointermotionmask | buttonpressmask
                                     | buttonreleasemask,
                                     grabmodeasync, grabmodeasync, none,
                                     cursor_direction_map[resize_direction],
                                     currenttime);
                        process_resize(client, x_start, y_start,
                                       resize_direction, quadrant,
                                       &x_start, &y_start, &orig, first);
                        
                    } else {
                        fprintf(stderr,
                                "received an unexpected button press\n");
                    }
                } else {
#ifdef debug
                    printf("\tignoring stray button press\n");
#endif /* debug */
                }
                break;
            
            case motionnotify:
                xevent = compress_motion(xevent);

                /* this can happen with motion compression */
                if (!(xevent->xmotion.state & button3mask)) {
                    fprintf(stderr,
                            "xwm: motion event w/o button while moving\n");
                    goto reset;
                }
                if (client == null) {
                    fprintf(stderr, "xwm: error, null client in resize\n");
                    goto reset;
                }

#ifdef debug
                printf("sizing from (%d,%d) to (%d,%d)\n",
                       x_start, y_start, xevent->xbutton.x_root,
                       xevent->xbutton.y_root);
#endif /* debug */
                process_resize(client, xevent->xbutton.x_root,
                               xevent->xbutton.y_root, resize_direction,
                               quadrant, &x_start, &y_start, &orig, middle);
                break;
            case buttonrelease:
                if (client == null) {
                    fprintf(stderr, "xwm: error, null client in resize\n");
                    goto reset;
                }
                process_resize(client, xevent->xmotion.x_root,
                               xevent->xmotion.y_root, resize_direction,
                               quadrant, &x_start, &y_start, &orig, last);
                goto done;
            
                break;
            default:
#ifdef debug
                printf("\tstart recursive event processing\n");
#endif /* debug */
                event_dispatch(xevent);
#ifdef debug
                printf("\tend recursive event processing\n");
#endif /* debug */
                break;
        }
        xnextevent(dpy, &event1);
        xevent = &event1;
    } while (client != null);
        
    return;
    
 reset:
    xclearwindow(dpy, root_window);

 done:

    if (client != null) {
        xmoveresizewindow(dpy, client->frame, client->x, client->y,
                          client->width, client->height);
        xresizewindow(dpy, client->window, client->width,
                      client->height - title_height);
        if (client->name != null) free(client->name);
        client_set_name(client);
        client_paint_titlebar(client);
    }

#ifdef debug
    printf("\tungrabbing the mouse\n");
#endif /* debug */

    client = null;
    x_start = y_start = -1;
    resize_direction = unknown;
    xungrabpointer(dpy, currenttime);
}

void mouse_handle_event(xevent *xevent)
{
    if (xevent->type != buttonpress) {
        fprintf(stderr, "xwm: error, mouse_handle_event called incorrectly\n");
        return;
    }
    if (xevent->xbutton.button == button1
        && (xevent->xbutton.state & mod1mask)) {
        mouse_move_client(xevent);
    } else if (xevent->xbutton.button == button3
               && (xevent->xbutton.state & mod1mask)) {
        mouse_resize_client(xevent);
    } else {
#ifdef debug
        printf("\tignoring unknown mouse event\n");
#endif /* debug */
    }
}

/* compress motion events, idea taken from windowmaker */
/* returns most recent event to deal with */
/* fixme:  deal with this more elegantly, process events
 * which do not have right modifiers separately */
static xevent *compress_motion(xevent *xevent)
{
    static xevent newer;

    while (xcheckmaskevent(dpy, buttonmotionmask, &newer)) {
        if (newer.type == motionnotify
            && newer.xmotion.window == xevent->xmotion.window
            && newer.xmotion.state == xevent->xmotion.state) {
            xevent = &newer;
#ifdef debug
            printf("\tmotion event compressed (%d,%d)\n",
                   xevent->xmotion.x_root, xevent->xmotion.y_root);
#endif /* debug */
        } else {
            /* can't happen */
            fprintf(stderr,
                    "xwm: accidentally ate up an event!\n");
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
    if (direction == west || direction == east)
        y_diff = 0;
    if (direction == north || direction == south)
        x_diff = 0;
#ifdef debug
    printf("\tx_diff = %d, y_diff = %d\n", x_diff, y_diff);
#endif /* debug */
    if (client->xsh != null && (client->xsh->flags & presizeinc)) {
#ifdef debug
        printf("\txsh->width_inc = %d, xsh->height_inc = %d\n",
               client->xsh->width_inc, client->xsh->height_inc);
#endif /* debug */
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
#ifdef debug
        printf("\t(after) x_diff = %d, y_diff = %d\n", x_diff, y_diff);
#endif /* debug */
    }

    if (x_diff != 0) {
        switch (direction) {
            case west:
            case sw:
            case nw:
                if (client->width - x_diff > 1
                    && new_x < orig->x + orig->width) {
                    if (client->xsh->flags & pminsize
                        && client->width - x_diff < client->xsh->min_width)
                        break;
                    if (client->xsh->flags & pmaxsize
                        && client->width - x_diff > client->xsh->max_width)
                        break;
                    client->x += x_diff;
                    client->width -= x_diff;
                    *old_x += x_diff;
                }
                break;
            case east:
            case se:
            case ne:
                if (client->width + x_diff > 1) {
                    if (client->xsh->flags & pminsize
                        && client->width + x_diff < client->xsh->min_width)
                        break;
                    if (client->xsh->flags & pmaxsize
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
            case north:
            case nw:
            case ne:
                if (client->height - y_diff > title_height + 1
                    && new_y < orig->y + orig->height) {
                    if (client->xsh->flags & pminsize
                        && client->height - y_diff <
                           client->xsh->min_height + title_height)
                        break;
                    if (client->xsh->flags & pmaxsize
                        && client->height - y_diff >
                           client->xsh->max_height + title_height)
                        break;
                    client->y += y_diff;
                    client->height -= y_diff;
                    *old_y += y_diff;
                }
                break;
            case south:
            case se:
            case sw:
                if (client->height + y_diff > title_height + 1) {
                    if (client->xsh->flags & pminsize
                        && client->height + y_diff <
                           client->xsh->min_height + title_height)
                        break;
                    if (client->xsh->flags & pmaxsize
                        && client->height + y_diff >
                           client->xsh->max_height + title_height)
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

    if (ordinal != middle || x != client->x
        || w != client->width || y != client->y) {
        /* redraw top bar */
        if (ordinal != first) {
            xdrawline(dpy, root_window, root_invert_gc,
                      x, y, x + w, y);
            if (direction == north || direction == nw || direction == ne)
                drafting_lines(client, north, x, y, x + w, y);
        }
        if (ordinal != last) {
            xdrawline(dpy, root_window, root_invert_gc, client->x,
                      client->y, client->x + client->width, client->y);
            if (direction == north || direction == nw || direction == ne)
                drafting_lines(client, north, client->x, client->y,
                               client->x + client->width, client->y);
        }
    }
    if (ordinal != middle || x != client->x || w != client->width
        || y + h != client->y + client->height) {
        /* redraw bottom bar */
        if (ordinal != first) {
            xdrawline(dpy, root_window, root_invert_gc,
                      x, y + h, x + w, y + h);
            if (direction == south || direction == sw || direction == se)
                drafting_lines(client, south, x, y + h, x + w, y + h);
        }
        if (ordinal != last) {
            xdrawline(dpy, root_window, root_invert_gc,
                      client->x, client->y + client->height,
                      client->x + client->width,
                      client->y + client->height);
            if (direction == south || direction == sw || direction == se)
                drafting_lines(client, south, client->x,
                               client->y + client->height,
                               client->x + client->width,
                               client->y + client->height);
        }
    }
    if (ordinal != middle || y != client->y ||
        h != client->height || x != client->x) {
        /* redraw left bar */
        if (ordinal != first) {
            xdrawline(dpy, root_window, root_invert_gc,
                      x, y, x, y + h);
            if (direction == west || direction == nw || direction == sw)
                drafting_lines(client, west, x, y, x, y + h);
                
        }
        if (ordinal != last) {
            xdrawline(dpy, root_window, root_invert_gc,
                      client->x, client->y, client->x,
                      client->y + client->height);
            if (direction == west || direction == nw || direction == sw)
                drafting_lines(client, west, client->x, client->y,
                               client->x, client->y + client->height);
        }
    }
    if (ordinal != middle || y != client->y || h != client->height
        || x + w != client->x + client->width) {
        /* redraw right bar */
        if (ordinal != first) {
            xdrawline(dpy, root_window, root_invert_gc,
                      x + w, y, x + w, y + h);
            if (direction == east || direction == ne || direction == se)
                drafting_lines(client, east, x + w, y, x + w, y + h);
        }
        if (ordinal != last) {
            xdrawline(dpy, root_window, root_invert_gc,
                      client->x + client->width,
                      client->y,
                      client->x + client->width,
                      client->y + client->height);
            if (direction == east || direction == ne || direction == se)
                drafting_lines(client, east, client->x + client->width,
                      client->y, client->x + client->width,
                      client->y + client->height);
        }
    }

    if (ordinal == first || x != client->x || y != client->y
        || w != client->width || h != client->height) {
        display_geometry("resizing", client);
    }
}

static void get_display_width_height(client_t *client, int *w, int *h)
{
    int w_inc, h_inc;
    
    if (client->xsh != null && (client->xsh->flags & presizeinc)) {
        w_inc = client->xsh->width_inc;
        h_inc = client->xsh->height_inc;
    } else {
        w_inc = h_inc = 1;
    }
    *w = client->width / w_inc;
    *h = (client->height - title_height) / h_inc;
}

#define drafting_offset 15

static void drafting_lines(client_t *client, resize_direction_t direction,
                           int x1, int y1, int x2, int y2)
{
    int display_int, h_inc, w_inc;
    
    if (client->xsh != null && (client->xsh->flags & presizeinc)) {
        w_inc = client->xsh->width_inc;
        h_inc = client->xsh->height_inc;
    } else {
        w_inc = h_inc = 1;
    }
    
    if (direction == WEST) {
        x1 -= DRAFTING_OFFSET;
        x2 -= DRAFTING_OFFSET;
    } else if (direction == EAST) {
        x1 += DRAFTING_OFFSET;
        x2 += DRAFTING_OFFSET;
    } else if (direction == NORTH) {
        y1 -= DRAFTING_OFFSET;
        y2 -= DRAFTING_OFFSET;
    } else if (direction == SOUTH) {
        y1 += DRAFTING_OFFSET;
        y2 += DRAFTING_OFFSET;
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
