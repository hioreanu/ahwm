/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include "place.h"

static void place_corner(client_t *client, int x, int y)
{
    client_t *c;

    for (c = client->next_client; c != client; c = client->next_client) {
        if (c->x >= x
            && c->y >= y
            && c->x + c->width <= x
            && c->y + c->height <= y)
            return False;
    }

    c->x = x;
    c->y = y;
    if (x == scr_width)
        c->x -= c->width;
    if (y == scr_height)
        c->y -= c->height;
    return True;
}

void place_least_overlap(client_t *client)
{
    ;
}

void place(client_t *client)
{
    if (!place_corner(client, 0, 0)
        && !place_corner(client, scr_width, 0)
        && !place_corner(client, 0, scr_height)
        && !place_corner(client, scr_width, scr_height)) {
        place_least_overlap(client);
    }
}
