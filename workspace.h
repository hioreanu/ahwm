/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "client.h"

/* 
 * Workspaces are counted starting from one, not zero.  They are
 * implemented by simply unmapping windows not in the current
 * workspace, like most other window managers do it.  Each workspace
 * has its own focus stack.  There is no "virtual root" window, just
 * the actual root created when X starts.  All functions exported here
 * are meant for binding to a key or pointer event.
 */

#define NO_WORKSPACES 7

extern int workspace_current;

extern unsigned long workspace_pixels[NO_WORKSPACES];
extern unsigned long workspace_dark_highlight[NO_WORKSPACES];
extern unsigned long workspace_darkest_highlight[NO_WORKSPACES];
extern unsigned long workspace_highlight[NO_WORKSPACES];

/*
 * move a client to a workspace and make it the top-level window in
 * the new workspace; void * argument is cast to integer
 */
void workspace_client_moveto(XEvent *e, void *workspace);

/*
 * make a workspace the current workspace
 */
void workspace_goto(XEvent *e, void *workspace);

/*
 * update the color on a workspace, allocate colors if needed
 */

void workspace_update_color();

#endif /* WORKSPACE_H */
