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
 * Bindings are stored in simple linked lists; parsing is done by
 * hand, no lex; possible combinations of modifiers to ignore is
 * computed in keyboard_init, iterated through when binding
 * keys/buttons.
 */

#include "config.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "keyboard-mouse.h"
#include "client.h"
#include "malloc.h"
#include "workspace.h"
#include "event.h"
#include "focus.h"
#include "cursor.h"
#include "debug.h"
#include "stacking.h"
#include "ewmh.h"
#include "compat.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#endif

typedef struct _boundkey {
    unsigned int keycode;
    unsigned int modifiers;
    int depress;
    key_fn function;
    arglist *args;
    struct _boundkey *next;
} boundkey;

typedef struct _boundbutton {
    unsigned int button;
    unsigned int modifiers;
    click_type type;
    int location;
    mouse_fn function;
    arglist *args;
    struct _boundbutton *next;
} boundbutton;

unsigned int MetaMask, SuperMask, HyperMask, AltMask, ModeMask;
unsigned int AllLocksMask;
mouse_fn mouse_ignore = keyboard_ignore;
mouse_fn mouse_quote = keyboard_quote;

/*
 * Array of booleans specifying whether a given bit is a 'locking'
 * modifier bit - for example, if XK_Num_Lock generates Mod3 (Mod3Mask
 * == 0x20, bit 5 set), then keyboard_is_lock[5] == True.
 */
static Bool keyboard_is_lock[8] = { False };

/* the mouse and keyboard bindings */
static boundkey *boundkeys = NULL;
static boundbutton *boundbuttons = NULL;

/* combinations of modifier keys to ignore */
static unsigned int *modifier_combinations = NULL;
static int n_modifier_combinations;

/* used for quoting key & mouse bindings */
static Bool quoting = False;
static Time last_quote_time;

/* used for changing desktop to white when quoting */
static Window white_window = None;

static void modifier_combinations_helper(unsigned int state, int *n, int bit);
static int figure_button(char *, unsigned int *);
static int figure_keycode(char *, unsigned int *);
static int get_location(XButtonEvent *e);
static void warn(KeySym new, char *old, char *mod);
static void figure_lock(KeySym keysym, int bit);
static void unquote(XEvent *e);
static void get_event_child_windows_mouse(Window *event, Window *child,
                                          unsigned int mask);
static void get_event_child_windows_keyboard(Window *event, Window *child,
                                             unsigned int mask);
static Bool in_window(XEvent *xevent, Window w);

static int parse_string(char *keystring, unsigned int *button_ret,
                        unsigned int *modifiers_ret,
                        int (*fn)(char *, unsigned int *));

void keyboard_init()
{
    XModifierKeymap *xmkm;
    int i, j, k;
    int meta_bit, super_bit, hyper_bit, alt_bit, mode_bit;
    char *meta_key, *super_key, *hyper_key, *alt_key, *mode_key;
    KeyCode keycode;
    KeySym keysym;

    meta_bit = super_bit = hyper_bit = alt_bit = mode_bit = -1;
    xmkm = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < xmkm->max_keypermod; j++) {
            keycode = xmkm->modifiermap[i * xmkm->max_keypermod + j];
            if (keycode != 0) {
                for (k = 0; k < 4; k++) {
                    keysym = XKeycodeToKeysym(dpy, keycode, k);
                    figure_lock(keysym, i);
                    switch (keysym) {
                        case XK_Meta_L:
                        case XK_Meta_R:
                            if (meta_bit != -1 && meta_bit != i)
                                warn(keysym, meta_key, "Meta");
                            meta_bit = i;
                            meta_key = XKeysymToString(keysym);
                            break;
                            
                        case XK_Alt_L:
                        case XK_Alt_R:
                            if (alt_bit != -1 && alt_bit != i)
                                warn(keysym, alt_key, "Alt");
                            alt_bit = i;
                            alt_key = XKeysymToString(keysym);
                            break;

                        case XK_Super_L:
                        case XK_Super_R:
                            if (super_bit != -1 && super_bit != i)
                                warn(keysym, super_key, "Super");
                            super_bit = i;
                            super_key = XKeysymToString(keysym);
                            break;

                        case XK_Hyper_L:
                        case XK_Hyper_R:
                            if (hyper_bit != -1 && hyper_bit != i)
                                warn(keysym, hyper_key, "Hyper");
                            hyper_bit = i;
                            hyper_key = XKeysymToString(keysym);
                            break;
                            
                        case XK_Mode_switch:
                            if (mode_bit != -1 && mode_bit != i)
                                warn(keysym, mode_key, "Mode Switch");
                            mode_bit = i;
                            mode_key = XKeysymToString(keysym);
                            break;
                    }
                }
            }
        }
    }
    
    XFreeModifiermap(xmkm);
                
    MetaMask  = 0;
    SuperMask = 0;
    HyperMask = 0;
    AltMask   = 0;
    ModeMask  = 0;
    if (meta_bit  != -1) MetaMask  = (1 << meta_bit);
    if (super_bit != -1) SuperMask = (1 << super_bit);
    if (hyper_bit != -1) HyperMask = (1 << hyper_bit);
    if (alt_bit   != -1) AltMask   = (1 << alt_bit);
    if (mode_bit  != -1) ModeMask  = (1 << mode_bit);

    AllLocksMask = 0;
    j = 0;
    for (i = 0; i < 8; i++) {
        if (keyboard_is_lock[i]) {
            AllLocksMask |= (1 << i);
            j *= j + 1;
            if (j == 0) j = 1;
        }
    }
    if (modifier_combinations != NULL) Free(modifier_combinations);
    modifier_combinations = NULL;
    n_modifier_combinations = j;
    if (j == 0) return;
            
    modifier_combinations = Malloc(sizeof(unsigned int) * j);
    if (modifier_combinations == NULL) {
        n_modifier_combinations = 0;
        return;
    }
    
    j = 0;
    for (i = 0; i < 8; i++) {
        modifier_combinations_helper(0, &j, i);
    }
}

/* FIXME: calculate in keyboard_init(), update on modifier change */
int keyboard_get_modifier_mask(int keycode)
{
    XModifierKeymap *xmkm;
    int i, j, mods;

    mods = 0;
    xmkm = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < xmkm->max_keypermod; j++) {
            if (keycode == xmkm->modifiermap[i * xmkm->max_keypermod + j]) {
                mods |= (1 << i);
            }
        }
    }
    XFreeModifiermap(xmkm);
    return mods;
}

void keyboard_ignore(XEvent *e, arglist *ignored)
{
    return;
}

void keyboard_bind_ex(unsigned int keycode, unsigned int modifiers,
                      int depress, key_fn fn, arglist *arg)
{
    boundkey *newbinding;

    newbinding = Malloc(sizeof(boundkey));
    if (newbinding == NULL) {
        fprintf(stderr, "AHWM: Cannot bind key, out of memory\n");
        return;
    }
    newbinding->next = boundkeys;
    newbinding->keycode = keycode;
    newbinding->modifiers = modifiers;
    newbinding->depress = depress;
    newbinding->function = fn;
    newbinding->args = arg;
    boundkeys = newbinding;
}

void mouse_bind_ex(unsigned int button, unsigned int modifiers,
                   click_type type, int location, mouse_fn fn,
                   struct _arglist *arg)
{
    boundbutton *newbinding;

    newbinding = Malloc(sizeof(boundbutton));
    if (newbinding == NULL) {
        fprintf(stderr, "AHWM: Cannot bind mouse button, out of memory\n");
        return;
    }
    newbinding->button = button;
    newbinding->modifiers = modifiers;
    newbinding->type = type;
    newbinding->location = location;
    newbinding->function = fn;
    newbinding->args = arg;
    newbinding->next = boundbuttons;
    boundbuttons = newbinding;
}

void keyboard_unbind_ex(unsigned int keycode, unsigned int modifiers,
                        int depress) 
{
    boundkey *kb, *tmp;

    tmp = NULL;
    for (kb = boundkeys; kb != NULL; kb = kb->next) {
        if (kb->keycode == keycode
            && kb->modifiers == modifiers
            && kb->depress == depress) {
            if (tmp == NULL) {
                boundkeys = boundkeys->next;
                free(kb);
            } else {
                tmp->next = kb->next;
                free(kb);
            }
        }
        tmp = kb;
    }
}

void mouse_unbind_ex(unsigned int button, unsigned int modifiers,
                     click_type type, int location)
{
    boundbutton *mb, *tmp;

    tmp = NULL;
    for (mb = boundbuttons; mb != NULL; mb = mb->next) {
        if (mb->button == button
            && mb->modifiers == modifiers
            && mb->type == type
            && mb->location == location) {
            if (tmp == NULL) {
                boundbuttons = boundbuttons->next;
                free(mb);
            } else {
                tmp->next = mb->next;
                free(mb);
            }
        }
        tmp = mb;
    }
}

/* FIXME: should also apply the bindings to all active clients */

void keyboard_bind(char *keystring, int depress,
                   key_fn fn, struct _arglist *arg)
{
    unsigned int keycode;
    unsigned int modifiers;

    if (parse_string(keystring, &keycode, &modifiers, figure_keycode) != 1) {
        fprintf(stderr, "AHWM: Cannot bind key, bad keystring '%s'\n", keystring);
        return;
    }
    keyboard_bind_ex(keycode, modifiers, depress, fn, arg);
}

void mouse_bind(char *mousestring, click_type type,
                int location, mouse_fn fn, struct _arglist *arg)
{
    unsigned int button;
    unsigned int modifiers;

    if (parse_string(mousestring, &button, &modifiers, figure_button) != 1) {
        fprintf(stderr, "AHWM: Cannot bind mouse button, bad string '%s'\n",
                mousestring);
        return;
    }
    mouse_bind_ex(button, modifiers, type, location, fn, arg);
}

void keyboard_unbind(char *keystring, int depress)
{
    unsigned int keycode;
    unsigned int modifiers;
    
    if (parse_string(keystring, &keycode, &modifiers, figure_keycode) != 1) {
        fprintf(stderr,
                "AHWM: Cannot unbind key, bad keystring '%s'\n", keystring);
        return;
    }
    keyboard_unbind_ex(keycode, modifiers, depress);
}

void mouse_unbind(char *mousestring, click_type type, int location)
{
    unsigned int button;
    unsigned int modifiers;

    if (parse_string(mousestring, &button, &modifiers, figure_button) != 1) {
        fprintf(stderr, "AHWM: Cannot unbind mouse button, bad string '%s'\n",
                mousestring);
        return;
    }
    mouse_unbind_ex(button, modifiers, type, location);
}

void keyboard_grab_keys(Window w)
{
    boundkey *kb;
    int i;

    for (kb = boundkeys; kb != NULL; kb = kb->next) {
        XGrabKey(dpy, kb->keycode, kb->modifiers, w, True,
                 GrabModeAsync, GrabModeAsync);
        for (i = 0; i < n_modifier_combinations; i++) {
            XGrabKey(dpy, kb->keycode,
                     modifier_combinations[i] | kb->modifiers,
                     w, True, GrabModeAsync, GrabModeAsync);
        }
    }
}

void keyboard_ungrab_keys(Window w)
{
    boundkey *kb;
    int i;

    for (kb = boundkeys; kb != NULL; kb = kb->next) {
        XUngrabKey(dpy, kb->keycode, kb->modifiers, w);
        for (i = 0; i < n_modifier_combinations; i++) {
            XUngrabKey(dpy, kb->keycode,
                     modifier_combinations[i] | kb->modifiers,
                     w);
        }
    }
}

void mouse_grab_buttons(client_t *client)
{
    boundbutton *mb;
    unsigned int mask;
    int i;

    if (client->dont_bind_mouse == 1) return;
    
    for (mb = boundbuttons; mb != NULL; mb = mb->next) {
        mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
        if (mb->location & MOUSE_FRAME) {
            XGrabButton(dpy, mb->button, mb->modifiers, client->frame,
                        True, mask, GrabModeSync, GrabModeAsync,
                        None, cursor_normal);
            for (i = 0; i < n_modifier_combinations; i++) {
                XGrabButton(dpy, mb->button,
                            modifier_combinations[i] | mb->modifiers,
                            client->frame, True, mask, GrabModeSync,
                            GrabModeAsync, None, cursor_normal);
            }
        }
        if (mb->location & MOUSE_TITLEBAR && client->titlebar != None) {
            XGrabButton(dpy, mb->button, mb->modifiers, client->titlebar,
                        True, mask, GrabModeSync, GrabModeAsync,
                        None, cursor_normal);
            for (i = 0; i < n_modifier_combinations; i++) {
                XGrabButton(dpy, mb->button,
                            modifier_combinations[i] | mb->modifiers,
                            client->titlebar, True, mask, GrabModeSync,
                            GrabModeAsync, None, cursor_normal);
            }
        }
    }
}

void mouse_ungrab_buttons(client_t *client)
{
    boundbutton *mb;
    unsigned int mask;
    int i;
    
    for (mb = boundbuttons; mb != NULL; mb = mb->next) {
        mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
        if (mb->location & MOUSE_FRAME) {
            XUngrabButton(dpy, mb->button, mb->modifiers, client->frame);
            for (i = 0; i < n_modifier_combinations; i++) {
                XUngrabButton(dpy, mb->button,
                            modifier_combinations[i] | mb->modifiers,
                            client->frame);
            }
        }
        if (mb->location & MOUSE_TITLEBAR && client->titlebar != None) {
            XUngrabButton(dpy, mb->button, mb->modifiers, client->titlebar);
            for (i = 0; i < n_modifier_combinations; i++) {
                XUngrabButton(dpy, mb->button,
                              modifier_combinations[i] | mb->modifiers,
                              client->titlebar);
            }
        }
    }
}

void keyboard_quote(XEvent *e, struct _arglist *ignored)
{
    XSetWindowAttributes xswa;
    Window windows[2];

    debug(("\tQuoting\n"));

    if (quoting) {
        /* Don't get here very often, KeyRelease v. KeyPress issue */
        unquote(e);
        return;
    }
    
    xswa.override_redirect = True;
    xswa.background_pixel = white;
    white_window = XCreateWindow(dpy, root_window, 0, 0, scr_width, scr_height,
                                 0, 0, InputOutput, DefaultVisual(dpy, scr),
                                 CWBackPixel | CWOverrideRedirect, &xswa);
    XLowerWindow(dpy, white_window);
    if (stacking_desktop_window != None) {
        windows[0] = white_window;
        windows[1] = stacking_desktop_window;
        XRestackWindows(dpy, windows, 2);
    }
    XMapWindow(dpy, white_window);
    
    quoting = True;
    last_quote_time = e->xkey.time;
}

void keyboard_replay(XKeyEvent *e)
{
    Window event, child, junk;
    int x, y;
    unsigned int mask;

    if (e->type == KeyPress) {
        mask = KeyPressMask;
    } else if (e->type == KeyRelease) {
        mask = KeyReleaseMask;
    } else {
        debug(("\tNot replaying unknown event type %d\n", e->type));
        return;
    }

    get_event_child_windows_keyboard(&event, &child, mask);
    if (event == None) return;
    if (child != event) e->subwindow = child;
    else e->subwindow = None;
    e->time = event_timestamp;
    if (e->time == CurrentTime) {
        e->time = last_quote_time;
    }
    if (XTranslateCoordinates(dpy, e->window, event,
                              e->x, e->y, &x, &y, &junk) != 0) {
        e->x = x;
        e->y = y;
    }
    e->window = event;
    debug(("\tSending keyboard event\n"));
    XSendEvent(dpy, event, True, mask, (XEvent *)e);
}

/* I'm not entirely sure that this works exactly correctly, but I
 * haven't yet seen any application which doesn't accept the synthetic
 * click. */
void mouse_replay(XButtonEvent *e)
{
    Window child, event, junk;
    int x, y;
    unsigned int mask;

    if (e->type == ButtonPress) {
        mask = ButtonPressMask;
    } else if (e->type == ButtonRelease) {
        mask = ButtonReleaseMask;
    } else {
        debug(("\tNot replaying unknown event type %d\n", e->type));
        return;
    }
    get_event_child_windows_mouse(&event, &child, mask);
    if (event == None) {
        debug(("\tNot replaying event, Event = None\n"));
        return;
    }
    if (XTranslateCoordinates(dpy, e->window, event,
                              e->x, e->y, &x, &y, &junk) != 0) {
        e->x = x;
        e->y = y;
    }
    e->window = event;
    if (child != event) e->subwindow = child;
    else e->subwindow = None;
    e->time = event_timestamp;
    if (e->time == CurrentTime) {
        e->time = last_quote_time;
    }
    XSendEvent(dpy, e->window, True, mask, (XEvent *)e);
}

key_fn keyboard_find_function(XKeyEvent *xevent, struct _arglist **args)
{
    boundkey *kb;
    int code;

    code = xevent->keycode;

    for (kb = boundkeys; kb != NULL; kb = kb->next) {
        if (kb->keycode == code
            && kb->modifiers == (xevent->state & (~AllLocksMask))
            && kb->depress == xevent->type) {

            if (args != NULL) *args = kb->args;
            return kb->function;
        }
    }
    return NULL;
}

Bool keyboard_handle_event(XKeyEvent *xevent)
{
    key_fn fn;
    arglist *args;
#ifdef DEBUG
    KeySym ks;

    ks = XKeycodeToKeysym(dpy, xevent->keycode, 0);
    debug(("\tWindow 0x%08X, keycode %d, state %d, keystring %s\n",
           (unsigned int)xevent->window, xevent->keycode,
           xevent->state, XKeysymToString(ks)));
#endif /* DEBUG */

    fn = keyboard_find_function(xevent, &args);
    if (xevent->window == focus_revert_window && focus_current != NULL) {
        /* nasty hack, but simplifies all user-visible functions */
        xevent->window = focus_current->window;
    }
    if (fn == NULL) {
        return False;
    } else {
        if (quoting) {
            unquote((XEvent *)xevent);
        } else {
            (*fn)((XEvent *)xevent, args);
        }
        return True;
    }
}

/* FIXME: perhaps should separate mouse/keyboard stuff, AGAIN */
#define DBLCLICK_THRESH 1000
#define DRAG_THRESH 5
#define ANYBUTTONMASK (Button1Mask | Button2Mask | Button3Mask \
                       | Button4Mask | Button5Mask)

static int saved_x, saved_y, saved_location;
static unsigned int saved_state, saved_button;
/* for determining double-clicks, see mouse_down() */
static Bool last_mousedown_set[5] = { False, False, False, False, False };

static int distance_squared(int x1, int y1, int x2, int y2);
static int button_to_index(unsigned int button);
static Bool mouse_invoke(XEvent *xevent, click_type type);
static Bool mouse_down(XButtonEvent *xevent);
static Bool mouse_up(XButtonEvent *xevent);
static Bool mouse_moved(XMotionEvent *xevent);
static void click_to_focus_clicked(XButtonEvent *xevent);

void mouse_get_drag_info(unsigned int *button, int *x, int *y)
{
    *button = saved_button;
    *x = saved_x;
    *y = saved_y;
}

/* we don't want to pull in math libraries only for square root function,
 * so we work in distance squared */
static int distance_squared(int x1, int y1, int x2, int y2)
{
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

static int button_to_index(unsigned int button)
{
    /* don't know if values of these symbols in headers change
     * on some machines...better be safe */
    switch (button) {
        case Button1:
            return 0;
        case Button2:
            return 1;
        case Button3:
            return 2;
        case Button4:
            return 3;
        case Button5:
            return 4;
        default:
            return -1;
    }
}

/*
 * Returns true if invoked function for event
 */

static Bool mouse_invoke(XEvent *xevent, click_type type)
{
    boundbutton *bb;
    Time time;

    for (bb = boundbuttons; bb != NULL; bb = bb->next) {
        if (bb->type == type &&
            bb->button == saved_button &&
            bb->location == saved_location &&
            bb->modifiers == saved_state) {

            time = xevent->xbutton.time;
            (bb->function)(xevent, bb->args);
            XUngrabPointer(dpy, time);
            return True;
        }
    }
    return False;
}

/*
 * On MouseDown,
 * grab pointer
 * check for double-click
 * if no double-click, just wait for MouseUp or Motion with pointer grabbed
 * 
 * Also deal with click-to-focus and GNOME root click proxying, but
 * those are trivial (whereas doing the whole click/drag/double-click
 * thing correctly is a little tricky)
 */

static Bool mouse_down(XButtonEvent *xevent)
{
    int button_ndx;
    static Time last_mousedown[5]; /* for determining double-clicks */
    static Window last_mousedown_window[5];

    button_ndx = button_to_index(xevent->button);

    saved_x = xevent->x_root;
    saved_y = xevent->y_root;
    saved_state = xevent->state & (~(ANYBUTTONMASK | AllLocksMask));
    saved_button = xevent->button;
    saved_location = get_location(xevent);

    XGrabPointer(dpy, xevent->window, False,
                 ButtonReleaseMask | ButtonPressMask | ButtonMotionMask,
                 GrabModeAsync, GrabModeAsync, None,
                 None, xevent->time);
    
    click_to_focus_clicked(xevent);

    /* must check absolute value of difference, X timestamps wrap */
    if (last_mousedown_set[button_ndx] &&
        ABS(xevent->time - last_mousedown[button_ndx]) <= DBLCLICK_THRESH &&
        xevent->window == last_mousedown_window[button_ndx]) {
        /* we have a double click; invalidate timestamp, invoke function */
        last_mousedown_set[button_ndx] = False;
        return mouse_invoke((XEvent *)xevent, MOUSE_DOUBLECLICK);
    } else {
        /* listen and wait; mouse-up = click, movement = drag */
        last_mousedown_set[button_ndx] = True;
        last_mousedown[button_ndx] = xevent->time;
        last_mousedown_window[button_ndx] = xevent->window;

        if (xevent->window == root_window) {
            ewmh_proxy_buttonevent((XEvent *)xevent);
        }
        return False;
    }
}

/*
 * On MouseUp, ungrab pointer whether or not invoked anything
 * (don't care about Motion events while button not down)
 * 
 * This also means that if you:
 * 1. left Mousedown on titlebar
 * 2. right-click on titlebar
 * 3. drag (left button still down)
 * this does NOT invoke drag action; similar thing for step (3)
 * MouseUp, does not invoke Click function.  Some toolkits work like
 * this, some don't.  Feels pretty intuitive.
 */

static Bool mouse_up(XButtonEvent *xevent)
{
    Time time;
    Bool retval;

    time = xevent->time;
    retval = mouse_invoke((XEvent *)xevent, MOUSE_CLICK);
    XUngrabPointer(dpy, time);
    return retval;
}

/*
 * On Motion events, check to see if distance travelled from original
 * MouseDown greater than threshold; invoke drag if so; keep waiting
 * (with pointer grabbed) if not.
 */

static Bool mouse_moved(XMotionEvent *xevent)
{
    Bool retval;
    XEvent ev;
    int i;
    
    retval = False;
    if (distance_squared(saved_x, saved_y, xevent->x_root,
                         xevent->y_root) >= DRAG_THRESH * DRAG_THRESH) {
        retval = mouse_invoke((XEvent *)xevent, MOUSE_DRAG);
        if (retval) {
            /* this is a little tricky
             * 
             * if dragged something
             * (a) ungrab pointer now when drag done (not when drag started)
             * (b) ensure server knows about Ungrab event
             * (c) get all events from server queue, put on local queue
             * (d) remove Motion events from local queue
             * 
             * if not done this way, may get stray Motion events later
             * on, will begin another drag action (remove calls to XSync
             * and XCheckMaskEvent then very quickly click and move
             * mouse, it will seem as if mouse up event ignored)
             */
            XUngrabPointer(dpy, CurrentTime);
            XSync(dpy, False);
            while (XCheckMaskEvent(dpy, ButtonMotionMask, &ev));
            /* invalidate pending double-clicks
             * eg, don't allow quick drag followed by click to trigger
             * double-click, force two MouseDowns after any drag
             * (this is the intuitive behaviour) */
            for (i = 0; i < 5; i++)
                last_mousedown_set[i] = False;
        }
    }
    return retval;
}

static void click_to_focus_clicked(XButtonEvent *xevent)
{
    client_t *client;

    if (xevent->button == Button1
        && (xevent->state & ~(AllLocksMask | ANYBUTTONMASK)) == 0) {
        
        client = client_find(xevent->window);
        if (client != NULL && client->focus_policy == ClickToFocus) {
            focus_set(client, xevent->time);
            stacking_raise(client);
            if (client->pass_focus_click) {
                XAllowEvents(dpy, ReplayPointer, CurrentTime);
            }
        }
    }
}

Bool mouse_handle_event(XEvent *xevent)
{
    switch (xevent->type) {
        case ButtonPress:
            return mouse_down(&xevent->xbutton);
        case ButtonRelease:
            return mouse_up(&xevent->xbutton);
        case MotionNotify:
            return mouse_moved(&xevent->xmotion);
        default:
            return False;
    }
}

#if 0
/*
 * This gets a little bit messy because we can't really "grab" a
 * ButtonRelease event like we can grab a KeyRelease event -
 * XGrabButton and XGrabKeys work differently (and this is not
 * immediately obvious from the documentation, BTW).  To get a
 * ButtonRelease, we grab the pointer on the corresponding
 * ButtonPress.
 */

#define ANYBUTTONMASK (Button1Mask | Button2Mask | Button3Mask \
                       | Button4Mask | Button5Mask)

Bool mouse_handle_event(XEvent *xevent)
{
    boundbutton *mb;
    unsigned int button, state;
    int location, i;
    client_t *client;
    Bool set_focus = False;
    click_type type;
    static int grabbed_button = 0;
    static Time last_mousedown[5]; /* for determining double-clicks */
    static Bool last_mousedown_set[5] = { False, False, False, False, False };

    button = xevent->xbutton.button;
    state = xevent->xbutton.state & (~(ANYBUTTONMASK | AllLocksMask));
    location = get_location(&xevent->xbutton);
    debug(("\tMouse event, button = %d, state = 0x%08X\n", button, state));
    
    if (xevent->xbutton.button == Button1
        && xevent->xbutton.state == 0) {
        client = client_find(xevent->xbutton.window);
        if (client != NULL
            && client->focus_policy == ClickToFocus) {
            focus_set(client, xevent->xbutton.time);
            stacking_raise(client);
            set_focus = True;
        }
    }

    if (xevent->type == ButtonPress) {
        switch (xevent->xbutton.button) {
            case Button1:
                i = 0;
                break;
            case Button2:
                i = 1;
                break;
            case Button3:
                i = 2;
                break;
            case Button4:
                i = 3;
                break;
            case Button5:
                i = 4;
                break;
            default:
                i = -1;
        }
        type = MOUSE_DRAG;
        if (i != -1) {
            /* X timestamps can wrap, must check absolute value */
            if (last_mousedown_set[i] &&
                ABS(xevent->xbutton.time - last_mousedown[i]) <= 1000) {

                type = MOUSE_DOUBLECLICK;
                last_mousedown_set[i] = False;
            } else {
                last_mousedown[i] = xevent->xbutton.time;
                last_mousedown_set[i] = True;
            }
        }
    } else {
        type = MOUSE_CLICK;
    }
    
    for (mb = boundbuttons; mb != NULL; mb = mb->next) {
        if (button == mb->button
            && state == mb->modifiers
            && (location & mb->location)) {
            if (type == mb->type ||
                (type == MOUSE_DOUBLECLICK && mb->type == MOUSE_DRAG)) {
                if (quoting) {
                    unquote(xevent);
                } else {
                    if (grabbed_button == 0
                        || (grabbed_button == xevent->xbutton.button
                            && xevent->type == ButtonRelease
                            && in_window(xevent, xevent->xbutton.window))) {
                        debug(("\tCalling function\n"));
                        (*mb->function)(xevent, mb->args);
                    } else {
                        debug(("\tNot calling function\n"));
                    }
                }
                debug(("\tUngrabbing pointer 1\n"));
                XUngrabPointer(dpy, xevent->xbutton.time);
                grabbed_button = 0;
            } else {
                debug(("\tGRABBING POINTER\n"));

                XGrabPointer(dpy, xevent->xbutton.window, False,
                             ButtonReleaseMask | ButtonPressMask,
                             GrabModeAsync, GrabModeAsync, None,
                             None, xevent->xbutton.time);
                grabbed_button = button;
            }
            return True;
        }
    }
    if (set_focus && client->pass_focus_click)
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
    if (grabbed_button != 0
        && xevent->type == ButtonRelease
        && xevent->xbutton.button == grabbed_button) {
        debug(("\tUngrabbing pointer 2\n"));
        XUngrabPointer(dpy, xevent->xbutton.time);
        grabbed_button = 0;
        return True;
    } else if (grabbed_button == 0) {
        debug(("\tUngrabbing pointer 3\n"));
        XUngrabPointer(dpy, xevent->xbutton.time);
    }
    if (xevent->xbutton.window == root_window) {
        ewmh_proxy_buttonevent(xevent);
        return True;
    }
    if (set_focus) return True;
    else return False;
}

#endif

/* counterpart of keyboard_quote, does not need to be public */
static void unquote(XEvent *e)
{
    XUnmapWindow(dpy, white_window);
    XDestroyWindow(dpy, white_window);
    white_window = None;
    quoting = False;
    if (e->type == ButtonPress
        || e->type == ButtonRelease)
        mouse_replay(&e->xbutton);
    else if (e->type == KeyPress
             || e->type == KeyRelease)
        keyboard_replay(&e->xkey);
}

/*
 * This is simple enough that we don't need to bring in lex (or, God
 * forbid, yacc).  Looks ugly, mostly just string manipulation.  Used
 * for both keyboard and mouse parsing; function arg is difference.
 */

int parse_string(char *keystring, unsigned int *button_ret,
                 unsigned int *modifiers_ret,
                 int (*fn)(char *, unsigned int *))
{
    char buf[512];
    char *cp1, *cp2;
    unsigned int keycode;
    unsigned int modifiers, tmp_modifier;

    if (keystring == NULL) return 0;
    memset(buf, 0, 512);
    modifiers = 0;

    while (*keystring != '\0') {
        while (isspace(*keystring)) keystring++;
        cp1 = strchr(keystring, '|');
        if (cp1 == NULL) {
            strncpy(buf, keystring, 512);
            buf[511] = '\0';
            while (isspace(*(buf + strlen(buf) - 1)))
                *(buf + strlen(buf) - 1) = '\0';
            if (fn(buf, &keycode) == -1) {
                return 0;
            }
            *modifiers_ret = modifiers;
            *button_ret = keycode;
            return 1;
        }
        /* found a modifier key */
        cp2 = cp1 - 1;
        while (isspace(*cp2)) cp2--;
        memcpy(buf, keystring, MIN(511, cp2 - keystring + 1));
        buf[MIN(511, cp2 - keystring + 1)] = '\0';

        if (strcasecmp(buf, "Mod1") == 0) {
            tmp_modifier = Mod1Mask;
        } else if (strcasecmp(buf, "Mod1Mask") == 0) {
            tmp_modifier = Mod1Mask;
        } else if (strcasecmp(buf, "Mod2") == 0) {
            tmp_modifier = Mod2Mask;
        } else if (strcasecmp(buf, "Mod2Mask") == 0) {
            tmp_modifier = Mod2Mask;
        } else if (strcasecmp(buf, "Mod3") == 0) {
            tmp_modifier = Mod3Mask;
        } else if (strcasecmp(buf, "Mod3Mask") == 0) {
            tmp_modifier = Mod3Mask;
        } else if (strcasecmp(buf, "Mod4") == 0) {
            tmp_modifier = Mod4Mask;
        } else if (strcasecmp(buf, "Mod4Mask") == 0) {
            tmp_modifier = Mod4Mask;
        } else if (strcasecmp(buf, "Mod5") == 0) {
            tmp_modifier = Mod5Mask;
        } else if (strcasecmp(buf, "Mod5Mask") == 0) {
            tmp_modifier = Mod5Mask;
        } else if (strcasecmp(buf, "Shift") == 0) {
            tmp_modifier = ShiftMask;
        } else if (strcasecmp(buf, "ShiftMask") == 0) {
            tmp_modifier = ShiftMask;
        } else if (strcasecmp(buf, "Control") == 0) {
            tmp_modifier = ControlMask;
        } else if (strcasecmp(buf, "ControlMask") == 0) {
            tmp_modifier = ControlMask;
        } else if (strcasecmp(buf, "Meta") == 0) {
            tmp_modifier = MetaMask;
        } else if (strcasecmp(buf, "MetaMask") == 0) {
            tmp_modifier = MetaMask;
        } else if (strcasecmp(buf, "Super") == 0) {
            tmp_modifier = SuperMask;
        } else if (strcasecmp(buf, "SuperMask") == 0) {
            tmp_modifier = SuperMask;
        } else if (strcasecmp(buf, "Hyper") == 0) {
            tmp_modifier = HyperMask;
        } else if (strcasecmp(buf, "HyperMask") == 0) {
            tmp_modifier = HyperMask;
        } else if (strcasecmp(buf, "Alt") == 0) {
            tmp_modifier = AltMask;
        } else if (strcasecmp(buf, "AltMask") == 0) {
            tmp_modifier = AltMask;
        } else {
            fprintf(stderr, "AHWM: Could not figure out modifier '%s'\n", buf);
            return 0;
        }
        modifiers |= tmp_modifier;

        keystring = cp1 + 1;
    }
    return 0;
}

/* utility for use with parse_string */
static int figure_button(char *s, unsigned int *b)
{
    if (strcasecmp(s, "Button1") == 0) {
        *b = Button1;
        return 0;
    } else if (strcasecmp(s, "Button2") == 0) {
        *b = Button2;
        return 0;
    } else if (strcasecmp(s, "Button3") == 0) {
        *b = Button3;
        return 0;
    } else if (strcasecmp(s, "Button4") == 0) {
        *b = Button4;
        return 0;
    } else if (strcasecmp(s, "Button5") == 0) {
        *b = Button5;
        return 0;
    } else {
        fprintf(stderr, "AHWM: Couldn't parse button '%s'\n", s);
        return -1;
    }
}

/* utility for use with parse_string */
static int figure_keycode(char *s, unsigned int *k) 
{
    KeySym ks;

    ks = XStringToKeysym(s);
    if (ks == NoSymbol) {
        fprintf(stderr, "AHWM: Couldn't figure out '%s'\n", s);
        return -1;
    }
    *k = XKeysymToKeycode(dpy, ks);
    if (*k == 0) {
        fprintf(stderr,
                "AHWM: XKeysymToKeycode(%s) failed (perhaps unmapped?)", s);
        return -1;
    }
    return 0;
}

/* returns the windows for placing a synthetic mouse event */
static void get_event_child_windows_mouse(Window *event, Window *child,
                                          unsigned int mask)
{
    Window junk1, new;
    int junk2;
    unsigned int junk3;
    client_t *client;
    XWindowAttributes xwa;

    *event = *child = None;
    /* get frame window */
    if (XQueryPointer(dpy, root_window, &junk1, &new,
                      &junk2, &junk2, &junk2, &junk2, &junk3) == 0) {
        debug(("\tXQueryPointer returns zero\n"));
        return;
    }
    /* get client window or frame window */
    if (XQueryPointer(dpy, new, &junk1, &new,
                      &junk2, &junk2, &junk2, &junk2, &junk3) == 0) {
        debug(("\tXQueryPointer returns zero\n"));
        return;
    }
    client = client_find(new);
    if (client == NULL) {
        debug(("\tPointer is not over a client, not replaying event\n"));
        return;
    }
    if (new == client->titlebar) {
        debug(("\tPointer is over titlebar, not replaying event\n"));
        return;
    }
    
    for (;;) {
        if (XGetWindowAttributes(dpy, new, &xwa) == 0) {
            debug(("\tXGetWindowAttributes fails, returning\n"));
            return;
        }
        *child = new;
        if (xwa.all_event_masks & mask) *event = new;
        if (XQueryPointer(dpy, new, &junk1, &new,
                          &junk2, &junk2, &junk2, &junk2, &junk3) == 0) {
            debug(("\tXQueryPointer returns zero\n"));
            return;
        }
        debug(("\tNew is 0x%08X\n", new));
        if (new == None || new == *child) return;
    }
}

/* returns the windows for placing a synthetic key event */
static void get_event_child_windows_keyboard(Window *event, Window *child,
                                             unsigned int mask)
{
    Window junk1, new;
    int junk2;
    unsigned int junk3;
    XWindowAttributes xwa;

    *event = *child = None;
    XGetInputFocus(dpy, &new, &junk2);
    if (XGetWindowAttributes(dpy, new, &xwa) == 0) {
        debug(("\tXGetWindowAttributes fails, returning\n"));
        return;
    }
    if (!(xwa.all_event_masks & mask)) {
        debug(("\tWindow 0x%08X does not accept events\n", new));
        if (focus_current == NULL) {
            /* nobody to whom to replay */
            *event = None;
            *child = None;
            return;
        }
        new = focus_current->window;
    } 
    *event = new;
    
    for (;;) {
        if (XQueryPointer(dpy, new, &junk1, &new,
                          &junk2, &junk2, &junk2, &junk2, &junk3) == 0) {
            debug(("\tXQueryPointer returns zero\n"));
            return;
        }
        if (XGetWindowAttributes(dpy, new, &xwa) == 0) {
            debug(("\tXGetWindowAttributes fails, returning\n"));
            return;
        }
        if (client_find(new) != NULL) return;
        *child = new;
        debug(("\tNew is 0x%08X\n", new));
        if (new == None || new == *child) return;
    }
}

static Bool in_window(XEvent *xevent, Window w)
{
    XWindowAttributes xwa;

    if (XGetWindowAttributes(dpy, w, &xwa) == 0) return False;
    if (xevent->xbutton.x >= xwa.x
        && xevent->xbutton.y >= xwa.y
        && xevent->xbutton.x <= xwa.x + xwa.width
        && xevent->xbutton.y <= xwa.y + xwa.height)
        return True;
    return False;
}

static int get_location(XButtonEvent *e)
{
    client_t *client;
    
    if (e->window == root_window) return MOUSE_ROOT;
    client = client_find(e->window);
    if (client == NULL) return MOUSE_NOWHERE;
    if (e->window == client->titlebar)
        return MOUSE_TITLEBAR;
    if (e->window == client->frame) {
        /* when we grab button1 for click-to-focus, we'll never get
         * events on the titlebar window for some reason */
        if (client->titlebar != None
            && e->x_root >= client->x
            && e->y_root >= client->y
            && e->x_root <= client->x + client->width
            && e->y_root <= client->y + TITLE_HEIGHT)
            return MOUSE_TITLEBAR;
        return MOUSE_FRAME;
    }
    return MOUSE_NOWHERE;
}

static void warn(KeySym new, char *old, char *mod)
{
    fprintf(stderr,
            "AHWM WARNING:  '%s' is generated by both the keysyms\n"
            "'%s' and '%s' on different modifiers.\n"
            "This is almost certainly an error, and some\n"
            "applications may misbehave.  Using the last modifier mapping.\n"
            "The only way to remove this error message is to remap\n"
            "your modifiers using 'xmodmap'\n",
            mod, old, XKeysymToString(new));
}

/* utility for keyboard_init */
static void figure_lock(KeySym keysym, int bit)
{
    int i;
    static KeySym locks[] = {
        XK_Scroll_Lock,
        XK_Num_Lock,
        XK_Caps_Lock,
        XK_Shift_Lock,
        XK_Kana_Lock,
        XK_ISO_Lock,
        XK_ISO_Level3_Lock,
        XK_ISO_Group_Lock,
        XK_ISO_Next_Group_Lock,
        XK_ISO_Prev_Group_Lock,
        XK_ISO_First_Group_Lock,
        XK_ISO_Last_Group_Lock,
    };

    for (i = 0; i < 12; i++) {
        if (keysym == locks[i]) {
            keyboard_is_lock[bit] = True;
        }
    }
}

/*
 * This will modify the array of all the posible combinations of the
 * various 'lock' modifiers the user has mapped.  All arguments are
 * used for keeping state in the recursion.
 * 
 * For example, if the user has XK_Caps_Lock generating the 'lock'
 * modifier, XK_Num_Lock generating the 'Mod3' modifier and
 * XK_Scroll_Lock generating 'Mod4', this will generate:
 * 
 * LockMask
 * LockMask | Mod3Mask
 * LockMask | Mod3Mask | Mod4Mask
 * LockMask | Mod4Mask
 * Mod3Mask
 * Mod3Mask | Mod4Mask
 * Mod4Mask
 * 
 * Of course, this takes an exponential amount of time and space, so
 * hopefully the user doesn't have six locking modifiers.
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
 */

static void modifier_combinations_helper(unsigned int state,
                                         int *n, int bit)
{
    int i;

    if (!keyboard_is_lock[bit]) return;

    state |= (1 << bit);
    modifier_combinations[(*n)++] = state;
    for (i = bit + 1; i < 8; i++) {
        modifier_combinations_helper(state, n, i);
    }
}
