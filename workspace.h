/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "client.h"

extern int workspace_current;

void workspace_move_client(client_t *, int workspace);
void workspace_goto(int workspace);

#endif /* WORKSPACE_H */
