#ifndef lint
static char sccsid[] = "@(#)petal.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * petal.c - Based on a program for some old PDP-11 Graphics Display Processors
 * at CMU.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 12-Aug-95: xlock version
 * Jan-95: xscreensaver version (Jamie Zawinski <jwz@netscape.com>)
 * 24-Jun-94: X11 version (Dale Moore  <Dale.Moore@cs.cmu.edu>)  
 */

/* 
 * original copyright
 * Copyright \(co 1994, by Carnegie Mellon University.  Permission to use,
 * copy, modify, distribute, and sell this software and its documentation
 * for any purpose is hereby granted without fee, provided fnord that the
 * above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation.
 * No representations are made about the  suitability of fnord this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 */

#include "xlock.h"
#include <math.h>
#define TWOPI (2*M_PI)

/* 
 * If MAXLINES is too big, we might not be able to get it
 * to the X server in the 2byte length field.
 * Must be less * than 16k
 */
#define MAXLINES (16*1024)

/* 
 * If the petal has only this many lines, it must be ugly and we dont
 * want to see it.
 */
#define MINLINES 6

ModeSpecOpt petal_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         width;
	int         height;
	int         maxlines;
	int         time;
	XPoint     *points;
	GC          gc;
} petalstruct;

static petalstruct petals[MAXSCREENS];

/* 
 * Macro:
 *   sine
 *   cosine
 * Description:
 *   Assume that degrees is .. oh 360... meaning that
 *   there are 360 degress in a circle.  Then this function
 *   would return the sin of the angle in degrees.  But lets
 *   say that they are really big degrees, with 4 big degrees
 *   the same as one regular degree.  Then this routine
 *   would be called mysin(t, 90) and would return sin(t degrees * 4)
 */
#define sine(t, degrees) sin(t * TWOPI / (degrees))
#define cosine(t, degrees) cos(t * TWOPI / (degrees))

/* 
 * Macro:
 *   rand_range
 * Description:
 *   Return a random number between a inclusive  and b exclusive.
 *    rand (3, 6) returns 3 or 4 or 5, but not 6.
 */
#define rand_range(a, b) (a + LRAND() % (b - a))

static int
gcd(int m, int n)
/* 
 * Greatest Common Divisor (also Greatest common factor).
 */
{
	int         r;

	for (;;) {
		r = m % n;
		if (r == 0)
			return (n);
		m = n;
		n = r;
	}
}

static int
numlines(int a, int b, int d)
/* 
 * Description:
 *
 *      Given parameters a and b, how many lines will we have to draw?
 *
 * Algorithm:
 *
 *      This algorithm assumes that r = sin (theta * a), where we
 *      evaluate theta on multiples of b.
 *
 *      LCM (i, j) = i * j / GCD (i, j);
 *
 *      So, at LCM (b, 360) we start over again.  But since we
 *      got to LCM (b, 360) by steps of b, the number of lines is
 *      LCM (b, 360) / b.
 *
 *      If a is odd, then at 180 we cross over and start the
 *      negative.  Someone should write up an elegant way of proving
 *      this.  Why?  Because I'm not convinced of it myself.
 *
 */
{
#define odd(x) (x & 1)
#define even(x) (!odd(x))
	if (odd(a) && odd(b) && even(d))
		d /= 2;
	return (d / gcd(d, b));
#undef odd
}

static int
compute_petal(XPoint * points, int maxlines, int sizex, int sizey)
/* 
 * Description:
 *
 *    Basically, it's combination spirograph and string art.
 *    Instead of doing lines, we just use a complex polygon,
 *    and use an even/odd rule for filling in between.
 *
 *    The spirograph, in mathematical terms is a polar
 *    plot of the form r = sin (theta * c);
 *    The string art of this is that we evaluate that
 *    function only on certain multiples of theta.  That is
 *    we let theta advance in some random increment.  And then
 *    we draw a straight line between those two adjacent points.
 *
 *    Eventually, the lines will start repeating themselves
 *    if we've evaluated theta on some rational portion of the
 *    whole.
 *
 *    The number of lines generated is limited to the
 *    ratio of the increment we put on theta to the whole.
 *    If we say that there are 360 degrees in a circle, then we
 *    will never have more than 360 lines.
 *
 * Return:
 *
 *    The number of points.
 *
 */
{
	int         a, b, d;	/* These describe a unique petal */

	double      r;
	int         theta = 0;
	XPoint     *p = points;
	int         count;
	int         numpoints;

	for (;;) {
		d = rand_range(MINLINES, maxlines);

		a = rand_range(1, d);
		b = rand_range(1, d);
		numpoints = numlines(a, b, d);
		if (numpoints > MINLINES)
			break;
	}

	/* it might be nice to try to move as much sin and cos computing
	 * (or at least the argument computing) out of the loop.
	 */
	for (count = numpoints; count--;) {
		r = sine(theta * a, d);

		/* Convert from polar to cartesian coordinates */
		/* We could round the results, but coercing seems just fine */
		p->x = sine(theta, d) * r * sizex + sizex;
		p->y = cosine(theta, d) * r * sizey + sizey;

		/* Advance index into array */
		p++;

		/* Advance theta */
		theta += b;
		theta %= d;
	}

	return (numpoints);
}

static void
random_petal(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	petalstruct *pp = &petals[screen];
	int         numpoints;
	XGCValues   gcv;

	numpoints = compute_petal(pp->points, pp->maxlines,
				  pp->width / 2, pp->height / 2);

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		gcv.function = GXcopy;
		gcv.background = BlackPixel(dsp, screen);
		gcv.foreground = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
		XChangeGC(dsp, pp->gc,
			  GCForeground | GCBackground | GCFunction, &gcv);
	}
	XFillPolygon(dsp, win, pp->gc, pp->points, numpoints, Complex,
		     CoordModeOrigin);
}

void
init_petal(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	petalstruct *pp = &petals[screen];

	pp->maxlines = MI_BATCHCOUNT(mi);
	if (pp->maxlines > MAXLINES)
		pp->maxlines = MAXLINES;
	else if (pp->maxlines < MINLINES + 2)
		pp->maxlines = MINLINES + 2;
	if (!pp->points) {
		XGCValues   gcv;

		pp->points = (XPoint *) malloc(pp->maxlines * sizeof (XPoint));

		gcv.function = GXcopy;
		gcv.foreground = WhitePixel(dsp, screen);
		gcv.background = BlackPixel(dsp, screen);
		pp->gc = XCreateGC(dsp, win,
			     GCForeground | GCBackground | GCFunction, &gcv);
	}
	pp->width = MI_WIN_WIDTH(mi);
	pp->height = MI_WIN_HEIGHT(mi);

	pp->time = 0;
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, pp->width, pp->height);

	random_petal(mi);
}

void
draw_petal(ModeInfo * mi)
{
	petalstruct *pp = &petals[screen];

	if (++pp->time > MI_CYCLES(mi))
		init_petal(mi);
}
