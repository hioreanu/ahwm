/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#ifndef ICCCM_H
#define ICCCM_H

#include "config.h"

#include <X11/Xlib.h>

/*
 * Set up "WM_Sn" selection for default screen.  This should be called
 * in the initialization phase, after convenience globals are set up
 * but before the root window is selected for events.
 */

void icccm_init();

/*
 * Deal with a selection request event on the "WM_Sn" selection.
 */

void icccm_selection_request(XSelectionRequestEvent *);

#endif /* ICCCM_H */
