/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef EVENT_H
#define EVENT_H

#include <X11/Xlib.h>

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

/*
 * Parse an event structure for a timestamp.  Returns CurrentTime if
 * the event doesn't have a timestamp or we don't know the format of
 * the event.
 */

Time event_timestamp(XEvent *event);

#endif /* EVENT_H */
