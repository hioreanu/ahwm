/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * Xlib calls it a "Pointer" which is more correct (I'm using a
 * trackpad in fact), but I'm calling it a "mouse" out of habit.
 */

#ifndef MOUSE_H
#define MOUSE_H

#include <X11/Xlib.h>
#include "client.h"

/* Functions which are called in response to mouse events: */

typedef void (*mouse_fn)(XEvent *);

/*
 * An example function of the above type which does nothing
 */

void mouse_ignore(XEvent *);

/*
 * FIXME:  document
 */

void mouse_set_function_ex(unsigned int button, unsigned int modifiers,
                           int depress, int location, mouse_fn fn);

#define MOUSE_DEPRESS ButtonPress
#define MOUSE_RELEASE ButtonRelease

#define MOUSE_NOWHERE    00
#define MOUSE_TITLEBAR   01
#define MOUSE_ROOT       02
#define MOUSE_FRAME      04
#define MOUSE_EVERYWHERE (MOUSE_NOWHERE | MOUSE_TITLEBAR | \
                          MOUSE_ROOT | MOUSE_FRAME)

void mouse_set_function(char *mousestring, int depress,
                        int location, mouse_fn fn);

int mouse_parse_string(char *mousestring, unsigned int *button,
                       unsigned int *modifiers);

/*
 * Grab the mouse buttons we use for a specified window
 * 
 * NB:  while you should call keyboard_grab_keys() on every window
 * (including those with override_redirect), you should NOT grab the
 * mouse buttons of any window with override_redirect.  This does not
 * check for override_redirect.
 */

void mouse_grab_buttons(client_t *client);

/*
 * Whenever a mouse event is received it should be passed to this
 * function; this includes XButtonEvent and XMotionNotify, etc.  The
 * pointer will NOT be grabbed when this function returns.
 */

void mouse_handle_event(XEvent *);

#endif /* MOUSE_H */
