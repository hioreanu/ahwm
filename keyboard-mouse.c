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
#define DEBUG 1
#include "debug.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

typedef struct _keybinding {
    unsigned int keycode;
    unsigned int modifiers;
    int depress;
    key_fn function;
    void *arg;
    struct _keybinding *next;
} keybinding;

typedef struct _mousebinding {
    unsigned int button;
    unsigned int modifiers;
    int depress;
    int location;
    mouse_fn function;
    void *arg;
    struct _mousebinding *next;
} mousebinding;

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
static keybinding *keybindings = NULL;
static mousebinding *mousebindings = NULL;
/* combinations of modifier keys to ignore */
static unsigned int *modifier_combinations = NULL;
static int n_modifier_combinations;
/* used for quoting key & mouse bindings */
static Bool quoting = False;
static Time last_quote_time;

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

void keyboard_ignore(XEvent *e, void *v)
{
    return;
}

void keyboard_bind_ex(unsigned int keycode, unsigned int modifiers,
                      int depress, key_fn fn, void *arg)
{
    keybinding *newbinding;

    newbinding = Malloc(sizeof(keybinding));
    if (newbinding == NULL) {
        fprintf(stderr, "XWM: Cannot bind key, out of memory\n");
        return;
    }
    newbinding->next = keybindings;
    newbinding->keycode = keycode;
    newbinding->modifiers = modifiers;
    newbinding->depress = depress;
    newbinding->function = fn;
    newbinding->arg = arg;
    keybindings = newbinding;
}

void mouse_bind_ex(unsigned int button, unsigned int modifiers,
                   int depress, int location, mouse_fn fn, void *arg)
{
    mousebinding *newbinding;

    newbinding = Malloc(sizeof(mousebinding));
    if (newbinding == NULL) {
        fprintf(stderr, "XWM: Cannot bind mouse button, out of memory\n");
        return;
    }
    newbinding->button = button;
    newbinding->modifiers = modifiers;
    newbinding->depress = depress;
    newbinding->location = location;
    newbinding->function = fn;
    newbinding->arg = arg;
    newbinding->next = mousebindings;
    mousebindings = newbinding;
}

void keyboard_unbind_ex(unsigned int keycode, unsigned int modifiers,
                        int depress) 
{
    keybinding *kb, *tmp;

    tmp = NULL;
    for (kb = keybindings; kb != NULL; kb = kb->next) {
        if (kb->keycode == keycode
            && kb->modifiers == modifiers
            && kb->depress == depress) {
            if (tmp == NULL) {
                keybindings = keybindings->next;
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
                     int depress, int location)
{
    mousebinding *mb, *tmp;

    tmp = NULL;
    for (mb = mousebindings; mb != NULL; mb = mb->next) {
        if (mb->button == button
            && mb->modifiers == modifiers
            && mb->depress == depress
            && mb->location == location) {
            if (tmp == NULL) {
                mousebindings = mousebindings->next;
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
                   key_fn fn, void *arg)
{
    unsigned int keycode;
    unsigned int modifiers;

    if (parse_string(keystring, &keycode, &modifiers, figure_keycode) != 1) {
        fprintf(stderr, "XWM: Cannot bind key, bad keystring '%s'\n", keystring);
        return;
    }
    keyboard_bind_ex(keycode, modifiers, depress, fn, arg);
}

void mouse_bind(char *mousestring, int depress,
                int location, mouse_fn fn, void *arg)
{
    unsigned int button;
    unsigned int modifiers;

    if (parse_string(mousestring, &button, &modifiers, figure_button) != 1) {
        fprintf(stderr, "XWM: Cannot bind mouse button, bad string '%s'\n",
                mousestring);
        return;
    }
    mouse_bind_ex(button, modifiers, depress, location, fn, arg);
}

void keyboard_unbind(char *keystring, int depress)
{
    unsigned int keycode;
    unsigned int modifiers;
    
    if (parse_string(keystring, &keycode, &modifiers, figure_keycode) != 1) {
        fprintf(stderr,
                "XWM: Cannot unbind key, bad keystring '%s'\n", keystring);
        return;
    }
    keyboard_unbind_ex(keycode, modifiers, depress);
}

void mouse_unbind(char *mousestring, int depress, int location)
{
    unsigned int button;
    unsigned int modifiers;

    if (parse_string(mousestring, &button, &modifiers, figure_button) != 1) {
        fprintf(stderr, "XWM: Cannot unbind mouse button, bad string '%s'\n",
                mousestring);
        return;
    }
    mouse_unbind_ex(button, modifiers, depress, location);
}

void keyboard_grab_keys(Window w)
{
    keybinding *kb;
    int i;

    for (kb = keybindings; kb != NULL; kb = kb->next) {
        XGrabKey(dpy, kb->keycode, kb->modifiers, w, True,
                 GrabModeAsync, GrabModeAsync);
        for (i = 0; i < n_modifier_combinations; i++) {
            XGrabKey(dpy, kb->keycode,
                     modifier_combinations[i] | kb->modifiers,
                     w, True, GrabModeAsync, GrabModeAsync);
        }
    }
}

void mouse_grab_buttons(client_t *client)
{
    mousebinding *mb;
    unsigned int mask;
    int i;
    
    for (mb = mousebindings; mb != NULL; mb = mb->next) {
        mask = ButtonPressMask | ButtonReleaseMask;
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

void keyboard_quote(XEvent *e, void *v)
{
    XSetWindowAttributes xswa;

    debug(("\tQuoting\n"));
    quoting = True;
    xswa.background_pixel = white;
    XChangeWindowAttributes(dpy, root_window, CWBackPixel, &xswa);
    XClearWindow(dpy, root_window);
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
    debug(("Sending keyboard event\n"));
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

Bool keyboard_handle_event(XKeyEvent *xevent)
{
    keybinding *kb;
    int code;

#ifdef DEBUG
    KeySym ks;

    ks = XKeycodeToKeysym(dpy, xevent->keycode, 0);
    debug(("\tWindow 0x%08X, keycode %d, state %d, keystring %s\n",
           (unsigned int)xevent->window, xevent->keycode,
           xevent->state, XKeysymToString(ks)));
#endif /* DEBUG */

    code = xevent->keycode;

    for (kb = keybindings; kb != NULL; kb = kb->next) {
        if (kb->keycode == code
            && kb->modifiers == (xevent->state & (~AllLocksMask))
            && kb->depress == xevent->type) {
            if (quoting) {
                unquote((XEvent *)xevent);
            } else {
                (*kb->function)((XEvent *)xevent, kb->arg);
            }
            return True;
        }
    }
    return False;
}

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

#include "xev.h"

Bool mouse_handle_event(XEvent *xevent)
{
    mousebinding *mb;
    unsigned int button, state;
    int location;
    client_t *client;
    static int grabbed_button = 0;
    Bool set_focus = False;

    button = xevent->xbutton.button;
    state = xevent->xbutton.state & (~(ANYBUTTONMASK | AllLocksMask));
    location = get_location(&xevent->xbutton);
    debug(("\tMouse event, button = %d, state = 0x%08X\n", button, state));
    xev_print(xevent);
    
    if (xevent->xbutton.button == Button1
        && xevent->xbutton.state == 0) {
        client = client_find(xevent->xbutton.window);
        if (client != NULL
            && client->focus_policy == ClickToFocus) {
            focus_set(client, xevent->xbutton.time);
            set_focus = True;
        }
    }
    
    for (mb = mousebindings; mb != NULL; mb = mb->next) {
        if (button == mb->button
            && state == mb->modifiers
            && (location & mb->location)) {
            if (xevent->type == mb->depress) {
                if (quoting) {
                    unquote(xevent);
                } else {
                    if (grabbed_button == 0
                        || (grabbed_button == xevent->xbutton.button
                            && xevent->type == ButtonRelease
                            && in_window(xevent, xevent->xbutton.window))) {
                        debug(("Calling function\n"));
                        (*mb->function)(xevent, mb->arg);
                    } else {
                        debug(("Not calling function\n"));
                    }
                }
                debug(("Ungrabbing pointer 1\n"));
                XUngrabPointer(dpy, xevent->xbutton.time);
                grabbed_button = 0;
            } else {
                debug(("GRABBING POINTER\n"));

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
        debug(("Ungrabbing pointer 2\n"));
        XUngrabPointer(dpy, xevent->xbutton.time);
        grabbed_button = 0;
        return True;
    } else if (grabbed_button == 0) {
        debug(("Ungrabbing pointer 3\n"));
        XUngrabPointer(dpy, xevent->xbutton.time);
    }
    if (set_focus) return True;
    else return False;
}

/* counterpart of keyboard_quote, does not need to be public */
static void unquote(XEvent *e)
{
    XSetWindowAttributes xswa;
    
    quoting = False;
    xswa.background_pixel = workspace_pixels[workspace_current - 1];
    XChangeWindowAttributes(dpy, root_window, CWBackPixel, &xswa);
    XClearWindow(dpy, root_window);
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
            fprintf(stderr, "XWM: Could not figure out modifier '%s'\n", buf);
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
        fprintf(stderr, "XWM: Couldn't parse button '%s'\n", s);
        return -1;
    }
}

/* utility for use with parse_string */
static int figure_keycode(char *s, unsigned int *k) 
{
    KeySym ks;

    ks = XStringToKeysym(s);
    if (ks == NoSymbol) {
        fprintf(stderr, "XWM: Couldn't figure out '%s'\n", s);
        return -1;
    }
    *k = XKeysymToKeycode(dpy, ks);
    if (*k == 0) {
        fprintf(stderr,
                "XWM: XKeysymToKeycode(%s) failed (perhaps unmapped?)", s);
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
        new = focus_current->window;
    } 
    *event = new;
    
    for (;;) {
        if (XQueryPointer(dpy, new, &junk1, &new,
                          &junk2, &junk2, &junk2, &junk2, &junk3) == 0) {
            debug(("XQueryPointer returns zero\n"));
            return;
        }
        if (XGetWindowAttributes(dpy, new, &xwa) == 0) {
            debug(("XGetWindowAttributes fails, returning\n"));
            return;
        }
        if (client_find(new) != NULL) return;
        *child = new;
        debug(("New is 0x%08X\n", new));
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
            "XWM WARNING:  '%s' is generated by both the keysyms\n"
            "'%s' and '%s' on different modifiers.\n"
            "This is almost certainly an error, and some applications\n"
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
