#ifndef lint
static char sccsid[] = "@(#)hop.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * hop.c - Real Plane Fractals for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 * 27-Jul-95: added Peter de Jong's hop from Scientific American
 *            July 87 p. 111.  Sometimes they are amazing but there are a
 *            few duds (I did not see a pattern in the parameters).
 * 29-Mar-95: changed name from hopalong to hop
 * 09-Dec-94: added sine hop
 *
 * Changes of Patrick J. Naughton
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
 */

#include "xlock.h"
#include <math.h>

#define SQRT 0
#define SIN 1
#define JONG 2
#define OPS 3

ModeSpecOpt hop_opts =
{0, NULL, NULL, NULL};

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
} hopstruct;

static hopstruct hops[MAXSCREENS];
static XPoint *pointBuffer = 0;	/* pointer for XDrawPoints */

void
init_hop(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	double      range;
	hopstruct  *hp = &hops[screen];

	hp->centerx = MI_WIN_WIDTH(mi) / 2;
	hp->centery = MI_WIN_HEIGHT(mi) / 2;
	/* Make the other operations less common since they are less interesting */
	if (LRAND() % 6)
		hp->op = SQRT;
	else
		hp->op = LRAND() % (OPS - 1) + 1;
	switch (hp->op) {
		case SQRT:
			range = sqrt((double) hp->centerx * hp->centerx +
				     (double) hp->centery * hp->centery) /
				(10.0 + LRAND() % 10);

			hp->a = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->b = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->c = (LRAND() / MAXRAND) * range - range / 2.0;
			if (LRAND() & 1)
				hp->c = 0.0;
			break;
		case SIN:
			hp->a = M_PI + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.7;
			break;
		case JONG:
			hp->a = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->b = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->c = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			hp->d = (LRAND() / MAXRAND) * 2.0 * M_PI - M_PI;
			break;
	}
	hp->pix = 0;
	hp->i = hp->j = 0.0;
	hp->inc = (int) ((LRAND() / MAXRAND) * 200) - 100;
	hp->bufsize = MI_BATCHCOUNT(mi);

	if (!pointBuffer)
		pointBuffer = (XPoint *) malloc(hp->bufsize * sizeof (XPoint));

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0,
		       hp->centerx * 2, hp->centery * 2);
	XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	hp->count = 0;
}


void
draw_hop(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	double      oldj, oldi;
	XPoint     *xp = pointBuffer;
	hopstruct  *hp = &hops[screen];
	int         k = hp->bufsize;

	hp->inc++;
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[hp->pix]);
		if (++hp->pix >= Scr[screen].npixels)
			hp->pix = 0;
	}
	while (k--) {
		oldj = hp->j;
		switch (hp->op) {
			case SQRT:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj + ((hp->i < 0)
					   ? sqrt(fabs(hp->b * oldi - hp->c))
					: -sqrt(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case SIN:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - sin(oldi);
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case JONG:
				oldi = hp->i + 4 * hp->inc / hp->centerx;
				hp->j = sin(hp->c * hp->i) - cos(hp->d * hp->j);
				hp->i = sin(hp->a * oldj) - cos(hp->b * oldi);
				xp->x = hp->centerx + (int) hp->centerx * (hp->i + hp->j) / 4;
				xp->y = hp->centery - (int) hp->centery * (hp->i - hp->j) / 4;
				break;
		}
		xp++;
	}
	XDrawPoints(dsp, win, Scr[screen].gc,
		    pointBuffer, hp->bufsize, CoordModeOrigin);
	if (++hp->count > MI_CYCLES(mi))
		init_hop(mi);
}
