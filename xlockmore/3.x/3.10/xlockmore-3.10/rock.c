#ifndef lint
static char sccsid[] = "@(#)rock.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * Flying through an asteroid field.  Based on TI Explorer Lisp code by
 *  John Nguyen <johnn@hx.lcs.mit.edu>
 *
 * Copyright (c) 1992 by Jamie Zawinski
 * 
 * 8-May-96: Blue on left instead of green for 3d.  It seems more common
 *           than green.  Use "-left3d Green" if you have the other kind.
 * 17-Jan-96: 3D mode for rock thanks to <theiling@coli.uni-sb.de>.
 *            Get out your 3D glasses, Red on right and Blue on left.
 * 14-Apr-95: Jeremie PETIT <petit@aurora.unice.fr> added a "move" feature.
 * 2-Sep-93: xlock version (David Bagley bagleyd@hertz.njit.edu)
 * 1992:     xscreensaver version (Jamie Zawinski jwz@netscape.com)
 */

/* 
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

#include <math.h>
#include "xlock.h"

/* 
 * For 3d effect get some 3D glasses, right lens red, and left lens blue.
 * Too bad monitors do not emit polarized light.
 */

#define MIN_ROCKS 1
#define MIN_DEPTH 2		/* rocks disappear when they get this close */
#define MAX_DEPTH 60		/* this is where rocks appear */
#define MIN_WIDTH 3		/* how small where pixmaps are not used */
#define MAX_WIDTH 200		/* how big (in pixels) rocks are at depth 1 */
#define DEPTH_SCALE 100		/* how many ticks there are between depths */
#define RESOLUTION 1000
#define MAX_DEP 0.3		/* how far the displacement can be (percents) */
#define DIRECTION_CHANGE_RATE 50	/* 0 is always */
#define MAX_DEP_SPEED 5		/* Maximum speed for movement */
#define MOVE_STYLE 0		/* Only 0 and 1. Distinguishes the fact that
				   these are the rocks that are moving (1) 
				   or the rocks source (0). */

/* there's not much point in the above being user-customizable, but those
   numbers might want to be tweaked for displays with an order of magnitude
   higher resolution or compute power. */


ModeSpecOpt rock_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         real_size;
	int         r;
	unsigned long color;
	int         theta;
	int         depth;
	int         size, x, y;
	int         diff;
} arock;

typedef struct {
	int         width, height;
	int         midx, midy;
	int         rotate_p, speed, nrocks;
	int         move_p;
	int         dep_x, dep_y;
	int         max_width;
	arock      *arocks;
	Pixmap     *pixmaps;
} rockstruct;

static double cos_array[RESOLUTION], sin_array[RESOLUTION];
static double depths[(MAX_DEPTH + 1) * DEPTH_SCALE];

static rockstruct *rocks = NULL;

static void rock_reset(ModeInfo * mi, arock * arocks);
static void rock_tick(ModeInfo * mi, arock * arocks, int d);
static void rock_compute(ModeInfo * mi, arock * arocks);
static void rock_draw(ModeInfo * mi, arock * arocks, int draw_p);
static void init_pixmaps(ModeInfo * mi);
static void tick_rocks(ModeInfo * mi, int d);
static int  compute_move(rockstruct * rp, int axe);

static void
rock_reset(ModeInfo * mi, arock * arocks)
{
	rockstruct *rp = &rocks[MI_SCREEN(mi)];

	arocks->real_size = rp->max_width;
	arocks->r = (RESOLUTION * 0.7) + (NRAND(30 * RESOLUTION));
	arocks->theta = NRAND(RESOLUTION);
	arocks->depth = MAX_DEPTH * DEPTH_SCALE;
	if (MI_NPIXELS(mi) > 2)
		arocks->color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
	else
		arocks->color = MI_WIN_WHITE_PIXEL(mi);
	rock_compute(mi, arocks);
	rock_draw(mi, arocks, True);
}

static void
rock_tick(ModeInfo * mi, arock * arocks, int d)
{
	rockstruct *rp = &rocks[MI_SCREEN(mi)];

	if (arocks->depth > 0) {
		rock_draw(mi, arocks, False);
		arocks->depth -= rp->speed;
		if (rp->rotate_p)
			arocks->theta = (arocks->theta + d) % RESOLUTION;
		while (arocks->theta < 0)
			arocks->theta += RESOLUTION;
		if (arocks->depth < (MIN_DEPTH * DEPTH_SCALE))
			arocks->depth = 0;
		else {
			rock_compute(mi, arocks);
			rock_draw(mi, arocks, True);
		}
	} else if (!NRAND(40))
		rock_reset(mi, arocks);
}

static void
rock_compute(ModeInfo * mi, arock * arocks)
{
	rockstruct *rp = &rocks[MI_SCREEN(mi)];
	double      factor = depths[arocks->depth];
	double      rsize = arocks->real_size * factor;

	arocks->size = (int) (rsize + 0.5);
	arocks->diff = (int) -(rsize * MI_DELTA3D(mi) + 0.5);
	arocks->x = rp->midx + (cos_array[arocks->theta] * arocks->r * factor);
	arocks->y = rp->midy + (sin_array[arocks->theta] * arocks->r * factor);
	if (rp->move_p) {
		double      move_factor = (double) (MOVE_STYLE - (double) arocks->depth /
			  (double) ((MAX_DEPTH + 1) * (double) DEPTH_SCALE));

		/* move_factor is 0 when the rock is close, 1 when far */
		arocks->x += (double) rp->dep_x * move_factor;
		arocks->y += (double) rp->dep_y * move_factor;
	}
}

static void
rock_draw(ModeInfo * mi, arock * arocks, int draw_p)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	rockstruct *rp = &rocks[MI_SCREEN(mi)];

	if (draw_p) {
		if (MI_WIN_IS_USE3D(mi) && MI_NPIXELS(mi) > 2)
			XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
		else
			XSetForeground(display, gc, arocks->color);
	} else
		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	if (arocks->x <= 0 || arocks->y <= 0 ||
	    arocks->x >= rp->width || arocks->y >= rp->height) {
		/* this means that if a rock were to go off the screen at 12:00, but
		   would have been visible at 3:00, it won't come back once the observer
		   rotates around so that the rock would have been visible again.
		   Oh well.
		 */
		if (!rp->move_p)
			arocks->depth = 0;
		return;
	}
	if (arocks->size <= 1) {
		if (MI_WIN_IS_USE3D(mi) && MI_NPIXELS(mi) > 2) {
			if (draw_p)
				XSetForeground(display, gc, MI_LEFT_COLOR(mi));
			XDrawPoint(display, window, gc, arocks->x - arocks->diff, arocks->y);
			if (draw_p)
				XSetForeground(display, gc, MI_RIGHT_COLOR(mi));
			XDrawPoint(display, window, gc, arocks->x + arocks->diff, arocks->y);
		} else
			XDrawPoint(display, window, gc, arocks->x, arocks->y);
	} else if (arocks->size <= MIN_WIDTH || !draw_p) {
		if (MI_WIN_IS_USE3D(mi) && MI_NPIXELS(mi) > 2) {
			if (draw_p)
				XSetForeground(display, gc, MI_LEFT_COLOR(mi));
			XFillRectangle(display, window, gc,
				       arocks->x - arocks->size / 2 - arocks->diff, arocks->y - arocks->size / 2,
				       arocks->size, arocks->size);
			if (draw_p)
				XSetForeground(display, gc, MI_RIGHT_COLOR(mi));
			XFillRectangle(display, window, gc,
				       arocks->x - arocks->size / 2 + arocks->diff, arocks->y - arocks->size / 2,
				       arocks->size, arocks->size);
		} else
			XFillRectangle(display, window, gc,
				       arocks->x - arocks->size / 2, arocks->y - arocks->size / 2,
				       arocks->size, arocks->size);
	} else if (arocks->size < rp->max_width) {
		if (MI_WIN_IS_USE3D(mi) && MI_NPIXELS(mi) > 2) {
			XSetForeground(display, gc, MI_LEFT_COLOR(mi));
			XCopyPlane(display, rp->pixmaps[arocks->size - MIN_WIDTH], window, gc,
				   0, 0, arocks->size, arocks->size,
				   arocks->x - arocks->size / 2 - arocks->diff, arocks->y - arocks->size / 2,
				   1L);
			XSetForeground(display, gc, MI_RIGHT_COLOR(mi));
			XCopyPlane(display, rp->pixmaps[arocks->size - MIN_WIDTH], window, gc,
				   0, 0, arocks->size, arocks->size,
				   arocks->x - arocks->size / 2 + arocks->diff, arocks->y - arocks->size / 2,
				   1L);
		} else
			XCopyPlane(display, rp->pixmaps[arocks->size - MIN_WIDTH], window, gc,
				   0, 0, arocks->size, arocks->size,
				   arocks->x - arocks->size / 2, arocks->y - arocks->size / 2,
				   1L);
	}
}

static void
init_pixmaps(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	rockstruct *rp = &rocks[MI_SCREEN(mi)];
	int         i;
	XGCValues   gcv;
	GC          fg_gc = 0, bg_gc = 0;

	rp->max_width = MI_CYCLES(mi);
	if (rp->max_width > rp->width - MIN_WIDTH)
		rp->max_width = rp->width - MIN_WIDTH;
	if (rp->max_width < 0)
		rp->max_width = 0;
	else if (rp->max_width > MAX_WIDTH)
		rp->max_width = MAX_WIDTH;
	rp->pixmaps = (Pixmap *) calloc(rp->max_width, sizeof (Pixmap));
	for (i = 0; i < rp->max_width; i++) {
		int         w = (1 + ((i + MIN_WIDTH) / 32)) << 5;	/* server might be faster if word-aligned */
		int         h = i + MIN_WIDTH;
		Pixmap      p = XCreatePixmap(display, MI_WINDOW(mi), w, h, 1);
		XPoint      points[7];

		rp->pixmaps[i] = p;
		if (!p)
			error("%s: Could not allocate pixmaps, in rock, exitting\n");
		if (!fg_gc) {	/* must use drawable of pixmap, not window (fmh) */
			gcv.foreground = 1;
			fg_gc = XCreateGC(display, p, GCForeground, &gcv);
			gcv.foreground = 0;
			bg_gc = XCreateGC(display, p, GCForeground, &gcv);
		}
		XFillRectangle(display, p, bg_gc, 0, 0, w, h);
		points[0].x = h * 0.15;
		points[0].y = h * 0.85;
		points[1].x = h * 0.00;
		points[1].y = h * 0.20;
		points[2].x = h * 0.30;
		points[2].y = h * 0.00;
		points[3].x = h * 0.40;
		points[3].y = h * 0.10;
		points[4].x = h * 0.90;
		points[4].y = h * 0.10;
		points[5].x = h * 1.00;
		points[5].y = h * 0.55;
		points[6].x = h * 0.45;
		points[6].y = h * 1.00;
		XFillPolygon(display, p, fg_gc, points, 7, Nonconvex, CoordModeOrigin);
	}
	XFreeGC(display, fg_gc);
	XFreeGC(display, bg_gc);
}

static void
tick_rocks(ModeInfo * mi, int d)
{
	rockstruct *rp = &rocks[MI_SCREEN(mi)];
	int         i;

	if (rp->move_p) {
		rp->dep_x = compute_move(rp, 0);
		rp->dep_y = compute_move(rp, 1);
	}
	for (i = 0; i < rp->nrocks; i++)
		rock_tick(mi, &rp->arocks[i], d);
}

static int
compute_move(rockstruct * rp, int axe)
				/* 0 for x, 1 for y */
{
	static int  current_dep[2] =
	{0, 0};
	static int  speed[2] =
	{0, 0};
	static short direction[2] =
	{0, 0};
	static int  limit[2] =
	{0, 0};
	int         change = 0;

	limit[0] = rp->midx;
	limit[1] = rp->midy;

	current_dep[axe] += speed[axe];		/* We adjust the displacement */

	if (current_dep[axe] > (int) (limit[axe] * MAX_DEP)) {
		if (current_dep[axe] > limit[axe])
			current_dep[axe] = limit[axe];
		direction[axe] = -1;
	}			/* This is when we reach the upper screen limit */
	if (current_dep[axe] < (int) (-limit[axe] * MAX_DEP)) {
		if (current_dep[axe] < -limit[axe])
			current_dep[axe] = -limit[axe];
		direction[axe] = 1;
	}			/* This is when we reach the lower screen limit */
	if (direction[axe] == 1)	/* We adjust the speed */
		speed[axe] += 1;
	else if (direction[axe] == -1)
		speed[axe] -= 1;

	if (speed[axe] > MAX_DEP_SPEED)
		speed[axe] = MAX_DEP_SPEED;
	else if (speed[axe] < -MAX_DEP_SPEED)
		speed[axe] = -MAX_DEP_SPEED;

	if (!NRAND(DIRECTION_CHANGE_RATE)) {
		/* We change direction */
		change = LRAND() & 1;
		if (change != 1)
			if (direction[axe] == 0)
				direction[axe] = change - 1;	/* 0 becomes either 1 or -1 */
			else
				direction[axe] = 0;	/* -1 or 1 become 0 */
	}
	return (current_dep[axe]);
}

void
init_rock(ModeInfo * mi)
{
	rockstruct *rp;
	int         i;
	static int  first = 1;

	if (rocks == NULL) {
		if ((rocks = (rockstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (rockstruct))) == NULL)
			return;
	}
	rp = &rocks[MI_SCREEN(mi)];

	rp->width = MI_WIN_WIDTH(mi);
	rp->height = MI_WIN_HEIGHT(mi);
	rp->midx = rp->width / 2;
	rp->midy = rp->height / 2;
	rp->speed = 100;
	rp->rotate_p = True;
	rp->move_p = True;
	rp->dep_x = 0;
	rp->dep_y = 0;
	rp->nrocks = MI_BATCHCOUNT(mi);
	if (rp->nrocks < -MIN_ROCKS) {
		if (rp->arocks) {
			(void) free((void *) rp->arocks);
			rp->arocks = NULL;
		}
		rp->nrocks = NRAND(-rp->nrocks - MIN_ROCKS + 1) + MIN_ROCKS;
	} else if (rp->nrocks < MIN_ROCKS)
		rp->nrocks = MIN_ROCKS;
	if (rp->speed < 1)
		rp->speed = 1;
	if (rp->speed > 100)
		rp->speed = 100;

	if (first) {
		first = 0;
		for (i = 0; i < RESOLUTION; i++) {
			sin_array[i] = sin((((double) i) / (RESOLUTION / 2)) * M_PI);
			cos_array[i] = cos((((double) i) / (RESOLUTION / 2)) * M_PI);
		}
		/* we actually only need i/speed of these, but wtf */
		for (i = 1; i < (sizeof (depths) / sizeof (depths[0])); i++)
			depths[i] = atan(((double) 0.5) / (((double) i) / DEPTH_SCALE));
		depths[0] = M_PI_2;	/* avoid division by 0 */
	}
	if (!rp->arocks)
		rp->arocks = (arock *) calloc(rp->nrocks, sizeof (arock));
	if (!rp->pixmaps)
		init_pixmaps(mi);
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_rock(ModeInfo * mi)
{
	static int  current_delta = 0;	/* observer Z rotation */
	static int  window_tick = 50;
	static int  new_delta = 0;
	static int  dchange_tick = 0;

	if (window_tick++ == 50)
		window_tick = 0;
	if (current_delta != new_delta) {
		if (dchange_tick++ == 5) {
			dchange_tick = 0;
			if (current_delta < new_delta)
				current_delta++;
			else
				current_delta--;
		}
	} else {
		if (!NRAND(50)) {
			new_delta = (NRAND(11) - 5);
			if (!NRAND(10))
				new_delta *= 5;
		}
	}
	tick_rocks(mi, current_delta);
}

void
release_rock(ModeInfo * mi)
{
	if (rocks != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			rockstruct *rp = &rocks[screen];
			int         i;

			if (rp->arocks != NULL)
				(void) free((void *) rp->arocks);
			if (rp->pixmaps != NULL) {
				for (i = 0; i < rp->max_width; i++)
					XFreePixmap(MI_DISPLAY(mi), rp->pixmaps[i]);
				(void) free((void *) rp->pixmaps);
			}
		}
		(void) free((void *) rocks);
		rocks = NULL;
	}
}
