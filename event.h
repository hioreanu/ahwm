/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

/*
 * The timestamp of the event currently being processed, or
 * CurrentTime if we can't get a timestamp from the event structure
 */

extern Time event_timestamp;

/*
 * Get an event from the event queue, place it into EVENT.  XFD is a
 * file descriptor which points to the X connection.
 */

void event_get(int xfd, XEvent *event);

/*
 * Deal with an event, all the action starts here.
 */

void event_dispatch(XEvent *event);

/*
 * Parse an event structure and get the "window" member.  Returns None
 * if don't know about the format of the event (all events defined by
 * Xlib have a window member).
 */

Window event_window(XEvent *event);

#endif /* EVENT_H */
