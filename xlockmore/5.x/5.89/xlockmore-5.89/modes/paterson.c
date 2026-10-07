/* -*- Mode: C; tab-width: 4 -*- */
#if 0
static const char sccsid[] = "@(#)paterson.c	5.85 2025/10/15 xlockmore";

#endif

/*-
 * Copyright (c) 2025 by David Bagley.
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
 * 08-May-2025: similar to ant but worms die if they are forced to
 *              repeat paths
 * 15-Oct-2025: Fixed neighbors of 3.
 *              Added neighbors of 5.  Its kind of an inverse Cairo
 *              pattern (where the lines connect the centers of
 *              pentagons).  The grid keeps track of a neighborhood
 *              of 8 which later gets converted to a neighborhood of 5.
 *              Since grid is not completely regular, identical rules
 *              can produce different patterns depending on starting
 *              point.  Rules that produce the lopsided 10 line
 *              hexagons are prefered to those short lived rules of
 *              triangles and squares.
 *              Fixed when put in Xscreensaver.
 *
 * Based on Martin Gardner's chapter on "Worm Paths" in his book
 * "Knotted Doughnuts".  Beeler builds on work from Paterson and
 * Conway called "Paterson's Worm" (worm mode already taken).
 *
 * And this was helpful too: https://www.mathpuzzle.com/Worms.html
 *
 */

/*-
  Grid     Number of Neighbors
  ----     ------------------
  Triangle 3
  Square   4
  Pentagon 5
  Hexagon  6
*/

#ifdef STANDALONE
# define MODE_paterson
# define DEFAULTS	"*delay: 50000 \n" \
			"*count: -3 \n" \
			"*cycles: 2000 \n" \
			"*size: -32 \n" \
			"*ncolors: 64 \n" \
			"*fullrandom: True \n" \
			"*verbose: False \n" \
			"*fpsSolid: True \n" \

# define reshape_paterson 0
# define paterson_handle_event 0
# define UNIFORM_COLORS
# include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
# include "xlock.h"		/* in xlockmore distribution */
#endif /* STANDALONE */
#include "automata.h"

#ifdef MODE_paterson

#define DEF_LABEL "True"
#define FONT_HEIGHT 19
#define FONT_WIDTH 15
/*-
 * neighbors of 0 randomizes it for 3, 4, 5, 6
 */

#define DEF_NEIGHBORS  "0"      /* choose random value */
#define DEF_RULE "0" /* All rules, XScreensaver does not like null */
#define DEF_VERTICAL "False"

static Bool label;
static int  neighbors;
static char *rule;
static Bool vertical;

static XrmOptionDescRec opts[] =
{
	{(char *) "-label", (char *) ".paterson.label", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+label", (char *) ".paterson.label", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-neighbors", (char *) ".paterson.neighbors", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-rule", (char *) ".paterson.rule", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-vertical", (char *) ".paterson.vertical", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+vertical", (char *) ".paterson.vertical", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(void *) & label, (char *) "label", (char *) "Label", (char *) DEF_LABEL, t_Bool},
	{(void *) & neighbors, (char *) "neighbors", (char *) "Neighbors", (char *) DEF_NEIGHBORS, t_Int},
	{(void *) & rule, (char *) "rule", (char *) "Rule", (char *) DEF_RULE, t_String},
	{(void *) & vertical, (char *) "vertical", (char *) "Vertical", (char *) DEF_VERTICAL, t_Bool}
};
static OptionStruct desc[] =
{
	{(char *) "-/+label", (char *) "turn on/off rule labeling"},
	{(char *) "-neighbors num", (char *) "connections of 3, 4, 5, and 6"},
	{(char *) "-rule string", (char *) "string for Paterson Worm Notation"},
	{(char *) "-/+vertical", (char *) "change orientation for hexagons and triangles"}
};

ENTRYPOINT  ModeSpecOpt paterson_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
const ModStruct paterson_description =
{"paterson", "init_paterson", "draw_paterson", "release_paterson",
 "init_paterson", "init_paterson", "free_paterson", &paterson_opts,
 2000, -3, 50000, -32, 64, 1.0, "",
 "Shows Paterson's Worm Paths", 0, NULL};

#endif

#define MINWORMS 1
#define MINGRIDSIZE 24
#define MINSIZE 1
#define MINRANDOMSIZE 5
#define ANGLES 360
#define SLEEP_COUNT 32
#define FIELD2_SIZE6 5
#define FIELD3_SIZE6 10
#define FIELD4_SIZE6 10
#define FIELD2_SIZE5 4
#define FIELD3_SIZE5 6
#define FIELD2_SIZE4 2
#define RULE_SIZE (1 + FIELD2_SIZE6 + FIELD3_SIZE6 + FIELD4_SIZE6)

#define WHITE (MI_NPIXELS(mi))
#define RED (0)
#define ORANGE (3 * MI_NPIXELS(mi) / 32)
#define YELLOW (MI_NPIXELS(mi) / 6)
#define GREEN (23 * MI_NPIXELS(mi) / 64)
#define CYAN (MI_NPIXELS(mi) / 2)
#define BLUE (45 * MI_NPIXELS(mi) / 64)
#define MAGENTA (53 * MI_NPIXELS(mi) / 64)

int internetRule[RULE_SIZE];
Bool gardner;
int gardner1Rule;
int gardner2Rule6[FIELD2_SIZE6]; /* 1, many possible for multiple worms */
int gardner3Rule6[FIELD3_SIZE6]; /* 4, more possible for multiple worms */
int gardner4Rule6[FIELD4_SIZE6]; /* 1, many possible for multiple worms */
int gardner2Rule5[FIELD2_SIZE5];
int gardner3Rule5[FIELD3_SIZE5];
int gardner2Rule4[FIELD2_SIZE4];
char newGardnerRule[4 + RULE_SIZE];

typedef struct {
	int         alive;
	int         oldcol, oldrow;
	int         newcol, newrow;
	int         newaltcol, newaltrow; // for edges
	short       direction;
	int         color;
	int         state[RULE_SIZE];
	int         pathMask2[FIELD2_SIZE6];
	int         pathMask3[FIELD3_SIZE6];
	int         pathMask4[FIELD4_SIZE6];
	int         positions;
	int         pathMask;
	int         internetRule[RULE_SIZE];
	char        internetRuleString[80];
} wormstruct;

typedef struct {
	Bool        painted, vertical;
	int         neighbors, polygon;
	int         generation, mode;
	int         left;
	int         xs, ys;
	int         xb, yb;
	int         nrows, ncols;
	int         width, height;
	int         n, sleepCount;
	unsigned char *field;
	wormstruct  *worms;
	int         labelOffsetX, labelOffsetY;
	char        gardnerRuleString[80];
	char        internetRuleLabel[80], gardnerRuleLabel[80];
#ifdef STANDALONE
	eraser_state *eraser;
#endif
} patersonstruct;

/* Neighbors 3 worm
direction    mask       angle
  0          1          60
   \          \          \
    *-<-1      *-<-2      *-<-180
   /          /          /
  2          4         300
7->5->5->5->5->4->5->5->5->5->0
*/
/* Quadrille worm (Neighbors 4)
direction    mask       angle
    1          2          90
    |          |          |
 0--*-<-2   1--*-<-4   0--*-<-180
    |          |          |
    3          8         270
15->11->-11->11->9 (ie. choice between 1 & 8)
*/
/* Cairo worm (Neighbors 5)
direction    mask       angle
  0   1      1   2      36 108
   \ /        \ /        \ /
    *-<-2      *-<-4      *-<-180
   / \        / \        / \
  4   3     16   8     324 252
31->27->27->27->27->27->27...
31->27->27->27...
31->27->27...
Actually more complicated as need to keep track of 8 directions
*/
/* Isometric worm (Neighbors 6)
direction    mask       angle
  1   2      2   4      60 120
   \ /        \ /        \ /
 0--*-<-3  1 --*-<- 8  0--*-<-180
   / \        / \        / \
  5   4     32   16    300 240
63->55->55->55->55->55->53...
63->55->55->51...
*/
#if 0
static int field1Choice6[2] = {1, 2};
#endif
static int field2Choice6[5][4] = {
	{4, 5, 0, 1},
	{4, 5, 0, 2},
	{4, 5, 1, 2},
	{4, 0, 1, 2},
	{5, 0, 1, 2}
};
/* not sure of the logic of the order in book */
static int field3Choice6[10][3] = {
#if 0
	{4, 5, 3}, /* 4.1 sharp */
	{4, 5, 2}, /* 4.2 gentle */
	/*{4, 0, 2}, impossible opposite */
	{5, 0, 2}, /* 2.2 gentle */

	{4, 5, 2}, /* 3.1 sharp */
	{4, 0, 2}, /* 3.2 gentle */
	/*{5, 0, 1}, impossible opposite */

	{4, 1, 2}, /* 2.1 sharp */
	{5, 1, 2}, /* 1.2 gentle */

	{0, 1, 2} /* 1.1 sharp */
#else
	{0, 1, 2},
	{2, 5, 1},

	{4, 1, 2},
	{0, 5, 1},

	{4, 5, 2},
	{0, 4, 2},

	{4, 5, 0},
	{4, 5, 1},

	{4, 0, 2},
	{5, 0, 1}
#endif
};
static int field4Choice6[10][2] = {
	{4, 5},
	{4, 0},
	{4, 1},
	{4, 2},

	{5, 0},
	{5, 1},
	{5, 2},

	{0, 1},
	{0, 2},

	{1, 2}
};
#if 0
static int mask2Choice6[5] = {51, 53, 54, 23, 39};
/* last ones needed from Gardner article if # worms > 1 */
static int mask3Choice6[10] = {7, 38, 22, 35, 52, 21, 49, 50, 37, 19};
static int mask4Choice6[10] = {48, 17, 18, 20, 33, 34, 36, 3, 5, 6};
#endif
/* following is inverse of above choices 2-4 (2^6) */
static int invMaskChoice6[64] = {
	-1, -1, -1, 7,   -1, 8, 9, 0,
	-1, -1, -1, -1,  -1, -1, -1, -1,
	-1, 1, 2, 9,  3, 5, 2, 3,
	-1, -1, -1, -1,  -1, -1, -1, -1,

	-1, 4, 5, 3,  6, 8, 1, 4,
	-1, -1, -1, -1,  -1, -1, -1, -1,
	0, 6, 7, 0,  4, 1, 2, -1,
	-1, -1, -1, -1,  -1, -1, -1, -1
};

/*
        (0,1) (0,0)
            \ /
             *
            / \
        (1,1) (1,0)
*/

#if 0
static int field1Choice5[2] = {0, 1};
#endif
static int field2Choice5[4][3] = {
	{3, 4, 0},
	{3, 4, 1},
	{3, 0, 1},
	{4, 0, 1}
};
static int field3Choice5[6][2] = {
	{3, 4},
	{3, 0},
	{3, 1},
	{4, 0},
	{4, 1},
	{0, 1}
};
#if 0
static int mask2Choice5[4] = {7, 22, 21, 19};
static int mask3Choice5[6] = {6, 5, 20, 3, 18, 17};
#endif
/* following is inverse of above choices 2-3 (2^5) */
static int invMaskChoice5[32] = {
	-1, -1, -1, 3,   -1, 1, 0, 0,
	-1, -1, -1, -1,  -1, -1, -1, -1,
	-1, 5, 4, 3,  2, 2, 1, -1,
	-1, -1, -1, -1,  -1, -1, -1, -1
};
#if 0
static int field1Choice4[1] = {1};
#endif
static int field2Choice4[1][2] = {
	{3, 0}
};
#if 0
static int mask2Choice4[1] = {9};
#endif
static int invMaskChoice4[16] = {
	-1, -1, -1, -1,  -1, -1, -1, -1,
	-1, 0, -1, -1,  -1, -1, -1, -1
};
#if 0
static int field1Choice3[1] = {0};
#endif

#if 0
/* Gardner's notation (notation found in Gardner's book, good for randomizing) */
char neighbors3_rule[] = "1a";
char quadrille_rulea[] = "1a2a";
char quadrille_ruleb[] = "1a2b";
char trimmed_triangle[] = "1a2b3cac4b";
char curlyq_maze[] = "1a2b3caca4b";
char radioactivity[] = "1b2(ac)34a"; /* "1b2(ac)3(abc)(abc)(abc)(abc)4" */
char zipper[] = "1b2d3b4"; /* "1b2d3(abc)(abc)b4" */
char three_pointed_spiral[] = "1a2c3acba4a";
char shoot_grower[] = "1a2d3caaa4b";
char long_one[] = "1a2d3cbac4b"; /* 220142 */
char unknown_length[] = "1b2a3bcaa4b";
char cloud_path[] = "1a2a3baac4(ab)"; /* "1a2abaa3c4(ab)" ? */
char superdoily[] = "1a2d3cbaa4b";
char x[] = "1a2c3b4b";

/* Internet notation (notation found on internet, more straight forward) */
char neighbors3_rule[] = "1";
char quadrille_rule3[] = "1,3";
char quadrille_rule0[] = "1,0";
char x[] = "1,0,5,1";
char radioactivity[] = "2,0,0";
char same_start0[] = "1,0,4,0,1,5";
char same_start1[]= "1,0,4,0,1,0,1";
char same_start2[]= "1,0,4,0,1,0,2";
char same_start3[]= "1,0,4,0,1,0,5";
char curlyq_maze[]= "1,5,2,0,1,2,1";
char superdoily2[]=  "1,4,5,1,4,1,2"; /* 1a2a3cbca4a */
char longest_known[] = "1,0,4,2,0,2,0"; /* 5.7 * 10^13 steps */
char unknown[] = "1,0,4,2,0,1,5";
char arule[] = "1,2,2,5,4,4,2";
#endif
/*
	1. if no segments eaten, turn right.
	2. if all segments eaten, die.
	3. if only one segment uneaten, take it.
	4. Otherwise follow rule.
*/
/* if hexagonal (neighbors 3) only one path (no choices)
   if square only 2 paths (only one choice),
   if triangular 299 distinct paths (less as some die before they get too far)
   (ideas: allow directed paths for triangular and paths through connected by Cairo pattern)
*/

#define NEIGHBORKINDS ((long) (sizeof plots / sizeof *plots))

static patersonstruct *patersons = (patersonstruct *) NULL;

static Bool
validDiagonal8(int col, int row, int discreteDir8)
{
	if ((col & 1) == 0)
		return (discreteDir8 == 2 * (row & 1) + 1);
	else
		return (discreteDir8 == 7 - 2 * (row & 1));
}

static Bool
validDiscreteDir8(int col, int row, int discreteDir8)
{
	if (discreteDir8 % 2 == 0)
		return True;
	return validDiagonal8(col, row, discreteDir8);
}

static int
toDiscreteDir8(int dir)
{
	return ((dir + ANGLES / 2) % ANGLES) / (ANGLES / 8);
}

static int
rotateBits(int value, int nbits, int shift)
{
	int i, rotated = 0, temp;

	for (i = 0; i < nbits; i++) {
		temp = (value >> i) & 1;
		if (i + shift < nbits)
			rotated |= (temp << (i + shift));
		else
			rotated |= (temp << (i + shift - nbits));
	}
	return rotated;
}

static int
createMask(int col, int row, int dir8)
{
	int i, mask = 0, diagonal;
	if ((col & 1) == 0) {
		if ((row & 1) == 0)
			diagonal = 1;
		else
			diagonal = 3;
	} else {
		if ((row & 1) == 0)
			diagonal = 7;
		else
			diagonal = 5;
	}
	mask |= (1 << diagonal);
	for (i = 0; i < 4; i++) {
		mask |= (1 << (2 * i));
	}
	return rotateBits(mask, 8, dir8);
}

static int
pathMask8ToPathMask5(int col, int row, int dir8, int pathMask8)
{
	int i, shift = 0, temp, pathMask5 = 0, offset = 0;
	int pathMask8Rot, validMask8Rot;
	int bcol = (col & 1), brow = (row & 1);
#ifdef DEBUG
	(void) printf("pathMask8ToPathMask5: bcol=%d , brow=%d, dir8=%d, pathMask8=0%o,%d\n",
		bcol, brow, dir8, pathMask8, pathMask8);
#endif
	if ((dir8 & 1) == 1)
		offset = -1;
	else if (dir8 == 2 && bcol == 0 && brow == 0)
		offset = -1;
	else if (dir8 == 4 && bcol == 0 && brow == 1)
		offset = -1;
	else if (dir8 == 6 && bcol == 1 && brow == 1)
		offset = -1;
	else if (dir8 == 0 && bcol == 1 && brow == 0)
		offset = -1;
	else if (dir8 == 4 && bcol == 0 && brow == 0)
		offset = -2;
	else if (dir8 == 6 && bcol == 0 && brow == 1)
		offset = -2;
	else if (dir8 == 0 && bcol == 1 && brow == 1)
		offset = -2;
	else if (dir8 == 2 && bcol == 1 && brow == 0)
		offset = -2;
	pathMask8Rot = rotateBits(pathMask8, 8, ((16 - dir8 - offset) % 8));
	validMask8Rot = createMask(bcol, row, ((16 - dir8 - offset) % 8));
	for (i = 0; i < 8; i++) {
		if (1 == ((validMask8Rot >> i) & 1)) {
			temp = (pathMask8Rot >> i) & 1;
			pathMask5 |= (temp << shift);
			shift++;
		}
	}
#ifdef DEBUG
	(void) printf(" pathMask5 result: 0%o\n", pathMask5);
#endif
	return pathMask5;
}

static int
deshift(int mask)
{
	int value = mask, count = -1;
	while (value > 0) {
		value = (value >> 1);
		count++;
	}
	return count;
}

static int
deshift5(int col, int row, int dir, int mask)
{
	int i, choice = 5;
	int pathMask5 = pathMask8ToPathMask5(col, row, dir, mask);
	for (i = 0; i < 5; i++)
		if (((pathMask5 >> i) & 1) == 1)
			choice = i;
	if (choice == 5)
		return choice;
	return (5 - choice) % 5;
}

/* 0 gentle cw, 1 sharp cw, 2 reverse, 3 sharp ccw, 4 gentle ccw */
static int
nextCairoAngle(int choice, int dir, int col, int row)
{
#ifdef DEBUG
	(void) printf ("nextCairoAngle(%d %d %d %d)\n", choice, dir, col, row);
#endif
	switch (choice) {
	case 0:
		switch (dir) {
		case 0:
			if ((col & 1) == 1)
				return 0;
			if ((row & 1) == 1)
				return 315;
			return 270;
		case 45:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 0;
			return -45;
		case 90:
			if ((row & 1) == 0)
				return 90;
			if ((col & 1) == 1)
				return 45;
			return 0;
		case 135:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 90;
			return -135;
		case 180:
			if ((col & 1) == 0)
				return 180;
			if ((row & 1) == 0)
				return 135;
			return 90;
		case 225:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 180;
			return -225;
		case 270:
			if ((row & 1) == 1)
				return 270;
			if ((col & 1) == 0)
				return 225;
			return 180;
		case 315:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 270;
			return -315;
		default:
			(void) fprintf(stderr, "choice 0, wrong direction %d\n", dir);
			return -1;
		}
	case 1:
		switch (dir) {
		case 0:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 225;
			return 270;
		case 45:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 270;
			return -45;
		case 90:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 315;
			return 0;
		case 135:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 0;
			return -135;
		case 180:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 45;
			return 90;
		case 225:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 90;
			return -225;
		case 270:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 135;
			return 180;
		case 315:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 180;
			return -315;
		default:
			(void) fprintf(stderr, "choice 1, wrong direction %d\n", dir);
			return -1;
		}
	case 2: /* reverse */
		return (dir + ANGLES / 2) % ANGLES;
	case 3:
		switch (dir) {
		case 0:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 135;
			return 90;
		case 45:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 180;
			return -45;
		case 90:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 225;
			return 180;
		case 135:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 270;
			return -135;
		case 180:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 315;
			return 270;
		case 225:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 0;
			return -225;
		case 270:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 45;
			return 0;
		case 315:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 90;
			return -315;
		default:
			(void) fprintf(stderr, "choice 3, wrong direction %d\n", dir);
			return -1;
		}
	case 4:
		switch (dir) {
		case 0:
			if ((col & 1) == 0)
				return 0;
			if ((row & 1) == 1)
				return 45;
			return 90;
		case 45:
			if (((col & 1) == 0) && ((row & 1) == 0))
				return 90;
			return -45;
		case 90:
			if ((row & 1) == 1)
				return 90;
			if ((col & 1) == 1)
				return 135;
			return 180;
		case 135:
			if (((col & 1) == 0) && ((row & 1) == 1))
				return 180;
			return -135;
		case 180:
			if ((col & 1) == 1)
				return 180;
			if ((row & 1) == 0)
				return 225;
			return 270;
		case 225:
			if (((col & 1) == 1) && ((row & 1) == 1))
				return 270;
			return -225;
		case 270:
			if ((row & 1) == 0)
				return 270;
			if ((col & 1) == 0)
				return 315;
			return 0;
		case 315:
			if (((col & 1) == 1) && ((row & 1) == 0))
				return 0;
			return -315;
		default:
			(void) fprintf(stderr, "choice 4, wrong direction %d\n", dir);
			return -1;
		}
	}
	(void) fprintf(stderr, "choice %d, dir %d\n", choice, dir);
	return -360;
}

static int
nextAngle(int neighbors, int n, int choice, int dir, int col, int row)
{
	if (choice == (neighbors - (neighbors & 1)) / 2) {
		/*(void) fprintf(stderr, "reverse? %d, neighbors=%d, n=%d\n", choice, neighbors, n);
		return (dir + ANGLES / 2) % ANGLES;*/
		return -1;
	}
	switch (neighbors) {
	case 3:
		return (ANGLES + dir + choice * ANGLES / (2 * neighbors)
			- ANGLES / (2 * neighbors)) % ANGLES;
	case 4:
	case 6:
		return (ANGLES + dir - choice * ANGLES / neighbors) % ANGLES;
	case 5:
		return nextCairoAngle(choice, dir, col, row);
	default:
		(void) fprintf(stderr, "n=%d, %d neighbors unknown\n", n, neighbors);
		return -1;
	}
}

static void
nextPosition(patersonstruct *pp, int newDir, int *pcol, int *prow,
		int *pcol2, int *prow2)
{
	int col = *pcol, row = *prow;
	int col2 = col, row2 = row;
#ifdef DEBUG
	(void) printf(" newDir=%d, col=%d, row=%d, neighbors=%d\n",
		newDir, col, row, pp->neighbors);
#endif
	if (pp->neighbors == 3 || pp->neighbors == 4
			|| pp->neighbors == 5 || pp->neighbors == 6) {
		switch (newDir) {
		case 0:
			col2 = col + 1;
			col = (col + 1 == pp->ncols) ? 0 : col + 1;
			break;
		case 45:
			col2 = col + 1;
			col = (col + 1 == pp->ncols) ? 0 : col + 1;
			row2 = row - 1;
			row = (!row) ? pp->nrows - 1 : row - 1;
			break;
		case 60:
			if (!(row & 1)) {
				col2 = col + 1;
				col = (col + 1 == pp->ncols) ? 0 : col + 1;
			}
			row2 = row - 1;
			row = (!row) ? pp->nrows - 1 : row - 1;
			break;
		case 90:
			row2 = row - 1;
			row = (!row) ? pp->nrows - 1 : row - 1;
			break;
		case 120:
			if (row & 1) {
				col2 = col - 1;
				col = (!col) ? pp->ncols - 1 : col - 1;
			}
			row2 = row - 1;
			row = (!row) ? pp->nrows - 1 : row - 1;
			break;
		case 135:
			col2 = col - 1;
			col = (!col) ? pp->ncols - 1 : col - 1;
			row2 = row - 1;
			row = (!row) ? pp->nrows - 1 : row - 1;
			break;
		case 180:
			col2 = col - 1;
			col = (!col) ? pp->ncols - 1 : col - 1;
			break;
		case 225:
			col2 = col - 1;
			col = (!col) ? pp->ncols - 1 : col - 1;
			row2 = row + 1;
			row = (row + 1 == pp->nrows) ? 0 : row + 1;
			break;
		case 240:
			if (row & 1) {
				col2 = col - 1;
				col = (!col) ? pp->ncols - 1 : col - 1;
			}
			row2 = row + 1;
			row = (row + 1 == pp->nrows) ? 0 : row + 1;
			break;
		case 270:
			row2 = row + 1;
			row = (row + 1 == pp->nrows) ? 0 : row + 1;
			break;
		case 300:
			if (!(row & 1)) {
				col2 = col + 1;
				col = (col + 1 == pp->ncols) ? 0 : col + 1;
			}
			row2 = row + 1;
			row = (row + 1 == pp->nrows) ? 0 : row + 1;
			break;
		case 315:
			col2 = col + 1;
			col = (col + 1 == pp->ncols) ? 0 : col + 1;
			row2 = row + 1;
			row = (row + 1 == pp->nrows) ? 0 : row + 1;
			break;
		default:
			(void) fprintf(stderr, "wrong direction %d\n", newDir);
		}
	}
	*pcol = col;
	*prow = row;
	*pcol2 = col2;
	*prow2 = row2;
}

static int
getColor(ModeInfo * mi, int i)
{
	switch (i) {
	case 0:
		return WHITE;
	case 1:
		return MAGENTA;
	case 2:
		return RED;
	case 3:
		return ORANGE;
	case 4:
		return YELLOW;
	case 5:
		return GREEN;
	case 6:
		return CYAN;
	case 7:
	default:
		return BLUE;
		/*(void) printf("need a new color!!!!\n");*/
	}
}

static void
drawLine(ModeInfo * mi, GC gc, int oldcol, int oldrow, int newcol, int newrow)
{
	patersonstruct *pp = &patersons[MI_SCREEN(mi)];
	Display *display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	XPoint oldPt, newPt;

	if (pp->neighbors == 3 || pp->neighbors == 6) {
		int coldcol = 2 * oldcol + !(oldrow & 1), coldrow = 2 * oldrow;
		int cnewcol = 2 * newcol + !(newrow & 1), cnewrow = 2 * newrow;

		if (pp->vertical) {
			oldPt.y = pp->xb + coldcol * pp->xs;
			oldPt.x = pp->yb + coldrow * pp->ys;
			newPt.y = pp->xb + cnewcol * pp->xs;
			newPt.x = pp->yb + cnewrow * pp->ys;
		} else {
			oldPt.x = pp->xb + coldcol * pp->xs;
			oldPt.y = pp->yb + coldrow * pp->ys;
			newPt.x = pp->xb + cnewcol * pp->xs;
			newPt.y = pp->yb + cnewrow * pp->ys;
		}
		XDrawLine(display, window, gc, oldPt.x, oldPt.y,
			newPt.x, newPt.y);
	} else if (pp->neighbors == 4 || pp->neighbors == 5) {
		oldPt.x = pp->xb + oldcol * pp->xs;
		oldPt.y = pp->yb + oldrow * pp->ys;
		newPt.x = pp->xb + newcol * pp->xs;
		newPt.y = pp->yb + newrow * pp->ys;
		XDrawLine(display, window, gc, oldPt.x, oldPt.y,
			newPt.x, newPt.y);
	}
}

static void
drawWorm(ModeInfo * mi, wormstruct *worm)
{
        Display    *display = MI_DISPLAY(mi);

	if (worm->color == 0 || MI_NPIXELS(mi) == 2)
		XSetForeground(display, MI_GC(mi), MI_WHITE_PIXEL(mi));
	else
		XSetForeground(display, MI_GC(mi),
			MI_PIXEL(mi, getColor(mi, worm->color)));
	drawLine(mi, MI_GC(mi), worm->oldcol, worm->oldrow,
		worm->newaltcol, worm->newaltrow);
}

static void
patersonWorm(ModeInfo * mi, wormstruct *aworm, int discreteDir, int otherDir)
{
	patersonstruct *pp = &patersons[MI_SCREEN(mi)];
	int fieldPosFrom, fieldPosTo;

	/* Can we change from undirected to directed? */
	fieldPosFrom = aworm->oldcol + aworm->oldrow * pp->ncols;
	if (pp->neighbors == 5)
		pp->field[fieldPosFrom] |= (1 << (discreteDir  % 8));
	else
		pp->field[fieldPosFrom] |= (1 << (discreteDir % pp->neighbors));
	fieldPosTo = aworm->newcol + aworm->newrow * pp->ncols;
	if (pp->neighbors == 5)
		pp->field[fieldPosTo] |= (1 << ((discreteDir + 4) % 8));
	else if ((pp->neighbors & 1) == 1)
		pp->field[fieldPosTo] |= (1 << (discreteDir % pp->neighbors));
	else
		pp->field[fieldPosTo] |= (1 << ((discreteDir + pp->neighbors / 2) % pp->neighbors));
#ifdef DEBUG
	(void) printf(" x=%d y=%d from=%d fromField=%d, x=%d y=%d to=%d toField=%d, discreteDir=%d\n",
		aworm->oldcol, aworm->oldrow, fieldPosFrom, pp->field[fieldPosFrom],
		aworm->newcol, aworm->newrow, fieldPosTo, pp->field[fieldPosTo], discreteDir);
#endif
	drawWorm(mi, aworm);
}

static int
getPathsRemaining(patersonstruct *pp, int fieldPos, int vector, int *pathMask)
{
	int i, count = 0, dirMask;
	*pathMask = 0;
#ifdef DEBUG
	(void) printf("fieldPos %d, vector %d, path ", fieldPos, vector);
#endif
	if (pp->neighbors == 5) {
		for (i = 0; i < 8; i++) {
			int col = fieldPos % pp->ncols;
			int row = fieldPos / pp->ncols;
			if (validDiscreteDir8(col, row, i) && (vector == -1 || (i != (vector + 4) % 8))) {
				dirMask = 1 << (i % 8);
				if ((pp->field[fieldPos] & dirMask) == 0) {
					*pathMask |= (1 << i);
#ifdef DEBUG
					(void) printf("%d,", i);
#endif
					count++;
				}
			}
		}
	} else {
		for (i = 0; i < pp->neighbors; i++) {
			dirMask = 1 << ((2 * pp->neighbors + vector - i) % pp->neighbors);
			if ((pp->field[fieldPos] & dirMask) == 0) {
				*pathMask |= (1 << i);
#ifdef DEBUG
				(void) printf("%d,", i);
#endif
				count++;
			}
		}
	}
#ifdef DEBUG
	(void) printf(" remaining: %d, pm 0%o\n", count, *pathMask);
#endif
	return count;
}

/*
  Gardner's rule (in Gardner's book he has a simplified version of
  Beeler's rules (not given)).
  The advantage for this rule is that you can randomize it, though some
  rules are not interesting.
  The ability to randomize easily may not be as trivial if there is more
  than one worm.
  The other rule, called here as the Internet rule (not sure who came up
  with it), is more straight forward and documented on the internet.  This
  rule is created more as you encounter different configurations of empty
  lines (can be considered as where there was food for the worm).
*/

static Bool
gardnerRule(char * stringRule)
{
	int i;

	for (i = 0; stringRule[i] != '\0'; i++) {
		if (stringRule[i] == 'a' || stringRule[i] == 'b') {
			return True;
		}
	}
	return False;
}

static void
setGardnerRule6(char *stringRule, int *rule1Choice,
		int *rule2Choice, int *rule3Choice, int *rule4Choice)
{
	int i, pathsTaken = 1;
	int case2 = 0, case3 = 0, case4 = 0;

	*rule1Choice = -1;
	for (i = 0; i < FIELD2_SIZE6; i++)
		rule2Choice[i] = -1;
	for (i = 0; i < FIELD3_SIZE6; i++)
		rule3Choice[i] = -1;
	for (i = 0; i < FIELD4_SIZE6; i++)
		rule4Choice[i] = -1;
	if (!stringRule)
		return;
	for (i = 0; stringRule[i] != '\0'; i++) {
		if (stringRule[i] >= '1' && stringRule[i] <= '4')
			pathsTaken = stringRule[i] - '0';
		else {
			switch (pathsTaken) {
			case 1:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'b') {
					*rule1Choice = stringRule[i] - 'a';
				}
				break;
			case 2:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'd' && case2 < FIELD2_SIZE6) {
					rule2Choice[case2] = stringRule[i] - 'a';
					case2++;
				}
				break;
			case 3:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'c' && case3 < FIELD3_SIZE6) {
					rule3Choice[case3] = stringRule[i] - 'a';
					case3++;
				}
				break;
			case 4:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'b' && case4 < FIELD4_SIZE6) {
					rule4Choice[case4] = stringRule[i] - 'a';
					case4++;
				}
				break;
			}
		}
	}
}

static void
setGardnerRule5(char *stringRule, int *rule1Choice,
		int *rule2Choice, int *rule3Choice)
{
	int i, pathsTaken = 1;
	int case2 = 0, case3 = 0;

	*rule1Choice = -1;
	for (i = 0; i < FIELD2_SIZE5; i++)
		rule2Choice[i] = -1;
	for (i = 0; i < FIELD3_SIZE5; i++)
		rule3Choice[i] = -1;
	if (!stringRule)
		return;
	for (i = 0; stringRule[i] != '\0'; i++) {
		if (stringRule[i] >= '1' && stringRule[i] <= '3')
			pathsTaken = stringRule[i] - '0';
		else {
			switch (pathsTaken) {
			case 1:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'b') {
					*rule1Choice = stringRule[i] - 'a';
				}
				break;
			case 2:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'c' && case2 < FIELD2_SIZE5) {
					rule2Choice[case2] = stringRule[i] - 'a';
					case2++;
				}
				break;
			case 3:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'b' && case3 < FIELD3_SIZE5) {
					rule3Choice[case3] = stringRule[i] - 'a';
					case3++;
				}
				break;
			}
		}
	}
}

static void
setGardnerRule4(char *stringRule, int *rule1Choice,
		int *rule2Choice)
{
	int i, pathsTaken = 1;
	int case2 = 0;

	*rule1Choice = -1;
	for (i = 0; i < FIELD2_SIZE4; i++)
		rule2Choice[i] = -1;
	if (!stringRule)
		return;
	for (i = 0; stringRule[i] != '\0'; i++) {
		if (stringRule[i] >= '1' && stringRule[i] <= '2')
			pathsTaken = stringRule[i] - '0';
		else {
			switch (pathsTaken) {
			case 1:
				if (stringRule[i] >= 'a') {
					*rule1Choice = stringRule[i] - 'a';
				}
				break;
			case 2:
				if (stringRule[i] >= 'a' && stringRule[i] <= 'b' && case2 < FIELD2_SIZE4) {
					rule2Choice[case2] = stringRule[i] - 'a';
					case2++;
				}
				break;
			}
		}
	}
}

static void
setGardnerRule3(char *stringRule, int *rule1Choice)
{
	int i, pathsTaken = 1;

	*rule1Choice = -1;
	if (!stringRule)
		return;
	for (i = 0; stringRule[i] != '\0'; i++) {
		if (stringRule[i] == '1')
			pathsTaken = stringRule[i] - '0';
		else {
			switch (pathsTaken) {
			case 1:
				if (stringRule[i] >= 'a') {
					*rule1Choice = stringRule[i] - 'a';
				}
				break;
			}
		}
	}
}

static void
setInternetRule(char *stringRule, int *rule)
{
	int i = 0, n = 0;

	for (i = 0; i < RULE_SIZE; i++)
		rule[i] = -1;
	if (!stringRule)
		return;
	i = 0;
	while (stringRule[n] && i < RULE_SIZE) {
		if (stringRule[n] >= '0' && stringRule[n] <= '5') {
			rule[i] = stringRule[n] - '0';
			i++;
		}
		n++;
	}
}

static Bool
createGardnerRule6(patersonstruct *pp, char *newRule)
{
	int i = 0, j;
	int size2, size3, size4;
	char store[4]; /* to exclude common zipper rules */

	if (pp->n == 1) { /* multiple worms seem to need more terms */
		size2 = 1;
		size3 = 4;
		store[2] = 'a'; /* store[2] may be used uninitialized? */
		size4 = 1;
	} else {
		size2 = FIELD2_SIZE6;
		size3 = FIELD3_SIZE6;
		size4 = FIELD4_SIZE6;
	}
	newRule[i++] = '1';
	newRule[i] = NRAND(2) + 'a';
	store[0] = newRule[i];
	store[1] = 'a'; /* or gives warning */
	i++;
	newRule[i++] = '2';
	for (j = 0; j < size2; j++) {
		newRule[i] = NRAND(4) + 'a';
		if (j == 0)
			store[1] = newRule[i];
		i++;
	}
	newRule[i++] = '3';
	for (j = 0; j < size3; j++) {
		newRule[i] = NRAND(3) + 'a';
		if (j == 0)
			store[2] = newRule[i];
		i++;
	}
	newRule[i++] = '4';
	for (j = 0; j < size4; j++) {
		newRule[i] = NRAND(2) + 'a';
		if (j == 0)
			store[3] = newRule[i];
		i++;
	}
	newRule[i] = '\0';
	if (store[0] == 'b' && store[2] == 'b') {
		if (store[1] == 'b' || store[1] == 'd' || store[3] == 'b') {
#ifdef DEBUG
			(void) printf("zipper rule\n");
#endif
			return True;
		}
	}
	return False;
}

static void
createGardnerRule5(patersonstruct *pp, char *newRule)
{
	int i = 0, j;
	int size2, size3;

	size2 = FIELD2_SIZE5;
	size3 = FIELD3_SIZE5;
	newRule[i++] = '1';
	/* make b less likely, as they do not last long */
	newRule[i] = (NRAND(10) == 0) ? 'b' : 'a';
	i++;
	newRule[i++] = '2';
	for (j = 0; j < size2; j++) {
		newRule[i] = NRAND(3) + 'a';
		i++;
	}
	newRule[i++] = '3';
	for (j = 0; j < size3; j++) {
		newRule[i] = NRAND(2) + 'a';
		i++;
	}
	newRule[i] = '\0';
}

static void
createGardnerRule4(patersonstruct *pp, char *newRule)
{
	int i = 0, j;
	int size2;

	if (pp->n == 1) { /* multiple worms seem to need more terms */
		size2 = 1;
	} else {
		size2 = FIELD2_SIZE4;
	}
	newRule[i++] = '1';
	newRule[i] = 'a';
	i++;
	newRule[i++] = '2';
	for (j = 0; j < size2; j++) {
		newRule[i] = NRAND(2) + 'a';
		i++;
	}
	newRule[i] = '\0';
}

static void
createGardnerRule3(patersonstruct *pp, char *newRule)
{
	int i;

	i = 0;
	newRule[i++] = '1';
	newRule[i] = 'a';
	i++;
	newRule[i] = '\0';
}

static int
changeHandDir(int dir)
{
	return (2 * ANGLES - dir) % ANGLES;
}

#if 0
static void
testPathMask8ToPathMask5()
{
	int col = 0, row = 0, dir, pathMask8, i;

	for (col = 0; col < 2; col++) {
		for (row = 0; row < 2; row++) {
			for (dir = 0; dir < 8; dir++) {
				for (i = 0; i < 8; i++)	{
			pathMask8 = 1 << i;
			printf("%d %d %d %d %d\n", col, row, dir, i, pathMask8ToPathMask5(col, row, dir, pathMask8));
				}
			}
		}
	}
}

static void
testNextAngle()
{
	int nb, n, c, d, col, row;
	for (nb = 3; nb < 7; nb++) {
		if (nb == 5)
			for (c = 0; c < n; c++) {
				for (d = 0; d < ANGLES; d += ANGLES / 8)
			  for (col = 0; col < 2; col++)
			    for (row = 0; row < 2; row++) {
					(void) printf("neighbors=%d, choice=%d, direction=%d, col%d, row %d, nextDirection=%d\n",
						n, c, d, col, row, nextAngle(nb, 1, c, d, col, row));
					/*(void) printf("opp neighbors=%d, choice=%d, direction=%d, nextDirection=%d\n",
						n, c, d, nextAngle(nb, 1, changeHandChoice(n, c), d, 0, 0));*/
				}
			}
		else
			for (c = 0; c < n; c++) {
				for (d = 0; d < ANGLES; d += ANGLES / n) {
					(void) printf("neighbors=%d, choice=%d, direction=%d, nextDirection=%d\n",
						n, c, d, nextAngle(nb, 1, c, d, 0, 0));
					/*(void) printf("opp neighbors=%d, choice=%d, direction=%d, nextDirection=%d\n",
						n, c, d, nextAngle(nb, 1, changeHandChoice(n, c), d, 0, 0));*/
			}
		}
	}
}
#endif

#if 0
static void
testRotateBits()
{
	int shift;
	for (shift = 0; shift < 8; shift++) {
		printf("%d %d\n", shift, rotateBits(5, 8, shift));
	}
}

static void
testGetCairoDir()
{
	int d, c, r;
	for (d = 0; d < ANGLES; d += ANGLES / 8) {
		if (d % 10 == 5)
			(void) printf("d=%d: %d\n",
				d, getCwCairoDir(d, 0, 0));
		else
			for (c = 0; c < 2; c++)
				for (r = 0; r < 2; r++) {
					(void) printf("d=%d, col=%d, row=%d: %d\n",
						d, c, r, getCwCairoDir(d, c, r));
			}
	}
}
#endif

static void
setGardnerRule6String(char *ruleString, int *pathMask2, int *pathMask3, int *pathMask4,
		int gardner1Rule, int *gardner2Rule6, int *gardner3Rule6, int *gardner4Rule6)
{
	int n, i = 0;

	ruleString[i++] = '1';
	if (gardner1Rule != -1)
		ruleString[i++] = (gardner1Rule + 'a');
	ruleString[i++] = '2';
	for (n = 0; n < FIELD2_SIZE6; n++) {
		if (gardner2Rule6[n] == -1 || (pathMask2 != NULL && pathMask2[n] == -1))
			break;
		else
			ruleString[i++] = (gardner2Rule6[n] + 'a');
	}
	ruleString[i++] = '3';
	for (n = 0; n < FIELD3_SIZE6; n++) {
		if (gardner3Rule6[n] == -1 || (pathMask3 != NULL && pathMask3[n] == -1))
			break;
		else
			ruleString[i++] = (gardner3Rule6[n] + 'a');
	}
	ruleString[i++] = '4';
	for (n = 0; n < FIELD4_SIZE6; n++) {
		if (gardner4Rule6[n] == -1 || (pathMask4 != NULL && pathMask4[4] == -1))
			break;
		else
			ruleString[i++] = (gardner4Rule6[n] + 'a');
	}
	ruleString[i] = '\0';
}

static void
setGardnerRule5String(char *ruleString, int *pathMask2, int *pathMask3,
		int gardner1Rule, int *gardner2Rule5, int *gardner3Rule5)
{
	int n, i = 0;

	ruleString[i++] = '1';
	if (gardner1Rule != -1)
		ruleString[i++] = (gardner1Rule + 'a');
	ruleString[i++] = '2';
	for (n = 0; n < FIELD2_SIZE5; n++) {
		if (gardner2Rule5[n] == -1 || (pathMask2 != NULL && pathMask2[n] == -1))
			break;
		else
			ruleString[i++] = (gardner2Rule5[n] + 'a');
	}
	ruleString[i++] = '3';
	for (n = 0; n < FIELD3_SIZE5; n++) {
		if (gardner3Rule5[n] == -1 || (pathMask3 != NULL && pathMask3[n] == -1))
			break;
		else
			ruleString[i++] = (gardner3Rule5[n] + 'a');
	}
	ruleString[i] = '\0';
}

static void
setGardnerRule4String(char *ruleString, int *pathMask2,
		int gardner1Rule, int *gardner2Rule4)
{
	int n, i = 0;

	ruleString[i++] = '1';
	if (gardner1Rule != -1)
		ruleString[i++] = (gardner1Rule + 'a');
	ruleString[i++] = '2';
	for (n = 0; n < FIELD2_SIZE4; n++) {
		if (gardner2Rule4[n] == -1 || (pathMask2 != NULL && pathMask2[n] == -1))
			break;
		else
			ruleString[i++] = (gardner2Rule4[n] + 'a');
	}
	ruleString[i] = '\0';
}

static void
setGardnerRule3String(char *ruleString, int gardner1Rule)
{
	int i = 0;

	ruleString[i++] = '1';
	if (gardner1Rule != -1)
		ruleString[i++] = (gardner1Rule + 'a');
	ruleString[i] = '\0';
}

static void
setInternetRuleString(char *ruleString, int *rule)
{
	int n, i = 0;

	ruleString[i++] = '{';
	ruleString[i++] = ' ';
	for (n = 0; n < RULE_SIZE; n++) {
		if (rule[n] < 0)
			break;
		if (n > 0)
			ruleString[i++] = ',';
		if (rule[n] < 6)
			ruleString[i++] = rule[n] + '0';
	}
	ruleString[i++] = ' ';
	ruleString[i++] = '}';
	ruleString[i] = '\0';
}

static int
gardnerFieldRuleShort(ModeInfo *mi, wormstruct *aworm, int ruleField)
{
	if (MI_IS_VERBOSE(mi))
		(void) printf(" Rule%d too short\n", ruleField);
	aworm->alive = False;
	return -1;
}

static int
gardnerNoChoice(wormstruct *aworm, int choice)
{
	if (choice == -1)
		(void) printf(" FIXME, no choice\n");
	(void) printf(" Death pathMask=0%o, choice=%d\n", aworm->pathMask, choice);
	aworm->alive = False;
	return choice;
}

static int
gardnerNoPath(patersonstruct *pp, wormstruct *aworm, int ruleField)
{
	if (pp->n < 1) {
		(void) printf(" FIXME, no path, n=%d, for neighbors %d\n", pp->n, pp->neighbors);
		(void) printf(" Death pathMask=0%o, ruleField=%d\n", aworm->pathMask, ruleField);
	}
	aworm->alive = False;
	return -1;
}

static int
useGardnerRule(ModeInfo *mi, int paths, int i, int oldDir)
{
	patersonstruct *pp;
	wormstruct *aworm;
	int j, ruleField, choice = -1;

	pp = &patersons[MI_SCREEN(mi)];
	aworm = &pp->worms[i];
	if (pp->neighbors == 6) {
		if (paths == 4) {
			ruleField = 2; /* pp->neighbors - paths */
			if (invMaskChoice6[aworm->pathMask] != -1) {
				for (j = 0; j < FIELD2_SIZE6; j++) {
					if (gardner2Rule6[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask2[j] == -1) {
						choice = field2Choice6[invMaskChoice6[aworm->pathMask]][gardner2Rule6[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice6[aworm->pathMask],
							(gardner2Rule6[j] + 'a'), choice);
#endif
						aworm->pathMask2[j] = aworm->pathMask;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d %s\n",
									ruleField, aworm->pathMask, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d %s\n",
									i, ruleField, aworm->pathMask, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask2[j] == aworm->pathMask) {
						choice = field2Choice6[invMaskChoice6[aworm->pathMask]][gardner2Rule6[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice6[aworm->pathMask],
							(gardner2Rule6[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		} else if (paths == 3) {
			ruleField = 3; /* pp->neighbors - paths */
			if (invMaskChoice6[aworm->pathMask] != -1) {
				for (j = 0; j < FIELD3_SIZE6; j++) {
					if (gardner3Rule6[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask3[j] == -1) {
						choice = field3Choice6[invMaskChoice6[aworm->pathMask]][gardner3Rule6[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice6[aworm->pathMask],
							(gardner3Rule6[j] + 'a'), choice);
#endif
						aworm->pathMask3[j] = aworm->pathMask;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d %s\n",
									ruleField, aworm->pathMask, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d %s\n",
									i, ruleField, aworm->pathMask, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask3[j] == aworm->pathMask) {
						choice = field3Choice6[invMaskChoice6[aworm->pathMask]][gardner3Rule6[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
						invMaskChoice6[aworm->pathMask],
							(gardner3Rule6[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		} else if (paths == 2) {
			ruleField = 4; /* pp->neighbors - paths */
			if (invMaskChoice6[aworm->pathMask] != -1) {
				for (j = 0; j < FIELD4_SIZE6; j++) {
					if (gardner4Rule6[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask4[j] == -1) {
						choice = field4Choice6[invMaskChoice6[aworm->pathMask]][gardner4Rule6[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice6[aworm->pathMask],
							(gardner4Rule6[j] + 'a'), choice);
#endif
						aworm->pathMask4[j] = aworm->pathMask;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d %s\n",
									ruleField, aworm->pathMask, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d %s\n",
									i, ruleField, aworm->pathMask, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask4[j] == aworm->pathMask) {
						choice = field4Choice6[invMaskChoice6[aworm->pathMask]][gardner4Rule6[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice6[aworm->pathMask],
							(gardner4Rule6[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		}
	} else if (pp->neighbors == 5) {
		if (paths == 3) {
			int col = aworm->newcol;
			int row = aworm->newrow;
			int pathMask5 = pathMask8ToPathMask5(col, row, oldDir, aworm->pathMask);
			ruleField = 2; /* pp->neighbors - paths */
			if (invMaskChoice5[pathMask5] != -1) {
				for (j = 0; j < FIELD2_SIZE5; j++) {
					if (gardner2Rule5[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask2[j] == -1) {
						choice = field2Choice5[invMaskChoice5[pathMask5]][gardner2Rule5[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d:%d [%d, %c] %d\n", ruleField, j, aworm->pathMask, pathMask5,
							invMaskChoice5[pathMask5],
							(gardner2Rule5[j] + 'a'), choice);
#endif
						aworm->pathMask2[j] = pathMask5;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d:%d %s\n",
									ruleField, aworm->pathMask, pathMask5, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d:%d %s\n",
									i, ruleField, aworm->pathMask, pathMask5, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask2[j] == pathMask5) {
						choice = field2Choice5[invMaskChoice5[pathMask5]][gardner2Rule5[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d:%d [%d, %c] %d\n", ruleField, j, aworm->pathMask, pathMask5,
							invMaskChoice5[pathMask5],
							(gardner2Rule5[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		} else if (paths == 2) {
			int col = aworm->newcol;
			int row = aworm->newrow;
			int pathMask5 = pathMask8ToPathMask5(col, row, oldDir, aworm->pathMask);
			ruleField = 3; /* pp->neighbors - paths */
			if (invMaskChoice5[pathMask5] != -1) {
				for (j = 0; j < FIELD3_SIZE5; j++) {
					if (gardner3Rule5[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask3[j] == -1) {
						choice = field3Choice5[invMaskChoice5[pathMask5]][gardner3Rule5[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d:%d [%d, %c] %d\n", ruleField, j, aworm->pathMask, pathMask5,
							invMaskChoice5[pathMask5],
							(gardner3Rule5[j] + 'a'), choice);
#endif
						aworm->pathMask3[j] = pathMask5;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d:%d %s\n",
									ruleField, aworm->pathMask, pathMask5, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d:%d %s\n",
									i, ruleField, aworm->pathMask, pathMask5, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask3[j] == pathMask5) {
						choice = field3Choice5[invMaskChoice5[pathMask5]][gardner3Rule5[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d:%d [%d, %c] %d\n", ruleField, j, aworm->pathMask, pathMask5,
							invMaskChoice5[pathMask5],
							(gardner3Rule5[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		}
	} else if (pp->neighbors == 4) {
		if (paths == 2) {
			ruleField = 2; /* pp->neighbors - paths */
			if (invMaskChoice4[aworm->pathMask] != -1) {
				for (j = 0; j < FIELD2_SIZE4; j++) {
					if (gardner2Rule4[j] == -1)
						return gardnerFieldRuleShort(mi, aworm, ruleField);
					if (aworm->pathMask2[j] == -1) {
						choice = field2Choice4[invMaskChoice4[aworm->pathMask]][gardner2Rule4[j]];
#ifdef DEBUG
						(void) printf(" Field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice4[aworm->pathMask],
							(gardner2Rule4[j] + 'a'), choice);
#endif
						aworm->pathMask2[j] = aworm->pathMask;
						aworm->internetRule[aworm->positions] = choice;
						if (MI_IS_VERBOSE(mi)) {
							setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
							if (pp->n == 1)
								(void) printf("field %d: mask %2d %s\n",
									ruleField, aworm->pathMask, aworm->internetRuleString);
							else
								(void) printf("worm %d, field %d: mask %2d %s\n",
									i, ruleField, aworm->pathMask, aworm->internetRuleString);
						}
						aworm->positions++;
						aworm->color = aworm->positions;
						break;
					} else if (aworm->pathMask2[j] == aworm->pathMask) {
						choice = field2Choice4[invMaskChoice4[aworm->pathMask]][gardner2Rule4[j]];
#ifdef DEBUG
						(void) printf(" field%d,%d %d [%d, %c] %d\n", ruleField, j, aworm->pathMask,
							invMaskChoice4[aworm->pathMask],
							(gardner2Rule4[j] + 'a'), choice);
#endif
						break;
					}
				}
				if (choice < 0)
					return gardnerNoChoice(aworm, choice);
			} else
				return gardnerNoPath(pp, aworm, ruleField);
		}
	}
	return choice;
}

static int
useInternetRule(ModeInfo *mi, int i)
{
	patersonstruct *pp;
	wormstruct *aworm;
	int j, choice;
	pp = &patersons[MI_SCREEN(mi)];
	aworm = &pp->worms[i];

	choice = -1;
	for (j = 2; j <= aworm->positions; j++) {
		if (aworm->state[j] == aworm->pathMask) {
			choice = aworm->internetRule[j - 1];
			break;
		}
	}
	if (choice == -1) {
		j = ++aworm->positions;
		aworm->state[j] = aworm->pathMask;
		choice = aworm->internetRule[j - 1];
#ifdef DEBUG
		(void) printf(" color %d: mask %d\n", j, aworm->pathMask);
#endif
		if (aworm->color < j - 1)
			aworm->color = j - 1;
	}
	return choice;
}

static void
free_paterson_screen(Display *display, patersonstruct *pp)
{
	if (pp == NULL) {
		return;
	}
	if (pp->field != NULL) {
		free(pp->field);
		pp->field = (unsigned char *) NULL;
	}
	if (pp->worms != NULL) {
		free(pp->worms);
		pp->worms = (wormstruct *) NULL;
	}
	pp = NULL;
}

ENTRYPOINT void
free_paterson(ModeInfo * mi)
{
	free_paterson_screen(MI_DISPLAY(mi), &patersons[MI_SCREEN(mi)]);
}

ENTRYPOINT void
init_paterson(ModeInfo * mi)
{
	Display *display = MI_DISPLAY(mi);
	int size = MI_SIZE(mi);
	patersonstruct *pp;
	int col, row, dir;
	int i, j;

	MI_INIT(mi, patersons);
	pp = &patersons[MI_SCREEN(mi)];

	pp->generation = 0;
	pp->n = MI_COUNT(mi);
	if (pp->n < -MINWORMS) {
		/* if pp->n is random ... the size can change */
		if (pp->worms != NULL) {
			free(pp->worms);
			pp->worms = (wormstruct *) NULL;
		}
		/*pp->n = NRAND(-pp->n - MINWORMS + 1) + MINWORMS;*/
		/* skew so that 1 is more likely */
		pp->n = -pp->n - (int) (sqrt(NRAND(pp->n *pp->n - 1))) - 1 +
			MINWORMS;
	} else if (pp->n < MINWORMS)
		pp->n = MINWORMS;

	if (MI_IS_FULLRANDOM(mi)) {
		pp->vertical = (Bool) (LRAND() & 1);
	} else {
		pp->vertical = vertical;
	}
	pp->width = MI_WIDTH(mi);
	pp->height = MI_HEIGHT(mi);

	if (neighbors == 3 || neighbors == 4
			|| neighbors == 5 || neighbors == 6)
		pp->neighbors = neighbors;
	else if (!NRAND(100))
		pp->neighbors = 3; /* kind of silly  :) */
	else if (!NRAND(50))
		pp->neighbors = 4;
	else if (NRAND(2))
		pp->neighbors = 5;
	else
		pp->neighbors = 6;
	if (pp->neighbors == 3 || pp->neighbors == 6) {
		int nccols, ncrows;

		pp->polygon = 6;
		if (pp->vertical) {
			pp->height = MI_WIDTH(mi);
			pp->width = MI_HEIGHT(mi);
		}
		if (pp->width < 8)
			pp->width = 8;
		if (pp->height < 8)
			pp->height = 8;
		if (size < -MINSIZE) {
			pp->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(pp->width, pp->height) /
				MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
			if (pp->ys < MINRANDOMSIZE)
				pp->ys = MIN(MINRANDOMSIZE,
					MAX(MINSIZE, MIN(pp->width, pp->height) / MINGRIDSIZE));
		} else if (size < MINSIZE) {
			if (!size)
				pp->ys = MAX(MINSIZE, MIN(pp->width, pp->height) / MINGRIDSIZE);
			else
				pp->ys = MINSIZE;
		} else
			pp->ys = MIN(size, MAX(MINSIZE, MIN(pp->width, pp->height) /
				MINGRIDSIZE));
		pp->xs = pp->ys;
		nccols = MAX(pp->width / pp->xs - 2, 2);
		ncrows = MAX(pp->height / pp->ys - 1, 4);
		pp->ncols = nccols / 2;
		pp->nrows = 2 * (ncrows / 4);
		pp->xb = (pp->width - pp->xs * nccols) / 2 + pp->xs / 2;
		pp->yb = (pp->height - pp->ys * (ncrows / 2) * 2) / 2 + pp->ys - 2;
	} else if (pp->neighbors == 4 || pp->neighbors == 5) {
		pp->polygon = 4;
		if (size < -MINSIZE) {
			pp->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(pp->width, pp->height) /
				MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
			if (pp->ys < MINRANDOMSIZE)
				pp->ys = MIN(MINRANDOMSIZE,
					MAX(MINSIZE, MIN(pp->width, pp->height) / MINGRIDSIZE));
		} else if (size < MINSIZE) {
			if (!size)
				pp->ys = MAX(MINSIZE, MIN(pp->width, pp->height) / MINGRIDSIZE);
			else
				pp->ys = MINSIZE;
		} else
			pp->ys = MIN(size, MAX(MINSIZE, MIN(pp->width, pp->height) /
				MINGRIDSIZE));
		pp->xs = pp->ys;
		pp->ncols = MAX(pp->width / pp->xs, 2);
		pp->nrows = MAX(pp->height / pp->ys, 2);
		if (pp->neighbors == 5) {
			/* make even */
			if ((pp->ncols & 1) == 1)
				pp->ncols--;
			if ((pp->nrows & 1) == 1)
				pp->nrows--;
		}
		pp->xb = (pp->width - pp->xs * pp->ncols) / 2;
		pp->yb = (pp->height - pp->ys * pp->nrows) / 2;
	}
	for (i = 0; i < RULE_SIZE; i++)
		internetRule[i] = -1;
	if (rule && rule[0] != '0') {
		gardner = gardnerRule(rule);
		if (gardner) {
			if (pp->neighbors == 6) {
				setGardnerRule6(rule, &gardner1Rule,
					&(gardner2Rule6[0]), &(gardner3Rule6[0]), &(gardner4Rule6[0]));
				setGardnerRule6String(pp->gardnerRuleLabel, NULL, NULL, NULL,
					gardner1Rule, gardner2Rule6, gardner3Rule6, gardner4Rule6);
				if (MI_IS_VERBOSE(mi))
					(void) printf("%s\n", pp->gardnerRuleLabel);
			} else if (pp->neighbors == 5) {
				setGardnerRule5(rule, &gardner1Rule,
					&(gardner2Rule5[0]), &(gardner3Rule5[0]));
				setGardnerRule5String(pp->gardnerRuleLabel, NULL, NULL,
					gardner1Rule, gardner2Rule5, gardner3Rule5);
				if (MI_IS_VERBOSE(mi))
					(void) printf("%s\n", pp->gardnerRuleLabel);
			} else if (pp->neighbors == 4) {
				setGardnerRule4(rule, &gardner1Rule,
					&(gardner2Rule4[0]));
				setGardnerRule4String(pp->gardnerRuleLabel, NULL,
					gardner1Rule, gardner2Rule4);
				if (MI_IS_VERBOSE(mi))
					(void) printf("%s\n", pp->gardnerRuleLabel);
			} else if (pp->neighbors == 3) {
				setGardnerRule3(rule, &gardner1Rule);
				setGardnerRule3String(pp->gardnerRuleLabel,
					gardner1Rule);
				if (MI_IS_VERBOSE(mi))
					(void) printf("%s\n", pp->gardnerRuleLabel);
			} else {
				(void) printf("no Gardner Rule\n");
			}
		} else {
			setInternetRule(rule, &(internetRule[0]));
			setInternetRuleString(pp->internetRuleLabel, internetRule);
			if (MI_IS_VERBOSE(mi))
				(void) printf("%s\n", pp->internetRuleLabel);
		}
	} else {
		gardner = True;
		if (pp->neighbors == 3) {
			/*internetRule[0] = 1;
			setInternetRuleString(pp->internetRuleLabel, internetRule);
			if (MI_IS_VERBOSE(mi))
				(void) printf("%s\n", pp->internetRuleLabel);*/
			createGardnerRule3(pp, &newGardnerRule[0]);
			setGardnerRule3(newGardnerRule, &gardner1Rule);
			if (pp->n == 1)
				setGardnerRule3String(pp->gardnerRuleLabel,
					gardner1Rule);
			setGardnerRule3String(pp->gardnerRuleString,
				gardner1Rule);
		} else if (pp->neighbors == 4) {
			/*internetRule[0] = 1;
			internetRule[1] = NRAND(2) * 3;
			setInternetRuleString(pp->internetRuleLabel, internetRule);
			if (MI_IS_VERBOSE(mi))
				(void) printf("%s\n", pp->internetRuleLabel);*/
			createGardnerRule4(pp, &newGardnerRule[0]);
			setGardnerRule4(newGardnerRule, &gardner1Rule,
				&(gardner2Rule4[0]));
			if (pp->n == 1)
				setGardnerRule4String(pp->gardnerRuleLabel, NULL,
					gardner1Rule, gardner2Rule4);
			setGardnerRule4String(pp->gardnerRuleString, NULL,
				gardner1Rule, gardner2Rule4);
		} else if (pp->neighbors == 6) {
			/* if an annoying rule try one more time */
			if (createGardnerRule6(pp, &newGardnerRule[0]))
				createGardnerRule6(pp, &newGardnerRule[0]);
			setGardnerRule6(newGardnerRule, &gardner1Rule,
				&(gardner2Rule6[0]), &(gardner3Rule6[0]), &(gardner4Rule6[0]));
			if (pp->n == 1)
				setGardnerRule6String(pp->gardnerRuleLabel, NULL, NULL, NULL,
					gardner1Rule, gardner2Rule6, gardner3Rule6, gardner4Rule6);
			setGardnerRule6String(pp->gardnerRuleString, NULL, NULL, NULL,
				gardner1Rule, gardner2Rule6, gardner3Rule6, gardner4Rule6);
		} else /*if (pp->neighbors == 5)*/ {
			createGardnerRule5(pp, &newGardnerRule[0]);
			setGardnerRule5(newGardnerRule, &gardner1Rule,
				&(gardner2Rule5[0]), &(gardner3Rule5[0]));
			if (pp->n == 1)
				setGardnerRule5String(pp->gardnerRuleLabel, NULL, NULL,
					gardner1Rule, gardner2Rule5, gardner3Rule5);
			setGardnerRule5String(pp->gardnerRuleString, NULL, NULL,
				gardner1Rule, gardner2Rule5, gardner3Rule5);
		}
		if (MI_IS_VERBOSE(mi)) {
			(void) printf("%s\n", pp->gardnerRuleString);
		}
	}

	XSetLineAttributes(display, MI_GC(mi), 1, LineSolid, CapNotLast, JoinMiter);
	MI_CLEARWINDOW(mi);
	pp->painted = False;

	pp->labelOffsetX = NRAND(8);
	pp->labelOffsetY = NRAND(8);
	/*parseRule(mi);*/
	if (pp->worms == NULL) {
		if ((pp->worms = (wormstruct *) malloc(pp->n * sizeof (wormstruct))) ==
				NULL) {
			free_paterson(mi);
			return;
		}
	}
	if (pp->field != NULL)
		free(pp->field);
	if ((pp->field = (unsigned char *) calloc(pp->ncols * pp->nrows,
			sizeof (unsigned char))) == NULL) {
		free_paterson(mi);
		return;
	}

	row = pp->nrows / 2;
	col = pp->ncols / 2;
	if (pp->polygon == 5) {
		int mod = (col + 2 * (row - 1)) % 4;
		dir = 0;
		col -= mod + NRAND(4);
	}
#ifdef DEBUG
#define SIMPLIFY 1
#endif
	pp->left = NRAND(2);
#ifdef SIMPLIFY
	pp->left = True;
#endif
	/* Have them all start in the same spot, why not? */
	for (i = 0; i < pp->n; i++) {
		wormstruct *aworm;
		if (pp->neighbors == 3)
			dir = NRAND(2 * pp->neighbors) * ANGLES / (2 * pp->neighbors);
		else if (pp->neighbors == 5)
			dir = NRAND(4) * ANGLES / 4;
		else
			dir = NRAND(pp->neighbors) * ANGLES / pp->neighbors;
#ifdef SIMPLIFY
		if (pp->neighbors == 3)
			dir = ANGLES / 2 + ANGLES / (2 * pp->neighbors);
		else if (pp->neighbors == 5)
			dir = ANGLES / 2;
		else
			dir = ANGLES / 2 + ANGLES / pp->neighbors;
#endif
		aworm = &pp->worms[i];
#ifdef SIMPLIFY
		aworm->newcol = col;
		aworm->newrow = row;
#else
		aworm->newcol = col + NRAND(2) - 1;
		aworm->newrow = row + NRAND(2) - 1;
#endif
		aworm->oldcol = 0;
		aworm->oldrow = 0;
		aworm->direction = dir;
		aworm->color = 0;
		aworm->alive = True;
		aworm->positions = 1;
		for (j = 0; j < RULE_SIZE; j++)
			aworm->internetRule[j] = internetRule[j];
		for (j = 0; j < FIELD2_SIZE6; j++)
			aworm->pathMask2[j] = -1;
		for (j = 0; j < FIELD3_SIZE6; j++)
			aworm->pathMask3[j] = -1;
		for (j = 0; j < FIELD4_SIZE6; j++)
			aworm->pathMask4[j] = -1;
	}
}

ENTRYPOINT void
draw_paterson(ModeInfo * mi)
{
	wormstruct *aworm;
	int i, fieldPos, clew = 0;
	patersonstruct *pp;

	if (patersons == NULL)
		return;
	pp = &patersons[MI_SCREEN(mi)];
	if (pp->worms == NULL)
		return;
#ifdef STANDALONE
	if (pp->eraser) {
		pp->eraser = erase_window (MI_DISPLAY(mi), MI_WINDOW(mi), pp->eraser);
		return;
	}
#endif

	MI_IS_DRAWN(mi) = True;
	pp->painted = True;
	for (i = 0; i < pp->n; i++) {
		aworm = &pp->worms[i];
		if (aworm->alive)
			clew++;
	}
	if (label) {
		int size = MAX(MIN(MI_WIDTH(mi), MI_HEIGHT(mi)) - 1, 1);

		if (size >= 10 * FONT_WIDTH) {
			/* hard code these to corners */
			XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
				MI_WHITE_PIXEL(mi));
			XDrawString(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				16 + pp->labelOffsetX,
				16 + pp->labelOffsetY + FONT_HEIGHT,
				pp->internetRuleLabel, strlen(pp->internetRuleLabel));
			XDrawString(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
				16 + pp->labelOffsetX, MI_HEIGHT(mi) - 16 -
				pp->labelOffsetY - FONT_HEIGHT / 2,
				pp->gardnerRuleLabel, strlen(pp->gardnerRuleLabel));
		}
	}
	if (clew == 0) {
		if (pp->sleepCount > SLEEP_COUNT) {
			init_paterson(mi);
			return;
		}
		pp->sleepCount++;
		return;
	}
	for (i = 0; i < pp->n; i++) {
		int col, row, altcol, altrow;
		int defaultAngle, choice, discreteDir, otherDir, oldDir, paths;

		aworm = &pp->worms[i];
		if (!aworm->alive)
			continue;
		if ((pp->neighbors & 1) == 1)
			defaultAngle = ANGLES / (2 * pp->neighbors);
		else
			defaultAngle = ANGLES / pp->neighbors;
		col = aworm->newcol;
		row = aworm->newrow;
		fieldPos = col + row * pp->ncols;
		if (pp->neighbors == 5) {
			oldDir = toDiscreteDir8(aworm->direction);
			if (aworm->oldcol == 0 && aworm->oldrow == 0)
				oldDir = -1;
		} else
			oldDir = aworm->direction / defaultAngle;
		paths = getPathsRemaining(pp, fieldPos, oldDir, &(aworm->pathMask));
		if (paths >= pp->neighbors - 1) { /* WHITE */
			if (gardner) {
				aworm->internetRule[0] = gardner1Rule + 1 - (pp->neighbors & 1);
			}
			choice = aworm->internetRule[0];
#ifdef DEBUG
			(void) printf(" White: defAngle=%d, choice=%d, mask=%d\n",
				defaultAngle, choice, aworm->pathMask);
#endif
		} else if (paths == 1) {
			if (pp->neighbors == 5)
				choice = deshift5(col, row, oldDir, aworm->pathMask);
			else
				choice = deshift(aworm->pathMask);
#ifdef DEBUG
			(void) printf(" ONLY 1 mask=%d %d\n", aworm->pathMask, choice);
#endif
		} else if (paths == 0) {
			aworm->alive = False;
			if (pp->neighbors == 6) {
				setGardnerRule6String(pp->gardnerRuleString,
					aworm->pathMask2, aworm->pathMask3, aworm->pathMask4,
					gardner1Rule, gardner2Rule6, gardner3Rule6, gardner4Rule6);
			} else if (pp->neighbors == 4) {
				setGardnerRule4String(pp->gardnerRuleString,
					aworm->pathMask2,
					gardner1Rule, gardner2Rule4);
			} else if (pp->neighbors == 3) {
				setGardnerRule3String(pp->gardnerRuleString,
					gardner1Rule);
			} else /*if (pp->neighbors == 5)*/ {
				setGardnerRule5String(pp->gardnerRuleString,
					aworm->pathMask2, aworm->pathMask3,
					gardner1Rule, gardner2Rule5, gardner3Rule5);
			}
			setInternetRuleString(aworm->internetRuleString, aworm->internetRule);
			if (pp->n == 1 && gardner) {
				setInternetRuleString(pp->internetRuleLabel, aworm->internetRule);
				/* gardner rule could be shorter now */
				/*(void) strcpy(pp->gardnerRuleLabel, pp->gardnerRuleString);*/
			}
			if (MI_IS_VERBOSE(mi)) {
				(void) printf("Death %s %s\n",
					pp->gardnerRuleString, aworm->internetRuleString);
			}
			return;
		} else if (gardner)
			choice = useGardnerRule(mi, paths, i, oldDir);
		else
			choice = useInternetRule(mi, i);
		if (choice == -1) {
			aworm->alive = False;
			if (MI_IS_VERBOSE(mi)) {
				(void) printf("Death no choice %s %s\n",
					pp->gardnerRuleString, aworm->internetRuleString);
			}
			return;
		}
		aworm->direction = nextAngle(pp->neighbors, pp->n, choice, aworm->direction, col, row);
		if (aworm->direction == -1) {
			aworm->alive = False;
			if (MI_IS_VERBOSE(mi)) {
				(void) printf("Death reversed %s %s\n",
					pp->gardnerRuleString, aworm->internetRuleString);
			}
			return;
		}
		nextPosition(pp, ((pp->left) ? changeHandDir(aworm->direction) : aworm->direction),
			&col, &row, &altcol, &altrow);
		if (pp->neighbors == 5) {
			discreteDir = toDiscreteDir8(aworm->direction);
			otherDir = (discreteDir + 4) % 8;
		} else {
			discreteDir = aworm->direction / defaultAngle;
			otherDir = aworm->direction / defaultAngle;
		}
#ifdef DEBUG
		(void) printf(" discreteDir=%d, choice=%d, oldcol=%d, oldrow=%d, newcol=%d, newrow=%d, gen=%d\n",
			discreteDir, choice, aworm->oldcol, aworm->oldrow, aworm->newcol, aworm->newrow, pp->generation);
#endif
#ifdef DEBUG
		(void) printf(" col=%d, row=%d, paths=%d, newdirection=%d, choice=%d\n",
			col, row, paths, aworm->direction, choice);
#endif
		aworm->oldcol = aworm->newcol;
		aworm->oldrow = aworm->newrow;
		aworm->newcol = col;
		aworm->newrow = row;
		aworm->newaltcol = altcol;
		aworm->newaltrow = altrow;
		patersonWorm(mi, aworm, discreteDir, otherDir);
		/* Translate relative direction to actual direction */
	}
	if (++pp->generation > MI_CYCLES(mi)) {
#ifdef STANDALONE
		pp->eraser = erase_window (MI_DISPLAY(mi), MI_WINDOW(mi), pp->eraser);
#endif
		init_paterson(mi);
	}
}

ENTRYPOINT void
release_paterson(ModeInfo * mi)
{
	if (patersons != NULL) {
		int	screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_paterson_screen(MI_DISPLAY(mi), &patersons[screen]);
		free(patersons);
		patersons = (patersonstruct *) NULL;
	}
}

XSCREENSAVER_MODULE ("Paterson", paterson)

#endif /* MODE_paterson */
