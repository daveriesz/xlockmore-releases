#ifndef lint
static char sccsid[] = "@(#)bouboule.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * bouboule.c (bouboule mode for xlockmore)
 *
 * Sort of starfield for xlockmore. I found that making a starfield for
 * a 3D engine and thought it could be a nice lock mode. For a real starfield,
 * I only scale the sort of sphere you see to the whole sky and clip the stars
 * to the camera screen.
 *
 *   Code Copyright 1996 by Jeremie PETIT (petit@eurecom.fr, jpetit@essi.fr)
 *
 *   This code must not be used elsewhere than in the xlockmore program
 * without my consent.
 *
 *   Use: batchcount is the number of stars.
 *        cycles is the maximum size for a star
 *
 * 20-Feb-96: Added tests so that already malloced objects are not
 *            malloced twice, thanks to the report from <mccomb@interport.net>
 * 01-Feb-96: Patched by Jouk Jansen <joukj@alpha.chem.uva.nl> for VMS
 *            Patched by <bagleyd@hertz.njit.edu> for TrueColor displays
 * 30-Jan-96: Wrote all that I wanted to.
 *
 *
 * 30-Jan-96: Wrote all that I wanted to.
 *
 * DONE: Build up a XArc list and Draw everything once with XFillArcs
 *       That idea came from looking at swarm code.
 * DONE: Add an old arcs list for erasing.
 * DONE: Make center of starfield SinVariable.
 * DONE: Add some random in the sinvary() function.
 * DONE: check time for erasing the stars with the two methods and use the
 *       better one. Note that sometimes the time difference between
 *       beginning of erasing and its end is negative! I check this, and
 *       do not use this result when it occurs. If all values are negative,
 *       the erasing will continue being done in the currently tested mode.
 * DONE: Allow stars size customization.
 * DONE: Make sizey be no less than half sizex or no bigger than twice sizex.
 *
 * IDEA: A simple check can be performed to know which stars are "behind"
 *       and which are "in front". So is possible to very simply change
 *       the drawing mode for these two sorts of stars. BUT: this would lead
 *       to a rewrite of the XArc list code because drawing should be done
 *       in two steps: "behind" stars then "in front" stars. Also, what could
 *       be the difference between the rendering of these two types of stars?
 * IDEA: Calculate the distance of each star to the "viewer" and render the
 *       star accordingly to this distance. Same remarks as for previous
 *       ideas can be pointed out. This would even lead to reget the old stars
 *       drawing code, that has been replaced by the XFillArcs. On another
 *       hand, this would allow particular stars (own color, shape...), as
 *       far as they would be individually drawn. One should be careful to
 *       draw them according to their distance, that is not drawing a far
 *       star after a close one.
 */

#include "xlock.h"
#include <math.h>

#define USEOLDXARCS  1		/* If 1 , we use old xarcs list for erasing.
				   * else we just roughly erase the window.
				   * This mainly depends on the number of stars,
				   * because when they are many, it is faster to
				   * erase the whole window than to erase each star
				 */

#if !defined( VMS ) || defined( XVMSUTILS ) || ( __VMS_VER >= 70000000 )
#define ADAPT_ERASE  1		/* If 1, then we try ADAPT_CHECKS black XFillArcs,
				   * and after, ADAPT_CHECKS XFillRectangle.
				   * We check which method seems better, knowing that
				   * XFillArcs is generally visually better. So we
				   * consider that XFillArcs is still better if its time
				   * is about XFillRectangle * ADAPT_ARC_PREFERED
				   * We need gettimeofday
				   * for this... Does it exist on other systems ? Do we
				   * have to use another function for others ?
				   * This value overrides USEOLDXARCS.
				 */
#endif

#if (ADAPT_ERASE == 1)
#if defined( VMS ) && ( __VMS_VER < 70000000 )
#if 0
#include "../xvmsutils/unix_time.h"
#else
#include <X11/unix_time.h>
#endif
#endif
#ifdef AIXV3
#include <sys/select.h>
#else
#include <sys/time.h>
#endif
#define ADAPT_CHECKS 50
#define ADAPT_ARC_PREFERED 150	/* Maybe the value that is the most important
				   * for adapting to a system */
#endif

#define dtor(x)    ((x) / (180.0 / M_PI))	/* Degrees to radians */
#define MIN(x,y)   (((x) < (y)) ? (x) : (y))	/* Minimum of 2 values */
#define MAX(x,y)   (((x) > (y)) ? (x) : (y))	/* Maximum of 2 values */

#define NBSTARS       1000	/* Default Number of stars */
#define COLOR_CHANGES 1		/* How often we change colors (1 = always)
				   * This value should be tuned accordingly to
				   * the number of stars */
#define MAX_SIZEX_SIZEY 2.	/* This controls whether the sphere can be very
				   * very large and have a small height (or the
				   * opposite) or no. */

#define THETACANRAND  80	/* percentage of changes for the speed of
				   * change of the 3 theta values */
#define SIZECANRAND   80	/* percentage of changes for the speed of
				   * change of the sizex and sizey values */
#define POSCANRAND    80	/* percentage of changes for the speed of
				   * change of the x and y values */
/* Note that these XXXCANRAND values can be 0, that is no rand acceleration *
   variation. */

#define VARRANDALPHA ((LRAND () % 3142) / 1000.0)
#define VARRANDSTEP  (M_PI / ((LRAND () % 100) + 100.0))
#define VARRANDMIN   - 70.0
#define VARRANDMAX   70.0

/* These values are the variation parameters of the acceleration variation *
   of the SinVariables that are randomized. */

ModeSpecOpt bouboule_opts =
{0, NULL, NULL, NULL};

/******************************/
typedef struct SinVariableStruct
/******************************/
{
	double      alpha;	/* 
				 * Alpha is the current state of the sinvariable
				 * alpha should be initialized to a value between
				 * 0.0 and 2 * M_PI
				 */
	double      step;	/*
				 * Speed of evolution of alpha. It should be a reasonable
				 * fraction of 2 * M_PI. This value directly influence
				 * the variable speed of variation.
				 */
	double      minimum;	/* Minimum value for the variable */
	double      maximum;	/* Maximum value for the variable */
	double      value;	/* Current value */
	int         mayrand;	/* Flag for knowing whether some randomization can be
				 * applied to the variable */
	struct SinVariableStruct *varrand;	/* Evolving Variable: the variation of
						   * alpha */
} SinVariable;

/***********************/
typedef struct StarStruct
/***********************/
{
	double      x, y, z;	/* Position of the star */
	short       size;	/* Try to guess */
} Star;

/****************************/
typedef struct StarFieldStruct
/****************************/
{
	short       width, height;	/* width and height of the starfield window */
	short       max_star_size;	/* Maximum radius for stars. stars radius will
					 * vary from 1 to MAX_STAR_SIZE */
	SinVariable x;		/* Evolving variables:               */
	SinVariable y;		/* Center of the field on the screen */
	SinVariable sizex;	/* Evolving variable: half width  of the field */
	SinVariable sizey;	/* Evolving variable: half height of the field */
	SinVariable thetax;	/* Evolving Variables:              */
	SinVariable thetay;	/* rotation angles of the starfield */
	SinVariable thetaz;	/* around x, y and z local axis     */

	Star       *star;	/* List of stars */
	XArc       *xarc;	/* Current List of arcs */
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
	XArc       *oldxarc;	/* Old list of arcs */
#endif
	unsigned long color;	/* Current color of the starfield */
	int         colorp;	/* Pointer to color of the starfield */
	int         NbStars;	/* Number of stars */
	short       colorchange;	/* Counter for the color change */
#if (ADAPT_ERASE == 1)
	short       hasbeenchecked;
	long        rect_time;
	long        xarc_time;
#endif
} StarField;

static StarField starfield[MAXSCREENS];

/*********/
static void
sinvary(SinVariable * v)
/*********/

{
	v->value = v->minimum +
		(v->maximum - v->minimum) * (sin(v->alpha) + 1.0) / 2.0;

	if (v->mayrand == 0)
		v->alpha += v->step;
	else {
		int         vaval = LRAND() % 100;

		if (vaval <= v->mayrand)
			sinvary(v->varrand);
		v->alpha += (100.0 + (v->varrand->value)) * v->step / 100.0;
	}

	if (v->alpha > 2 * M_PI)
		v->alpha -= 2 * M_PI;
}

/*************************************************/
static void
sininit(SinVariable * v,
	double alpha, double step, double minimum, double maximum,
	short int mayrand)
{
	v->alpha = alpha;
	v->step = step;
	v->minimum = minimum;
	v->maximum = maximum;
	v->mayrand = mayrand;
	if (mayrand != 0) {
		if (v->varrand == NULL)
			v->varrand = (SinVariable *) malloc(sizeof (SinVariable));
		sininit(v->varrand,
			VARRANDALPHA,
			VARRANDSTEP,
			VARRANDMIN,
			VARRANDMAX,
			0);
		sinvary(v->varrand);
	}
	/* We calculate the values at least once for initialization */
	sinvary(v);
}

/***************/
void
init_bouboule(ModeInfo * mi)
/***************/


/* The stars init part was first inspirated from the net3d game starfield *
   code. * But net3d starfield is not really 3d starfield, and I needed real
   3d, * so only remains the net3d starfield initialization main idea, that is
   * the stars distribution on a sphere (theta and omega computing) */
{
	Window      win = MI_WINDOW(mi);
	int         i;
	double      theta, omega;
	Star       *star;
	XArc       *arc;

#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
	XArc       *oarc;

#endif

	starfield[screen].width = MI_WIN_WIDTH(mi);
	starfield[screen].height = MI_WIN_HEIGHT(mi);

	(void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	(void) XFillRectangle(dsp, win, Scr[screen].gc,
			      0, 0,
			      starfield[screen].width,
			      starfield[screen].height);

	starfield[screen].max_star_size = MI_CYCLES(mi);

	starfield[screen].NbStars = MI_BATCHCOUNT(mi);
	if (starfield[screen].NbStars < 1)
		starfield[screen].NbStars = NBSTARS;

	/* We get memory for lists of objects */
	if (starfield[screen].star == NULL)
		starfield[screen].star = (Star *)
			malloc(starfield[screen].NbStars * sizeof (Star));
	if (starfield[screen].xarc == NULL)
		starfield[screen].xarc = (XArc *)
			malloc(starfield[screen].NbStars * sizeof (XArc));
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
	if (starfield[screen].oldxarc == NULL)
		starfield[screen].oldxarc = (XArc *)
			malloc(starfield[screen].NbStars * sizeof (XArc));
#endif

	/* We initialize evolving variables */
	sininit(&starfield[screen].x,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 100) + 100.0),
		((double) starfield[screen].width) / 4.0,
		3.0 * ((double) starfield[screen].width) / 4.0,
		POSCANRAND);
	sininit(&starfield[screen].y,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 100) + 100.0),
		((double) starfield[screen].height) / 4.0,
		3.0 * ((double) starfield[screen].height) / 4.0,
		POSCANRAND);

	sininit(&starfield[screen].sizex,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 100) + 100.0),
	  MIN(((double) starfield[screen].width) - starfield[screen].x.value,
	      starfield[screen].x.value) / 5.0,
	  MIN(((double) starfield[screen].width) - starfield[screen].x.value,
	      starfield[screen].x.value),
		SIZECANRAND);

	sininit(&starfield[screen].sizey,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 100) + 100.0),
		MAX(starfield[screen].sizex.value / MAX_SIZEX_SIZEY,
		    starfield[screen].sizey.maximum / 5.0),
		MIN(starfield[screen].sizex.value * MAX_SIZEX_SIZEY,
		    MIN(((double) starfield[screen].height) -
			starfield[screen].y.value,
			starfield[screen].y.value)),
		SIZECANRAND);

	sininit(&starfield[screen].thetax,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 200) + 200.0),
		-M_PI, M_PI,
		THETACANRAND);
	sininit(&starfield[screen].thetay,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 200) + 200.0),
		-M_PI, M_PI,
		THETACANRAND);
	sininit(&starfield[screen].thetaz,
		(LRAND() % 3142) / 1000.0, M_PI / ((LRAND() % 400) + 400.0),
		-M_PI, M_PI,
		THETACANRAND);

	for (i = 0; i < starfield[screen].NbStars; i++) {
		star = &(starfield[screen].star[i]);
		arc = &(starfield[screen].xarc[i]);
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
		oarc = &(starfield[screen].oldxarc[i]);
#endif
		/* Elevation and bearing of the star */
		theta = dtor((rand() % 1800) / 10.0 - 90.0);
		omega = dtor((rand() % 3600) / 10.0 - 180.0);

		/* Stars coordinates in a 3D space */
		star->x = cos(theta) * sin(omega);
		star->y = sin(omega) * sin(theta);
		star->z = cos(omega);

		/* We set the stars size */
		star->size = LRAND() % (2 * starfield[screen].max_star_size);
		if (star->size < starfield[screen].max_star_size)
			star->size = 0;
		else
			star->size -= starfield[screen].max_star_size;

		/* We set default values for the XArc lists elements */
		arc->x = 0;
		arc->y = 0;
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
		oarc->x = 0;
		oarc->y = 0;
#endif
		arc->width = 2 + star->size;
		arc->height = 2 + star->size;
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
		oarc->width = 2 + star->size;
		oarc->height = 2 + star->size;
#endif

		arc->angle1 = 0;
		arc->angle2 = 360 * 64;
#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
		oarc->angle1 = 0;
		oarc->angle2 = 360 * 64;	/* ie. we draw whole disks:
						 * from 0 to 360 degrees */
#endif
	}

	/* We set up the starfield color */
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		starfield[screen].color = Scr[screen].pixels[starfield[screen].colorp];
	else
		starfield[screen].color = WhitePixel(dsp, screen);

#if (ADAPT_ERASE == 1)
	/* We initialize the adaptation code for screen erasing */
	starfield[screen].hasbeenchecked = ADAPT_CHECKS * 2;
	starfield[screen].rect_time = 0;
	starfield[screen].xarc_time = 0;
#endif
}

/****************/
void
draw_bouboule(ModeInfo * mi)
/****************/

{
	Window      win = MI_WINDOW(mi);
	int         i;
	double      CX, CY, CZ, SX, SY, SZ;
	Star       *star;
	XArc       *arc;

#if (ADAPT_ERASE == 1)
	struct timeval tv1;
	struct timeval tv2;

#endif

#if ((USEOLDXARCS == 0) || (ADAPT_ERASE == 1))
	short       x_1, y_1, x_2, y_2;		/* bounding rectangle around the old starfield,

						   * for erasing with the smallest rectangle
						   * instead of filling the whole screen */
#endif

#if ((USEOLDXARCS == 0) || (ADAPT_ERASE == 1))
	x_1 = starfield[screen].x.value -
		starfield[screen].sizex.value -
		starfield[screen].max_star_size;
	y_1 = starfield[screen].y.value -
		starfield[screen].sizey.value -
		starfield[screen].max_star_size;
	x_2 = 2 * (starfield[screen].sizex.value + starfield[screen].max_star_size);
	y_2 = 2 * (starfield[screen].sizey.value + starfield[screen].max_star_size);
#endif
	/* We make variables vary. */
	sinvary(&starfield[screen].thetax);
	sinvary(&starfield[screen].thetay);
	sinvary(&starfield[screen].thetaz);

	sinvary(&starfield[screen].x);
	sinvary(&starfield[screen].y);

	/* A little trick to prevent the bouboule from being
	 * bigger than the screen */
	starfield[screen].sizex.maximum =
		MIN(((double) starfield[screen].width) - starfield[screen].x.value,
		    starfield[screen].x.value);
	starfield[screen].sizex.minimum = starfield[screen].sizex.maximum / 3.0;

	/* Another trick to make the ball not too flat */
	starfield[screen].sizey.minimum =
		MAX(starfield[screen].sizex.value / MAX_SIZEX_SIZEY,
		    starfield[screen].sizey.maximum / 3.0);
	starfield[screen].sizey.maximum =
		MIN(starfield[screen].sizex.value * MAX_SIZEX_SIZEY,
	 MIN(((double) starfield[screen].height) - starfield[screen].y.value,
	     starfield[screen].y.value));

	sinvary(&starfield[screen].sizex);
	sinvary(&starfield[screen].sizey);

	/*
	 * We calculate the rotation matrix values. We just make the
	 * rotation on the fly, without using a matrix.
	 * Star positions are recorded as unit vectors pointing in various
	 * directions. We just make them all rotate.
	 */
	CX = cos(starfield[screen].thetax.value);
	SX = sin(starfield[screen].thetax.value);
	CY = cos(starfield[screen].thetay.value);
	SY = sin(starfield[screen].thetay.value);
	CZ = cos(starfield[screen].thetaz.value);
	SZ = sin(starfield[screen].thetaz.value);

	for (i = 0; i < starfield[screen].NbStars; i++) {
		star = &(starfield[screen].star[i]);
		arc = &(starfield[screen].xarc[i]);
		arc->x = (short) ((starfield[screen].sizex.value *
				   ((CY * CZ - SX * SY * SZ) * star->x +
				    (-CX * SZ) * star->y +
				    (SY * CZ + SZ * SX * CY) * star->z) +
				   starfield[screen].x.value));
		arc->y = (short) ((starfield[screen].sizey.value *
				   ((CY * SZ + SX * SY * CZ) * star->x +
				    (CX * CZ) * star->y +
				    (SY * SZ - SX * CY * CZ) * star->z) +
				   starfield[screen].y.value));
		if (star->size != 0) {
			arc->x -= star->size;
			arc->y -= star->size;
		}
	}

	/* First, we erase the previous starfield */
	(void) XSetForeground(dsp, Scr[screen].gc,
			      BlackPixel(dsp, screen));

#if (ADAPT_ERASE == 1)
	if (starfield[screen].hasbeenchecked == 0) {
		/* We just calculate which method is the faster and eventually free
		 * the oldxarc list */
		if (starfield[screen].xarc_time >
		    ADAPT_ARC_PREFERED * starfield[screen].rect_time) {
			starfield[screen].hasbeenchecked = -2;	/* XFillRectangle mode */
			(void) free((void *) starfield[screen].oldxarc);
		} else {
			starfield[screen].hasbeenchecked = -1;	/* XFillArcs mode */
		}
	}
	if (starfield[screen].hasbeenchecked == -2) {
		/* Erasing is done with XFillRectangle */
		(void) XFillRectangle(dsp, win, Scr[screen].gc,
				      x_1, y_1, x_2, y_2);
	} else if (starfield[screen].hasbeenchecked == -1) {
		/* Erasing is done with XFillArcs */
		(void) XFillArcs(dsp, win, Scr[screen].gc,
				 starfield[screen].oldxarc,
				 starfield[screen].NbStars);
	} else if (starfield[screen].hasbeenchecked > ADAPT_CHECKS) {
		(void) gettimeofday(&tv1, NULL);
		(void) XFillRectangle(dsp, win, Scr[screen].gc,
				      x_1, y_1, x_2, y_2);
		(void) gettimeofday(&tv2, NULL);
		if (tv2.tv_usec - tv1.tv_usec > 0) {
			starfield[screen].rect_time += tv2.tv_usec - tv1.tv_usec;
			starfield[screen].hasbeenchecked--;
		}
	} else {
		(void) gettimeofday(&tv1, NULL);
		(void) XFillArcs(dsp, win, Scr[screen].gc,
				 starfield[screen].oldxarc,
				 starfield[screen].NbStars);
		(void) gettimeofday(&tv2, NULL);
		if (tv2.tv_usec - tv1.tv_usec > 0) {
			starfield[screen].xarc_time += tv2.tv_usec - tv1.tv_usec;
			starfield[screen].hasbeenchecked--;
		}
	}
#else
#if (USEOLDXARCS == 1)
	(void) XFillArcs(dsp, win, Scr[screen].gc,
			 starfield[screen].oldxarc,
			 starfield[screen].NbStars);
#else
	(void) XFillRectangle(dsp, win, Scr[screen].gc,
			      x_1, y_1, x_2, y_2);
#endif
#endif

	/* Then we draw the new one */
	(void) XSetForeground(dsp, Scr[screen].gc, starfield[screen].color);
	(void) XFillArcs(dsp, win, Scr[screen].gc,
			 starfield[screen].xarc,
			 starfield[screen].NbStars);

#if ((USEOLDXARCS == 1) || (ADAPT_ERASE == 1))
#if (ADAPT_ERASE == 1)
	if (starfield[screen].hasbeenchecked >= -1) {
		arc = starfield[screen].xarc;
		starfield[screen].xarc = starfield[screen].oldxarc;
		starfield[screen].oldxarc = arc;
	}
#else
	arc = starfield[screen].xarc;
	starfield[screen].xarc = starfield[screen].oldxarc;
	starfield[screen].oldxarc = arc;
#endif
#endif

	/* We set up the color for the next drawing */
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2 &&
	    (++starfield[screen].colorchange >= COLOR_CHANGES)) {
		starfield[screen].colorchange = 0;
		if (++starfield[screen].colorp >= Scr[screen].npixels)
			starfield[screen].colorp = 0;
		starfield[screen].color = Scr[screen].pixels[starfield[screen].colorp];
	}
}
