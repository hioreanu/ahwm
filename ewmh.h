/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * This implements a subset of the hints in the "Extended Window
 * Manager Hints" document, version 1.1 (which is a standard
 * replacement for the previously incompatible GNOME and KDE hints).
 * At this point, this is a standard for QT and KDE applications, and
 * I doubt that any other toolkit will ever implement this stuff as a
 * lot of it is specific to QT.
 *
 * unsupported:
 * 
 * _NET_CLIENT_LIST_STACKING - this is stupid and should not be part
 * of the spec.  Anyone who wants this information can select for
 * stacking changes on the root window.  The spec assumes that the
 * window manager keeps an internal list to mirror the results of
 * XQueryTree, but this is not needed.
 * 
 * UTF-8 - once there is a *standard* library that implements this,
 * and commercial unix vendors ship with this standard library, then
 * maybe.  I am *NOT* linking my window manager to QT just to get UTF8
 * support.  This should not be part of the spec, as X already has
 * other functions for international language support which should be
 * used instead (and they probably won't become part of the spec since
 * QT most likely won't implement them).
 * 
 * _NET_DESKTOP_NAMES - We wouldn't do anything with this at this
 * time, and this is optional anyway.
 * 
 * anything having to do with icons - not needed, we don't do anything
 * with icons except allowing applications to iconify/deiconify
 * themselves.  Most of this stuff is optional.
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

#include <X11/Xlib.h>
#include "client.h"

/*
 * initialize module, getting atoms and setting root properties
 */

void ewmh_init();

/*
 * Try to handle a ClientMessage event.  Returns True if the event is
 * known and has been handled, else returns False.
 */

Bool ewmh_handle_clientmessage(XClientMessageEvent *xevent);

void ewmh_client_list_add(client_t *client);
void ewmh_client_list_remove(client_t *client);
void ewmh_client_list_stacking_set_top(client_t *client); /* FIXME */
void ewmh_client_list_stacking_remove(client_t *client); /* FIXME */

/* update _NET_CURRENT_DESKTOP according to 'desktop_current' */
void ewmh_current_desktop_update();

/* update _NET_ACTIVE_WINDOW according to 'focus_current' */
void ewmh_active_window_update();

#endif /* EWMH_H */
