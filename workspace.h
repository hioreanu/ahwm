/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/* Workspaces are counted starting from one, not zero.  They are
 * implemented by simply unmapping windows not in the current
 * workspace, like most other window managers do it.  Each workspace
 * has its own focus stack.
 */

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "client.h"

#define NO_WORKSPACES 7

extern int workspace_current;

/*
 * move a client to a workspace and make it the top-level window in
 * the new workspace
 */
void workspace_move_client(client_t *, int workspace);

/*
 * make a workspace the current workspace
 */
void workspace_goto(int workspace);

#endif /* WORKSPACE_H */
