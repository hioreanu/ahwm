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
 * Currently, this module simply paints the titlebar and caches color
 * combinations.  Each client has eight colors for the titlebar:
 * normal, lowlight, hilight, focused, focused-lolight,
 * focused-hilight, text and focused-text.  Most clients will have
 * similar colors, so we group these together to save memory (only
 * keep an integer index in the client structure).  Eventually, we
 * might implement gradients and this will serve nicely as an
 * interface to gradient caching.
 * 
 * I also thought about implementing titlebar colors something like this:
 * 
 * TitlebarColor = "#C0C0C0";
 * IsFocused True {
 *     TitlebarColor = "#A0A0A0";
 * }
 * 
 * However, there are no other options that I could see as useful to
 * be different in a focused and unfocused window, so the IsFocused
 * keyword would only serve one purpose.  This is cleaner.
 */

#ifndef PAINT_H
#define PAINT_H

#include "config.h"

#include "client.h"

/*
 * The largest "ascent" member from the font.  Set by xwm.c.
 */

extern int paint_ascent;

/*
 * Initialize the painting module, no dependencies
 */

void paint_init();

/*
 * This sets client->color_index depending on the colors specified.
 * The string arguments are the X color specs given by the user.
 */

void paint_calculate_colors(client_t *client, char *normal, char *focused,
                            char *text, char *focused_text);
/*
 * Adds a button to be displayed on the titlebar.  NB, you also need
 * to use keyboard_add_button() to make the button work.  IMAGE is one
 * of "box" "down" "topbar" "up" "wins" "x".  If LEFT is true, the
 * button is added to the left; if false, the button is added to the
 * left.  The button positions are determined by the order in which
 * this function is called.
 */

void paint_add_button(char *image, Bool left);

/*
 * Paints the titlebar.  All of it.  At once.
 * Also adds a 3-D hilights if user specified colors for the titlebar.
 */

void paint_titlebar(client_t *client);

#endif /* PAINT_H */
