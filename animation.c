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

#include "animation.h"
#include "timer.h"

#include <stdlib.h>

#define ANIMATION_INTERVAL 8  /* milliseconds */

struct _animation {
    int i;
    void *v;
    callback_fn callback;
    finalize_fn finalize;
    int forwards;
};

/*
 * These numbers are from a table of the normal probability
 * distribution function.  Source:  An Introduction to Probability
 * Theory and its Applications vol. 1, William Feller, 1950.  The last
 * element should be 1.0.  We approximate that Phi(3.2) = 1.0.
 * 
 * This gives a very pleasing aesthetic effect.  With these
 * parameters, it takes around half a second to complete an animation.
 */

float animation_multiplicands[65] = {
    0.000000,
    0.000968, 0.001350, 0.001866, 0.002555, 0.003467, 0.004661, 0.006210,
    0.008198, 0.010724, 0.013903, 0.017864, 0.022750, 0.028717, 0.035930,
    0.044565, 0.054799, 0.066807, 0.080757, 0.096800, 0.115070, 0.135666,
    0.158655, 0.184060, 0.211855, 0.241964, 0.274253, 0.308538, 0.344578,
    0.382089, 0.420740, 0.460172, 0.500000, 0.539828, 0.579260, 0.617911,
    0.655422, 0.691462, 0.725747, 0.758036, 0.788145, 0.815940, 0.841345,
    0.864334, 0.884930, 0.903200, 0.919243, 0.933193, 0.945201, 0.955435,
    0.964070, 0.971283, 0.977250, 0.982136, 0.986097, 0.989276, 0.991802,
    0.993790, 0.995339, 0.996533, 0.997445, 0.998134, 0.998650, 0.999032,
    1.000000
};

static void animate_fire(timer *t, void *v);

animation *animate(callback_fn callback, finalize_fn finalize, void *v)
{
    animation *data;

    data = malloc(sizeof(animation));
    if (data == NULL) return NULL;
    data->i = 0;
    data->v = v;
    data->callback = callback;
    data->finalize = finalize;
    data->forwards = 1;
    timer_new(ANIMATION_INTERVAL, animate_fire, (void *)data);
    return data;
}

void animation_reverse(animation *data)
{
    data->forwards = !data->forwards;
}

void animation_start_backwards(animation *data)
{
    animation_reverse(data);
    data->i = (sizeof(animation_multiplicands) / sizeof(float)) - data->i - 1;
}

static void animate_fire(timer *t, void *v)
{
    animation *data = (animation *)v;

    (data->callback)(animation_multiplicands[data->i], data->v);

    timer_cancel(t);
    if (data->forwards) {
        data->i++;
        if (data->i == sizeof(animation_multiplicands) / sizeof(float)) {
            if (data->finalize != NULL) (data->finalize)(data->v);
            free(data);
        } else {
            timer_new(ANIMATION_INTERVAL, animate_fire, v);
        }
    } else {
        data->i--;
        if (data->i < 0) {
            if (data->finalize != NULL) (data->finalize)(data->v);
            free(data);
        } else {
            timer_new(ANIMATION_INTERVAL, animate_fire, v);
        }
    }
}
