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

/*
 * This gets a bit hairy....
 */
void mouse_handle_event(XEvent *xevent)
{
    static client_t *moving = NULL;
    static client_t *sizing = NULL;
    static int x_start, y_start;
    int done = 0;
    XEvent event1, event2;

    while (!done) {
        switch (xevent->type) {
            case EnterNotify:
            case LeaveNotify:
                /* we have a separate event loop here because we want
                 * to gobble up all EnterNotify and LeaveNotify events
                 * while moving or resizing the window */
                break;
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
                        /* FIXME: set the cursor, input mask, etc. */
                    }
                }
                /* else ignore the event */
                break;
            
            case MotionNotify:
                /* compress motion events, idea taken from WindowMaker */
                while (XCheckMaskEvent(dpy, ButtonMotionMask, &event2)) {
                    if (event2.type == MotionNotify
                        && event2.xmotion.window == xevent->xmotion.window
                        && event2.xmotion.state == xevent->xmotion.state) {
#ifdef DEBUG
                        printf("\tMotion event compressed\n");
#endif /* DEBUG */
                        xevent = &event2;
                    } else {
                        /* Can't happen */
                        fprintf(stderr,
                                "Accidentally ate up an event!\n");
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
            
                if (event2.type != LASTEvent) {
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
