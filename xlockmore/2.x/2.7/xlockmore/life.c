#ifndef lint
static char sccsid[] = "@(#)life.c	2.7 95/02/21 xlockmore";
#endif
/*-
 * life.c - Conway's game of Life for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@source.asset.com>
 * 07-Jan-95: life now has a random soup pattern.
 * 07-Dec-94: life now has new organisms.  They are now better centered.
 *            Some of the nonperiodic forms were removed. New life forms
 *            were taken from xlife (an AMAZING collection of life forms).
 *            life's gliders now come from the edge of the screen except
 *             when generated by a life form.
 * 23-Nov-94: Bug fix for different iconified window sizes  
 * 21-Jul-94: Took out bzero & bcopy since memset & memcpy is more portable
 * 10-Jun-94: Changed name of function 'kill', which is a libc function on
 *            many systems from Victor Langeveld <vic@mbfys.kun.nl>
 *
 * Changes of Patrick J. Naughton
 * 24-May-91: Added wraparound code from johnson@bugs.comm.mot.com.
 *	      Made old cells stay blue.
 *	      Made batchcount control the number of generations till restart.
 * 29-Jul-90: support for multiple screens.
 * 07-Feb-90: remove bogus semi-colon after #include line.
 * 15-Dec-89: Fix for proper skipping of {White,Black}Pixel() in colors.
 * 08-Oct-89: Moved seconds() to an extern.
 * 20-Sep-89: Written (life algorithm courtesy of Jim Graham, flar@sun.com).
 */

#include "xlock.h"
#include "life.xbm"

static XImage logo = {
    0, 0,			/* width, height */
    0, XYBitmap, 0,		/* xoffset, format, data */
    LSBFirst, 8,		/* byte-order, bitmap-unit */
    LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};
#define	MAXROWS	155
#define MAXCOLS	144
#define TIMEOUT 30

typedef struct {
    int         pixelmode;
    int         xs;
    int         ys;
    int         xb;
    int         yb;
    int         generation;
    long        shooterTime;
    int         nrows;
    int         ncols;
    int         width;
    int         height;
    unsigned char buffer[(MAXROWS + 2) * (MAXCOLS + 2) + 2];
    unsigned char tempbuf[MAXCOLS * 2];
    unsigned char lastbuf[MAXCOLS];
    unsigned char agebuf[(MAXROWS + 2) * (MAXCOLS + 2)];
}           lifestruct;

static lifestruct lifes[MAXSCREENS];
static int  icon_width, icon_height;

/* Buffer stores the data for each cell. Each cell is stored as
 * 8 bits representing the presence of a critter in each of it's
 * surrounding 8 cells. There is an empty row and column around
 * the whole array to allow stores without bounds checking as well
 * as an extra row at the end for the fetches into tempbuf.
 *
 * Tempbuf stores the data for the next two rows so that we know
 * the state of those critter before he was modified by the fate
 * of the critters that have already been processed.
 *
 * Agebuf stores the age of each critter.
 */

#define	UPLT	0x01
#define UP	0x02
#define UPRT	0x04
#define LT	0x08
#define RT	0x10
#define DNLT	0x20
#define DN	0x40
#define DNRT	0x80

/* Fates is a lookup table for the fate of a critter. The 256
 * entries represent the 256 possible combinations of the 8
 * neighbor cells. Each entry is one of BIRTH (create a cell
 * or leave one alive), SAME (leave the cell alive or dead),
 * or DEATH (kill anything in the cell).
 */
#define BIRTH	0
#define SAME	1
#define DEATH	2
#define NUMPTS  63
static unsigned char fates[256];
static int  initialized = 0;

/* Patterns have < NUMPTS pts (and should have a size of <= 32x32,
   the Gun is an exception) */
static int  patterns[][2 * NUMPTS + 1] = {
    {                           /* GLIDER GUN */
        6, -4,
        5, -3, 6, -3,
        -6, -2, -5, -2, 8, -2, 9, -2, 16, -2,
        -7, -1, 8, -1, 9, -1, 10, -1, 16, -1, 17, -1,
        -18, 0, -17, 0, -8, 0, 8, 0, 9, 1,
        -17, 1, -8, 1, 5, 1, 6, 1,
        -8, 2, 6, 2,
        -7, 3,
        -6, 4, -5, 4,
        127
    },
    {				/* FIGURE EIGHT */
	-3, -3, -2, -3, -1, -3,
	-3, -2, -2, -2, -1, -2,
	-3, -1, -2, -1, -1, -1,
	0, 0, 1, 0, 2, 0,
	0, 1, 1, 1, 2, 1,
	0, 2, 1, 2, 2, 2,
	127
    },
    {				/* PULSAR */
	-2, -1, -1, -1, 0, -1, 1, -1, 2, -1,
	-2, 0, 2, 0,
	127
    },
    {				/* BARBER POLE P2 */
        -6, -6, -5, -6,
        -6, -5, -4, -5,
        -4, -3, -2, -3,
        -2, -1, 0, -1,
        0, 1, 2, 1,
        2, 3, 4, 3,
        5, 4,
        4, 5, 5, 5,
        127
    },
    {				/* ACHIM P5 */
        -6, -6, -5, -6,
        -6, -5,
        -4, -4,
        -4, -3, -2, -3,
        -2, -1, 0, -1,
        0, 1, 2, 1,
        2, 3, 3, 3,
        5, 4,
        4, 5, 5, 5,
	127
    },
    {				/* HERTZ P4 */
	-2, -5, -1, -5,
	-2, -4, -1, -4,
	-7, -2, -6, -2, -2, -2, -1, -2, 0, -2, 1, -2, 5, -2, 6, -2,
	-7, -1, -5, -1, -3, -1, 2, -1, 4, -1, 6, -1,
	-5, 0, -3, 0, -2, 0, 2, 0, 4, 0,
	-7, 1, -5, 1, -3, 1, 2, 1, 4, 1, 6, 1,
	-7, 2, -6, 2, -2, 2, -1, 2, 0, 2, 1, 2, 5, 2, 6, 2,
	-2, 4, -1, 4,
	-2, 5, -1, 5,
	127
    },
    {				/* TUMBLER */
        -2, -3, -1, -3, 1, -3, 2, -3,
        -2, -2, -1, -2, 1, -2, 2, -2,
        -1, -1, 1, -1,
        -3, 0, -1, 0, 1, 0, 3, 0,
        -3, 1, -1, 1, 1, 1, 3, 1,
        -3, 2, -2, 2, 2, 2, 3, 2,
	127
    },
    {				/* PULSE1 P4*/
        0, -3, 1, -3,
        -2, -2, 0, -2,
        -3, -1, 3, -1,
        -2, 0, 2, 0, 3, 0,
        0, 2, 2, 2,
        1, 3,
	127
    },
    {				/* SHINING FLOWER P5 */
	-1, -4, 0, -4,
	-2, -3, 1, -3,
	-3, -2, 2, -2,
	-4, -1, 3, -1,
	-4, 0, 3, 0,
	-3, 1, 2, 1,
	-2, 2, 1, 2,
	-1, 3, 0, 3,
	127
    },
    {				/* PULSE2 P6 */
	0, -4, 1, -4,
	-4, -3, -3, -3, -1, -3,
	-4, -2, -3, -2, 0, -2, 3, -2,
	1, -1, 3, -1,
	2, 0,
	1, 2, 2, 2,
	1, 3, 2, 3,
	127
    },
    {				/* PINWHEEL, CLOCK P4 */
	-2, -6, -1, -6,
	-2, -5, -1, -5,
	-2, -3, -1, -3, 0, -3, 1, -3,
	-3, -2, -1, -2, 2, -2, 4, -2, 5, -2,
	-3, -1, 1, -1, 2, -1, 4, -1, 5, -1,
	-6, 0, -5, 0, -3, 0, 0, 0, 2, 0,
	-6, 1, -5, 1, -3, 1, 2, 1,
	-2, 2, -1, 2, 0, 2, 1, 2,
	0, 4, 1, 4,
	0, 5, 1, 5,
	127
    },
    {				/* PENTADECATHOLON */
	-5, 0, -4, 0, -3, 0, -2, 0, -1, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0,
	127
    },
    {                           /* PISTON */
        1, -3, 2, -3,
        0, -2,
        -10, -1, -1, -1,
        -11, 0, -10, 0, -1, 0,  9, 0, 10, 0,
        -1, 1, 9, 1,
        0, 2,
        1, 3, 2, 3,
	127
    },
    {                           /* PISTON2 */
       -3, -5,
       -14, -4, -13, -4, -4, -4, -3, -4, 13, -4, 14, -4,
       -14, -3, -13, -3, -5, -3, -4, -3, 13, -3, 14, -3,
       -4, -2, -3, -2, 0, -2, 1, -2,
       -4,  2, -3, 2, 0, 2, 1, 2,
       -14, 3, -13, 3, -5, 3, -4, 3, 13, 3, 14, 3,
       -14, 4, -13, 4, -4, 4, -3, 4, 13, 4, 14, 4,
       -3, 5,
        127
    },
    {                           /* SWITCH ENGINE */
        -12, -3, -10, -3,
        -13, -2,
        -12, -1, -9, -1,
        -10, 0, -9,  0, -8,  0,
        13, 2, 14,  2,
        13, 3,
        127
    },
    {                           /* GEARS (gear, flywheel, blinker) */
        -1, -4,
        -1, -3, 1, -3,
        -3, -2,
        2, -1, 3, -1,
        -4, 0, -3, 0,
        2, 1,
        -2, 2, 0, 2,
        0, 3,

        5, 3,
        3, 4, 4, 4,
        5, 5, 6, 5,
        4, 6,

        8, 0,
        8, 1,
        8, 2,
        127
    },
    {                           /* TURBINE8 */
        -4, -4, -3, -4, -2, -4, -1, -4, 0, -4, 1, -4, 3, -4, 4, -4,
        -4, -3, -3, -3, -2, -3, -1, -3, 0, -3, 1, -3, 3, -3, 4, -3,
        3, -2, 4, -2,
        -4, -1, -3, -1, 3, -1, 4, -1,
        -4, 0, -3, 0, 3, 0, 4, 0,
        -4, 1, -3, 1, 3, 1, 4, 1,
        -4, 2, -3, 2,
        -4, 3, -3, 3, -1, 3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3,
        -4, 4, -3, 4, -1, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4,
        127
    },
    {                            /* P16 */
        -3, -6, 1, -6, 2, -6, 
        -3, -5, 0, -5, 3, -5, 
        3, -4, 
        -5, -3, -4, -3, 1, -3, 2, -3, 5, -3, 6, -3, 
        -6, -2, -3, -2, 
        -6, -1, -3, -1, 
        -5, 0, 5, 0, 
        3, 1, 6, 1, 
        3, 2, 6, 2, 
        -6, 3, -5, 3, -2, 3, -1, 3, 4, 3, 5, 3, 
        -3, 4, 
        -3, 5, 0, 5, 3, 5, 
        -2, 6, -1, 6, 3, 6, 
        127
    },
    {				/* PUFFER */
        1, -9,
        2, -8,
        -2, -7, 2, -7,
        -1, -6, 0, -6, 1, -6, 2, -6,
        -2, -2,
        -1, -1, 0, -1,
        0, 0,
        0, 1,
        -1, 2,
        1, 5,
        2, 6,
        -2, 7, 2, 7,
        -1, 8, 0, 8, 1, 8, 2, 8,
        127
    },
    {				/* ESCORT */
        3, -8,
        4, -7,
        -2, -6, 4, -6,
        -1, -5, 0, -5, 1, -5, 2, -5, 3, -5, 4, -5,
        -5, -1, -4, -1, -3, -1, -2, -1, -1, -1, 0, -1,
        1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1,
        -6, 0, 6, 0,
        6, 1,
        5, 2,
        3, 4,
        4, 5,
        -2, 6, 4, 6,
        -1, 7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7,
        127
    },
    {                           /* DART SPEED 1/3 */
        3, -7,
        2, -6, 4, -6,
        1, -5, 2, -5,
        4, -4,
        0, -3, 4, -3,
        -3, -2, 0, -2,
        -4, -1, -2, -1, 1, -1, 2, -1, 3, -1, 4, -1,
        -5, 0, -2, 0,
        -4, 1, -2, 1, 1, 1, 2, 1, 3, 1, 4, 1,
        -3, 2, 0, 2,
        0, 3, 4, 3,
        4, 4,
        1, 5, 2, 5,
        2, 6, 4, 6,
        3, 7,
        127
    },
    {                           /* PERIOD 4 SPEED 1/2 */
        -3, -5,
        -4, -4, -3, -4, -2, -4, -1, -4, 0, -4,
        -5, -3, -4, -3, 0, -3, 1, -3, 3, -3,
        -4, -2, 4, -2,
        -3, -1, -2, -1, 1, -1, 3, -1,
        -3, 1, -2, 1, 1, 1, 3, 1,
        -4, 2, 4, 2,
        -5, 3, -4, 3, 0, 3, 1, 3, 3, 3,
        -4, 4, -3, 4, -2, 4, -1, 4, 0, 4,
        -3, 5,
        127
    },
    {                           /* ANOTHER PERIOD 4 SPEED 1/2 */
        -4, -7, -3, -7, -1, -7, 0, -7, 1, -7, 2, -7, 3, -7, 4, -7, 
        -5, -6, -4, -6, -3, -6, -2, -6, 5, -6, 
        -6, -5, -5, -5, 
        -5, -4, 5, -4, 
        -4, -3, -3, -3, -2, -3, 0, -3, 
        -2, -2, 
        -2, -1, 
        -1, 0, 
        -2, 1, 
        -2, 2, 
        -4, 3, -3, 3, -2, 3, 0, 3, 
        -5, 4, 5, 4, 
        -6, 5, -5, 5, 
        -5, 6, -4, 6, -3, 6, -2, 6, 5, 6, 
        -4, 7, -3, 7, -1, 7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 
        127
    },
    {                           /* SMALLEST KNOWN PERIOD 3 SPACESHIP SPEED 1/3 */
        0, -8, 
        -1, -7, 1, -7, 
        -1, -6, 1, -6, 
        -1, -5, 
        -2, -3, -1, -3, 
        -1, -2, 1, -2, 
        -2, -1, 0, -1, 
        -2, 0, -1, 0, 0, 0, 
        -1, 2, 1, 2, 
        -1, 3, 0, 3, 
        0, 4, 
        0, 5, 2, 5, 
        0, 6, 2, 6, 
        1, 7, 
        127
    },
    {                           /* TURTLE SPEED 1/3 */
        -4, -5, -3, -5, -2, -5, 6, -5,
        -4, -4, -3, -4, 0, -4, 2, -4, 3, -4, 5, -4, 6, -4,
        -2, -3, -1, -3, 0, -3, 5, -3,
        -4, -2, -1, -2, 1, -2, 5, -2,
        -5, -1, 0, -1, 5, -1,
        -5, 0, 0, 0, 5, 0,
        -4, 1, -1, 1, 1, 1, 5, 1,
        -2, 2, -1, 2, 0, 2, 5, 2,
        -4, 3, -3, 3, 0, 3, 2, 3, 3, 3, 5, 3, 6, 3,
        -4, 4, -3, 4, -2, 4, 6, 4,
        127
    },
    {                           /* SMALLEST KNOWN PERIOD 5 SPEED 2/5 */
        1, -7, 3, -7,
        -2, -6, 3, -6,
        -3, -5, -2, -5, -1, -5, 4, -5,
        -4, -4, -2, -4,
        -5, -3, -4, -3, -1, -3, 0, -3, 5, -3,
        -4, -2, -3, -2, 0, -2, 1, -2, 2, -2, 3, -2, 4, -2,
        -4, 2, -3, 2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2,
        -5, 3, -4, 3, -1, 3, 0, 3, 5, 3,
        -4, 4, -2, 4,
        -3, 5, -2, 5, -1, 5, 4, 5,
        -2, 6, 3, 6,
        1, 7, 3, 7,
        127
    },
    {				/* SYM PUFFER */
        1, -4, 2, -4, 3, -4, 4, -4,
        0, -3, 4, -3,
        4, -2,
        -4, -1, -3, -1, 0, -1, 3, -1,
        -4, 0, -3, 0, -2, 0,
        -4, 1, -3, 1, 0, 1, 3, 1,
        4, 2,
        0, 3, 4, 3,
        1, 4, 2, 4, 3, 4, 4, 4,
        127
    },
    {				/* ], NEAR SHIP, PI HEPTOMINO */
	-2, -1, -1, -1, 0, -1,
	1, 0,
	-2, 1, -1, 1, 0, 1,
	127
    },
    {                           /* R PENTOMINO */
        0, -1, 1, -1,
        -1, 0, 0, 0,
        0, 1,
        127
    }
};

#define NPATS	(sizeof patterns / sizeof patterns[0])


static void
drawcell(win, row, col)
    Window      win;
    int         row, col;
{
    lifestruct *lp = &lifes[screen];

    XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
    if (!mono && Scr[screen].npixels > 2) {
	unsigned char *loc = lp->buffer + ((row + 1) * (lp->ncols + 2)) + col + 1;
	unsigned char *ageptr = lp->agebuf + (loc - lp->buffer);
	unsigned char age = *ageptr;

	/* if we aren't up to blue yet, then keep aging the cell. */
	if (age < Scr[screen].npixels * 0.7)
	    ++age;

	XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[age]);
	*ageptr = age;
    }
    if (lp->pixelmode)
	XFillRectangle(dsp, win, Scr[screen].gc,
	       lp->xb + lp->xs * col, lp->yb + lp->ys * row, lp->xs, lp->ys);
    else
	XPutImage(dsp, win, Scr[screen].gc, &logo,
		  0, 0, lp->xb + lp->xs * col, lp->yb + lp->ys * row,
		  icon_width, icon_height);
}


static void
erasecell(win, row, col)
    Window      win;
    int         row, col;
{
    lifestruct *lp = &lifes[screen];
    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc,
	       lp->xb + lp->xs * col, lp->yb + lp->ys * row, lp->xs, lp->ys);
}


static void
spawn(loc)
    unsigned char *loc;
{
    lifestruct *lp = &lifes[screen];
    unsigned char *ulloc, *ucloc, *urloc, *clloc, *crloc, *llloc, *lcloc, *lrloc,
               *arloc;
    int         off, row, col, lastrow;

    lastrow = (lp->nrows) * (lp->ncols + 2);
    off = loc - lp->buffer;
    col = off % (lp->ncols + 2);
    row = (off - col) / (lp->ncols + 2);
    ulloc = loc - lp->ncols - 3;
    ucloc = loc - lp->ncols - 2;
    urloc = loc - lp->ncols - 1;
    clloc = loc - 1;
    crloc = loc + 1;
    arloc = loc + 1;
    llloc = loc + lp->ncols + 1;
    lcloc = loc + lp->ncols + 2;
    lrloc = loc + lp->ncols + 3;
    if (row == 1) {
	ulloc += lastrow;
	ucloc += lastrow;
	urloc += lastrow;
    }
    if (row == lp->nrows) {
	llloc -= lastrow;
	lcloc -= lastrow;
	lrloc -= lastrow;
    }
    if (col == 1) {
	ulloc += lp->ncols;
	clloc += lp->ncols;
	llloc += lp->ncols;
    }
    if (col == lp->ncols) {
	urloc -= lp->ncols;
	crloc -= lp->ncols;
	lrloc -= lp->ncols;
    }
    *ulloc |= UPLT;
    *ucloc |= UP;
    *urloc |= UPRT;
    *clloc |= LT;
    *crloc |= RT;
    *arloc |= RT;
    *llloc |= DNLT;
    *lcloc |= DN;
    *lrloc |= DNRT;

    *(lp->agebuf + (loc - lp->buffer)) = 0;
}


static void
ckill(loc)
    unsigned char *loc;
{
    lifestruct *lp = &lifes[screen];

    unsigned char *ulloc, *ucloc, *urloc, *clloc, *crloc, *llloc, *lcloc,
               *lrloc, *arloc;
    int         off, row, col, lastrow;

    lastrow = (lp->nrows) * (lp->ncols + 2);
    off = loc - lp->buffer;
    row = off / (lp->ncols + 2);
    col = off % (lp->ncols + 2);
    row = (off - col) / (lp->ncols + 2);
    ulloc = loc - lp->ncols - 3;
    ucloc = loc - lp->ncols - 2;
    urloc = loc - lp->ncols - 1;
    clloc = loc - 1;
    crloc = loc + 1;
    arloc = loc + 1;
    llloc = loc + lp->ncols + 1;
    lcloc = loc + lp->ncols + 2;
    lrloc = loc + lp->ncols + 3;
    if (row == 1) {
	ulloc += lastrow;
	ucloc += lastrow;
	urloc += lastrow;
    }
    if (row == lp->nrows) {
	llloc -= lastrow;
	lcloc -= lastrow;
	lrloc -= lastrow;
    }
    if (col == 1) {
	ulloc += lp->ncols;
	clloc += lp->ncols;
	llloc += lp->ncols;
    }
    if (col == lp->ncols) {
	urloc -= lp->ncols;
	crloc -= lp->ncols;
	lrloc -= lp->ncols;
    }
    *ulloc &= ~UPLT;
    *ucloc &= ~UP;
    *urloc &= ~UPRT;
    *clloc &= ~LT;
    *crloc &= ~RT;
    *arloc &= ~RT;
    *llloc &= ~DNLT;
    *lcloc &= ~DN;
    *lrloc &= ~DNRT;
}


static void
setcell(win, row, col)
    Window      win;
    int         row;
    int         col;
{
    lifestruct *lp = &lifes[screen];
    unsigned char *loc;

    loc = lp->buffer + ((row + 1) * (lp->ncols + 2)) + col + 1;
    spawn(loc);
    drawcell(win, row, col);
}


static void
init_fates()
{
    int         i, bits, neighbors;

    for (i = 0; i < 256; i++) {
	neighbors = 0;
	for (bits = i; bits; bits &= (bits - 1))
	    neighbors++;
	if (neighbors == 3)
	    fates[i] = BIRTH;
	else if (neighbors == 2)
	    fates[i] = SAME;
	else
	    fates[i] = DEATH;
    }
}

static void RandomSoup(win, n, v)
Window win;
int n, v;
{
  lifestruct *lp = &lifes[screen];
  int row, col;

  v /= 2;
  if (v < 1)
    v = 1;
  for (row = lp->nrows / 2 - v; row < lp->nrows / 2 + v; ++row)
    for (col = lp->ncols / 2 - v; col < lp->ncols / 2 + v; ++col)
        if ((RAND() % 100) < n && col >= 0 && row >= 0 &&
            col < lp->ncols && row < lp->nrows)
	  setcell(win, row, col);
}

static void GetPattern(win, i)
Window win;
int i;
{
  lifestruct *lp = &lifes[screen];
  int row, col;
  int *patptr;

  patptr = &patterns[i][0];
  while ((col = *patptr++) != 127) {
    row = *patptr++;
    col += lp->ncols / 2;
    row += lp->nrows / 2;
    if (col >= 0 && row >= 0 && col < lp->ncols && row < lp->nrows)
      setcell(win, row, col);
  }
}

void
initlife(win)
    Window      win;
{
    XWindowAttributes xgwa;
    lifestruct *lp = &lifes[screen];
    int i;

    lp->generation = 0;
    lp->shooterTime = seconds();
    icon_width = life_width;
    icon_height = life_height;

    if (!initialized) {
	initialized = 1;
	init_fates();
	logo.data = (char *) life_bits;
	logo.width = icon_width;
	logo.height = icon_height;
	logo.bytes_per_line = (icon_width + 7) / 8;
    }
    (void) XGetWindowAttributes(dsp, win, &xgwa);
    lp->width = xgwa.width;
    lp->height = xgwa.height;
    lp->pixelmode = (lp->width + lp->height < 8 * (icon_width + icon_height));
    if (lp->pixelmode) {
	lp->ncols = min(lp->width / 2, MAXCOLS);
	lp->nrows = min(lp->height / 2, MAXROWS);
    } else {
	lp->ncols = min(lp->width / icon_width, MAXCOLS);
	lp->nrows = min(lp->height / icon_height, MAXROWS);
    }
    lp->xs = lp->width / lp->ncols;
    lp->ys = lp->height / lp->nrows;
    lp->xb = (lp->width - lp->xs * lp->ncols) / 2;
    lp->yb = (lp->height - lp->ys * lp->nrows) / 2;

    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, lp->width, lp->height);

    (void) memset(lp->buffer, 0, sizeof(lp->buffer));

    i = RAND() % (NPATS + 2);
    if (i >= NPATS)
      RandomSoup(win, 30, 15);
    else
      GetPattern(win, i);
}

static void shooter(win)
Window win;
{
    int hsp, vsp, hoff = 1, voff = 1;
    lifestruct *lp = &lifes[screen];

    /* Generate the glider at the edge of the screen */
    if (RAND() % 2) {
      hsp = (RAND() % 2) ? 0 : lp->ncols - 1;
      vsp = RAND() % lp->nrows;
    } else {
      vsp = (RAND() % 2) ? 0 : lp->nrows - 1;
      hsp = RAND() % lp->ncols;
    } 
    if (vsp > lp->nrows / 2)
      voff = -1;
    if (hsp > lp->ncols / 2)
      hoff = -1;
    setcell(win, vsp + 0 * voff, hsp + 2 * hoff);
    setcell(win, vsp + 1 * voff, hsp + 2 * hoff);
    setcell(win, vsp + 2 * voff, hsp + 2 * hoff);
    setcell(win, vsp + 2 * voff, hsp + 1 * hoff);
    setcell(win, vsp + 1 * voff, hsp + 0 * hoff);
    lp->shooterTime = seconds();
}

void
drawlife(win)
    Window      win;
{
    unsigned char *loc, *temploc, *lastloc;
    int         row, col;
    unsigned char fate;
    lifestruct *lp = &lifes[screen];

    loc = lp->buffer + lp->ncols + 2 + 1;
    temploc = lp->tempbuf;
    /* copy the first 2 rows to the tempbuf */
    (void) memcpy(temploc, loc, lp->ncols);
    (void) memcpy(temploc + lp->ncols, loc + lp->ncols + 2, lp->ncols);

    lastloc = lp->lastbuf;
    /* copy the last row to another buffer for wraparound */
    (void) memcpy(lastloc, loc + ((lp->nrows - 1) * (lp->ncols + 2)),
             lp->ncols);

    for (row = 0; row < lp->nrows; ++row) {
	for (col = 0; col < lp->ncols; ++col) {
	    fate = fates[*temploc];
	    *temploc = (row == (lp->nrows - 3)) ?
		*(lastloc + col) :
		*(loc + (lp->ncols + 2) * 2);
	    switch (fate) {
	    case BIRTH:
		if (!(*(loc + 1) & RT)) {
		    spawn(loc);
		}
		/* NO BREAK */
	    case SAME:
		if (*(loc + 1) & RT) {
		    drawcell(win, row, col);
		}
		break;
	    case DEATH:
		if (*(loc + 1) & RT) {
		    ckill(loc);
		    erasecell(win, row, col);
		}
		break;
	    }
	    loc++;
	    temploc++;
	}
	loc += 2;
	if (temploc >= lp->tempbuf + lp->ncols * 2)
	    temploc = lp->tempbuf;
    }

    if (++lp->generation > batchcount)
	initlife(win);

    /*
     * generate a randomized shooter aimed roughly toward the center of the
     * screen after timeout.
     */

    if (seconds() - lp->shooterTime > TIMEOUT)
        shooter(win);
}
