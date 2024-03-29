/* -*- Mode: C; tab-width: 4 -*- */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)cartoon.c	4.03 97/05/10 xlockmore";

#endif

/*-
 * cartoon.c - A bouncing cartoon for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1988 by Sun Microsystems
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Revision History:
 *
 * 10-May-97: Compatible with xscreensaver
 * 14-Oct-96: <dhansen@metapath.com>
 *            Updated the equations for mapping the charater
 *            to follow a parabolic time curve.  Also adjust
 *            the rate based on the size (area) of the cartoon.
 *            Eliminated unused variables carried over from bounce.c
 *            Still needs a user attribute to control the rate
 *            depending on their computer and preference.
 * 20-Sep-94: I done this file starting from the previous bounce.c
 *            I draw the 8 cartoons like they are.
 *            Lorenzo Patocchi <patol@info.isbiel.ch>
 *            David monkeyed around with the cmap stuff to get it to work.
 * 2-Sep-93: xlock version David Bagley <bagleyd@bigfoot.com>
 * 1986: Sun Microsystems
 */

/*-
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

#ifdef STANDALONE
#define PROGCLASS            "Cartoon"
#define HACK_INIT            init_cartoon
#define HACK_DRAW            draw_cartoon
#define DEF_DELAY            10000
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */

#endif /* STANDALONE */

#if defined( USE_XPM ) || defined( USE_XPMINC )

#ifndef STANDALONE
ModeSpecOpt cartoon_opts =
{0, NULL, 0, NULL, NULL};

#endif

/* Yes, it does not have to be done again but you could override the above... */
#if defined( USE_XPM ) || defined( USE_XPMINC )

#if USE_XPMINC
#include <xpm.h>
#else
#include <X11/xpm.h>		/* Normal spot */
#endif

#include "pixmaps/calvin2.xpm"
#include "pixmaps/gravity.xpm"
#include "pixmaps/calvin4.xpm"
#include "pixmaps/calvinf.xpm"
#include "pixmaps/hobbes.xpm"
#include "pixmaps/calvin.xpm"
#include "pixmaps/marino2.xpm"
#include "pixmaps/calvin3.xpm"
#include "pixmaps/garfield.xpm"
#ifdef ALL_CARTOONS		/* Some of these gave me trouble on some systems */
#include "pixmaps/dragon.xpm"
#include "pixmaps/cal_hob.xpm"
#define NUM_CARTOONS    11
#else
#define NUM_CARTOONS    9
#endif
#else
#define NUM_CARTOONS    0	/* Not much point... */
#endif

#define MAX_X_STRENGTH  8
#define MAX_Y_STRENGTH  3

static char **cartoonlist[NUM_CARTOONS] =
{
#if NUM_CARTOONS
	calvin2, gravity, calvin4, calvinf, hobbes,
	calvin, marino2, calvin3, garfield
#ifdef ALL_CARTOONS
	,dragon, cal_hob
#endif
#endif
};

typedef struct {
	int         x, y, xlast, ylast;
	int         vx, vy;
	int         ytime, yrate, ybase;
	int         width, height;
	GC          backGC;
	XImage     *image;
	int         success;
	int         choice;
	Colormap    cm;
	unsigned long black, white;
} cartoonstruct;

static cartoonstruct *cartoons = NULL;

static void erase_image(Display * display, Window win, GC gc,
			int x, int y, int xl, int yl, int xsize, int ysize);

static int  tbx = 0;
static int  tby = 0;

static void
initimage(ModeInfo * mi, int cartoon)
{
#if defined( USE_XPM ) || defined( USE_XPMINC )
	cartoonstruct *cp = &cartoons[MI_SCREEN(mi)];
	XpmAttributes attrib;
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	if (cp->cm != None) {
		if (cp->backGC != None) {
			XFreeGC(MI_DISPLAY(mi), cp->backGC);
			cp->backGC = None;
		}
		XFreeColormap(MI_DISPLAY(mi), cp->cm);
	}
	if (!fixedColors(mi)) {
		cp->cm = XCreateColormap(display, window, MI_VISUAL(mi), AllocNone);
		attrib.colormap = cp->cm;
		reserveColors(mi, cp->cm, &cp->black, &cp->white);
	} else
		attrib.colormap = MI_WIN_COLORMAP(mi);

	attrib.visual = MI_VISUAL(mi);
	attrib.depth = MI_WIN_DEPTH(mi);
	attrib.valuemask = XpmVisual | XpmColormap | XpmDepth;

	if (cp->image)
		XDestroyImage(cp->image);
	/* if (MI_NPIXELS(mi) > 2) *//* Does not look too bad in monochrome */
	cp->success = (XpmSuccess == XpmCreateImageFromData(
								   display, cartoonlist[cartoon], &cp->image, (XImage **) NULL, &attrib));
#endif
	if (MI_WIN_IS_VERBOSE(mi))
		(void) fprintf(stderr, "Color image %d%s loaded.\n",
			       cartoon, (cp->success) ? "" : " could not be");
	if (cp->cm != None) {
		if (cp->backGC == None) {
			XGCValues   xgcv;

			setColormap(display, window, cp->cm, MI_WIN_IS_INWINDOW(mi));
			xgcv.background = cp->black;
			cp->backGC = XCreateGC(display, window, GCBackground, &xgcv);
		}
	} else {
		cp->black = MI_WIN_BLACK_PIXEL(mi);
		cp->backGC = MI_GC(mi);
	}
}

static void
put_cartoon(ModeInfo * mi)
{
	cartoonstruct *cp = &cartoons[MI_SCREEN(mi)];

/* PURIFY 4.0.1 on SunOS4 and on Solaris 2 reports a 29756 byte memory leak on
   * the next line. */
	XPutImage(MI_DISPLAY(mi), MI_WINDOW(mi), cp->backGC,
		  cp->image, 0, 0,
		  cp->x - tbx, cp->y - tby,
		  cp->image->width, cp->image->height);
	if (cp->xlast != -1) {
		erase_image(MI_DISPLAY(mi), MI_WINDOW(mi), cp->backGC,
			    cp->x - tbx, cp->y - tby,
			    cp->xlast, cp->ylast,
			    cp->image->width, cp->image->height);
	}
}				/* put_cartoon */

static void
select_cartoon(ModeInfo * mi)
{
#if ( NUM_CARTOONS <= 0 )
	return;
#else
	cartoonstruct *cp = &cartoons[MI_SCREEN(mi)];

#if ( NUM_CARTOONS > 1 )
	int         old;

	old = cp->choice;
	while (old == cp->choice)
		cp->choice = NRAND(NUM_CARTOONS);
	if (MI_WIN_IS_DEBUG(mi))
		(void) fprintf(stderr, "cartoon %d.\n", cp->choice);
	initimage(mi, cp->choice);
#else /* Probably will never be used but ... */
	cp->choice = 0;
	if (MI_WIN_IS_DEBUG(mi))
		(void) fprintf(stderr, "cartoon %d.\n", cp->choice);
	if (!cp->success)
		initimage(mi, cp->choice);
#endif
#endif
}				/* select_cartoon */

static void
move_cartoon(ModeInfo * mi)
{
	cartoonstruct *cp = &cartoons[MI_SCREEN(mi)];

	cp->xlast = cp->x - tbx;
	cp->ylast = cp->y - tby;

	cp->x += cp->vx;
	if ((cp->width > cp->image->width && cp->x > cp->width - cp->image->width) ||
	    (cp->width < cp->image->width && cp->x < 0)) {
		/* Bounce off the right edge */
		cp->x = 2 * (cp->width - cp->image->width) - cp->x;
		cp->vx = -cp->vx;
	} else if ((cp->width < cp->image->width &&
		    cp->x > cp->width - cp->image->width) ||
		   (cp->width > cp->image->width && cp->x < 0)) {
		/* Bounce off the left edge */
		cp->x = -cp->x;
		cp->vx = -cp->vx;
	}
	cp->ytime++;

	cp->y += ((cp->ytime / cp->yrate) - cp->ybase);


	if (cp->y >= (cp->height + cp->image->height)) {
		/* Back at the bottom edge, time for a new cartoon */
		init_cartoon(mi);
	}
}				/* move_cartoon */

/* This stops some flashing, could be more efficient */
static void
erase_image(Display * display, Window win, GC gc,
	    int x, int y, int xl, int yl, int xsize, int ysize)
{
	if (y < 0)
		y = 0;
	if (x < 0)
		x = 0;
	if (xl < 0)
		xl = 0;
	if (yl < 0)
		yl = 0;

	if (xl - x > 0)
		XFillRectangle(display, win, gc, x + xsize, yl, xl - x, ysize);
	if (yl - y > 0)
		XFillRectangle(display, win, gc, xl, y + ysize, xsize, yl - y);
	if (y - yl > 0)
		XFillRectangle(display, win, gc, xl, yl, xsize, y - yl);
	if (x - xl > 0)
		XFillRectangle(display, win, gc, xl, yl, x - xl, ysize);
}				/* erase_image */

void
init_cartoon(ModeInfo * mi)
{
	cartoonstruct *cp;
	int         i;

	if (cartoons == NULL) {
		if ((cartoons = (cartoonstruct *) calloc(MI_NUM_SCREENS(mi),
					    sizeof (cartoonstruct))) == NULL)
			return;
	}
	cp = &cartoons[MI_SCREEN(mi)];

	if (!cp->success) {
		cp->choice = -1;

	}
	cp->width = MI_WIN_WIDTH(mi);
	cp->height = MI_WIN_HEIGHT(mi);

	select_cartoon(mi);

	if (cp->success) {
		cp->vx = ((LRAND() & 1) ? -1 : 1) * NRAND(MAX_X_STRENGTH);

		cp->x = (cp->vx >= 0) ? 0 : cp->width - cp->image->width;

		cp->y = cp->height + cp->image->height;
		cp->ytime = 0;
		i = 100000 / (cp->image->width * cp->image->height);
		cp->yrate = (1 + NRAND(MAX_Y_STRENGTH)) * (i * i) / 4;
		cp->ybase = (int) sqrt(((3 * cp->y / 4) +
					(NRAND(cp->y / 4))) * 2 / cp->yrate);

		cp->xlast = -1;
		cp->ylast = 0;

		XSetForeground(MI_DISPLAY(mi), cp->backGC, cp->black);
		XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), cp->backGC,
			       0, 0, cp->width, cp->height);
	}
}				/* init_cartoon */

void
draw_cartoon(ModeInfo * mi)
{
	cartoonstruct *cp = &cartoons[MI_SCREEN(mi)];

	if (cp->success) {
		put_cartoon(mi);
		move_cartoon(mi);
	} else
		init_cartoon(mi);
}				/* draw_cartoon */

void
release_cartoon(ModeInfo * mi)
{
	if (cartoons != NULL) {
#if defined( USE_XPM ) || defined( USE_XPMINC )
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			cartoonstruct *cp = &cartoons[screen];

			if (cp->image)
				XDestroyImage(cp->image);
			if (cp->cm != None) {
				if (cp->backGC != None)
					XFreeGC(MI_DISPLAY(mi), cp->backGC);
				XFreeColormap(MI_DISPLAY(mi), cp->cm);
			}
		}
#endif
		(void) free((void *) cartoons);
		cartoons = NULL;
	}
}				/* release_cartoon */

#endif
