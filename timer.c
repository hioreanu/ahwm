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
static void timer_run_with_tv(struct timeval *now);

static timer_t array[NTIMERS];

/* small optimization, avoid walking list if timer_run calls timer_new */
static int first_free = -1;

/* small optimization, avoid walking list in timer_next_time */
static int minimum = -1;

static int in_timer_run = 0;

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

    if (first_free != -1) {
        i = first_free;
        first_free = -1;
        if (in_timer_run == 0) {
            gettimeofday(&tv, NULL);
            array[i].tv = tv;
        }
        /* else leave array[i].tv as-is, save a call to gettimeofday() */
    } else {
        for (i = 0; i < NTIMERS; i++) {
            if (array[i].state == FREE) {
                break;
            }
        }
        if (i == NTIMERS) {
            return NULL;
        }
        gettimeofday(&tv, NULL);
        array[i].tv = tv;
    }
    array[i].state = ACTIVE;
    array[i].fn = fn;
    array[i].arg = arg;
    array[i].tv.tv_usec += msecs * 1000;
    timeval_normalize(&array[i].tv);
    if (minimum == -1 ||
        timeval_compare(&array[minimum].tv, &array[i].tv) > 0) {
            
        minimum = i;
    }
    return &(array[i]);
}

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

int timer_next_time(struct timeval *tv)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    timer_run_with_tv(&now);
    if (minimum == -1) return 0;
    tv->tv_sec = array[minimum].tv.tv_sec - now.tv_sec;
    if (now.tv_usec > array[minimum].tv.tv_usec) {
        now.tv_usec -= 1000000;
        tv->tv_sec++;
    }
    tv->tv_usec = array[minimum].tv.tv_usec - now.tv_usec;
    return 1;
}

void timer_cancel(timer_t *timer)
{
    int i;

    timer->state = FREE;
    if (timer - array == minimum) {
        minimum = -1;
        for (i = 0; i < NTIMERS; i++) {
            if (array[i].state == ACTIVE) {
                if (minimum == -1 ||
                    timeval_compare(&array[minimum].tv, &array[i].tv) > 0) {
                    
                    minimum = i;
                }
            }
        }
    }
}

/* small optimization, don't call gettimeofday twice if possible */
static void timer_run_with_tv(struct timeval *now)
{
    int i, prev_min;

    in_timer_run = 1;
    prev_min = -1;
    for (i = 0; i < NTIMERS; i++) {
        if (array[i].state == ACTIVE) {
            if (timeval_compare(now, &array[i].tv) <= 0) {
                array[i].state = FREE;
                array[i].tv = *now; /* works with first_free, see timer_new() */
                first_free = i;
                (array[i].fn)(array[i].arg);
                if (i == minimum) {
                    minimum = prev_min;
                }
            } else if (minimum == -1 ||
                       timeval_compare(&array[minimum].tv, &array[i].tv) > 0) {
                prev_min = minimum;
                minimum = i;
            }
        }
    }
    in_timer_run = 0;
}

void timer_run()
{
    struct timeval now;

    gettimeofday(&now, NULL);
    timer_run_with_tv(&now);
}

void timer_run_first()
{
    struct timeval now;

    now = array[minimum].tv;
    timer_run_with_tv(&now);
}
