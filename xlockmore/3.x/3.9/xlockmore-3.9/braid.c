#ifndef lint
static char sccsid[] = "@(#)braid.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * braid - an xlock module which draws random braids around a circle
 *         and then changes the color in a rotational pattern
 *
 * Copyright (c) 1995 by John Neil.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 01-Sep-95: color knotted components differently, J. Neil.
 * 29-Aug-95: Written.  John Neil <neil@math.idbsu.edu>
 */

#include <math.h>
#include "xlock.h"

#if defined( COLORROUND ) && defined( COLORCOMP )
#undef COLORROUND
#undef COLORCOMP
#endif

#if !defined( COLORROUND ) && !defined( COLORCOMP )
#if 0
/* to color in a circular pattern use COLORROUND */
#define COLORROUND
#else
/* to color by component use COLORCOMP */
#define COLORCOMP
#endif
#endif

#define MAXLENGTH  50		/* the maximum length of a braid word */
#define MINLENGTH  8		/* the minimum length of a braid word */
#define MAXSTRANDS  15		/* the maximum number of strands in the braid */
#define MINSTRANDS  3		/* the minimum number of strands in the braid */
#define SPINRATE  12.0		/* the rate at which the colors spin */

#define INTRAND(min,max) (LRAND()%((max+1)-(min))+(min))
#define FLOATRAND(min,max) ((min)+((double) LRAND()/((double) MAXRAND))*((max)-(min)))
#define ABS(x)  ((x<0)?(-x):(x))
#define MIN(a,b)  ((a<b)?(a):(b))
#define MAX(a,b)  ((a>b)?(a):(b))
#define TWOPI  (2*M_PI)

ModeSpecOpt braid_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         braidword[MAXLENGTH];
	int         components[MAXSTRANDS];
	int         startcomp[MAXLENGTH][MAXSTRANDS];
	int         nstrands;
	int         braidlength;
	float       startcolor;
	int         width;
	int         height;
	float       center_x;
	float       center_y;
	float       min_radius;
	float       max_radius;
	float       top, bottom, left, right;
	int         age;
} braidtype;

static braidtype braids[MAXSCREENS];

static int
applyword(int string, int position)
{
	braidtype  *braid = &braids[screen];
	int         i, c;

	c = string;
	for (i = position; i < braid->braidlength; i++) {
		if (c == ABS(braid->braidword[i]))
			c--;
		else if (c == ABS(braid->braidword[i]) - 1)
			c++;
	}
	for (i = 0; i < position; i++) {
		if (c == ABS(braid->braidword[i]))
			c--;
		else if (c == ABS(braid->braidword[i]) - 1)
			c++;
	}
	return c;
}

#if 0
static int
applywordto(string, position)
	int         string;
	int         position;
{
	braidtype  *braid = &braids[screen];
	int         i, c;

	c = string;
	for (i = 0; i < position; i++) {
		if (c == ABS(braid->braidword[i])) {
			c--;
		} else if (c == ABS(braid->braidword[i]) - 1) {
			c++;
		}
	}
	return c;
}
#endif

static int
applywordbackto(int string, int position)
{
	braidtype  *braid = &braids[screen];
	int         i, c;

	c = string;
	for (i = position - 1; i >= 0; i--) {
		if (c == ABS(braid->braidword[i])) {
			c--;
		} else if (c == ABS(braid->braidword[i]) - 1) {
			c++;
		}
	}
	return c;
}

void
init_braid(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i, count, comp, c;
	float       min_length;
	braidtype  *braid = &braids[screen];
	int         used[MAXSTRANDS];

	braid->width = MI_WIN_WIDTH(mi);
	braid->height = MI_WIN_HEIGHT(mi);
	braid->age = 0;

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, braid->width, braid->height);

	braid->center_x = (float) (braid->width) / 2.0;
	braid->center_y = (float) (braid->height) / 2.0;
	min_length = (braid->center_x > braid->center_y) ?
		braid->center_y : braid->center_x;
	braid->min_radius = min_length * 0.30;
	braid->max_radius = min_length * 0.90;

	if (MI_BATCHCOUNT(mi) < MINSTRANDS)
		braid->nstrands = MINSTRANDS;
	else
		braid->nstrands = INTRAND(MINSTRANDS,
				      MIN(MIN(MAXSTRANDS, MI_BATCHCOUNT(mi)),
		     (int) ((braid->max_radius - braid->min_radius) / 5.0)));
	braid->braidlength = INTRAND(MINLENGTH, MIN(MAXLENGTH, braid->nstrands * 6));

	for (i = 0; i < braid->braidlength; i++) {
		braid->braidword[i] = INTRAND(1, braid->nstrands - 1) * (INTRAND(1, 2) * 2 - 3);
		if (i > 0)
			while (braid->braidword[i] == -braid->braidword[i - 1])
				braid->braidword[i] = INTRAND(1, braid->nstrands - 1) * (INTRAND(1, 2) * 2 - 3);
	}

	while (braid->braidword[0] == -braid->braidword[braid->braidlength - 1])
		braid->braidword[braid->braidlength - 1] =
			INTRAND(1, braid->nstrands - 1) * (INTRAND(1, 2) * 2 - 3);

	do {
		(void) memset((char *) used, 0, sizeof (used));
		count = 0;
		for (i = 0; i < braid->braidlength; i++)
			used[ABS(braid->braidword[i])]++;
		for (i = 0; i < braid->nstrands; i++)
			count += (used[i] > 0) ? 1 : 0;
		if (count < braid->nstrands - 1) {
			braid->braidword[braid->braidlength] =
				INTRAND(1, braid->nstrands - 1) * (INTRAND(1, 2) * 2 - 3);
			while (braid->braidword[braid->braidlength] ==
			       -braid->braidword[braid->braidlength - 1] &&
			       braid->braidword[0] == -braid->braidword[braid->braidlength])
				braid->braidword[braid->braidlength] =
					INTRAND(1, braid->nstrands - 1) * (INTRAND(1, 2) * 2 - 3);
			braid->braidlength++;
		}
	} while (count < braid->nstrands - 1 && braid->braidlength < MAXLENGTH);

	braid->startcolor = 0.0;
	/* XSetLineAttributes (dsp, Scr[screen].gc, 2, LineSolid, CapRound, 
	   JoinRound); */

	(void) memset((char *) braid->components, 0, sizeof (braid->components));
	c = 1;
	comp = 0;
	braid->components[0] = 1;
	do {
		i = comp;
		do {
			i = applyword(i, 0);
			braid->components[i] = braid->components[comp];
		} while (i != comp);
		count = 0;
		for (i = 0; i < braid->nstrands; i++)
			if (braid->components[i] == 0)
				count++;
		if (count > 0) {
			for (comp = 0; braid->components[comp] != 0; comp++);
			braid->components[comp] = ++c;
		}
	} while (count > 0);

	for (i = 0; i < braid->nstrands; i++)
		if (!(braid->components[i] & 1))
			braid->components[i] *= -1;
}

void
draw_braid(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);

	braidtype  *braid = &braids[screen];
	float       num_points, t_inc;
	float       theta, psi;
	float       t, r_diff;
	int         i, s;
	float       x_1, y_1, x_2, y_2, r1, r2;
	float       color, color_use = 0.0, color_inc;

	num_points = 500.0;
	theta = TWOPI / (float) (braid->braidlength);
	t_inc = TWOPI / num_points;
	color_inc = (float) (Scr[screen].npixels) / num_points;
	braid->startcolor += SPINRATE * color_inc;
	if (braid->startcolor >= Scr[screen].npixels)
		braid->startcolor = 0.0;

	r_diff = (braid->max_radius - braid->min_radius) / (float) (braid->nstrands);

	color = braid->startcolor;
	psi = 0.0;
	for (i = 0; i < braid->braidlength; i++) {
		psi += theta;
		for (t = 0.0; t < theta; t += t_inc) {
#ifdef COLORROUND
			color += color_inc;
			if (color >= (float) (Scr[screen].npixels))
				color = 0.0;
			color_use = color;
#endif
			for (s = 0; s < braid->nstrands; s++) {
				if (ABS(braid->braidword[i]) == s)
					continue;
				if (ABS(braid->braidword[i]) - 1 == s) {
					/* crosSINFg */
#ifdef COLORCOMP
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
						color_use = color + SPINRATE *
							braid->components[applywordbackto(s, i)] +
							(psi + t) / 2.0 / M_PI * (float) (Scr[screen].npixels);
						while (color_use >= (float) (Scr[screen].npixels))
							color_use -= (float) (Scr[screen].npixels);
						while (color_use < 0.0)
							color_use += (float) (Scr[screen].npixels);
					}
#endif
#ifdef COLORROUND
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
						color_use += SPINRATE * color_inc;
						while (color_use >= (float) (Scr[screen].npixels))
							color_use -= (float) (Scr[screen].npixels);
					}
#endif
					r1 = braid->min_radius + r_diff * (float) (s);
					r2 = braid->min_radius + r_diff * (float) (s + 1);
					if (braid->braidword[i] > 0 ||
					    (FABSF(t - theta / 2.0) > theta / 7.0)) {
						x_1 = ((0.5 * (1.0 + SINF(t / theta * M_PI - M_PI_2)) * r2 +
							0.5 * (1.0 + SINF((theta - t) / theta * M_PI - M_PI_2)) * r1)) *
							COSF(t + psi) + braid->center_x;
						y_1 = ((0.5 * (1.0 + SINF(t / theta * M_PI - M_PI_2)) * r2 +
							0.5 * (1.0 + SINF((theta - t) / theta * M_PI - M_PI_2)) * r1)) *
							SINF(t + psi) + braid->center_y;
						x_2 = ((0.5 * (1.0 + SINF((t + t_inc) / theta * M_PI - M_PI_2)) * r2 +
							0.5 * (1.0 + SINF((theta - t - t_inc) / theta * M_PI - M_PI_2)) * r1)) *
							COSF(t + t_inc + psi) + braid->center_x;
						y_2 = ((0.5 * (1.0 + SINF((t + t_inc) / theta * M_PI - M_PI_2)) * r2 +
							0.5 * (1.0 + SINF((theta - t - t_inc) / theta * M_PI - M_PI_2)) * r1)) *
							SINF(t + t_inc + psi) + braid->center_y;
						if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
							XSetForeground(dsp, Scr[screen].gc,
								       Scr[screen].pixels[(int) color_use]);
						else
							XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

						XDrawLine(dsp, win, Scr[screen].gc,
							  (int) (x_1), (int) (y_1), (int) (x_2), (int) (y_2));
					}
#ifdef COLORCOMP
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
						color_use = color + SPINRATE *
							braid->components[applywordbackto(s + 1, i)] +
							(psi + t) / 2.0 / M_PI * (float) (Scr[screen].npixels);
						while (color_use >= (float) (Scr[screen].npixels))
							color_use -= (float) (Scr[screen].npixels);
						while (color_use < 0.0)
							color_use += (float) (Scr[screen].npixels);
					}
#endif
					if (braid->braidword[i] < 0 ||
					    (FABSF(t - theta / 2.0) > theta / 7.0)) {
						x_1 = ((0.5 * (1.0 + SINF(t / theta * M_PI - M_PI_2)) * r1 +
							0.5 * (1.0 + SINF((theta - t) / theta * M_PI - M_PI_2)) * r2)) *
							COSF(t + psi) + braid->center_x;
						y_1 = ((0.5 * (1.0 + SINF(t / theta * M_PI - M_PI_2)) * r1 +
							0.5 * (1.0 + SINF((theta - t) / theta * M_PI - M_PI_2)) * r2)) *
							SINF(t + psi) + braid->center_y;
						x_2 = ((0.5 * (1.0 + SINF((t + t_inc) / theta * M_PI - M_PI_2)) * r1 +
							0.5 * (1.0 + SINF((theta - t - t_inc) / theta * M_PI - M_PI_2)) * r2)) *
							COSF(t + t_inc + psi) + braid->center_x;
						y_2 = ((0.5 * (1.0 + SINF((t + t_inc) / theta * M_PI - M_PI_2)) * r1 +
							0.5 * (1.0 + SINF((theta - t - t_inc) / theta * M_PI - M_PI_2)) * r2)) *
							SINF(t + t_inc + psi) + braid->center_y;
						if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
							XSetForeground(dsp, Scr[screen].gc,
								       Scr[screen].pixels[(int) color_use]);
						else
							XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

						XDrawLine(dsp, win, Scr[screen].gc,
							  (int) (x_1), (int) (y_1), (int) (x_2), (int) (y_2));
					}
				} else {
					/* no crosSINFg */
#ifdef COLORCOMP
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
						color_use = color + SPINRATE *
							braid->components[applywordbackto(s, i)] +
							(psi + t) / 2.0 / M_PI * (float) (Scr[screen].npixels);
						while (color_use >= (float) (Scr[screen].npixels))
							color_use -= (float) (Scr[screen].npixels);
						while (color_use < 0.0)
							color_use += (float) (Scr[screen].npixels);
					}
#endif
#ifdef COLORROUND
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
						color_use += SPINRATE * color_inc;
						while (color_use >= (float) (Scr[screen].npixels))
							color_use -= (float) (Scr[screen].npixels);
					}
#endif
					r1 = braid->min_radius + r_diff * (float) (s);
					x_1 = r1 * COSF(t + psi) + braid->center_x;
					y_1 = r1 * SINF(t + psi) + braid->center_y;
					x_2 = r1 * COSF(t + t_inc + psi) + braid->center_x;
					y_2 = r1 * SINF(t + t_inc + psi) + braid->center_y;
					if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
						XSetForeground(dsp, Scr[screen].gc,
							       Scr[screen].pixels[(int) color_use]);
					else
						XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

					XDrawLine(dsp, win, Scr[screen].gc,
						  (int) (x_1), (int) (y_1), (int) (x_2), (int) (y_2));
				}
			}
		}
	}

	if (++braid->age > MI_CYCLES(mi))
		init_braid(mi);
}
