/* $Id$ */
/* Copyright (c) 2001 Alex Hioreanu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WORKSPACE_H
#define WORKSPACE_H

#include "config.h"

#include "client.h"
#include "prefs.h"

/* 
 * Workspaces are counted starting from one, not zero.  They are
 * implemented by simply unmapping windows not in the current
 * workspace, like most other window managers do it.  Each workspace
 * has its own focus stack.  There is no "virtual root" window, just
 * the actual root created when X starts.
 * 
 * Workspace zero is special - this indicates that the window has not
 * yet been mapped into any workspace.
 */

extern unsigned int nworkspaces;

extern unsigned int workspace_current;

/*
 * move a client to a workspace and make it the top-level window in
 * the new workspace
 */

void workspace_client_moveto(client_t *client, unsigned int ws);

/* same as above for binding */
void workspace_client_moveto_bindable(XEvent *e, arglist *args);

/*
 * make a workspace the current workspace
 */

void workspace_goto(unsigned int ws);

/* same as above for binding */
void workspace_goto_bindable(XEvent *e, arglist *args);

#endif /* WORKSPACE_H */
