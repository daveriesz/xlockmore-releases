
#ifndef lint
static char sccsid[] = "@(#)pyro.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * pyro.c - Fireworks for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 16-Mar-91: Written. (received from David Brooks, brooks@osf.org).
 */

/*-
 * The physics of the rockets is a little bogus, but it looks OK.  Each is
 * given an initial velocity impetus.  They decelerate slightly (gravity
 * overcomes the rocket's impulse) and explode as the rocket's main fuse
 * gives out (we could add a ballistic stage, maybe).  The individual
 * stars fan out from the rocket, and they decelerate less quickly.
 * That's called bouyancy, but really it's again a visual preference.
 */

#include "xlock.h"
#include <math.h>

#define MINROCKETS 1
/* Define this >1 to get small rectangles instead of points */
#ifndef STARSIZE
#define STARSIZE 2
#endif

#define SILENT 0
#define REDGLARE 1
#define BURSTINGINAIR 2

#define CLOUD 0
#define DOUBLECLOUD 1
/* Clearly other types and other fascinating visual effects could be added... */

/* P_xxx parameters represent the reciprocal of the probability... */
#define P_IGNITE 5000		/* ...of ignition per cycle */
#define P_DOUBLECLOUD 10	/* ...of an ignition being double */
#define P_MULTI 75		/* ...of an ignition being several @ once */
#define P_FUSILLADE 250		/* ...of an ignition starting a fusillade */

#define ROCKETW 2		/* Dimensions of rocket */
#define ROCKETH 4
#define XVELFACTOR 0.0025	/* Max horizontal velocity / screen width */
#define MINYVELFACTOR 0.016	/* Min vertical velocity / screen height */
#define MAXYVELFACTOR 0.018
#define GRAVFACTOR 0.0002	/* delta v / screen height */
#define MINFUSE 50		/* range of fuse lengths for rocket */
#define MAXFUSE 100

#define FUSILFACTOR 10		/* Generate fusillade by reducing P_IGNITE */
#define FUSILLEN 100		/* Length of fusillade, in ignitions */

#define SVELFACTOR 0.1		/* Max star velocity / yvel */
#define BOUYANCY 0.2		/* Reduction in grav deceleration for stars */
#define MAXSTARS 75		/* Number of stars issued from a shell */
#define MINSTARS 50
#define MINSFUSE 50		/* Range of fuse lengths for stars */
#define MAXSFUSE 100

#define INTRAND(min,max) (NRAND((max+1)-(min))+(min))
#define FLOATRAND(min,max) ((min)+((double) LRAND()/((double) MAXRAND))*((max)-(min)))

ModeSpecOpt pyro_opts =
{0, NULL, NULL, NULL};

typedef struct {
	float       sx, sy;	/* Distance from notional center  */
	float       sxvel, syvel;	/* Relative to notional center */
} star;

typedef struct {
	int         state;
	int         shelltype;
	int         fuse;
	float       xvel, yvel;
	float       x, y;
	unsigned long color[2];
	int         nstars;
#if STARSIZE > 1
	XRectangle  Xpoints[2][MAXSTARS];
#else
	XPoint      Xpoints[2][MAXSTARS];
#endif
	star        stars[MAXSTARS];
} rocket;

typedef struct {
	int         p_ignite;
	unsigned long rockpixel;
	int         nflying, nrockets;
	int         fusilcount;
	int         width, lmargin, rmargin, height;
	float       minvelx, maxvelx;
	float       minvely, maxvely;
	float       maxsvel;
	float       rockdecel, stardecel;
	rocket     *rockq;
} pyrostruct;

static void ignite(ModeInfo * mi, pyrostruct * pp);
static void animate(ModeInfo * mi, pyrostruct * pp, rocket * rp);
static void shootup(ModeInfo * mi, pyrostruct * pp, rocket * rp);
static void burst(ModeInfo * mi, pyrostruct * pp, rocket * rp);

static pyrostruct *pyros = NULL;
static int  orig_p_ignite;
static int  just_started = True;	/* Greet the user right away */

static void
ignite(ModeInfo * mi, pyrostruct * pp)
{
	rocket     *rp;
	int         multi, shelltype, nstars, fuse, pix;
	unsigned long color[2];
	float       xvel, yvel, x;

	x = NRAND(pp->width);
	xvel = FLOATRAND(-pp->maxvelx, pp->maxvelx);
/* All this to stop too many rockets going offscreen: */
	if ((x < pp->lmargin && xvel < 0.0) || (x > pp->rmargin && xvel > 0.0))
		xvel = -xvel;
	yvel = FLOATRAND(pp->minvely, pp->maxvely);
	fuse = INTRAND(MINFUSE, MAXFUSE);
	nstars = INTRAND(MINSTARS, MAXSTARS);
	if (MI_NPIXELS(mi) > 2) {
		pix = NRAND(MI_NPIXELS(mi));
		color[0] = MI_PIXEL(mi, pix);
		color[1] = MI_PIXEL(mi, ((pix + (MI_NPIXELS(mi) / 2)) % MI_NPIXELS(mi)));
	} else {
		color[0] = color[1] = MI_WIN_WHITE_PIXEL(mi);
	}

	multi = 1;
	if (NRAND(P_DOUBLECLOUD) == 0)
		shelltype = DOUBLECLOUD;
	else {
		shelltype = CLOUD;
		if (NRAND(P_MULTI) == 0)
			multi = INTRAND(5, 15);
	}

	rp = pp->rockq;
	while (multi--) {
		if (pp->nflying >= pp->nrockets)
			return;
		while (rp->state != SILENT)
			rp++;
		pp->nflying++;
		rp->shelltype = shelltype;
		rp->state = REDGLARE;
		rp->color[0] = color[0];
		rp->color[1] = color[1];
		rp->xvel = xvel;
		rp->yvel = FLOATRAND(yvel * 0.97, yvel * 1.03);
		rp->fuse = INTRAND((fuse * 90) / 100, (fuse * 110) / 100);
		rp->x = x + FLOATRAND(multi * 7.6, multi * 8.4);
		rp->y = pp->height - 1;
		rp->nstars = nstars;
	}
}

static void
animate(ModeInfo * mi, pyrostruct * pp, rocket * rp)
{
	int         starn;
	float       r, theta;

	if (rp->state == REDGLARE) {
		shootup(mi, pp, rp);

/* Handle setup for explosion */
		if (rp->state == BURSTINGINAIR) {
			for (starn = 0; starn < rp->nstars; starn++) {
				rp->stars[starn].sx = rp->stars[starn].sy = 0.0;
				rp->Xpoints[0][starn].x = (int) rp->x;
				rp->Xpoints[0][starn].y = (int) rp->y;
				if (rp->shelltype == DOUBLECLOUD) {
					rp->Xpoints[1][starn].x = (int) rp->x;
					rp->Xpoints[1][starn].y = (int) rp->y;
				}
/* This isn't accurate solid geometry, but it looks OK. */

				r = FLOATRAND(0.0, pp->maxsvel);
				theta = FLOATRAND(0.0, M_PI * 2.0);
				rp->stars[starn].sxvel = r * COSF(theta);
				rp->stars[starn].syvel = r * SINF(theta);
			}
			rp->fuse = INTRAND(MINSFUSE, MAXSFUSE);
		}
	}
	if (rp->state == BURSTINGINAIR) {
		burst(mi, pp, rp);
	}
}

static void
shootup(ModeInfo * mi, pyrostruct * pp, rocket * rp)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);

	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, window, gc, (int) (rp->x), (int) (rp->y),
		       ROCKETW, ROCKETH + 3);

	if (rp->fuse-- <= 0) {
		rp->state = BURSTINGINAIR;
		return;
	}
	rp->x += rp->xvel;
	rp->y += rp->yvel;
	rp->yvel += pp->rockdecel;
	XSetForeground(display, gc, pp->rockpixel);
	XFillRectangle(display, window, gc, (int) (rp->x), (int) (rp->y),
		       ROCKETW, (int) (ROCKETH + NRAND(4)));
}

static void
burst(ModeInfo * mi, pyrostruct * pp, rocket * rp)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	register int starn;
	register int nstars, stype;
	register float rx, ry, sd;	/* Help compiler optimize :-) */
	register float sx, sy;

	nstars = rp->nstars;
	stype = rp->shelltype;
	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));

#if STARSIZE > 1
	XFillRectangles(display, window, gc, rp->Xpoints[0], nstars);
	if (stype == DOUBLECLOUD)
		XFillRectangles(display, window, gc, rp->Xpoints[1], nstars);
#else
	XDrawPoints(display, window, gc, rp->Xpoints[0], nstars, CoordModeOrigin);
	if (stype == DOUBLECLOUD)
		XDrawPoints(display, window, gc, rp->Xpoints[1], nstars, CoordModeOrigin);
#endif

	if (rp->fuse-- <= 0) {
		rp->state = SILENT;
		pp->nflying--;
		return;
	}
/* Stagger the stars' decay */
	if (rp->fuse <= 7) {
		if ((rp->nstars = nstars = nstars * 90 / 100) == 0)
			return;
	}
	rx = rp->x;
	ry = rp->y;
	sd = pp->stardecel;
	for (starn = 0; starn < nstars; starn++) {
		sx = rp->stars[starn].sx += rp->stars[starn].sxvel;
		sy = rp->stars[starn].sy += rp->stars[starn].syvel;
		rp->stars[starn].syvel += sd;
		rp->Xpoints[0][starn].x = (int) (rx + sx);
		rp->Xpoints[0][starn].y = (int) (ry + sy);
		if (stype == DOUBLECLOUD) {
			rp->Xpoints[1][starn].x = (int) (rx + 1.7 * sx);
			rp->Xpoints[1][starn].y = (int) (ry + 1.7 * sy);
		}
	}
	rp->x = rx + rp->xvel;
	rp->y = ry + rp->yvel;
	rp->yvel += sd;

	XSetForeground(display, gc, rp->color[0]);
#if STARSIZE > 1
	XFillRectangles(display, window, gc, rp->Xpoints[0], nstars);
	if (stype == DOUBLECLOUD) {
		XSetForeground(display, gc, rp->color[1]);
		XFillRectangles(display, window, gc, rp->Xpoints[1], nstars);
	}
#else
	XDrawPoints(display, window, gc, rp->Xpoints[0], nstars, CoordModeOrigin);
	if (stype == DOUBLECLOUD) {
		XSetForeground(display, gc, rp->color[1]);
		XDrawPoints(display, window, gc, rp->Xpoints[1], nstars,
			    CoordModeOrigin);
	}
#endif
}

void
init_pyro(ModeInfo * mi)
{
	pyrostruct *pp;
	rocket     *rp;
	int         rockn, starn, bsize;

	if (pyros == NULL) {
		if ((pyros = (pyrostruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (pyrostruct))) == NULL)
			return;
	}
	pp = &pyros[MI_SCREEN(mi)];

	pp->width = MI_WIN_WIDTH(mi);
	pp->height = MI_WIN_HEIGHT(mi);
	pp->lmargin = pp->width / 16;
	pp->rmargin = pp->width - pp->lmargin;

	pp->nrockets = MI_BATCHCOUNT(mi);
	if (pp->nrockets < -MINROCKETS) {
		if (pp->rockq) {
			(void) free((void *) pp->rockq);
			pp->rockq = NULL;
		}
		pp->nrockets = NRAND(-pp->nrockets - MINROCKETS + 1) + MINROCKETS;
	} else if (pp->nrockets < MINROCKETS)
		pp->nrockets = MINROCKETS;
	orig_p_ignite = P_IGNITE / pp->nrockets;
	if (orig_p_ignite <= 0)
		orig_p_ignite = 1;
	pp->p_ignite = orig_p_ignite;

	if (!pp->rockq) {
		pp->rockq = (rocket *) malloc(pp->nrockets * sizeof (rocket));
	}
	pp->nflying = pp->fusilcount = 0;

	bsize = (pp->height <= 64) ? 1 : STARSIZE;
	for (rockn = 0, rp = pp->rockq; rockn < pp->nrockets; rockn++, rp++) {
		rp->state = SILENT;
#if STARSIZE > 1
		for (starn = 0; starn < MAXSTARS; starn++) {
			rp->Xpoints[0][starn].width = rp->Xpoints[0][starn].height =
				rp->Xpoints[1][starn].width = rp->Xpoints[1][starn].height = bsize;
		}
#endif
	}

	if (MI_NPIXELS(mi) > 3)
		pp->rockpixel = MI_PIXEL(mi, 3);	/* Just the right shade of
							 * orange */
	else
		pp->rockpixel = MI_WIN_WHITE_PIXEL(mi);

/* Geometry-dependent physical data: */
	pp->maxvelx = (float) (pp->width) * XVELFACTOR;
	pp->minvelx = -pp->maxvelx;
	pp->minvely = -(float) (pp->height) * MINYVELFACTOR;
	pp->maxvely = -(float) (pp->height) * MAXYVELFACTOR;
	pp->maxsvel = pp->minvely * SVELFACTOR;
	pp->rockdecel = (float) (pp->height) * GRAVFACTOR;
	pp->stardecel = pp->rockdecel * BOUYANCY;

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

/* ARGSUSED */
void
draw_pyro(ModeInfo * mi)
{
	pyrostruct *pp = &pyros[MI_SCREEN(mi)];
	rocket     *rp;
	int         rockn;

	if (just_started || (NRAND(pp->p_ignite) == 0)) {
		just_started = False;
		if (NRAND(P_FUSILLADE) == 0) {
			pp->p_ignite = orig_p_ignite / FUSILFACTOR;
			if (pp->p_ignite <= 0)
				pp->p_ignite = 1;
			pp->fusilcount = INTRAND(FUSILLEN * 9 / 10, FUSILLEN * 11 / 10);
		}
		ignite(mi, pp);
		if (pp->fusilcount > 0) {
			if (--pp->fusilcount == 0)
				pp->p_ignite = orig_p_ignite;
		}
	}
	for (rockn = pp->nflying, rp = pp->rockq; rockn > 0; rp++) {
		if (rp->state != SILENT) {
			animate(mi, pp, rp);
			rockn--;
		}
	}
}

void
release_pyro(ModeInfo * mi)
{
	if (pyros != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			pyrostruct *pp = &pyros[screen];

			if (pp->rockq != NULL) {
				(void) free((void *) pp->rockq);
			}
		}
		(void) free((void *) pyros);
		pyros = NULL;
	}
}
