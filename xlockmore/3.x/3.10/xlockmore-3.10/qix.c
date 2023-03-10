
#ifndef lint
static char sccsid[] = "@(#)qix.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * qix.c - Vector swirl for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 29-Jul-90: support for multiple screens.
 *	      made check_bounds_?() a macro.
 *	      fixed initial parameter setup.
 * 15-Dec-89: Fix for proper skipping of {White,Black}Pixel() in colors.
 * 08-Oct-89: Fixed bug in memory allocation in init_qix().
 *	      Moved seconds() to an extern.
 * 23-Sep-89: Switch to random() and fixed bug w/ less than 4 lines.
 * 20-Sep-89: Lint.
 * 24-Mar-89: Written.
 */

#include "xlock.h"

ModeSpecOpt qix_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         pix;
	int         first;
	int         last;
	int         dx1;
	int         dy1;
	int         dx2;
	int         dy2;
	int         x1;
	int         y1;
	int         x2;
	int         y2;
	int         offset;
	int         delta;
	int         width;
	int         height;
	int         nlines;
	XPoint     *lineq;
} qixstruct;

static qixstruct *qixs = NULL;

#define check_bounds(qp, val, del, max)				\
{								\
    if ((val) < 0) {						\
	*(del) = NRAND((qp)->delta) + (qp)->offset;	\
    } else if ((val) > (max)) {					\
	*(del) = -(NRAND((qp)->delta)) - (qp)->offset;	\
    }								\
}

void
init_qix(ModeInfo * mi)
{
	qixstruct  *qp;

	if (qixs == NULL) {
		if ((qixs = (qixstruct *) calloc(MI_NUM_SCREENS(mi),
						 sizeof (qixstruct))) == NULL)
			return;
	}
	qp = &qixs[MI_SCREEN(mi)];

	qp->nlines = (MI_CYCLES(mi) + 1) * 2;
	if (!qp->lineq)
		qp->lineq = (XPoint *) calloc(qp->nlines, sizeof (XPoint));
	qp->width = MI_WIN_WIDTH(mi);
	qp->height = MI_WIN_HEIGHT(mi);
	qp->delta = 16;

	if (qp->width < 100) {	/* icon window */
		qp->nlines /= 4;
		qp->delta /= 4;
	}
	qp->offset = qp->delta / 3;
	qp->last = 0;
	qp->pix = 0;
	qp->dx1 = NRAND(qp->delta) + qp->offset;
	qp->dy1 = NRAND(qp->delta) + qp->offset;
	qp->dx2 = NRAND(qp->delta) + qp->offset;
	qp->dy2 = NRAND(qp->delta) + qp->offset;
	qp->x1 = NRAND(qp->width);
	qp->y1 = NRAND(qp->height);
	qp->x2 = NRAND(qp->width);
	qp->y2 = NRAND(qp->height);
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_qix(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	qixstruct  *qp = &qixs[MI_SCREEN(mi)];

	qp->first = (qp->last + 2) % qp->nlines;

	qp->x1 += qp->dx1;
	qp->y1 += qp->dy1;
	qp->x2 += qp->dx2;
	qp->y2 += qp->dy2;
	check_bounds(qp, qp->x1, &qp->dx1, qp->width);
	check_bounds(qp, qp->y1, &qp->dy1, qp->height);
	check_bounds(qp, qp->x2, &qp->dx2, qp->width);
	check_bounds(qp, qp->y2, &qp->dy2, qp->height);
	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	XDrawLine(display, MI_WINDOW(mi), gc,
		  qp->lineq[qp->first].x, qp->lineq[qp->first].y,
		  qp->lineq[qp->first + 1].x, qp->lineq[qp->first + 1].y);
	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(display, gc, MI_PIXEL(mi, qp->pix));
		if (++qp->pix >= MI_NPIXELS(mi))
			qp->pix = 0;
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));

	XDrawLine(display, MI_WINDOW(mi), gc, qp->x1, qp->y1, qp->x2, qp->y2);

	qp->lineq[qp->last].x = qp->x1;
	qp->lineq[qp->last].y = qp->y1;
	qp->last++;
	if (qp->last >= qp->nlines)
		qp->last = 0;

	qp->lineq[qp->last].x = qp->x2;
	qp->lineq[qp->last].y = qp->y2;
	qp->last++;
	if (qp->last >= qp->nlines)
		qp->last = 0;
}

void
release_qix(ModeInfo * mi)
{
	if (qixs != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			qixstruct  *qp = &qixs[screen];

			if (qp->lineq)
				(void) free((void *) qp->lineq);
		}
		(void) free((void *) qixs);
		qixs = NULL;
	}
}
