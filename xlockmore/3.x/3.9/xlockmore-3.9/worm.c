#ifndef lint
static char sccsid[] = "@(#)worm.c	3.9 96/05/25 xlockmore";

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
 *
 * This code is a static memory pig... like almost 200K... but as contributed
 * it leaked at a massive rate, so I made everything static up front... feel
 * free to contribute the proper memory management code.
 * 
 */

#include	"xlock.h"
#include	<math.h>

#define CIRCSIZE 2
#define MAXWORMLEN 50

#define SEGMENTS  36
#define IRINT(x) ((int)(((x)>0.0)?(x)+0.5:(x)-0.5))

ModeSpecOpt worm_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         xcirc[MAXWORMLEN];
	int         ycirc[MAXWORMLEN];
	int         dir;
	int         tail;
	int         x;
	int         y;
} wormstuff;

typedef struct {
	int         xsize;
	int         ysize;
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

static wormstruct worms[MAXSCREENS];

static int
round(float x)
{
	/*return ((int) rint ((double) x)); */
	return IRINT((double) x);
}


static void
worm_doit(Window win, int which, long unsigned int color)
{
	wormstruct *wp = &worms[screen];
	wormstuff  *ws = &wp->worm[which];
	int         x, y;

	ws->tail++;
	if (ws->tail == wp->wormlength)
		ws->tail = 0;

	x = ws->xcirc[ws->tail];
	y = ws->ycirc[ws->tail];
	XClearArea(dsp, win, x, y, CIRCSIZE, CIRCSIZE, False);

	if (LRAND() & 1)
		ws->dir = (ws->dir + 1) % SEGMENTS;
	else
		ws->dir = (ws->dir + SEGMENTS - 1) % SEGMENTS;

	x = (ws->x + costab[ws->dir] + wp->xsize) % wp->xsize;
	y = (ws->y + sintab[ws->dir] + wp->ysize) % wp->ysize;

	ws->xcirc[ws->tail] = x;
	ws->ycirc[ws->tail] = y;
	ws->x = x;
	ws->y = y;

	wp->rects[color][wp->size[color]].x = x;
	wp->rects[color][wp->size[color]].y = y;
	wp->size[color]++;
}


void
init_worm(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	int         i, j;
	wormstruct *wp = &worms[screen];

	if (MI_WIN_IS_MONO(mi) || MI_NPIXELS(mi) <= 2)
		wp->nc = 2;
	else
		wp->nc = Scr[screen].npixels;
	if (wp->nc > NUMCOLORS)
		wp->nc = NUMCOLORS;

	wp->nw = MI_BATCHCOUNT(mi);
	if (wp->nw < 1)
		wp->nw = 1;
	if (!wp->worm)
		wp->worm = (wormstuff *) malloc(wp->nw * sizeof (wormstuff));

	if (!init_table) {
		init_table = 1;
		for (i = 0; i < SEGMENTS; i++) {
			sintab[i] = round(CIRCSIZE * sin(i * 2 * M_PI / SEGMENTS));
			costab[i] = round(CIRCSIZE * cos(i * 2 * M_PI / SEGMENTS));
		}
	}
	wp->xsize = MI_WIN_WIDTH(mi);
	wp->ysize = MI_WIN_HEIGHT(mi);

	wp->monopix = WhitePixel(dsp, screen);
	if (wp->xsize + wp->ysize < 200)
		wp->wormlength = MAXWORMLEN / 8;
	else
		wp->wormlength = MAXWORMLEN;

	for (i = 0; i < wp->nc; i++) {
		for (j = 0; j < wp->nw / wp->nc + 1; j++) {
			wp->rects[i][j].width = CIRCSIZE;
			wp->rects[i][j].height = CIRCSIZE;

		}
	}
	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	for (i = 0; i < wp->nw; i++) {
		for (j = 0; j < wp->wormlength; j++) {
			wp->worm[i].xcirc[j] = wp->xsize / 2;
			wp->worm[i].ycirc[j] = wp->ysize / 2;
		}
		wp->worm[i].dir = (unsigned) LRAND() % SEGMENTS;
		wp->worm[i].tail = 0;
		wp->worm[i].x = wp->xsize / 2;
		wp->worm[i].y = wp->ysize / 2;
	}

	XClearWindow(dsp, win);
}


void
draw_worm(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	wormstruct *wp = &worms[screen];
	unsigned long wcolor;
	static unsigned int chromo = 0;
	int         i;

	(void) memset((char *) wp->size, 0, wp->nc * sizeof (int));

	for (i = 0; i < wp->nw; i++) {
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
			wcolor = (i + chromo) % wp->nc;

			worm_doit(win, i, wcolor);
		} else
			worm_doit(win, i, (unsigned long) 0);
	}

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		for (i = 0; i < wp->nc; i++) {
			XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[i]);
			XFillRectangles(dsp, win, Scr[screen].gc,
					wp->rects[i], wp->size[i]);
		}
	} else {
		XSetForeground(dsp, Scr[screen].gc, wp->monopix);
		XFillRectangles(dsp, win, Scr[screen].gc,
				wp->rects[0], wp->size[0]);
	}

	if (++chromo == wp->nc)
		chromo = 0;
}
