/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef MOVE_RESIZE_H
#define MOVE_RESIZE_H

#include <X11/Xlib.h>

/*
 * Functions for moving and resizing a client window, suitable for
 * binding to a mouse click or a keyboard press.
 * 
 * Whether invoked from mouse or keyboard, the following keys are
 * available for both move and resize (keyboard is grabbed):
 * 
 * Enter - End the move/resize, applying the changes
 * Escape - End the move/resize, discarding the changes
 * Control - change from resizing to moving and vice versa
 * Up, Down, Left, Right, j, k, h, l, w, a, s, d - move or resize
 * Shift + one of above - move to extreme edge or resize by 10 units
 * 
 * When invoked from a mouse button down event, the mouse can also be
 * used to move/resize the client (mouse is grabbed).  If invoked from
 * the keyboard, does not grab the pointer.
 * 
 * When resizing with the mouse, the quadrant of the client which
 * contains the pointer determines the resize direction (Up+Left,
 * Down+Right, etc.).  When invoked from the keyboard, the resize
 * direction will initially be Down+Right.
 * 
 * Additionally, when resizing, hitting the spacebar will do the
 * following:
 * 
 * keyboard resize:  cycle the resize direction in the following order:
 *                   Down+Right -> Up+Right -> Up+Left -> Down+Left -> ....
 * 
 * mouse resize:  cycle the resize direction based upon the initial
 *                resize direction; for example:
 *                Down+Right -> Down -> Right -> Down+Right -> ....
 * 
 * These functions will grab the keyboard and may grab the mouse; they
 * also have their own event loops.  It does not make sense to move
 * and resize at the same time (use Control key to toggle).  The void
 * * argument is ignored.
 */

void move_client(XEvent *xevent, void *v);
void resize_client(XEvent *xevent, void *v);

/*
 * Toggle maximization state; if client->prev_width == -1 or
 * client->prev_heigth == -1, then the client is NOT maximized; else
 * the client is maximized and client->prev* holds the previous window
 * configuration.  This manipulates those appropriate attributes and
 * toggles the maximization state.  The void * argument is ignored.
 */

void resize_maximize(XEvent *xevent, void *v);

#endif /* MOVE_RESIZE_H */
