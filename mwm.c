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

#include "ahwm.h"
#include "mwm.h"
#include "MwmUtil.h"
#include "client.h"
#include "debug.h"

Atom _MOTIF_WM_HINTS;

void mwm_init()
{
    _MOTIF_WM_HINTS = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
}

void mwm_apply(client_t *client)
{
    Atom actual;
    int fmt;
    unsigned long bytes_after_return, nitems;
    mwm_hints *hints;

    hints = NULL;
    if (XGetWindowProperty(dpy, client->window, _MOTIF_WM_HINTS, 0,
                           sizeof(mwm_hints), False, _MOTIF_WM_HINTS,
                           &actual, &fmt, &nitems, &bytes_after_return,
                           (unsigned char **)&hints) != Success) {
        debug(("XGetWindowProperty(_MOTIF_WM_HINTS) FAILED!\n"));
        return;
    }

    if (actual != _MOTIF_WM_HINTS || fmt != 32) {
        debug(("Actual = %d, _MOTIF_WM_HINTS = %d, fmt = %d\n",
               actual, _MOTIF_WM_HINTS, fmt));
        if (hints != NULL) XFree(hints);
        return;
    }
    
    debug(("%s has set: %s%s%s%s\n",
           client->name,
           hints->flags & MWM_FLAGS_FUNCTIONS ? "functions " : "",
           hints->flags & MWM_FLAGS_DECORATIONS ? "decorations " : "",
           hints->flags & MWM_FLAGS_INPUT_MODE ? "input_mode " : "",
           hints->flags & MWM_FLAGS_STATUS ? "status " : ""));

    /* AHWM does not have a window menu, so we ignore the "functions"
     * member.  The only decoration we have is a titlebar, so that's
     * the only 'decorations' member we don't ignore */

    if ((hints->flags & MWM_FLAGS_DECORATIONS) &&
        (!(hints->decorations & MWM_DECORATIONS_TITLEBAR))) {
        
        debug(("Removing titlebar of %s because of Motif hints\n",
               client->name));
        if (client->has_titlebar_set <= HintSet) {
            client_remove_titlebar(client);
            client->has_titlebar_set = HintSet;
        }
    }
    
    /* FIXME:  could also interpret disabling the "move" menu
     * as wanting to be immutable */
    
    XFree(hints);
}
