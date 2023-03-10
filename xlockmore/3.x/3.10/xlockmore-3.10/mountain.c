#ifndef lint
static char sccsid[] = "@(#)mountain.c	3.10 96/07/20 xlockmore";

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

#define RANGE_RAND(min,max) ((min) + NRAND((max) - (min)))

ModeSpecOpt mountain_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         pixelmode;
	int         width;
	int         height;
	int         x, y;
	int         offset;
	int         stage;
	int         h[WORLDWIDTH][WORLDWIDTH];
	int         time;	/* up time */
	int         first;
	GC          stippled_GC;
} mountainstruct;

static mountainstruct *mountains = NULL;

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

static void
drawamountain(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	mountainstruct *mp = &mountains[MI_SCREEN(mi)];
	int         x2, y2, x3, y3, y4, y5, c = 0;
	XPoint      p[5];

	if (MI_NPIXELS(mi) > 2) {
		c = (mp->h[mp->x][mp->y] + mp->h[mp->x + 1][mp->y] +
		  mp->h[mp->x][mp->y + 1] + mp->h[mp->x + 1][mp->y + 1]) / 4;
		c = (c / 10 + mp->offset) % MI_NPIXELS(mi);
	}
	x2 = mp->x * (2 * mp->width) / (3 * WORLDWIDTH);
	y2 = mp->y * (2 * mp->height) / (3 * WORLDWIDTH);
	p[0].x = (x2 - y2 / 2) + (mp->width / 4);
	p[0].y = (y2 - mp->h[mp->x][mp->y]) + mp->height / 4;

	x3 = (mp->x + 1) * (2 * mp->width) / (3 * WORLDWIDTH);
	y3 = mp->y * (2 * mp->height) / (3 * WORLDWIDTH);
	p[1].x = (x3 - y3 / 2) + (mp->width / 4);
	p[1].y = (y3 - mp->h[mp->x + 1][mp->y]) + mp->height / 4;

	y4 = (mp->y + 1) * (2 * mp->height) / (3 * WORLDWIDTH);
	p[2].x = (x3 - y4 / 2) + (mp->width / 4);
	p[2].y = (y4 - mp->h[mp->x + 1][mp->y + 1]) + mp->height / 4;

	y5 = (mp->y + 1) * (2 * mp->height) / (3 * WORLDWIDTH);
	p[3].x = (x2 - y5 / 2) + (mp->width / 4);
	p[3].y = (y5 - mp->h[mp->x][mp->y + 1]) + mp->height / 4;

	p[4].x = p[0].x;
	p[4].y = p[0].y;

	if (MI_NPIXELS(mi) > 2)
		XSetForeground(display, gc, MI_PIXEL(mi, c));
	else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	XFillPolygon(display, window, gc, p, 4, Complex, CoordModeOrigin);

	if (!mp->pixelmode) {
		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
		XDrawLines(display, window, gc, p, 5, CoordModeOrigin);
	}
	mp->x++;
	if (mp->x == WORLDWIDTH - 1) {
		mp->y++;
		mp->x = 0;
	}
	if (mp->y == WORLDWIDTH - 1)
		mp->stage++;
}

void
init_mountain(ModeInfo * mi)
{
	mountainstruct *mp;
	int         i, j, x, y;
	XGCValues   gcv;

	if (mountains == NULL) {
		if ((mountains = (mountainstruct *) calloc(MI_NUM_SCREENS(mi),
					   sizeof (mountainstruct))) == NULL)
			return;
	}
	mp = &mountains[MI_SCREEN(mi)];
	mp->width = MI_WIN_WIDTH(mi);
	mp->height = MI_WIN_HEIGHT(mi);
	mp->pixelmode = (mp->width + mp->height < 200);
	mp->stage = 0;
	mp->time = 0;
	mp->x = mp->y = 0;
	if (!mp->first) {
		mp->first = 1;
		gcv.foreground = MI_WIN_WHITE_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);

		mp->stippled_GC = XCreateGC(MI_DISPLAY(mi), MI_WINDOW(mi),
					  GCForeground | GCBackground, &gcv);
	}
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++)
			mp->h[x][y] = 0;

	j = MI_BATCHCOUNT(mi);
	if (j < 0)
		j = NRAND(-j) + 1;
	for (i = 0; i < j; i++)
		mp->h[RANGE_RAND(1, WORLDWIDTH - 1)][RANGE_RAND(1, WORLDWIDTH - 1)] =
			NRAND(MAXHEIGHT);

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++)
			spread(mp->h, x, y);

	for (y = 0; y < WORLDWIDTH; y++)
		for (x = 0; x < WORLDWIDTH; x++) {
			mp->h[x][y] = mp->h[x][y] + NRAND(10) - 5;
			if (mp->h[x][y] < 10)
				mp->h[x][y] = 0;
		}

	if (MI_NPIXELS(mi) > 2)
		mp->offset = NRAND(MI_NPIXELS(mi));
	else
		mp->offset = 0;
}

void
draw_mountain(ModeInfo * mi)
{
	mountainstruct *mp = &mountains[MI_SCREEN(mi)];

	switch (mp->stage) {
		case 0:
			drawamountain(mi);
			break;
		case 1:
			MI_PAUSE(mi) = 1000000;
			/*if (++mp->time > MI_CYCLES(mi)); */
			mp->stage++;
			break;
		case 2:
			init_mountain(mi);
			break;
	}
}

void
release_mountain(ModeInfo * mi)
{
	if (mountains != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			mountainstruct *mp = &mountains[screen];

			XFreeGC(MI_DISPLAY(mi), mp->stippled_GC);
		}
		(void) free((void *) mountains);
		mountains = NULL;
	}
}
