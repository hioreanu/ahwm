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

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include "cursor.h"
#include "xwm.h"

Cursor cursor_normal = None;
Cursor cursor_moving = None;
Cursor cursor_sizing = None;

Cursor cursor_direction_map[9];

void cursor_init()
{
    cursor_normal = XCreateFontCursor(dpy, XC_left_ptr);
    cursor_moving = XCreateFontCursor(dpy, XC_fleur);
    cursor_sizing = XCreateFontCursor(dpy, XC_sizing);

    /* these aren't too pretty, but the ones that I drew to replace
     * these really, really suck */
    cursor_direction_map[0] = XCreateFontCursor(dpy, XC_top_left_corner);
    cursor_direction_map[1] = XCreateFontCursor(dpy, XC_top_right_corner);
    cursor_direction_map[2] = XCreateFontCursor(dpy, XC_bottom_right_corner);
    cursor_direction_map[3] = XCreateFontCursor(dpy, XC_bottom_left_corner);
    cursor_direction_map[4] = XCreateFontCursor(dpy, XC_top_side);
    cursor_direction_map[5] = XCreateFontCursor(dpy, XC_bottom_side);
    cursor_direction_map[6] = XCreateFontCursor(dpy, XC_right_side);
    cursor_direction_map[7] = XCreateFontCursor(dpy, XC_left_side);
    cursor_direction_map[8] = cursor_normal;

    /* I would prefer not changing the root cursor at all (user can
     * already do that with 'xsetroot'), but it's extremely
     * distracting to have the cursor change when it simply hovers
     * over something, and we need to define a cursor for some of our
     * windows.  X does not provide any mechanism for getting a
     * window's cursor, so we define the root cursor for
     * consistency. */
    XDefineCursor(dpy, root_window, cursor_normal);
}
