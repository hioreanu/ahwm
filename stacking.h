/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef STACKING_H
#define STACKING_H

#include "config.h"

#include <X11/Xlib.h>

#include "client.h"

/*
 * All the functions here will change the stacking order on screen
 * according to the following rules (defining precedence):
 * 
 * 1.  if (A.keep_on_bottom and not B.keep_on_bottom) A is under B
 * 2.  if (A.keep_on_top and not B.keep_on_top) A is on top of B
 * 3.  A client's transients are on top of the client
 * 
 * Each of these functions call XRestackWindows(), and they work on
 * clients that are not mapped.
 */

/*
 * if this is not None, this window will be kept on top of all other
 * windows
 */

extern Window stacking_hiding_window;

/*
 * if this is not None, this will be kept under all other windows
 */

extern Window stacking_desktop_window;

/*
 * add a client to the top of stacking list, ignoring transients
 */

void stacking_add(client_t *client);

/*
 * remove client from stacking list
 */

void stacking_remove(client_t *client);

/*
 * Returns the top-most window
 */
client_t *stacking_top();

/*
 * This will:
 * 
 * 1. raise the client to the top of the stacking list
 * 
 * 2. raise the client's transients above the client
 * 
 * 3. raise the client (and its transients) to the top of the
 *    transients mapped on top of the client's leader
 * 
 * 3. map each window in the entire transience tree which is in the
 *    current workspace
 */

void stacking_raise(client_t *client);

#endif /* STACKING_H */
