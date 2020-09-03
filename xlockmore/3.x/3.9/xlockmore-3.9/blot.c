
#ifndef lint
static char sccsid[] = "@(#)blot.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * Rorschach's ink blot test
 *
 * Copyright (c) 1992 by Jamie Zawinski
 *
 * 05-Jan-95: patch for Dual-Headed machines from Greg Onufer
 *            <Greg.Onufer@Eng.Sun.COM>
 * 07-Dec-94: now randomly has xsym, ysym, or both.
 * 02-Sep-93: xlock version (David Bagley bagleyd@hertz.njit.edu)
 * 1992:      xscreensaver version (Jamie Zawinski jwz@netscape.com)
 */

/*-
 * original copyright
 * Copyright (c) 1992 by Jamie Zawinski
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "xlock.h"

ModeSpecOpt blot_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         width;
	int         height;
	int         xmid, ymid;
	int         offset;
	int         xsym, ysym;
	int         size;
	int         pix;
	int         count;
} blotstruct;

static blotstruct blots[MAXSCREENS];
static XPoint *pointBuffer = 0;	/* pointer for XDrawPoints */
static unsigned long pointBufferSize = 0;

void
init_blot(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	blotstruct *bp = &blots[screen];

	bp->width = MI_WIN_WIDTH(mi);
	bp->height = MI_WIN_HEIGHT(mi);
	bp->xmid = bp->width / 2;
	bp->ymid = bp->height / 2;

	bp->offset = 4;
	bp->ysym = LRAND() & 1;
	bp->xsym = (bp->ysym) ? LRAND() & 1 : 1;
	bp->pix = 0;
	if (bp->offset <= 0)
		bp->offset = 3;
	bp->size = MI_BATCHCOUNT(mi);
	if (bp->size >= 100 || bp->size < 1)
		bp->size = 6;
	/* Fudge the size so it takes up the whole screen */
	bp->size *= bp->width * bp->height / 1024;

	if (!pointBuffer || pointBufferSize < (bp->size * sizeof (XPoint))) {
		if (pointBuffer != NULL)
			(void) free((void *) pointBuffer);
		pointBuffer = (XPoint *) malloc(bp->size * sizeof (XPoint));
		pointBufferSize = bp->size * sizeof (XPoint);
	}
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, bp->width, bp->height);
	XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	bp->count = 0;
}

void
draw_blot(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         x, y;
	int         k;
	XPoint     *xp = pointBuffer;
	blotstruct *bp = &blots[screen];

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[bp->pix]);
		if (++bp->pix >= Scr[screen].npixels)
			bp->pix = 0;
	}
	x = bp->xmid;
	y = bp->ymid;
	k = bp->size;
	while (k >= 4) {
		x += ((LRAND() % (1 + (bp->offset << 1))) - bp->offset);
		y += ((LRAND() % (1 + (bp->offset << 1))) - bp->offset);
		k--;
		xp->x = x;
		xp->y = y;
		xp++;
		if (bp->xsym) {
			k--;
			xp->x = bp->width - x;
			xp->y = y;
			xp++;
		}
		if (bp->ysym) {
			k--;
			xp->x = x;
			xp->y = bp->height - y;
			xp++;
		}
		if (bp->xsym && bp->ysym) {
			k--;
			xp->x = bp->width - x;
			xp->y = bp->height - y;
			xp++;
		}
	}
	XDrawPoints(dsp, win, Scr[screen].gc,
		    pointBuffer, bp->size - k, CoordModeOrigin);
	if (++bp->count > MI_CYCLES(mi))
		init_blot(mi);
}
