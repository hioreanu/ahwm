/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "workspace.h"

int workspace_current = 1;

void workspace_goto(int ws)
{
    workspace_current = ws;
}

void workspace_client_moveto(client_t *client, int ws)
{
    ;
}


