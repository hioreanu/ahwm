/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "mouse.h"
#include "client.h"
#include <stdio.h>

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
    static client_t *moving = NULL;
    static client_t *sizing = NULL;
    static int x_start, y_start;
    XEvent tmp_event;
    
    switch (xevent->type) {
        case ButtonPress:
            if (xevent->xbutton.button == Button1) {
                if (moving != NULL) {
                    /* Received button1 down while moving a window */
                    fprintf(stderr, "Can't happen\n");
                    goto reset;
                } else if (xevent->xbutton.state & Mod1Mask) {
#ifdef DEBUG
                    printf("\tGrabbing the mouse\n");
#endif /* DEBUG */
                    moving = client_find(xevent->xbutton.window);
                    if (moving == NULL) {
#ifdef DEBUG
                        printf("\tNot moving a non-client\n");
#endif /* DEBUG */
                        return;
                    }
                    /* I kind of like being able to move a window
                     * without giving it focus, and this doesn't
                     * raise any problems: */
                    /* focus_set(moving); */
                    /* focus_ensure(); */
                    x_start = xevent->xbutton.x_root;
                    y_start = xevent->xbutton.y_root;
                    XGrabPointer(dpy, root_window, True,
                                 PointerMotionMask | ButtonPressMask |
                                 ButtonReleaseMask,
                                 GrabModeAsync, GrabModeAsync, None,
                                 cursor_moving, CurrentTime);
                }
            } else if (xevent->xbutton.button == Button3) {
                if (sizing != NULL) {
                    /* Received button3 down while sizing a window */
                    fprintf(stderr, "Can't happen\n");
                    goto reset;
                } else if (xevent->xbutton.state & Mod1Mask) {
                    sizing = client_find(xevent->xbutton.window);
                    /* FIXME: set the cursor, input mask, grab pointer, etc. */
                }
            }
            /* else ignore the event */
            break;
            
        case MotionNotify:
            /* compress motion events, idea taken from WindowMaker */
            tmp_event.type = LASTEvent;
            while (XCheckMaskEvent(dpy, ButtonMotionMask, &tmp_event)) {
                /* we can only compress events for which the window,
                 * buttons and modifiers are the same since our processing
                 * depends on those things; if we find a future event that
                 * differs from the event we are currently processing,
                 * we finish processing the current event and then process
                 * the the future event (a bit out of order, but it should
                 * work nicely)
                 */
                if (tmp_event.xmotion.window == xevent->xmotion.window
                    && tmp_event.xmotion.state == xevent->xmotion.state) {
#ifdef DEBUG
                    printf("\tMotion event compressed\n");
#endif /* DEBUG */
                    xevent = &tmp_event;
                }
            }

            if (moving && !(xevent->xmotion.state & Button1Mask)) {
                fprintf(stderr, "Can't happen\n");
                goto reset;
            } else if (sizing && !(xevent->xmotion.state & Button3Mask)) {
                fprintf(stderr, "Can't happen\n");
                goto reset;
                /* FIXME: reset input masks, cursors, etc. */
            }

            if (sizing) {
                /* just resize the window */
            } else if (moving) {
                /* just move the window */
                moving->x += xevent->xbutton.x_root - x_start;
                moving->y += xevent->xbutton.y_root - y_start;

                /* take out these two lines for a fun effect :) */
                x_start = xevent->xbutton.x_root;
                y_start = xevent->xbutton.y_root;
                XMoveWindow(dpy, moving->frame, moving->x, moving->y);
            }
            
            if (xevent != &tmp_event && tmp_event.type != LASTEvent) {
#ifdef DEBUG
                printf("\tProcessing leftover event\n");
#endif /* DEBUG */
                mouse_handle_event(&tmp_event);
            }
            break;
        case ButtonRelease:
            moving->x += xevent->xbutton.x_root - x_start;
            moving->y += xevent->xbutton.y_root - y_start;
            goto reset;
            
            break;
        default:
            fprintf(stderr, "You've found an xwm bug, contact the author\n");
            break;
    }

    return;
    
 reset:

    if (moving != NULL) {
        XMoveWindow(dpy, moving->frame, moving->x, moving->y);
    }
    if (sizing != NULL) {
        XResizeWindow(dpy, sizing->frame, sizing->width, sizing->height);
    }

#ifdef DEBUG
    printf("\tUngrabbing the mouse\n");
#endif /* DEBUG */

    moving = sizing = NULL;
    XUngrabPointer(dpy, CurrentTime);
}
