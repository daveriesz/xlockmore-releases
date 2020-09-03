#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)julia.c	4.00 97/01/01 xlockmore";

#endif

/*- 
 * julia.c - continuously varying Julia set for xlock
 * 
 * Copyright (c) 1995 Sean McCullough <bankshot@mailhost.nmt.edu>.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 2-Dec-95: snagged boilerplate from hop.c
 *           used ifs {w0 = sqrt(x-c), w1 = -sqrt(x-c)} with random iteration 
 *           to plot the julia set, and sinusoidially varied parameter for set 
 *	     and plotted parameter with a circle.
 */
/*-
 * One thing to note is that batchcount is the *depth* of the search tree,
 * so the number of points computed is 2^batchcount - 1.  I use 8 or 9
 * on a dx266 and it looks okay.  The sinusoidal variation of the parameter
 * might not be as interesting as it could, but it still gives an idea of
 * the effect of the parameter.
 */
#include "xlock.h"

#define numpoints ((0x2<<jp->depth)-1)
ModeSpecOpt julia_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	int         centerx;
	int         centery;	/* center of the screen */
	double      cr;
	double      ci;		/* julia params */
	int         depth;
	int         inc;
	int         circsize;
	int         erase;
	int         pix;
	long        itree;
	int         buffer;
	int         nbuffers;
	int         redrawing, redrawpos;
	Pixmap      pixmap;
	GC          stippledGC;
	XPoint    **pointBuffer;	/* pointer for XDrawPoints */
} juliastruct;

static juliastruct *julias = NULL;

/* How many segments to draw per cycle when redrawing */
#define REDRAWSTEP 3

static void
apply(juliastruct * jp, register double xr, register double xi, int d)
{
	double      theta, r;

	jp->pointBuffer[jp->buffer][jp->itree].x =
		(int) (0.5 * xr * jp->centerx + jp->centerx);
	jp->pointBuffer[jp->buffer][jp->itree].y =
		(int) (0.5 * xi * jp->centery + jp->centery);
	jp->itree++;

	if (d > 0) {
		xi -= jp->ci;
		xr -= jp->cr;

		theta = atan2(xi, xr) / 2.0;

		/*r = pow(xi * xi + xr * xr, 0.25); */
		r = sqrt(sqrt(xi * xi + xr * xr));	/* 3 times faster */

		xr = r * cos(theta);
		xi = r * sin(theta);

		d--;
		apply(jp, xr, xi, d);
		apply(jp, -xr, -xi, d);
	}
}

static void
incr(juliastruct * jp)
{
	jp->cr = 1.5 * sin(M_PI * (jp->inc / 300.0)) * sin(jp->inc * M_PI / 200.0);
	jp->ci = 1.5 * cos(M_PI * (jp->inc / 300.0)) * cos(jp->inc * M_PI / 200.0);

	jp->cr += 0.5 * cos(M_PI * jp->inc / 400.0);
	jp->ci += 0.5 * sin(M_PI * jp->inc / 400.0);
}

void
init_julia(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	juliastruct *jp;
	XGCValues   gcv;
	int         i;

	if (julias == NULL) {
		if ((julias = (juliastruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (juliastruct))) == NULL)
			return;
	}
	jp = &julias[MI_SCREEN(mi)];

	jp->centerx = MI_WIN_WIDTH(mi) / 2;
	jp->centery = MI_WIN_HEIGHT(mi) / 2;

	jp->depth = MI_BATCHCOUNT(mi);
	if (jp->depth > 10)
		jp->depth = 10;


	if (jp->pixmap != None &&
	    jp->circsize != (MIN(jp->centerx, jp->centery) / 60) * 2 + 1) {
		XFreePixmap(display, jp->pixmap);
		jp->pixmap = None;
	}
	if (jp->pixmap == None) {
		GC          fg_gc = None, bg_gc = None;

		jp->circsize = (MIN(jp->centerx, jp->centery) / 96) * 2 + 1;
		jp->pixmap = XCreatePixmap(display, window, jp->circsize, jp->circsize, 1);
		gcv.foreground = 1;
		fg_gc = XCreateGC(display, jp->pixmap, GCForeground, &gcv);
		gcv.foreground = 0;
		bg_gc = XCreateGC(display, jp->pixmap, GCForeground, &gcv);
		XFillRectangle(display, jp->pixmap, bg_gc,
			       0, 0, jp->circsize, jp->circsize);
		if (jp->circsize < 2)
			XDrawPoint(display, jp->pixmap, fg_gc, 0, 0);
		else
			XFillArc(display, jp->pixmap, fg_gc,
				 0, 0, jp->circsize, jp->circsize, 0, 23040);
		if (fg_gc != None)
			XFreeGC(display, fg_gc);
		if (bg_gc != None)
			XFreeGC(display, bg_gc);
	}
	if (!jp->stippledGC) {
		gcv.foreground = MI_WIN_BLACK_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);
		if ((jp->stippledGC = XCreateGC(display, window,
				 GCForeground | GCBackground, &gcv)) == None)
			return;
	}
	if (MI_NPIXELS(mi) > 2)
		jp->pix = NRAND(MI_NPIXELS(mi));
	jp->inc = ((LRAND() & 1) * 2 - 1) * NRAND(200);
	jp->nbuffers = (MI_CYCLES(mi) + 1);
	if (!jp->pointBuffer)
		jp->pointBuffer = (XPoint **) calloc(jp->nbuffers, sizeof (XPoint *));
	for (i = 0; i < jp->nbuffers; ++i)
		if (jp->pointBuffer[i])
			(void) memset((char *) jp->pointBuffer[i], 0,
				      numpoints * sizeof (XPoint));
		else
			jp->pointBuffer[i] = (XPoint *) calloc(numpoints, sizeof (XPoint));
	jp->buffer = 0;
	jp->redrawing = 0;
	jp->erase = 0;
	XClearWindow(display, window);
}

void
draw_julia(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	juliastruct *jp = &julias[MI_SCREEN(mi)];
	double      r, theta;
	register double xr = 0.0, xi = 0.0;
	int         k = 64, rnd = 0, i, j;
	XPoint     *xp = jp->pointBuffer[jp->buffer], old_circle, new_circle;

	extern void XEraseImage(Display * display, Window window, GC gc,
		   int x, int y, int xlast, int ylast, int xsize, int ysize);

	old_circle.x = (int) (jp->centerx * jp->cr / 2) + jp->centerx - 2;
	old_circle.y = (int) (jp->centery * jp->ci / 2) + jp->centery - 2;
	incr(jp);
	new_circle.x = (int) (jp->centerx * jp->cr / 2) + jp->centerx - 2;
	new_circle.y = (int) (jp->centery * jp->ci / 2) + jp->centery - 2;
	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	XEraseImage(display, window, gc, new_circle.x, new_circle.y,
		    old_circle.x, old_circle.y, jp->circsize, jp->circsize);
	/* draw a circle at the c-parameter so you can see it's effect on the
	   structure of the julia set */
	XSetTSOrigin(display, jp->stippledGC, new_circle.x, new_circle.y);
	XSetForeground(display, jp->stippledGC, MI_WIN_WHITE_PIXEL(mi));
	XSetStipple(display, jp->stippledGC, jp->pixmap);
	XSetFillStyle(display, jp->stippledGC, FillOpaqueStippled);
	XFillRectangle(display, window, jp->stippledGC, new_circle.x, new_circle.y,
		       jp->circsize, jp->circsize);
	XFlush(display);
	if (jp->erase == 1) {
		XDrawPoints(display, window, gc,
		    jp->pointBuffer[jp->buffer], numpoints, CoordModeOrigin);
	}
	jp->inc++;
	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(display, gc, MI_PIXEL(mi, jp->pix));
		if (++jp->pix >= MI_NPIXELS(mi))
			jp->pix = 0;
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	while (k--) {

		/* save calls to LRAND by using bit shifts over and over on the same
		   int for 32 iterations, then get a new random int */
		if (!(k % 32))
			rnd = LRAND();

		/* complex sqrt: x^0.5 = radius^0.5*(cos(theta/2) + i*sin(theta/2)) */

		xi -= jp->ci;
		xr -= jp->cr;

		theta = atan2(xi, xr) / 2.0;

		/*r = pow(xi * xi + xr * xr, 0.25); */
		r = sqrt(sqrt(xi * xi + xr * xr));	/* 3 times faster */

		xr = r * cos(theta);
		xi = r * sin(theta);

		if ((rnd >> (k % 32)) & 0x1) {
			xi = -xi;
			xr = -xr;
		}
		xp->x = jp->centerx + (int) ((jp->centerx >> 1) * xr);
		xp->y = jp->centery + (int) ((jp->centery >> 1) * xi);
		xp++;
	}

	jp->itree = 0;
	apply(jp, xr, xi, jp->depth);

	XDrawPoints(display, window, gc,
		    jp->pointBuffer[jp->buffer], numpoints, CoordModeOrigin);

	jp->buffer++;
	if (jp->buffer > jp->nbuffers - 1) {
		jp->buffer -= jp->nbuffers;
		jp->erase = 1;
	}
	if (jp->redrawing) {
		for (i = 0; i < REDRAWSTEP; i++) {
			j = (jp->buffer - jp->redrawpos + jp->nbuffers) % jp->nbuffers;
			XDrawPoints(display, window, gc,
			     jp->pointBuffer[j], numpoints, CoordModeOrigin);

			if (++(jp->redrawpos) >= jp->nbuffers) {
				jp->redrawing = 0;
				break;
			}
		}
	}
}

void
release_julia(ModeInfo * mi)
{
	if (julias != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			Display    *display = MI_DISPLAY(mi);
			juliastruct *jp = &julias[screen];
			int         buffer;

			if (jp->pointBuffer) {
				for (buffer = 0; buffer < jp->nbuffers; buffer++)
					if (jp->pointBuffer[buffer])
						(void) free((void *) jp->pointBuffer[buffer]);
				(void) free((void *) jp->pointBuffer);
			}
			if (jp->stippledGC != None)
				XFreeGC(display, jp->stippledGC);
			if (jp->pixmap != None)
				XFreePixmap(display, jp->pixmap);
		}
		(void) free((void *) julias);
		julias = NULL;
	}
}

void
refresh_julia(ModeInfo * mi)
{
	juliastruct *jp = &julias[MI_SCREEN(mi)];

	jp->redrawing = 1;
	jp->redrawpos = 0;
}
