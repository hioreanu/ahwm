/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

/*
 * OK, the stuff in here gets a little bit confusing because we have
 * to do things *just so* or client applications will misbehave.  Each
 * function has a distinct purpose.  There are a number of separate
 * things we can do in any given function:
 * 
 * 1.  update some pointers, like focus_current or focus_stacks[]
 * 2.  update the focus stack (focus order)
 * 3.  update clients' titlebars to show input focus
 * 4.  call XSetInputFocus()
 * 5.  raise the active client
 */

/* 
 * invariant:
 * 
 * focus_current == focus_stacks[workspace_current - 1]
 * 
 * This should hold whenever we enter or leave the window manager.  In
 * a few fringe cases, focus_current is not the real input focus (ie,
 * what XGetInputFocus() would return), but represents what what the
 * real input focus will be once some condition has been satisfied.
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include "focus.h"
#include "client.h"
#include "workspace.h"
#include "debug.h"
#include "event.h"
#include "ewmh.h"

client_t *focus_current = NULL;

client_t *focus_stacks[NO_WORKSPACES] = { NULL };

static void focus_change_current(client_t *, Time, Bool);
static void focus_set_internal(client_t *, Time, Bool);
static void permute(client_t *, client_t *);

static Bool in_alt_tab = False; /* see focus_alt_tab, focus_ensure */

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_add(client_t *client, Time timestamp)
{
    client_t *old;

    focus_remove(client, timestamp);
    old = focus_stacks[client->workspace - 1];
    if (old == NULL) {
        debug(("\tsetting focus stack of workspace %d to 0x%08X ('%.10s')\n",
               client->workspace, client->window, client->name));
        client->next_focus = client;
        client->prev_focus = client;
        focus_stacks[client->workspace - 1] = client;
    } else {
        client->next_focus = old;
        client->prev_focus = old->prev_focus;
        old->prev_focus->next_focus = client;
        old->prev_focus = client;
        focus_stacks[client->workspace - 1] = client;
    }
    if (client->workspace == workspace_current) {
        focus_change_current(client, timestamp, True);
    }
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_remove(client_t *client, Time timestamp)
{
    client_t *stack, *orig;

    stack = orig = focus_stacks[client->workspace - 1];
    if (stack == NULL) return;
    do {
        if (stack == client) {
            /* remove from list */
            client->prev_focus->next_focus = client->next_focus;
            client->next_focus->prev_focus = client->prev_focus;
            /* if was focused for workspace, update workspace pointer */
            if (focus_stacks[client->workspace - 1] == client) {
                debug(("\tsetting focus stack of workspace "
                       "%d to 0x%08X ('%.10s')\n",
                       client->workspace, client->next_focus->window,
                       client->next_focus->name));
                focus_stacks[client->workspace - 1] = client->next_focus;
            }
            /* if only client left on workspace, set to NULL */
            if (client->next_focus == client) {
                debug(("\tSetting focus stack of workspace %d to null\n",
                       client->workspace));
                focus_stacks[client->workspace - 1] = NULL;
                client->next_focus = NULL;
                client->prev_focus = NULL;
            }
            /* if removed was focused window, refocus now */
            if (client == focus_current) {
                focus_change_current(client->next_focus, timestamp, True);
            }
            return;
        }
        stack = stack->next_focus;
    } while (stack != orig);
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus iff (call_focus_ensure == True)
 */

static void focus_set_internal(client_t *client, Time timestamp,
                               Bool call_focus_ensure)
{
    client_t *p;

    if (client == focus_current) return;

    if (client == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }
    
    p = focus_stacks[client->workspace - 1];
    if (p == NULL) {
        fprintf(stderr, "XWM: current focus list is empty, shouldn't be\n");
        return;
    }
    do {
        if (p == client) {
            debug(("\tSetting focus stack of workspace %d to 0x%08X ('%.10s')\n",
                   client->workspace, client->window, client->name));
            focus_stacks[client->workspace - 1] = client;
            if (client->workspace == workspace_current) {
                focus_change_current(client, timestamp, call_focus_ensure);
            }
            return;
        }
        p = p->next_focus;
    } while (p != focus_stacks[client->workspace - 1]);
    fprintf(stderr, "XWM: client not found on focus list, shouldn't happen\n");
}

/*
 * This will:
 * update pointers
 * update the focus stack
 * update the clients' titlebars 
 * call XSetInputFocus
 */

void focus_set(client_t *client, Time timestamp)
{
    client_t *old = focus_current;
    
    focus_set_internal(client, timestamp, True);
    if (old != NULL && client != NULL)
        permute(old, client);
}

/*
 * This will:
 * call XSetInputFocus
 * raise active client
 * 
 * This is the only function in the module that will call
 * XSetInputFocus; all other functions call this function.
 */

void focus_ensure(Time timestamp)
{
    if (focus_current == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }

    debug(("\tCalling XSetInputFocus(0x%08X) ('%.10s')\n",
           (unsigned int)focus_current->window, focus_current->name));

    ewmh_active_window_update();

    if (in_alt_tab) {
        client_raise(focus_current);
        return;
    }
    
    /* see ICCCM 4.1.7 */
    if (focus_current->xwmh != NULL &&
        focus_current->xwmh->flags & InputHint &&
        focus_current->xwmh->input == False) {
        /* FIXME:  we shouldn't call XSetInputFocus here */
        debug(("\tInput hint is False\n"));
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
    } else {
        debug(("\tInput hint is True\n"));
        XSetInputFocus(dpy, focus_current->window,
                       RevertToPointerRoot, timestamp);
    }
    if (focus_current->protocols & PROTO_TAKE_FOCUS) {
        debug(("\tUses TAKE_FOCUS protocol\n"));
        client_sendmessage(focus_current, WM_TAKE_FOCUS,
                           timestamp, 0, 0, 0);
    } else {
        debug(("\tDoesn't use TAKE_FOCUS protocol\n"));
    }

    client_raise(focus_current);
}

/*
 * This will:
 * update pointers
 * update titlebars
 * call XSetInputFocus iff (call_focus_ensure == True)
 */

static void focus_change_current(client_t *new, Time timestamp,
                                 Bool call_focus_ensure)
{
    client_t *old;

    old = focus_current;
    focus_current = new;
    client_paint_titlebar(old);
    client_paint_titlebar(new);
    if (call_focus_ensure) focus_ensure(timestamp);
}

#ifdef DEBUG
void dump_focus_list()
{
    client_t *client, *orig;

    orig = client = focus_current;

    printf("STACK: ");
    do {
        if (client == NULL) break;
        printf("\t'%s'\n", client->name);
        client = client->next_focus;
    } while (client != orig);
    printf("\n");
}
#else
#define dump_focus_list() /* */
#endif

/*
 * After extensive experimentation, I determined that this is the best
 * way for this function to behave.
 * 
 * We want clients to receive a Focus{In,Out} event when the focus is
 * transferred, and will only transfer the focus when the alt-tab
 * action is completed (when user lets go of alt).  Some clients
 * (notably all applications which use the QT toolkit) will only think
 * that they've received the input focus when they receive a FocusIn,
 * NotfyNormal event.  The problem is that if we transfer the input
 * focus while we have the keyboard grabbed, this will generate
 * FocusIn, NotfyGrab events, which some applications completely
 * ignore (whether or not this is the correct behaviour, I don't care,
 * that's how it is), and then any XSetInputFocus call to the current
 * focus window made after ungrabbing the keyboard will be a no-op;
 * thus QT applications will not believe they have the input focus
 * (and the way QT is set up, QT applications won't take keyboard
 * input in this state).  Needless to say, this is quite annoying.
 * The solution is to delay all calls to XSetInputFocus until we've
 * ungrabbed the keyboard.
 * 
 * This will behave correctly if clients are added or removed while
 * this function is active.  That's the purpose of the in_alt_tab
 * boolean - it ensures we don't call XSetInputFocus while this
 * function is active.  This will return immediately if all the
 * clients disappear from under our nose.
 * 
 * This is still sub-optimal as some clients may receive FocusIn,
 * NotifyPointer events while we are in this function and will
 * incorrectly think they have the input focus and update themselves
 * accordingly.  Xterm does this.  This only lasts until the function
 * exits, but is a bit annoying.  There is no way to fix this other
 * than fixing the bug in xterm (other window managers have the same
 * behaviour).
 */

void focus_alt_tab(XEvent *xevent, void *v)
{
    client_t *orig_focus;
    unsigned int action_keycode;
    KeyCode keycode_Alt_L, keycode_Alt_R;

    if (focus_current == NULL) {
        debug(("\tNo clients in alt-tab, returning\n"));
        return;
    }
    debug(("\tEntering alt-tab\n"));
    in_alt_tab = True;
    orig_focus = focus_current;
    debug(("\torig_focus = '%s'\n", orig_focus ? orig_focus->name : "NULL"));
    dump_focus_list();
    action_keycode = xevent->xkey.keycode;
    keycode_Alt_L = XKeysymToKeycode(dpy, XK_Alt_L);
    keycode_Alt_R = XKeysymToKeycode(dpy, XK_Alt_R);

    XGrabKeyboard(dpy, root_window, True, GrabModeAsync,
                  GrabModeAsync, CurrentTime);
    for (;;) {
        switch (xevent->type) {
            case KeyPress:
                if (xevent->xkey.keycode == action_keycode) {
                    if (xevent->xkey.state & ShiftMask) {
                        focus_set_internal(focus_current->prev_focus,
                                           event_timestamp, False);
                    } else {
                        focus_set_internal(focus_current->next_focus,
                                           event_timestamp, False);
                    }
                } /* FIXME:  else end the action */
                break;
            case KeyRelease:
                if (xevent->xkey.keycode == keycode_Alt_L
                    || xevent->xkey.keycode == keycode_Alt_R) {
                    /* user let go of all modifiers */
                    if (focus_current != NULL) {
                        permute(orig_focus, focus_current);
                    }
                    
                    XUngrabKeyboard(dpy, CurrentTime);
                    /* we want to make sure server knows we've
                     * ungrabbed keyboard before calling
                     * XSetInputFocus: */
                    XSync(dpy, False);
                    in_alt_tab = False;
                    focus_ensure(CurrentTime);
                    
                    debug(("\tfocus_current = '%.10s'\n",
                           focus_current ? focus_current->name : "NULL"));
                    dump_focus_list();
                    debug(("\tLeaving alt-tab\n"));
                    return;
                }
                break;
            default:
                event_dispatch(xevent);
                if (focus_current == NULL) {
                    debug(("\tAll clients gone, returning from alt-tab\n"));
                    XUngrabKeyboard(dpy, CurrentTime);
                    in_alt_tab = False;
                    focus_ensure(CurrentTime);
                    return;
                }
        }
        event_get(ConnectionNumber(dpy), xevent);
    }
}

/*
 * This will swap two elements in the focus stack, moving one to the
 * top of the stack.
 * 
 * ring looks like this:
 * A-B-...-C-D-E-...-F-A
 * and we want it to look like this:
 * D-A-B-...-C-E-...-F-D
 * 
 * special cases:
 * A-A-...  ->  A-A-...  (no change)
 * A-B-A-...  ->  B-A-B-... (no change, just update focus_current)
 * A-D-E-...-F-A  ->  D-A-E-...-F-D  (just swap A & D)
 * A-B-...-C-D-A  ->  D-A-B-...-C-D  (no change, just update focus_current)
 * We actually don't change B in any way, so it's left out.
 */

static void permute(client_t *A, client_t *D)
{
    client_t *C, *E, *F;

    /* if have only one or two elements, or not moving, done */
    if (A == D || (A->next_focus == D && D->next_focus == A))
        return;

    F = A->prev_focus;
    C = D->prev_focus;
    E = D->next_focus;

    if (A->next_focus == D) {
        /* no B or C, just swap A & D */
        A->prev_focus = D;
        D->next_focus = A;
        
        A->next_focus = E;
        E->prev_focus = A;
        
        F->next_focus = D;
        D->prev_focus = F;
        
    } else if (A->prev_focus == D) {
        /* no E or F, no need for change */
    } else {
        /* B and D are separated by at least one node on each side */
        C->next_focus = E;
        E->prev_focus = C;

        F->next_focus = D;
        D->prev_focus = F;

        D->next_focus = A;
        A->prev_focus = D;
    }
}
