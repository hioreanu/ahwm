/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <X11/Xlib.h>
#include "client.h"

/* Functions which are called in response to keyboard events: */

typedef int (*key_fn)(Window, Window, Time, int, int, int, int);

/*
 * An example function of the above type which does nothing and
 * does not propagate the event to the subwindow.
 * 
 * All of the parameters are from the XKeyEvent structure which
 * came with the event.
 * Returns 1 if the event should be synthesized and propagated
 * to SUBWINDOW (if it exists) or WIN.
 * 
 * This particular function always returns 0, so the event is
 * simply gobbled up and ignored.
 */

int keyboard_ignore(Window win, Window subwindow, Time t,
                    int x, int y, int root_x, int root_y);

/*
 * Bind a key to a function.  KEYCODE and MODS are the Keycode and
 * Modifiers to bind; DEPRESS is KEYBOARD_DEPRESS, KEYBOARD_RELEASE or
 * a logical OR of both, indicating whether to call the function on
 * key press, release or at both times; FN is the function to be
 * called, with the above semantics.  If duplicate keybindings are
 * assigned, the most recent is used.
 */

void keyboard_set_function_ex(int keycode, unsigned int mods,
                              int depress, key_fn fn);

#define KEYBOARD_DEPRESS KeyPress
#define KEYBOARD_RELEASE KeyRelease

/*
 * Function to convert a string which describes a keyboard binding to
 * the representation we have to deal with in these functions.
 * Returns 1 if the string is ok, 0 if it is not in the language
 * described below.  KEYCODE and MODIFIERS are changed upon reading a
 * string in the language.
 * 
 * The grammar for KEYSTRING has tokens which are:
 * 1.  One of the symbols from <X11/keysym.h> with the 'XK_' prefix
 *     removed EXCEPT for the following symbols which are NOT tokens:
 *     Shift_L
 *     Shift_R
 *     Control_L
 *     Control_R
 *     Meta_L
 *     Meta_R
 *     Alt_L
 *     Alt_R
 *     Super_L
 *     Super_R
 *     Hyper_L
 *     Hyper_R
 *     Caps_Lock
 *     Shift_Lock
 *     All of the symbols in this group are case-sensitive.
 * 
 * 2.  One of the following symbols:
 *     Shift, ShiftMask          // standard
 *     Control, ControlMask      // standard
 *     Mod1, Mod1Mask            // standard
 *     Mod2, Mod2Mask            // standard
 *     Mod3, Mod3Mask            // standard
 *     Mod4, Mod4Mask            // standard
 *     Mod5, Mod5Mask            // standard
 *     Alt, AltMask              // nonstandard
 *     Meta, MetaMask            // nonstandard
 *     Hyper, HyperMask          // nonstandard
 *     Super, SuperMask          // nonstandard
 *     All of the symbols in this group are case-insensitive.
 * 
 * The symbols from group (2) above which are marked "nonstandard" are
 * not well-defined; there is a complex relationship between them
 * which I am completely ignoring.  I'm assumming 'Alt' and 'Meta'
 * mean Mod1, Super means Mod3 and Hyper means Mod4 since that's how
 * my keyboard is set up right now.
 * None of my keyboards have numlock or capslock keys, so I'm not
 * going to deal with them (and if these keys annoy you, unmap them
 * with xmodmap, your application should have remappable keybindings
 * or a software function to emulate capslock).
 * You can see what keysyms your modifier keys generate with 'xev'
 * and you can see what modifier bits they correspond to using
 * 'xmodmap -pm'
 * 
 * The grammar (informally) is as follows:
 * 
 * STRING       ::= MODLIST* WHITESPACE KEY
 * WHITESPACE   ::= ('\t' | ' ' | '\v' | '\f' | '\n' | '\r')*
 * MODLIST      ::= MODIFIER WHITESPACE '|' WHITESPACE
 * MODIFIER     ::= <one of the above symbols from group 2>
 * KEY          ::= <one of the above symbols from group 1>
 * 
 * For example:
 * "Meta | Shift | Control | e" is ok
 * " aLT | Tab   " is ok
 * "Alt | TAB " is NOT ok
 * "Shift | a" should behave the same as "A"
 * "Meta | a" is usually (not always) equivalent to "Mod1 | a"
 */

int keyboard_string_to_keycode(char *keystring, int *keycode,
                               unsigned int *modifiers);

/*
 * Utility function which calls keyboard_string_to_keycode() and then
 * keyboard_set_function_ex()
 */

void keyboard_set_function(char *keystring, int depress, key_fn fn);

/*
 * Do a "soft" grab on all the keys that are of interest to us - this
 * should be called once when the window is created (before the client
 * has a chance to call XGrabKeys on the newly-created window).
 */

void keyboard_grab_keys(Window);

/* 
 * process a key event
 */
void keyboard_process(XKeyEvent *xevent);

/* FIXME:  there should be a function to unmap a key if we really want
 * this to be dynamic */

#endif /* KEYBOARD_H */
