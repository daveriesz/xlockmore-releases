#ifndef lint
static char sccsid[] = "@(#)bug.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * bug.c - Michael Palmiter's simulated evolution for xlock,
 *           the X Window System lockscreen.
 *
 * Copyright (c) 1995 by David Bagley.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 24-Aug-95: Coded from A.K. Dewdney's, "Computer Recreations", Scientific
 *            American Magazine" May 1989 pp138-141 and Sept 1989 p 183.
 *            also used wator.c as a guide.
 */

#include <math.h>
#include "xlock.h"

#define	MAXROWS	180
#define MAXCOLS	170
#define BACTERIA 0
#define BUG 1
#define ORIENTS 6

#define MAXENERGY 1500		/* Max energy storage */
#define INITBACTERIA 400	/* Initial bacteria */
#define INITENERGY 400		/* Initial energy */
#define FOODPERCYCLE 1
#define BACTERIAENERGY 40
#define STRONG 1000		/* Min energy to breed */
#define MATURE 800		/* Breeding time of bugs */
#define MAXGENE 6
#define MINGENE (-MAXGENE)
#define EDENWIDTH 40
#define EDENHEIGHT 39

/* Relative bug moves */
/* Forward, Right, Hard Right, Reverse, Hard Left, Left */

/* Compass bug moves */
#define NE 0
#define E 1
#define SE 2
#define SW 3
#define W 4
#define NW 5

ModeSpecOpt bug_opts =
{0, NULL, NULL, NULL};

/* Bug data */
typedef struct {
	char        direction;
	int         age, energy;
	unsigned long color;
	int         col, row;
	int         gene[ORIENTS];
	double      gene_prob[ORIENTS];
} bugstruct;

/* Doublely linked list */
typedef struct _BugList {
	bugstruct   info;
	struct _BugList *previous, *next;
} BugList;

typedef struct {
	int         initialized;
	int         n;		/* Number of bugs */
	int         eden;	/* Does the garden exist? */
	int         generation;
	int         nhcols, nhrows;
	int         nccols, ncrows;
	int         width, height;
	int         edenstartx, edenstarty;
	int         xs, ys, xb, yb;
	BugList    *currbug, *babybug, *lastbug, *firstbug, *lasttemp, *firsttemp;
	BugList    *arr[MAXCOLS * MAXROWS];	/* 0=empty or pts to a bug */
	char        bacteria[MAXCOLS * MAXROWS];	/* 0=empty or food */
} bugfarmstruct;

static double genexp[MAXGENE - MINGENE + 1];
static bugfarmstruct bugfarms[MAXSCREENS];

#if 0
static void
cart2hex(cx, cy, hx, hy)
	int         cx, cy, *hx, *hy;
{
	*hy = cy / 2;
	*hx = (cx + 1) / 2 - 1;
}

#endif

static void
hex2cart(int hx, int hy, int *cx, int *cy)
{
	*cy = hy * 2 + 1;
	*cx = 2 * hx + 1 + !(hy & 1);
}

/* hexagon compass move */
static char
hexcmove(int hx, int hy, int dir, int *nhx, int *nhy)
{
	bugfarmstruct *bp = &bugfarms[screen];

	switch (dir) {
		case NE:
			*nhy = hy - 1;
			*nhx = hx + (int) !(hy & 1);
			return (*nhy >= 0 && *nhx < bp->nhcols - !(*nhy & 1));
		case E:
			*nhy = hy;
			*nhx = hx + 1;
			return (*nhx < bp->nhcols - !(*nhy & 1));
		case SE:
			*nhy = hy + 1;
			*nhx = hx + (int) !(hy & 1);
			return (*nhy < bp->nhrows && *nhx < bp->nhcols - !(*nhy & 1));
		case SW:
			*nhy = hy + 1;
			*nhx = hx - (int) (hy & 1);
			return (*nhy < bp->nhrows && *nhx >= 0);
		case W:
			*nhy = hy;
			*nhx = hx - 1;
			return (*nhx >= 0);
		case NW:
			*nhy = hy - 1;
			*nhx = hx - (int) (hy & 1);
			return (*nhy >= 0 && *nhx >= 0);
		default:
			return 0;
	}
}

static int
neighbor(int hcol, int hrow)
{
	bugfarmstruct *bp = &bugfarms[screen];
	int         colrow = hcol + hrow * MAXCOLS;

	if ((hrow > 0) && bp->arr[colrow - MAXCOLS])
		return True;
	if ((hrow < bp->nhrows - 1) && bp->arr[colrow + MAXCOLS])
		return True;
	if (hcol > 0) {
		if (bp->arr[colrow - 1])
			return True;
		if (hrow & 1) {
			if ((hrow > 0) && bp->arr[colrow - 1 - MAXCOLS])
				return True;
			if ((hrow < bp->nhrows - 1) && bp->arr[colrow - 1 + MAXCOLS])
				return True;
		}
	}
	if (hcol < bp->nhcols - 1) {
		if (bp->arr[colrow + 1])
			return True;
		if (!(hrow & 1)) {
			if ((hrow > 0) && bp->arr[colrow + 1 - MAXCOLS])
				return True;
			if ((hrow < bp->nhrows - 1) && bp->arr[colrow + 1 + MAXCOLS])
				return True;
		}
	}
	return False;
}

static void
drawabacterium(ModeInfo * mi, int hcol, int hrow, char color)
{
	bugfarmstruct *bp = &bugfarms[screen];
	int         ccol, crow;

	if (color)
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	else
		XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	hex2cart(hcol, hrow, &ccol, &crow);
	XFillRectangle(dsp, MI_WINDOW(mi), Scr[screen].gc,
		       bp->xb + bp->xs * ccol, bp->yb + bp->ys * crow,
		       bp->xs, bp->ys);
}

static void
drawabug(ModeInfo * mi, int hcol, int hrow, long unsigned int color)
{
	bugfarmstruct *bp = &bugfarms[screen];
	int         ccol, crow;

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[color]);
	else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	hex2cart(hcol, hrow, &ccol, &crow);
	XFillRectangle(dsp, MI_WINDOW(mi), Scr[screen].gc,
		  bp->xb + bp->xs * (ccol - 1), bp->yb + bp->ys * (crow - 1),
		       3 * bp->xs, 3 * bp->ys);
}


static void
eraseabug(ModeInfo * mi, int hcol, int hrow)
{
	bugfarmstruct *bp = &bugfarms[screen];
	int         ccol, crow;

	XSetForeground(MI_DISPLAY(mi), Scr[screen].gc, BlackPixel(dsp, screen));
	hex2cart(hcol, hrow, &ccol, &crow);
	XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), Scr[screen].gc,
		  bp->xb + bp->xs * (ccol - 1), bp->yb + bp->ys * (crow - 1),
		       3 * bp->xs, 3 * bp->ys);
}

static void
init_buglist(void)
{
	bugfarmstruct *bp = &bugfarms[screen];

	/* Waste some space at the beginning and end of list
	   so we do not have to complicated checks against falling off the ends. */
	bp->lastbug = (BugList *) malloc(sizeof (BugList));
	bp->firstbug = (BugList *) malloc(sizeof (BugList));
	bp->firstbug->previous = bp->lastbug->next = NULL;
	bp->firstbug->next = bp->lastbug->previous = NULL;
	bp->firstbug->next = bp->lastbug;
	bp->lastbug->previous = bp->firstbug;

	bp->lasttemp = (BugList *) malloc(sizeof (BugList));
	bp->firsttemp = (BugList *) malloc(sizeof (BugList));
	bp->firsttemp->previous = bp->lasttemp->next = NULL;
	bp->firsttemp->next = bp->lasttemp->previous = NULL;
	bp->firsttemp->next = bp->lasttemp;
	bp->lasttemp->previous = bp->firsttemp;
}

static void
addto_buglist(bugstruct info)
{
	bugfarmstruct *bp = &bugfarms[screen];

	bp->currbug = (BugList *) malloc(sizeof (BugList));
	bp->lastbug->previous->next = bp->currbug;
	bp->currbug->previous = bp->lastbug->previous;
	bp->currbug->next = bp->lastbug;
	bp->lastbug->previous = bp->currbug;
	bp->currbug->info = info;
}

static void
removefrom_buglist(BugList * ptr)
{
	bugfarmstruct *bp = &bugfarms[screen];

	ptr->previous->next = ptr->next;
	ptr->next->previous = ptr->previous;
	bp->arr[ptr->info.col + ptr->info.row * MAXCOLS] = 0;
	(void) free((void *) ptr);
}

static void
dupin_buglist(void)
{
	bugfarmstruct *bp = &bugfarms[screen];
	BugList    *temp;

	temp = (BugList *) malloc(sizeof (BugList));
	temp->previous = bp->babybug;
	temp->next = bp->babybug->next;
	bp->babybug->next = temp;
	temp->next->previous = temp;
	temp->info = bp->babybug->info;
	bp->babybug = temp;
}

/* 
 * new bug at end of list, this rotates who goes first, young bugs go last
 * this most likely will not change the feel to any real degree
 */
static void
cutfrom_buglist(void)
{
	bugfarmstruct *bp = &bugfarms[screen];

	bp->babybug = bp->currbug;
	bp->currbug = bp->currbug->previous;
	bp->currbug->next = bp->babybug->next;
	bp->babybug->next->previous = bp->currbug;
	bp->babybug->next = bp->lasttemp;
	bp->babybug->previous = bp->lasttemp->previous;
	bp->babybug->previous->next = bp->babybug;
	bp->babybug->next->previous = bp->babybug;
}

static void
reattach_buglist(void)
{
	bugfarmstruct *bp = &bugfarms[screen];

	bp->currbug = bp->lastbug->previous;
	bp->currbug->next = bp->firsttemp->next;
	bp->currbug->next->previous = bp->currbug;
	bp->lastbug->previous = bp->lasttemp->previous;
	bp->lasttemp->previous->next = bp->lastbug;
	bp->lasttemp->previous = bp->firsttemp;
	bp->firsttemp->next = bp->lasttemp;
}

static void
flush_buglist(void)
{
	bugfarmstruct *bp = &bugfarms[screen];

	while (bp->lastbug->previous != bp->firstbug) {
		bp->currbug = bp->lastbug->previous;
		bp->currbug->previous->next = bp->lastbug;
		bp->lastbug->previous = bp->currbug->previous;
		bp->arr[bp->currbug->info.col + bp->currbug->info.row * MAXCOLS] = 0;
		(void) free((void *) bp->currbug);
	}

	while (bp->lasttemp->previous != bp->firsttemp) {
		bp->currbug = bp->lasttemp->previous;
		bp->currbug->previous->next = bp->lasttemp;
		bp->lasttemp->previous = bp->currbug->previous;
		bp->arr[bp->currbug->info.col + bp->currbug->info.row * MAXCOLS] = 0;
		(void) free((void *) bp->currbug);
	}
}

static int
dirbug(bugstruct * info)
{
	double      sum = 0.0, prob;
	int         i;

	prob = (double) LRAND() / MAXRAND;
	for (i = 0; i < ORIENTS; i++) {
		sum += info->gene_prob[i];
		if (prob < sum)
			return i;
	}
	return ORIENTS - 1;	/* Could miss due to rounding */
}

static void
mutatebug(bugstruct * info)
{
	double      sum = 0.0;
	int         gene;

	gene = LRAND() % ORIENTS;
	if (LRAND() & 1) {
		if (info->gene[gene] == MAXGENE)
			return;
		info->gene[gene]++;
	} else {
		if (info->gene[gene] == MINGENE)
			return;
		info->gene[gene]--;
	}
	for (gene = 0; gene < ORIENTS; gene++)
		sum += genexp[info->gene[gene] + MAXGENE];
	for (gene = 0; gene < ORIENTS; gene++)
		info->gene_prob[gene] = genexp[info->gene[gene] + MAXGENE] / sum;
}

static void
makebacteria(ModeInfo * mi, int n, int startx, int starty, int width, int height)
{
	int         i = 0, j = 0, hcol, hrow, colrow;
	bugfarmstruct *bp = &bugfarms[screen];

	/* Make bacteria but if can't, don't loop forever */
	while (i < n && j < 2 * n) {
		hrow = LRAND() % height + starty;
		hcol = LRAND() % (width - (!(hrow & 1))) + startx;
		colrow = hcol + hrow * MAXCOLS;
		j++;
		if (!bp->arr[colrow] && !bp->bacteria[colrow]) {
			i++;
			bp->bacteria[colrow] = True;
			drawabacterium(mi, hcol, hrow, True);
		}
	}
}

void
init_bug(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i = 0, j = 0, hcol, hrow, gene, colrow;
	double      sum;
	bugfarmstruct *bp = &bugfarms[screen];
	bugstruct   info;

	bp->generation = 0;
	if (!bp->initialized) {	/* Genesis */
		bp->initialized = 1;
		/* Set up what will be a 'triply' linked list.
		   doubly linked list, doubly linked to an array */
		(void) memset((char *) bp->arr, 0, sizeof (bp->arr));
		init_buglist();
		genexp[MAXGENE] = 1;
		for (i = 1; i <= MAXGENE; i++) {
			genexp[MAXGENE + i] = genexp[MAXGENE + i - 1] * M_E;
			genexp[MAXGENE - i] = 1.0 / genexp[MAXGENE + i];
		}
	} else			/* Apocolypse */
		flush_buglist();
	(void) memset((char *) bp->bacteria, 0, sizeof (bp->bacteria));

	bp->width = MI_WIN_WIDTH(mi) - 2;
	bp->height = MI_WIN_HEIGHT(mi);
	if (bp->width < 2)
		bp->width = 2;
	bp->nccols = min(bp->width, 2 * MAXCOLS);
	bp->ncrows = min(bp->height, 2 * MAXROWS);
	bp->nhcols = bp->nccols / 2;
	bp->nhrows = bp->ncrows / 2;
	if (!(bp->nhrows & 1)) {	/* Force odd rows  */
		bp->ncrows -= 2;
		bp->nhrows--;
	}
	bp->xs = bp->width / bp->nccols;
	bp->ys = bp->height / bp->ncrows;
	bp->xb = (bp->width - bp->xs * bp->nccols) / 2;
	bp->yb = (bp->height - bp->ys * bp->ncrows) / 2;

	if (bp->nhcols > 3 * EDENWIDTH && bp->nhrows > 3 * EDENHEIGHT) {
		bp->eden = 1;
		bp->edenstartx = LRAND() % (bp->nhcols - 3 * EDENWIDTH) + EDENWIDTH;
		bp->edenstarty = LRAND() % (bp->nhrows - 3 * EDENHEIGHT) + EDENHEIGHT;
		if (bp->edenstarty & 1)
			bp->edenstarty++;
	}
	/* Play G-d with these numbers */
	bp->n = MI_BATCHCOUNT(mi);
	if (bp->n < 1)
		bp->n = 1;

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, bp->width, bp->height);

	/* Make bugs but if can't, don't loop forever */
	i = 0, j = 0;
	while (i < bp->n && j < 2 * bp->n) {
		hrow = LRAND() % bp->nhrows;
		hcol = LRAND() % (bp->nhcols - (!(hrow & 1)));
		colrow = hcol + hrow * MAXCOLS;
		j++;
		if (!bp->arr[colrow] && !neighbor(hcol, hrow)) {
			i++;
			info.age = 0;
			info.energy = INITENERGY;
			info.direction = LRAND() % ORIENTS;
			for (gene = 0; gene < ORIENTS; gene++)
#if 0				/* Some creationalism, may create gliders or twirlers */
				do {
					double      temp = (double) LRAND() / MAXRAND;

					info.gene[gene] = ((int) (1.0 / (1.0 - temp * temp)) - 1) *
						((LRAND() & 1) ? -1 : 1);
				} while (info.gene[gene] > MAXGENE / 2 ||
					 info.gene[gene] < MINGENE / 2);
#else /* Jitterbugs, evolve or die */
				info.gene[gene] = 0;
#endif
			sum = 0;
			for (gene = 0; gene < ORIENTS; gene++)
				sum += genexp[info.gene[gene] + MAXGENE];
			for (gene = 0; gene < ORIENTS; gene++)
				info.gene_prob[gene] = genexp[info.gene[gene] + MAXGENE] / sum;
			if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
				info.color = LRAND() % Scr[screen].npixels;
			else
				info.color = 0;
			info.col = hcol;
			info.row = hrow;
			addto_buglist(info);
			bp->arr[colrow] = bp->currbug;
			drawabug(mi, hcol, hrow, bp->currbug->info.color);
		}
	}
	makebacteria(mi, bp->nhcols * bp->nhrows / 2, 0, 0, bp->nhcols, bp->nhrows);
	if (bp->eden)
		makebacteria(mi, EDENWIDTH * EDENHEIGHT / 2,
		      bp->edenstartx, bp->edenstarty, EDENWIDTH, EDENHEIGHT);
}

#define ENOUGH 16
void
draw_bug(ModeInfo * mi)
{
	int         hcol, hrow, nhcol, nhrow, colrow, ncolrow, absdir, try;
	bugfarmstruct *bp = &bugfarms[screen];

	bp->currbug = bp->firstbug->next;
	while (bp->currbug != bp->lastbug) {
		hcol = bp->currbug->info.col;
		hrow = bp->currbug->info.row;
		colrow = hcol + hrow * MAXCOLS;
		if (bp->currbug->info.energy-- < 0) {	/* Time to die, Bug */
			/* back up one or else in void */
			bp->currbug = bp->currbug->previous;
			removefrom_buglist(bp->arr[colrow]);
			bp->arr[colrow] = 0;
			eraseabug(mi, hcol, hrow);
			bp->n--;
		} else {	/* try to move */
			bp->arr[colrow] = 0;	/* Don't want neighbor to detect itself */
			try = 0;
			do {
				if (try++ > ENOUGH) {
					break;
				}
				absdir = (bp->currbug->info.direction +
				       dirbug(&bp->currbug->info)) % ORIENTS;
			} while (!hexcmove(hcol, hrow, absdir, &nhcol, &nhrow) ||
				 neighbor(nhcol, nhrow));
			bp->currbug->info.age++;
			bp->currbug->info.energy--;
			if (try <= ENOUGH) {
				ncolrow = nhcol + nhrow * MAXCOLS;
				if (bp->bacteria[ncolrow]) {
					bp->currbug->info.energy += BACTERIAENERGY;
					bp->bacteria[ncolrow] = 0;
					if (bp->currbug->info.energy > MAXENERGY)
						bp->currbug->info.energy = MAXENERGY;
				}
				bp->currbug->info.col = nhcol;
				bp->currbug->info.row = nhrow;
				bp->currbug->info.direction = absdir;
				bp->arr[ncolrow] = bp->currbug;
				if (bp->currbug->info.energy > STRONG &&
				    bp->currbug->info.age > MATURE) {	/* breed */
					drawabug(mi, nhcol, nhrow, bp->currbug->info.color);
					cutfrom_buglist();	/* This rotates out who goes first */
					bp->babybug->info.age = 0;
					bp->babybug->info.energy = INITENERGY;
					dupin_buglist();
					mutatebug(&bp->babybug->previous->info);
					mutatebug(&bp->babybug->info);
					bp->arr[colrow] = bp->babybug;
					bp->babybug->info.col = hcol;
					bp->babybug->info.row = hrow;
					bp->n++;
				} else {
					eraseabug(mi, hcol, hrow);
					drawabug(mi, nhcol, nhrow, bp->currbug->info.color);
				}
			} else
				bp->arr[colrow] = bp->currbug;
		}
		bp->currbug = bp->currbug->next;
	}
	reattach_buglist();
	makebacteria(mi, FOODPERCYCLE, 0, 0, bp->nhcols, bp->nhrows);
	if (bp->eden)
		makebacteria(mi, FOODPERCYCLE,
		      bp->edenstartx, bp->edenstarty, EDENWIDTH, EDENHEIGHT);
	if (!bp->n || bp->generation >= MI_CYCLES(mi))
		init_bug(mi);
	bp->generation++;
}
