/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"

/*
 * The windows in a workspace which can possibly receive the input
 * focus are stored in a ring (a circular doubly-linked list).
 */

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
 * Set the input focus to the next on the ring
 */

void focus_next();

/*
 * Set the input focus to the previous on the ring
 */

void focus_prev();

/*
 * Ensure some window in the current workspace is focused if possible;
 * the timestamp should be the timestamp of the event that caused the
 * focus change, or CurrentTime if the event doesn't have a timestamp.
 */
void focus_ensure(Time);

/*
 * returns one if the client accepts keyboard focus, zero o/w
 */

int focus_canfocus(client_t *);

#endif /* FOCUS_H */
