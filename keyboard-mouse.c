/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "keyboard.h"
#include "client.h"

/*
 * This is pretty fucked up.  Xlib is a really great API - very
 * flexible, extensible, easy to use, but the shit they do with
 * modifier keys is frightening.  If I'm reading ICCCM correctly, all
 * of this is necessary, even for the simplest application.
 */

void keyboard_grab_keys(Window w)
{
    printf("Grabbing keys of window 0x%08X\n", w);
    XGrabKey(dpy, XK_a, ControlMask, w, True,
             GrabModeAsync, GrabModeAsync);
/*
    XGrabKey(dpy, XK_a, 0, w, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XK_a, Mod1Mask, w, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XK_a, Mod2Mask, w, True,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XK_a, ShiftMask, w, True,
             GrabModeAsync, GrabModeAsync);
*/
}
