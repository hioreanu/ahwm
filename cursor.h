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
extern Cursor cursor_sizing_nw_se;
extern Cursor cursor_sizing_ne_sw;
extern Cursor cursor_sizing_n_s;
extern Cursor cursor_sizing_e_w;

/*
 * Initialize the above cursor objects
 * 
 * Returns 1 if things went OK, 0 if there was a problem
 */

int cursor_init();

#endif /* CURSORS_H */
