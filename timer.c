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
 * This stuff is speed-critical, so we pull all the stops.  The
 * latency associated with a gettimeofday() system call is killer, so
 * we avoid those like a bad cliché.  We keep the timers sorted using
 * a heap.  This is the only place where we use a heap, so we don't
 * abstract out the priority queue data type.
 */

#include "timer.h"

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* we use a standard heap */
#define RCHILD(i) (2*(i)+1)
#define LCHILD(i) (2*(i)+2)
#define PARENT(i) (((i)-1)/2)

struct _timer {
    enum { DONE, ACTIVE } state;
    int index;
    struct timeval tv;
    timer_fn fn;
    void *arg;
};

static timer **timers;

static int nused, nallocated;

static int timeval_compare(struct timeval *tv1, struct timeval *tv2);
static void timeval_normalize(struct timeval *tv);
static void timer_run_with_tv(struct timeval *now);
static void remove_timer(timer *t);

void timer_init()
{
    /* start out with eight timers */
    timers = malloc(sizeof(timer *) * 1);
    if (timers == NULL) {
        perror("malloc");
        exit(1);
    }
    nallocated = 1;
}

int timer_pending(timer *t)
{
    return t->state == ACTIVE ? 1 : 0;
}

timer *timer_new(int msecs, timer_fn fn, void *arg)
{
    timer *t, *tmp, **tmp2;
    struct timeval now;
    int i;

    t = malloc(sizeof(timer));
    if (t == NULL) {
        return NULL;
    }
    /* enlarge array if needed (we never shrink it again) */
    if (nused == nallocated) {
        tmp2 = realloc(timers, sizeof(timer *) * nallocated * 2);
        if (tmp2 == NULL) {
            free(t);
            return NULL;
        }
        timers = tmp2;
        nallocated *= 2;
    }

    gettimeofday(&now, NULL);
    t->state = ACTIVE;
    t->fn = fn;
    t->arg = arg;
    t->tv.tv_sec = now.tv_sec;
    t->tv.tv_usec = now.tv_usec + msecs * 1000;
    timeval_normalize(&t->tv);

    /* put element in last position of complete tree */
    timers[nused] = t;
    t->index = nused++;

    /* if this is the only element, it's in correct position */
    if (nused == 1) {
        return t;
    }
    
    /* sift up */
    i = nused - 1;
    while (timeval_compare(&timers[PARENT(i)]->tv, &timers[i]->tv) < 0) {
        /* swap */
        tmp = timers[i];
        timers[i] = timers[PARENT(i)];
        timers[PARENT(i)] = tmp;
        timers[i]->index = i;
        timers[PARENT(i)]->index = PARENT(i);

        i = PARENT(i);
    }
    return t;
}

/*
 * Remove an element of the heap, not necessarily the top one.
 * 
 * Swapping with the last element and sifting down works with all
 * elements of the heap, not only the top one.
 */

static void remove_timer(timer *t)
{
    int i, smallest;
    timer *tmp;

    if (t->state == DONE) {
        return;
    }
    i = t->index;
    t->state = DONE;
    tmp = t;
    timers[i] = timers[nused - 1];
    timers[nused - 1] = tmp;
    timers[i]->index = i;
    timers[nused - 1]->index = nused - 1;
    nused--;

    for (;;) {
        /* find the smallest child */
        smallest = LCHILD(i);
        if (smallest >= nused) {
            break;
        }
        if (RCHILD(i) < nused &&
            timeval_compare(&timers[RCHILD(i)]->tv,
                            &timers[smallest]->tv) > 0) {
            smallest = RCHILD(i);
        }
        /* sift down if needed */
        if (timeval_compare(&timers[i]->tv, &timers[smallest]->tv) < 0) {
            tmp = timers[i];
            timers[i] = timers[smallest];
            timers[smallest] = tmp;
            timers[i]->index = i;
            timers[smallest]->index = smallest;
            
            i = smallest;
        } else {
            break;
        }
    }
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

void timer_cancel(timer *t)
{
    remove_timer(t);
    free(t);
}

int timer_next_time(struct timeval *tv)
{
    struct timeval now, next;
    
    gettimeofday(&now, NULL);
    timer_run_with_tv(&now);

    if (nused == 0) {
        return 0;
    }
    next = timers[0]->tv;
    tv->tv_sec = next.tv_sec - now.tv_sec;
    if (next.tv_usec < now.tv_usec) {
        now.tv_usec -= 1000000;
        tv->tv_sec--;
    }
    tv->tv_usec = next.tv_usec - now.tv_usec;
    return 1;
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

    if (nused > 0) {
        now = timers[0]->tv;
        timer_run_with_tv(&now);
    }
}

static void timer_run_with_tv(struct timeval *now)
{
    timer *t;
    
    while (nused > 0 && timeval_compare(now, &timers[0]->tv) <= 0) {
        /* must remove before calling user function - user function
         * might call timer_cancel, which calls free() */
        t = timers[0];
        remove_timer(t);
        (t->fn)(t, t->arg);
    }
}
