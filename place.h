/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * window placement algorithm
 */

#ifndef PLACE_H
#define PLACE_H

#include "config.h"

#include "client.h"

/*
 * First, we try each of the four corners.  Then we put it where it
 * would have the least amount of overlap with the other windows on
 * screen.
 */

void place(client_t *client);

#endif /* PLACE_H */
