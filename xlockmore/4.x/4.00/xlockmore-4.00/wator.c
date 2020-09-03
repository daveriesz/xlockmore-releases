#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)wator.c	4.00 97/01/01 xlockmore";

#endif

/*-
 * wator.c - Dewdney's Wa-Tor, water torus simulation for xlock,
 *           the X Window System lockscreen.
 *
 * Copyright (c) 1994 by David Bagley.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 29-Aug-95: Efficiency improvements.
 * 12-Dec-94: Coded from A.K. Dewdney's "The Armchair Universe, Computer 
 *            Recreations from the Pages of Scientific American Magazine"
 *            W.H. Freedman and Company, New York, 1988  (Dec 1984 and
 *            June 1985) also used life.c as a guide.
 */

#include "xlock.h"
#include "bitmaps/fish-0.xbm"
#include "bitmaps/fish-1.xbm"
#include "bitmaps/fish-2.xbm"
#include "bitmaps/fish-3.xbm"
#include "bitmaps/fish-4.xbm"
#include "bitmaps/fish-5.xbm"
#include "bitmaps/fish-6.xbm"
#include "bitmaps/fish-7.xbm"
#include "bitmaps/shark-0.xbm"
#include "bitmaps/shark-1.xbm"
#include "bitmaps/shark-2.xbm"
#include "bitmaps/shark-3.xbm"
#include "bitmaps/shark-4.xbm"
#include "bitmaps/shark-5.xbm"
#include "bitmaps/shark-6.xbm"
#include "bitmaps/shark-7.xbm"

#define FISH 0
#define SHARK 1
#define KINDS 2
#define ORIENTS 4
#define REFLECTS 2
#define BITMAPS (ORIENTS*REFLECTS*KINDS)
#define KINDBITMAPS (ORIENTS*REFLECTS)
#define MINGRIDSIZE 3		/* It is possible for the fish to take over */
#define MINSIZE 4

ModeSpecOpt wator_opts =
{0, NULL, 0, NULL, NULL};

static XImage logo[BITMAPS] =
{
	{0, 0, 0, XYBitmap, (char *) fish0_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish1_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish2_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish3_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish4_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish5_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish6_bits, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, (char *) fish7_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark0_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark1_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark2_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark3_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark4_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark5_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark6_bits, LSBFirst, 8, LSBFirst, 8, 1},
      {0, 0, 0, XYBitmap, (char *) shark7_bits, LSBFirst, 8, LSBFirst, 8, 1},
};

/* Fish and shark data */
typedef struct {
	char        kind, age, food, direction;
	unsigned long color;
	int         col, row;
} cellstruct;

/* Doublely linked list */
typedef struct _CellList {
	cellstruct  info;
	struct _CellList *previous, *next;
} CellList;

typedef struct {
	int         initialized;
	int         nkind[KINDS];	/* Number of fish and sharks */
	int         breed[KINDS];	/* Breeding time of fish and sharks */
	int         sstarve;	/* Time the sharks starve if they dont find a fish */
	int         kind;	/* Currently working on fish or sharks? */
	int         xs, ys;	/* Size of fish and sharks */
	int         xb, yb;	/* Bitmap offset for fish and sharks */
	int         pixelmode;
	int         generation;
	int         ncols, nrows;
	int         width, height;
	CellList   *currkind, *babykind, *lastkind[KINDS + 1], *firstkind[KINDS + 1];
	CellList  **arr;	/* 0=empty or pts to a fish or shark */
} watorstruct;

static watorstruct *wators = NULL;
static int  icon_width, icon_height;

static void
drawcell(ModeInfo * mi, int col, int row, unsigned long color, int bitmap)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	watorstruct *wp = &wators[MI_SCREEN(mi)];

	if (MI_NPIXELS(mi) > 2)
		XSetForeground(display, gc, MI_PIXEL(mi, color));
	else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	if (wp->pixelmode)
		if (bitmap >= BITMAPS / 2 || (wp->xs == 1 && wp->ys == 1))
			XFillRectangle(display, MI_WINDOW(mi), gc,
				       wp->xb + wp->xs * col, wp->yb + wp->ys * row, wp->xs, wp->ys);
		else
			XFillArc(display, MI_WINDOW(mi), gc,
				 wp->xb + wp->xs * col, wp->yb + wp->ys * row, wp->xs, wp->ys, 0, 23040);
	else
		XPutImage(display, MI_WINDOW(mi), gc, &logo[bitmap], 0, 0,
			  wp->xb + wp->xs * col, wp->yb + wp->ys * row, icon_width, icon_height);
}


static void
erasecell(ModeInfo * mi, int col, int row)
{
	watorstruct *wp = &wators[MI_SCREEN(mi)];

	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
	       wp->xb + wp->xs * col, wp->yb + wp->ys * row, wp->xs, wp->ys);
}

static void
init_kindlist(watorstruct * wp, int kind)
{
	/* Waste some space at the beginning and end of list
	   so we do not have to complicated checks against falling off the ends. */
	wp->lastkind[kind] = (CellList *) malloc(sizeof (CellList));
	wp->firstkind[kind] = (CellList *) malloc(sizeof (CellList));
	wp->firstkind[kind]->previous = wp->lastkind[kind]->next = NULL;
	wp->firstkind[kind]->next = wp->lastkind[kind]->previous = NULL;
	wp->firstkind[kind]->next = wp->lastkind[kind];
	wp->lastkind[kind]->previous = wp->firstkind[kind];
}

static void
addto_kindlist(watorstruct * wp, int kind, cellstruct info)
{
	wp->currkind = (CellList *) malloc(sizeof (CellList));
	wp->lastkind[kind]->previous->next = wp->currkind;
	wp->currkind->previous = wp->lastkind[kind]->previous;
	wp->currkind->next = wp->lastkind[kind];
	wp->lastkind[kind]->previous = wp->currkind;
	wp->currkind->info = info;
}

static void
removefrom_kindlist(watorstruct * wp, CellList * ptr)
{
	ptr->previous->next = ptr->next;
	ptr->next->previous = ptr->previous;
	wp->arr[ptr->info.col + ptr->info.row * wp->ncols] = 0;
	(void) free((void *) ptr);
}

static void
dupin_kindlist(watorstruct * wp)
{
	CellList   *temp;

	temp = (CellList *) malloc(sizeof (CellList));
	temp->previous = wp->babykind;
	temp->next = wp->babykind->next;
	wp->babykind->next = temp;
	temp->next->previous = temp;
	temp->info = wp->babykind->info;
	wp->babykind = temp;
}

/* 
 * new fish at end of list, this rotates who goes first, young fish go last
 * this most likely will not change the feel to any real degree
 */
static void
cutfrom_kindlist(watorstruct * wp)
{
	wp->babykind = wp->currkind;
	wp->currkind = wp->currkind->previous;
	wp->currkind->next = wp->babykind->next;
	wp->babykind->next->previous = wp->currkind;
	wp->babykind->next = wp->lastkind[KINDS];
	wp->babykind->previous = wp->lastkind[KINDS]->previous;
	wp->babykind->previous->next = wp->babykind;
	wp->babykind->next->previous = wp->babykind;
}

static void
reattach_kindlist(watorstruct * wp, int kind)
{
	wp->currkind = wp->lastkind[kind]->previous;
	wp->currkind->next = wp->firstkind[KINDS]->next;
	wp->currkind->next->previous = wp->currkind;
	wp->lastkind[kind]->previous = wp->lastkind[KINDS]->previous;
	wp->lastkind[KINDS]->previous->next = wp->lastkind[kind];
	wp->lastkind[KINDS]->previous = wp->firstkind[KINDS];
	wp->firstkind[KINDS]->next = wp->lastkind[KINDS];
}

static void
flush_kindlist(watorstruct * wp, int kind)
{
	while (wp->lastkind[kind]->previous != wp->firstkind[kind]) {
		wp->currkind = wp->lastkind[kind]->previous;
		wp->currkind->previous->next = wp->lastkind[kind];
		wp->lastkind[kind]->previous = wp->currkind->previous;
		/* wp->arr[wp->currkind->info.col + wp->currkind->info.row * wp->ncols] = 0; */
		(void) free((void *) wp->currkind);
	}
}


void
init_wator(ModeInfo * mi)
{
	int         size = MI_SIZE(mi);
	watorstruct *wp;
	int         i, col, row, colrow, kind;
	cellstruct  info;

	if (wators == NULL) {
		if ((wators = (watorstruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (watorstruct))) == NULL)
			return;
	}
	wp = &wators[MI_SCREEN(mi)];

	wp->generation = 0;
	if (!wp->initialized) {	/* Genesis */
		icon_width = fish0_width;
		icon_height = fish0_height;
		wp->initialized = 1;
		/* Set up what will be a 'triply' linked list.
		   doubly linked list, doubly linked to an array */
		for (kind = FISH; kind <= KINDS; kind++)
			init_kindlist(wp, kind);
		for (i = 0; i < BITMAPS; i++) {
			logo[i].width = icon_width;
			logo[i].height = icon_height;
			logo[i].bytes_per_line = (icon_width + 7) / 8;
		}
	} else			/* Exterminate all  */
		for (i = FISH; i <= KINDS; i++)
			flush_kindlist(wp, i);

	wp->width = MI_WIN_WIDTH(mi);
	wp->height = MI_WIN_HEIGHT(mi);
	if (wp->width < 2)
		wp->width = 2;
	if (wp->height < 2)
		wp->height = 2;
	if (size == 0 ||
	 MINGRIDSIZE * size > wp->width || MINGRIDSIZE * size > wp->height) {
		if (wp->width > MINGRIDSIZE * icon_width &&
		    wp->height > MINGRIDSIZE * icon_height) {
			wp->pixelmode = False;
			wp->xs = icon_width;
			wp->ys = icon_height;
		} else {
			wp->pixelmode = True;
			wp->xs = wp->ys = MAX(MINSIZE, MIN(wp->width, wp->height) / MINGRIDSIZE);
		}
	} else {
		wp->pixelmode = True;
		if (size < -MINSIZE)
			wp->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(wp->width, wp->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE)
			wp->ys = MINSIZE;
		else
			wp->ys = MIN(size, MAX(MINSIZE, MIN(wp->width, wp->height) /
					       MINGRIDSIZE));
		wp->xs = wp->ys;
	}
	wp->ncols = MAX(wp->width / wp->xs, 2);
	wp->nrows = MAX(wp->height / wp->ys, 2);

	if (wp->arr != NULL)
		(void) free((void *) wp->arr);
	wp->arr = (CellList **) calloc(wp->ncols * wp->nrows, sizeof (CellList *));

	/* Play G-d with these numbers */
	wp->nkind[FISH] = wp->ncols * wp->nrows / 3;
	wp->nkind[SHARK] = wp->nkind[FISH] / 10;
	wp->kind = FISH;
	if (!wp->nkind[SHARK])
		wp->nkind[SHARK] = 1;
	wp->breed[FISH] = MI_BATCHCOUNT(mi);
	wp->breed[SHARK] = 10;
	if (wp->breed[FISH] < 1)
		wp->breed[FISH] = 1;
	else if (wp->breed[FISH] > wp->breed[SHARK])
		wp->breed[FISH] = 4;
	wp->sstarve = 3;

	wp->xb = (wp->width - wp->xs * wp->ncols) / 2;
	wp->yb = (wp->height - wp->ys * wp->nrows) / 2;

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	for (kind = FISH; kind <= SHARK; kind++) {
		i = 0;
		while (i < wp->nkind[kind]) {
			col = NRAND(wp->ncols);
			row = NRAND(wp->nrows);
			colrow = col + row * wp->ncols;
			if (!wp->arr[colrow]) {
				i++;
				info.kind = kind;
				info.age = NRAND(wp->breed[kind]);
				info.food = NRAND(wp->sstarve);
				info.direction = NRAND(KINDBITMAPS) + kind * KINDBITMAPS;
				if (MI_NPIXELS(mi) > 2)
					info.color = NRAND(MI_NPIXELS(mi));
				else
					info.color = 0;
				info.col = col;
				info.row = row;
				addto_kindlist(wp, kind, info);
				wp->arr[colrow] = wp->currkind;
				drawcell(mi, col, row,
					 wp->currkind->info.color,
					 wp->currkind->info.direction);
			}
		}
	}
}

void
draw_wator(ModeInfo * mi)
{
	watorstruct *wp = &wators[MI_SCREEN(mi)];
	int         col, row, colprevious, rowprevious, colnext, rownext;
	int         colrowp, colrown, colprow, colnrow, colrow, cr;
	int         i, numok;

	struct {
		int         x, y, dir;
	} acell[4];


	/* Alternate updates, fish and sharks live out of phase with each other */
	wp->kind = (wp->kind + 1) % KINDS;
	{
		wp->currkind = wp->firstkind[wp->kind]->next;

		while (wp->currkind != wp->lastkind[wp->kind]) {
			col = wp->currkind->info.col;
			row = wp->currkind->info.row;
			colprevious = (!col) ? wp->ncols - 1 : col - 1;
			rowprevious = (!row) ? wp->nrows - 1 : row - 1;
			colnext = (col == wp->ncols - 1) ? 0 : col + 1;
			rownext = (row == wp->nrows - 1) ? 0 : row + 1;
			i = row * wp->ncols;
			colrowp = col + rowprevious * wp->ncols;
			colnrow = colnext + i;
			colrown = col + rownext * wp->ncols;
			colprow = colprevious + i;
			colrow = col + row * wp->ncols;
			numok = 0;
			if (wp->kind == SHARK) {	/* Scan for fish */
				if (wp->arr[colrowp] && wp->arr[colrowp]->info.kind == FISH) {
					acell[numok].x = col;
					acell[numok].y = rowprevious;
					acell[numok++].dir = 0;
				}
				if (wp->arr[colnrow] && wp->arr[colnrow]->info.kind == FISH) {
					acell[numok].x = colnext;
					acell[numok].y = row;
					acell[numok++].dir = 1;
				}
				if (wp->arr[colrown] && wp->arr[colrown]->info.kind == FISH) {
					acell[numok].x = col;
					acell[numok].y = rownext;
					acell[numok++].dir = 2;
				}
				if (wp->arr[colprow] && wp->arr[colprow]->info.kind == FISH) {
					acell[numok].x = colprevious;
					acell[numok].y = row;
					acell[numok++].dir = 3;
				}
				if (numok) {	/* No thanks, I'm vegetarian */
					i = NRAND(numok);
					wp->nkind[FISH]--;
					cr = acell[i].x + acell[i].y * wp->ncols;
					removefrom_kindlist(wp, wp->arr[cr]);
					wp->arr[cr] = wp->currkind;
					wp->currkind->info.direction = acell[i].dir +
						((NRAND(REFLECTS)) ? 0 : ORIENTS) + wp->kind * KINDBITMAPS;
					wp->currkind->info.col = acell[i].x;
					wp->currkind->info.row = acell[i].y;
					wp->currkind->info.food = wp->sstarve;
					drawcell(mi, wp->currkind->info.col, wp->currkind->info.row,
						 wp->currkind->info.color, wp->currkind->info.direction);
					if (++(wp->currkind->info.age) >= wp->breed[wp->kind]) {	/* breed */
						cutfrom_kindlist(wp);	/* This rotates out who goes first */
						wp->babykind->info.age = 0;
						dupin_kindlist(wp);
						wp->arr[colrow] = wp->babykind;
						wp->babykind->info.col = col;
						wp->babykind->info.row = row;
						wp->babykind->info.age = -1;	/* Make one a little younger */
#if 0
						if (MI_NPIXELS(mi) > 2 && (LRAND() & 1))
							/* A color mutation */
							if (++(wp->babykind->info.color) >= MI_NPIXELS(mi))
								wp->babykind->info.color = 0;
#endif
						wp->nkind[wp->kind]++;
					} else {
						wp->arr[colrow] = 0;
						erasecell(mi, col, row);
					}
				} else {
					if (wp->currkind->info.food-- < 0) {	/* Time to die, Jaws */
						/* back up one or else in void */
						wp->currkind = wp->currkind->previous;
						removefrom_kindlist(wp, wp->arr[colrow]);
						wp->arr[colrow] = 0;
						erasecell(mi, col, row);
						wp->nkind[wp->kind]--;
						numok = -1;	/* Want to escape from next if */
					}
				}
			}
			if (!numok) {	/* Fish or shark search for a place to go */
				if (!wp->arr[colrowp]) {
					acell[numok].x = col;
					acell[numok].y = rowprevious;
					acell[numok++].dir = 0;
				}
				if (!wp->arr[colnrow]) {
					acell[numok].x = colnext;
					acell[numok].y = row;
					acell[numok++].dir = 1;
				}
				if (!wp->arr[colrown]) {
					acell[numok].x = col;
					acell[numok].y = rownext;
					acell[numok++].dir = 2;
				}
				if (!wp->arr[colprow]) {
					acell[numok].x = colprevious;
					acell[numok].y = row;
					acell[numok++].dir = 3;
				}
				if (numok) {	/* Found a place to go */
					i = NRAND(numok);
					wp->arr[acell[i].x + acell[i].y * wp->ncols] = wp->currkind;
					wp->currkind->info.direction = acell[i].dir +
						((NRAND(REFLECTS)) ? 0 : ORIENTS) + wp->kind * KINDBITMAPS;
					wp->currkind->info.col = acell[i].x;
					wp->currkind->info.row = acell[i].y;
					drawcell(mi,
						 wp->currkind->info.col, wp->currkind->info.row,
						 wp->currkind->info.color, wp->currkind->info.direction);
					if (++(wp->currkind->info.age) >= wp->breed[wp->kind]) {	/* breed */
						cutfrom_kindlist(wp);	/* This rotates out who goes first */
						wp->babykind->info.age = 0;
						dupin_kindlist(wp);
						wp->arr[colrow] = wp->babykind;
						wp->babykind->info.col = col;
						wp->babykind->info.row = row;
						wp->babykind->info.age = -1;	/* Make one a little younger */
						wp->nkind[wp->kind]++;
					} else {
						wp->arr[colrow] = 0;
						erasecell(mi, col, row);
					}
				} else {
					/* I'll just sit here and wave my tail so you know I am alive */
					wp->currkind->info.direction =
						(wp->currkind->info.direction + ORIENTS) % KINDBITMAPS +
						wp->kind * KINDBITMAPS;
					drawcell(mi, col, row, wp->currkind->info.color,
					       wp->currkind->info.direction);
				}
			}
			wp->currkind = wp->currkind->next;
		}
		reattach_kindlist(wp, wp->kind);
	}

	if ((wp->nkind[FISH] >= wp->nrows * wp->ncols) ||
	    (!wp->nkind[FISH] && !wp->nkind[SHARK]) ||
	    wp->generation >= MI_CYCLES(mi)) {
		init_wator(mi);
	}
	if (wp->kind == SHARK)
		wp->generation++;
}

void
release_wator(ModeInfo * mi)
{
	if (wators != NULL) {
		int         screen, kind;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			watorstruct *wp = &wators[screen];

			for (kind = 0; kind <= KINDS; kind++) {
				flush_kindlist(wp, kind);
				(void) free((void *) wp->lastkind[kind]);
				(void) free((void *) wp->firstkind[kind]);
			}
			if (wp->arr != NULL)
				(void) free((void *) wp->arr);
		}
		(void) free((void *) wators);
		wators = NULL;
	}
}

void
refresh_wator(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
