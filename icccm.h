/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * functions whose behaviour is entirely dictacted by the ICCCM are
 * grouped in this file.
 */

#include "client.h"

/*
 * Handle a property notify event if possible; returns 1 if the
 * event has been handled by this function, 0 if property is
 * not defined by the ICCCM.
 */
int icccm_prop_changed(XPropertyEvent);

/*
 * Returns a newly-malloced string containing the name of the client
 * (to appear in the titlebar); this function always succeeds.
 */

char *icccm_get_window_name(client_t *client);

