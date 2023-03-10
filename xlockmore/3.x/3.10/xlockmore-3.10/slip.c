
#ifndef lint
static char sccsid[] = "@(#)slip.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * slip.c - lots of blits
 *
 * Copyright (c) 1992 by Scott Draves (spot@cs.cmu.edu)
 *
 * See xlock.c for copying information.
 * 01-Dec-95: Patched for VMS <joukj@alpha.chem.uva.nl>.
 */

#include "xlock.h"
#include <math.h>

ModeSpecOpt slip_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         width, height;
	int         nblits_remaining;
	int         blit_width, blit_height;
	int         mode;
	int         first_time;
} slipstruct;
static slipstruct *slips = NULL;

static short
halfrandom(int mv)
{
	static short lasthalf = 0;
	unsigned long r;

	if (lasthalf) {
		r = lasthalf;
		lasthalf = 0;
	} else {
		r = LRAND();
		lasthalf = r >> 16;
	}
	return r % mv;
}

static int
erandom(int mv)
{
	static int  stage = 0;
	static unsigned long r;
	int         res;

	if (0 == stage) {
		r = LRAND();
		stage = 7;
	}
	res = r & 0xf;
	r = r >> 4;
	stage--;
	if (res & 8)
		return res & mv;
	else
		return -(res & mv);
}

static void
prepare_screen(ModeInfo * mi, slipstruct * s)
{

	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	int         i, n, w = s->width / 20;
	int         not_solid = halfrandom(10);

	if (s->first_time || !halfrandom(5)) {
		XClearWindow(display, MI_WINDOW(mi));
		n = 300;
	} else {
		if (halfrandom(5))
			return;
		if (halfrandom(5))
			n = 100;
		else
			n = 2000;
	}

	XSetForeground(display, gc, MI_PIXEL(mi, halfrandom(MI_NPIXELS(mi))));

	for (i = 0; i < n; i++) {
		if (not_solid)
			XSetForeground(display, gc,
				   MI_PIXEL(mi, halfrandom(MI_NPIXELS(mi))));
		XFillRectangle(display, MI_WINDOW(mi), gc,
			       halfrandom(s->width - w),
			       halfrandom(s->height - w),
			       w, w);
	}
	s->first_time = 0;
}

static int
quantize(double d)
{
	int         i = floor(d);
	double      f = d - i;

	if ((LRAND() & 0xff) < f * 0xff)
		i++;
	return i;
}

void
init_slip(ModeInfo * mi)
{
	slipstruct *sp;

	if (slips == NULL) {
		if ((slips = (slipstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (slipstruct))) == NULL)
			return;
	}
	sp = &slips[MI_SCREEN(mi)];

	sp->width = MI_WIN_WIDTH(mi);
	sp->height = MI_WIN_HEIGHT(mi);

	sp->blit_width = sp->width / 25;
	sp->blit_height = sp->height / 25;
	sp->nblits_remaining = 0;
	sp->mode = 0;
	sp->first_time = 1;

	/* no "NoExpose" events from XCopyArea wanted */
	XSetGraphicsExposures(MI_DISPLAY(mi), MI_GC(mi), False);
}

void
draw_slip(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	slipstruct *s = &slips[MI_SCREEN(mi)];
	int         timer;

	timer = MI_BATCHCOUNT(mi) * MI_CYCLES(mi);

	while (timer--) {
		int         xi = halfrandom(s->width - s->blit_width);
		int         yi = halfrandom(s->height - s->blit_height);
		double      x, y, dx = 0, dy = 0, t, s1, s2;

		if (0 == s->nblits_remaining--) {
			static      lut[] =
			{0, 0, 0, 1, 1, 1, 2};

			prepare_screen(mi, s);
			s->nblits_remaining = MI_BATCHCOUNT(mi) *
				(2000 + halfrandom(1000) + halfrandom(1000));
			if (s->mode == 2)
				s->mode = halfrandom(2);
			else
				s->mode = lut[halfrandom(7)];
		}
		x = (2 * xi + s->blit_width) / (double) s->width - 1;
		y = (2 * yi + s->blit_height) / (double) s->height - 1;

		/* (x,y) is in biunit square */
		switch (s->mode) {
			case 0:
				dx = x;
				dy = y;

				if (dy < 0) {
					dy += 0.04;
					if (dy > 0)
						dy = 0.00;
				}
				if (dy > 0) {
					dy -= 0.04;
					if (dy < 0)
						dy = 0.00;
				}
				t = dx * dx + dy * dy + 1e-10;
				s1 = 2 * dx * dx / t - 1;
				s2 = 2 * dx * dy / t;

				dx = s1 * 5;
				dy = s2 * 5;
				break;
			case 1:
				dx = erandom(3);
				dy = erandom(3);
				break;
			case 2:
				dx = x * 3;
				dy = y * 3;
				break;
		}
		{
			int         qx = xi + quantize(dx), qy = yi + quantize(dy);
			int         wrap;

			if (qx < 0 || qy < 0 ||
			    qx >= s->width - s->blit_width ||
			    qy >= s->height - s->blit_height)
				continue;

			XCopyArea(display, window, window, gc, xi, yi,
				  s->blit_width, s->blit_height,
				  qx, qy);

			switch (s->mode) {
				case 0:
					/* wrap */
					wrap = s->width - (2 * s->blit_width);
					if (qx > wrap)
						XCopyArea(display, window, window, gc, qx, qy,
						s->blit_width, s->blit_height,
							  qx - wrap, qy);

					if (qx < 2 * s->blit_width)
						XCopyArea(display, window, window, gc, qx, qy,
						s->blit_width, s->blit_height,
							  qx + wrap, qy);

					wrap = s->height - (2 * s->blit_height);
					if (qy > wrap)
						XCopyArea(display, window, window, gc, qx, qy,
						s->blit_width, s->blit_height,
							  qx, qy - wrap);

					if (qy < 2 * s->blit_height)
						XCopyArea(display, window, window, gc, qx, qy,
						s->blit_width, s->blit_height,
							  qx, qy + wrap);
					break;
				case 1:
				case 2:
					break;
			}
		}
	}
}

void
release_slip(ModeInfo * mi)
{
	if (slips != NULL) {
		(void) free((void *) slips);
		slips = NULL;
	}
}
