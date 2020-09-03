#ifndef lint
static char sccsid[] = "@(#)image.c	3.9 96/05/25 xlockmore";

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
#include "image.xbm"

#ifdef HAS_XPM
#if 1
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#include "image.xpm"
#endif

extern int  XbmReadFileToImage(char *filename,
			       int *width, int *height, unsigned char **bits);

ModeSpecOpt image_opts =
{0, NULL, NULL, NULL};

extern char *imagefile;

static XImage blogo =
{
	0, 0,			/* width, height */
	0, XYBitmap, 0,		/* xoffset, format, data */
	LSBFirst, 8,		/* byte-order, bitmap-unit */
	LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};
static XImage *logo;


#define MAXICONS 64


typedef struct {
	int         width;
	int         height;
	int         nrows;
	int         ncols;
	XPoint      image_offset;
	int         iconmode;
	int         iconcount;
	XPoint      icons[MAXICONS];
	GC          back_gc;
	unsigned long black;
} imagestruct;

static imagestruct ims[MAXSCREENS];

void
draw_image(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	imagestruct *ip = &ims[MI_SCREEN(mi)];
	int         i;

	XSetForeground(display, ip->back_gc, ip->black);
	for (i = 0; i < ip->iconcount; i++) {
		if (!ip->iconmode)
			XFillRectangle(display, window, ip->back_gc,
			   logo->width * ip->icons[i].x + ip->image_offset.x,
			  logo->height * ip->icons[i].y + ip->image_offset.y,
				       logo->width, logo->height);
		ip->icons[i].x = LRAND() % ip->ncols;
		ip->icons[i].y = LRAND() % ip->nrows;
	}
	if (MI_WIN_IS_MONO(mi) || MI_NPIXELS(mi) <= 2)
		XSetForeground(display, ip->back_gc, WhitePixel(display, screen));

	for (i = 0; i < ip->iconcount; i++) {
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
			XSetForeground(display, ip->back_gc,
			  Scr[screen].pixels[LRAND() % Scr[screen].npixels]);

		XPutImage(display, window, ip->back_gc, logo,
			  0, 0,
			  logo->width * ip->icons[i].x + ip->image_offset.x,
			  logo->height * ip->icons[i].y + ip->image_offset.y,
			  logo->width, logo->height);
	}
}

void
init_image(ModeInfo * mi)
{
	Window      window = MI_WINDOW(mi);
	imagestruct *ip = &ims[screen];

	if (!logo) {
		int         ret = -6;

		ip->black = MI_WIN_BLACK_PIXEL(mi);
		ip->back_gc = MI_GC(mi);

		randomImage(imagefile);

		if (strlen(imagefile))
			if (readable(imagefile)) {
				if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
					if (RasterSuccess == RasterFileToImage(dsp, imagefile, &logo
					/* , Scr[screen].fgcol, Scr[screen].bgcol */ ))
						ret = 2;
					if (ret == 2 && MI_WIN_IS_INSTALL(mi) && !MI_WIN_IS_INROOT(mi)) {
						Colormap    cmap;	/* Probably should be in record for deallocation */
						XGCValues   xgcv;

						cmap = XCreateColormap(dsp, window, MI_VISUAL(mi), AllocNone);
						SetImageColors(dsp, cmap);
						set_colormap(window, cmap, MI_WIN_IS_INWINDOW(mi));
						xgcv.background = ip->black = GetBlack();
						ip->back_gc = XCreateGC(dsp, window, GCBackground,
								      &xgcv);
					}
#ifdef HAS_XPM
					if (ret < 0)
						if (XpmSuccess == XpmReadFileToImage(dsp, imagefile, &logo,
										     (XImage **) NULL, (XpmAttributes *) NULL))
							ret = 1;
#endif
				}
				if (ret < 0)
					if (BitmapSuccess == XbmReadFileToImage(imagefile,
							&blogo.width, &blogo.height, (unsigned char **) &blogo.data)) {
						blogo.bytes_per_line = (blogo.width + 7) / 8;
						logo = &blogo;
						ret = 1;
					}
				if (ret < 0)
					(void) fprintf(stderr,
						       "\"%s\" is in an unrecognized format\n", imagefile);
			} else
				(void) fprintf(stderr,
				  "could not read file \"%s\"\n", imagefile);
#ifdef HAS_XPM
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2 && ret < 0)
			if (XpmSuccess == XpmCreateImageFromData(dsp, image_name,
			    &logo, (XImage **) NULL, (XpmAttributes *) NULL))
				ret = 1;
#endif
		if (ret < 0) {
			blogo.data = (char *) image_bits;
			blogo.width = image_width;
			blogo.height = image_height;
			blogo.bytes_per_line = (image_width + 7) / 8;
			logo = &blogo;
			ret = 0;
		}
	}
	ip->width = MI_WIN_WIDTH(mi);
	ip->height = MI_WIN_HEIGHT(mi);
	if (ip->width > logo->width)
		ip->ncols = ip->width / logo->width;
	else
		ip->ncols = 1;
	if (ip->height > logo->height)
		ip->nrows = ip->height / logo->height;
	else
		ip->nrows = 1;
	ip->image_offset.x = (ip->width - ip->ncols * logo->width) / 2;
	ip->image_offset.y = (ip->height - ip->nrows * logo->height) / 2;
	ip->iconmode = (ip->ncols < 2 || ip->nrows < 2);
	if (ip->iconmode) {
		ip->iconcount = 1;	/* icon mode */
	} else {
		ip->iconcount = MI_BATCHCOUNT(mi);
		if (ip->iconcount > MAXICONS)
			ip->iconcount = 16;
	}
	XSetForeground(dsp, ip->back_gc, ip->black);
	XFillRectangle(dsp, window, ip->back_gc, 0, 0, ip->width, ip->height);
}
