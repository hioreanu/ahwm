/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef FOCUS_H
#define FOCUS_H

#include "client.h"

extern client_t *focus_current;

void focus_lost();
void focus_set(client_t *);
int focus_wantsfocus(client_t *);

#endif /* FOCUS_H */
