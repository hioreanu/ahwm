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

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xproto.h>         /* for CARD32 */
#include <X11/Xatom.h>

#include "kill.h"
#include "event.h"
#include "debug.h"
#include "timer.h"

/*
 * Sometimes we'll set a timer on a client and unconditionally kill
 * the client when time runs out.  Problem is that client might
 * already be killed by the time the timer triggers and the client
 * pointer is a stale pointer.  Same problem just holding a window
 * XID; the XID may have already been recycled.  Can't figure out any
 * way to use X to uniquely identify a client, so doing some bogus
 * heuristics.  Works fine in practice.
 */

typedef struct _murder_info {
    client_t *client;
    Window window;
    unsigned long hash;
} murder_info;

static char my_hostname[SYS_NMLN];
static Atom _NET_WM_PID;

static void set_murder_timer(client_t *client);
static void murder_on_timeout(timer *t, void *v);
static unsigned long compute_hash(client_t *client);

/*
 * ICCCM is somewhat vague about exactly what this is supposed to
 * mean; understandable, since X runs on non-unix systems which might
 * have no idea of what a hostname is.
 * 
 * I have a bad feeling about using _NET_WM_PID:  suppose I'm running
 * AHWM locally on orpheus.rh.uchicago.edu and I log on to
 * orpheus.cs.uchicago.edu and run netscape from there.  When netscape
 * hangs, AHWM can't distinguish that orpheus.cs is different from
 * orpheus.rh (all it sees is WM_CLIENT_MACHINE = "orpheus") and AHWM
 * will kill some process on orpheus.rh.  Clearly not desirable
 * behaviour.  Should turn off this behaviour by default.
 */

void kill_init()
{
    struct utsname utsn;

    if (uname(&utsn) != 0) {
        perror("AHWM: uname");
        return;
    }
    strncpy(my_hostname, utsn.nodename, SYS_NMLN);
    my_hostname[SYS_NMLN - 1] = '\0';
    _NET_WM_PID = XInternAtom(dpy, "_NET_WM_PID", False);
}

void kill_nicely(XEvent *xevent, arglist *ignored)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "AHWM: Unable to kill client, client not found\n");
        return;
    }
    if (client->protocols & PROTO_DELETE_WINDOW) {
        debug(("\tPolitely requesting window to die\n"));
        client_sendmessage(client, WM_DELETE_WINDOW, event_timestamp,
                           0, 0, 0);
        if (client->patience != 0) {
            set_murder_timer(client);
        }
    } else {
        debug(("\tWindow isn't civilized, exterminating it\n"));
        kill_with_extreme_prejudice(xevent, ignored);
    }
}

Bool kill_using_net_wm_pid(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long bytes_after_return, nitems;
    CARD32 *pid;
    char *their_hostname;

    pid = NULL;
    if (XGetWindowProperty(dpy, client->window, _NET_WM_PID, 0,
                           sizeof(CARD32), False, XA_CARDINAL,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&pid) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_PID) failed\n"));
        return False;
    }
    if (pid == NULL || actual != XA_CARDINAL || fmt != 32 || nitems != 1) {
        
        if (pid != NULL) XFree(pid);
        return False;
    }
    their_hostname = NULL;
    if (XGetWindowProperty(dpy, client->window, XA_WM_CLIENT_MACHINE, 0,
                           sizeof(char *), False, XA_STRING,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&their_hostname) != Success) {
        debug(("\tXGetWindowProperty(_NET_WM_PID) failed\n"));
        return False;
    }
    if (their_hostname == NULL || actual != XA_STRING || fmt != 8) {
        
        XFree(pid);
        if (their_hostname != NULL) XFree(their_hostname);
        return False;
    }
    if (strcmp(my_hostname, their_hostname) == 0) {
        kill((pid_t)*pid, SIGTERM);
        debug(("killed %d (%s) using _NET_WM_PID\n", *pid, client_dbg(client)));
        XFree(pid);
        XFree(their_hostname);
        return True;
    }
    XFree(pid);
    XFree(their_hostname);
    return False;
}

void kill_with_extreme_prejudice(XEvent *xevent, arglist *ignored)
{
    client_t *client;

    client = client_find(xevent->xbutton.window);
    if (client == NULL) {
        fprintf(stderr, "AHWM: Unable to kill client, client not found\n");
        return;
    }
    debug(("\tCommiting windicide\n"));
    if (client->use_net_wm_pid) {
        kill_using_net_wm_pid(client);
    }
    XKillClient(dpy, client->window);
}

static unsigned long compute_hash(client_t *client)
{
    int i, offset;
    unsigned long hash;
    char *s;

    if (client->instance != NULL) {
        s = client->instance;
    } else if (client->class != NULL) {
        s = client->class;
    } else {
        s = client->name;
    }
    hash = offset = 0;
    for (i = 0; i < strlen(s); i++) {
        hash ^= s[i] << offset * 8;
        offset = (offset + 1) % sizeof(unsigned long);
    }
    return hash ^ client->frame;
}

static void set_murder_timer(client_t *client)
{
    murder_info *info;

    info = malloc(sizeof(murder_info));
    if (info == NULL) {
        return;
    }
    info->window = client->window;
    info->client = client;
    info->hash = compute_hash(client);
    timer_new(client->patience, murder_on_timeout, info);
}

static void murder_on_timeout(timer *t, void *v)
{
    client_t *client;
    murder_info *info = (murder_info *)v;

    timer_cancel(t);
    client = info->client;
    if (client != client_find(info->window)) {
        free(info);
        return;
    }
    if (info->hash != compute_hash(client)) {
        free(info);
        return;
    }
    free(info);
    fflush(stdout);
    kill_using_net_wm_pid(client);
    XKillClient(dpy, client->window);
}
