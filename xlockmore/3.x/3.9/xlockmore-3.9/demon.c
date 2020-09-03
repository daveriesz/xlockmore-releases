#ifndef lint
static char sccsid[] = "@(#)demon.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * demon.c - David Griffeath's cellular automata for xlock,
 *           the X Window System lockscreen.
 *
 * Copyright (c) 1995 by David Bagley.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 30-May-96: Ron Hitchens <ronh@utw.com>
 *            Fixed memory management that caused leaks
 * 14-Apr-96: -neighbors 6 runtime-time option added
 *            -gridsize <int> runtime option added 50 good for hexagons
 *            while 170 for squares.
 * 21-Aug-95: Coded from A.K. Dewdney's "Computer Recreations", Scientific
 *            American Magazine" Aug 1989 pp 102-105.
 *            also used life.c as a guide.
 */

#include "xlock.h"

/*-
  Grid     Number of Neigbors
  ----     ------------------
  Square   4 (or 8)          <- 8 is not implemented
  Hexagon  6
  Triangle (3, 9?, or 12)    <- 3, 9, and 12 are not implemented
*/

ModeSpecOpt demon_opts =
{0, NULL, NULL, NULL};

extern      neighbors;
extern      gridsize;

/* Singly linked list */
typedef struct _DemonList {
	XPoint      pt;
	struct _DemonList *next;
} DemonList;

typedef struct {
	int         generation;
	int         xs, ys;
	int         xb, yb;
	int         nrows, ncols;
	int         width, height;
	int         states;
	int         state;
	int        *ncells;
	DemonList **demonlist;
	unsigned char *old, *new;
	XPoint      hexagonList[6];
} demonstruct;

static XPoint hexagonUnit[6] =
{
	{0, 0},
	{1, 1},
	{0, 2},
	{-1, 1},
	{-1, -1},
	{0, -2}
};

static demonstruct *demons = NULL;

#ifdef DEBUG
static void
drawcell(ModeInfo * mi, int col, int row, unsigned char state)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];

	if (!state)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	else if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			   MI_PIXEL(mi, (((int) state - 1) * MI_NPIXELS(mi) /
					 (dp->states - 1)) % MI_NPIXELS(mi)));
	else
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), (state % 2) ?
			    MI_WIN_WHITE_PIXEL(mi) : MI_WIN_BLACK_PIXEL(mi));
	if (neighbors == 6) {
		int         ccol = 2 * col + !(row & 1), crow = 2 * row;

		dp->hexagonList[0].x = dp->xb + ccol * dp->xs;
		dp->hexagonList[0].y = dp->yb + crow * dp->ys;
		XFillPolygon(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			     dp->hexagonList, 6, Convex, CoordModePrevious);
	} else {
		XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
		dp->xb + dp->xs * col, dp->yb + dp->ys * row, dp->xs, dp->ys);
	}
}
#endif

static void
addtolist(ModeInfo * mi, int col, int row, unsigned char state)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];
	DemonList  *current;

	current = dp->demonlist[state];
	dp->demonlist[state] = (DemonList *) malloc(sizeof (DemonList));
	dp->demonlist[state]->pt.x = col;
	dp->demonlist[state]->pt.y = row;
	dp->demonlist[state]->next = current;
	dp->ncells[state]++;
}

#ifdef DEBUG
static void
print_state(ModeInfo * mi, int state)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];
	DemonList  *locallist;
	int         i = 0;

	locallist = dp->demonlist[state];
	(void) printf("state %d\n", state);
	while (locallist) {
		(void) printf("%d	x %d, y %d\n", i,
			      locallist->pt.x, locallist->pt.y);
		locallist = locallist->next;
    i++;
	}
}

#endif

static void
free_state(int screen_num, int state)
{
	demonstruct *dp = &demons[screen_num];
	DemonList  *current;

	while (dp->demonlist[state]) {
		current = dp->demonlist[state];
		dp->demonlist[state] = dp->demonlist[state]->next;
		(void) free((void *) current);
	}
	dp->demonlist[state] = NULL;
	dp->ncells[state] = 0;
}


static void
free_list(int screen_num)
{
	demonstruct *dp = &demons[screen_num];
	int         state;

	for (state = 0; state < dp->states; state++)
		free_state(screen_num, state);
}

static void
draw_state(ModeInfo * mi, int state)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];
	XRectangle *rects;
	DemonList  *current;

	if (!state)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	else if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi),
			   MI_PIXEL(mi, (((int) state - 1) * MI_NPIXELS(mi) /
					 (dp->states - 1)) % MI_NPIXELS(mi)));
	else
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), (state % 2) ?
			    MI_WIN_WHITE_PIXEL(mi) : MI_WIN_BLACK_PIXEL(mi));

	if (neighbors == 6) {	/*Draw right away, slow */
		current = dp->demonlist[state];
		while (current) {
			int         col, row, ccol, crow;

			col = current->pt.x;
			row = current->pt.y;
			ccol = 2 * col + !(row & 1), crow = 2 * row;
			dp->hexagonList[0].x = dp->xb + ccol * dp->xs;
			dp->hexagonList[0].y = dp->yb + crow * dp->ys;
			XFillPolygon(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
			      dp->hexagonList, 6, Convex, CoordModePrevious);
			current = current->next;
		}
	} else {		/* Take advantage of XDrawRectangles */
		int         ncells;

		/* Create Rectangle list from part of the demonlist */
		rects = (XRectangle *) malloc(dp->ncells[state] * sizeof (XRectangle));
		ncells = 0;
		current = dp->demonlist[state];
		while (current) {
			rects[ncells].x = dp->xb + current->pt.x * dp->xs;
			rects[ncells].y = dp->yb + current->pt.y * dp->ys;
			rects[ncells].width = dp->xs;
			rects[ncells].height = dp->ys;
			current = current->next;
			ncells++;
		}
		/* Finally get to draw */
		XFillRectangles(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi), rects, ncells);
		/* Free up rects list and the appropriate part of the demonlist */
		(void) free((void *) rects);
	}
	free_state(MI_SCREEN(mi), state);
}

static void
RandomSoup(ModeInfo * mi)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];
	int         row, col, mrow = 0;

	for (row = 0; row < dp->nrows; ++row) {
		for (col = 0; col < dp->ncols; ++col) {
			dp->old[col + mrow] =
				(unsigned char) LRAND() % ((unsigned char) dp->states);
			addtolist(mi, col, row, dp->old[col + mrow]);
		}
		mrow += dp->ncols;
	}
}

void
init_demon(ModeInfo * mi)
{
	demonstruct *dp;

	if (demons == NULL) {
		if ((demons = (demonstruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (demonstruct))) == NULL)
			return;
	}
	dp = &demons[MI_SCREEN(mi)];
	dp->generation = 0;
	dp->states = MI_BATCHCOUNT(mi);
	if (dp->states < 1)
		dp->states = 1;
	if (dp->demonlist == NULL)
		dp->demonlist = (DemonList **) calloc(dp->states, sizeof (DemonList *));
	else
		free_list(MI_SCREEN(mi));
	if (dp->ncells == NULL)
		dp->ncells = (int *) calloc(dp->states, sizeof (int));

	else
		(void) memset((char *) dp->ncells, 0, dp->states * sizeof (int));
	dp->state = 0;

	dp->width = MI_WIN_WIDTH(mi);
	dp->height = MI_WIN_HEIGHT(mi);

	if (neighbors != 4 && neighbors != 6)
		neighbors = 4;
	if (gridsize < 4)
		gridsize = 4;

	if (neighbors == 6) {
		int         nccols, ncrows, i;

		nccols = min(dp->width, 2 * (gridsize + 1));
		ncrows = min(dp->height, 2 * (gridsize + 2));
		dp->ncols = nccols >> 1;
		dp->nrows = (ncrows >> 2) << 1;
		nccols = dp->ncols << 1;
		ncrows = dp->nrows << 1;
		dp->xs = dp->width / nccols;
		if (dp->xs < 2)
			dp->xs = 2;
		dp->ys = dp->height / ncrows;
		if (dp->ys < 2)
			dp->ys = 2;
		dp->xb = (dp->width - dp->xs * nccols) / 2 + dp->xs;
		dp->yb = (dp->height - dp->ys * ncrows) / 2 + dp->ys;
		dp->ncols -= 1;
		dp->nrows -= 2;
		for (i = 0; i < 6; i++) {
			dp->hexagonList[i].x = (dp->xs - 1) * hexagonUnit[i].x;
			dp->hexagonList[i].y = (dp->ys - 1) * hexagonUnit[i].y / 2;
		}
	} else {
		dp->ncols = min(dp->width, gridsize);
		dp->nrows = min(dp->height, gridsize);
		dp->xs = dp->width / dp->ncols;
		dp->ys = dp->height / dp->nrows;
		dp->xb = (dp->width - dp->xs * dp->ncols) / 2;
		dp->yb = (dp->height - dp->ys * dp->nrows) / 2;
	}
	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
		       0, 0, dp->width, dp->height);

	if (dp->old == NULL)
		dp->old = (unsigned char *)
			malloc(gridsize * gridsize * sizeof (unsigned char));

	if (dp->new == NULL)
		dp->new = (unsigned char *)
			malloc(gridsize * gridsize * sizeof (unsigned char));

	RandomSoup(mi);
}

void
draw_demon(ModeInfo * mi)
{
	demonstruct *dp = &demons[MI_SCREEN(mi)];
	int         i, j, k, l, mj = 0, ml;

	if (dp->state >= dp->states) {
		(void) memcpy((char *) dp->new, (char *) dp->old,
			      dp->ncols * dp->nrows * sizeof (unsigned char));

		if (neighbors == 6) {
			for (j = 0; j < dp->nrows; j++) {
				for (i = 0; i < dp->ncols; i++) {
					/* NE */
					if (!(j & 1))
						k = (i + 1 == dp->ncols) ? 0 : i + 1;
					else
						k = i;
					l = (!j) ? dp->nrows - 1 : j - 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* E */
					k = (i + 1 == dp->ncols) ? 0 : i + 1;
					l = j;
					ml = mj;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* SE */
					if (!(j & 1))
						k = (i + 1 == dp->ncols) ? 0 : i + 1;
					else
						k = i;
					l = (j + 1 == dp->nrows) ? 0 : j + 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* SW */
					if (j & 1)
						k = (!i) ? dp->ncols - 1 : i - 1;
					else
						k = i;
					l = (j + 1 == dp->nrows) ? 0 : j + 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* W */
					k = (!i) ? dp->ncols - 1 : i - 1;
					l = j;
					ml = mj;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* NW */
					if (j & 1)
						k = (!i) ? dp->ncols - 1 : i - 1;
					else
						k = i;
					l = (!j) ? dp->nrows - 1 : j - 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
				}
				mj += dp->ncols;
			}
		} else {
			for (j = 0; j < dp->nrows; j++) {
				for (i = 0; i < dp->ncols; i++) {
					/* N */
					k = i;
					l = (!j) ? dp->nrows - 1 : j - 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* E */
					k = (i + 1 == dp->ncols) ? 0 : i + 1;
					l = j;
					ml = mj;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* S */
					k = i;
					l = (j + 1 == dp->nrows) ? 0 : j + 1;
					ml = l * dp->ncols;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
					/* W */
					k = (!i) ? dp->ncols - 1 : i - 1;
					l = j;
					ml = mj;
					if (dp->old[k + ml] == (int) (dp->old[i + mj] + 1) % dp->states)
						dp->new[i + mj] = dp->old[k + ml];
				}
				mj += dp->ncols;
			}
		}
		mj = 0;
		for (j = 0; j < dp->nrows; j++) {
			for (i = 0; i < dp->ncols; i++)
				if (dp->old[i + mj] != dp->new[i + mj]) {
					dp->old[i + mj] = dp->new[i + mj];
					addtolist(mi, i, j, dp->old[i + mj]);
				}
			mj += dp->ncols;
		}
		if (++dp->generation > MI_CYCLES(mi))
			init_demon(mi);
		dp->state = 0;
	} else {
		if (dp->ncells[dp->state])
			draw_state(mi, dp->state);
		dp->state++;
	}
}

void
release_demon(ModeInfo * mi)
{
	if (demons != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			free_list(screen);
			if (demons[screen].demonlist != NULL)
				(void) free((void *) demons[screen].demonlist);
			if (demons[screen].ncells != NULL)
				(void) free((void *) demons[screen].ncells);
			/* ncells is referenced by free_list() above */
			if (demons[screen].old != NULL)
				(void) free((void *) demons[screen].old);
			if (demons[screen].new != NULL)
				(void) free((void *) demons[screen].new);
      /*
       * No need to nullify these pointers, they will
       * evaporate when the global array "demons" is freed
       */
		}
		(void) free((void *) demons);
		demons = NULL;
	}
}
