/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef CURSORS_H
#define CURSORS_H

#include <X11/Xlib.h>

/*
 * The available cursor objects; defined in cursors.c, initialized in
 * the function below
 */

extern Cursor cursor_normal;
extern Cursor cursor_moving;
extern Cursor cursor_sizing;

/*
 * these are used by the resizing code (indexed by resize_direction_t):
 */

extern Cursor cursor_direction_map[9];

/*
 * Initialize the above cursor objects
 * 
 * Returns 1 if things went OK, 0 if there was a problem
 */

int cursor_init();

#endif /* CURSORS_H */
