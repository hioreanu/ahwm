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

extern Window stacking_desktop_frame;

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
 * Returns the client immediately on top of or below given client
 */

client_t *stacking_prev(client_t *client);
client_t *stacking_next(client_t *client);

/*
 * This will:
 * 
 * 1. raise the client to the top of the stacking list
 * 
 * 2. map the client if it is in the current workspace
 * 
 * 3. perform (1) and (2) for each client in the transience
 *    tree for which client->keep_transients_on_top is set
 */

void stacking_raise(client_t *client);

/*
 * This will ensure the client is in the correct stacking order, but
 * will not raise the client among its peers unless needed.  Use this
 * after client->always_on_top or client->always_on_bottom changes.
 */

void stacking_restack(client_t *client);

#endif /* STACKING_H */
