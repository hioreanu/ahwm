/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include "mouse.h"
#include "move-resize.h"
#include "xwm.h"
#include "cursor.h"

void mouse_grab_buttons(Window w)
{
    XGrabButton(dpy, Button1, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_moving);
    XGrabButton(dpy, Button3, Mod1Mask, w, True, ButtonPressMask,
                GrabModeSync, GrabModeAsync, None, cursor_normal);
}

void mouse_handle_event(XEvent *xevent)
{
    if (xevent->type != ButtonPress) {
        fprintf(stderr, "XWM: error, mouse_handle_event called incorrectly\n");
        return;
    }
    if (xevent->xbutton.button == Button1
        && (xevent->xbutton.state & Mod1Mask)) {
        move_resize_meta_button1(xevent);
    } else if (xevent->xbutton.button == Button3
               && (xevent->xbutton.state & Mod1Mask)) {
        move_resize_meta_button3(xevent);
    } else {
#ifdef DEBUG
        printf("\tIgnoring unknown mouse event\n");
#endif /* DEBUG */
    }
}
