/* -*- Mode: C; tab-width: 4 -*- */
/*-
 * ant --- Chris Langton's generalized turing machine ants (also known
 *         as Greg Turk's turmites) whose tape is the screen */

#if 0
static const char sccsid[] = "@(#)ant.c	5.24 2007/01/18 xlockmore";

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
 * 18-Jan-2007: Added vertical option.
 * 01-Nov-2000: Allocation checks
 * 10-May-1997: Compatible with xscreensaver
 * 16-Apr-1997: -neighbors 3 and 8 added
 * 01-Jan-1997: Updated ant.c to handle more kinds of ants.  Thanks to
 *              J Austin David <Austin.David@tlogic.com>.  Check it out in
 *              java at http://havoc.gtf.gatech.edu/austin  He thought up the
 *              new Ladder ant.
 * 04-Apr-1996: -neighbors 6 runtime-time option added for hexagonal ants
 *              (bees), coded from an idea of Jim Propp's in Science News,
 *              Oct 28, 1995 VOL. 148 page 287
 * 20-Sep-1995: Memory leak in ant fixed.  Now random colors.
 * 05-Sep-1995: Coded from A.K. Dewdney's "Computer Recreations", Scientific
 *              American Magazine" Sep 1989 pp 180-183, Mar 1990 p 121
 *              Also used Ian Stewart's Mathematical Recreations, Scientific
 *              American Jul 1994 pp 104-107
 *              also used demon.c and life.c as a guide.
 */

/*-
  Species Grid     Number of Neighbors
  ------- ----     ------------------
  Ants    Square   4 (or 8)
  Bees    Hexagon  6
  Bees    Triangle 3 (or 9, 12)
          Pentagon 5 (or 7)

  Neighbors 6 and neighbors 3 produce the same Turk ants.
*/

#ifdef STANDALONE
# define MODE_ant
# define DEFAULTS "*delay: 20000 \n" \
	"*count: -3 \n" \
	"*cycles: 40000 \n" \
	"*size: -12 \n" \
	"*ncolors: 64 \n" \
	"*fullrandom: True \n" \
	"*verbose: False \n" \
	"*fpsSolid: True \n" \

# define reshape_ant 0
# define ant_handle_event 0
# include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
# include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */
# define DO_STIPPLE
#include "automata.h"

#ifdef MODE_ant

#define DEF_LABEL "True"
#define FONT_HEIGHT 19
#define FONT_WIDTH 15

/*-
 * neighbors of 0 randomizes it for 3, 4, 5, 6, 8 (last 1 is less likely)
 */

#define DEF_NEIGHBORS  "0"      /* choose random value */
#define DEF_RULE "A" /* All rules */
#define DEF_TRUCHET  "False"
#define DEF_EYES  "False"
#define DEF_SHARPTURN  "False"
#define DEF_VERTICAL "False"

static Bool label;
static int  neighbors;
static char *rule;
static Bool truchet;
static Bool eyes;
static Bool sharpturn;
static Bool vertical;

static XrmOptionDescRec opts[] =
{
	{(char *) "-label", (char *) ".ant.label", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+label", (char *) ".ant.label", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-neighbors", (char *) ".ant.neighbors", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-rule", (char *) ".ant.rule", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-truchet", (char *) ".ant.truchet", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+truchet", (char *) ".ant.truchet", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-eyes", (char *) ".ant.eyes", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+eyes", (char *) ".ant.eyes", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-sharpturn", (char *) ".ant.sharpturn", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+sharpturn", (char *) ".ant.sharpturn", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-vertical", (char *) ".ant.vertical", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+vertical", (char *) ".ant.vertical", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(void *) & label, (char *) "label", (char *) "Label", (char *) DEF_LABEL, t_Bool},
	{(void *) & neighbors, (char *) "neighbors", (char *) "Neighbors", (char *) DEF_NEIGHBORS, t_Int},
	{(void *) & rule, (char *) "rule", (char *) "Rule", (char *) DEF_RULE, t_String},
	{(void *) & truchet, (char *) "truchet", (char *) "Truchet", (char *) DEF_TRUCHET, t_Bool},
	{(void *) & eyes, (char *) "eyes", (char *) "Eyes", (char *) DEF_EYES, t_Bool},
	{(void *) & sharpturn, (char *) "sharpturn", (char *) "SharpTurn", (char *) DEF_SHARPTURN, t_Bool},
	{(void *) & vertical, (char *) "vertical", (char *) "Vertical", (char *) DEF_VERTICAL, t_Bool}
};
static OptionStruct desc[] =
{
	{(char *) "-/+label", (char *) "turn on/off rule labeling"},
	{(char *) "-neighbors num", (char *) "squares 4 or 8, hexagons 6, triangles 3, pentagons 5"},
	{(char *) "-rule string", (char *) "binary string for Turk's Ant"},
	{(char *) "-/+truchet", (char *) "turn on/off Truchet lines"},
	{(char *) "-/+eyes", (char *) "turn on/off eyes"},
	{(char *) "-/+sharpturn", (char *) "turn on/off sharp turns (5, 6, 8 neighbors only)"},
	{(char *) "-/+vertical", (char *) "change orientation for hexagons and triangles"}
};

ENTRYPOINT  ModeSpecOpt ant_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
const ModStruct ant_description =
{"ant", "init_ant", "draw_ant", "release_ant",
 "refresh_ant", "init_ant", "free_ant", &ant_opts,
 1000, -3, 40000, -12, 64, 1.0, "",
 "Shows Langton's and Turk's generalized ants", 0, NULL};

#endif

#define ANTBITS(n,w,h)\
  if ((ap->pixmaps[ap->init_bits]=\
  XCreatePixmapFromBitmapData(display,window,(char *)n,w,h,1,0,1))==None){\
  free_ant(mi); return;} else {ap->init_bits++;}

/* If you change the table you may have to change the following 2 constants */
#define STATES 2
#define MINANTS 1
#define REDRAWSTEP 2000		/* How much tape to draw per cycle */
#define MINGRIDSIZE 24
#define MINSIZE 1
#define MINRANDOMSIZE 5
#define ANGLES(x) ((x==7) ? 357 : 360)  /* 360 not divisible by 7 */

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
	Bool        painted, vertical;
	int         neighbors, polygon;
	int         generation;
	int         xs, ys;
	int         xb, yb;
	int         init_dir;
	int         nrows, ncols;
	int         width, height;
	unsigned char ncolors, nstates;
	int         n;
	int         redrawing, redrawpos;
	Bool        truchet;	/* Only for Turk modes */
	Bool        eyes;
	Bool        useTable;
	Bool        sharpturn;
	statestruct machine[NUMSTIPPLES * STATES];
	unsigned char *tape;
	unsigned char *truchet_state;
	antstruct  *ants;
	int         init_bits;
	unsigned char colors[NUMSTIPPLES - 1];
#ifdef DO_STIPPLE
	GC          stippledGC;
#endif /* DO_STIPPLE */
	Pixmap      pixmaps[NUMSTIPPLES - 1];
	int         labelOffsetX, labelOffsetY;
	char        ruleString[50];
	union {		/* Need extra point for truchet */
		XPoint      triangle[2][4];
		XPoint      pentagon[4][6];
		XPoint      hexagon[7];
	} shape;
#ifdef STANDALONE
	eraser_state *eraser;
#endif
} antfarmstruct;

/* 7,9,12 are not likely to produce anything, judging by 8 */
static char plots[] =
{3, 4, 5, 6,
#ifdef NUMBER_7
7,
#endif
8
#ifdef NUMBER_9
,9
#endif
#ifdef NUMBER_12
,12
#endif
};

static int newDir5[] = {0, 2, 1, 4, 3};

#define NEIGHBORKINDS ((long) (sizeof plots / sizeof *plots))
#define GOODNEIGHBORKINDS 4

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

static antfarmstruct *antfarms = (antfarmstruct *) NULL;

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
	{			/* Langton's ant  (-rule 10) */
		2, 1,
		1, TLS, 0, 0, TRS, 0
	},
#endif
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
};

#define NTABLES   (sizeof tables / sizeof tables[0])

static int
heading(int n, int direction, int turn)
{
	int oldDirection = direction;
	int chgDir;

	if (n == 5) {
		chgDir = newDir5[(turn / 72) % n] * 72;
	} else {
		chgDir = (2 * ANGLES(n) - turn) % ANGLES(n);
	}
	if (neighbors == 5) {
		oldDirection = newDir5[oldDirection / 72] * 72;
	}
	return (chgDir + oldDirection) % ANGLES(n);
}

static void
positionOfNeighbor(antfarmstruct * ap, int dir, int *pcol, int *prow)
{
	int col = *pcol, row = *prow;

	if (ap->neighbors == 4 || ap->neighbors == 6 || ap->neighbors == 8) {
		switch (dir) {
		case 0:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			break;
		case 45:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case 60:
			if (!(row & 1))
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case 90:
			row = (!row) ? ap->nrows - 1 : row - 1;
			break;
		case 120:
			if (row & 1)
				col = (!col) ? ap->ncols - 1 : col - 1;
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
		case 240:
			if (row & 1)
				col = (!col) ? ap->ncols - 1 : col - 1;
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case 270:
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case 300:
			if (!(row & 1))
				col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		case 315:
			col = (col + 1 == ap->ncols) ? 0 : col + 1;
			row = (row + 1 == ap->nrows) ? 0 : row + 1;
			break;
		default:
			(void) fprintf(stderr, "wrong direction %d\n", dir);
		}
	} else if (ap->neighbors == 5 || ap->neighbors == 7) {
		int orient = ((row & 1) * 2 + col) % 4;
		switch (orient) { /* up, down, left, right */
		case 0:
			switch (dir) {
			case 0:
				col++;
				break;
			case 51: /* 7 */
			case 72: /* 5 */
				col = (col + 2 >= ap->ncols) ? 0 : col + 2;
				break;
			case 102: /* 7 corner */
				col = (col + 3 >= ap->ncols) ? 1 : col + 3;
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 144: /* 5 */
			case 153: /* 7 */
				col++;
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 204: /* 7 */
			case 216: /* 5 */
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 255: /* 7 */
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 288: /* 5 */
			case 306: /* 7 */
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n",
					dir);
			}
			break;
		case 1:
			switch (dir) {
			case 0:
				col--;
				break;
			case 51: /* 7 */
			case 72: /* 5 */
				col = (col == 1) ? ap->ncols - 1 : col - 2;
				break;
			case 102: /* 7 */
				col = (col == 1) ? ap->ncols - 2 : col - 3;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 144: /* 5 */
			case 153: /* 7 */
				col--;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 204: /* 7 */
			case 216: /* 5 */
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 255: /* 7 */
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 288: /* 5 */
			case 306: /* 7 */
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n",
					dir);
			}
			break;
		case 2:
			switch (dir) {
			case 0:
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				break;
			case 51: /* 7 */
			case 72: /* 5 */
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				col++;
				break;
			case 102: /* 7 */
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 144: /* 5 */
			case 153: /* 7 */
				col = (col == 0) ? ap->ncols - 2 : col - 2;
				break;
			case 204: /* 7 */
			case 216: /* 5 */
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				break;
			case 255: /* 7 */
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				break;
			case 288: /* 5 */
			case 306: /* 7 */
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n",
					dir);
			}
			break;
		case 3:
			switch (dir) {
			case 0:
				col--;
				break;
			case 51: /* 7 */
			case 72: /* 5 */
				col = (col == 0) ? ap->ncols - 1 : col - 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 102: /* 7 */
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				row = (row + 1 == ap->nrows) ? 0 : row + 1;
				break;
			case 144: /* 5 */
			case 153: /* 7 */
				col = (col + 2 >= ap->ncols) ? 1 : col + 2;
				break;
			case 204: /* 7 */
			case 216: /* 5 */
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				break;
			case 255: /* 7 */
				col = (col + 1 >= ap->ncols) ? 0 : col + 1;
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			case 288: /* 5 */
			case 306: /* 7 */
				row = (row == 0) ? ap->nrows - 1 : row - 1;
				break;
			default:
				(void) fprintf(stderr, "wrong direction %d\n",
					dir);
			}
			break;
		default:
			(void) fprintf(stderr, "wrong orient %d\n",
				orient);
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
	Display *display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);

	if (ap->neighbors == 6) {
		int ccol = 2 * col + !(row & 1), crow = 2 * row;

		if (ap->vertical) {
			ap->shape.hexagon[0].x = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].y = ap->yb + crow * ap->ys;
		} else {
			ap->shape.hexagon[0].y = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].x = ap->yb + crow * ap->ys;
		}
		if (ap->xs == 1 && ap->ys == 1)
			XDrawPoint(display, window, gc,
				ap->shape.hexagon[0].x, ap->shape.hexagon[0].y);
		else
			XFillPolygon(display, window, gc,
				ap->shape.hexagon, 6, Convex, CoordModePrevious);
	} else if (ap->neighbors == 5 || ap->neighbors == 7) {
		int map[4] = {2, 0, 1, 3};
		int orient = ((row & 1) * 2 + col) % 4;
		int offsetX = 0, offsetY = 0;
		switch (orient) {
		case 0: /* up */
			offsetX = ap->xs;
			break;
		case 1: /* down */
			offsetY = 3 * ap->ys / 2 - 1;
			break;
		case 2: /* left */
			offsetX = -ap->xs / 2;
			offsetY = 3 * ap->ys / 4;
			break;
		case 3: /* right */
			offsetX = 3 * ap->xs / 2 - 1;
			offsetY = 3 * ap->ys / 4;
			break;
		default:
			(void) printf("wrong orient %d\n", orient);
		}
		orient = map[orient];
		ap->shape.pentagon[orient][0].x = ap->xb +
			col * ap->xs + offsetX;
		ap->shape.pentagon[orient][0].y = ap->yb +
			row * ap->ys + offsetY;
		if (ap->xs <= 2 || ap->ys <= 2)
			XDrawPoint(display, window, gc,
				ap->shape.pentagon[orient][0].x, ap->shape.pentagon[orient][0].y);
		else
			XFillPolygon(display, window, gc,
				ap->shape.pentagon[orient], 5, Convex, CoordModePrevious);
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		XFillRectangle(display, window, gc,
		ap->xb + ap->xs * col, ap->yb + ap->ys * row,
	 	ap->xs - (ap->xs > 3), ap->ys - (ap->ys > 3));
	} else {		/* TRI */
		int orient = (col + row) % 2;	/* O left 1 right */
		Bool small = (ap->xs <= 3 || ap->ys <= 3);

		if (ap->vertical) {
			ap->shape.triangle[orient][0].x = ap->xb + col * ap->xs;
			ap->shape.triangle[orient][0].y = ap->yb + row * ap->ys;
			if (small)
				ap->shape.triangle[orient][0].x +=
					((orient) ? -1 : 1);
			else
				ap->shape.triangle[orient][0].x +=
					(ap->xs / 2  - 1) * ((orient) ? 1 : -1);
		} else {
			ap->shape.triangle[orient][0].y = ap->xb + col * ap->xs;
			ap->shape.triangle[orient][0].x = ap->yb + row * ap->ys;
			if (small)
				ap->shape.triangle[orient][0].y +=
					((orient) ? -1 : 1);
			else
				ap->shape.triangle[orient][0].y +=
					(ap->xs / 2  - 1) * ((orient) ? 1 : -1);
		}
		if (small) {
			XDrawPoint(display, window, gc,
				ap->shape.triangle[orient][0].x,
				ap->shape.triangle[orient][0].y);
		} else {
			XFillPolygon(display, window, gc,
				ap->shape.triangle[orient], 3,
				Convex, CoordModePrevious);
			XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
			XDrawLines(display, window, gc,
				ap->shape.triangle[orient], 4,
				CoordModePrevious);
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
		int         sides = 6;
#ifdef ARC
		XPoint      hex, hex2;

		if (ap->sharpturn) {
			int         fudge = 7;	/* fudge because the hexagons are not exact */
			hex.x = ap->xb + ccol * ap->xs - (int) ((double) ap->xs / 2.0) - 1;
			hex.y = ap->yb + crow * ap->ys - (int) ((double) ap->ys / 2.0) - 1;
			for (side = 0; side < sides; side++) {
				if (side) {
					if (ap->vertical) {
						hex.x += ap->shape.hexagon[side].x;
						hex.y += ap->shape.hexagon[side].y;
					} else {
						hex.y += ap->shape.hexagon[side].x;
						hex.x += ap->shape.hexagon[side].y;
					}
				}
				if (truchetstate == side % 2) {
					if (ap->vertical) {
					    XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						hex.x, hex.y, ap->xs, ap->ys,
						((570 - (side * 60) + fudge) % 360) * 64,
						(120 - 2 * fudge) * 64);
					} else {
					    XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						hex.y, hex.x, ap->ys, ap->xs,
						((300 + (side * 60) + fudge) % 360) * 64,
						(120 - 2 * fudge) * 64);
					}
				}
			}
		} else {
			/* Very crude approx of Sqrt 3, so it will not cause drawing errors. */
			hex.x = ap->xb + ccol * ap->xs - (int) ((double) ap->xs * 1.6 / 2.0) - 1;
			hex.y = ap->yb + crow * ap->ys - (int) ((double) ap->ys * 1.6 / 2.0) - 1;
			for (side = 0; side < sides; side++) {
				if (side) {
					if (ap->vertical) {
						hex.x += ap->shape.hexagon[side].x;
						hex.y += ap->shape.hexagon[side].y;
					} else {
						hex.y += ap->shape.hexagon[side].x;
						hex.x += ap->shape.hexagon[side].y;
					}
				}
				if (ap->vertical) {
					hex2.x = hex.x + ap->shape.hexagon[side + 1].x / 2;
					hex2.y = hex.y + ap->shape.hexagon[side + 1].y / 2 + 1;
				} else {
					hex2.y = hex.y + ap->shape.hexagon[side + 1].x / 2;
					hex2.x = hex.x + ap->shape.hexagon[side + 1].y / 2 + 1;
				}
				/* Lots of fudging here */
				if (side == 0) {
					hex2.x += 1;
				} else if (side == 1) {
					hex2.x += (short) (ap->xs * 0.1 + 1);
					hex2.y += (short) (ap->ys * 0.1 - ((ap->ys > 5) ? 1 : 0));
				} else if (side == 2) {
					hex2.x += (short) (ap->xs * 0.1 + 1);
					hex2.y += 1;
				} else if (side == 3) {
					hex2.y += 1;
				} else if (side == 4) {
					hex2.x += (short) (ap->xs * 0.1);
					hex2.y += (short) (ap->ys * 0.1 - 1);
				} else if (side == 5) {
					hex2.x += (short) (ap->xs * 0.5);
					hex2.y += (short) (-ap->ys * 0.3);
				}
				if (truchetstate == side % 3) {
					/* Crude approx of 120 deg, so it will not cause drawing errors. */
					if (ap->vertical) {
					    XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						 hex2.x, hex2.y,
						 (int) ((double) ap->xs * 1.5), (int) ((double) ap->ys * 1.5),
						 ((555 - (side * 60)) % 360) * 64, 90 * 64);
					} else {
					    XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
						 hex2.y, hex2.x,
						 (int) ((double) ap->ys * 1.5), (int) ((double) ap->xs * 1.5),
						 ((345 + (side * 60)) % 360) * 64, 90 * 64);
					}
				}
			}
		}
#else
		XPoint polylist[6];
		XPoint poly[6];
		XPoint poly2[6];
		XPoint center;
		int skip = (ap->sharpturn) ? 1 : 2;

		if (ap->vertical) {
			ap->shape.hexagon[0].x = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].y = ap->yb + crow * ap->ys;
		} else {
			ap->shape.hexagon[0].y = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].x = ap->yb + crow * ap->ys;
		}
		for (side = 0; side < sides; side++) {
                        polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.hexagon[side].x;
                        polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.hexagon[side].y;
		}
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			poly[side].x = (polylist[side].x
				+ polylist[(side + sides - 1) % sides].x) / 2;
			poly[side].y = (polylist[side].y
				+ polylist[(side + sides - 1) % sides].y) / 2;
			center.x += poly[side].x;
			center.y += poly[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (side = 0; side < sides; side++) {
			poly2[side].x = (poly[side].x + center.x) / 2;
			poly2[side].y = (poly[side].y + center.y) / 2;
			/* fudge to not draw so close to edge */
			poly[side].x = (poly[side].x * 7 + center.x) / 8;
			poly[side].y = (poly[side].y * 7 + center.y) / 8;
		}
		for (side = 0; side < sides; side++) {
			if (truchetstate == side % (1 + skip)) {
				int side2 = (side + skip) % sides;
				XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
					poly[side].x, poly[side].y,
					poly2[side].x, poly2[side].y);
				XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
					poly2[side].x, poly2[side].y,
					poly2[side2].x, poly2[side2].y);
				XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
					poly2[side2].x, poly2[side2].y,
					poly[side2].x, poly[side2].y);
			}
		}
#endif
	} else if (ap->neighbors == 5) {
		int side;
		int sides = 5;
		XPoint polylist[5];
		XPoint poly[5];
		XPoint poly2[5];
		XPoint center;
		int skip = (ap->sharpturn) ? 1 : 2;
		int map[4] = {2, 0, 1, 3};
		int orient = ((row & 1) * 2 + col) % 4;
		int offsetX = 0, offsetY = 0;
		int side2;
		switch (orient) {
		case 0: /* up */
			offsetX = ap->xs;
			break;
		case 1: /* down */
			offsetY = 3 * ap->ys / 2 - 1;
			break;
		case 2: /* left */
			offsetX = -ap->xs / 2;
			offsetY = 3 * ap->ys / 4;
			break;
		case 3: /* right */
			offsetX = 3 * ap->xs / 2 - 1;
			offsetY = 3 * ap->ys / 4;
			break;
		default:
			(void) printf("wrong orient %d\n", orient);
		}
		orient = map[orient];

		/* no ARC, no vertical */
		ap->shape.pentagon[orient][0].x = ap->xb +
			col * ap->xs + offsetX;
		ap->shape.pentagon[orient][0].y = ap->yb +
			row * ap->ys + offsetY;
		for (side = 0; side < sides; side++) {
                        polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.pentagon[orient][side].x;
                        polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.pentagon[orient][side].y;
		}
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			poly[side].x = (polylist[side].x
				+ polylist[(side + sides - 1) % sides].x) / 2;
			poly[side].y = (polylist[side].y
				+ polylist[(side + sides - 1) % sides].y) / 2;
			center.x += poly[side].x;
			center.y += poly[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (side = 0; side < sides; side++) {
			poly2[side].x = (poly[side].x + skip * center.x) / (skip + 1);
			poly2[side].y = (poly[side].y + skip * center.y) / (skip + 1);
		}
		for (side = 0; side < sides; side++) {
			poly[side].x = (poly[side].x * 4 + center.x) / 5;
			poly[side].y = (poly[side].y * 4 + center.y) / 5;
		}
		side = truchetstate;
		side2 = (side + skip) % sides;
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly[side].x, poly[side].y,
			poly2[side].x, poly2[side].y);
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly2[side].x, poly2[side].y,
			poly2[side2].x, poly2[side2].y);
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly2[side2].x, poly2[side2].y,
			poly[side2].x, poly[side2].y);
	} else if (ap->neighbors == 4) {
		/* ARC only */
		if (truchetstate) {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - (ap->xs - 1) / 2,
				 ap->yb + ap->ys * row + (ap->ys - 1) / 2,
				 ap->xs - 2, ap->ys - 2,
				 0 * 64, 90 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + (ap->xs - 1) / 2,
				 ap->yb + ap->ys * row - (ap->ys - 1) / 2,
				 ap->xs - 2, ap->ys - 2,
				 -90 * 64, -90 * 64);
		} else {
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col - (ap->xs - 1) / 2,
				 ap->yb + ap->ys * row - (ap->ys - 1) / 2,
				 ap->xs - 2, ap->ys - 2,
				 0 * 64, -90 * 64);
			XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				 ap->xb + ap->xs * col + (ap->xs - 1) / 2,
				 ap->yb + ap->ys * row + (ap->ys - 1) / 2,
				 ap->xs - 2, ap->ys - 2,
				 90 * 64, 90 * 64);
		}
	} else if (ap->neighbors == 3) {
		int orient = (col + row) % 2;	/* O left 1 right */
		int side;
		int sides = 3;
#ifdef ARC
		int ang;
		int fudge = 7;	/* fudge because the triangles are not exact */
		double fudge2 = 1.18;
		XPoint tri;

		tri.x = ap->xb + col * ap->xs;
		tri.y = ap->yb + row * ap->ys;
		if (orient) {
			tri.x += (ap->xs / 2 - 1);
		} else {
			tri.x -= (ap->xs / 2 - 1);
		}
		for (side = 0; side < sides; side++) {
			if (side > 0) {
				if (ap->vertical) {
					tri.x += ap->shape.triangle[orient][side].x;
					tri.y += ap->shape.triangle[orient][side].y;
				} else {
					tri.y += ap->shape.triangle[orient][side].x;
					tri.x += ap->shape.triangle[orient][side].y;
				}
			}
			if (truchetstate == side) {
				if (ap->vertical) {
					if (orient)
						ang = (510 - side * 120) % 360;		/* Right */
					else
						ang = (690 - side * 120) % 360;		/* Left */
				} else {
					if (orient)
						ang = (60 + side * 120) % 360;		/* Right */
					else
						ang = (240 + side * 120) % 360;		/* Left */
				}
				if (ap->vertical) {
					XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				    (int) (tri.x - ap->xs * fudge2 / 2),
				    (int) (tri.y - 3 * ap->ys * fudge2 / 4),
				    (unsigned int) (ap->xs * fudge2),
				    (unsigned int) (3 * ap->ys * fudge2 / 2),
				    (ang + fudge) * 64, (60 - 2 * fudge) * 64);
				} else {
					XDrawArc(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				    (int) (tri.y - 3 * ap->ys * fudge2 / 4),
				    (int) (tri.x - ap->xs * fudge2 / 2),
				    (unsigned int) (3 * ap->ys * fudge2 / 2),
				    (unsigned int) (ap->xs * fudge2),
				    (ang + fudge) * 64, (60 - 2 * fudge) * 64);
				}
				break;
			}
		}
#else
		XPoint polylist[3];
		XPoint poly[3];
		XPoint poly2[3];
		XPoint center;
		int offsetX = 0;
		int side2;

		if (orient) {
			offsetX += (ap->xs / 2 - 1);
		} else {
			offsetX -= (ap->xs / 2 - 1);
		}
		if (ap->vertical) {
			ap->shape.triangle[orient][0].x = ap->xb +
				col * ap->xs + offsetX;
			ap->shape.triangle[orient][0].y = ap->yb +
				row * ap->ys;
		} else {
			ap->shape.triangle[orient][0].y = ap->xb +
				col * ap->xs + offsetX;
			ap->shape.triangle[orient][0].x = ap->yb +
				row * ap->ys;
		}
		for (side = 0; side < sides; side++) {
                        polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.triangle[orient][side].x;
                        polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.triangle[orient][side].y;
		}
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			poly[side].x = (polylist[side].x
				+ polylist[(side + sides - 1) % sides].x) / 2;
			poly[side].y = (polylist[side].y
				+ polylist[(side + sides - 1) % sides].y) / 2;
			center.x += poly[side].x;
			center.y += poly[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (side = 0; side < sides; side++) {
			poly2[side].x = (poly[side].x + center.x) / 2;
			poly2[side].y = (poly[side].y + center.y) / 2;
		}
		for (side = 0; side < sides; side++) {
			poly[side].x = (poly[side].x * 11 + center.x) / 12;
			poly[side].y = (poly[side].y * 11 + center.y) / 12;
		}
		side = truchetstate;
		side2 = (side + 1) % sides;
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly[side].x, poly[side].y,
			poly2[side].x, poly2[side].y);
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly2[side].x, poly2[side].y,
			poly2[side2].x, poly2[side2].y);
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			poly2[side2].x, poly2[side2].y,
			poly[side2].x, poly[side2].y);
#endif
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
#ifdef DO_STIPPLE
	} else if (MI_NPIXELS(mi) <= 2) {
		XGCValues   gcv;

		gcv.stipple = ap->pixmaps[color - 1];
		gcv.foreground = MI_WHITE_PIXEL(mi);
		gcv.background = MI_BLACK_PIXEL(mi);
		XChangeGC(MI_DISPLAY(mi), ap->stippledGC,
			  GCStipple | GCForeground | GCBackground, &gcv);
		gc = ap->stippledGC;
#endif /* !DO_STIPPLE */
	} else {
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			MI_PIXEL(mi, ap->colors[color - 1]));
		gc = MI_GC(mi);
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
draw_paireyes(ModeInfo * mi, int direction, int col, int row)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int eyes = 2;

	XSetForeground(display, MI_GC(mi), MI_BLACK_PIXEL(mi));
	if (ap->neighbors == 6) {
		XPoint polylist[6];
		XPoint pupil;
		XPoint center;
		int ccol = 2 * col + !(row & 1), crow = 2 * row;
		int side, eye;
		int sides = 6;
		int ang = direction * sides / 360;

		if (ap->vertical) {
			ap->shape.hexagon[0].x = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].y = ap->yb + crow * ap->ys;
		} else {
			ap->shape.hexagon[0].y = ap->xb + ccol * ap->xs;
			ap->shape.hexagon[0].x = ap->yb + crow * ap->ys;
		}
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.hexagon[side].x;
			polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.hexagon[side].y;
			center.x += polylist[side].x;
			center.y += polylist[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (eye = 0; eye < eyes; eye++) {
			side = (sides + eyes - eye - ang) % sides;
			pupil.x = (polylist[side].x + center.x) / 2;
			pupil.y = (polylist[side].y + center.y) / 2;
			XDrawPoint(display, window, MI_GC(mi),
				pupil.x, pupil.y);
		}
	} else if (ap->neighbors == 5) {
		XPoint polylist[5];
		XPoint pupil;
		XPoint center;
		int side, eye;
		int sides = 5;
		int ang = direction * sides / 360;
		int map[4] = {2, 0, 1, 3};
		int orient = ((row & 1) * 2 + col) % 4;
		int offsetX = 0, offsetY = 0;
		switch (orient) {
		case 0: /* up */
			offsetX = ap->xs;
			break;
		case 1: /* down */
			offsetY = 3 * ap->ys / 2 - 1;
			break;
		case 2: /* left */
			offsetX = -ap->xs / 2;
			offsetY = 3 * ap->ys / 4;
			break;
		case 3: /* right */
			offsetX = 3 * ap->xs / 2 - 1;
			offsetY = 3 * ap->ys / 4;
			break;
		default:
			(void) printf("wrong orient %d\n", orient);
		}
		orient = map[orient];
		ap->shape.pentagon[orient][0].x = ap->xb +
			col * ap->xs + offsetX;
		ap->shape.pentagon[orient][0].y = ap->yb +
			row * ap->ys + offsetY;
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.pentagon[orient][side].x;
			polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.pentagon[orient][side].y;
			center.x += polylist[side].x;
			center.y += polylist[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (eye = 0; eye < eyes; eye++) {
			side = (sides - ang + 2 * eye + 3) % sides;
			pupil.x = (polylist[side].x + center.x) / 2;
			pupil.y = (polylist[side].y + center.y) / 2;
			XDrawPoint(display, window, MI_GC(mi),
				pupil.x, pupil.y);
		}
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		switch (direction) {
			case 0:
				XDrawPoint(display, window, MI_GC(mi),
					ap->xb + ap->xs * (col + 1) - 3,
				 	ap->yb + ap->ys * row + ap->ys / 2 - 2);
				XDrawPoint(display, window, MI_GC(mi),
					ap->xb + ap->xs * (col + 1) - 3,
				 	ap->yb + ap->ys * row + ap->ys / 2);
				break;
			case 45:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * (col + 1) - 4,
					ap->yb + ap->ys * row + 1);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * (col + 1) - 3,
					ap->yb + ap->ys * row + 2);
				break;
			case 90:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + ap->xs / 2 - 2,
					ap->yb + ap->ys * row + 1);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + ap->xs / 2,
					ap->yb + ap->ys * row + 1);
				break;
			case 135:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + 2,
					ap->yb + ap->ys * row + 1);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + 1,
					ap->yb + ap->ys * row + 2);
				break;
			case 180:
				XDrawPoint(display, window, MI_GC(mi),
					ap->xb + ap->xs * col + 1,
				 	ap->yb + ap->ys * row + ap->ys / 2 - 2);
				XDrawPoint(display, window, MI_GC(mi),
					ap->xb + ap->xs * col + 1,
				 	ap->yb + ap->ys * row + ap->ys / 2);
				break;
			case 225:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + 2,
					ap->yb + ap->ys * (row + 1) - 3);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + 1,
					ap->yb + ap->ys * (row + 1) - 4);
				break;
			case 270:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + ap->xs / 2 - 2,
					ap->yb + ap->ys * (row + 1) - 3);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * col + ap->xs / 2,
					ap->yb + ap->ys * (row + 1) - 3);
				break;
			case 315:
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * (col + 1) - 4,
					ap->yb + ap->ys * (row + 1) - 3);
				XDrawPoint(display, window, MI_GC(mi),
				 	ap->xb + ap->xs * (col + 1) - 3,
					ap->yb + ap->ys * (row + 1) - 4);
				break;
			default:
				(void) fprintf(stderr, "wrong eyes direction %d for ant eyes\n", direction);
		}
	} else {		/* TRI */
		XPoint polylist[3];
		XPoint pupil;
		XPoint center;
		int side, eye;
		int sides = 3;
		int ang = direction * sides / 360;
		int orient = (col + row) % 2;	/* O left 1 right */
		int offsetX = 0;

		if (!(ap->xs > 6 && ap->ys > 6))
			return;
		if (orient)
			offsetX = ap->xs / 2 - 1;
		else
			offsetX = -ap->xs / 2 + 1;
#if 0
		XPoint tri;
		if (ap->vertical) {
			tri.x = ap->xb + col * ap->xs;
			tri.y = ap->yb + row * ap->ys;
			if (orient)
				tri.x += (ap->xs / 6 - 1);
			else
				tri.x -= (ap->xs / 6 - 1);
		} else {
			tri.y = ap->xb + col * ap->xs;
			tri.x = ap->yb + row * ap->ys;
			if (orient)
				tri.y += (ap->xs / 6 - 1);
			else
				tri.y -= (ap->xs / 6 - 1);
		}
		ang = direction * ap->neighbors / ANGLES(ap->neighbors);
		/* approx... does not work that well for even numbers */
		if (
#ifdef NUMBER_9
		ap->neighbors == 9 ||
#endif
		ap->neighbors == 12) {
#ifdef UNDER_CONSTRUCTION
			/* Not sure why this does not work */
			ang = ((ang + ap->neighbors / 6) / (ap->neighbors / 3)) % 3;
#else
			return;
#endif
		}
		for (side = 0; side < 3; side++) {
			if (side) {
				if (ap->vertical) {
					tri.x += ap->shape.triangle[orient][side].x / 3;
					tri.y += ap->shape.triangle[orient][side].y / 3;
				} else {
					tri.y += ap->shape.triangle[orient][side].x / 3;
					tri.x += ap->shape.triangle[orient][side].y / 3;
				}
			}
			/* Either you have the eyes in back or one eye in front */
#if 0
			if (side == ang)
				XDrawPoint(display, window, MI_GC(mi), tri.x, tri.y);
#else
			if (side == (ang + 2) % 3)
				XDrawPoint(display, window, MI_GC(mi), tri.x, tri.y);
			if (side == (ang + 1) % 3)
				XDrawPoint(display, window, MI_GC(mi), tri.x, tri.y);
#endif
		}
#endif
		if (ap->vertical) {
			ap->shape.triangle[orient][0].x = ap->xb +
				col * ap->xs + offsetX;
			ap->shape.triangle[orient][0].y = ap->yb +
				row * ap->ys;
		} else {
			ap->shape.triangle[orient][0].y = ap->xb +
				col * ap->xs + offsetX;
			ap->shape.triangle[orient][0].x = ap->yb +
				row * ap->ys;
		}
		center.x = 0;
		center.y = 0;
		for (side = 0; side < sides; side++) {
			polylist[side].x = ((side) ? polylist[side - 1].x : 0) +
				ap->shape.triangle[orient][side].x;
			polylist[side].y = ((side) ? polylist[side - 1].y : 0) +
				ap->shape.triangle[orient][side].y;
			center.x += polylist[side].x;
			center.y += polylist[side].y;
		}
		center.x /= sides;
		center.y /= sides;
		for (eye = 0; eye < eyes; eye++) {
			side = (sides + eyes - eye + ang) % sides;
			pupil.x = (polylist[side].x + center.x) / 2;
			pupil.y = (polylist[side].y + center.y) / 2;
			XDrawPoint(display, window, MI_GC(mi),
				pupil.x, pupil.y);
		}
	}
}

static void
draw_anant(ModeInfo * mi, int direction, int col, int row)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);

	XSetForeground(display, MI_GC(mi), MI_WHITE_PIXEL(mi));
	fillcell(mi, MI_GC(mi), col, row);
	if (ap->eyes && ap->xs > 3 && ap->ys > 3 &&
			!(ap->useTable && (ap->neighbors == 5 ||
			(ap->neighbors == 6 && ap->sharpturn))) &&
			ap->neighbors != 7 && ap->neighbors <= 8) {
		draw_paireyes(mi, direction, col, row);
	}
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
	int angles = ANGLES(local_neighbors);
	int deflect = 2;

	if (local_neighbors == 8) {
		deflect = 3;
	}
	/* Crafted to work for odd number of neighbors */
	switch (dir) {
		case FS:
			return 0;
		case TLS:
			return (angles / local_neighbors);
		case THLS:
			return (deflect * angles / local_neighbors);
		case TBS:
			return ((local_neighbors / 2) * angles / local_neighbors);
		case THRS:
			return (angles - deflect * angles / local_neighbors);
		case TRS:
			return (angles - angles / local_neighbors);
		case SF:
			return angles;
		case STL:
			return (angles + angles / local_neighbors);
		case STHL:
			return (angles + deflect * angles / local_neighbors);
		case STB:
			return (angles + (local_neighbors / 2) * angles / local_neighbors);
		case STHR:
			return (2 * angles - deflect * angles / local_neighbors);
		case STR:
			return (2 * angles - angles / local_neighbors);
		default:
			(void) fprintf(stderr, "wrong direction %d from table\n", dir);
	}
	return -1;
}

static void
getTable(ModeInfo * mi, int i)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int         j, total;
	unsigned char *patptr;
	statestruct *status;

	patptr = &tables[i][0];
	ap->ncolors = *patptr++;
	ap->nstates = *patptr++;
	total = ap->ncolors * ap->nstates;
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
			"ants %d, neighbors %d, table number %d, colors %d, states %d\n",
			ap->n, ap->neighbors, i, ap->ncolors, ap->nstates);
	if (label)
		(void) sprintf(ap->ruleString, "%d table number %d", ap->n, i);
	for (j = 0; j < total; j++) {
		status = &(ap->machine[j]);
		status->color = *patptr++;
		if (ap->sharpturn && ap->neighbors > 4) {
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
			status->direction = fromTableDirection(k, ap->neighbors);
		} else {
			status->direction = fromTableDirection(*patptr++, ap->neighbors);
		}
		status->next = *patptr++;
	}
	ap->truchet = False;
	ap->useTable = True;
}

static void
getTurk(ModeInfo * mi, int logClass, int number)
{
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	int power2, j, total;
	char rule[NUMSTIPPLES * STATES + 1];

	power2 = 1 << (logClass - 1);
	ap->ncolors = logClass;
	ap->nstates = 1;
	total = ap->ncolors * ap->nstates;
	for (j = 0; j < total; j++) {
		ap->machine[j].color = (j + 1) % total;
		if (ap->sharpturn && ap->neighbors > 4) {
			ap->machine[j].direction = (power2 & number) ?
				fromTableDirection(THRS, ap->neighbors) :
				fromTableDirection(THLS, ap->neighbors);
		} else {
			ap->machine[j].direction = (power2 & number) ?
				fromTableDirection(TRS, ap->neighbors) :
				fromTableDirection(TLS, ap->neighbors);
		}
		rule[j] = ((power2 & number) ? 1 : 0) + '0';
		ap->machine[j].next = 0;
		power2 >>= 1;
	}
	rule[total] = '\0';
	ap->truchet = (ap->truchet && ap->xs > 2 && ap->ys > 2 &&
		ap->neighbors >= 3 && ap->neighbors <= 6);
	ap->useTable = False;
	if (MI_IS_VERBOSE(mi))
		(void) fprintf(stdout,
			"ants %d, neighbors %d, Turk's number 0x%x (%s), colors %d\n",
			ap->n, ap->neighbors, number, rule, ap->ncolors);
	if (label)
		(void) sprintf(ap->ruleString, "%d Turk's number 0x%x",
			ap->n, number);
}

static void
parseRule(ModeInfo * mi)
{
	if (rule) {
		int n = 0, l, number = 0, logClass = 0, power2;
		Bool aZero = False;

		while (rule[n]) {
			if (rule[n] == 'T' || rule[n] == 't') {
				n++;
				while (rule[n]) {
					number = number * 10 +
						(int) (rule[n] - '0');
					n++;
				}
				getTable(mi, number % NTABLES);
				return;
			}
			if (rule[n] == 'A' || rule[n] == 'a') {
				if (!NRAND(NUMSTIPPLES)) {
					getTable(mi, (int) (NRAND(NTABLES)));
					return;
				} else {
					logClass = (int) (NRAND(NUMSTIPPLES - 2)) + 2;
					/* <logClass = 4;> has 4 binary digits */
					power2 = 1 << (logClass - 1);
					/* Do not want numbers which in binary are all 1's. */
					number = NRAND(power2 - 1) + power2;
					getTurk(mi, logClass, number);
					return;
				}
			} else {
				l = rule[n] - '0';
				if (l >= 0 && l <= 1) {
					number = (number << 1) + l;
					if (logClass > 0 || l != 0)
						logClass++;
					if (logClass > 0 && l == 0)
						aZero = True;
				}
			}
			n++;
		}
		if (!aZero) {
			getTurk(mi, 2, 2); /* Default to Langton's ant */
		} else {
			getTurk(mi, logClass, number);
		}
	}
}

static void
free_ant_screen(Display *display, antfarmstruct *ap)
{
	int         shade;

	if (ap == NULL) {
		return;
	}
#ifdef DO_STIPPLE
	if (ap->stippledGC != None) {
		XFreeGC(display, ap->stippledGC);
		ap->stippledGC = None;
	}
#endif /* DO_STIPPLE */
	for (shade = 0; shade < ap->init_bits; shade++) {
		XFreePixmap(display, ap->pixmaps[shade]);
	}
	ap->init_bits = 0;
	if (ap->tape != NULL) {
		free(ap->tape);
		ap->tape = (unsigned char *) NULL;
	}
	if (ap->ants != NULL) {
		free(ap->ants);
		ap->ants = (antstruct *) NULL;
	}
	if (ap->truchet_state != NULL) {
		free(ap->truchet_state);
		ap->truchet_state = (unsigned char *) NULL;
	}
	ap = NULL;
}

ENTRYPOINT void
free_ant(ModeInfo * mi)
{
	Display *display = MI_DISPLAY(mi);
	antfarmstruct *ap = &antfarms[MI_SCREEN(mi)];
	free_ant_screen(display, ap);
}

ENTRYPOINT void
init_ant(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
#ifdef DO_STIPPLE
	Window      window = MI_WINDOW(mi);
#endif
	int         size = MI_SIZE(mi);
	antfarmstruct *ap;
	int         col, row, dir;
	int         i;

	MI_INIT(mi, antfarms);
	/*if (antfarms == NULL) {
		if ((antfarms = (antfarmstruct *) calloc(MI_NUM_SCREENS(mi),
					    sizeof (antfarmstruct))) == NULL)
			return;
	}*/
	ap = &antfarms[MI_SCREEN(mi)];

	ap->redrawing = 0;
#ifdef DO_STIPPLE
	if (MI_NPIXELS(mi) <= 2) {
		if (ap->stippledGC == None) {
			XGCValues   gcv;

			gcv.fill_style = FillOpaqueStippled;
			if ((ap->stippledGC = XCreateGC(display, window,
					GCFillStyle, &gcv)) == None) {
				free_ant(mi);
				return;
			}
		}
		if (ap->init_bits == 0) {
			for (i = 1; i < NUMSTIPPLES; i++) {
				ANTBITS(stipples[i], STIPPLESIZE, STIPPLESIZE);
			}
		}
	}
#endif /* DO_STIPPLE */
	ap->generation = 0;
	ap->n = MI_COUNT(mi);
	if (ap->n < -MINANTS) {
		/* if ap->n is random ... the size can change */
		if (ap->ants != NULL) {
			free(ap->ants);
			ap->ants = (antstruct *) NULL;
		}
		/*ap->n = NRAND(-ap->n - MINANTS + 1) + MINANTS;*/
		/* skew so that 1 is more likely */
		ap->n = -ap->n - (int) (sqrt(NRAND(ap->n *ap->n - 1))) - 1 +
			MINANTS;
	} else if (ap->n < MINANTS)
		ap->n = MINANTS;

	if (MI_IS_FULLRANDOM(mi)) {
		ap->truchet = (Bool) (LRAND() & 1);
		ap->eyes = (Bool) (LRAND() & 1);
		ap->sharpturn = (Bool) (LRAND() & 1);
		ap->vertical = (Bool) (LRAND() & 1);
	} else {
		ap->truchet = truchet;
		ap->eyes = eyes;
		ap->sharpturn = sharpturn;
		ap->vertical = vertical;
	}
	ap->width = MI_WIDTH(mi);
	ap->height = MI_HEIGHT(mi);

	for (i = 0; i < NEIGHBORKINDS; i++) {
		if (neighbors == plots[i]) {
			ap->neighbors = plots[i];
			break;
		}
		if (i == NEIGHBORKINDS - 1) {
			if (!NRAND(10)) {
				/* Make above 6 rare */
				ap->neighbors = plots[NRAND(NEIGHBORKINDS)];
			} else {
				ap->neighbors = plots[NRAND(GOODNEIGHBORKINDS)];
			}
			break;
		}
	}
	if (ap->neighbors == 6) {
		int         nccols, ncrows;

		ap->polygon = 6;
		if (!ap->vertical) {
			ap->height = MI_WIDTH(mi);
			ap->width = MI_HEIGHT(mi);
		}
		if (ap->width < 8)
			ap->width = 8;
		if (ap->height < 8)
			ap->height = 8;
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
		ncrows = MAX(ap->height / ap->ys - 1, 4);
		ap->ncols = nccols / 2;
		ap->nrows = 2 * (ncrows / 4);
		ap->xb = (ap->width - ap->xs * nccols) / 2 + ap->xs / 2;
		ap->yb = (ap->height - ap->ys * (ncrows / 2) * 2) / 2 + ap->ys - 2;
		for (i = 0; i < 6; i++) {
			if (ap->vertical) {
				ap->shape.hexagon[i].x =
					(ap->xs - 1) * hexagonUnit[i].x;
				ap->shape.hexagon[i].y =
					((ap->ys - 1) * hexagonUnit[i].y /
					2) * 4 / 3;
			} else {
				ap->shape.hexagon[i].y =
					(ap->xs - 1) * hexagonUnit[i].x;
				ap->shape.hexagon[i].x =
					((ap->ys - 1) * hexagonUnit[i].y /
					2) * 4 / 3;
			}
		}
		/* Avoid array bounds read of hexagonUnit */
		ap->shape.hexagon[6].x = 0;
		ap->shape.hexagon[6].y = 0;
	} else if (ap->neighbors == 5 || ap->neighbors == 7) {
		int orient, sides;
		ap->polygon = 5;
		if (ap->width < 2)
			ap->width = 2;
		if (ap->height < 2)
			ap->height = 2;
		if (size < -MINSIZE)
			ap->xs = NRAND(MIN(-size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE) {
			if (!size) {
				int min = MIN(ap->width, ap->height) / (4 * MINGRIDSIZE);
				int max = MIN(ap->width, ap->height) / (MINGRIDSIZE);

				ap->xs = MAX(MINSIZE, min + NRAND(max - min + 1));
			} else
				ap->xs = MINSIZE;
		} else
			ap->xs = MIN(size, MAX(MINSIZE, MIN(ap->width, ap->height) /
				MINGRIDSIZE));
		ap->ys = ap->xs * 2;
		ap->ncols = (MAX((ap->width - 4) / ap->xs, 8) / 4) * 4;
		ap->nrows = (MAX((ap->height - ap->ys / 2) / ap->ys, 8)) / 2 * 2;
		ap->xb = (ap->width - ap->xs * ap->ncols) / 2;
		ap->yb = (ap->height - ap->ys * ap->nrows) / 2 - 2;
		for (orient = 0; orient < 4; orient++) {
			for (sides = 0; sides < 5; sides++) {
				ap->shape.pentagon[orient][sides].x =
					2 * (ap->xs - 1) * pentagonUnit[orient][sides].x / 4;
				ap->shape.pentagon[orient][sides].y =
					(ap->ys - 1) * pentagonUnit[orient][sides].y / 4;
			}
			ap->shape.pentagon[orient][5].x = 0;
			ap->shape.pentagon[orient][5].y = 0;
		}
	} else if (ap->neighbors == 4 || ap->neighbors == 8) {
		ap->polygon = 4;
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
		int orient;

		ap->polygon = 3;
		if (!ap->vertical) {
			ap->height = MI_WIDTH(mi);
			ap->width = MI_HEIGHT(mi);
		}
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
				if (ap->vertical) {
					ap->shape.triangle[orient][i].x =
						(ap->xs - 2) * triangleUnit[orient][i].x;
					ap->shape.triangle[orient][i].y =
						(ap->ys - 2) * triangleUnit[orient][i].y;
				} else {
					ap->shape.triangle[orient][i].y =
						(ap->xs - 2) * triangleUnit[orient][i].x;
					ap->shape.triangle[orient][i].x =
						(ap->ys - 2) * triangleUnit[orient][i].y;
				}
			}
			/* Avoid array bounds read of triangleUnit */
			ap->shape.triangle[orient][3].x = 0;
			ap->shape.triangle[orient][3].y = 0;
		}
	}

	XSetLineAttributes(display, MI_GC(mi), 1, LineSolid, CapNotLast, JoinMiter);
	MI_CLEARWINDOW(mi);
	ap->painted = False;

	ap->labelOffsetX = NRAND(8);
	ap->labelOffsetY = NRAND(8);
	parseRule(mi);
	if (MI_NPIXELS(mi) > 2) {
		int offset = NRAND(MI_NPIXELS(mi));

		for (i = 0; i < (int) ap->ncolors - 1; i++) {
			ap->colors[i] = (unsigned char) ((offset +
				(i * MI_NPIXELS(mi) /
				(int) (ap->ncolors - 1))) % MI_NPIXELS(mi));
		}
	}
	if (ap->ants == NULL) {
		if ((ap->ants = (antstruct *) malloc(ap->n * sizeof (antstruct))) ==
				NULL) {
			free_ant(mi);
			return;
		}
	}
	if (ap->tape != NULL)
		free(ap->tape);
	if ((ap->tape = (unsigned char *) calloc(ap->ncols * ap->nrows,
			sizeof (unsigned char))) == NULL) {
		free_ant(mi);
		return;
	}
	if (ap->truchet_state != NULL)
		free(ap->truchet_state);
	if ((ap->truchet_state = (unsigned char *) calloc(ap->ncols * ap->nrows,
			sizeof (unsigned char))) == NULL) {
		free_ant(mi);
		return;
	}

	row = ap->nrows / 2;
	col = ap->ncols / 2;
	if (col > 0 && ((ap->neighbors % 2) || ap->neighbors == 12) && (LRAND() & 1))
		col--;
	if (ap->polygon == 5) {
		int mod = (col + 2 * (row - 1)) % 4;
		dir = 0;
		col -= mod + NRAND(4);
	} else {
		dir = NRAND(ap->polygon) * ANGLES(ap->neighbors) / ap->neighbors;
	}
	ap->init_dir = dir;
	/* Have them all start in the same spot, why not? */
	for (i = 0; i < ap->n; i++) {
		ap->ants[i].col = col;
		ap->ants[i].row = row;
		ap->ants[i].direction = dir;
		ap->ants[i].state = 0;
	}
	draw_anant(mi, dir, col, row);
}

ENTRYPOINT void
draw_ant(ModeInfo * mi)
{
	antstruct  *anant;
	statestruct *status;
	int         i, state_pos, tape_pos;
	unsigned char color;
	short       oldDir;
	int         otherDir = 0;
	antfarmstruct *ap;

	if (antfarms == NULL)
		return;
	ap = &antfarms[MI_SCREEN(mi)];
	if (ap->ants == NULL)
		return;

#ifdef STANDALONE
	if (ap->eraser) {
		ap->eraser = erase_window (MI_DISPLAY(mi), MI_WINDOW(mi), ap->eraser);
		return;
	}
#endif

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
		oldDir = anant->direction;
		anant->direction = heading(ap->neighbors, oldDir, status->direction);
		if (ap->truchet) {
			int a = 0, b;

			if (ap->neighbors == 6) {
				if (ap->sharpturn) {
					a = (((ANGLES(ap->neighbors) + anant->direction - oldDir) %
						ANGLES(ap->neighbors)) == 240);
					/* should be some way of getting rid of the init_dir dependency... */
					b = !(ap->init_dir % 120);
					a = ((a && !b) || (b && !a));
					drawtruchet(mi, anant->col, anant->row, status->color, a);
				} else {
					a = (oldDir / 60) % 3;
					b = (anant->direction / 60) % 3;
					a = (a + b + 1) % 3;
					drawtruchet(mi, anant->col, anant->row, status->color, a);
				}
			} else if (ap->neighbors == 5) {
				int a, b, c;
				a = (oldDir / 72) % 5;
				b = (anant->direction / 72) % 5;
				if (ap->sharpturn) {
					c = (a + b + 4) % 5;
					if ((a == 2 && b == 0) || (a == 3 && b == 0)) {
						c = (c + 1) % 5;
					}
					if ((a == 0 && b == 1) || (a == 1 && b == 3) ||
							(a == 4 && b == 4)) {
						c = (c + 2) % 5;
					}
					if (a == 2 && b == 2) {
						c = (c + 3) % 5;
					}
					if (a == 3 && b == 3) {
						c = (c + 4) % 5;
					}
				} else {
					c = (a + b) % 5;
					if (a == 2 && b == 4) {
						c = (c + 1) % 5;
					}
					if (a == 3 && b == 1) {
						c = (c + 3) % 5;
					}
					if ((a == 0 && b == 2) || (a == 1 && b == 4) ||
							(a == 3 && b == 2) || (a == 4 && b == 0)) {
						c = (c + 4) % 5;
					}
				}
				otherDir = anant->direction;
				if ((a == 0 && b == 2) || (a == 0 && b == 4) ||
						(a == 1 && b == 4) || (a == 2 && b == 2) ||
						(a == 2 && b == 4) || (a == 3 && b == 2) ||
						(a == 4 && b == 2) || (a == 4 && b == 4)) {
					otherDir = ((anant->direction + 216) % 360);
				}
				if ((a == 1 && b == 0) || (a == 2 && b == 0)
						|| (a == 3 && b == 0) || (a == 4 && b == 0)) {
					otherDir = ((anant->direction + 288) % 360);
				}
				drawtruchet(mi, anant->col, anant->row, status->color, c);
			} else if (ap->neighbors == 4) {
				a = oldDir / 180;
				b = anant->direction / 180;
				a = ((a && !b) || (b && !a));
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			} else if (ap->neighbors == 3) {
				int chgDir = ((720 - status->direction) % 360);

				if (chgDir == 240)
					a = (2 + anant->direction / 120) % 3;
				else
					a = (1 + anant->direction / 120) % 3;
				drawtruchet(mi, anant->col, anant->row, status->color, a);
			}
			ap->truchet_state[tape_pos] = a + 1;
		}
		anant->state = status->next;

		/* Allow step first then turn */
		oldDir = ((status->direction < ANGLES(ap->neighbors)) ? anant->direction : oldDir);
#ifdef DEBUG
		(void) printf("oldDir %d, col %d, row %d", oldDir, anant->col, anant->row);
#endif
		positionOfNeighbor(ap, oldDir, &(anant->col), &(anant->row));
#ifdef DEBUG
		(void) printf(", ncol %d, nrow %d\n", anant->col, anant->row);
#endif
		if (neighbors == 5) {
			draw_anant(mi, otherDir, anant->col, anant->row);
		} else {
			draw_anant(mi, anant->direction, anant->col, anant->row);
		}
	}
	if (++ap->generation > MI_CYCLES(mi)) {
#ifdef STANDALONE
		ap->eraser = erase_window (MI_DISPLAY(mi), MI_WINDOW(mi), ap->eraser);
#endif
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
	if (label) {
		int size = MAX(MIN(MI_WIDTH(mi), MI_HEIGHT(mi)) - 1, 1);

		if (size >= 10 * FONT_WIDTH) {
			/* hard code these to corners */
			XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
				MI_WHITE_PIXEL(mi));
			/*XDrawString(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				16 + ap->labelOffsetX,
				16 + ap->labelOffsetY + FONT_HEIGHT,
				ap->ruleString, strlen(ap->ruleString)); */
			XDrawString(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				16 + ap->labelOffsetX, MI_HEIGHT(mi) - 16 -
				ap->labelOffsetY - FONT_HEIGHT / 2,
				ap->ruleString, strlen(ap->ruleString));
		}
	}
}

ENTRYPOINT void
release_ant(ModeInfo * mi)
{
	if (antfarms != NULL) {
		int        screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_ant_screen(MI_DISPLAY(mi), &antfarms[screen]);
		free(antfarms);
		antfarms = (antfarmstruct *) NULL;
	}
}

#ifndef STANDALONE
ENTRYPOINT void
refresh_ant(ModeInfo * mi)
{
	antfarmstruct *ap;

	if (antfarms == NULL)
		return;
	ap = &antfarms[MI_SCREEN(mi)];

	if (ap->painted) {
		MI_CLEARWINDOW(mi);
		ap->redrawing = 1;
		ap->redrawpos = 0;
	}
}
#endif

XSCREENSAVER_MODULE ("Ant", ant)

#endif /* MODE_ant */