/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include "icccm.h"
#include <X11/Xatom.h>
#include <string.h>

/* snarfed from WindowMaker and ctwm */
char *icccm_get_window_name(client_t *client)
{
    XTextProperty xtp;
    char **list;
    char *retval;
    int n;

    if (XGetWMName(dpy, client->window, &xtp) == 0) {
        return strdup("");      /* client did not set a window name */
    }
    if (xtp.value == NULL || xtp.nitems <= 0) {
        /* client set window name to NULL */
        retval = strdup("");
    } else {
        if (xtp.encoding == XA_STRING) {
            /* usual case */
            retval = strdup(xtp.value);
        } else {
            /* client is using UTF-16 or something equally stupid */
            /* haven't seen this block actually run yet */
            xtp.nitems = strlen((char *)xtp.value);
            if (XmbTextPropertyToTextList(dpy, &xtp, &list, &n) == Success
                && n > 0 && *list != NULL) {
                retval = strdup(*list);
                XFreeStringList(list);
            } else {
                retval = strdup("");
            }
        }
    }
    XFree(xtp.value);
        
    printf("\tClient 0x%08X is %s\n", client, retval);
    return retval;
}
