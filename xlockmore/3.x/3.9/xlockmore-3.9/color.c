#ifndef lint
static char sccsid[] = "@(#)color.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * color.c - extracted from swirl.c, xlock.c and utils.c
 *
 * See xlock.c for copying information.
 *
 * xlock.c and utils.c Copyright (c) 1988-91 by Patrick J. Naughton.
 * swirl.c Copyright (c) 1994 M.Dobie <mrd@ecs.soton.ac.uk>
 *
 */

#include "xlock.h"

/* Formerly in utils.c */
/*-
 * Create an HSB ramp.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 * 22-Jun-94: Modified for VMS
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * Changes of Patrick J. Naughton
 * 29-Jul-90: renamed hsbramp.c from HSBmap.c
 *	      minor optimizations.
 * 01-Sep-88: Written.
 */

#include <sys/types.h>
#include <math.h>

static void
hsb2rgb(double H, double S, double B,
	unsigned char *r, unsigned char *g, unsigned char *b)
{
	int         i;
	double      f, bb;
	unsigned char p, q, t;

	H -= floor(H);		/* remove anything over 1 */
	H *= 6.0;
	i = floor(H);		/* 0..5 */
	f = H - (float) i;	/* f = fractional part of H */
	bb = 255.0 * B;
	p = (unsigned char) (bb * (1.0 - S));
	q = (unsigned char) (bb * (1.0 - (S * f)));
	t = (unsigned char) (bb * (1.0 - (S * (1.0 - f))));
	switch (i) {
		case 0:
			*r = (unsigned char) bb;
			*g = t;
			*b = p;
			break;
		case 1:
			*r = q;
			*g = (unsigned char) bb;
			*b = p;
			break;
		case 2:
			*r = p;
			*g = (unsigned char) bb;
			*b = t;
			break;
		case 3:
			*r = p;
			*g = q;
			*b = (unsigned char) bb;
			break;
		case 4:
			*r = t;
			*g = p;
			*b = (unsigned char) bb;
			break;
		case 5:
			*r = (unsigned char) bb;
			*g = p;
			*b = q;
			break;
	}
}


/* 
 * Input is two points in HSB color space and a count
 * of how many discreet rgb space values the caller wants.
 *
 * Output is that many rgb triples which describe a linear
 * interpolate ramp between the two input colors.
 */

static void
hsbramp(double h1, double s1, double b1, double h2, double s2, double b2,
    int count, unsigned char *red, unsigned char *green, unsigned char *blue)
{
	double      dh, ds, db;

	dh = (h2 - h1) / count;
	ds = (s2 - s1) / count;
	db = (b2 - b1) / count;
	while (count--) {
		hsb2rgb(h1, s1, b1, red++, green++, blue++);
		h1 += dh;
		s1 += ds;
		b1 += db;
	}
}


/* Formerly in xlock.c */

unsigned long
allocpixel(Colormap cmap, char *name, char *def)
{
	XColor      col;
	XColor      tmp;

	(void) XParseColor(dsp, cmap, name, &col);
	if (!XAllocColor(dsp, cmap, &col)) {
		(void) fprintf(stderr, "could not allocate: %s, using %s instead\n",
			       name, def);
		(void) XAllocNamedColor(dsp, cmap, def, &col, &tmp);
	}
	return col.pixel;
}

void
fix_colormap(Window window, int screen, float saturation,
	     Bool install, Bool inroot, Bool inwindow, Bool verbose)
{
	Screen     *scr = ScreenOfDisplay(dsp, screen);
	Colormap    cmap, dcmap = DefaultColormapOfScreen(scr);
	int         colorcount = NUMCOLORS;
	unsigned char red[NUMCOLORS], green[NUMCOLORS], blue[NUMCOLORS];
	int         i;
	XColor      xcolor;
	static int  first = 1;
	static float lastsat[MAXSCREENS];

	/* XWindowAttributes wa; */

	if (first) {
		first = 0;
		for (i = 0; i < MAXSCREENS; i++)
			lastsat[i] = -1.0;
	}
	if (saturation != lastsat[screen]) {
		lastsat[screen] = saturation;

		cmap = Scr[screen].cmap;
		if (cmap != None)
			XFreeColors(dsp, cmap, Scr[screen].pixels, Scr[screen].npixels, 0);
		else {
			/* "allocate" the black and white pixels, so that they
			   will be included by XCopyColormapAndFree() if it
			   gets called */
			xcolor.pixel = BlackPixelOfScreen(scr);
			XQueryColor(dsp, dcmap, &xcolor);
			(void) XAllocColor(dsp, dcmap, &xcolor);
			xcolor.pixel = WhitePixelOfScreen(scr);
			XQueryColor(dsp, dcmap, &xcolor);
			(void) XAllocColor(dsp, dcmap, &xcolor);
			/*xcolor.pixel = Scr[screen].fgcol;
			   XQueryColor(dsp, dcmap, &xcolor);
			   (void) XAllocColor(dsp, dcmap, &xcolor);
			   xcolor.pixel = Scr[screen].bgcol;
			   XQueryColor(dsp, dcmap, &xcolor);
			   (void) XAllocColor(dsp, dcmap, &xcolor); */
			cmap = dcmap;
		}

		hsbramp(0.0, saturation, 1.0, 1.0, saturation, 1.0, colorcount,
			red, green, blue);

		Scr[screen].npixels = 0;
		for (i = 0; i < colorcount; i++) {
			xcolor.red = red[i] << 8;
			xcolor.green = green[i] << 8;
			xcolor.blue = blue[i] << 8;
			xcolor.flags = DoRed | DoGreen | DoBlue;

			if (!XAllocColor(dsp, cmap, &xcolor)) {
				if (!install || cmap != dcmap)
					break;
				if ((cmap = XCopyColormapAndFree(dsp, cmap)) == dcmap)
					break;
				if (verbose)
					(void) fprintf(stderr, "using private colormap\n");
				if (!XAllocColor(dsp, cmap, &xcolor))
					break;
			}
			Scr[screen].pixels[i] = xcolor.pixel;
			Scr[screen].npixels++;
		}
		if (verbose)
			(void) fprintf(stderr, "%d pixels allocated\n", Scr[screen].npixels);
	} else
		cmap = Scr[screen].cmap;

	if (install && !inroot) {
		/* (void) XGetWindowAttributes(dsp, window, &wa); if(cmap !=
		   wa.colormap) */
		set_colormap(window, cmap, inwindow);
	}
	Scr[screen].cmap = cmap;
}

void
set_colormap(Window window, Colormap map, Bool inwindow)
{
	XSetWindowColormap(dsp, window, map);
	/* Now, here we have a problem.  When we are running full-screen, the
	   window's override_redirect attribute is on.  So, the window manager
	   never gets the ColormapNotify event that gets generated on the
	   above XSetWindowColormap() call, and does not So, a quick solution
	   is to install it ourselves.  The problem with this is that it
	   violates the ICCCM convention that only window managers should
	   install colormaps. Indeed, Fvwm _enforces_ this by immediately
	   un-doing any XInstallColormap() performed by a client (which is why
	   this does not work right under Fvwm). */
	if (!inwindow)
		XInstallColormap(dsp, map);
}
