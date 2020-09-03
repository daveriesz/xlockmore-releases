
#ifndef lint
static char sccsid[] = "@(#)flame.c	3.9 96/05/25 xlockmore";

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

#define MAXBATCH  10
#define MAXLEV    4
#define MAXKINDS  9

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
	Window      win;
	XPoint      pts[MAXBATCH];
} flamestruct;

static flamestruct flames[MAXSCREENS];

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

void
init_flame(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	flamestruct *fs = &flames[screen];

	fs->width = MI_WIN_WIDTH(mi);
	fs->height = MI_WIN_HEIGHT(mi);

	fs->max_levels = MI_BATCHCOUNT(mi);
	fs->win = win;
	fs->cycles = MI_CYCLES(mi);

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, fs->width, fs->height);

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		fs->pixcol = halfrandom(Scr[screen].npixels);
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[fs->pixcol]);
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

	fs->variation = LRAND() % MAXKINDS;
}

static      Bool
recurse(flamestruct * fs, register double x, register double y, register int l)
{
	int         i;
	double      nx, ny;

	if (l == fs->max_levels) {
		fs->total_points++;
		if (fs->total_points > fs->cycles)	/* how long each fractal runs */
			return False;

		if (x > -1.0 && x < 1.0 && y > -1.0 && y < 1.0) {
			fs->pts[fs->num_points].x = (int) ((fs->width / 2) * (x + 1.0));
			fs->pts[fs->num_points].y = (int) ((fs->height / 2) * (y + 1.0));
			fs->num_points++;
			if (fs->num_points > MAXBATCH) {	/* point buffer size */
				XDrawPoints(dsp, fs->win, Scr[screen].gc, fs->pts,
					    fs->num_points, CoordModeOrigin);
				fs->num_points = 0;
			}
		}
	} else {
		for (i = 0; i < fs->snum; i++) {
			nx = fs->f[0][0][i] * x + fs->f[0][1][i] * y + fs->f[0][2][i];
			ny = fs->f[1][0][i] * x + fs->f[1][1][i] * y + fs->f[1][2][i];
			if (i < fs->anum) {
				switch (fs->variation) {
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
			if (!recurse(fs, nx, ny, l + 1))
				return False;
		}
	}
	return True;
}


void
draw_flame(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	flamestruct *fs = &flames[screen];
	int         i, j, k;
	static      alt = 0;

	if (!(fs->cur_level++ % fs->max_levels)) {
		XClearWindow(dsp, fs->win);
		alt = !alt;
	} else {
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
			XSetForeground(dsp, Scr[screen].gc,
				       Scr[screen].pixels[fs->pixcol]);
			if (--fs->pixcol < 0)
				fs->pixcol = Scr[screen].npixels - 1;
		}
	}

	/* number of functions */
	fs->snum = 2 + (fs->cur_level % (MAXLEV - 1));

	/* how many of them are of alternate form */
	if (alt)
		fs->anum = 0;
	else
		fs->anum = halfrandom(fs->snum) + 2;

	/* 6 coefs per function */
	for (k = 0; k < fs->snum; k++) {
		for (i = 0; i < 2; i++)
			for (j = 0; j < 3; j++)
				fs->f[i][j][k] = ((double) (LRAND() & 1023) / 512.0 - 1.0);
	}
	fs->num_points = 0;
	fs->total_points = 0;
	(void) recurse(fs, 0.0, 0.0, 0);
	XDrawPoints(dsp, win, Scr[screen].gc,
		    fs->pts, fs->num_points, CoordModeOrigin);
}
