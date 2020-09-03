#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)lisa.c        4.00 97/03/22 xlockmore";

#endif

/*-
 * lisa.c - Shows animated full-loop lisajous figures (XLock Module)
 *
 * See xlock.c for copying information.
 *
 * The inspiration for this program, Lasp, was written by Adam B. Roach 
 * in 1990, assisted by me, Caleb Cullen.  It was written first in C, then
 * in assembly, and used pre-calculated data tables to graph lisajous
 * figures on 386 machines and lower.  This version bears only superficial
 * resemblances to the original Lasp.
 *
 * The `lissie' module's source code was studied as an example of how
 * to incorporate a new module into xlock.  Resemblances to it are
 * expected, but not intended to be plaigiaristic.
 */

#include "xlock.h"

#define  DRAWLINES    1
#define  TWOLOOPS     1
#define  ADDITIVE     "True"
#define  XVMAX        10	/* Maximum velocities */
#define  YVMAX        10
#define  LISAMAXFUNCS 2
#define  NUMSTDFUNCS  10
#define  MAXCYCLES    3
#define  lisasetcolor() \
if (MI_NPIXELS(mi) > 2) { \
  XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_PIXEL(mi, loop->color)); \
  if (++(loop->color) >= MI_NPIXELS(mi)) { loop->color=0; } \
  } else { XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_WHITE_PIXEL(mi)); }
#define getRadius(context) \
  ((context->width > context->height)?context->height:context->width) * 3 / 8
#define checkRadius(loop, context) \
  if ((context->height / 2 > MI_SIZE(mi)) && (context->width / 2 > MI_SIZE(mi))) \
      loop->radius = MI_SIZE(mi); \
  if ((loop->radius < 0) || \
      (loop->radius > loop->center.x) || \
      (loop->radius > loop->center.y)) loop->radius = getRadius(context)


typedef struct lisafunc_struct {
	double      xcoeff[2], ycoeff[2];
	int         nx, ny;
	int         index;
} lisafuncs;

typedef struct lisa_struct {
	int         radius, color, dx, dy, nsteps, nfuncs, melting;
	double      pistep, phi, theta;
	XPoint      center, *lastpoint;
	lisafuncs  *function[LISAMAXFUNCS];
} lisas;

typedef struct lisacontext_struct {
	lisas      *lisajous;
	int         width, height, nlisajous, loopcount;
  int         maxcycles;
} lisacons;

static lisacons *Lisa = NULL;
static Bool additive;

static XrmOptionDescRec lisa_xrm_opts[] =
{
	{"-additive", ".lisa.additive", XrmoptionNoArg, (caddr_t) "True"},
	{"+additive", ".lisa.additive", XrmoptionNoArg, (caddr_t) "False"}
};

static argtype lisa_vars[] =
{
	{(caddr_t *) & additive, "additive", "Additive", ADDITIVE, t_Bool}
};

static OptionStruct lisa_vars_desc[] =
{
	{"-/+additive", "turn on/off additive functions mode"}
};

ModeSpecOpt lisa_opts =
{2, lisa_xrm_opts, 1, lisa_vars, lisa_vars_desc};

static lisafuncs Function[NUMSTDFUNCS] =
{
	{
		{1.0, 2.0},
		{1.0, 2.0}, 2, 2, 0},
	{
		{1.0, 2.0},
		{1.0, 1.0}, 2, 2, 1},
	{
		{1.0, 3.0},
		{1.0, 2.0}, 2, 2, 2},
	{
		{1.0, 3.0},
		{1.0, 3.0}, 2, 2, 3},
	{
		{2.0, 4.0},
		{1.0, 2.0}, 2, 2, 4},
	{
		{1.0, 4.0},
		{1.0, 3.0}, 2, 2, 5},
	{
		{1.0, 4.0},
		{1.0, 4.0}, 2, 2, 6},
	{
		{1.0, 5.0},
		{1.0, 5.0}, 2, 2, 7},
	{
		{2.0, 5.0},
		{2.0, 5.0}, 2, 2, 8},
	{
		{1.0, 0.0},
		{1.0, 0.0}, 1, 1, 9}
};

static void
drawlisa(ModeInfo * mi, lisas * loop)
{
	XPoint     *np;
	XPoint     *lp = loop->lastpoint;
	lisacons   *lc = &Lisa[MI_SCREEN(mi)];
	lisafuncs **lf = loop->function;
	int         phase = lc->loopcount % loop->nsteps;
	int         pctr, fctr, xctr, yctr;
	double      xprod, yprod, xsum, ysum;

	/* Allocate the np array */
	np = (XPoint *) calloc(loop->nsteps, sizeof (XPoint));

	/* Update the center */
	loop->center.x += loop->dx;
	loop->center.y += loop->dy;
	checkRadius(loop, lc);
	if ((loop->center.x - loop->radius) <= 0) {
		loop->center.x = loop->radius;
		loop->dx = NRAND(XVMAX);
	} else if ((loop->center.x + loop->radius) >= lc->width) {
		loop->center.x = lc->width - loop->radius;
		loop->dx = -NRAND(XVMAX);
	};
	if ((loop->center.y - loop->radius) <= 0) {
		loop->center.y = loop->radius;
		loop->dy = NRAND(YVMAX);
	} else if ((loop->center.y + loop->radius) >= lc->height) {
		loop->center.y = lc->height - loop->radius;
		loop->dy = -NRAND(YVMAX);
	};

	/* Now draw the points, and erase the ones from the last cycle */

	for (pctr = 0; pctr < loop->nsteps; pctr++) {
		fctr = loop->nfuncs;
		loop->phi = (double) (pctr - phase) * loop->pistep;
		loop->theta = (double) (pctr + phase) * loop->pistep;
		xsum = ysum = 0;
		while (fctr--) {
			xctr = lf[fctr]->nx;
			yctr = lf[fctr]->ny;
			if (additive) {
				xprod = yprod = 0.0;
				while (xctr--)
					xprod += sin(lf[fctr]->xcoeff[xctr] * loop->theta);
				while (yctr--)
					yprod += sin(lf[fctr]->ycoeff[yctr] * loop->phi);
				if (loop->melting) {
					if (fctr) {
						xsum += xprod \
							*(double) (loop->nsteps - loop->melting) \
							/(double) loop->nsteps;
						ysum += yprod \
							*(double) (loop->nsteps - loop->melting) \
							/(double) loop->nsteps;
					} else {
						xsum += xprod \
							*(double) loop->melting \
							/(double) loop->nsteps;
						ysum += yprod \
							*(double) loop->melting \
							/(double) loop->nsteps;
					}
				} else {
					xsum = xprod;
					ysum = yprod;
				}
				if (!fctr) {
					xsum = xsum \
						*(double) loop->radius \
						/(double) lf[fctr]->nx;
					ysum = ysum \
						*(double) loop->radius \
						/(double) lf[fctr]->ny;
				}
			} else {
				if (loop->melting) {
					if (fctr) {
						yprod = xprod = (double) loop->radius \
							*(double) (loop->nsteps - loop->melting) \
							/(double) (loop->nsteps);
					} else {
						yprod = xprod = (double) loop->radius \
							*(double) (loop->melting) \
							/(double) (loop->nsteps);
					}
				} else {
					xprod = yprod = (double) loop->radius;
				}
				while (xctr--)
					xprod *= sin(lf[fctr]->xcoeff[xctr] * loop->theta);
				while (yctr--)
					yprod *= sin(lf[fctr]->ycoeff[yctr] * loop->phi);
				xsum += xprod;
				ysum += yprod;
			}
		}
		if ((loop->nfuncs > 1) && (!loop->melting)) {
			xsum /= (double) loop->nfuncs;
			ysum /= (double) loop->nfuncs;
		}
		xsum += (double) loop->center.x;
		ysum += (double) loop->center.y;

		np[pctr].x = (int) ceil(xsum);
		np[pctr].y = (int) ceil(ysum);
	}
	if (loop->melting) {
		if (!--loop->melting) {
			loop->nfuncs = 1;
			loop->function[0] = loop->function[1];
		}
	}
	for (pctr = 0; pctr < loop->nsteps; pctr++) {

#if defined DRAWLINES
		/* erase the last cycle's point */
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), \
			  MI_GC(mi), lp[pctr].x, lp[pctr].y, \
			  lp[(pctr + 1) % loop->nsteps].x, \
			  lp[(pctr + 1) % loop->nsteps].y);

		/* Set the new color */
		lisasetcolor();

		/* plot this cycle's point */
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), \
			  MI_GC(mi), np[pctr].x, np[pctr].y, \
			  np[(pctr + 1) % loop->nsteps].x, \
			  np[(pctr + 1) % loop->nsteps].y);
#else
		/* erase the last cycle's point */
		XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
		XDrawPoint(MI_DISPLAY(mi), MI_WINDOW(mi), \
			   MI_GC(mi), lp[pctr].x, lp[pctr].y);

		/* Set the new color */
		lisasetcolor();

		/* plot this cycle's point */
		XDrawPoint(MI_DISPLAY(mi), MI_WINDOW(mi), \
			   MI_GC(mi), np[pctr].x, np[pctr].y);
#endif
	}
	(void) free((void *) lp);
	loop->lastpoint = np;
}

static void
initlisa(ModeInfo * mi, lisas * loop)
{
	lisacons   *lc = &Lisa[MI_SCREEN(mi)];
	lisafuncs **lf = loop->function;
	XPoint     *lp;
	int         phase, pctr, fctr, xctr, yctr;
	double      xprod, yprod, xsum, ysum;

	if (MI_NPIXELS(mi) > 2) {
		loop->color = 0;
	} else
		loop->color = MI_WIN_WHITE_PIXEL(mi);
	loop->nsteps = MI_CYCLES(mi);
  if (loop->nsteps == 0)
		loop->nsteps = 1;
	lc->maxcycles = (MAXCYCLES * loop->nsteps) - 1;
	loop->melting = 0;
	loop->nfuncs = 1;
	loop->pistep = 2.0 * M_PI / (double) loop->nsteps;
	loop->center.x = lc->width / 2;
	loop->center.y = lc->height / 2;
	loop->radius = MI_SIZE(mi);
	checkRadius(loop, lc);
	loop->dx = NRAND(XVMAX);
	loop->dy = NRAND(YVMAX);
	loop->dx++;
	loop->dy++;
	lf[0] = &Function[lc->loopcount % NUMSTDFUNCS];
	if ((lp = loop->lastpoint = (XPoint *)
		 calloc(loop->nsteps, sizeof (XPoint))) == NULL)
		return;
	phase = lc->loopcount % loop->nsteps;

	for (pctr = 0; pctr < loop->nsteps; pctr++) {
		loop->phi = (double) (pctr - phase) * loop->pistep;
		loop->theta = (double) (pctr + phase) * loop->pistep;
		fctr = loop->nfuncs;
		xsum = ysum = 0.0;
		while (fctr--) {
			xprod = yprod = (double) loop->radius;
			xctr = lf[fctr]->nx;
			yctr = lf[fctr]->ny;
			while (xctr--)
				xprod *= sin(lf[fctr]->xcoeff[xctr] * loop->theta);
			while (yctr--)
				yprod *= sin(lf[fctr]->ycoeff[yctr] * loop->phi);
			xsum += xprod;
			ysum += yprod;
		}
		if (loop->nfuncs > 1) {
			xsum /= 2.0;
			ysum /= 2.0;
		}
		xsum += (double) loop->center.x;
		ysum += (double) loop->center.y;

		lp[pctr].x = (int) ceil(xsum);
		lp[pctr].y = (int) ceil(ysum);
	}
	for (pctr = 0; pctr < loop->nsteps; pctr++) {
		/* Set the color */
		lisasetcolor();
#if defined DRAWLINES
		XDrawLine(MI_DISPLAY(mi), MI_WINDOW(mi), \
			  MI_GC(mi), lp[pctr].x, lp[pctr].y, \
			  lp[(pctr + 1) % loop->nsteps].x, \
			  lp[(pctr + 1) % loop->nsteps].y);
#else
		XDrawPoint(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi), \
			   lp[pctr].x, lp[pctr].y);
#endif
	}
}

void
init_lisa(ModeInfo * mi)
{
	lisacons   *lc;
	int         lctr;

	if (Lisa == NULL) {
		if ((Lisa = (lisacons *) calloc(MI_NUM_SCREENS(mi), sizeof (lisacons))) \
		    == NULL)
			return;
	}
	lc = &Lisa[MI_SCREEN(mi)];
	lc->width = MI_WIN_WIDTH(mi);
	lc->height = MI_WIN_HEIGHT(mi);
	lc->nlisajous = MI_BATCHCOUNT(mi);
	lc->loopcount = 0;

	if (lc->lisajous == NULL) {
		if ((lc->lisajous = (lisas *) calloc(lc->nlisajous, sizeof (lisas))) \
		    == NULL)
			return;
		XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
		for (lctr = 0; lctr < lc->nlisajous; lctr++) {
			initlisa(mi, &lc->lisajous[lctr]);
			lc->loopcount++;
		}
	} else {
		refresh_lisa(mi);
	}
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_lisa(ModeInfo * mi)
{
	lisacons   *lc = &Lisa[MI_SCREEN(mi)];

	if (++lc->loopcount > lc->maxcycles) {
		change_lisa(mi);
	}
	refresh_lisa(mi);
}

void
refresh_lisa(ModeInfo * mi)
{
	lisacons   *lc = &Lisa[MI_SCREEN(mi)];
	int         lctr;

	for (lctr = 0; lctr < lc->nlisajous; lctr++) {
		drawlisa(mi, &lc->lisajous[lctr]);
	}
}

void
release_lisa(ModeInfo * mi)
{
	lisacons   *lc;
	int         lctr, sctr;

	if (Lisa) {
		for (sctr = 0; sctr < MI_NUM_SCREENS(mi); sctr++) {
			lc = &Lisa[sctr];
			while (lc->lisajous) {
				for (lctr = 0; lctr < lc->nlisajous; lctr++) {
					(void) free(lc->lisajous[lctr].lastpoint);
				}
				(void) free(lc->lisajous);
				lc->lisajous = NULL;
			}
		}
		(void) free(Lisa);
		Lisa = NULL;
	}
}

void
change_lisa(ModeInfo * mi)
{
	lisacons   *lc = &Lisa[MI_SCREEN(mi)];
	lisas      *loop;
	int         lctr;

	lc->loopcount = 0;
	for (lctr = 0; lctr < lc->nlisajous; lctr++) {
		loop = &lc->lisajous[lctr];
		loop->function[1] = &Function[(loop->function[0]->index + 1) %
			NUMSTDFUNCS];
		loop->melting = loop->nsteps - 1;
		loop->nfuncs = 2;
	}
}
