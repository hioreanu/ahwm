/* $Id$
 * This was written by Alex Hioreanu in 2001.
 * This code is in the public domain and the author disclaims all
 * copyright privileges.
 */

#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include "focus.h"
#include "client.h"
#include "workspace.h"
#include "debug.h"
#include "event.h"

client_t *focus_current = NULL;

client_t *focus_stacks[NO_WORKSPACES] = { NULL };

static void focus_change_current(client_t *, Time);
static void permute(client_t *, client_t *);

/* 
 * invariant:
 * 
 * focus_current == focus_stacks[workspace_current - 1]
 * 
 * This should hold whenever we enter or leave the window manager.
 * Call focus_ensure() whenever leaving the window manager (and the
 * invariant may not hold) to update 'focus_current'.
 */

void focus_add(client_t *client, Time timestamp)
{
    client_t *old;

    focus_remove(client, timestamp);
    old = focus_stacks[client->workspace - 1];
    if (old == NULL) {
        debug(("\tsetting focus stack of workspace %d to %s\n",
               client->workspace, client->name));
        client->next_focus = client;
        client->prev_focus = client;
        focus_stacks[client->workspace - 1] = client;
    } else {
        client->next_focus = old;
        client->prev_focus = old->prev_focus;
        old->prev_focus->next_focus = client;
        old->prev_focus = client;
    }
    if (client->workspace == workspace_current) {
        focus_change_current(client, timestamp);
    }
}

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
                debug(("\tsetting focus stack of workspace %d to %s\n",
                       client->workspace, client->next_focus->name));
                focus_stacks[client->workspace - 1] = client->next_focus;
            }
            /* if only client left on workspace, set to NULL */
            if (client->next_focus == client) {
                debug(("setting focus stack of workspace %d to null\n",
                       client->workspace));
                focus_stacks[client->workspace - 1] = NULL;
                client->next_focus = NULL;
                client->prev_focus = NULL;
            }
            /* if removed was focused window, refocus now */
            if (client == focus_current) {
                focus_change_current(client->next_focus, timestamp);
            }
            return;
        }
        stack = stack->next_focus;
    } while (stack != orig);
}

static void focus_set_internal(client_t *client, Time timestamp)
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
            debug(("setting focus stack of workspace %d to %s\n",
                   client->workspace, client->name));
            focus_stacks[client->workspace - 1] = client;
            if (client->workspace == workspace_current) {
                focus_change_current(client, timestamp);
            }
            return;
        }
        p = p->next_focus;
    } while (p != focus_stacks[client->workspace - 1]);
    fprintf(stderr, "XWM: client not found on focus list, shouldn't happen\n");
}

void focus_set(client_t *client, Time timestamp)
{
    client_t *old = focus_current;
    
    focus_set_internal(client, timestamp);
    if (old != NULL && client != NULL)
        permute(old, client);
}

void focus_ensure(Time timestamp)
{
    if (focus_current == NULL) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        return;
    }

    debug(("\tSetting focus to 0x%08X (%s)...\n",
           (unsigned int)focus_current->window, focus_current->name));

    /* see ICCCM 4.1.7 */
    if (focus_current->xwmh != NULL &&
        focus_current->xwmh->flags & InputHint &&
        focus_current->xwmh->input == False) {
        XSetInputFocus(dpy, root_window, RevertToPointerRoot, CurrentTime);
        debug(("\tdoesn't want focus\n"));
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
            debug(("\tglobally active focus\n"));
        }
    } else {
        debug(("\twants focus\n"));
        if (focus_current->protocols & PROTO_TAKE_FOCUS) {
            debug(("\twill forcibly take focus\n"));
            client_sendmessage(focus_current, WM_TAKE_FOCUS,
                               timestamp, 0, 0, 0);
        }
        XSetInputFocus(dpy, focus_current->window,
                       RevertToPointerRoot, timestamp);
    }
    XSync(dpy, False);
    XFlush(dpy);

    client_raise(focus_current);
}

#ifdef DEBUG
void dump_focus_list()
{
    client_t *client, *orig;

    orig = client = focus_current;

    printf("STACK: ");
    do {
        printf("%s ", client->name);
        client = client->next_focus;
    } while (client != orig);
    printf("\n");
}
#else
#define dump_focus_list() /* */
#endif

void focus_alt_tab(XEvent *xevent, void *v)
{
    client_t *orig_focus;
    unsigned int action_keycode;
    KeyCode keycode_Alt_L, keycode_Alt_R;

    orig_focus = focus_current;
    debug(("\torig_focus = '%s'\n", orig_focus ? orig_focus->name : "NULL"));
    dump_focus_list();
    action_keycode = xevent->xkey.keycode;
    keycode_Alt_L = XKeysymToKeycode(dpy, XK_Alt_L);
    keycode_Alt_R = XKeysymToKeycode(dpy, XK_Alt_R);

    XGrabKeyboard(dpy, root_window, True, GrabModeAsync,
                  GrabModeAsync, event_timestamp);
    for (;;) {
        switch (xevent->type) {
            case KeyPress:
                if (xevent->xkey.keycode == action_keycode) {
                    if (xevent->xkey.state & ShiftMask) {
                        focus_set_internal(focus_current->prev_focus,
                                           event_timestamp);
                    } else {
                        focus_set_internal(focus_current->next_focus,
                                           event_timestamp);
                    }
                }
                break;
            case KeyRelease:
                if (xevent->xkey.keycode == keycode_Alt_L
                    || xevent->xkey.keycode == keycode_Alt_R) {
                    /* user let go of all modifiers */
                    debug(("\tfocus_current = '%s'\n",
                           focus_current ? focus_current->name : "NULL"));
                    if (focus_current != NULL) {
                        permute(orig_focus, focus_current);
                    }
                    XUngrabKeyboard(dpy, event_timestamp);
                    debug(("RETURNING FROM ALT-TAB\n"));
                    dump_focus_list();
                    return;
                }
                break;
            default:
                event_dispatch(xevent);
        }
        event_get(ConnectionNumber(dpy), xevent);
    }
}

static void focus_change_current(client_t *new, Time timestamp)
{
    client_t *old;

    old = focus_current;
    focus_current = new;
    client_paint_titlebar(old);
    client_paint_titlebar(new);
    focus_ensure(timestamp);
}

/*
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
