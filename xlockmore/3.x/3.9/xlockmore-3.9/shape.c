#ifndef lint
static char sccsid[] = "@(#)shape.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * Copyright (c) 1992 by Jamie Zawinski
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 03-Nov-95: formerly rect.c
 * 11-Aug-95: slight change to initialization of pixmaps
 * 27-Jun-95: added ellipses
 * 27-Feb-95: patch for VMS
 * 29-Sep-94: multidisplay bug fix (epstein_caleb@jpmorgan.com)
 * 15-Jul-94: xlock version (David Bagley bagleyd@hertz.njit.edu)
 * 1992:     xscreensaver version (Jamie Zawinski jwz@netscape.com)
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

#include	"xlock.h"

#define NBITS 12

#ifdef VMS
#if 0
#include "../bitmaps/stipple.xbm"
#include "../bitmaps/cross_weave.xbm"
#include "../bitmaps/dimple1.xbm"
#include "../bitmaps/dimple3.xbm"
#include "../bitmaps/flipped_gray.xbm"
#include "../bitmaps/gray1.xbm"
#include "../bitmaps/gray3.xbm"
#include "../bitmaps/hlines2.xbm"
#include "../bitmaps/light_gray.xbm"
#include "../bitmaps/root_weave.xbm"
#include "../bitmaps/vlines2.xbm"
#include "../bitmaps/vlines3.xbm"
#else
#include <decw$bitmaps/stipple.xbm>
#include <decw$bitmaps/cross_weave.xbm>
#include <decw$bitmaps/dimple1.xbm>
#include <decw$bitmaps/dimple3.xbm>
#include <decw$bitmaps/flipped_gray.xbm>
#include <decw$bitmaps/gray1.xbm>
#include <decw$bitmaps/gray3.xbm>
#include <decw$bitmaps/hlines2.xbm>
#include <decw$bitmaps/light_gray.xbm>
#include <decw$bitmaps/root_weave.xbm>
#include <decw$bitmaps/vlines2.xbm>
#include <decw$bitmaps/vlines3.xbm>
#endif
#else
#include <X11/bitmaps/stipple>
#include <X11/bitmaps/cross_weave>
#include <X11/bitmaps/dimple1>
#include <X11/bitmaps/dimple3>
#include <X11/bitmaps/flipped_gray>
#include <X11/bitmaps/gray1>
#include <X11/bitmaps/gray3>
#include <X11/bitmaps/hlines2>
#include <X11/bitmaps/light_gray>
#include <X11/bitmaps/root_weave>
#include <X11/bitmaps/vlines2>
#include <X11/bitmaps/vlines3>
#endif
#define BITS(n,w,h)\
  rp->pixmaps[rp->init_bits++]=\
  XCreatePixmapFromBitmapData(dsp,win,(char *)n,w,h,1,0,1)

ModeSpecOpt shape_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         width;
	int         height;
	int         borderx;
	int         bordery;
	int         init_bits;
	int         time;
	int         x, y, w, h;
	GC          stippled_GC;
	Pixmap      pixmaps[NBITS];
} shapestruct;


static shapestruct shapes[MAXSCREENS];

void
init_shape(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	XGCValues   gcv;
	Screen     *scr;
	shapestruct *rp = &shapes[screen];

	scr = ScreenOfDisplay(dsp, screen);
	rp->width = MI_WIN_WIDTH(mi);
	rp->height = MI_WIN_HEIGHT(mi);
	rp->borderx = rp->width / 20;
	rp->bordery = rp->height / 20;
	rp->time = 0;

	if (!rp->init_bits) {
		gcv.foreground = WhitePixelOfScreen(scr);
		gcv.background = BlackPixelOfScreen(scr);
		gcv.fill_style = FillOpaqueStippled;
		rp->stippled_GC = XCreateGC(dsp, win,
			    GCForeground | GCBackground | GCFillStyle, &gcv);

		BITS(stipple_bits, stipple_width, stipple_height);
		BITS(cross_weave_bits, cross_weave_width, cross_weave_height);
		BITS(dimple1_bits, dimple1_width, dimple1_height);
		BITS(dimple3_bits, dimple3_width, dimple3_height);
		BITS(flipped_gray_bits, flipped_gray_width, flipped_gray_height);
		BITS(gray1_bits, gray1_width, gray1_height);
		BITS(gray3_bits, gray3_width, gray3_height);
		BITS(hlines2_bits, hlines2_width, hlines2_height);
		BITS(light_gray_bits, light_gray_width, light_gray_height);
		BITS(root_weave_bits, root_weave_width, root_weave_height);
		BITS(vlines2_bits, vlines2_width, vlines2_height);
		BITS(vlines3_bits, vlines3_width, vlines3_height);
	}
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, rp->width, rp->height);
}

void
draw_shape(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	XGCValues   gcv;
	shapestruct *rp = &shapes[screen];

	gcv.stipple = rp->pixmaps[LRAND() % NBITS];
	gcv.foreground = (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) ?
		Scr[screen].pixels[LRAND() % Scr[screen].npixels] :
		WhitePixel(dsp, screen);
	gcv.background = (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) ?
		Scr[screen].pixels[LRAND() % Scr[screen].npixels] :
		BlackPixel(dsp, screen);
	XChangeGC(dsp, rp->stippled_GC, GCStipple | GCForeground | GCBackground,
		  &gcv);
	if (LRAND() % 3) {
		rp->w = rp->borderx + (LRAND() % (rp->width - 2 * rp->borderx)) *
			(LRAND() % rp->width) / rp->width;
		rp->h = rp->bordery + (LRAND() % (rp->height - 2 * rp->bordery)) *
			(LRAND() % rp->height) / rp->height;
		rp->x = LRAND() % (rp->width - rp->w);
		rp->y = LRAND() % (rp->height - rp->h);
		if (LRAND() & 1)
			XFillRectangle(dsp, win, rp->stippled_GC, rp->x, rp->y, rp->w, rp->h);
		else
			XFillArc(dsp, win, rp->stippled_GC, rp->x, rp->y, rp->w, rp->h,
				 0, 23040);
	} else {
		XPoint      triangleList[3];

		triangleList[0].x = rp->borderx + LRAND() % (rp->width - 2 * rp->borderx);
		triangleList[0].y = rp->bordery + LRAND() % (rp->height - 2 * rp->bordery);
		triangleList[1].x = rp->borderx + LRAND() % (rp->width - 2 * rp->borderx);
		triangleList[1].y = rp->bordery + LRAND() % (rp->height - 2 * rp->bordery);
		triangleList[2].x = rp->borderx + LRAND() % (rp->width - 2 * rp->borderx);
		triangleList[2].y = rp->bordery + LRAND() % (rp->height - 2 * rp->bordery);
		XFillPolygon(dsp, win, rp->stippled_GC, triangleList, 3,
			     Convex, CoordModeOrigin);
	}


	if (++rp->time > MI_CYCLES(mi))
		init_shape(mi);
}
