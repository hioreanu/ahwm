/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef MOUSE_H
#define MOUSE_H

#include <X11/XEvent.h>

/*
 * Grab the mouse buttons we use for a specified window
 * 
 * NB:  while you should call keyboard_grab_keys() on every window
 * (including those with override_redirect), you should NOT grab the
 * mouse buttons of any window with override_redirect.  This does not
 * check for override_redirect.
 */

void mouse_grab_buttons(Window);

/*
 * Whenever a mouse event is received it should be passed to this
 * function; this include XButtonEvent and XMotionNotify, etc.
 */

void mouse_handle_event(XEvent *);

#endif /* MOUSE_H */
