/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef MOVE_RESIZE_H
#define MOVE_RESIZE_H

#include <X11/Xlib.h>

void move_resize_meta_button1(XEvent *xevent);
void move_resize_meta_button3(XEvent *xevent);
void move_client(XEvent *xevent, unsigned int init_state,
                 unsigned int init_button, unsigned int init_button_mask);
void resize_client(XEvent *xevent, unsigned int init_state,
                   unsigned int init_button, unsigned int init_button_mask);

#endif /* MOVE_RESIZE_H */
