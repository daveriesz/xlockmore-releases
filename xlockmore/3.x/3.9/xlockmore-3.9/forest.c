
#ifndef lint
static char sccsid[] = "@(#)forest.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * forest.c - forest for xlockmore
 *
 * Copyright (c) 1995 Pascal Pensa <pensa@aurora.unice.fr>
 *
 * Original idea : Guillaume Ramey <ramey@aurora.unice.fr>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "xlock.h"
#include <math.h>

#define MINHEIGHT  20		/* Tree height range */
#define MAXHEIGHT  40

#define MINANGLE   15		/* (degree) angle between soon */
#define MAXANGLE   35
#define RANDANGLE  15		/* (degree) Max random angle from default */

#define REDUCE     90		/* Height % from father */

#define ITERLEVEL  10		/* Tree iteration */

#define COLORSPEED  2		/* Color increment */

/* degree to radian */
#define DEGTORAD(x) (((float)(x)) * 0.017453293)

#define PIONTWO 1.5707963

#define MAX_RAND(max) (LRAND() % (max))
#define RANGE_RAND(min,max) ((min) + LRAND() % ((max) - (min)))

ModeSpecOpt forest_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         width;
	int         height;
	int         time;	/* up time */
} foreststruct;

static foreststruct forests[MAXSCREENS];

static void
draw_tree(ModeInfo * mi, short int x, short int y, short int len, float a, float as, short int c, short int level)
				/* Father's end */
				/* Length */
				/* color */
				/* Height level */
				/* Father's angle */
				/* Father's angle step */
{
	Window      win = MI_WINDOW(mi);
	short       x_1, y_1, x_2, y_2;
	float       a1, a2;

	/* left */

	a1 = a + as + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);

	x_1 = x + (short) (COSF(a1) * ((float) len));
	y_1 = y + (short) (SINF(a1) * ((float) len));

	/* right */

	a2 = a - as + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);

	x_2 = x + (short) (COSF(a2) * ((float) len));
	y_2 = y + (short) (SINF(a2) * ((float) len));

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[c]);
		c = (c + COLORSPEED) % Scr[screen].npixels;
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

	XDrawLine(dsp, win, Scr[screen].gc, x, y, x_1, y_1);
	XDrawLine(dsp, win, Scr[screen].gc, x, y, x_2, y_2);

	if (level < 2) {
		XDrawLine(dsp, win, Scr[screen].gc, x + 1, y, x_1 + 1, y_1);
		XDrawLine(dsp, win, Scr[screen].gc, x + 1, y, x_2 + 1, y_2);
	}
	len = (len * REDUCE * 10) / 1000;

	if (level < ITERLEVEL) {
		draw_tree(mi, x_1, y_1, len, a1, as, c, level + 1);
		draw_tree(mi, x_2, y_2, len, a2, as, c, level + 1);
	}
}

void
init_forest(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	foreststruct *fp = &forests[screen];

	fp->width = MI_WIN_WIDTH(mi);
	fp->height = MI_WIN_HEIGHT(mi);
	fp->time = 0;

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, fp->width, fp->height);
}

void
draw_forest(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	short       x, y, x_2, y_2, len, c = 0;
	float       a, as;
	foreststruct *fp = &forests[screen];

	if (fp->time < MI_BATCHCOUNT(mi)) {

		x = RANGE_RAND(0, fp->width);
		y = RANGE_RAND(0, fp->height + MAXHEIGHT);
		a = -PIONTWO + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);
		as = DEGTORAD(RANGE_RAND(MINANGLE, MAXANGLE));
		len = ((RANGE_RAND(MINHEIGHT, MAXHEIGHT) * (fp->width / 20)) / 50) + 2;

		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
			c = MAX_RAND(Scr[screen].npixels);
			XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[c]);
			c = (c + COLORSPEED) % Scr[screen].npixels;
		} else
			XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

		x_2 = x + (short) (COSF(a) * ((float) len));
		y_2 = y + (short) (SINF(a) * ((float) len));

		XDrawLine(dsp, win, Scr[screen].gc, x, y, x_2, y_2);
		XDrawLine(dsp, win, Scr[screen].gc, x + 1, y, x_2 + 1, y_2);

		draw_tree(mi, x_2, y_2, (len * REDUCE) / 100, a, as, c, 1);
	}
	if (++fp->time > MI_CYCLES(mi))
		init_forest(mi);
}
