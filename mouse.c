/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "mouse.h"
#include "client.h"

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
                GrabModeSync, GrabModeAsync, None, cursor_sizing);
}

void mouse_handle_event(XEvent *xevent)
{
    static client moving = NULL;
    static client sizing = NULL;
    XEvent tmp_event;
    
    switch (xevent->type) {
        case ButtonPress:
            if (xevent->xbutton->button == Button1) {
                if (moving != NULL) {
                    /* Received button1 down while moving a window */
                    fprintf(stderr, "Can't happen\n");
                    moving = sizing = 0;
                    /* FIXME: reset input masks, cursors, etc. */
                } else if (xevent->xbutton->state & Mod1Mask) {
                    moving = 1;
                    /* FIXME: set the cursor, input mask, grab pointer, etc. */
                }
            } else if (xevent->xbutton->button == Button3) {
                if (sizing != NULL) {
                    /* Received button3 down while sizing a window */
                    fprintf(stderr, "Can't happen\n");
                    moving = sizing = 0;
                    /* FIXME: reset input masks, cursors, etc. */
                } else if (xevent->xbutton->state & Mod1Mask) {
                    sizing = 1;
                    /* FIXME: set the cursor, input mask, grab pointer, etc. */
                }
            }
            /* else ignore the event */
            break;
            
        case MotionEvent:
            /* compress motion events, idea taken from WindowMaker */
            while (XCheckMaskEvent(dpy, ButtonMotionMask, &tmp_event)) {
                /* we can only compress events for which the window,
                 * buttons and modifiers are the same since our processing
                 * depends on those things; if we find a future event that
                 * differs from the event we are currently processing,
                 * we finish processing the current event and then process
                 * the the future event (a bit out of order, but it should
                 * work nicely)
                 */
                if (tmp_event->xmotion->window == xevent->xmotion->window
                    && tmp_event->xmotion->state == xevent->xmotion->state) {
                    xevent = &tmp_event;
                }
            }

            if (moving && (!(xevent->xmotion->state & Mod1Mask)
                           || !(xevent->xmotion->state & Button1Mask))) {
                fprintf(stderr, "Can't happen\n");
                moving = sizing = 0;
                /* FIXME: reset input masks, cursors, etc. */
            } else if (sizing &&
                       (!(xevent->xmotion->state & Mod1Mask)
                        || !(xevent->xmotion->state & Button3Mask))) {
                fprintf(stderr, "Can't happen\n");
                moving = sizing = 0;
                /* FIXME: reset input masks, cursors, etc. */
            }

            if (sizing) {
                /* just resize the window */
            } else if (moving) {
                /* just move the window */
            }
            
            /* process the event, move things, etc. */
            break;
        case ButtonRelease:
            break;
    }
}
