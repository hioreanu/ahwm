/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <X11/Xlib.h>
#include "client.h"

/*
 * Do a "soft" grab on all the keys that are of interest to us - this
 * should be called once when the window is created (before the client
 * has a chance to call XGrabKeys on the newly-created window).
 */

void keyboard_grab_keys(Window);

/*
 * Process a keyboard event - just edit this function if
 * you want to change any keybindings.
 */

void keyboard_process_key(int keycode, unsigned int modifiers);

#endif /* KEYBOARD_H */
