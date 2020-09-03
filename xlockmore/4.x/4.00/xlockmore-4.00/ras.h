/*-
 * @(#)ras.h	3.9 96/05/25 xlockmore
 *
 * Utilities for Sun rasterfile processing
 *
 * Copyright (c) 1995 by Tobias Gloth
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 12-Dec-95: Modified to be a used in more than one mode
 *            (joukj@alpha.chem.uva.nl)
 * 22-May-95: Written.
 */

#define RasterColorError   1
#define RasterSuccess      0
#define RasterOpenFailed  -1
#define RasterFileInvalid -2
#define RasterNoMemory    -3
#define RasterColorFailed -4

extern int  RasterFileToImage(Display * display, char *filename, XImage ** image
  /* , unsigned int fgpix, unsigned int bgpix */ );
extern void SetImageColors(Display * display, Colormap cmap);
extern unsigned long GetBlack(void);
extern unsigned long GetWhite(void);
extern unsigned long GetForeground(Display * display, unsigned long pixel);
extern unsigned long GetBackground(Display * display, unsigned long pixel);
