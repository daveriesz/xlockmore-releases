#ifndef lint
static char sccsid[] = "@(#)spline.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * spline.c - spline fun #3 
 *
 * Copyright (c) 1992 by Jef Poskanzer
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 17-Jan-96: added compile time option, FOLLOW to erase old splines like Qix
 *           thanks to Richard Duran <rduran@cs.utep.edu>
 * 9-Mar-95: changed how batchcount is used
 * 2-Sep-93: xlock version: (David Bagley bagleyd@hertz.njit.edu)
 * 1992:     X11 version (Jef Poskanzer jef@netcom.com || jef@well.sf.ca.us)
 */

/*-
 * original copyright
 * xsplinefun.c - X11 version of spline fun #3
 * Displays colorful moving splines in the X11 root window.
 * Copyright (C) 1992 by Jef Poskanzer
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  This software is provided "as is" without express or
 * implied warranty.
 */

#include	"xlock.h"

#define MINPOINTS 3
#define MAXPOINTS 6

/* #define FOLLOW */
#ifdef FOLLOW
#define MAX_DELTA 16
#else
#define MAX_DELTA 3
#endif

#define SPLINE_THRESH 5

ModeSpecOpt spline_opts =
{0, NULL, NULL, NULL};

typedef struct {
#ifdef FOLLOW
	int         x;
	int         y;
} point;

typedef struct {
	int         first;
	int         last;
	point     **splineq;
#endif
	int         width;
	int         height;
	int         color;
	int         points;
	int         nsplines;
	int         x[MAXPOINTS], y[MAXPOINTS], dx[MAXPOINTS], dy[MAXPOINTS];
} splinestruct;

static splinestruct splines[MAXSCREENS];

static void XDrawSpline(Display * display, Drawable d, GC gc, int x0, int y0, int x1, int y1, int x2, int y2);

void
init_spline(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i;
	splinestruct *sp = &splines[screen];

	sp->width = MI_WIN_WIDTH(mi);
	sp->height = MI_WIN_HEIGHT(mi);
	/* batchcount is the upper bound on the number of points */
	sp->points = MI_BATCHCOUNT(mi);
	if (sp->points > MAXPOINTS)
		sp->points = LRAND() % (MAXPOINTS - MINPOINTS + 1) + MINPOINTS;
	/* Absolute minimum */
	if (sp->points < MINPOINTS)
		sp->points = MINPOINTS;
#ifdef FOLLOW
	sp->last = 0;

	sp->nsplines = (MI_CYCLES(mi) + 1) * 2;
	if (!sp->splineq) {
		sp->splineq = (point **) malloc(sp->nsplines * sizeof (point *));
		for (i = 0; i < sp->nsplines; ++i) {
			sp->splineq[i] = (point *) malloc(sp->points * sizeof (point));
			(void) memset((char *) sp->splineq[i], 0,
				      sp->points * sizeof (point));
		}
	}
#else
	sp->nsplines = 0;
#endif

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, sp->width, sp->height);

	/* Initialize points. */
	for (i = 0; i < sp->points; ++i) {
		sp->x[i] = LRAND() % sp->width;
		sp->y[i] = LRAND() % sp->height;
		sp->dx[i] = LRAND() % (MAX_DELTA * 2) - MAX_DELTA;
		if (sp->dx[i] <= 0)
			--sp->dx[i];
		sp->dy[i] = LRAND() % (MAX_DELTA * 2) - MAX_DELTA;
		if (sp->dy[i] <= 0)
			--sp->dy[i];
	}

	sp->color = 0;
}


void
draw_spline(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i, t, px, py, zx, zy, nx, ny;
	splinestruct *sp = &splines[screen];

#ifdef FOLLOW
	sp->first = (sp->last + 2) % sp->nsplines;
#endif

	/* Move the points. */
	for (i = 0; i < sp->points; i++) {
		for (;;) {
			t = sp->x[i] + sp->dx[i];
			if (t >= 0 && t < sp->width)
				break;
			sp->dx[i] = LRAND() % (MAX_DELTA * 2) - MAX_DELTA;
			if (sp->dx[i] <= 0)
				--sp->dx[i];
		}
		sp->x[i] = t;
		for (;;) {
			t = sp->y[i] + sp->dy[i];
			if (t >= 0 && t < sp->height)
				break;
			sp->dy[i] = LRAND() % (MAX_DELTA * 2) - MAX_DELTA;
			if (sp->dy[i] <= 0)
				--sp->dy[i];
		}
		sp->y[i] = t;
	}

#ifdef FOLLOW
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));

	/* Erase first figure. */
	px = zx = (sp->splineq[sp->first][0].x +
		   sp->splineq[sp->first][sp->points - 1].x) / 2;
	py = zy = (sp->splineq[sp->first][0].y +
		   sp->splineq[sp->first][sp->points - 1].y) / 2;
	for (i = 0; i < sp->points - 1; ++i) {
		nx = (sp->splineq[sp->first][i + 1].x +
		      sp->splineq[sp->first][i].x) / 2;
		ny = (sp->splineq[sp->first][i + 1].y +
		      sp->splineq[sp->first][i].y) / 2;
		XDrawSpline(dsp, win, Scr[screen].gc,
			    px, py, sp->splineq[sp->first][i].x,
			    sp->splineq[sp->first][i].y, nx, ny);
		px = nx;
		py = ny;
	}

	XDrawSpline(dsp, win, Scr[screen].gc,
		    px, py, sp->splineq[sp->first][sp->points - 1].x,
		    sp->splineq[sp->first][sp->points - 1].y, zx, zy);

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[sp->color]);
		if (++sp->color >= Scr[screen].npixels)
			sp->color = 0;
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
#endif

	/* Draw the figure. */
	px = zx = (sp->x[0] + sp->x[sp->points - 1]) / 2;
	py = zy = (sp->y[0] + sp->y[sp->points - 1]) / 2;
	for (i = 0; i < sp->points - 1; ++i) {
		nx = (sp->x[i + 1] + sp->x[i]) / 2;
		ny = (sp->y[i + 1] + sp->y[i]) / 2;
		XDrawSpline(dsp, win, Scr[screen].gc,
			    px, py, sp->x[i], sp->y[i], nx, ny);
		px = nx;
		py = ny;
	}
#ifndef FOLLOW
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[sp->color]);
		if (++sp->color >= Scr[screen].npixels)
			sp->color = 0;
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
#endif

	XDrawSpline(dsp, win, Scr[screen].gc,
	       px, py, sp->x[sp->points - 1], sp->y[sp->points - 1], zx, zy);

#ifdef FOLLOW
	for (i = 0; i < sp->points; ++i) {
		sp->splineq[sp->last][i].x = sp->x[i];
		sp->splineq[sp->last][i].y = sp->y[i];
	}
	sp->last++;
	if (sp->last >= sp->nsplines)
		sp->last = 0;
#else
	if (++sp->nsplines > MI_CYCLES(mi))
		init_spline(mi);
#endif
}

/* X spline routine. */

#define ABS(x) ((x) < 0 ? -(x) : (x))

static void
XDrawSpline(Display * display, Drawable d, GC gc, int x0, int y0, int x1, int y1, int x2, int y2)
{
	register int xa, ya, xb, yb, xc, yc, xp, yp;

	xa = (x0 + x1) / 2;
	ya = (y0 + y1) / 2;
	xc = (x1 + x2) / 2;
	yc = (y1 + y2) / 2;
	xb = (xa + xc) / 2;
	yb = (ya + yc) / 2;

	xp = (x0 + xb) / 2;
	yp = (y0 + yb) / 2;
	if (ABS(xa - xp) + ABS(ya - yp) > SPLINE_THRESH)
		XDrawSpline(display, d, gc, x0, y0, xa, ya, xb, yb);
	else
		XDrawLine(display, d, gc, x0, y0, xb, yb);

	xp = (x2 + xb) / 2;
	yp = (y2 + yb) / 2;
	if (ABS(xc - xp) + ABS(yc - yp) > SPLINE_THRESH)
		XDrawSpline(display, d, gc, xb, yb, xc, yc, x2, y2);
	else
		XDrawLine(display, d, gc, xb, yb, x2, y2);
}
