#ifndef lint
static char sccsid[] = "@(#)mountain.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * mountain.c - mountain for xlockmore
 *
 * Copyright (c) 1995 Pascal Pensa <pensa@aurora.unice.fr>
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

/* ~ 5000 Max mountain height (1000 - 10000) */
#define MAXHEIGHT  (3 * (mp->width + mp->height))

#define WORLDWIDTH 50		/* World size x * y */

#define MAX_RAND(max) (LRAND() % (max))
#define RANGE_RAND(min,max) ((min) + LRAND() % ((max) - (min)))

ModeSpecOpt mountain_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         pixelmode;
	int         width;
	int         height;
	int         h[WORLDWIDTH][WORLDWIDTH];
	int         time;	/* up time */
	int         first;
	GC          stippled_GC;
} mountainstruct;

static mountainstruct mountains[MAXSCREENS];

static void
spread(int  (*m)[50], int x, int y)
{
	int         x2, y2;
	int         h = m[x][y];

	for (y2 = y - 1; y2 <= y + 1; y2++)
		for (x2 = x - 1; x2 <= x + 1; x2++)
			if (x2 >= 0 && y2 >= 0 && x2 < WORLDWIDTH && y2 < WORLDWIDTH)
				m[x2][y2] = (m[x2][y2] + h) / 2;
}

void
init_mountain(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i, x, y, offset = 0;
	XGCValues   gcv;
	mountainstruct *mp = &mountains[screen];

	mp->width = MI_WIN_WIDTH(mi);
	mp->height = MI_WIN_HEIGHT(mi);
	mp->pixelmode = (mp->width + mp->height < 200);

	mp->time = 0;

	if (!mp->first) {
		Screen     *scr;

		mp->first = 1;
		scr = ScreenOfDisplay(dsp, screen);
		gcv.foreground = WhitePixelOfScreen(scr);
		gcv.background = BlackPixelOfScreen(scr);

		mp->stippled_GC = XCreateGC(dsp, win,
					  GCForeground | GCBackground, &gcv);
	}
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, mp->width, mp->height);

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++)
			mp->h[x][y] = 0;

	for (i = 0; i < MI_BATCHCOUNT(mi); i++)
		mp->h[RANGE_RAND(1, WORLDWIDTH - 1)][RANGE_RAND(1, WORLDWIDTH - 1)] =
			MAX_RAND(MAXHEIGHT);

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++)
			spread(mp->h, x, y);

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++) {
			mp->h[x][y] = mp->h[x][y] + MAX_RAND(10) - 5;
			if (mp->h[x][y] < 10)
				mp->h[x][y] = 0;
		}

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		offset = MAX_RAND(Scr[screen].npixels);

	for (y = 0; y < WORLDWIDTH - 1; y++)
		for (x = 0; x < WORLDWIDTH - 1; x++) {
			int         x2, y2, x3, y3, y4, y5, c = 0;
			XPoint      p[5];

			if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
				c = (mp->h[x][y] + mp->h[x + 1][y] + mp->h[x][y + 1] + mp->h[x + 1][y + 1]) / 4;
				c = (c / 10 + offset) % Scr[screen].npixels;
				if (c >= Scr[screen].npixels)
					c = Scr[screen].npixels - 1;
			}
			x2 = x * (2 * mp->width) / (3 * WORLDWIDTH);
			y2 = y * (2 * mp->height) / (3 * WORLDWIDTH);
			p[0].x = (x2 - y2 / 2) + (mp->width / 4);
			p[0].y = (y2 - mp->h[x][y]) + mp->height / 4;

			x3 = (x + 1) * (2 * mp->width) / (3 * WORLDWIDTH);
			y3 = y * (2 * mp->height) / (3 * WORLDWIDTH);
			p[1].x = (x3 - y3 / 2) + (mp->width / 4);
			p[1].y = (y3 - mp->h[x + 1][y]) + mp->height / 4;

			y4 = (y + 1) * (2 * mp->height) / (3 * WORLDWIDTH);
			p[2].x = (x3 - y4 / 2) + (mp->width / 4);
			p[2].y = (y4 - mp->h[x + 1][y + 1]) + mp->height / 4;

			y5 = (y + 1) * (2 * mp->height) / (3 * WORLDWIDTH);
			p[3].x = (x2 - y5 / 2) + (mp->width / 4);
			p[3].y = (y5 - mp->h[x][y + 1]) + mp->height / 4;

			p[4].x = p[0].x;
			p[4].y = p[0].y;

			if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
				XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[c]);
			else
				XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
			XFillPolygon(dsp, win, Scr[screen].gc, p, 4, Complex, CoordModeOrigin);

			if (!mp->pixelmode) {
				XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
				XDrawLines(dsp, win, Scr[screen].gc, p, 5, CoordModeOrigin);
			}
		}
}

void
draw_mountain(ModeInfo * mi)
{
	mountainstruct *mp = &mountains[screen];

	if (++mp->time > MI_CYCLES(mi))
		init_mountain(mi);
}
