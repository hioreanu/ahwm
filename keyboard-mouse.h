/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <X11/Xlib.h>
#include "client.h"

/*
 * Array of booleans specifying whether a given bit is a 'locking'
 * modifier bit - for example, if XK_Num_Lock generates Mod3 (Mod3Mask
 * == 0x20, bit 5 set), then keyboard_is_lock[5] == True.
 */

extern Bool keyboard_is_lock[8];

/*
 * Modifier masks which may be arbitrarily mapped by the user
 */

extern unsigned int MetaMask, SuperMask, HyperMask, AltMask, ModeMask;

/*
 * Mask of all 'locking' modifiers
 */

extern unsigned int AllLocksMask;

/*
 * initialize or reinitialize keyboard module's internals (called at
 * startup and whenever keyboard mapping changed)
 */

void keyboard_init();

/*
 * This will return an array of all the posible combinations of MODS
 * and the various 'lock' modifiers the user has mapped.  The number
 * of elements in the array is put in N and the array should not be
 * freed or otherwise messed with.
 * 
 * For example, if the user has XK_Caps_Lock generating the 'lock'
 * modifier, XK_Num_Lock generating the 'Mod3' modifier and
 * XK_Num_Lock generating 'Mod4', and the argument passed in is
 * 'Mod1Mask', this will return:
 * 
 * Mod1Mask | LockMask
 * Mod1Mask | LockMask | Mod3Mask
 * Mod1Mask | LockMask | Mod3Mask | Mod4Mask
 * Mod1Mask | Mod3Mask
 * Mod1Mask | Mod3Mask | Mod4Mask
 * Mod1Mask | Mod4Mask
 * 
 * Note that the original modifiers (without locking modifiers ORed
 * in) is not returned.  Of course, this takes an exponential amount
 * of time and space (specifically factorial of the number of locking
 * modifiers, which is exponential), so hopefully the user doesn't
 * have six locking modifiers.  To quote Knuth (Art of Computer
 * Programming, 1.2.5):
 * 
 * "It is helpful to keep the value 10! = 3,628,800 in mind; one
 * should remember that 10! is about 3 and half million.  In a sense,
 * this number represents an approximate dividing line between things
 * that are practical to compute and things that are not."
 * 
 * I am considering the following keysyms 'locking' modifiers (based
 * purely upon the fact that they end with '_Lock'):
 * 
 *      XK_Scroll_Lock
 *      XK_Num_Lock
 *      XK_Caps_Lock
 *      XK_Shift_Lock
 *      XK_Kana_Lock
 *      XK_ISO_Lock
 *      XK_ISO_Level3_Lock
 *      XK_ISO_Group_Lock
 *      XK_ISO_Next_Group_Lock
 *      XK_ISO_Prev_Group_Lock
 *      XK_ISO_First_Group_Lock
 *      XK_ISO_Last_Group_Lock
 * 
 * The only reason this mess is exported is because the mousebinding
 * module needs to use it.
 */

unsigned int *keyboard_modifier_combinations(unsigned int mods, int *n);

/* FIXME */
KeySym keyboard_event_to_keysym(XKeyEvent *);

/* Functions which are called in response to keyboard events: */

typedef void (*key_fn)(XEvent *, void *);

/*
 * An example function of the above type which does nothing
 */

void keyboard_ignore(XEvent *, void *);

/*
 * A special function of the above type, put here for lack of a better
 * place.  This will "quote" the next keyboard or mouse event and send
 * it to the client instead of processesing it; this also changes the
 * root background to white to let user know that something is being
 * quoted.  You can cancel a quote by simply clicking on a titlebar or
 * doing any other action which will generate a grab but which the
 * client doesn't care about or can't see.
 */

void keyboard_quote(XEvent *, void *);

/* these go with the above function and are used by the mouse module */
extern Bool quoting;
void keyboard_unquote(XKeyEvent *);

/*
 * Bind a key to a function.  KEYCODE and MODIFIERS are the Keycode
 * and Modifiers to bind; DEPRESS is KEYBOARD_DEPRESS or
 * KEYBOARD_RELEASE (one or the other, not a logical OR), indicating
 * whether to call the function on key press or release; FN is the
 * function to be called, with the above semantics.  If duplicate
 * keybindings are assigned, the most recent is used.
 */

void keyboard_set_function_ex(unsigned int keycode, unsigned int modifiers,
                              int depress, key_fn fn, void *arg);

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

int keyboard_parse_string(char *keystring, unsigned int *keycode,
                          unsigned int *modifiers);

/*
 * Utility function which calls keyboard_parse_string() and then
 * keyboard_set_function_ex()
 */

void keyboard_set_function(char *keystring, int depress,
                           key_fn fn, void *arg);

/*
 * Do a "soft" grab on all the keys that are of interest to us - this
 * should be called once when the window is mapped.
 */

void keyboard_grab_keys(client_t *client);

/* 
 * process a key event
 */
void keyboard_process(XKeyEvent *xevent);

/* FIXME:  there should be a function to unmap a key if we really want
 * this to be dynamic */

#endif /* KEYBOARD_H */
