/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef FOCUS_H
#define FOCUS_H

#include "config.h"

#include "client.h"
#include "workspace.h"

/*
 * The window which contains the current input focus.  This may become
 * invalid after you call any of the functions in this header; call
 * 'focus_ensure()' to ensure that this is current and that this
 * window is raised.
 */

extern client_t *focus_current;

/*
 * Initialize the focus module
 */

void focus_init();

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
 * Apply a function to all currently mapped clients, starting with the
 * currently focused window.  The void pointer is passed unchanged to
 * all invocations of the function.  If the invoked function returns
 * False, processing immediately stops and focus_forall returns False.
 * If focus_forall applies the function to all mapped windows and the
 * function returns True each time, focus_forall returns True.
 */

typedef Bool (*forall_fn)(client_t *, void *);
Bool focus_forall(forall_fn fn, void *);

/*
 * Ensure some window in the current workspace is focused if possible;
 * the timestamp should be the timestamp of the event that caused the
 * focus change, or CurrentTime if the event doesn't have a timestamp.
 */
void focus_ensure(Time);

void focus_workspace_changed(Time);

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
 * 
 * If the user strikes a key or clicks a button during this function,
 * it will invoke a bound function if something is bound to that
 * key/button; otherwise it will attempt to pass the key/button event
 * to a client window by sending a synthetic event.
 */

void focus_alt_tab(XEvent *e, arglist *ignored);

/*
 * This module needs to know when a client changes its focus policy to
 * or from ClickToFocus (in order to grab the focusing mouse click).
 */

void focus_policy_to_click(client_t *client);
void focus_policy_from_click(client_t *client);

#endif /* FOCUS_H */
