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

/*
 * Please note:  this file was written by Alex Hioreanu and is
 * Copyright (C) Alex Hioreanu 2001.  It was not copied from a
 * commercial Motif distribution, OpenMotif or LessTif.  All of these
 * have licenses which are not compatible with AHWM.  OpenMotif has a
 * particularly nasty license, and I want nothing to do with that.  I
 * put this together from stuff gleaned from mailings lists and google
 * searches.  IANAL, but as I understand it, you can't copyright
 * numbers or data structures.
 * 
 * I've never used a Motif application that uses Motif hints, so I'm
 * by no means an expert on this stuff.  This seems to work with all
 * of the (non-motif) motif-hint-using applications I've seen, and
 * that's all I care about.  This is particularly simple stuff (albeit
 * undocumented), so I don't see where I can screw this up.
 * 
 * This isn't integrated into mwm.c to make it easier for you to snarf
 * this code for your own window manager.
 */

#ifndef MWMUTIL_H
#define MWMUTIL_H

/*
 * "_MOTIF_WM_HINTS" the X atom/property used for Motif hints.  Access
 * it something like this:
 * 
 * Atom _motif_wm_hints, actual;
 * int fmt;
 * unsigned long nitems, bytes_after_return;
 * mwm_hints *hints;
 * 
 * _motif_wm_hints = XInternAtom("_MOTIF_WM_HINTS");
 * if (XGetWindowProperty(dpy, window, _motif_wm_hints, 0, sizeof(mwm_hints),
 *                        False, _motif_wm_hints, &actual, &fmt,
 *                        &nitems, &bytes_after_return,
 *                        (unsigned char **)&hints) != Sucess) {
 *      ...error...
 * }
 * if (actual != _motif_wm_hints || actual_fmt != 32) {
 *     ...error...
 * }
 * ...do something with "hints"...
 * XFree(hints);
 */

/*
 * All of the structure members are bit fields.  The bits for each
 * member are defined below.
 * 
 * FLAGS defines which of the other members are set.
 * 
 * FUNCTIONS defines which menu functions are enabled.  As I
 * understand it, MWM had a window menu, similar to the "system menu"
 * in other window managers.  This field determines which menu items
 * should be enabled.
 * 
 * DECORATIONS defines which window manager decorations should be
 * enabled.
 * 
 * In addition, the structure should contain two other members:
 * 
 * INPUT_MODE, the fourth member, seems to have been an attempt at
 * doing modal dialog boxes.  I suppose one could use it in
 * conjunction with TRANSIENT_FOR to do modal dialog boxes.  The
 * values for this member are (this member is not a bit field):
 * 0: modeless
 * 1: primary application modal
 * 2: system modal
 * 3: full application modal
 * I have no idea what the difference between (1) and (3) would be.
 * 
 * STATUS, the fifth member, seems to be used for those "tear-off"
 * menus in some way.  Perhaps it is a way to recognize those windows.
 * Only the first bit seems defined to do anything.  Further
 * investigation required.
 */

typedef struct _mwm_hints {
    int flags;
    int functions;
    int decorations;
    int ignored1;
    int ignored2;
} mwm_hints;

/* mwm_hints->flags is a logical OR of these: */
#define MWM_FLAGS_FUNCTIONS 1
#define MWM_FLAGS_DECORATIONS (1 << 1)
#define MWM_FLAGS_INPUT_MODE (1 << 2)
#define MWM_FLAGS_STATUS (1 << 3)

/* mwm_hints->functions is a logical OR of these: */
#define MWM_FUNCTIONS_ALL 1
#define MWM_FUNCTIONS_RESIZE (1 << 1)
#define MWM_FUNCTIONS_MOVE (1 << 2)
#define MWM_FUNCTIONS_MINIMIZE (1 << 3)
#define MWM_FUNCTIONS_MAXIMIZE (1 << 4)
#define MWM_FUNCTIONS_CLOSE (1 << 5)

/* mwm_hints->decorations is a logical OR of these: */
#define MWM_DECORATIONS_ALL 1
#define MWM_DECORATIONS_BORDER (1 << 1)
#define MWM_DECORATIONS_RESIZE_HANDLE (1 << 2)
#define MWM_DECORATIONS_TITLEBAR (1 << 3)
#define MWM_DECORATIONS_MENU (1 << 4)
#define MWM_DECORATIONS_MAXIMIZE (1 << 5)
#define MWM_DECORATIONS_MINIMIZE (1 << 6)

#endif /* MWMUTIL_H */
