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

/*
 * This implements a subset of the hints in the "Extended Window
 * Manager Hints" document, version 1.1 (which is a standard
 * replacement for the previously incompatible GNOME and KDE hints).
 * 
 * This also implements the older GNOME hints, since they are very
 * similar to the new EWMH.  The older GNOME hints only have a
 * somewhat unfinished standards document, but that document is
 * extremely clear and precise.
 *
 * TODO:
 * root messages:
 * _NET_WM_MOVERESIZE
 * 
 * client properties:
 * _NET_WM_DESKTOP,
 * _NET_WM_WINDOW_TYPE,
 * _NET_WM_STATE,
 * _NET_WM_STRUT
 * 
 * the "ping" protocol
 */

#ifndef EWMH_H
#define EWMH_H

#include "config.h"

#include <X11/Xlib.h>

#include "client.h"

/* These are needed by event.c: */
extern Atom _NET_WM_WINDOW_TYPE, _NET_WM_STATE, _NET_WM_STRUT, _NET_WM_DESKTOP;

/*
 * initialize module, getting atoms and setting root properties
 * Depends on number of workspaces being set
 */

void ewmh_init();

/*
 * Try to handle a ClientMessage event.  Returns True if the event is
 * known and has been handled, else returns False.
 */

Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent);

void ewmh_client_list_add(client_t *client);
void ewmh_client_list_remove(client_t *client);
void ewmh_stacking_list_update(Window *w, int nwindows);

/* update _NET_CURRENT_DESKTOP according to 'desktop_current' */
void ewmh_current_desktop_update();

/* update _NET_ACTIVE_WINDOW according to 'focus_current' */
void ewmh_active_window_update();

#endif /* EWMH_H */
