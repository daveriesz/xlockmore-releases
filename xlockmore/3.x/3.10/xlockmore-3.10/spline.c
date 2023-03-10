
#ifndef lint
static char sccsid[] = "@(#)spline.c	3.10 96/07/20 xlockmore";

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
	XPoint      pos, delta;
} splinepointstruct;

typedef struct {
#ifdef FOLLOW
	int         first;
	int         last;
	XPoint    **splineq;
#endif
	int         width;
	int         height;
	int         color;
	int         points;
	int         nsplines;
	splinepointstruct *pt;
} splinestruct;

static splinestruct *splines = NULL;

static void XDrawSpline(Display * display, Drawable d, GC gc,
			int x0, int y0, int x1, int y1, int x2, int y2);

void
init_spline(ModeInfo * mi)
{
	splinestruct *sp;
	int         i;

	if (splines == NULL) {
		if ((splines = (splinestruct *) calloc(MI_NUM_SCREENS(mi),
					     sizeof (splinestruct))) == NULL)
			return;
	}
	sp = &splines[MI_SCREEN(mi)];

	sp->width = MI_WIN_WIDTH(mi);
	sp->height = MI_WIN_HEIGHT(mi);
	/* batchcount is the upper bound on the number of points */
	sp->points = MI_BATCHCOUNT(mi);
	if (sp->points < -MINPOINTS) {
		if (sp->pt) {
			(void) free((void *) sp->pt);
			sp->pt = NULL;
		}
		sp->points = NRAND(-sp->points - MINPOINTS + 1) + MINPOINTS;
	} else if (sp->points < MINPOINTS)
		sp->points = MINPOINTS;
	if (!sp->pt)
		sp->pt = (splinepointstruct *) malloc(sp->points *
						 sizeof (splinepointstruct));

#ifdef FOLLOW
	sp->last = 0;

	sp->nsplines = (MI_CYCLES(mi) + 1) * 2;
	if (!sp->splineq) {
		sp->splineq = (XPoint **) malloc(sp->nsplines * sizeof (XPoint *));
		for (i = 0; i < sp->nsplines; ++i)
			sp->splineq[i] = (XPoint *) calloc(sp->points, sizeof (XPoint));
	}
#else
	sp->nsplines = 0;
#endif

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

	/* Initialize points. */
	for (i = 0; i < sp->points; ++i) {
		sp->pt[i].pos.x = NRAND(sp->width);
		sp->pt[i].pos.y = NRAND(sp->height);
		sp->pt[i].delta.x = NRAND(MAX_DELTA * 2) - MAX_DELTA;
		if (sp->pt[i].delta.x <= 0 && sp->width > 1)
			--sp->pt[i].delta.x;
		sp->pt[i].delta.y = NRAND(MAX_DELTA * 2) - MAX_DELTA;
		if (sp->pt[i].delta.y <= 0 && sp->height > 1)
			--sp->pt[i].delta.y;
	}

	sp->color = 0;
}


void
draw_spline(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	splinestruct *sp = &splines[MI_SCREEN(mi)];
	int         i, t, px, py, zx, zy, nx, ny;

#ifdef FOLLOW
	sp->first = (sp->last + 2) % sp->nsplines;
#endif

	/* Move the points. */
	for (i = 0; i < sp->points; i++) {
		for (;;) {
			t = sp->pt[i].pos.x + sp->pt[i].delta.x;
			if (t >= 0 && t < sp->width)
				break;
			sp->pt[i].delta.x = NRAND(MAX_DELTA * 2) - MAX_DELTA;
			if (sp->pt[i].delta.x <= 0 && sp->width > 1)
				--sp->pt[i].delta.x;
		}
		sp->pt[i].pos.x = t;
		for (;;) {
			t = sp->pt[i].pos.y + sp->pt[i].delta.y;
			if (t >= 0 && t < sp->height)
				break;
			sp->pt[i].delta.y = NRAND(MAX_DELTA * 2) - MAX_DELTA;
			if (sp->pt[i].delta.y <= 0 && sp->height > 1)
				--sp->pt[i].delta.y;
		}
		sp->pt[i].pos.y = t;
	}

#ifdef FOLLOW
	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));

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
		XDrawSpline(display, window, gc,
			    px, py, sp->splineq[sp->first][i].x,
			    sp->splineq[sp->first][i].y, nx, ny);
		px = nx;
		py = ny;
	}

	XDrawSpline(display, window, gc,
		    px, py, sp->splineq[sp->first][sp->points - 1].x,
		    sp->splineq[sp->first][sp->points - 1].y, zx, zy);

	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(display, gc, MI_PIXEL(mi, sp->color));
		if (++sp->color >= MI_NPIXELS(mi))
			sp->color = 0;
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
#endif

	/* Draw the figure. */
	px = zx = (sp->pt[0].pos.x + sp->pt[sp->points - 1].pos.x) / 2;
	py = zy = (sp->pt[0].pos.y + sp->pt[sp->points - 1].pos.y) / 2;
	for (i = 0; i < sp->points - 1; ++i) {
		nx = (sp->pt[i + 1].pos.x + sp->pt[i].pos.x) / 2;
		ny = (sp->pt[i + 1].pos.y + sp->pt[i].pos.y) / 2;
		XDrawSpline(display, window, gc,
			    px, py, sp->pt[i].pos.x, sp->pt[i].pos.y, nx, ny);
		px = nx;
		py = ny;
	}
#ifndef FOLLOW
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(display, gc, MI_PIXEL(mi, sp->color));
		if (++sp->color >= MI_NPIXELS(mi))
			sp->color = 0;
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
#endif

	XDrawSpline(display, window, gc, px, py,
	 sp->pt[sp->points - 1].pos.x, sp->pt[sp->points - 1].pos.y, zx, zy);

#ifdef FOLLOW
	for (i = 0; i < sp->points; ++i) {
		sp->splineq[sp->last][i].x = sp->pt[i].pos.x;
		sp->splineq[sp->last][i].y = sp->pt[i].pos.y;
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

void
release_spline(ModeInfo * mi)
{
	if (splines != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			splinestruct *sp = &splines[screen];

			if (sp->pt)
				(void) free((void *) sp->pt);
#ifdef FOLLOW
			if (sp->splineq)
				(void) free((void *) sp->splineq);
#endif
		}
		(void) free((void *) splines);
		splines = NULL;
	}
}
