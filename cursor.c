/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * OK, so there are two things that really piss me off about X:
 * 
 * 1. the whole problem with "Meta" != "Mod1", etc.
 * 2. the standard cursors
 * 
 * So we get cursors to display the Starship Enterprise but I can't
 * find a cursor suitable for representing a NorthEast-SouthWest
 * resize action.  Furthermore, the cursors that ship with all the X
 * distros I've seen are *ugly*.  I mean, I was ready to switch from
 * Netscape to something else just when I saw that god-awful hand
 * cursor.  The NorthEast-SouthWest cursor is the only one I really
 * needed, but I had to make others to match it once I drew it.  This
 * was actually quite difficult, I suck at these drawing things.  Use
 * the standard X program 'bitmap' to change them.
 */

#include <X11/cursorfont.h>

#include "cursor.h"
#include "xwm.h"

#include "ew_cursor_black.xbm"
#include "ne_cursor_black.xbm"
#include "ns_cursor_black.xbm"
#include "nw_cursor_black.xbm"
#include "ew_cursor_white.xbm"
#include "ne_cursor_white.xbm"
#include "ns_cursor_white.xbm"
#include "nw_cursor_white.xbm"

Cursor cursor_normal       = None;
Cursor cursor_moving       = None;
Cursor cursor_sizing_nw_se = None;
Cursor cursor_sizing_ne_sw = None;
Cursor cursor_sizing_n_s   = None;
Cursor cursor_sizing_e_w   = None;

int cursor_init()
{
    Pixmap black, white;
    XColor fg, bg;

    cursor_normal = XCreateFontCursor(dpy, XC_left_ptr);
    cursor_moving = XCreateFontCursor(dpy, XC_fleur);

    fg.pixel = BlackPixel(dpy, scr);
    bg.pixel = WhitePixel(dpy, scr);
    XQueryColor(dpy, DefaultColormap(dpy, scr), &fg);
    XQueryColor(dpy, DefaultColormap(dpy, scr), &bg);

    /* cut-and-paste start */
    black = XCreateBitmapFromData(dpy, root_window, ew_cursor_black_bits,
                                   ew_cursor_black_width, ew_cursor_black_height);
    white = XCreateBitmapFromData(dpy, root_window, ew_cursor_white_bits,
                                   ew_cursor_white_width, ew_cursor_white_height);
    if (black == None || white == None)
        return 0;
    cursor_sizing_e_w = XCreatePixmapCursor(dpy, black, white, &fg, &bg,
                                            ew_cursor_black_x_hot,
                                            ew_cursor_black_y_hot);
    XFreePixmap(dpy, black);
    XFreePixmap(dpy, white);
    /* cut-and-paste end */
    
    black = XCreateBitmapFromData(dpy, root_window, ne_cursor_black_bits,
                                   ne_cursor_black_width, ne_cursor_black_height);
    white = XCreateBitmapFromData(dpy, root_window, ne_cursor_white_bits,
                                   ne_cursor_white_width, ne_cursor_white_height);
    if (black == None || white == None)
        return 0;
    cursor_sizing_ne_sw = XCreatePixmapCursor(dpy, black, white, &fg, &bg,
                                              ne_cursor_black_x_hot,
                                              ne_cursor_black_y_hot);
    XFreePixmap(dpy, black);
    XFreePixmap(dpy, white);
    
    black = XCreateBitmapFromData(dpy, root_window, ns_cursor_black_bits,
                                   ns_cursor_black_width, ns_cursor_black_height);
    white = XCreateBitmapFromData(dpy, root_window, ns_cursor_white_bits,
                                   ns_cursor_white_width, ns_cursor_white_height);
    if (black == None || white == None)
        return 0;
    cursor_sizing_n_s = XCreatePixmapCursor(dpy, black, white, &fg, &bg,
                                            ns_cursor_black_x_hot,
                                            ns_cursor_black_y_hot);
    XFreePixmap(dpy, black);
    XFreePixmap(dpy, white);
    
    black = XCreateBitmapFromData(dpy, root_window, nw_cursor_black_bits,
                                   nw_cursor_black_width, nw_cursor_black_height);
    white = XCreateBitmapFromData(dpy, root_window, nw_cursor_white_bits,
                                   nw_cursor_white_width, nw_cursor_white_height);
    if (black == None || white == None)
        return 0;
    cursor_sizing_nw_se = XCreatePixmapCursor(dpy, black, white, &fg, &bg,
                                              nw_cursor_black_x_hot,
                                              nw_cursor_black_y_hot);
    XFreePixmap(dpy, black);
    XFreePixmap(dpy, white);

    if (cursor_normal == None)       return 0;
    if (cursor_moving == None)       return 0;
    if (cursor_sizing_nw_se == None) return 0;
    if (cursor_sizing_ne_sw == None) return 0;
    if (cursor_sizing_n_s == None)   return 0;
    if (cursor_sizing_e_w == None)   return 0;
    return 1;
}
