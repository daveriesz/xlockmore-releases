#ifndef lint
static char sccsid[] = "@(#)lissie.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * lissie.c - The Lissajous worm by Alexander Jolk
 *            <ub9x@rz.uni-karlsruhe.de>
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 01-May-96: written.
 */

#include "xlock.h"
#include <math.h>

#define Lissie(n)\
  if (lissie->xs[(n)] >= 0 && lissie->ys[(n)] >= 0 \
      && lissie->xs[(n)] <= gp->width && lissie->ys[(n)] <= gp->height) {\
    if (lissie->ri < 2)\
     XDrawPoint(dsp, win, Scr[screen].gc, lissie->xs[(n)], lissie->ys[(n)]);\
    else\
     XDrawArc(dsp, win, Scr[screen].gc,\
       lissie->xs[(n)] - lissie->ri / 2, lissie->ys[(n)] - lissie->ri / 2,\
       lissie->ri, lissie->ri,\
       0, 23040);\
   }

#define FLOATRAND(min,max)	((min)+(LRAND()/MAXRAND)*((max)-(min)))
#define INTRAND(min,max)     ((min)+(LRAND()%((max)-(min)+1)))

#define MINDT  0.01
#define MAXDT  0.15

#define MAXLISSIELEN  100
#define MINLISSIELEN  10


ModeSpecOpt lissie_opts =
{0, NULL, NULL, NULL};

typedef struct {
	double
	            tx, ty, dtx, dty;
	int
	            xi, yi, ri, rx, ry, len, pos, xs[MAXLISSIELEN], ys[MAXLISSIELEN];
	int         color;
} lissiestruct;

typedef struct {
	int         width, height;
	int         nlissies;
	lissiestruct *lissie;
} lissstruct;

static lissstruct lisss[MAXSCREENS];

static int  loopcount;

static void initlissie(ModeInfo * mi, lissiestruct * lissie);
static void drawlissie(ModeInfo * mi, lissiestruct * lissie);

void
init_lissie(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	lissstruct *gp = &lisss[screen];
	unsigned char ball;

	gp->width = MI_WIN_WIDTH(mi);
	gp->height = MI_WIN_HEIGHT(mi);

	gp->nlissies = MI_BATCHCOUNT(mi);
	if (gp->nlissies < 0)
		gp->nlissies = 1;
	else if (gp->nlissies > 20)
		gp->nlissies = 3;

	loopcount = 0;

	if (!gp->lissie)
		gp->lissie = (lissiestruct *) calloc(gp->nlissies, sizeof (lissiestruct));
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, gp->width, gp->height);
	for (ball = 0; ball < (unsigned char) gp->nlissies; ball++)
		initlissie(mi, &gp->lissie[ball]);

}

static void
initlissie(ModeInfo * mi, lissiestruct * lissie)
{
	lissstruct *gp = &lisss[screen];

	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
		lissie->color = LRAND() % Scr[screen].npixels;
	else
		lissie->color = WhitePixel(dsp, screen);
	/* Initialize parameters */
	lissie->ri = INTRAND(0, min(gp->width, gp->height) / 8);
	lissie->xi = INTRAND(gp->width / 4 + lissie->ri,
			     gp->width * 3 / 4 - lissie->ri);
	lissie->yi = INTRAND(gp->height / 4 + lissie->ri,
			     gp->height * 3 / 4 - lissie->ri);
	lissie->rx = INTRAND(gp->width / 4,
		   min(gp->width - lissie->xi, lissie->xi)) - 2 * lissie->ri;
	lissie->ry = INTRAND(gp->height / 4,
		  min(gp->height - lissie->yi, lissie->yi)) - 2 * lissie->ri;
	lissie->len = INTRAND(MINLISSIELEN, MAXLISSIELEN - 1);
	lissie->pos = 0;

	lissie->tx = FLOATRAND(0, 2 * M_PI);
	lissie->ty = FLOATRAND(0, 2 * M_PI);
	lissie->dtx = FLOATRAND(MINDT, MAXDT);
	lissie->dty = FLOATRAND(MINDT, MAXDT);

	/* Draw lissie */
	drawlissie(mi, lissie);
}

void
draw_lissie(ModeInfo * mi)
{
	lissstruct *gp = &lisss[screen];
	register unsigned char ball;

	if (++loopcount > MI_CYCLES(mi))
		init_lissie(mi);
	else
		for (ball = 0; ball < (unsigned char) gp->nlissies; ball++)
			drawlissie(mi, &gp->lissie[ball]);
}

static void
drawlissie(ModeInfo * mi, lissiestruct * lissie)
{
	Window      win = MI_WINDOW(mi);
	lissstruct *gp = &lisss[screen];
	int         p = (++lissie->pos) % MAXLISSIELEN;
	int         oldp = (lissie->pos - lissie->len + MAXLISSIELEN) % MAXLISSIELEN;

	/* Let time go by ... */
	lissie->tx += lissie->dtx;
	lissie->ty += lissie->dty;
	if (lissie->tx > 2 * M_PI)
		lissie->tx -= 2 * M_PI;
	if (lissie->ty > 2 * M_PI)
		lissie->ty -= 2 * M_PI;

	/* slightly vary both (x/y) speeds for amusement */
	lissie->dtx += FLOATRAND(-MINDT / 5.0, MINDT / 5.0);
	lissie->dty += FLOATRAND(-MINDT / 5.0, MINDT / 5.0);
	if (lissie->dtx < MINDT)
		lissie->dtx = MINDT;
	else if (lissie->dtx > MAXDT)
		lissie->dtx = MAXDT;
	if (lissie->dty < MINDT)
		lissie->dty = MINDT;
	else if (lissie->dty > MAXDT)
		lissie->dty = MAXDT;

	lissie->xs[p] = lissie->xi + sin(lissie->tx) * lissie->rx;
	lissie->ys[p] = lissie->yi + sin(lissie->ty) * lissie->ry;

	/* Mask */
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	Lissie(oldp);

	/* Redraw */
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[lissie->color]);
		if (++lissie->color >= Scr[screen].npixels)
			lissie->color = 0;
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	Lissie(p);
}
