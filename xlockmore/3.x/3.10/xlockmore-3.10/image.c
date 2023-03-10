
#ifndef lint
static char sccsid[] = "@(#)image.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * image.c - image bouncer for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 * 03-Nov-95: Patched to add an arbitrary xpm file. 
 * 21-Sep-95: Patch if xpm fails to load <Markus.Zellner@anu.edu.au>.
 * 17-Jun-95: Pixmap stuff of Skip_Burrell@sterling.com added.
 * 07-Dec-94: Icons are now better centered if do not exactly fill an area.
 *
 * Changes of Patrick J. Naughton
 * 29-Jul-90: Written.
 */

#include "xlock.h"
#include "ras.h"

/* aliases for vars defined in the bitmap file */
#define IMAGE_WIDTH	image_width
#define IMAGE_HEIGHT	image_height
#define IMAGE_BITS	image_bits
#define IMAGE_NAME	image_name

#include "image.xbm"

#ifdef HAS_XPM
#include "image.xpm"
#else
static char **image_name = NULL;

#endif

ModeSpecOpt image_opts =
{0, NULL, NULL, NULL};


#define MINICONS 1

extern void getImage(ModeInfo * mi, XImage ** logo,
		     int width, int height, unsigned char *bits, char **name,
		     int *graphics_format, Bool * newcolormap);
extern void destroyImage(XImage * logo, int graphics_format);

typedef struct {
	int         width;
	int         height;
	int         nrows;
	int         ncols;
	XPoint      image_offset;
	int         iconmode;
	int         iconcount;
	XPoint     *icons;
	Bool        newcolormap;
	int         graphics_format;
	GC          backGC;
	XImage     *logo;
	Colormap    cm;
	unsigned long black;
} imagestruct;

static imagestruct *ims = NULL;

void
init_image(ModeInfo * mi)
{
	Display *display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	imagestruct *ip;

	if (ims == NULL) {
		if ((ims = (imagestruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (imagestruct))) == NULL)
			return;
	}
	ip = &ims[MI_SCREEN(mi)];

	if (!ip->logo) {
		getImage(mi, &ip->logo, IMAGE_WIDTH, IMAGE_HEIGHT, IMAGE_BITS,
			 IMAGE_NAME, &ip->graphics_format, &ip->newcolormap);
		if (ip->newcolormap) {
			XGCValues   xgcv;

			ip->cm = XCreateColormap(display, window, MI_VISUAL(mi), AllocNone);
			SetImageColors(display, ip->cm);
			setColormap(display, window, ip->cm, MI_WIN_IS_INWINDOW(mi));
			xgcv.background = ip->black = GetBlack();
			ip->backGC = XCreateGC(display, window, GCBackground, &xgcv);
		} else {
			ip->cm = None;
			ip->black = MI_WIN_BLACK_PIXEL(mi);
			ip->backGC = MI_GC(mi);
		}
	}
	ip->width = MI_WIN_WIDTH(mi);
	ip->height = MI_WIN_HEIGHT(mi);
	if (ip->width > ip->logo->width)
		ip->ncols = ip->width / ip->logo->width;
	else
		ip->ncols = 1;
	if (ip->height > ip->logo->height)
		ip->nrows = ip->height / ip->logo->height;
	else
		ip->nrows = 1;
	ip->image_offset.x = (ip->width - ip->ncols * ip->logo->width) / 2;
	ip->image_offset.y = (ip->height - ip->nrows * ip->logo->height) / 2;
	ip->iconmode = (ip->ncols < 2 || ip->nrows < 2);
	if (ip->iconmode) {
		ip->iconcount = 1;	/* icon mode */
	} else {
		ip->iconcount = MI_BATCHCOUNT(mi);
		if (ip->iconcount < -MINICONS)
			ip->iconcount = NRAND(-ip->iconcount - MINICONS + 1) + MINICONS;
		else if (ip->iconcount < MINICONS)
			ip->iconcount = MINICONS;
	}
	if (ip->icons != NULL)
		(void) free((void *) ip->icons);
	ip->icons = (XPoint *) malloc(ip->iconcount * sizeof (XPoint));
	XSetForeground(display, ip->backGC, ip->black);
	XFillRectangle(display, window, ip->backGC,
		       0, 0, ip->width, ip->height);
}

void
draw_image(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	imagestruct *ip = &ims[MI_SCREEN(mi)];
	int         i;

	XSetForeground(display, ip->backGC, ip->black);
	for (i = 0; i < ip->iconcount; i++) {
		if (!ip->iconmode)
			XFillRectangle(display, window, ip->backGC,
			ip->logo->width * ip->icons[i].x + ip->image_offset.x,
				       ip->logo->height * ip->icons[i].y + ip->image_offset.y,
				       ip->logo->width, ip->logo->height);
		ip->icons[i].x = NRAND(ip->ncols);
		ip->icons[i].y = NRAND(ip->nrows);
	}

	if (MI_NPIXELS(mi) <= 2)
		XSetForeground(display, ip->backGC, MI_WIN_WHITE_PIXEL(mi));
	for (i = 0; i < ip->iconcount; i++) {
		if (MI_NPIXELS(mi) > 2)
			XSetForeground(display, ip->backGC,
				       MI_PIXEL(mi, NRAND(MI_NPIXELS(mi))));
		XPutImage(display, window, ip->backGC, ip->logo,
			  0, 0,
		       ip->logo->width * ip->icons[i].x + ip->image_offset.x,
		      ip->logo->height * ip->icons[i].y + ip->image_offset.y,
			  ip->logo->width, ip->logo->height);
	}
}

void
release_image(ModeInfo * mi)
{
	if (ims != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			imagestruct *ip = &ims[screen];

			if (ip->icons != NULL)
				(void) free((void *) ip->icons);
      if (ip->newcolormap) {
			  if (!ip->backGC)
				  XFreeGC(MI_DISPLAY(mi), ip->backGC);
			  if (ip->cm != None)
				  XFreeColormap(MI_DISPLAY(mi), ip->cm);
      }
			destroyImage(ip->logo, ip->graphics_format);
		}
		(void) free((void *) ims);
		ims = NULL;
	}
}
