/*
 * This stuff is speed-critical, so we pull all the stops.  The most
 * unfortunate thing is the latency involved with the gettimeofday
 * system call - don't see any way around it that preserves the
 * correct functionality.
 * 
 * If we are using lots of timers (like hundreds or thousands) than it
 * pays to keep the list of active timers somehow sorted so we don't
 * have to go through all of them on timer_run.  I think using a heap
 * would work out best for this.  However, we don't expect to have
 * more than four or five timers, so using a simple array is a win.
 */

/*
 * FIXME: keep variable which is last used index, don't search through
 * all of array on timer_run
 */

#include "timer.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

/* max number of active timers
 * could also grow array dynamically, but no need for that in AHWM */
#define NTIMERS 8

struct _timer_t {
    enum { ACTIVE, FREE } state;
    struct timeval tv;
    timer_fn fn;
    void *arg;
};

static int timeval_compare(struct timeval *tv1, struct timeval *tv2);
static void timeval_normalize(struct timeval *tv);

static timer_t array[NTIMERS];

/* small optimization, avoid walking list if timer_run calls timer_new */
static int first_free = -1;

/* small optimization, avoid walking list in timer_next_time */
static int minimum = -1;

void timer_init()
{
    int i;
    for (i = 0; i < NTIMERS; i++) {
        array[i].state = FREE;
    }
}

timer_t *timer_new(int msecs, timer_fn fn, void *arg)
{
    int i;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    if (first_free != -1) {
        i = first_free;
        first_free = -1;
    } else {
        for (i = 0; i < NTIMERS; i++) {
            if (array[i].state == FREE) {
                break;
            }
        }
    }
    if (i == NTIMERS) {
        return NULL;
    } else {
        array[i].state = ACTIVE;
        array[i].fn = fn;
        array[i].arg = arg;
        array[i].tv = tv;
        tv.tv_usec += msecs * 1000;
        timeval_normalize(&tv);
        array[i].tv = tv;
        if (minimum == -1) {
            minimum = i;
        } else if (timeval_compare(&array[minimum].tv, &array[i].tv) > 0) {
            minimum = i;
        }
        return &(array[i]);
    }
}

#if 0
/* FIXME, find minimum, first_free */
void timer_cancel(timer_t *timer)
{
    timer->state = FREE;
    /* FIXME: can also calculate array index, set first_free */
}
#endif

/* assumes both members are positive */
static void timeval_normalize(struct timeval *tv)
{
    while (tv->tv_usec > 1000000) {
        tv->tv_sec++;
        tv->tv_usec -= 1000000;
    }
}

/* -1: tv1 later; 0: tv1 = tv2; 1: tv2 later */
/* assumes both args have been "normalized" */
/* FIXME: examine assembly, perhaps can change semantics to remove
 * some comparisons */
static int timeval_compare(struct timeval *tv1, struct timeval *tv2)
{
    if (tv1->tv_sec == tv2->tv_sec) {
        if (tv1->tv_usec > tv2->tv_usec) {
            return -1;
        } else {
            return tv1->tv_usec == tv2->tv_usec ? 0 : 1;
        }
    } else {
        return tv1->tv_sec > tv2->tv_sec ? -1 : 1;
    }
}

/* this is also bogus, keep track of next time */
/* FIXME: this should return difference with gettimeofday() */
/* FIXME: timer_run and timer_next_time will many times be run
 * together, reduce number of calls to gettimeofday() */
int timer_next_time(struct timeval *tv)
{
    struct timeval now;

    gettimeofday(&now);
#if 0
    min = -1;
    for (i = 0; i < NTIMERS; i++) {
        if (array[i].state == ACTIVE) {
            if (min == -1) {
                min = i;
            } else {
                if (timeval_compare(&array[min].tv, &array[i].tv) > 0) {
                    min = i;
                }
            }
        }
    }
#endif
    if (minimum == -1) return 0;
    tv->tv_sec = array[minimum].tv.tv_sec - now.tv_sec;
    if (now.tv_usec > array[minimum].tv.tv_usec) {
        now.tv_usec -= 1000000;
        tv->tv_sec++;
    }
    tv->tv_usec = array[minimum].tv.tv_usec - now.tv_usec;
    return 1;
}

void timer_run()
{
    int i;
    struct timeval now;

    gettimeofday(&now, NULL);
//    timeval_normalize(&now);
    for (i = 0; i < NTIMERS; i++) {
        if (array[i].state == ACTIVE) {
            if (timeval_compare(&now, &array[i].tv) <= 0) {
                array[i].state = FREE;
                first_free = i;
                (array[i].fn)(array[i].arg);
            } else if (minimum == -1) {
                minimum = i;
            } else if (timeval_compare(&array[minimum].tv, &array[i].tv) > 0) {
                minimum = i;
            }
        }
    }
}
