/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"
#include "workspace.h"

/*
 * The windows in a workspace which can possibly receive the input
 * focus are stored in a ring (a circular doubly-linked list).
 */

extern client_t *focus_stacks[NO_WORKSPACES];

/*
 * The window which contains the current input focus.  This may become
 * invalid after you call any of the functions in this header; call
 * 'focus_ensure()' to ensure that this is current and that this
 * window is raised.
 */

extern client_t *focus_current;

/*
 * Add a client to the top of the focus stack for its workspace
 */

void focus_add(client_t *, Time);

/*
 * Remove a client from the list of currently focusable windows for
 * its workspace
 */

void focus_remove(client_t *, Time);

/*
 * Move a client from somewhere in the focus stack to the top of the
 * stack, call XSetInputFocus
 */

void focus_set(client_t *, Time);

/*
 * Ensure some window in the current workspace is focused if possible;
 * the timestamp should be the timestamp of the event that caused the
 * focus change, or CurrentTime if the event doesn't have a timestamp.
 */
void focus_ensure(Time);

/*
 * A function to bind to a key which behaves similarly to the Alt-Tab
 * action in Microsoft Windows.  This behaves like in Windows because
 * the algorithm Windows uses for this is superior to anything else -
 * it shifts the most frequently used applications to be available
 * with fewer keystrokes.
 * 
 * Specifically, the algorithm works as follows:
 * 
 * 1. All windows are maintained on a stack.
 * 
 * 2. Whenever a window is mapped, it is pushed onto the stack.
 * 
 * 3. Whenever you switch to a window, it is removed from its current
 *    position in the stack and is pushed onto the top of the stack.
 * 
 * 4. When you hit Alt-Tab, it does not switch to any windows until
 *    the Alt key is released.
 * 
 * We don't display any icons on Alt-Tab, so we actually raise and
 * focus each window whenever Tab is hit with Alt down, but we don't
 * modify the stack until Alt is released.  The keyboard is grabbed in
 * this function, and no window will actually receive the input focus
 * until user lets go of Alt.
 */

void focus_alt_tab(XEvent *, void *);

#endif /* FOCUS_H */
