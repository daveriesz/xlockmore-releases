#ifndef lint
static char sccsid[] = "@(#)image.c	2.10 95/06/17 xlockmore";
#endif
/*-
 * image.c - image bouncer for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@source.asset.com>
 * 17-Jun-95: Pixmap stuff of Skip_Burrell@sterling.com added.
 * 07-Dec-94: Icons are now better centered if do not exactly fill an area.
 *
 * Changes of Patrick J. Naughton
 * 29-Jul-90: Written.
 */

#include "xlock.h"
#ifdef HAS_XPM
#include <X11/xpm.h>
#include "image.xpm"

#else
#include "image.xbm"

static XImage blogo = {
    0, 0,			/* width, height */
    0, XYBitmap, 0,		/* xoffset, format, data */
    LSBFirst, 8,		/* byte-order, bitmap-unit */
    LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};
#endif
static XImage *logo = NULL;

#define MAXICONS 64

typedef struct {
    int         x;
    int         y;
}           point;

typedef struct {
    int         width;
    int         height;
    int         nrows;
    int         ncols;
    int         xb;
    int         yb;
    int         xoff;
    int         yoff;
    int         iconmode;
    int         iconcount;
    point       icons[MAXICONS];
}           imagestruct;

static imagestruct ims[MAXSCREENS];

void
drawimage(win)
    Window      win;
{
    imagestruct *ip = &ims[screen];
    int         i;

    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    for (i = 0; i < ip->iconcount; i++) {
	if (!ip->iconmode)
	    XFillRectangle(dsp, win, Scr[screen].gc,
			   ip->xb + logo->width * ip->icons[i].x + ip->xoff,
			   ip->yb + logo->height * ip->icons[i].y + ip->yoff,
			   logo->width, logo->height);
	ip->icons[i].x = LRAND() % ip->ncols;
	ip->icons[i].y = LRAND() % ip->nrows;
    }
#ifndef HAS_XPM
    if (mono || Scr[screen].npixels <= 2)
      XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
#endif

    for (i = 0; i < ip->iconcount; i++) {
#ifndef HAS_XPM
	if (!mono && Scr[screen].npixels > 2)
	  XSetForeground(dsp, Scr[screen].gc,
	                 Scr[screen].pixels[LRAND() % Scr[screen].npixels]);
#endif

	XPutImage(dsp, win, Scr[screen].gc, logo,
		  0, 0,
		  ip->xb + logo->width * ip->icons[i].x + ip->xoff,
		  ip->yb + logo->height * ip->icons[i].y + ip-> yoff,
		  logo->width, logo->height);
    }
}

void
initimage(win)
    Window      win;
{
    XWindowAttributes xgwa;
    imagestruct *ip = &ims[screen];

/*
sprintf (buffer[0],  "#%06lx", Scr[screen].pixels[LRAND() % Scr[screen].npixels]);
sprintf (buffer[1],  "#%06lx", BlackPixel(dsp, screen));
sprintf (buffer[2],  "#%06lx", WhitePixel(dsp, screen));
 printf("%s %s %s\n", buffer[0], buffer[1], buffer[2]);
*/
    if (!logo)
#if HAS_XPM
      XpmCreateImageFromData(dsp, (char **) image_name, &logo, NULL, 0);
#else
    {
      blogo.data = (char *) image_bits;
      blogo.width = image_width;
      blogo.height = image_height;
      blogo.bytes_per_line = (image_width + 7) / 8;
      logo = &blogo;
    }
#endif

    (void) XGetWindowAttributes(dsp, win, &xgwa);
    ip->width = xgwa.width;
    ip->height = xgwa.height;
    if (ip->width > logo->width)
      ip->ncols = ip->width / logo->width;
    else
      ip->ncols = 1;
    if (ip->height > logo->height)
      ip->nrows = ip->height / logo->height;
    else
      ip->nrows = 1;
    ip->xoff = (ip->width - ip->ncols * logo->width) / 2;
    ip->yoff = (ip->height - ip->nrows * logo->height) / 2;
    ip->iconmode = (ip->ncols < 2 || ip->nrows < 2);
    if (ip->iconmode) {
	ip->xb = 0;
	ip->yb = 0;
	ip->iconcount = 1;	/* icon mode */
    } else {
	ip->xb = (ip->width - logo->width * ip->ncols) / 2;
	ip->yb = (ip->height - logo->height * ip->nrows) / 2;
	ip->iconcount = batchcount;
	if (ip->iconcount > MAXICONS)
	  ip->iconcount = 16;
    }
    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, ip->width, ip->height);
}
