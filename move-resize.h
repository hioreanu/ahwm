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

#ifndef MOVE_RESIZE_H
#define MOVE_RESIZE_H

#include "config.h"

#include <X11/Xlib.h>

/* one if moving/resizing, zero otherwise */
extern int moving, sizing;

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
