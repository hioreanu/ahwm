/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "mouse.h"
#include "keyboard.h"
#include "move-resize.h"
#include "xwm.h"
#include "cursor.h"
#include "malloc.h"
#include "debug.h"
#include "workspace.h"
#include "event.h"

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

typedef struct _mousebinding {
    unsigned int button;
    unsigned int modifiers;
    int depress;
    int location;
    mouse_fn function;
    void *arg;
    struct _mousebinding *next;
} mousebinding;

static mousebinding *bindings = NULL;

static int figure_button(char *, unsigned int *);
static int get_location(Window w);

void mouse_ignore(XEvent *xevent, void *v)
{
    XUngrabPointer(dpy, CurrentTime);
}

void mouse_grab_buttons(client_t *client)
{
    mousebinding *mb;
    unsigned int *combs, mask;
    int i, n;
    
    for (mb = bindings; mb != NULL; mb = mb->next) {
        combs = keyboard_modifier_combinations(mb->modifiers, &n);
//        mask = (mb->depress==ButtonPress?ButtonPressMask:ButtonReleaseMask);
        mask = ButtonPressMask | ButtonReleaseMask;
        if (mb->location & MOUSE_FRAME) {
            XGrabButton(dpy, mb->button, mb->modifiers, client->frame,
                        True, mask, GrabModeSync, GrabModeAsync,
                        None, cursor_normal);
            for (i = 0; i < n; i++) {
                XGrabButton(dpy, mb->button, combs[i], client->frame,
                            True, mask, GrabModeSync, GrabModeAsync,
                            None, cursor_normal);
            }
        }
        if (mb->location & MOUSE_TITLEBAR && client->titlebar != None) {
            XGrabButton(dpy, mb->button, mb->modifiers, client->titlebar,
                        True, mask, GrabModeSync, GrabModeAsync,
                        None, cursor_normal);
            for (i = 0; i < n; i++) {
                XGrabButton(dpy, mb->button, combs[i], client->titlebar,
                            True, mask, GrabModeSync, GrabModeAsync,
                            None, cursor_normal);
            }
        }
    }
}

static void get_event_child_windows(Window *event, Window *child,
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
    get_event_child_windows(&event, &child, mask);
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

static void mouse_unquote(XButtonEvent *e)
{
    XSetWindowAttributes xswa;
    
    debug(("\tUnquoting\n"));
    quoting = False;
    xswa.background_pixel = workspace_pixels[workspace_current - 1];
    XChangeWindowAttributes(dpy, root_window, CWBackPixel, &xswa);
    XClearWindow(dpy, root_window);
    mouse_replay(e);
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

void mouse_handle_event(XEvent *xevent)
{
    mousebinding *mb;
    unsigned int button, state;
    static int grabbed_button = 0;

    button = xevent->xbutton.button;
    state = xevent->xbutton.state & (~(ANYBUTTONMASK | AllLocksMask));
    debug(("MOUSE event, button = %d, state = 0x%08X\n", button, state));
    
    for (mb = bindings; mb != NULL; mb = mb->next) {
        if (button == mb->button) {
            if (state == mb->modifiers &&
                (mb->location & get_location(xevent->xbutton.window))) {
                if (xevent->type == mb->depress) {
                    if (quoting) {
                        mouse_unquote((XButtonEvent *)xevent);
                    } else {
                        if (grabbed_button == 0
                            || (grabbed_button == xevent->xbutton.button
                                && xevent->type == ButtonRelease
                                && in_window(xevent, xevent->xbutton.window)))
                            (*mb->function)(xevent, mb->arg);
                    }
                    XUngrabPointer(dpy, CurrentTime);
                    grabbed_button = 0;
                } else {
                    XGrabPointer(dpy, xevent->xbutton.window, False,
                                 ButtonReleaseMask | ButtonPressMask,
                                 GrabModeAsync, GrabModeAsync, None,
                                 None, CurrentTime);
                    grabbed_button = button;
                }
                return;
            }
        }
    }
    if (grabbed_button != 0
        && xevent->type == ButtonRelease
        && xevent->xbutton.button == grabbed_button) {
        XUngrabPointer(dpy, CurrentTime);
        grabbed_button = 0;
    }
}

static int get_location(Window w)
{
    client_t *client;
    
    if (w == root_window) return MOUSE_ROOT;
    client = client_find(w);
    if (client == NULL) return MOUSE_NOWHERE;
    if (w == client->frame) return MOUSE_FRAME;
    if (w == client->titlebar) return MOUSE_TITLEBAR;
    return MOUSE_NOWHERE;
}

/*
 * just a cut-and-pase job from keyboard.c, too lazy to filter out the
 * common code right now
 */

void mouse_set_function_ex(unsigned int button, unsigned int modifiers,
                           int depress, int location, mouse_fn fn, void *arg)
{
    mousebinding *newbinding;
    /* for binding to ButtonRelease, where the modifier mask changes */
    static unsigned int table[] = {
        0,                      /* No button */
        Button1Mask,            /* Button1 */
        Button2Mask,            /* Button2 */
        Button3Mask,            /* Button3 */
        Button4Mask,            /* Button4 */
        Button5Mask,            /* Button5 */
    };

    newbinding = Malloc(sizeof(mousebinding));
    if (newbinding == NULL) {
        fprintf(stderr, "XWM: Cannot bind mouse button, out of memory\n");
        return;
    }
    if (depress == MOUSE_RELEASE) {
        debug(("MODIFYING the modifiers, 0x%08X\n", modifiers));
//        modifiers |= table[button];
        debug(("MODIFYING the modifiers, 0x%08X\n", modifiers));
    }
    newbinding->button = button;
    newbinding->modifiers = modifiers;
    newbinding->depress = depress;
    newbinding->location = location;
    newbinding->function = fn;
    newbinding->arg = arg;
    newbinding->next = bindings;
    bindings = newbinding;
}

void mouse_set_function(char *mousestring, int depress,
                        int location, mouse_fn fn, void *arg)
{
    unsigned int button;
    unsigned int modifiers;

    if (mouse_parse_string(mousestring, &button, &modifiers) != 1) {
        fprintf(stderr, "XWM: Cannot bind mouse button, bad string '%s'\n",
                mousestring);
        return;
    }
    mouse_set_function_ex(button, modifiers, depress, location, fn, arg);
}

int mouse_parse_string(char *keystring, unsigned int *button_ret,
                       unsigned int *modifiers_ret)
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
            if (figure_button(buf, &keycode) == -1) {
                fprintf(stderr,
                        "XWM: Couldn't figure out mouse button '%s'\n", buf);
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
        return -1;
    }
}

