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

/* see prefs.c:xmessage()
 * 
 * These are all separate arguments to the xmessage program.  Do not
 * include in more than one C file.
 */

#ifndef DEFAULT_MESSAGE_H
#define DEFAULT_MESSAGE_H

char *default_message[] = {
"xmessage",
"-center",
"-buttons",
"OK",
"Welcome to AHWM!\n",
"\n",
"Since this is the first time you've run AHWM, a default configuration\n",
"file has been created for you.  This is ~/.ahwmrc and you can view the\n",
"documentation for this by invoking \"man ahwmrc\".\n",
"\n",
"The default configuration file has the following bindings:\n",
"\n",
"Control + Alt + Shift + t      Starts \"xterm\"\n",
"Control + Alt + Shift + n      Opens \"netscape\"\n",
"Control + Alt + Shift + e      Opens \"emacs\"\n",
"Left-drag on titlebar          Moves window\n",
"Alt + Left-drag in window      Moves window\n",
"Alt + Right-drag in window     Resizes window\n",
"Right-click on titlebar        Maximizes window\n",
"Middle-click on titlebar       Closes window\n",
"Alt + Tab                      Cycle windows forward\n",
"Alt + Shift + Tab              Cycle windows backward\n",
"Alt + n                        Go to workspace n, where 1 <= n <= 7\n",
"Control + Alt + n              Send window to workspace n\n",
"Control + Alt + Shift + r      Restart AHWM\n",
"\n",
"Workspace 1 is click-to-focus, and the other workspaces are sloppy focus.\n",
"\n",
"The default configuration file sets a number of other esoteric\n",
"bindings and options, so you are encouraged to edit the file to suit\n",
"your preferences.  You can make your changes using a text editor and\n",
"then restart AHWM (via Control + Alt + Shift + r) to have AHWM re-read\n",
"the configuration file.\n",
"\n",
"Extensive documentation is available in the manual page and at:\n",
"http://people.cs.uchicago.edu/~ahiorean/ahwm/doc/\n",
NULL                            /* last element must be NULL */
};
#endif /* DEFAULT_MESSAGE_H */
