#ifndef lint
static char sccsid[] = "@(#)ras.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * Utilities for Sun rasterfile processing
 *
 * Copyright (c) 1995 by Tobias Gloth
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *  3-Mar-96: Added random image selection.
 * 12-Dec-95: Modified to be a used in more than one mode
 *            (joukj@alpha.chem.uva.nl)
 * 22-May-95: Written.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "xlock.h"
#include "ras.h"

char       *imagefile;

#define COLORMAPSIZE 0x100
#define RGBCOLORMAPSIZE 0x300
typedef struct {
	int         entry[COLORMAPSIZE];
	unsigned char header[32];
	unsigned char color[RGBCOLORMAPSIZE];
	unsigned char red[COLORMAPSIZE], green[COLORMAPSIZE], blue[COLORMAPSIZE];
	unsigned char *data;
	unsigned long sign;
	unsigned long width, height;
	unsigned long depth;
	unsigned long colors;
} raster;

static raster ras;

static unsigned long find_nearest(int r, int g, int b);
static unsigned long get_long(int n);

static void
analyze_header(void)
{
	ras.sign = get_long(0);
	ras.width = get_long(1);
	ras.height = get_long(2);
	ras.depth = get_long(3);
	ras.colors = get_long(7) / 3;
}

static void
convert_colors( /*Display *display, unsigned long fgpix, unsigned long bgpix */ )
{
	int         i;

	for (i = 0; i < ras.colors; i++) {
		ras.red[i] = ras.color[i + 0 * ras.colors];
		ras.green[i] = ras.color[i + 1 * ras.colors];
		ras.blue[i] = ras.color[i + 2 * ras.colors];
	}
#if 0
	{
		XColor      col;

		if (ras.colors <= 0xfc) {
			col.pixel = bgpix;
			XQueryColor(display, DefaultColormap(display, DefaultScreen(display)),
				    &col);
			ras.red[0xfc] = col.red >> 8;
			ras.green[0xfc] = col.green >> 8;
			ras.blue[0xfc] = col.blue >> 8;
		}
		if (ras.colors <= 0xfd) {
			col.pixel = fgpix;
			XQueryColor(display, DefaultColormap(display, DefaultScreen(display)),
				    &col);
			ras.red[0xfd] = col.red >> 8;
			ras.green[0xfd] = col.green >> 8;
			ras.blue[0xfd] = col.blue >> 8;
		}
	}
#endif
	if (ras.colors <= 0xfe)
		ras.red[0xfe] = ras.green[0xfe] = ras.blue[0xfe] = 0xff;
	if (ras.colors <= 0xff)
		ras.red[0xff] = ras.green[0xff] = ras.blue[0xff] = 0;
}

static unsigned long
find_nearest(int r, int g, int b)
{
	unsigned long i, minimum = RGBCOLORMAPSIZE, imin = 0, t;

	for (i = 0; i < COLORMAPSIZE; i++) {
		t = abs(r - (int) ras.red[i]) + abs(g - (int) ras.green[i]) +
		  abs(b - (int) ras.blue[i]);
		if (t < minimum) {
			minimum = t;
			imin = i;
		}
	}
#ifdef DEBUG
	(void) fprintf(stderr, "giving (%d/%d/%d)[%ld] for (%d/%d/%d) diff %ld\n",
		       ras.red[imin], ras.green[imin], ras.blue[imin], imin, r, g, b, minimum);
#endif
	return imin;
}

unsigned long
GetWhite(void)
{
	return ras.entry[find_nearest(0xff, 0xff, 0xff)];
}

unsigned long
GetBlack(void)
{
	return ras.entry[find_nearest(0, 0, 0)];
}

unsigned long
GetForeground(Display * display, unsigned long pixel)
{
	XColor      fgcol;

	fgcol.pixel = pixel;
	XQueryColor(display, DefaultColormap(display, DefaultScreen(display)),
		    &fgcol);
	return ras.entry[find_nearest(
			 fgcol.red >> 8, fgcol.green >> 8, fgcol.blue >> 8)];
}

unsigned long
GetBackground(Display * display, unsigned long pixel)
{
	XColor      bgcol;

	bgcol.pixel = pixel;
	XQueryColor(display, DefaultColormap(display, DefaultScreen(display)),
		    &bgcol);
	return ras.entry[find_nearest(
			 bgcol.red >> 8, bgcol.green >> 8, bgcol.blue >> 8)];
}

static unsigned long
get_long(int n)
{
	return
		(((unsigned long) ras.header[4 * n + 0]) << 24) +
		(((unsigned long) ras.header[4 * n + 1]) << 16) +
		(((unsigned long) ras.header[4 * n + 2]) << 8) +
		(((unsigned long) ras.header[4 * n + 3]) << 0);
}

int
RasterFileToImage(Display * display, char *filename, XImage ** image
	/* , unsigned int fgpix, unsigned int bgpix */ )
{
	int         read_width;
	FILE       *file;

	if (!(file = fopen(filename, "rb"))) {
		/*(void) fprintf(stderr, "could not read file \"%s\"\n", filename); */
		return RasterOpenFailed;
	}
	(void) fread((void *) ras.header, 8, 4, file);
	analyze_header();
	if (ras.sign != 0x59a66a95) {
		/* not a raster file */
		(void) fclose(file);
		return RasterFileInvalid;
	}
	if (ras.depth != 8) {
		(void) fclose(file);
		(void) fprintf(stderr, "only 8-bit Raster files are supported\n");
		return RasterColorFailed;
	}
	read_width = ras.width;
	if ((ras.width & 1) != 0)
		read_width++;
	ras.data = (unsigned char *) malloc((int) (read_width * ras.height));
	if (!ras.data) {
		(void) fprintf(stderr, "out of memory for Raster file\n");
		return RasterNoMemory;
	}
	*image = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
			      8, ZPixmap, 0,
			(char *) ras.data, (int) ras.width, (int) ras.height,
			      16, (int) read_width);
	if (!*image) {
		(void) fprintf(stderr, "could not create image from Raster file\n");
		return RasterColorError;
	}
	(void) fread((void *) ras.color, (int) ras.colors, 3, file);
	(void) fread((void *) ras.data, read_width, (int) ras.height, file);
	(void) fclose(file);
	convert_colors( /* display, fgpix, bgpix */ );
	return RasterSuccess;
}

void
SetImageColors(Display * display, Colormap cmap)
{
	XColor      xcolor[COLORMAPSIZE];
	int         i;

	for (i = 0; i < COLORMAPSIZE; i++) {
		xcolor[i].flags = DoRed | DoGreen | DoBlue;
		xcolor[i].red = ras.red[i] << 8;
		xcolor[i].green = ras.green[i] << 8;
		xcolor[i].blue = ras.blue[i] << 8;
		if (!XAllocColor(display, cmap, xcolor + i))
			error("not enough colors.\n");
		ras.entry[i] = xcolor[i].pixel;
	}
/* for (i = 0; i < ras.width * ras.height; i++) ras.data[i] =
   ras.entry[ras.data[i]]; */
}
