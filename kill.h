/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef KILL_H
#define KILL_H

#include "config.h"

#include "client.h"

/*
 * Send a DELETE client message if the client supports it, otherwise
 * use XKillClient().  Meant to be bound to a key or pointer event.
 */

void kill_nicely(XEvent *, void *);

/*
 * XKillClient() wrapper, to be bound to a keystroke or click.
 */

void kill_with_extreme_prejudice(XEvent *, void *);

#endif /* KILL_H */
