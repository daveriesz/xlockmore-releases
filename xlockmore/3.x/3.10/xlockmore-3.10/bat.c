
#ifndef lint
static char sccsid[] = "@(#)bat.c	3.10 96/07/290 xlockmore";

#endif

/*-
 * bat.c - A bouncing bat for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1988 by Sun Microsystems
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 18-Sep-95: 5 bats now in color (patol@info.isbiel.ch)
 * 20-Sep-94: 5 bats instead of bouncing balls, based on bounce.c
 *            (patol@info.isbiel.ch)
 * 2-Sep-93: bounce version (David Bagley bagleyd@hertz.njit.edu)
 * 1986: Sun Microsystems
 */

/* 
 * original copyright
 * **************************************************************************
 * Copyright 1988 by Sun Microsystems, Inc. Mountain View, CA.
 *
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Sun or MIT not be used in advertising
 * or publicity pertaining to distribution of the software without specific
 * prior written permission. Sun and M.I.T. make no representations about the
 * suitability of this software for any purpose. It is provided "as is"
 * without any express or implied warranty.
 *
 * SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL SUN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * ***************************************************************************
 */

#include "xlock.h"
#include <math.h>

#ifdef HAS_XPM
#if 1
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#include "pixmaps/bat-0.xpm"
#include "pixmaps/bat-1.xpm"
#include "pixmaps/bat-2.xpm"
#include "pixmaps/bat-3.xpm"
#include "pixmaps/bat-4.xpm"
#endif

#include "bitmaps/bat-0.xbm"
#include "bitmaps/bat-1.xbm"
#include "bitmaps/bat-2.xbm"
#include "bitmaps/bat-3.xbm"
#include "bitmaps/bat-4.xbm"

#define MAX_STRENGTH 24
#define FRICTION 15
#define PENETRATION 0.4
#define SLIPAGE 4
#define TIME 32
#define MINBATS 1

#define ARE_XBM -1

#define ORIENTS 8
#define ORIENTCYCLE 32
#define CCW 1
#define CW (ORIENTS-1)
#define DIR(x)	(((x)>=0)?CCW:CW)
#define SIGN(x)	(((x)>=0)?1:-1)
#define ABS(x)	(((x)>=0)?x:-(x))

ModeSpecOpt bat_opts =
{0, NULL, NULL, NULL};

static XImage bimages[] =
{
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1}
};

typedef struct {
	int         x, y, xlast, ylast;
	int         spincount, spindelay, spindir, orient;
	int         vx, vy, vang;
	int         mass, size, sizex, sizey;
	unsigned long color;
} batstruct;

typedef struct {
	int         width, height;
	int         nbats;
	int         restartnum;
	int         initialized;
	int         graphics_format;
	batstruct  *bats;
	XImage     *images[ORIENTS / 2 + 1];
} bouncestruct;

static bouncestruct *bounces = NULL;

static void checkCollision(bouncestruct * bp, int a_bat);
static void drawabat(ModeInfo * mi, batstruct * bat);
static void movebat(bouncestruct * bp, batstruct * bat);
static void flapbat(batstruct * bat, int dir, int *vel);
static int  collide(bouncestruct * bp, int a_bat);
extern void XEraseImage(Display * display, Window window, GC gc,
		   int x, int y, int xlast, int ylast, int xsize, int ysize);


static unsigned char *bits[] =
{
	bat0_bits, bat1_bits, bat2_bits, bat3_bits, bat4_bits
};

#ifdef HAS_XPM
static char **pixs[] =
{
	bat0, bat1, bat2, bat3, bat4
};

#endif

static void
init_images(ModeInfo * mi)
{
	bouncestruct *bp = &bounces[MI_SCREEN(mi)];
	int         i = 0;

#if HAS_XPM
	XpmAttributes attrib;
	Display    *display = MI_DISPLAY(mi);
	int         screen = MI_SCREEN(mi);

	attrib.visual = DefaultVisual(display, screen);
	attrib.colormap = DefaultColormap(display, screen);
	attrib.depth = DefaultDepth(display, screen);
	attrib.valuemask = XpmVisual | XpmColormap | XpmDepth;

	if (MI_NPIXELS(mi) > 2) {
		for (i = 0; i <= ORIENTS / 2; i++)
			if (XpmSuccess != XpmCreateImageFromData(MI_DISPLAY(mi), pixs[i],
				&(bp->images[i]), (XImage **) NULL, &attrib))
				break;
		bp->initialized = i;
		if (bp->initialized <= ORIENTS / 2) {	/* All or nothing */
			for (i = 0; i < bp->initialized; i++)
				XDestroyImage(bp->images[i]);
		}
	}
	if (bp->initialized <= ORIENTS / 2)
#endif

	{
		bp->initialized = ARE_XBM;	/* ie. not using xpm */
		if (!bimages[i].data)	/* Only need to do this once */
			for (i = 0; i <= ORIENTS / 2; i++) {
				bimages[i].data = (char *) bits[i];
				bimages[i].width = bat0_width;
				bimages[i].height = bat0_height;
				bimages[i].bytes_per_line = (bat0_width + 7) / 8;
			}
		for (i = 0; i <= ORIENTS / 2; i++)
			bp->images[i] = &(bimages[i]);
	}
}

static void
checkCollision(bouncestruct * bp, int a_bat)
{
	int         i, amount, spin, d, size;
	double      x, y;

	for (i = 0; i < bp->nbats; i++) {
		if (i != a_bat) {
			x = (double) (bp->bats[i].x - bp->bats[a_bat].x);
			y = (double) (bp->bats[i].y - bp->bats[a_bat].y);
			d = (int) sqrt(x * x + y * y);
			size = (bp->bats[i].size + bp->bats[a_bat].size) / 2;
			if (d > 0 && d < size) {
				amount = size - d;
				if (amount > PENETRATION * size)
					amount = PENETRATION * size;
				bp->bats[i].vx += amount * x / d;
				bp->bats[i].vy += amount * y / d;
				bp->bats[i].vx -= bp->bats[i].vx / FRICTION;
				bp->bats[i].vy -= bp->bats[i].vy / FRICTION;
				bp->bats[a_bat].vx -= amount * x / d;
				bp->bats[a_bat].vy -= amount * y / d;
				bp->bats[a_bat].vx -= bp->bats[a_bat].vx / FRICTION;
				bp->bats[a_bat].vy -= bp->bats[a_bat].vy / FRICTION;
				spin = (bp->bats[i].vang - bp->bats[a_bat].vang) /
					(2 * size * SLIPAGE);
				bp->bats[i].vang -= spin;
				bp->bats[a_bat].vang += spin;
				bp->bats[i].spindir = DIR(bp->bats[i].vang);
				bp->bats[a_bat].spindir = DIR(bp->bats[a_bat].vang);
				if (!bp->bats[i].vang) {
					bp->bats[i].spindelay = 1;
					bp->bats[i].spindir = 0;
				} else
					bp->bats[i].spindelay = M_PI * bp->bats[i].size /
						(ABS(bp->bats[i].vang)) + 1;
				if (!bp->bats[a_bat].vang) {
					bp->bats[a_bat].spindelay = 1;
					bp->bats[a_bat].spindir = 0;
				} else
					bp->bats[a_bat].spindelay = M_PI * bp->bats[a_bat].size /
						(ABS(bp->bats[a_bat].vang)) + 1;
				return;
			}
		}
	}
}

static void
drawabat(ModeInfo * mi, batstruct * bat)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	bouncestruct *bp = &bounces[MI_SCREEN(mi)];

	if (bat->sizex < bat0_width) {
		if (bat->xlast != -1) {
			XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
			XFillRectangle(display, window, MI_GC(mi),
			     bat->xlast, bat->ylast, bat->sizex, bat->sizey);
		}
		XSetForeground(display, MI_GC(mi), bat->color);
		XFillRectangle(display, window, MI_GC(mi),
			       bat->x, bat->y, bat->sizex, bat->sizey);
	} else {
		XSetForeground(display, MI_GC(mi), bat->color);
		XPutImage(display, window, MI_GC(mi),
			  bp->images[(bat->orient > ORIENTS / 2) ?
				     ORIENTS - bat->orient : bat->orient],
			  0, 0, bat->x, bat->y, bat->sizex, bat->sizey);
		if (bat->xlast != -1) {
			XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
			XEraseImage(display, window, MI_GC(mi),
				    bat->x, bat->y, bat->xlast, bat->ylast, bat->sizex, bat->sizey);
		}
	}
}

static void
movebat(bouncestruct * bp, batstruct * bat)
{
	bat->xlast = bat->x;
	bat->ylast = bat->y;
	bat->x += bat->vx;
	if (bat->x > (bp->width - bat->sizex)) {
		/* Bounce off the right edge */
		bat->x = 2 * (bp->width - bat->sizex) - bat->x;
		bat->vx = -bat->vx + bat->vx / FRICTION;
		flapbat(bat, 1, &bat->vy);
	} else if (bat->x < 0) {
		/* Bounce off the left edge */
		bat->x = -bat->x;
		bat->vx = -bat->vx + bat->vx / FRICTION;
		flapbat(bat, -1, &bat->vy);
	}
	bat->vy++;
	bat->y += bat->vy;
	if (bat->y >= (bp->height + bat->sizey)) {	/* Don't see bat bounce */
		/* Bounce off the bottom edge */
		bat->y = (bp->height - bat->sizey);
		bat->vy = -bat->vy + bat->vy / FRICTION;
		flapbat(bat, -1, &bat->vx);
	}			/* else if (bat->y < 0) { */
	/* Bounce off the top edge */
	/*bat->y = -bat->y;
	   bat->vy = -bat->vy + bat->vy / FRICTION;
	   flapbat(bat, 1, &bat->vx);
	   } */
	if (bat->spindir) {
		bat->spincount--;
		if (!bat->spincount) {
			bat->orient = (bat->spindir + bat->orient) % ORIENTS;
			bat->spincount = bat->spindelay;
		}
	}
}

static void
flapbat(batstruct * bat, int dir, int *vel)
{
	*vel -= (*vel + SIGN(*vel * dir) * bat->spindelay * ORIENTCYCLE /
		 (M_PI * bat->size)) / SLIPAGE;
	if (*vel) {
		bat->spindir = DIR(*vel * dir);
		bat->vang = *vel * ORIENTCYCLE;
		bat->spindelay = M_PI * bat->size / (ABS(bat->vang)) + 1;
	} else
		bat->spindir = 0;
}

static int
collide(bouncestruct * bp, int a_bat)
{
	int         i, d, x, y;

	for (i = 0; i < a_bat; i++) {
		x = (bp->bats[i].x - bp->bats[a_bat].x);
		y = (bp->bats[i].y - bp->bats[a_bat].y);
		d = (int) sqrt((double) (x * x + y * y));
		if (d < (bp->bats[i].size + bp->bats[a_bat].size) / 2)
			return i;
	}
	return i;
}

void
init_bat(ModeInfo * mi)
{
	bouncestruct *bp;
	int         i, tryagain = 0;

	if (bounces == NULL) {
		if ((bounces = (bouncestruct *) calloc(MI_NUM_SCREENS(mi),
					     sizeof (bouncestruct))) == NULL)
			return;
	}
	bp = &bounces[MI_SCREEN(mi)];
	if (!bp->initialized) {
		init_images(mi);
	}
	bp->width = MI_WIN_WIDTH(mi);
	bp->height = MI_WIN_HEIGHT(mi);
	if (bp->width < 2)
		bp->width = 2;
	if (bp->height < 2)
		bp->height = 2;
	bp->restartnum = TIME;

	bp->nbats = MI_BATCHCOUNT(mi);
	if (bp->nbats < -MINBATS) {
		/* if bp->nbats is random ... the size can change */
		if (bp->bats != NULL) {
			(void) free((void *) bp->bats);
			bp->bats = NULL;
		}
		bp->nbats = NRAND(-bp->nbats - MINBATS + 1) + MINBATS;
	} else if (bp->nbats < MINBATS)
		bp->nbats = MINBATS;
	if (!bp->bats)
		bp->bats = (batstruct *) malloc(bp->nbats * sizeof (batstruct));
	i = 0;
	while (i < bp->nbats) {
		if (bat0_width > bp->width / 2 || bat0_height > bp->height / 2) {
			bp->bats[i].sizex = 7;
			bp->bats[i].sizey = 3;
			bp->bats[i].size = (bp->bats[i].sizex + bp->bats[i].sizey) / 2;
		} else {
			bp->bats[i].sizex = bat0_width;
			bp->bats[i].sizey = bat0_height;
			bp->bats[i].size = (bp->bats[i].sizex + bp->bats[i].sizey) / 2;
		}
		bp->bats[i].vx = ((LRAND() & 1) ? -1 : 1) * (NRAND(MAX_STRENGTH) + 1);
		bp->bats[i].x = (bp->bats[i].vx >= 0) ? 0 : bp->width - bp->bats[i].sizex;
		bp->bats[i].y = NRAND(bp->height / 2);
		if (i == collide(bp, i) || tryagain >= 8) {
			if (MI_NPIXELS(mi) > 2)
				bp->bats[i].color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
			else
				bp->bats[i].color = MI_WIN_WHITE_PIXEL(mi);
			bp->bats[i].xlast = -1;
			bp->bats[i].ylast = 0;
			bp->bats[i].spincount = 1;
			bp->bats[i].spindelay = 1;
			bp->bats[i].vy = ((LRAND() & 1) ? -1 : 1) * NRAND(MAX_STRENGTH);
			bp->bats[i].spindir = 0;
			bp->bats[i].vang = 0;
			bp->bats[i].orient = NRAND(ORIENTS);
			i++;
		} else
			tryagain++;
	}
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_bat(ModeInfo * mi)
{
	bouncestruct *bp = &bounces[MI_SCREEN(mi)];
	int         i;

	for (i = 0; i < bp->nbats; i++) {
		drawabat(mi, &bp->bats[i]);
		movebat(bp, &bp->bats[i]);
	}
	for (i = 0; i < bp->nbats; i++)
		checkCollision(bp, i);
	if (!NRAND(TIME))	/* Put some randomness into the time */
		bp->restartnum--;
	if (!bp->restartnum)
		init_bat(mi);
}

void
release_bat(ModeInfo * mi)
{
	if (bounces != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			bouncestruct *bp = &bounces[screen];

#ifdef HAS_XPM
			int         i;

			for (i = 0; i < bp->initialized; i++)	/* ignores ARE_XBM */
				XDestroyImage(bp->images[i]);
#endif

			if (bp->bats != NULL)
				(void) free((void *) bp->bats);
		}
		(void) free((void *) bounces);
		bounces = NULL;
	}
}
