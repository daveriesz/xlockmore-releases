
#ifndef lint
static char sccsid[] = "@(#)flame.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * flame.c - recursive fractal cosmic flames.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 11-Aug-95: Got rid of polyominal since it was crashing xlock on some
 *            machines.
 * 01-Jun-95: This should look more like the original with some updates by
 *            Scott Draves.
 * 27-Jun-91: vary number of functions used.
 * 24-Jun-91: fixed portability problem with integer mod (%).
 * 06-Jun-91: Written. (received from Scott Draves, spot@cs.cmu.edu).
 */

#include "xlock.h"
#include <math.h>

#define MAXLEV    4
#define MAXKINDS  9
#define MAXBATCH  12

ModeSpecOpt flame_opts =
{0, NULL, NULL, NULL};

typedef struct {
	double      f[2][3][MAXLEV];	/* three non-homogeneous transforms */
	int         variation;
	int         max_levels;
	int         cur_level;
	int         snum;
	int         anum;
	int         width, height;
	int         num_points;
	int         total_points;
	int         pixcol;
	int         cycles;
	XPoint      pts[MAXBATCH];
} flamestruct;

static flamestruct *flames = NULL;

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

static      Bool
recurse(ModeInfo * mi, flamestruct * fp,
	register double x, register double y, register int l)
{
	int         i;
	double      nx, ny;

	if (l == fp->max_levels) {
		fp->total_points++;
		if (fp->total_points > fp->cycles)	/* how long each fractal runs */
			return False;

		if (x > -1.0 && x < 1.0 && y > -1.0 && y < 1.0) {
			fp->pts[fp->num_points].x = (int) ((fp->width / 2) * (x + 1.0));
			fp->pts[fp->num_points].y = (int) ((fp->height / 2) * (y + 1.0));
			fp->num_points++;
			if (fp->num_points >= MAXBATCH) {	/* point buffer size */
				XDrawPoints(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi), fp->pts,
					    fp->num_points, CoordModeOrigin);
				fp->num_points = 0;
			}
		}
	} else {
		for (i = 0; i < fp->snum; i++) {
			nx = fp->f[0][0][i] * x + fp->f[0][1][i] * y + fp->f[0][2][i];
			ny = fp->f[1][0][i] * x + fp->f[1][1][i] * y + fp->f[1][2][i];
			if (i < fp->anum) {
				switch (fp->variation) {
					case 0:	/* sinusoidal */
						nx = sin(nx);
						ny = sin(ny);
						break;
					case 1:	/* complex */
						{
							double      r2 = nx * nx + ny * ny + 1e-6;

							nx = nx / r2;
							ny = ny / r2;
						}
						break;
					case 2:	/* bent */
						if (nx < 0.0)
							nx = nx * 2.0;
						if (ny < 0.0)
							ny = ny / 2.0;
						break;
					case 3:	/* swirl */
						{
							double      r = (nx * nx + ny * ny);	/* times k here is fun */
							double      c1 = sin(r);
							double      c2 = cos(r);
							double      t = nx;

							nx = c1 * nx - c2 * ny;
							ny = c2 * t + c1 * ny;
						}
						break;
					case 4:	/* horseshoe */
						{
							double      r = atan2(nx, ny);	/* times k here is fun */
							double      c1 = sin(r);
							double      c2 = cos(r);
							double      t = nx;

							nx = c1 * nx - c2 * ny;
							ny = c2 * t + c1 * ny;
						}
						break;
					case 5:	/* drape */
						{
							double      t = atan2(nx, ny) / M_PI;

							ny = sqrt(nx * nx + ny * ny) - 1.0;
							nx = t;
						}
						break;
					case 6:	/* broken */
						if (nx > 1.0)
							nx = nx - 1.0;
						if (nx < -1.0)
							nx = nx + 1.0;
						if (ny > 1.0)
							ny = ny - 1.0;
						if (ny < -1.0)
							ny = ny + 1.0;
						break;
					case 7:	/* spherical */
						{
							double      r = 0.5 + sqrt(nx * nx + ny * ny + 1e-6);

							nx = nx / r;
							ny = ny / r;
						}
						break;
					case 8:	/*  */
						nx = atan(nx) / M_PI_2;
						ny = atan(ny) / M_PI_2;
						break;
#if 0
/* core dumps on some machines, why not all? */
					case 9:	/* complex sine */
						{
							double      u = nx,
							            v = ny;
							double      ev = exp(v);
							double      emv = exp(-v);

							nx = (ev + emv) * sin(u) / 2.0;
							ny = (ev - emv) * cos(u) / 2.0;
						}
						break;
					case 10:	/* polynomial */
						if (nx < 0)
							nx = -nx * nx;
						else
							nx = nx * nx;
						if (ny < 0)
							ny = -ny * ny;
						else
							ny = ny * ny;
						break;
#endif
					default:
						nx = sin(nx);
						ny = sin(ny);
				}
			}
			if (!recurse(mi, fp, nx, ny, l + 1))
				return False;
		}
	}
	return True;
}

void
init_flame(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	flamestruct *fp;

	if (flames == NULL) {
		if ((flames = (flamestruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (flamestruct))) == NULL)
			return;
	}
	fp = &flames[MI_SCREEN(mi)];

	fp->width = MI_WIN_WIDTH(mi);
	fp->height = MI_WIN_HEIGHT(mi);

	fp->max_levels = MI_BATCHCOUNT(mi);
	fp->cycles = MI_CYCLES(mi);

	XClearWindow(display, MI_WINDOW(mi));

	if (MI_NPIXELS(mi) > 2) {
		fp->pixcol = halfrandom(MI_NPIXELS(mi));
		XSetForeground(display, gc, MI_PIXEL(mi, fp->pixcol));
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));

	fp->variation = NRAND(MAXKINDS);
}

void
draw_flame(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	flamestruct *fp = &flames[MI_SCREEN(mi)];
	int         i, j, k;
	static      alt = 0;

	if (!(fp->cur_level++ % fp->max_levels)) {
		XClearWindow(display, MI_WINDOW(mi));
		alt = !alt;
	} else {
		if (MI_NPIXELS(mi) > 2) {
			XSetForeground(display, MI_GC(mi), MI_PIXEL(mi, fp->pixcol));
			if (--fp->pixcol < 0)
				fp->pixcol = MI_NPIXELS(mi) - 1;
		}
	}

	/* number of functions */
	fp->snum = 2 + (fp->cur_level % (MAXLEV - 1));

	/* how many of them are of alternate form */
	if (alt)
		fp->anum = 0;
	else
		fp->anum = halfrandom(fp->snum) + 2;

	/* 6 coefs per function */
	for (k = 0; k < fp->snum; k++) {
		for (i = 0; i < 2; i++)
			for (j = 0; j < 3; j++)
				fp->f[i][j][k] = ((double) (LRAND() & 1023) / 512.0 - 1.0);
	}
	fp->num_points = 0;
	fp->total_points = 0;
	(void) recurse(mi, fp, 0.0, 0.0, 0);
	XDrawPoints(display, MI_WINDOW(mi), MI_GC(mi),
		    fp->pts, fp->num_points, CoordModeOrigin);
}

void
release_flame(ModeInfo * mi)
{
	if (flames != NULL) {
		(void) free((void *) flames);
		flames = NULL;
	}
}
