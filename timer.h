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

#ifndef TIMER_H
#define TIMER_H

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

/*
 * Copy into TV the next time a timer will shoot.  The value placed
 * into TV is suitable for use with select(2).  Returns 1 if copied in
 * a value; returns 0 if there are no pending timers.  Value placed in
 * TV will always be positive; this function will call timer_run() if
 * necessary to make TV positive.
 */

int timer_next_time(struct timeval *tv);

/*
 * Run timers
 */

void timer_run();

/*
 * Like timer_run(), but avoids calling gettimeofday(); this is meant
 * to be called after select() times out with a period returned from
 * timer_next_time().  This assumes the current time is equal to what
 * timer_next_time uses.
 */

void timer_run_first();

#endif /* TIMER_H */
