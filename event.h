/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef EVENT_H
#define EVENT_H

void event_get(int xfd, XEvent *event);
void event_dispatch(XEvent *event);

#endif /* EVENT_H */
