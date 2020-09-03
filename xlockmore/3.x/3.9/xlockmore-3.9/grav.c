
#ifndef lint
static char sccsid[] = "@(#)grav.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * grav.c - for xlock, the X Window System lockscreen.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 11-Jul-94: color version
 * 6-Oct-93: by Greg Bowering <greg@smug.student.adelaide.edu.au>
 */

#include "xlock.h"
#include <math.h>

#define GRAV			-0.02	/* Gravitational constant */
#define DIST			16.0
#define COLLIDE			0.0001
#define ALMOST			15.99
#define HALF			0.5
/* #define INTRINSIC_RADIUS     200.0 */
#define INTRINSIC_RADIUS	((float) (gp->height/5))
#define STARRADIUS		(unsigned int)(gp->height/(2*DIST))
#define AVG_RADIUS		(INTRINSIC_RADIUS/DIST)
#define RADIUS			(unsigned int)(INTRINSIC_RADIUS/(POS(Z)+DIST))

#define XR			HALF*ALMOST
#define YR			HALF*ALMOST
#define ZR			HALF*ALMOST

#define VR			0.04

#define DIMENSIONS		3
#define X			0
#define Y			1
#define Z			2

#if 0
#define TRAIL 1			/* For trails (works good in mono only) */
#define DAMPING 1		/* For decaying orbits */
#endif
#if DAMPING
#define DAMP			0.999999
#define MaxA			0.1	/* Maximum acceleration */
#endif

#define POS(c) planet->P[c]
#define VEL(c) planet->V[c]
#define ACC(c) planet->A[c]

#define Planet(x,y)\
  if ((x) >= 0 && (y) >= 0 && (x) <= gp->width && (y) <= gp->height) {\
    if (planet->ri < 2)\
     XDrawPoint(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi), (x), (y));\
    else\
     XFillArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),\
      (x) - planet->ri / 2, (y) - planet->ri / 2, planet->ri, planet->ri,\
      0, 23040);\
   }

#define FLOATRAND(min,max)	((min)+(LRAND()/MAXRAND)*((max)-(min)))

ModeSpecOpt grav_opts =
{0, NULL, NULL, NULL};

typedef struct {
	double
	            P[DIMENSIONS], V[DIMENSIONS], A[DIMENSIONS];
	int
	            xi, yi, ri;
	unsigned long colors;
} planetstruct;

typedef struct {
	int         width, height;
	int
	            x, y, sr, nplanets;
	unsigned long starcolor;
	planetstruct *planets;
} gravstruct;

static gravstruct gravs[MAXSCREENS];

static void init_planet(ModeInfo * mi, planetstruct * planet);
static void draw_planet(ModeInfo * mi, planetstruct * planet);

void
init_grav(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	gravstruct *gp = &gravs[screen];
	unsigned char ball;

	gp->width = MI_WIN_WIDTH(mi);
	gp->height = MI_WIN_HEIGHT(mi);

	gp->sr = STARRADIUS;

	gp->nplanets = MI_BATCHCOUNT(mi);
	if (gp->nplanets < 0)
		gp->nplanets = 1;
	else if (gp->nplanets > 20)
		gp->nplanets = 10;

	if (!gp->planets)
		gp->planets = (planetstruct *) calloc(gp->nplanets, sizeof (planetstruct));
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, gp->width, gp->height);
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		gp->starcolor = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
	else
		gp->starcolor = WhitePixel(dsp, screen);
	for (ball = 0; ball < (unsigned char) gp->nplanets; ball++)
		init_planet(mi, &gp->planets[ball]);

	/* Draw centrepoint */
	XDrawArc(dsp, win, Scr[screen].gc,
		 gp->width / 2 - gp->sr / 2, gp->height / 2 - gp->sr / 2, gp->sr, gp->sr,
		 0, 23040);
}

static void
init_planet(ModeInfo * mi, planetstruct * planet)
{
	gravstruct *gp = &gravs[screen];

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		planet->colors = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
	else
		planet->colors = WhitePixel(dsp, screen);
	/* Initialize positions */
	POS(X) = FLOATRAND(-XR, XR);
	POS(Y) = FLOATRAND(-YR, YR);
	POS(Z) = FLOATRAND(-ZR, ZR);

	if (POS(Z) > -ALMOST) {
		planet->xi = (unsigned int)
			((double) gp->width * (HALF + POS(X) / (POS(Z) + DIST)));
		planet->yi = (unsigned int)
			((double) gp->height * (HALF + POS(Y) / (POS(Z) + DIST)));
	} else
		planet->xi = planet->yi = -1;
	planet->ri = RADIUS;

	/* Initialize velocities */
	VEL(X) = FLOATRAND(-VR, VR);
	VEL(Y) = FLOATRAND(-VR, VR);
	VEL(Z) = FLOATRAND(-VR, VR);

	/* Draw planets */
	Planet(planet->xi, planet->yi);
}

void
draw_grav(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	gravstruct *gp = &gravs[screen];
	register unsigned char ball;

	/* Mask centrepoint */
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XDrawArc(dsp, win, Scr[screen].gc,
		 gp->width / 2 - gp->sr / 2, gp->height / 2 - gp->sr / 2, gp->sr, gp->sr,
		 0, 23040);

	/* Resize centrepoint */
	switch (LRAND() % 4) {
		case 0:
			if (gp->sr < STARRADIUS)
				gp->sr++;
			break;
		case 1:
			if (gp->sr > 2)
				gp->sr--;
	}

	/* Draw centrepoint */
	XSetForeground(dsp, Scr[screen].gc, gp->starcolor);
	XDrawArc(dsp, win, Scr[screen].gc,
		 gp->width / 2 - gp->sr / 2, gp->height / 2 - gp->sr / 2, gp->sr, gp->sr,
		 0, 23040);

	for (ball = 0; ball < (unsigned char) gp->nplanets; ball++)
		draw_planet(mi, &gp->planets[ball]);
}

static void
draw_planet(ModeInfo * mi, planetstruct * planet)
{
	gravstruct *gp = &gravs[screen];
	double      D;		/* A distance variable to work with */
	register unsigned char cmpt;

	D = POS(X) * POS(X) + POS(Y) * POS(Y) + POS(Z) * POS(Z);
	if (D < COLLIDE)
		D = COLLIDE;
	D = sqrt(D);
	D = D * D * D;
	for (cmpt = X; cmpt < DIMENSIONS; cmpt++) {
		ACC(cmpt) = POS(cmpt) * GRAV / D;
#ifdef DAMPING
		if (ACC(cmpt) > MaxA)
			ACC(cmpt) = MaxA;
		else if (ACC(cmpt) < -MaxA)
			ACC(cmpt) = -MaxA;
		VEL(cmpt) = VEL(cmpt) + ACC(cmpt);
		VEL(cmpt) *= DAMP;
#else
		/* update velocity */
		VEL(cmpt) = VEL(cmpt) + ACC(cmpt);
#endif
		/* update position */
		POS(cmpt) = POS(cmpt) + VEL(cmpt);
	}

	gp->x = planet->xi;
	gp->y = planet->yi;

	if (POS(Z) > -ALMOST) {
		planet->xi = (unsigned int)
			((double) gp->width * (HALF + POS(X) / (POS(Z) + DIST)));
		planet->yi = (unsigned int)
			((double) gp->height * (HALF + POS(Y) / (POS(Z) + DIST)));
	} else
		planet->xi = planet->yi = -1;

	/* Mask */
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	Planet(gp->x, gp->y);
#ifdef TRAIL
	XSetForeground(dsp, Scr[screen].gc, planet->colors);
	XDrawPoint(dsp, win, Scr[screen].gc, gp->x, gp->y);
#endif
	/* Move */
	gp->x = planet->xi;
	gp->y = planet->yi;
	planet->ri = RADIUS;

	/* Redraw */
	XSetForeground(dsp, Scr[screen].gc, planet->colors);
	Planet(gp->x, gp->y);
}
