/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef EWMH_H
#define EWMH_H

#include <X11/Xlib.h>
#include "client.h"

extern Atom _NET_CURRENT_DESKTOP;

void ewmh_init();

void ewmh_client_list_add(client_t *client);
void ewmh_client_list_remove(client_t *client);
void ewmh_client_list_stacking_set_top(client_t *client);
void ewmh_client_list_stacking_remove(client_t *client);

void ewmh_current_desktop_update();

void ewmh_active_window_update();

#endif /* EWMH_H */
