/*
 * Provides fine-grained timers.  This allows you to say "call this
 * function in fifty milliseconds."  Needs to be integrated with your
 * main event loop.  This should be plenty fast for a few timers; if
 * you want lots of timers, I have a different implementation with the
 * same interface.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

/*
 * opaque type
 */

struct _timer_t;
typedef struct _timer_t timer_t;

/*
 * Functions to be called on timer trigger
 */

typedef void (*timer_fn)(void *);

/*
 * init module, no dependencies
 */

void timer_init();

/*
 * Create a new timer.  FN will be called with ARG in MSECS
 * milliseconds.  Resolution depends on how often timer_run() is
 * called.  Returns opaque type; can't do much with the return value
 * except timer_cancel().  Timers are not recurring (eg, call
 * timer_new in your function if you want it to be called again,
 * module will deal with this efficiently).  Returns NULL if out of
 * memory.
 */

timer_t *timer_new(int msecs, timer_fn fn, void *arg);

#if 0
/*
 * Cancel a timer before it shoots.
 */

void timer_cancel(timer_t *timer);
#endif

/*
 * Copy into TV the next time a timer will shoot.  The value placed
 * into TV is suitable for use with select(2).  Returns 1 if copied in
 * a value; returns 0 if there are no pending timers.
 */

int timer_next_time(struct timeval *tv);

/*
 * Run timers
 */

void timer_run();
