/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"

/*
 * The window which contains the current input focus.  This may become
 * invalid after you call any of the functions in this header; call
 * 'focus_ensure()' to ensure that this is current and that this
 * window is mapped.
 */

extern client_t *focus_current;

/*
 * Add a client to the top of the focus stack for its workspace
 */

void focus_add(client_t *);

/*
 * Remove a client from the list of currently focusable
 * windows for its workspace
 */

void focus_remove(client_t *);

/*
 * Move a client from somewhere in the focus stack to the top of the stack
 */

void focus_set(client_t *);

/*
 * Ensure some window in the current workspace is mapped if possible
 */
void focus_ensure();

/*
 * returns 1 if the client accepts keyboard focus, 0 o/w
 */

int focus_canfocus(client_t *);

#endif /* FOCUS_H */
