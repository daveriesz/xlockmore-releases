/* -*- Mode: C; tab-width: 4 -*- */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)roll.c	4.03 97/05/10 xlockmore";

#endif

/*-
 * roll.c - roll for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1995 by Charles Vidal <vidalc@univ-mlv.fr>
 *         http://fillmore.univ-mlv.fr/~vidalc
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 * 10-May-97:  Compatible with xscreensaver
 * 1995: Written.
 */

#ifdef STANDALONE
#define PROGCLASS            "Roll"
#define HACK_INIT            init_roll
#define HACK_DRAW            draw_roll
#define DEF_DELAY            100000
#define DEF_BATCHCOUNT       25
#define DEF_SIZE             -64
#define DEF_NCOLORS          200
#define SPREAD_COLORS
#include "xlockmore.h"    /* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"    /* in xlockmore distribution */
ModeSpecOpt roll_opts =
{0, NULL, 0, NULL, NULL};
 
#endif /* STANDALONE */

#define MINPTS 1
#define MINSIZE 8
#define FACTOR 8.0
#define SPEED 25.0

typedef struct {
	float       t, u, v;
	float       t1, u1, v1;
} ptsstruct;
typedef struct {
	ptsstruct  *pts;
	XPoint     *p;
	int         maxpts, npts;
	float       alpha, theta, phi, r;
	XPoint      sphere, direction;
	int         color;
	int         width, height;
} rollstruct;

static rollstruct *rolls = NULL;

static void
createsphere(rollstruct * rp, int n1, int n2)
{
	double      i, j;
	int         n = 0;

	for (i = 0.0; i < FACTOR * M_PI; i += (FACTOR * M_PI) / n1)
		for (j = 0.0; j < FACTOR * M_PI; j += (FACTOR * M_PI) / n2) {
			rp->pts[n].t1 = rp->r * COSF(i) * COSF(j);
			rp->pts[n].u1 = rp->r * COSF(i) * SINF(j);
			rp->pts[n].v1 = rp->r * SINF(i);
			n++;
		}
}

static void
rotation3d(rollstruct * rp)
{
	float       c1, c2, c3, c4, c5, c6, c7, c8, c9, x, y, z;
	float       sintheta, costheta;
	float       sinphi, cosphi;
	float       sinalpha, cosalpha;
	int         i;

	sintheta = SINF(rp->theta);
	costheta = COSF(rp->theta);
	sinphi = SINF(rp->phi);
	cosphi = COSF(rp->phi);
	sinalpha = SINF(rp->alpha);
	cosalpha = COSF(rp->alpha);

	c1 = cosphi * costheta;
	c2 = sinphi * costheta;
	c3 = -sintheta;

	c4 = cosphi * sintheta * sinalpha - sinphi * cosalpha;
	c5 = sinphi * sintheta * sinalpha + cosphi * cosalpha;
	c6 = costheta * sinalpha;

	c7 = cosphi * sintheta * cosalpha + sinphi * sinalpha;
	c8 = sinphi * sintheta * cosalpha - cosphi * sinalpha;
	c9 = costheta * cosalpha;
	for (i = 0; i < rp->maxpts; i++) {
		x = rp->pts[i].t;
		y = rp->pts[i].u;
		z = rp->pts[i].v;
		rp->pts[i].t = c1 * x + c2 * y + c3 * z;
		rp->pts[i].u = c4 * x + c5 * y + c6 * z;
		rp->pts[i].v = c7 * x + c8 * y + c9 * z;
	}
}

static void
project(rollstruct * rp)
{
	int         i;

	for (i = 0; i < rp->maxpts; i++) {
		rp->p[i].x = (short) (2 * rp->pts[i].t);
		rp->p[i].y = (short) (2 * rp->pts[i].u);
	}
}

void
init_roll(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	rollstruct *rp;
	int         i;
	int         size = MI_SIZE(mi);
	double      ang;

	if (rolls == NULL) {
		if ((rolls = (rollstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (rollstruct))) == NULL)
			return;
	}
	rp = &rolls[MI_SCREEN(mi)];

	ang = (double) NRAND(75) + 7.5;
	rp->direction.x = ((2 * (LRAND() & 1)) - 1) * (int)
		(SPEED * SINF(ang * M_PI / 180.0));
	rp->direction.y = ((2 * (LRAND() & 1)) - 1) * (int)
		(SPEED * COSF(ang * M_PI / 180.0));
	rp->width = MI_WIN_WIDTH(mi);
	rp->height = MI_WIN_HEIGHT(mi);
	if (size < -MINSIZE)
		rp->r = NRAND(MIN(-size, MAX(MINSIZE,
		   MIN(rp->width, rp->height) / 4)) - MINSIZE + 1) + MINSIZE;
	else if (size < MINSIZE) {
		if (!size)
			rp->r = MAX(MINSIZE, MIN(rp->width, rp->height) / 4);
		else
			rp->r = MINSIZE;
	} else
		rp->r = MIN(size, MAX(MINSIZE,
				      MIN(rp->width, rp->height) / 4));
	rp->sphere.x = NRAND(MAX(1, rp->width - 4 * (int) rp->r)) +
		2 * (int) rp->r;
	rp->sphere.y = NRAND(MAX(1, rp->height - 4 * (int) rp->r)) +
		2 * (int) rp->r;
	rp->alpha = 0;
	rp->theta = 0;
	rp->phi = 0;
	rp->maxpts = MI_BATCHCOUNT(mi);
	if (rp->maxpts < -MINPTS) {
		/* if rp->maxpts is random ... the size can change */
		if (rp->pts != NULL) {
			(void) free((void *) rp->pts);
			rp->pts = NULL;
		}
		rp->maxpts = NRAND(-rp->maxpts - MINPTS + 1) + MINPTS;
	} else if (rp->maxpts < MINPTS)
		rp->maxpts = MINPTS;
	i = rp->maxpts;
	rp->maxpts *= rp->maxpts;
	rp->npts = 0;
	if (rp->pts == NULL)
		rp->pts = (ptsstruct *) malloc(rp->maxpts * sizeof (ptsstruct));
	if (rp->p != NULL) {
		(void) free((void *) rp->p);
		rp->p = NULL;
	}
	if (MI_NPIXELS(mi) > 2)
		rp->color = NRAND(MI_NPIXELS(mi));
	createsphere(rp, i, i);
	XClearWindow(display, MI_WINDOW(mi));
}

void
draw_roll(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	rollstruct *rp = &rolls[MI_SCREEN(mi)];
	int         i;

	for (i = 0; i < rp->maxpts; i++) {
		rp->pts[i].t = rp->pts[i].t1;
		rp->pts[i].u = rp->pts[i].u1;
		rp->pts[i].v = rp->pts[i].v1;
	}
	rp->alpha += ((FACTOR * M_PI) / 200.0);
	rp->theta += ((FACTOR * M_PI) / 200.0);
	rp->phi += ((FACTOR * M_PI) / 200.0);
	if (rp->alpha > (FACTOR * M_PI))
		rp->alpha -= (FACTOR * M_PI);
	if (rp->theta > (FACTOR * M_PI))
		rp->theta -= (FACTOR * M_PI);
	if (rp->phi > (FACTOR * M_PI))
		rp->phi -= (FACTOR * M_PI);

	if (rp->npts) {
		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
		XDrawPoints(display, window, gc, rp->p, rp->npts, CoordModeOrigin);
	} else {
		if (rp->p)
			(void) free((void *) rp->p);
		rp->p = (XPoint *) malloc(rp->maxpts * sizeof (XPoint));
	}
	rotation3d(rp);
	project(rp);
	rp->npts = 0;
	for (i = 0; i < rp->maxpts; i++) {
		if (rp->pts[i].v > 0.0) {
			rp->p[rp->npts].x += rp->sphere.x;
			rp->p[rp->npts].y += rp->sphere.y;
			rp->npts++;
		}
	}
	if (MI_NPIXELS(mi) <= 2)
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	else {
		rp->color = (rp->color + 1) % MI_NPIXELS(mi);
		XSetForeground(display, gc, MI_PIXEL(mi, rp->color));
	}
	XDrawPoints(display, window, gc, rp->p, rp->npts, CoordModeOrigin);
	if (rp->sphere.x >= rp->width - (int) rp->r && rp->direction.x > 0)
		rp->direction.x = -rp->direction.x;
	else if (rp->sphere.x <= (int) rp->r && rp->direction.x < 0)
		rp->direction.x = -rp->direction.x;
	else if (rp->sphere.x < rp->width - 2 * (int) rp->r ||
		 rp->sphere.x > 2 * (int) rp->r) {
		if (rp->sphere.x >= rp->width - 2 * (int) rp->r && rp->direction.x > 0)
			rp->direction.x = -rp->direction.x;
		else if (rp->sphere.x <= 2 * (int) rp->r && rp->direction.x < 0)
			rp->direction.x = -rp->direction.x;
	}
	if (rp->sphere.y >= rp->height - (int) rp->r && rp->direction.y > 0)
		rp->direction.y = -rp->direction.y;
	else if (rp->sphere.y <= (int) rp->r && rp->direction.y < 0)
		rp->direction.y = -rp->direction.y;
	else if (rp->sphere.y < rp->height - 2 * (int) rp->r ||
		 rp->sphere.y > 2 * (int) rp->r) {
		if (rp->sphere.y >= rp->height - 2 * (int) rp->r && rp->direction.y > 0)
			rp->direction.y = -rp->direction.y;
		else if (rp->sphere.y <= 2 * (int) rp->r && rp->direction.y < 0)
			rp->direction.y = -rp->direction.y;
	}
	rp->sphere.x += rp->direction.x;
	rp->sphere.y += rp->direction.y;
}

void
release_roll(ModeInfo * mi)
{
	if (rolls != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			rollstruct *rp = &rolls[screen];

			if (rp->pts != NULL)
				(void) free((void *) rp->pts);
			if (rp->p != NULL)
				(void) free((void *) rp->p);
		}
		(void) free((void *) rolls);
		rolls = NULL;
	}
}

void
refresh_roll(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
