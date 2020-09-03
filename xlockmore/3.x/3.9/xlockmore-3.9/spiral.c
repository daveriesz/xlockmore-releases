#ifndef lint
static char sccsid[] = "@(#)spiral.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * spiral.c - low cpu screen design for xlock, the X Window System lockscreen.
 *
 * Idea based on a graphics demo I
 * saw a *LONG* time ago.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 24-Jul-95: Fix to allow cycles not to have an arbitrary value by
 *            Peter Schmitzberger (schmitz@coma.sbg.ac.at).
 * 06-Mar-95: Finished cleaning up and final testing.
 * Copyright (c) 1994 by Darrick Brown.
 *
 * 03-Mar-95: Cleaned up code.
 * 12-Jul-94: Written.
 */

#include "xlock.h"
#include <math.h>

#define MAXTRAIL 512		/* The length of the trail */
#define MAXDOTS 40
#define MINDOTS 1
#define TWOPI (2*M_PI)		/* for convienence */
#define JAGGINESS 4		/* This sets the "Craziness" of the spiral (I like 4) */
#define SPEED 2.0

ModeSpecOpt spiral_opts =
{0, NULL, NULL, NULL};

typedef struct {
	float       hx, hy, ha, hr;
} Traildots;

typedef struct {
	Traildots  *traildots;
	float       cx;
	float       cy;
	float       angle;
	float       radius;
	float       dr;
	float       da;
	float       dx;
	float       dy;
	int         erase;
	int         inc;
	float       colors;
	int         width;
	int         height;
	float       top, bottom, left, right;
	int         dots, nlength;
} spiralstruct;

static spiralstruct spirals[MAXSCREENS];

static void draw_dots(Window win, int in);


static int
tfx(float x)
{

	int         a;
	spiralstruct *sp = &spirals[screen];

	a = (int) ((x / sp->right) * (float) sp->width);

	return (a);
}

static int
tfy(float y)
{

	int         a;
	spiralstruct *sp = &spirals[screen];

	a = sp->height - (int) ((y / sp->top) * (float) sp->height);

	return (a);

}

static void
draw_dots(Window win, int in)
{

	float       i, inc;
	float       x, y;

	spiralstruct *sp = &spirals[screen];

	inc = TWOPI / (float) sp->dots;
	for (i = 0.0; i < TWOPI; i += inc) {
		x = sp->traildots[in].hx + COSF(i + sp->traildots[in].ha) *
			sp->traildots[in].hr;
		y = sp->traildots[in].hy + SINF(i + sp->traildots[in].ha) *
			sp->traildots[in].hr;
		XDrawPoint(dsp, win, Scr[screen].gc, tfx(x), tfy(y));
	}

}

/* DrawSpiral */
void
draw_spiral(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	spiralstruct *sp = &spirals[screen];


	if (sp->erase == 1) {
		XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
		draw_dots(win, sp->inc);
	}
	sp->cx += sp->dx;
	sp->traildots[sp->inc].hx = sp->cx;

	if ((sp->cx > 9000.0) || (sp->cx < 1000.0))
		sp->dx *= -1.0;

	sp->cy += sp->dy;
	sp->traildots[sp->inc].hy = sp->cy;

	if ((sp->cy > 9000.0) || (sp->cy < 1000.0))
		sp->dy *= -1.0;

	sp->radius += sp->dr;
	sp->traildots[sp->inc].hr = sp->radius;

	if ((sp->radius > 2500.0) && (sp->dr > 0.0))
		sp->dr *= -1.0;
	else if ((sp->radius < 50.0) && (sp->radius < 0.0))
		sp->dr *= -1.0;

	/* Randomly give some variations to:  */

	/* spiral direction (if it is within the boundaries) */
	if ((LRAND() % 3000 < 1 * JAGGINESS) &&
	    (((sp->cx > 2000.0) && (sp->cx < 8000.0)) &&
	     ((sp->cy > 2000.0) && (sp->cy < 8000.0)))) {
		sp->dx = (float) (10 - LRAND() % 20) * SPEED;
		sp->dy = (float) (10 - LRAND() % 20) * SPEED;
	}
	/* The speed of the change in size of the spiral */
	if (LRAND() % 3000 < 1 * JAGGINESS) {
		if (LRAND() % 100 < 50)
			sp->dr += (float) (LRAND() % 3 + 1);
		else
			sp->dr -= (float) (LRAND() % 3 + 1);

		/* don't let it get too wild */
		if (sp->dr > 18.0)
			sp->dr = 18.0;
		else if (sp->dr < 4.0)
			sp->dr = 4.0;
	}
	/* The speed of rotation */
	if (LRAND() % 3000 < 1 * JAGGINESS)
		sp->da = (float) (LRAND() % 360) / 7200.0 + 0.01;

	/* Reverse rotation */
	if (LRAND() % 3000 < 1 * JAGGINESS)
		sp->da *= -1.0;

	sp->angle += sp->da;
	sp->traildots[sp->inc].ha = sp->angle;

	if (sp->angle > TWOPI)
		sp->angle -= TWOPI;
	else if (sp->angle < 0.0)
		sp->angle += TWOPI;

	sp->colors += (float) (Scr[screen].npixels) / ((float) (2 * sp->nlength));
	if (sp->colors >= (float) (Scr[screen].npixels))
		sp->colors = 0.0;

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[(int) sp->colors]);
	else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	draw_dots(win, sp->inc);
	sp->inc++;

	if (sp->inc > sp->nlength - 1) {
		sp->inc -= sp->nlength;
		sp->erase = 1;
	}
}

void
init_spiral(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	spiralstruct *sp = &spirals[screen];
	int         i;

	sp->width = MI_WIN_WIDTH(mi);
	sp->height = MI_WIN_HEIGHT(mi);

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, sp->width, sp->height);

	/* Init */
	sp->nlength = MI_CYCLES(mi);

	if (!sp->traildots)
		sp->traildots = (Traildots *) malloc(sp->nlength * sizeof (Traildots));

	/* initialize the allocated array */
	for (i = 0; i < sp->nlength; i++) {
		sp->traildots[i].hx = 0.0;
		sp->traildots[i].hy = 0.0;
		sp->traildots[i].ha = 0.0;
		sp->traildots[i].hr = 0.0;
	}

	/* keep the window parameters proportional */
	sp->top = 10000.0;
	sp->bottom = 0;
	sp->right = (float) (sp->width) / (float) (sp->height) * (10000.0);
	sp->left = 0;

	/* assign the initial values */
	sp->cx = (float) (5000.0 - (LRAND() % 2000)) / 10000.0 * sp->right;
	sp->cy = (float) (5000.0 - (LRAND() % 2000));
	sp->radius = (float) (LRAND() % 200 + 200);
	sp->angle = 0.0;
	sp->dx = (float) (10 - LRAND() % 20) * SPEED;
	sp->dy = (float) (10 - LRAND() % 20) * SPEED;
	sp->dr = (float) ((LRAND() % 10 + 4) * (1 - LRAND() % 2 * 2));
	sp->da = (float) (LRAND() % 360) / 7200.0 + 0.01;
	sp->colors = 0.0;
	sp->erase = 0;
	sp->inc = 0;
	sp->traildots[sp->inc].hx = sp->cx;
	sp->traildots[sp->inc].hy = sp->cy;
	sp->traildots[sp->inc].ha = sp->angle;
	sp->traildots[sp->inc].hr = sp->radius;
	sp->inc++;

	sp->dots = MI_BATCHCOUNT(mi);
	if (sp->dots > MAXDOTS)
		sp->dots = LRAND() % (MAXDOTS - MINDOTS + 1) + MINDOTS;
	else
		/* Absolute minimum */
	if (sp->dots < 1)
		sp->dots = 1;
}
