/* -*- Mode: C; tab-width: 4 -*- */
/* turtle --- fractal curves */

#if 0
static const char sccsid[] = "@(#)turtle.c	5.00 2000/11/01 xlockmore";

#endif

/*-
 * Copyright (c) 1996 by David Bagley.
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
 * 30-Nov-2025: Added a few more turtle like curves from internet
 * 01-Nov-2000: Allocation checks
 * 10-May-1997: Compatible with xscreensaver
 * 01-Dec-1996: Not too proud how I hacked in 2 more curves
 * 30-Sep-1996: started with Hilbert curve, David Bagley
 * From Fractal Programming in C by Roger T. Stevens
 */

#ifdef STANDALONE
#define MODE_turtle
#define DEFAULTS "*delay: 1000000 \n" \
	"*cycles: 20 \n" \
	"*ncolors: 64 \n" \

# define free_turtle 0
# define reshape_turtle 0
# define turtle_handle_event 0
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */

#ifdef MODE_turtle

ENTRYPOINT ModeSpecOpt turtle_opts =
{0, (XrmOptionDescRec *) NULL, 0, (argtype *) NULL, (OptionStruct *) NULL};

#ifdef USE_MODULES
ModStruct   turtle_description =
{"turtle", "init_turtle", "draw_turtle", "release_turtle",
 "init_turtle", "init_turtle", (char *) NULL, &turtle_opts,
 1000000, 1, 20, 1, 64, 1.0, "",
 "Shows turtle fractals", 0, NULL};

#endif

#define HILBERT 0		/* Not exactly a turtle algorithm... p199 */
#define HARTER_HEIGHWAY 1	/* Dragon curve p292 */
#define CESARO_VAR 2		/* Telephone like curve p168 (similar to Levy C curve) */
#define LEVY_C 3		/* Not exactly a turtle algorithm */
#define KOCH_SNOWFLAKE 4
#define SIERPINSKI 5
#define SIERPINSKI_ARROWHEAD 6
#define TURTLE_CURVES 7

/* https://en.wikipedia.org/wiki/Hilbert_curve */
/* HILBERT */
#define ALPHA_0 "A B"
#define CONST_0 "F + -"
#define AXIOM_0 "A" /* U */
#define PROD_0_A "+BF-AFA-FB+"
#define PROD_0_B "+AF-BFB-FA+"
#define DEG_0 90

/* https://en.wikipedia.org/wiki/Dragon_curve */
/* HARTER HEIGHWAY (Dragon curve) */
#define ALPHA_1 "F H"
#define CONST_1 "+ -"
#define AXIOM_1 "F"
#define PROD_1_F "F-H"
#define PROD_1_H "F+H"
#define DEG_1 90

/* https://en.wikipedia.org/wiki/Lévy_C_curve */
/* LEVY C (similar to Cesaro Variant) */
#define ALPHA_2 "F"
#define CONST_2 "+ -"
#define AXIOM_2 "F" /* a line */
#define PROD_2_F "-F++F-"
#define DEG_2 45

/* https://en.wikipedia.org/wiki/Koch_snowflake */
#define ALPHA_4 "F"
#define CONST_4 "+ -"
#define AXIOM_4 "F" /* Snowflake "F--F--F" */
#define PROD_4_F "F+F--F+F"
#define DEG_4 60

/* https://en.wikipedia.org/wiki/Sierpiński_curve */
/* SIERPINSKI */
#define ALPHA_5 "F G X"
#define CONST_5 "F G + -"
#define AXIOM_5 "F--XF--F--XF"
#define PROD_5_X "XF+G+XF--F--XF+G+X"
#define DEG_5 45

/* SIERPINSKI ARROWHEAD */
#define ALPHA_6 "X Y"
#define CONST_6 "F + -"
#define AXIOM_6 "XF"
#define PROD_6_X "YF+XF+Y"
#define PROD_6_Y "XF−YF−X"
#define DEG_6 60


#define POINT(x_1,y_1,x_2,y_2) (((x_2)==(x_1))?(((y_2)>(y_1))?90.0:270.0):\
  ((x_2)>(x_1))?(atan(((y_2)-(y_1))/((x_2)-(x_1)))*(180.0/M_PI)):\
  (atan(((y_2)-(y_1))/((x_2)-(x_1)))*(180.0/M_PI)+180.0))
#define TURN(theta, angle) ((theta)+=(angle))
#define STEP(x, y, r, theta) ((x)+=(r)*cos((theta)*M_PI/180.0)),\
  ((y)+=(r)*sin((theta)*M_PI/180.0))

typedef struct {
	char alphabet[16];
	char constants[16];
	char axiom[16];
	char production_rules[4][32];
	int degrees;
} lsystemtype;

typedef struct {
	double      r, theta, x, y;
} turtletype;

typedef struct {
	double      x, y;
} complextype;

typedef struct {
	int         width, height;
	int         time;
	int         level;
	int         curve;
	int         r, maxlevel, min, dir;
	XPoint      pt1, pt2;
	XPoint      start;
	turtletype  turtle;
	int         sign;
	int         lineWidth;
} turtlestruct;

static turtlestruct *turtles = (turtlestruct *) NULL;

static void
generate_hilbert(ModeInfo * mi, int r1, int r2)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];

	tp->level--;

	if (tp->level > 0)
		generate_hilbert(mi, r2, r1);

	tp->pt2.x += r1;
	tp->pt2.y += r2;
	XDrawLine(display, window, gc,
		tp->pt1.x, tp->pt1.y, tp->pt2.x, tp->pt2.y);
	tp->pt1.x = tp->pt2.x;
	tp->pt1.y = tp->pt2.y;

	if (tp->level > 0)
		generate_hilbert(mi, r1, r2);

	tp->pt2.x += r2;
	tp->pt2.y += r1;
	XDrawLine(display, window, gc,
		 tp->pt1.x, tp->pt1.y, tp->pt2.x, tp->pt2.y);
	tp->pt1.x = tp->pt2.x;
	tp->pt1.y = tp->pt2.y;

	if (tp->level > 0)
		generate_hilbert(mi, r1, r2);

	tp->pt2.x -= r1;
	tp->pt2.y -= r2;
	XDrawLine(display, window, gc,
		 tp->pt1.x, tp->pt1.y, tp->pt2.x, tp->pt2.y);
	tp->pt1.x = tp->pt2.x;
	tp->pt1.y = tp->pt2.y;

	if (tp->level > 0)
		generate_hilbert(mi, -r2, -r1);

	tp->level++;
}

static void
generate_harter_heightway(ModeInfo * mi, double pt1x, double pt1y,
			double pt2x, double pt2y, int level, int sign)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];
	complextype points[4];
	int         sign2 = -1;

	level--;

	tp->turtle.r = sqrt((double) ((pt2x - pt1x) * (pt2x - pt1x) +
				      (pt2y - pt1y) * (pt2y - pt1y)) / 2.0);
	points[0].x = pt1x;
	points[0].y = pt1y;
	points[2].x = pt2x;
	points[2].y = pt2y;
	tp->turtle.theta = POINT(pt1x, pt1y, pt2x, pt2y);
	tp->turtle.x = pt1x;
	tp->turtle.y = pt1y;
	TURN(tp->turtle.theta, 45.0 * (double) sign);
	STEP(tp->turtle.x, tp->turtle.y, tp->turtle.r, tp->turtle.theta);
	points[1].x = tp->turtle.x;
	points[1].y = tp->turtle.y;
	if (level > 0) {
		int         j;

		for (j = 0; j < 2; j++) {
			pt1x = points[j].x;
			pt2x = points[j + 1].x;
			pt1y = points[j].y;
			pt2y = points[j + 1].y;
			generate_harter_heightway(mi, pt1x, pt1y, pt2x, pt2y, level, sign2);
			sign2 *= -1;
		}
	} else {
		XDrawLine(display, window, gc,
			(int) points[0].x + tp->start.x, (int) points[0].y + tp->start.y,
			(int) points[1].x + tp->start.x, (int) points[1].y + tp->start.y);
		XDrawLine(display, window, gc,
			(int) points[1].x + tp->start.x, (int) points[1].y + tp->start.y,
			(int) points[2].x + tp->start.x, (int) points[2].y + tp->start.y);
	}
}

static void
generate_cesaro_variant(ModeInfo * mi, double pt1x, double pt1y,
		 double pt2x, double pt2y, double angle, int level, int sign)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];
	complextype points[4];

	level--;

	tp->turtle.r = sqrt((double) ((pt2x - pt1x) * (pt2x - pt1x) +
				(pt2y - pt1y) * (pt2y - pt1y))) / 2.0;
	points[0].x = pt1x;
	points[0].y = pt1y;
	points[2].x = pt2x;
	points[2].y = pt2y;
	tp->turtle.theta = POINT(pt1x, pt1y, pt2x, pt2y);
	tp->turtle.x = pt1x;
	tp->turtle.y = pt1y;
	STEP(tp->turtle.x, tp->turtle.y, tp->turtle.r, tp->turtle.theta);
	points[3].x = tp->turtle.x;
	points[3].y = tp->turtle.y;
	TURN(tp->turtle.theta, angle * (double) sign);
	STEP(tp->turtle.x, tp->turtle.y, tp->turtle.r, tp->turtle.theta);
	points[1].x = tp->turtle.x;
	points[1].y = tp->turtle.y;
	sign = -1;
	if (level > 0) {
		int         j;

		for (j = 0; j < 2; j++) {
			pt1x = points[j].x;
			pt2x = points[j + 1].x;
			pt1y = points[j].y;
			pt2y = points[j + 1].y;
			generate_cesaro_variant(mi, pt1x, pt1y, pt2x, pt2y, angle, level, sign);
		}
	} else {
		XDrawLine(display, window, gc,
			(int) points[0].x + tp->start.x, (int) points[0].y + tp->start.y,
			(int) points[2].x + tp->start.x, (int) points[2].y + tp->start.y);
		XDrawLine(display, window, gc,
			(int) points[1].x + tp->start.x, (int) points[1].y + tp->start.y,
			(int) points[3].x + tp->start.x, (int) points[3].y + tp->start.y);
	}
}

static void
generate_levy_c(ModeInfo * mi, double x, double y, double len, double angle,
		int dir, int level)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	double fx = x;
	double fy = y;
	double fxPrime, fyPrime;
	double length = len;
	double alpha = angle;
	int sign = (dir < 2) ? 1 : -1;
	if (level > 1) {
		length = (length / M_SQRT2);
		generate_levy_c(mi, fx, fy, length, (alpha + 45), dir, (level - 1));
		if (dir % 2 == 0) {
			fxPrime = (fx + (sign * length * sin((alpha + 45) * M_PI / 180.0)));
			fyPrime = (fy + (sign * length * cos((alpha + 45) * M_PI / 180.0)));
		} else {
			fxPrime = (fx + (sign * length * cos((alpha + 45) * M_PI / 180.0)));
			fyPrime = (fy + (sign * length * sin((alpha + 45) * M_PI / 180.0)));
		}
		generate_levy_c(mi, fxPrime, fyPrime, length, (alpha - 45), dir, (level - 1));
	} else {
		if (dir % 2 == 0) {
			fxPrime = (fx + (sign * length * sin(alpha * M_PI / 180.0)));
			fyPrime = (fy + (sign * length * cos(alpha * M_PI / 180.0)));
		} else {
			fxPrime = (fx + (sign * length * cos(alpha * M_PI / 180.0)));
			fyPrime = (fy + (sign * length * sin(alpha * M_PI / 180.0)));
		}
		XDrawLine(display, window, gc, (int) fx, (int) fy,
			(int) fxPrime, (int) fyPrime);
	}
}

static void
generate_koch(ModeInfo * mi, double x, double y, double length, int level)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];
	if (level > 1) {
		length /= 3.0;
		generate_koch(mi, x, y, length, level - 1);
		TURN(tp->turtle.theta, -60);
		x = tp->turtle.x;
		y = tp->turtle.y;
		generate_koch(mi, x, y, length, level - 1);
		TURN(tp->turtle.theta, 120);
		x = tp->turtle.x;
		y = tp->turtle.y;
		generate_koch(mi, x, y, length, level - 1);
		TURN(tp->turtle.theta, -60);
		x = tp->turtle.x;
		y = tp->turtle.y;
		generate_koch(mi, x, y, length, level - 1);
	} else {
		tp->turtle.x = x;
		tp->turtle.y = y;
		STEP(tp->turtle.x, tp->turtle.y, length, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
	}
}

static void
generate_koch_snowflake(ModeInfo * mi, double x, double y,
		 double length, int level)
{
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];
	int i;

	for (i = 0 ; i < 3; i++) {
		generate_koch(mi, x, y, length, level);
		TURN(tp->turtle.theta, 120);
		x = tp->turtle.x;
		y = tp->turtle.y;
	}
}

static void
generate_sierpinski_part(ModeInfo * mi, double x, double y,
		 double length, double diag, int level)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];

	if (level > 0) {
		tp->turtle.x = x;
		tp->turtle.y = y;

		generate_sierpinski_part(mi, x, y, length, diag, level - 1);

		TURN(tp->turtle.theta, -45);
		x = tp->turtle.x;
		y = tp->turtle.y;
		STEP(tp->turtle.x, tp->turtle.y, diag, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
		x = tp->turtle.x;
		y = tp->turtle.y;
		TURN(tp->turtle.theta, -45);

		generate_sierpinski_part(mi, x, y, length, diag, level - 1);

		TURN(tp->turtle.theta, 90);
		x = tp->turtle.x;
		y = tp->turtle.y;
		STEP(tp->turtle.x, tp->turtle.y, length, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
		x = tp->turtle.x;
		y = tp->turtle.y;
		TURN(tp->turtle.theta, 90);

		generate_sierpinski_part(mi, x, y, length, diag, level - 1);

		TURN(tp->turtle.theta, -45);
		x = tp->turtle.x;
		y = tp->turtle.y;
		STEP(tp->turtle.x, tp->turtle.y, diag, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
		x = tp->turtle.x;
		y = tp->turtle.y;
		TURN(tp->turtle.theta, -45);

		generate_sierpinski_part(mi, x, y, length, diag, level - 1);
	}
}

static void
generate_sierpinski(ModeInfo * mi, double x, double y,
		 double length, int level)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];
	double diag = (length / M_SQRT2);
	int i;

	if (level > 0) {
		length = length / (1 << (level - 1));
		diag = diag / (1 << (level - 1));
	}
	x += length;
	y -= length / 2;
	for (i = 0 ; i < 4; i++) {
		generate_sierpinski_part(mi, x, y, length, diag, level);

		TURN(tp->turtle.theta, -45);
		x = tp->turtle.x;
		y = tp->turtle.y;
		STEP(tp->turtle.x, tp->turtle.y, diag, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
		x = tp->turtle.x;
		y = tp->turtle.y;

		TURN(tp->turtle.theta, -45);
	}
}

static void
generate_sierpinski_arrowhead(ModeInfo * mi, double x, double y,
		double length, double angle, int level)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	turtlestruct *tp = &turtles[MI_SCREEN(mi)];

	if (level > 0) {
		generate_sierpinski_arrowhead(mi, x, y, length / 2, -angle, level - 1);
		TURN(tp->turtle.theta, angle);
		x = tp->turtle.x;
		y = tp->turtle.y;
		generate_sierpinski_arrowhead(mi, x, y, length / 2, angle, level - 1);
		TURN(tp->turtle.theta, angle);
		x = tp->turtle.x;
		y = tp->turtle.y;
		generate_sierpinski_arrowhead(mi, x, y, length / 2, -angle, level - 1);
	} else {
		tp->turtle.x = x;
		tp->turtle.y = y;
		STEP(tp->turtle.x, tp->turtle.y, length, tp->turtle.theta);
		XDrawLine(display, window, gc, (int) x, (int) y,
			(int) tp->turtle.x, (int) tp->turtle.y);
	}
}

ENTRYPOINT void
init_turtle(ModeInfo * mi)
{
	int	 i;
	turtlestruct *tp;

	MI_INIT(mi, turtles);
	tp = &turtles[MI_SCREEN(mi)];

	tp->width = MI_WIDTH(mi);
	tp->height = MI_HEIGHT(mi);
	tp->min = MIN(tp->width, tp->height);
	tp->maxlevel = 0;
	i = tp->min;
	do {
		tp->r = i;
		tp->maxlevel++;
		i = (tp->min - 1) / (1 << tp->maxlevel);
	} while (i > 1);
	tp->maxlevel--;
	tp->r = tp->min = (tp->r << tp->maxlevel);
	tp->width = MAX(tp->width, tp->min + 1);
	tp->height = MAX(tp->height, tp->min + 1);
	tp->lineWidth = log(tp->min) + 1;
	tp->curve = NRAND(TURTLE_CURVES);
	tp->dir = NRAND(4);
	switch (tp->curve) {
	case HILBERT:
		tp->start.x = NRAND(tp->width - tp->min);
		tp->start.y = NRAND(tp->height - tp->min);
		break;
	case CESARO_VAR:
		tp->r <<= 6;
		tp->min = 3 * tp->min / 4;
		tp->start.x = tp->width / 2;
		tp->start.y = tp->height / 2;
		switch (NRAND(4)) {
		case 0:
			tp->pt1.x = -tp->min / 2;
			tp->pt1.y = 0;
			tp->pt2.x = tp->min / 2;
			tp->pt2.y = 0;
			tp->start.y -= tp->min / 6;
			break;
		case 1:
			tp->pt1.x = 0;
			tp->pt1.y = -tp->min / 2;
			tp->pt2.x = 0;
			tp->pt2.y = tp->min / 2;
			tp->start.x += tp->min / 6;
			break;
		case 2:
			tp->pt1.x = tp->min / 2;
			tp->pt1.y = 0;
			tp->pt2.x = -tp->min / 2;
			tp->pt2.y = 0;
			tp->start.y += tp->min / 6;
			break;
		case 3:
			tp->pt1.x = 0;
			tp->pt1.y = tp->min / 2;
			tp->pt2.x = 0;
			tp->pt2.y = -tp->min / 2;
			tp->start.x -= tp->min / 6;
			break;
		}
		break;
	case HARTER_HEIGHWAY:
		tp->r <<= 6;
		tp->min = 3 * tp->min / 4;
		tp->start.x = tp->width / 2;
		tp->start.y = tp->height / 2;
		switch (NRAND(4)) {
		case 0:
			tp->pt1.x = -tp->min / 2;
			tp->pt1.y = -tp->min / 12;
			tp->pt2.x = tp->min / 2;
			tp->pt2.y = -tp->min / 12;
			break;
		case 1:
			tp->pt1.x = tp->min / 12;
			tp->pt1.y = -tp->min / 2;
			tp->pt2.x = tp->min / 12;
			tp->pt2.y = tp->min / 2;
			break;
		case 2:
			tp->pt1.x = tp->min / 2;
			tp->pt1.y = tp->min / 12;
			tp->pt2.x = -tp->min / 2;
			tp->pt2.y = tp->min / 12;
			break;
		case 3:
			tp->pt1.x = -tp->min / 12;
			tp->pt1.y = tp->min / 2;
			tp->pt2.x = -tp->min / 12;
			tp->pt2.y = -tp->min / 2;
			break;
		}
		break;
	case LEVY_C:
		tp->r <<= 6;
		tp->min = 3 * tp->min / 4;
		tp->start.x = tp->width / 2;
		tp->start.y = tp->height / 2;
		switch (tp->dir) {
		case 1:
		case 2:
			tp->start.x += tp->width / 8;
			break;
		case 0:
		case 3:
			tp->start.x -= tp->width / 8;
			tp->start.y += 7 * tp->height / 8;
			break;
		}
		break;
	case KOCH_SNOWFLAKE:
		tp->r >>= 1;
		tp->min = 3 * tp->min / 4;
		tp->start.x = NRAND(tp->width - tp->min);
		tp->start.y = NRAND(tp->height - tp->min) / 2 + tp->height / 4;
		if ((tp->dir & 1) == 1) {
			TURN(tp->turtle.theta, -60);
			tp->start.y += tp->height / 3;
		}
		break;
	case SIERPINSKI:
		tp->r >>= 1;
		tp->min = 2 * tp->min / 3;
		tp->start.x = NRAND(tp->width - tp->min);
		tp->start.y = NRAND(tp->height - tp->min) / 2 + 3 * tp->height / 8;
		break;
	case SIERPINSKI_ARROWHEAD:
		tp->min = 3 * tp->min / 4;
		tp->start.x = NRAND(tp->width - tp->min);
		tp->start.y = NRAND(tp->height - tp->min) / 2 + 3 * tp->height / 4;
		break;
	}
	tp->level = 0;
	tp->sign = 1;
	tp->time = 0;

	MI_CLEARWINDOW(mi);

	if (MI_NPIXELS(mi) <= 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
}

ENTRYPOINT void
draw_turtle(ModeInfo * mi)
{
	turtlestruct *tp;

	if (turtles == NULL)
		return;
	tp = &turtles[MI_SCREEN(mi)];

	if (++tp->time > MI_CYCLES(mi))
		init_turtle(mi);

	MI_IS_DRAWN(mi) = True;

	if (MI_NPIXELS(mi) > 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			MI_PIXEL(mi, NRAND(MI_NPIXELS(mi))));

	tp->r = tp->r >> 1;
	tp->level++;
	if (tp->lineWidth > 1) {
		tp->lineWidth--;
		XSetLineAttributes(MI_DISPLAY(mi), MI_GC(mi), tp->lineWidth,
			LineSolid, CapRound, JoinRound);
	}
	if (tp->r > 1)
		switch (tp->curve) {
		case HILBERT:
			switch (tp->dir) {
			case 0:
				tp->pt1.x = tp->pt2.x = tp->start.x + tp->r / 2;
				tp->pt1.y = tp->pt2.y = tp->start.y + tp->r / 2;
				generate_hilbert(mi, 0, tp->r);
				break;
			case 1:
				tp->pt1.x = tp->pt2.x = tp->start.x + tp->min - tp->r / 2;
				tp->pt1.y = tp->pt2.y = tp->start.y + tp->min - tp->r / 2;
				generate_hilbert(mi, 0, -tp->r);
				break;
			case 2:
				tp->pt1.x = tp->pt2.x = tp->start.x + tp->min - tp->r / 2;
				tp->pt1.y = tp->pt2.y = tp->start.y + tp->min - tp->r / 2;
				generate_hilbert(mi, -tp->r, 0);
				break;
			case 3:
				tp->pt1.x = tp->pt2.x = tp->start.x + tp->r / 2;
				tp->pt1.y = tp->pt2.y = tp->start.y + tp->r / 2;
				generate_hilbert(mi, tp->r, 0);
				break;
			}
			break;
		case HARTER_HEIGHWAY:
			generate_harter_heightway(mi,
				(double) tp->pt1.x, (double) tp->pt1.y,
				(double) tp->pt2.x, (double) tp->pt2.y,
				tp->level, tp->sign);
			break;
		case CESARO_VAR:
			generate_cesaro_variant(mi,
				(double) tp->pt1.x, (double) tp->pt1.y,
				(double) tp->pt2.x, (double) tp->pt2.y,
				90.0, tp->level, tp->sign);
			break;
		case LEVY_C:
			generate_levy_c(mi,
				tp->start.x, tp->start.y / 2,
				5 * tp->min / 8, 90, tp->dir, tp->level);
			break;
		case KOCH_SNOWFLAKE:
			generate_koch_snowflake(mi, tp->start.x, tp->start.y,
				tp->min, tp->level);
			break;
		case SIERPINSKI:
			generate_sierpinski(mi,
				tp->start.x / 2, tp->start.y * 2,
				3 * tp->min / 8,
				tp->level);
			break;
		case SIERPINSKI_ARROWHEAD:
			TURN(tp->turtle.theta, -60);
			generate_sierpinski_arrowhead(mi, tp->start.x, tp->start.y,
					tp->min, 60, tp->level);
			break;
		}
	XSetLineAttributes(MI_DISPLAY(mi), MI_GC(mi), 1,
		LineSolid, CapButt, JoinMiter);
}

ENTRYPOINT void
release_turtle(ModeInfo * mi)
{
	if (turtles != NULL) {
		free(turtles);
		turtles = (turtlestruct *) NULL;
	}
}

XSCREENSAVER_MODULE ("Turtle", turtle)

#endif /* MODE_turtle */
