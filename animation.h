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
 * Calls a callback function at a certain frequency, providing a
 * cartoonish accelerate/decelerate effect useful for shading,
 * automated moving, etc.
 */

#ifndef ANIMATION_H
#define ANIMATION_H

struct _animation;
typedef struct _animation animation;

/*
 * Callback functions take two parameters:  a data member which you
 * define when you start the animation and a float which represents
 * how much of the animation has completed thus far (0.0 at first,
 * 1.0 at the last step).
 */
typedef void (*callback_fn)(float, void *);

/*
 * The finalize function is called after calling the callback for the
 * last time.
 */
typedef void (*finalize_fn)(void *);

/*
 * start an animation and return an opaque object (which is useful for
 * reversing the animation).
 */
animation *animate(callback_fn callback, finalize_fn finalize, void *v);

/*
 * Reverse sets the animation to go backwards, retracing the last step.
 * If the animation has already completed n steps, it will do those n steps
 * again, but backwards
 */
void animation_reverse(animation *);

/*
 * start_backwards is like reverse, except instead of retracing the
 * last n steps, it starts retracing from the last minus nth step.
 */
void animation_start_backwards(animation *);

#endif /* ANIMATION_H */
