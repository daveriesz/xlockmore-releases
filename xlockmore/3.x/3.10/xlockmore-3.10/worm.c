
#ifndef lint
static char sccsid[] = "@(#)worm.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * worm.c - draw wiggly worms.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 27-Sep-95: put back malloc
 * 23-Sep-93: got rid of "rint". (David Bagley)
 * 27-Sep-91: got rid of all malloc calls since there were no calls to free().
 * 25-Sep-91: Integrated into X11R5 contrib xlock.
 *
 * Adapted from a concept in the Dec 87 issue of Scientific American p. 142.
 *
 * SunView version: Brad Taylor (brad@sun.com)
 * X11 version: Dave Lemke (lemke@ncd.com)
 * xlock version: Boris Putanec (bp@cs.brown.edu)
 */

#include	"xlock.h"
#include	<math.h>

#define CIRCSIZE 2

#define SEGMENTS  36
#define MINWORMS 1
#define IRINT(x) ((int)(((x)>0.0)?(x)+0.5:(x)-0.5))

ModeSpecOpt worm_opts =
{0, NULL, NULL, NULL};

typedef struct {
	XPoint     *circ;
	int         dir;
	int         tail;
	XPoint      loc;
} wormstuff;

typedef struct {
	XPoint      window_size;
	int         wormlength;
	unsigned long monopix;
	int         nc;
	int         nw;
	wormstuff  *worm;
	XRectangle  rects[NUMCOLORS][NUMCOLORS];
	int         size[NUMCOLORS];
} wormstruct;

static int  sintab[SEGMENTS];
static int  costab[SEGMENTS];
static int  init_table = 0;

static wormstruct *worms = NULL;

static void
worm_doit(ModeInfo * mi, int which, unsigned long color)
{
	wormstruct *wp = &worms[MI_SCREEN(mi)];
	wormstuff  *ws = &wp->worm[which];
	int         x, y;

	ws->tail++;
	if (ws->tail == wp->wormlength)
		ws->tail = 0;

	x = ws->circ[ws->tail].x;
	y = ws->circ[ws->tail].y;
	XClearArea(MI_DISPLAY(mi), MI_WINDOW(mi), x, y, CIRCSIZE, CIRCSIZE, False);

	if (LRAND() & 1)
		ws->dir = (ws->dir + 1) % SEGMENTS;
	else
		ws->dir = (ws->dir + SEGMENTS - 1) % SEGMENTS;

	x = (ws->loc.x + costab[ws->dir] + wp->window_size.x) % wp->window_size.x;
	y = (ws->loc.y + sintab[ws->dir] + wp->window_size.y) % wp->window_size.y;

	ws->circ[ws->tail].x = x;
	ws->circ[ws->tail].y = y;
	ws->loc.x = x;
	ws->loc.y = y;

	wp->rects[color][wp->size[color]].x = x;
	wp->rects[color][wp->size[color]].y = y;
	wp->size[color]++;
}

static void
free_worms(wormstruct * wp)
{
	int         wn;

	if (wp->worm) {
		for (wn = 0; wn < wp->nw; wn++)
			if (wp->worm[wn].circ)
				(void) free((void *) wp->worm[wn].circ);
		(void) free((void *) wp->worm);
		wp->worm = NULL;
	}
}

void
init_worm(ModeInfo * mi)
{
	wormstruct *wp;
	int         i, j;

	if (worms == NULL) {
		if ((worms = (wormstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (wormstruct))) == NULL)
			return;
	}
	wp = &worms[MI_SCREEN(mi)];
	if (MI_NPIXELS(mi) <= 2)
		wp->nc = 2;
	else
		wp->nc = MI_NPIXELS(mi);
	if (wp->nc > NUMCOLORS)
		wp->nc = NUMCOLORS;

	free_worms(wp);
	wp->nw = MI_BATCHCOUNT(mi);
	if (wp->nw < -MINWORMS)
		wp->nw = NRAND(-wp->nw - MINWORMS + 1) + MINWORMS;
	else if (wp->nw < MINWORMS)
		wp->nw = MINWORMS;
	if (!wp->worm)
		wp->worm = (wormstuff *) malloc(wp->nw * sizeof (wormstuff));

	if (!init_table) {
		init_table = 1;
		for (i = 0; i < SEGMENTS; i++) {
			/* round */
			sintab[i] = IRINT(CIRCSIZE * sin(i * 2 * M_PI / SEGMENTS));
			costab[i] = IRINT(CIRCSIZE * cos(i * 2 * M_PI / SEGMENTS));
		}
	}
	wp->window_size.x = MI_WIN_WIDTH(mi);
	wp->window_size.y = MI_WIN_HEIGHT(mi);

	wp->monopix = MI_WIN_WHITE_PIXEL(mi);


	for (i = 0; i < wp->nc; i++) {
		for (j = 0; j < wp->nw / wp->nc + 1; j++) {
			wp->rects[i][j].width = CIRCSIZE;
			wp->rects[i][j].height = CIRCSIZE;

		}
	}
	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	wp->wormlength = sqrt(wp->window_size.x + wp->window_size.y) *
		MI_CYCLES(mi) / 8;	/* Fudge this to something reasonable */
	for (i = 0; i < wp->nw; i++) {
		wp->worm[i].circ = (XPoint *) malloc(wp->wormlength * sizeof (XPoint));
		for (j = 0; j < wp->wormlength; j++) {
			wp->worm[i].circ[j].x = wp->window_size.x / 2;
			wp->worm[i].circ[j].y = wp->window_size.y / 2;
		}
		wp->worm[i].dir = NRAND(SEGMENTS);
		wp->worm[i].tail = 0;
		wp->worm[i].loc.x = wp->window_size.x / 2;
		wp->worm[i].loc.y = wp->window_size.y / 2;
	}

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_worm(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	wormstruct *wp = &worms[MI_SCREEN(mi)];
	unsigned long wcolor;
	static unsigned int chromo = 0;
	int         i;

	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	for (i = 0; i < wp->nw; i++) {
		if (MI_NPIXELS(mi) > 2) {
			wcolor = (i + chromo) % wp->nc;

			worm_doit(mi, i, wcolor);
		} else
			worm_doit(mi, i, (unsigned long) 0);
	}

	if (MI_NPIXELS(mi) > 2) {
		for (i = 0; i < wp->nc; i++) {
			XSetForeground(display, gc, MI_PIXEL(mi, i));
			XFillRectangles(display, MI_WINDOW(mi), gc, wp->rects[i], wp->size[i]);
		}
	} else {
		XSetForeground(display, gc, wp->monopix);
		XFillRectangles(display, MI_WINDOW(mi), gc,
				wp->rects[0], wp->size[0]);
	}

	if (++chromo == wp->nc)
		chromo = 0;
}

void
release_worm(ModeInfo * mi)
{
	if (worms != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_worms(&worms[screen]);
		(void) free((void *) worms);
		worms = NULL;
	}
}
