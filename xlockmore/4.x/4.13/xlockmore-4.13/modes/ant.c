/* -*- Mode: C; tab-width: 4 -*- */
/*-
 * ant --- Chris Langton's generalized turing machine ants (also known
 *         as Greg Turk's turmites) whose tape is the screen
 */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)ant.c	4.11 98/06/18 xlockmore";

#endif

/*-
 * Copyright (c) 1995 by David Bagley.
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
 * 10-May-97: Compatible with xscreensaver
 * 16-Apr-97: -neighbors 3 and 8 added
 * 01-Jan-97: Updated ant.c to handle more kinds of ants.  Thanks to
 *            J Austin David <Austin.David@tlogic.com>.  Check it out in
 *            java at http://havoc.gtf.gatech.edu/austin  He thought up the
 *            new Ladder ant.
 * 04-Apr-96: -neighbors 6 runtime-time option added for hexagonal ants
 *            (bees), coded from an idea of Jim Propp's in Science News,
 *            Oct 28, 1995 VOL. 148 page 287
 * 20-Sep-95: Memory leak in ant fixed.  Now random colors.
 * 05-Sep-95: Coded from A.K. Dewdney's "Computer Recreations", Scientific
 *            American Magazine" Sep 1989 pp 180-183, Mar 1990 p 121
 *            Also used Ian Stewart's Mathematical Recreations, Scientific
 *            American Jul 1994 pp 104-107
 *            also used demon.c and life.c as a guide.
 */

/*-
  Species Grid     Number of Neigbors
  ------- ----     ------------------
  Ants    Square   4 (or 8)
  Bees    Hexagon  6
  Bees    Triangle 3 (or 9, 12)

  Neighbors 6 and neighbors 3 produce the same Turk ants.
*/

#ifdef STANDALONE
#define PROGCLASS "Ant"
#define HACK_INIT init_ant
#define HACK_DRAW draw_ant
#define ant_opts xlockmore_opts
#define DEFAULTS "*delay: 1000 \n" \
 "*count: -3 \n" \
 "*cycles: 40000 \n" \
 "*size: -12 \n" \
 "*ncolors: 64 \n" \
 "*neighbors: 0 \n" \
 "*sharpturn: False \n" \
 "*fullrandom: True \n" \
 "*verbose: False \n"
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */
#include "automata.h"

#ifdef MODE_ant

/*-
 * neighbors of 0 randomizes it between 3, 4 and 6.
 * 8, 9 12 are available also but not recommended.
 */
extern int  neighbors;

#define DEF_TRUCHET  "False"
#define DEF_SHARPTURN  "False"

static Bool truchet;
static Bool sharpturn;

static XrmOptionDescRec opts[] =
{
	{"-truchet", ".ant.truchet", XrmoptionNoArg, (caddr_t) "on"},
	{"+truchet", ".ant.truchet", XrmoptionNoArg, (caddr_t) "off"},
	{"-sharpturn", ".ant.sharpturn", XrmoptionNoArg, (caddr_t) "on"},
	{"+sharpturn", ".ant.sharpturn", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(caddr_t *) & truchet, "truchet", "Truchet", DEF_TRUCHET, t_Bool},
   {(caddr_t *) & sharpturn, "sharpturn", "SharpTurn", DEF_SHARPTURN, t_Bool}
};
static OptionStruct desc[] =
{
#if 0
      {"-neighbors num", "squares 4 or 8, hexagons 6, triangles 3, 9 or 12"},
#endif
	{"-/+truchet", "turn on/off Truchet lines"},
	{"-/+sharpturn", "turn on/off sharp turns (6 or 12 neighbors only)"}
};

ModeSpecOpt ant_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
ModStruct   ant_description =
{"ant", "init_ant", "draw_ant", "release_ant",
 "refresh_ant", "init_ant", NULL, &ant_opts,
 1000, -3, 40000, -12, 64, 1.0, "",
 "Shows Langton's and Turk's generalized ants", 0, NULL};

#endif

#define ANTBITS(n,w,h)\
  ap->pixmaps[ap->init_bits++]=\
  XCreatePixmapFromBitmapData(display,window,(char *)n,w,h,1,0,1)

/* If you change the table you may have to change the following 2 constants */
#define STATES 2
#define MINANTS 1
#define REDRAWSTEP 2000		/* How much tape to draw per cycle */
#define MINGRIDSIZE 24
#define MINSIZE 1
#define MINRANDOMSIZE 5
#define ANGLES 360

typedef struct {
	unsigned char color;
	short       direction;
	unsigned char next;
} statestruct;

typedef struct {
	int         col, row;
	short       direction;
	unsigned char state;
} antstruct;

typedef struct {
	Bool        painted;
	int         neighbors;
	int         generation;
	int         xs, ys;
	int         xb, yb;
	int         nrows, ncols;
	int         width, height;
	unsigned char ncolors, nstates;
	int         n;
	int         redrawing, redrawpos;
	int         truchet;	/* Only for Turk modes */
	int         sharpturn;	/* Only for even neighbors > 4 (i.e. 6 and 12) */
	statestruct machine[NUMSTIPPLES * STATES];
	unsigned char *tape;
	unsigned char *truchet_state;
	antstruct  *ants;
	int         init_bits;
	unsigned char colors[NUMSTIPPLES - 1];
	GC          stippledGC;
	Pixmap      pixmaps[NUMSTIPPLES - 1];
	union {
		XPoint      hexagon[7];		/* Need more than 6 for truchet */
		XPoint      triangle[2][4];	/* Need more than 3 for truchet */
	} shape;
} antfarmstruct;

static char plots[] =
{3,
#if 1				/* Without this... this mode is misnamed... */
 4,
#endif
 6};				/* Neighborhoods, 8 just makes a mess */

#define NEIGHBORKINDS (long) (sizeof plots / sizeof *plots)

/* Relative ant moves */
#define FS 0			/* Step */
#define TRS 1			/* Turn right, then step */
#define THRS 2			/* Turn hard right, then step */
#define TBS 3			/* Turn back, then step */
#define THLS 4			/* Turn hard left, then step */
#define TLS 5			/* Turn left, then step */
#define SF 6			/* Step */
#define STR 7			/* Step then turn right */
#define STHR 8			/* Step then turn hard right */
#define STB 9			/* Step then turn back */
#define STHL 10			/* Step then turn hard left */
#define STL 11			/* Step then turn left */

static antfarmstruct *antfarms = NULL;

/* LANGTON'S ANT (10) Chaotic after 500, Builder after 10,000 (104p) */
/* TURK'S 100 ANT Always chaotic?, tested past 150,000,000 */
/* TURK'S 101 ANT Always chaotic? */
/* TURK'S 110 ANT Builder at 150 (18p) */
/* TURK'S 1000 ANT Always chaotic? */
/* TURK'S 1100 SYMMETRIC ANT  all even run 1's and 0's are symmetric */
/* other examples 1001, 110011, 110000, 1001101 */
/* TURK'S 1101 ANT Builder after 250,000 (388p) */
/* Once saw a chess horse type builder (i.e. non-45 degree builder) */

/* BEE ONLY */
/* All alternating 10 appear symmetric, no proof (i.e. 10, 1010, etc) */
/* Even runs of 0's and 1's are also symmetric */
/* I have seen Hexagonal builders but they are more rare. */

static unsigned char tables[][3 * NUMSTIPPLES * STATES + 2] =
{
#if 0
  /* Here just so you can figure out notation */
	{			/* Langton's ant */
		2, 1,
		1, TLS, 0, 0, TRS, 0
	},
#else
  /* First 2 numbers are the size (ncolors, nstates) */
	{			/* LADDER BUILDER */
		4, 1,
		1, STR, 0, 2, STL, 0, 3, TRS, 0, 0, TLS, 0
	},
	{			/* SPIRALING PATTERN */
		2, 2,
		1, TLS, 0, 0, FS, 1,
		1, TRS, 0, 1, TRS, 0
	},
	{			/* SQUARE (HEXAGON) BUILDER */
		2, 2,
		1, TLS, 0, 0, FS, 1,
		0, TRS, 0, 1, TRS, 0
	},
#endif
};

#define NTABLES   (sizeof tables / sizeof tables[0])

static void
position_of_neighbor(antfarmstruct * ap, int dir, int *pcol, int *prow)
{
	int         col = *pcol, row = *prow;

	if (ap->neighbors == 6) {
		switch (dir) {
			case 0:
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
				break;
			case 60:
				if (!(row & 1))
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
				row = (!row) ? ap->nrows - 1 : row - 1;
				break;
			case 120:
				if (row & 1)
					col = (!col) ? ap->ncols - 1 : col - 1;
				row = (!row) ? ap->nrows - 1 : row - 1;
				break;
			case 180:
				col = (!col) ? ap->ncols - 1 : col - 1;
				break;
			case 240:
				if (row & 1)
					col = (!col) ? ap->ncols - 1 : col - 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 300:
				if (!(row & 1))
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n", dir);
		}
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		switch (dir) {
			case 0:
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
				break;
			case 45:
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
				row = (!row) ? ap->nrows - 1 : row - 1;
				break;
			case 90:
				row = (!row) ? ap->nrows - 1 : row - 1;
				break;
			case 135:
				col = (!col) ? ap->ncols - 1 : col - 1;
				row = (!row) ? ap->nrows - 1 : row - 1;
				break;
			case 180:
				col = (!col) ? ap->ncols - 1 : col - 1;
				break;
			case 225:
				col = (!col) ? ap->ncols - 1 : col - 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 270:
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 315:
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n", dir);
		}
	} else {		/* TRI */
		if ((col + row) % 2) {	/* right */
			switch (dir) {
				case 0:
					col = (!col) ? ap->ncols - 1 : col - 1;
					break;
				case 30:
				case 40:
					col = (!col) ? ap->ncols - 1 : col - 1;
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				case 60:
					col = (!col) ? ap->ncols - 1 : col - 1;
					if (!row)
						row = ap->nrows - 2;
					else if (!(row - 1))
						row = ap->nrows - 1;
					else
						row = row - 2;
					break;
				case 80:
				case 90:
					if (!row)
						row = ap->nrows - 2;
					else if (!(row - 1))
						row = ap->nrows - 1;
					else
						row = row - 2;
					break;
				case 120:
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				case 150:
				case 160:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				case 180:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					break;
				case 200:
				case 210:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				case 240:
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				case 270:
				case 280:
					if (row + 1 == ap->nrows)
						row = 1;
					else if (row + 2 == ap->nrows)
						row = 0;
					else
						row = row + 2;
					break;
				case 300:
					col = (!col) ? ap->ncols - 1 : col - 1;
					if (row + 1 == ap->nrows)
						row = 1;
					else if (row + 2 == ap->nrows)
						row = 0;
					else
						row = row + 2;
					break;
				case 320:
				case 330:
					col = (!col) ? ap->ncols - 1 : col - 1;
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				default:
					(void) fprintf(stderr, "wrong direction %d\n", dir);
			}
		} else {	/* left */
			switch (dir) {
				case 0:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					break;
				case 30:
				case 40:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				case 60:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					if (row + 1 == ap->nrows)
						row = 1;
					else if (row + 2 == ap->nrows)
						row = 0;
					else
						row = row + 2;
					break;
				case 80:
				case 90:
					if (row + 1 == ap->nrows)
						row = 1;
					else if (row + 2 == ap->nrows)
						row = 0;
					else
						row = row + 2;
					break;
				case 120:
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				case 150:
				case 160:
					col = (!col) ? ap->ncols - 1 : col - 1;
					row = (row + 1 == ap->nrows) ? 0 : row + 1;
					break;
				case 180:
					col = (!col) ? ap->ncols - 1 : col - 1;
					break;
				case 200:
				case 210:
					col = (!col) ? ap->ncols - 1 : col - 1;
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				case 240:
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				case 270:
				case 280:
					if (!row)
						row = ap->nrows - 2;
					else if (row == 1)
						row = ap->nrows - 1;
					else
						row = row - 2;
					break;
				case 300:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					if (!row)
						row = ap->nrows - 2;
					else if (row == 1)
						row = ap->nrows - 1;
					else
						row = row - 2;
					break;
				case 320:
				case 330:
					col = (col + 1 == ap->ncols) ? 0 : col + 1;
					row = (!row) ? ap->nrows - 1 : row - 1;
					break;
				default:
					(void) fprintf(stderr, "wrong direction %d\n", dir);
			}
		}
	}
	*pcol = col;
	*prow = row;
}

static void
fillcell(ModeInfo * mi, GC gc, int col, int row)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (ap->neighbors == 6) {
		int         ccol = 2 * col + !(row & 1), crow = 2 * row;

		ap->shape.hexagon[0].x = ap->xb + ccol * ap->xs;
		ap->shape.hexagon[0].y = ap->yb + crow * ap->ys;
		if (ap->xs == 1 && ap->ys == 1)
			XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), gc,
			ap->shape.hexagon[0].x, ap->shape.hexagon[0].y, 1, 1);
		else
			XFillPolygon(MI_DISPLAY(mi), MI_WINDOW(mi), gc,
			    ap->shape.hexagon, 6, Convex, CoordModePrevious);
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), gc,
		ap->xb + ap->xs * col, ap->yb + ap->ys * row,
	 	ap->xs - (ap->xs > 3), ap->ys - (ap->ys > 3));
	} else {		/* TRI */
		int         orient = (col + row) % 2;	/* O left 1 right */

		ap->shape.triangle[orient][0].x = ap->xb + col * ap->xs;
		ap->shape.triangle[orient][0].y = ap->yb + row * ap->ys;
		if (ap->xs <= 3 || ap->ys <= 3)
			XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), gc,
			((orient) ? -1 : 1) + ap->shape.triangle[orient][0].x,
				       ap->shape.triangle[orient][0].y, 1, 1);
		else {
			if (orient)
				ap->shape.triangle[orient][0].x += (ap->xs / 2 - 1);
			else
				ap->shape.triangle[orient][0].x -= (ap->xs / 2 - 1);
			XFillPolygon(MI_DISPLAY(mi), MI_WINDOW(mi), gc,
				     ap->shape.triangle[orient], 3, Convex, CoordModePrevious);
		}
	}
}

static void
truchetcell(ModeInfo * mi, int col, int row, int truchetstate)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (ap->neighbors == 6) {

		int         ccol = 2 * col + !(row & 1), crow = 2 * row;
		int         side;
		int         fudge = 7;	/* fudge because the hexagons are not exact */
		XPoint      hex, hex2;

		if (ap->sharpturn) {
			hex.x = ap->xb + ccol * ap->xs - (int) ((double) ap->xs / 2.0) - 1;
			hex.y = ap->yb + crow * ap->ys - (int) ((double) ap->ys / 2.0) - 1;
			for (side = 0; side < 6; side++) {
				if (side > 0) {
					hex.x += ap->shape.hexagon[side].x;
					hex.y += ap->shape.hexagon[side].y;
				}
				if (truchetstate == side % 2)
					XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						 hex.x, hex.y, ap->xs, ap->ys,
						 ((570 - (side * 60) + fudge) % 360) * 64, (120 - 2 * fudge) * 64);
			}
		} else {
			/* Very crude approx of Sqrt 3, so it will not cause drawing errors. */
			hex.x = ap->xb + ccol * ap->xs - (int) ((double) ap->xs * 1.6 / 2.0);
			hex.y = ap->yb + crow * ap->ys - (int) ((double) ap->ys * 1.6 / 2.0);
			for (side = 0; side < 6; side++) {
				if (side > 0) {
					hex.x += ap->shape.hexagon[side].x;
					hex.y += ap->shape.hexagon[side].y;
				}
				hex2.x = hex.x + ap->shape.hexagon[side + 1].x / 2;
				hex2.y = hex.y + ap->shape.hexagon[side + 1].y / 2;
				if (truchetstate == side % 3)
					/* Crude approx of 120 deg, so it will not cause drawing errors. */
					XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						 hex2.x, hex2.y,
						 (int) ((double) ap->xs * 1.5), (int) ((double) ap->ys * 1.5),
						 ((555 - (side * 60)) % 360) * 64, 90 * 64);
			}
		}
	} else if (ap->neighbors == 4) {
		if (truchetstate) {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - ap->xs / 2 + 1,
				 ap->yb + ap->ys * row + ap->ys / 2 - 1,
				 ap->xs - 2, ap->ys - 2,
				 0 * 64, 90 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + ap->xs / 2 - 1,
				 ap->yb + ap->ys * row - ap->ys / 2 + 1,
				 ap->xs - 2, ap->ys - 2,
				 -90 * 64, -90 * 64);
		} else {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - ap->xs / 2 + 1,
				 ap->yb + ap->ys * row - ap->ys / 2 + 1,
				 ap->xs - 2, ap->ys - 2,
				 0 * 64, -90 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + ap->xs / 2 - 1,
				 ap->yb + ap->ys * row + ap->ys / 2 - 1,
				 ap->xs - 2, ap->ys - 2,
				 90 * 64, 90 * 64);
		}
	} else if (ap->neighbors == 3) {
		int         orient = (col + row) % 2;	/* O left 1 right */
		int         side, ang;
		int         fudge = 7;	/* fudge because the triangles are not exact */
		XPoint      tri;

		tri.x = ap->xb + col * ap->xs;
		tri.y = ap->yb + row * ap->ys;
		if (orient) {
			tri.x += (ap->xs / 2 - 1);
		} else {
			tri.x -= (ap->xs / 2 - 1);
		}
		for (side = 0; side < 3; side++) {
			if (side > 0) {
				tri.x += ap->shape.triangle[orient][side].x;
				tri.y += ap->shape.triangle[orient][side].y;
			}
			if (truchetstate == side % 3) {
				if (orient)
					ang = (510 - side * 120) % 360;		/* Right */
				else
					ang = (690 - side * 120) % 360;		/* Left */
				XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				  tri.x - ap->xs / 2, tri.y - 3 * ap->ys / 4,
					 ap->xs, 3 * ap->ys / 2,
				  (ang + fudge) * 64, (60 - 2 * fudge) * 64);
			}
		}
	}
}

static void
drawcell(ModeInfo * mi, int col, int row, unsigned char color)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	GC          gc;

	if (!color) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_BLACK_PIXEL(mi));
		gc = MI_GC(mi);
	} else if (MI_NPIXELS(mi) > 2) {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			       MI_PIXEL(mi, ap->colors[color - 1]));
		gc = MI_GC(mi);
	} else {
		XGCValues   gcv;

		gcv.stipple = ap->pixmaps[color - 1];
		gcv.foreground = MI_WHITE_PIXEL(mi);
		gcv.background = MI_BLACK_PIXEL(mi);
		XChangeGC(MI_DISPLAY(mi), ap->stippledGC,
			  GCStipple | GCForeground | GCBackground, &gcv);
		gc = ap->stippledGC;
	}
	fillcell(mi, gc, col, row);
}

static void
drawtruchet(ModeInfo * mi, int col, int row,
	    unsigned char color, unsigned char truchetstate)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (!color)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
	else if (MI_NPIXELS(mi) > 2 || color > ap->ncolors / 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_BLACK_PIXEL(mi));
	else
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
	truchetcell(mi, col, row, truchetstate);
}

static void
draw_anant(ModeInfo * mi, int col, int row)
{
	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
	fillcell(mi, MI_GC(mi), col, row);
#if 0				/* Can not see eyes */
	{
		antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
		Display    *display = MI_DISPLAY(mi);
		Window      window = MI_WINDOW(mi);

		if (ap->xs > 2 && ap->ys > 2) {		/* Draw Eyes */

			XSetForeground(display, MI_GC(mi), MI_BLACK_PIXEL(mi));
			switch (direction) {
				case 0:
					XDrawPoint(display, window, MI_GC(mi),
					    ap->xb + ap->xs - 1, ap->yb + 1);
					XDrawPoint(display, window, MI_GC(mi),
						   ap->xb + ap->xs - 1, ap->yb + ap->ys - 2);
					break;
				case 180:
					XDrawPoint(display, window, MI_GC(mi), ap->xb, ap->yb + 1);
					XDrawPoint(display, window, MI_GC(mi), ap->xb, ap->yb + ap->ys - 2);
					break;
					if (neighbors == 4) {
				case 90:
						XDrawPoint(display, window, MI_GC(mi), ap->xb + 1, ap->yb);
						XDrawPoint(display, window, MI_GC(mi),
						ap->xb + ap->xs - 2, ap->yb);
						break;
				case 270:
						XDrawPoint(display, window, MI_GC(mi),
							   ap->xb + 1, ap->yb + ap->ys - 1);
						XDrawPoint(display, window, MI_GC(mi),
							   ap->xb + ap->xs - 2, ap->yb + ap->ys - 1);
						break;
					}	/* else BEE */
				default:
			}
		}
	}
#endif
}

#if 0
static void
RandomSoup(mi)
	ModeInfo   *mi;
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         row, col, mrow = 0;

	for (row = 0; row < ap->nrows; ++row) {
		for (col = 0; col < ap->ncols; ++col) {
			ap->old[col + mrow] = (unsigned char) NRAND((int) ap->ncolors);
			drawcell(mi, col, row, ap->old[col + mrow]);
		}
		mrow += ap->nrows;
	}
}

#endif

static short
fromTableDirection(unsigned char dir, int local_neighbors)
{
	switch (dir) {
		case FS:
			return 0;
		case TRS:
			return (ANGLES / local_neighbors);
		case THRS:
			return (ANGLES / 2 - ANGLES / local_neighbors);
		case TBS:
			return (ANGLES / 2);
		case THLS:
			return (ANGLES / 2 + ANGLES / local_neighbors);
		case TLS:
			return (ANGLES - ANGLES / local_neighbors);
		case SF:
			return ANGLES;
		case STR:
			return (ANGLES + ANGLES / local_neighbors);
		case STHR:
			return (3 * ANGLES / 2 - ANGLES / local_neighbors);
		case STB:
			return (3 * ANGLES / 2);
		case STHL:
			return (3 * ANGLES / 2 + ANGLES / local_neighbors);
		case STL:
			return (2 * ANGLES - ANGLES / local_neighbors);
		default:
			(void) fprintf(stderr, "wrong direction %d\n", dir);
	}
	return -1;
}

static void
getTable(ModeInfo * mi, int i)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         j, total;
	unsigned char *patptr;

	patptr = &tables[i][0];
	ap->ncolors = *patptr++;
	ap->nstates = *patptr++;
	total = ap->ncolors * ap->nstates;
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
			       "ants %d, neighbors %d, table number %d, colors %d, states %d\n",
			  ap->n, ap->neighbors, i, ap->ncolors, ap->nstates);
	for (j = 0; j < total; j++) {
		ap->machine[j].color = *patptr++;
		if (ap->sharpturn && ap->neighbors > 4 && !(ap->neighbors % 2)) {
			int         k = *patptr++;

			switch (k) {
				case TRS:
					k = THRS;
					break;
				case THRS:
					k = TRS;
					break;
				case THLS:
					k = TLS;
					break;
				case TLS:
					k = THLS;
					break;
				case STR:
					k = STHR;
					break;
				case STHR:
					k = STR;
					break;
				case STHL:
					k = STL;
					break;
				case STL:
					k = STHL;
					break;
				default:
					break;
			}
			ap->machine[j].direction = fromTableDirection(k, ap->neighbors);
		} else {
			ap->machine[j].direction = fromTableDirection(*patptr++, ap->neighbors);
		}
		ap->machine[j].next = *patptr++;
	}
	ap->truchet = False;
}

static void
getTurk(ModeInfo * mi, int i)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         power2, j, number, total;

	/* To force a number, say <i = 2;> has  i + 2 (or 4) binary digits */
	power2 = 1 << (i + 1);
	/* Do not want numbers which in binary are all 1's. */
	number = NRAND(power2 - 1) + power2;
	/* To force a particular number, say <number = 10;> */

	ap->ncolors = i + 2;
	ap->nstates = 1;
	total = ap->ncolors * ap->nstates;
	for (j = 0; j < total; j++) {
		ap->machine[j].color = (j + 1) % total;
		if (ap->sharpturn && ap->neighbors > 4 && !(ap->neighbors % 2)) {
			ap->machine[j].direction = (power2 & number) ?
				fromTableDirection(THRS, ap->neighbors) :
				fromTableDirection(THLS, ap->neighbors);
		} else {
			ap->machine[j].direction = (power2 & number) ?
				fromTableDirection(TRS, ap->neighbors) :
				fromTableDirection(TLS, ap->neighbors);
		}
		ap->machine[j].next = 0;
		power2 >>= 1;
	}
	ap->truchet = (ap->truchet && ap->xs > 2 && ap->ys > 2 &&
	   (ap->neighbors == 3 || ap->neighbors == 4 || ap->neighbors == 6));
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
		      "ants %d, neighbors %d, Turk's number %d, colors %d\n",
			       ap->n, ap->neighbors, number, ap->ncolors);
}

void
init_ant(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         size = MI_SIZE(mi);
	antfarmstruct *ap;
	int         col, row, dir;
	long        i;

	if (antfarms == NULL) {
		if ((antfarms = (antfarmstruct *) calloc(MI_NUM_SCREENS(mi),
					    sizeof (antfarmstruct))) == NULL)
			return;
	}
	ap = &antfarms[MI_SCREEN(mi)];
	ap->redrawing = 0;
	if (MI_NPIXELS(mi) <= 2) {
		if (ap->stippledGC == None) {
			XGCValues   gcv;

			gcv.fill_style = FillOpaqueStippled;
			ap->stippledGC = XCreateGC(display, window, GCFillStyle, &gcv);
		}
		if (ap->init_bits == 0) {
			for (i = 1; i < NUMSTIPPLES; i++)
				ANTBITS(stipples[i], STIPPLESIZE, STIPPLESIZE);
		}
	}
	ap->generation = 0;
	ap->n = MI_COUNT(mi);
	if (ap->n < -MINANTS) {
		/* if ap->n is random ... the size can change */
		if (ap->ants != NULL) {
			(void) free((void *) ap->ants);
			ap->ants = NULL;
		}
		ap->n = NRAND(-ap->n - MINANTS + 1) + MINANTS;
	} else if (ap->n < MINANTS)
		ap->n = MINANTS;

	ap->width = MI_WIDTH(mi);
	ap->height = MI_HEIGHT(mi);

	if (neighbors == 8 || neighbors == 9 || neighbors == 12)
		ap->neighbors = neighbors;	/* Discourage but not deny use... */
	else
		for (i = 0; i < NEIGHBORKINDS; i++) {
			if (neighbors == plots[i]) {
				ap->neighbors = plots[i];
				break;
			}
			if (i == NEIGHBORKINDS - 1) {
				ap->neighbors = plots[NRAND(NEIGHBORKINDS)];
				break;
			}
		}

	if (ap->neighbors == 6) {
		int         nccols, ncrows;

		if (ap->width < 2)
			ap->width = 2;
		if (ap->height < 4)
			ap->height = 4;
		if (size < -MINSIZE) {
			ap->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
			if (ap->ys < MINRANDOMSIZE)
				ap->ys = MIN(MINRANDOMSIZE,
					     MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE));
		} else if (size < MINSIZE) {
			if (!size)
				ap->ys = MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE);
			else
				ap->ys = MINSIZE;
		} else
			ap->ys = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
					       MINGRIDSIZE));
		ap->xs = ap->ys;
		nccols = MAX(ap->width / ap->xs - 2, 2);
		ncrows = MAX(ap->height / ap->ys - 1, 2);
		ap->ncols = nccols / 2;
		ap->nrows = 2 * (ncrows / 4);
		ap->xb = (ap->width - ap->xs * nccols) / 2 + ap->xs / 2;
		ap->yb = (ap->height - ap->ys * (ncrows / 2) * 2) / 2 + ap->ys;
		for (i = 0; i < 6; i++) {
			ap->shape.hexagon[i].x = (ap->xs - 1) * hexagonUnit[i].x;
			ap->shape.hexagon[i].y = ((ap->ys - 1) * hexagonUnit[i].y / 2) * 4 / 3;
		}
		/* Avoid array bounds read of hexagonUnit */
		ap->shape.hexagon[6].x = 0;
		ap->shape.hexagon[6].y = 0;
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		if (size < -MINSIZE) {
			ap->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
			if (ap->ys < MINRANDOMSIZE)
				ap->ys = MIN(MINRANDOMSIZE,
					     MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE));
		} else if (size < MINSIZE) {
			if (!size)
				ap->ys = MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE);
			else
				ap->ys = MINSIZE;
		} else
			ap->ys = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
					       MINGRIDSIZE));
		ap->xs = ap->ys;
		ap->ncols = MAX(ap->width / ap->xs, 2);
		ap->nrows = MAX(ap->height / ap->ys, 2);
		ap->xb = (ap->width - ap->xs * ap->ncols) / 2;
		ap->yb = (ap->height - ap->ys * ap->nrows) / 2;
	} else {		/* TRI */
		int         orient;

		if (ap->width < 2)
			ap->width = 2;
		if (ap->height < 2)
			ap->height = 2;
		if (size < -MINSIZE) {
			ap->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
			if (ap->ys < MINRANDOMSIZE)
				ap->ys = MIN(MINRANDOMSIZE,
					     MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE));
		} else if (size < MINSIZE) {
			if (!size)
				ap->ys = MAX(MINSIZE, MIN(ap->width, ap->height) / MINGRIDSIZE);
			else
				ap->ys = MINSIZE;
		} else
			ap->ys = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
					       MINGRIDSIZE));
		ap->xs = (int) (1.52 * ap->ys);
		ap->ncols = (MAX(ap->width / ap->xs - 1, 2) / 2) * 2;
		ap->nrows = (MAX(ap->height / ap->ys - 1, 2) / 2) * 2;
		ap->xb = (ap->width - ap->xs * ap->ncols) / 2 + ap->xs / 2;
		ap->yb = (ap->height - ap->ys * ap->nrows) / 2 + ap->ys;
		for (orient = 0; orient < 2; orient++) {
			for (i = 0; i < 3; i++) {
				ap->shape.triangle[orient][i].x =
					(ap->xs - 2) * triangleUnit[orient][i].x;
				ap->shape.triangle[orient][i].y =
					(ap->ys - 2) * triangleUnit[orient][i].y;
			}
			/* Avoid array bounds read of triangleUnit */
			ap->shape.triangle[orient][3].x = 0;
			ap->shape.triangle[orient][3].y = 0;
		}
	}

	XSetLineAttributes(display, MI_GC(mi), 1, LineSolid, CapNotLast, JoinMiter);
	MI_CLEARWINDOW(mi);
	ap->painted = False;

	if (MI_IS_FULLRANDOM(mi)) {
		ap->truchet = (Bool) (LRAND() & 1);
		ap->sharpturn = (Bool) (LRAND() & 1);
	} else {
		ap->truchet = truchet;
		ap->sharpturn = sharpturn;
	}
	/* Exclude odd # of neighbors, stepping forward not defined */
	if (!NRAND(NUMSTIPPLES) && ((ap->neighbors + 1) % 2)) {
		getTable(mi, (int) (NRAND(NTABLES)));
	} else
		getTurk(mi, (int) (NRAND(NUMSTIPPLES - 1)));
	if (MI_NPIXELS(mi) > 2)
		for (i = 0; i < (int) ap->ncolors - 1; i++)
			ap->colors[i] = (unsigned char) (NRAND(MI_NPIXELS(mi)) +
			     i * MI_NPIXELS(mi)) / ((int) (ap->ncolors - 1));
	if (ap->ants == NULL)
		ap->ants = (antstruct *) malloc(ap->n * sizeof (antstruct));
	if (ap->tape != NULL)
		(void) free((void *) ap->tape);
	ap->tape = (unsigned char *)
		calloc(ap->ncols * ap->nrows, sizeof (unsigned char));

	if (ap->truchet_state != NULL)
		(void) free((void *) ap->truchet_state);
	ap->truchet_state = (unsigned char *)
		calloc(ap->ncols * ap->nrows, sizeof (unsigned char));

	col = ap->ncols / 2;
	row = ap->nrows / 2;
	dir = NRAND(ap->neighbors) * ANGLES / ap->neighbors;
	/* Have them all start in the same spot, why not? */
	for (i = 0; i < ap->n; i++) {
		ap->ants[i].col = col;
		ap->ants[i].row = row;
		ap->ants[i].direction = dir;
		ap->ants[i].state = 0;
	}
	draw_anant(mi, col, row);
}

void
draw_ant(ModeInfo * mi)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	antstruct  *anant;
	statestruct *status;
	int         i, state_pos, tape_pos;
	unsigned char color;
	short       chg_dir, old_dir;

	MI_IS_DRAWN(mi) = True;

	ap->painted = True;
	for (i = 0; i < ap->n; i++) {
		anant = &ap->ants[i];
		tape_pos = anant->col + anant->row * ap->ncols;
		color = ap->tape[tape_pos];	/* read tape */
		state_pos = color + anant->state * ap->ncolors;
		status = &(ap->machine[state_pos]);
		drawcell(mi, anant->col, anant->row, status->color);
		ap->tape[tape_pos] = status->color;	/* write on tape */

		/* Find direction of Bees or Ants. */
		/* Translate relative direction to actual direction */
		old_dir = anant->direction;
		chg_dir = (2 * ANGLES - status->direction) % ANGLES;
		anant->direction = (chg_dir + old_dir) % ANGLES;
		if (ap->truchet) {
			int         a = 0, b;

			if (ap->neighbors == 6) {
				if (ap->sharpturn) {
					a = (chg_dir / 120 == 2);
					drawtruchet(mi, anant->col, anant->row, status->color, a);
				} else {
					a = (old_dir / 60) % 3;
					b = (anant->direction / 60) % 3;
					a = (a + b + 1) % 3;
					drawtruchet(mi, anant->col, anant->row, status->color, a);
				}
			} else if (ap->neighbors == 4) {
				a = old_dir / 180;
				b = anant->direction / 180;
				a = ((a && !b) || (b && !a));
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			} else if (ap->neighbors == 3) {
				if (chg_dir == 240)
					a = (2 + anant->direction / 120) % 3;
				else
					a = (1 + anant->direction / 120) % 3;
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			}
			ap->truchet_state[tape_pos] = a + 1;
		}
		anant->state = status->next;

		/* If edge than wrap it */
		old_dir = ((status->direction < ANGLES) ? anant->direction : old_dir);
		position_of_neighbor(ap, old_dir, &(anant->col), &(anant->row));
		draw_anant(mi, anant->col, anant->row);
	}
	if (++ap->generation > MI_CYCLES(mi)) {
		init_ant(mi);
	}
	if (ap->redrawing) {
		for (i = 0; i < REDRAWSTEP; i++) {
			if (ap->tape[ap->redrawpos] ||
			 (ap->truchet && ap->truchet_state[ap->redrawpos])) {
				drawcell(mi, ap->redrawpos % ap->ncols, ap->redrawpos / ap->ncols,
					 ap->tape[ap->redrawpos]);
				if (ap->truchet)
					drawtruchet(mi, ap->redrawpos % ap->ncols, ap->redrawpos / ap->ncols,
						    ap->tape[ap->redrawpos],
					ap->truchet_state[ap->redrawpos] - 1);
			}
			if (++(ap->redrawpos) >= ap->ncols * ap->nrows) {
				ap->redrawing = 0;
				break;
			}
		}
	}
}

void
release_ant(ModeInfo * mi)
{
	if (antfarms != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			antfarmstruct *ap = &antfarms[screen];
			int         shade;

			if (ap->stippledGC != None) {
				XFreeGC(MI_DISPLAY(mi), ap->stippledGC);
			}
			for (shade = 0; shade < ap->init_bits; shade++)
				XFreePixmap(MI_DISPLAY(mi), ap->pixmaps[shade]);
			if (ap->tape != NULL)
				(void) free((void *) ap->tape);
			if (ap->ants != NULL)
				(void) free((void *) ap->ants);
			if (ap->truchet_state != NULL)
				(void) free((void *) ap->truchet_state);
		}
		(void) free((void *) antfarms);
		antfarms = NULL;
	}
}

void
refresh_ant(ModeInfo * mi)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];

	if (ap->painted) {
		MI_CLEARWINDOW(mi);
		ap->redrawing = 1;
		ap->redrawpos = 0;
	}
}

#endif /* MODE_ant */
