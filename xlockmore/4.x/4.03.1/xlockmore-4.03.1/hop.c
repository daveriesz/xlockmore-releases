
/* -*- Mode: C; tab-width: 4 -*- */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)hop.c	4.03 97/05/10 xlockmore";

#endif

/*-
 * hop.c - Real Plane Fractals for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
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
 * Changes in xlockmore distribution
 * 24-Jun-97: EJK and RR functions stolen from xmartin2.2
 *            Ed Kubaitis <ejk@ux2.cso.uiuc.edu> ejk functions and xmartin 
 *            Renaldo Recuerdo rr function, generalized exponent version
 *              of the Barry Martin's square root function
 * 10-May-97: Compatible with xscreensaver
 * 27-Jul-95: added Peter de Jong's hop from Scientific American
 *            July 87 p. 111.  Sometimes they are amazing but there are a
 *            few duds (I did not see a pattern in the parameters).
 * 29-Mar-95: changed name from hopalong to hop
 * 09-Dec-94: added Barry Martin's sine hop
 * Changes in original xlock
 * 29-Oct-90: fix bad (int) cast.
 * 29-Jul-90: support for multiple screens.
 * 08-Jul-90: new timing and colors and new algorithm for fractals.
 * 15-Dec-89: Fix for proper skipping of {White,Black}Pixel() in colors.
 * 08-Oct-89: Fixed long standing typo bug in RandomInitHop();
 *	      Fixed bug in memory allocation in init_hop();
 *	      Moved seconds() to an extern.
 *	      Got rid of the % mod since .mod is slow on a sparc.
 * 20-Sep-89: Lint.
 * 31-Aug-88: Forked from xlock.c for modularity.
 * 23-Mar-88: Coded HOPALONG routines from Scientific American Sept. 86 p. 14.
 *            Hopalong was attributed to Barry Martin of Aston University
 *            (Birmingham, England)
 */


#ifdef STANDALONE
#define PROGCLASS            "Hop"
#define HACK_INIT            init_hop
#define HACK_DRAW            draw_hop
#define DEF_DELAY            10000
#define DEF_BATCHCOUNT       1000
#define DEF_CYCLES           2500
#define DEF_NCOLORS          200
#define SMOOTH_COLORS
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */

#endif /* STANDALONE */

#define DEF_MARTIN "False"
#define DEF_EJK1 "False"
#define DEF_EJK2 "False"
#define DEF_EJK3 "False"
#define DEF_EJK4 "False"
#define DEF_EJK5 "False"
#define DEF_EJK6 "False"
#define DEF_RR "False"
#define DEF_JONG "False"
#define DEF_SINE "False"

static Bool martin;
static Bool ejk1;
static Bool ejk2;
static Bool ejk3;
static Bool ejk4;
static Bool ejk5;
static Bool ejk6;
static Bool rr;
static Bool jong;
static Bool sine;

#ifndef STANDALONE
static XrmOptionDescRec opts[] =
{
	{"-martin", ".hop.martin", XrmoptionNoArg, (caddr_t) "on"},
	{"+martin", ".hop.martin", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk1", ".hop.ejk1", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk1", ".hop.ejk1", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk2", ".hop.ejk2", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk2", ".hop.ejk2", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk3", ".hop.ejk3", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk3", ".hop.ejk3", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk4", ".hop.ejk4", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk4", ".hop.ejk4", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk5", ".hop.ejk5", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk5", ".hop.ejk5", XrmoptionNoArg, (caddr_t) "off"},
	{"-ejk6", ".hop.ejk6", XrmoptionNoArg, (caddr_t) "on"},
	{"+ejk6", ".hop.ejk6", XrmoptionNoArg, (caddr_t) "off"},
	{"-rr", ".hop.rr", XrmoptionNoArg, (caddr_t) "on"},
	{"+rr", ".hop.rr", XrmoptionNoArg, (caddr_t) "off"},
	{"-jong", ".hop.jong", XrmoptionNoArg, (caddr_t) "on"},
	{"+jong", ".hop.jong", XrmoptionNoArg, (caddr_t) "off"},
	{"-sine", ".hop.sine", XrmoptionNoArg, (caddr_t) "on"},
	{"+sine", ".hop.sine", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(caddr_t *) & martin, "martin", "Martin", DEF_MARTIN, t_Bool},
	{(caddr_t *) & ejk1, "ejk1", "EJK1", DEF_EJK1, t_Bool},
	{(caddr_t *) & ejk2, "ejk2", "EJK2", DEF_EJK2, t_Bool},
	{(caddr_t *) & ejk3, "ejk3", "EJK3", DEF_EJK3, t_Bool},
	{(caddr_t *) & ejk4, "ejk4", "EJK4", DEF_EJK4, t_Bool},
	{(caddr_t *) & ejk5, "ejk5", "EJK5", DEF_EJK5, t_Bool},
	{(caddr_t *) & ejk6, "ejk6", "EJK6", DEF_EJK6, t_Bool},
	{(caddr_t *) & rr, "rr", "RR", DEF_RR, t_Bool},
	{(caddr_t *) & jong, "jong", "Jong", DEF_JONG, t_Bool},
	{(caddr_t *) & sine, "sine", "Sine", DEF_SINE, t_Bool}
};
static OptionStruct desc[] =
{
	{"-/+martin", "turn on/off sqrt format"},
	{"-/+ejk1", "turn on/off ejk1 format"},
	{"-/+ejk2", "turn on/off ejk2 format"},
	{"-/+ejk3", "turn on/off ejk3 format"},
	{"-/+ejk4", "turn on/off ejk4 format"},
	{"-/+ejk5", "turn on/off ejk5 format"},
	{"-/+ejk6", "turn on/off ejk6 format"},
	{"-/+rr", "turn on/off rr format"},
	{"-/+jong", "turn on/off jong format"},
	{"-/+sine", "turn on/off sine format"}
};

ModeSpecOpt hop_opts =
{20, opts, 10, vars, desc};

#endif /* !STANDALONE */

#define MARTIN 0
#define EJK1 1
#define EJK2 2
#define EJK3 3
#define EJK4 4
#define EJK5 5
#define EJK6 6
#define RR 7
#define CP1 8
#define JONG 9
#define SINE 10
#ifdef OFFENDING
#define OPS SINE		/* Sine might be too close to a Swastika for some... */
#else
#define OPS (SINE+1)
#endif

typedef struct {
	int         centerx;
	int         centery;	/* center of the screen */
	double      a;
	double      b;
	double      c;
	double      d;
	double      i;
	double      j;		/* hopalong parameters */
	int         inc;
	int         pix;
	int         op;
	int         count;
	int         bufsize;
	XPoint     *pointBuffer;	/* pointer for XDrawPoints */
} hopstruct;

static hopstruct *hops = NULL;

void
init_hop(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	hopstruct  *hp;
	double      range;

	if (hops == NULL) {
		if ((hops = (hopstruct *) calloc(MI_NUM_SCREENS(mi),
						 sizeof (hopstruct))) == NULL)
			return;
	}
	hp = &hops[MI_SCREEN(mi)];

	hp->centerx = MI_WIN_WIDTH(mi) / 2;
	hp->centery = MI_WIN_HEIGHT(mi) / 2;
	/* Make the other operations less common since they are less interesting */
	if (MI_WIN_IS_FULLRANDOM(mi)) {
		hp->op = NRAND(OPS);
	} else {
		if (martin)
			hp->op = MARTIN;
		else if (ejk1)
			hp->op = EJK1;
		else if (ejk2)
			hp->op = EJK2;
		else if (ejk3)
			hp->op = EJK3;
		else if (ejk4)
			hp->op = EJK4;
		else if (ejk5)
			hp->op = EJK5;
		else if (ejk6)
			hp->op = EJK6;
		else if (rr)
			hp->op = RR;
		else if (jong)
			hp->op = JONG;
		else if (sine)
			hp->op = SINE;
		else
			hp->op = NRAND(OPS);
	}

	switch (hp->op) {
		case MARTIN:
			range = sqrt((double) hp->centerx * hp->centerx +
				     (double) hp->centery * hp->centery) / (10.0 + NRAND(10));

			hp->a = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->b = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->c = (LRAND() / MAXRAND) * range - range / 2.0;
			if (LRAND() & 1)
				hp->c = 0.0;
			/* hp->a = (LRAND() / MAXRAND) * 1500.0 + 40.0;
			   hp->b = (LRAND() / MAXRAND) * 17.0 + 3.0;
			   hp->c = (LRAND() / MAXRAND) * 3000.0 + 100.0; */
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "sqrt a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK1:
			hp->a = (LRAND() / MAXRAND) * 500.0;
			hp->b = (LRAND() / MAXRAND) * 0.4;
			hp->c = (LRAND() / MAXRAND) * 100.0 + 10.0;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk1 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK2:
			hp->a = (LRAND() / MAXRAND) * 500.0;
			hp->b = pow(10.0, 6.0 + (LRAND() / MAXRAND) * 24.0);
			if (LRAND() & 1)
				hp->b = hp->b;
			hp->c = pow(10.0, (LRAND() / MAXRAND) * 9.0);
			if (LRAND() & 1)
				hp->c = hp->c;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk2 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK3:
			hp->a = (LRAND() / MAXRAND) * 500.0;
			hp->b = (LRAND() / MAXRAND) * 0.35 + 0.5;
			hp->c = (LRAND() / MAXRAND) * 80.0 + 30.0;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk3 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK4:
			hp->a = (LRAND() / MAXRAND) * 1000.0;
			hp->b = (LRAND() / MAXRAND) * 9.0 + 1.0;
			hp->c = (LRAND() / MAXRAND) * 40.0 + 30.0;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk4 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK5:
			hp->a = (LRAND() / MAXRAND) * 600.0;
			hp->b = (LRAND() / MAXRAND) * 0.3 + 0.1;
			hp->c = (LRAND() / MAXRAND) * 90.0 + 20.0;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk5 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK6:
			hp->a = (LRAND() / MAXRAND) * 100.0 + 550.0;
			hp->b = (LRAND() / MAXRAND) + 0.5;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "ejk6 a=%g, b=%g\n", hp->a, hp->b);
			break;
		case RR:
			hp->a = (LRAND() / MAXRAND) * 100.0;
			hp->b = (LRAND() / MAXRAND) * 20.0;
			hp->c = (LRAND() / MAXRAND) * 200.0;
			hp->d = (LRAND() / MAXRAND) * 0.9;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "rr a=%g, b=%g, c=%g, d=%g\n",
					       hp->a, hp->b, hp->c, hp->d);
			break;
		case JONG:
			hp->a = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->b = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->c = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->d = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "jong a=%g, b=%g, c=%g, d=%g\n",
					       hp->a, hp->b, hp->c, hp->d);
			break;
		case SINE:	/* MARTIN2 */
			hp->a = M_PI + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.7;
			/* hp->a = (LRAND() / MAXRAND) * 0.14 + MI_PI - .07; */
			if (MI_WIN_IS_VERBOSE(mi))
				(void) fprintf(stdout, "sine a=%g\n", hp->a);
			break;
	}
	if (MI_NPIXELS(mi) > 2)
		hp->pix = NRAND(MI_NPIXELS(mi));
	hp->i = hp->j = 0.0;
	hp->inc = (int) ((LRAND() / MAXRAND) * 200) - 100;
	hp->bufsize = MI_BATCHCOUNT(mi);

	if (!hp->pointBuffer)
		hp->pointBuffer = (XPoint *) malloc(hp->bufsize * sizeof (XPoint));

	XClearWindow(display, MI_WINDOW(mi));

	XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	hp->count = 0;
}


void
draw_hop(ModeInfo * mi)
{
	hopstruct  *hp = &hops[MI_SCREEN(mi)];
	double      oldj, oldi;
	XPoint     *xp = hp->pointBuffer;
	int         k = hp->bufsize;

	hp->inc++;
	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_PIXEL(mi, hp->pix));
		if (++hp->pix >= MI_NPIXELS(mi))
			hp->pix = 0;
	}
	while (k--) {
		oldj = hp->j;
		switch (hp->op) {
			case MARTIN:	/* SQRT, MARTIN1 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj + ((hp->i < 0)
					   ? sqrt(fabs(hp->b * oldi - hp->c))
					: -sqrt(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK1:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? (hp->b * oldi - hp->c) :
						-(hp->b * oldi - hp->c));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK2:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i < 0) ? log(fabs(hp->b * oldi - hp->c)) :
					   -log(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK3:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
						-sin(hp->b * oldi) - hp->c);
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK4:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
					  -sqrt(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK5:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
						-(hp->b * oldi - hp->c));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK6:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - asin((hp->b * oldi) - (long) (hp->b * oldi));
				xp->x = (int) (hp->i);
				xp->y = (int) (hp->j);
				break;
			case RR:	/* RR1 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i < 0) ? -pow(fabs(hp->b * oldi - hp->c), hp->d) :
				     pow(fabs(hp->b * oldi - hp->c), hp->d));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case JONG:
				if (hp->centerx > 0)
					oldi = hp->i + 4 * hp->inc / hp->centerx;
				else
					oldi = hp->i;
				hp->j = sin(hp->c * hp->i) - cos(hp->d * hp->j);
				hp->i = sin(hp->a * oldj) - cos(hp->b * oldi);
				xp->x = hp->centerx + (int) (hp->centerx * (hp->i + hp->j) / 4.0);
				xp->y = hp->centery - (int) (hp->centery * (hp->i - hp->j) / 4.0);
				break;
			case SINE:	/* MARTIN2 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - sin(oldi);
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
		}
		xp++;
	}
	XDrawPoints(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
		    hp->pointBuffer, hp->bufsize, CoordModeOrigin);
	if (++hp->count > MI_CYCLES(mi))
		init_hop(mi);
}

void
release_hop(ModeInfo * mi)
{
	if (hops != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			hopstruct  *hp = &hops[screen];

			if (hp->pointBuffer)
				(void) free((void *) hp->pointBuffer);
		}

		(void) free((void *) hops);
		hops = NULL;
	}
}

void
refresh_hop(ModeInfo * mi)
{
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}
