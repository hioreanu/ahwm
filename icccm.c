/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "icccm.h"
#include <X11/Xatom.h>

char *icccm_get_window_name(client_t *client)
{
    XTextProperty xtp;

    if (XGetWMName(dpy, client->window, &xtp) == 0) return 0;
    printf("\tClient 0x%08X is %s\n", client, xtp.value);
    return xtp.value;
}

