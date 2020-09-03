#ifndef lint
static char sccsid[] = "@(#)flag.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * flag.c - PEtite demo X11 de charles vidal 15 05 96
 *          <charles@cln46fw.der.edf.fr>
 *          tourne sous Linux et SOLARIS
 *          thank's to Bas van Gaalen, Holland, PD, for his sources
 *          in pascal vous devez rajouter une ligne dans mode.c
 *
 * See xlock.c for copying information.
 *
 * Revision History: 
 * 01-May-96: written.
 */

#include <math.h>
#include "xlock.h"
#include "flag.h"

#define MAXSCALE 7.5
#define MINSCALE 2.0
#define MAXINITSIZE 6.0
#define MININITSIZE 2.0
#define MINAMP 5
#define MAXAMP 20
#define MAXW		((int) (MAXSCALE * flag_width) + 2 * MAXAMP + MAXSCALE - 1)
#define MAXH		((int) (MAXSCALE * flag_height) + 2 * MAXAMP + MAXSCALE - 1)
#define MINW		((int) (MINSCALE * flag_width) + 2 * MINAMP)
#define MINH		((int) (MINSCALE * flag_height) + 2 * MINAMP)
#define SLEN		255

ModeSpecOpt flag_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         samp;
	int         sofs;
	int         sidx;
	int         x_flag, y_flag;
	int         timer;
	int         initialized;
	int         stab[SLEN];
	Pixmap      cache;
	int         width, height;
	double      size;
	double      inctaille;
} flagstruct;

static flagstruct *flags = NULL;

static int
random_num(int n)
{
	return ((int) (((double) LRAND() / MAXRAND) * (n + 1.0)));
}

static void
initSintab(ModeInfo * mi)
{
	flagstruct *fp = &flags[MI_SCREEN(mi)];
	int         i;

	for (i = 0; i < SLEN; i++)
		fp->stab[i] = (int) (sin(i * 4 * M_PI / SLEN) * fp->samp) + fp->sofs;
}

void
init_flag(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	flagstruct *fp;

	if (flags == NULL) {
		if ((flags = (flagstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (flagstruct))) == NULL)
			return;
	}
	fp = &flags[MI_SCREEN(mi)];

	fp->samp = MAXAMP;	/* Amplitude */
	fp->sofs = 20;		/* ???????? */
	fp->size = MAXINITSIZE;		/* Initial distance between pts */
	fp->inctaille = 0.05;
	fp->timer = 0;
	fp->sidx = fp->x_flag = fp->y_flag = 0;

	if (!fp->initialized) {
		fp->initialized = True;
		if (!(fp->cache = XCreatePixmap(display, MI_WINDOW(mi),
					      MAXW, MAXH, MI_WIN_DEPTH(mi))))
			error("%s: catastrophe memoire\n");
	}
	XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, fp->cache, MI_GC(mi), 0, 0, MAXW, MAXH);

	fp->width = MI_WIN_WIDTH(mi);
	fp->height = MI_WIN_HEIGHT(mi);

	if (fp->width <= MAXW || fp->height <= MAXH) {
		fp->samp = MINAMP;
		fp->sofs = 0;
		fp->x_flag = random_num(fp->width - MINW);
		fp->y_flag = random_num(fp->height - MINH);
	} else {
		fp->samp = MAXAMP;
		fp->sofs = 20;
		fp->x_flag = random_num(fp->width - MAXW);
		fp->y_flag = random_num(fp->height - MAXH);
	}

	initSintab(mi);

	XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, MI_WINDOW(mi), MI_GC(mi),
		       0, 0, fp->width, fp->height);
}

static void
affiche(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	int         x, y, xp, yp, indice = 0;
	flagstruct *fp = &flags[MI_SCREEN(mi)];

	for (x = 0; x < flag_width; x++)
		for (y = flag_height; y > 0; y--) {
			xp = (int) (fp->size * (double) x) +
				fp->stab[(fp->sidx + x + y) % SLEN];
			yp = (int) (fp->size * (double) y) +
				fp->stab[(fp->sidx + 4 * x + y + y) % SLEN];
#ifdef INVERSE
			if (flag_bits[indice])
#else
			if (!flag_bits[indice])
#endif
				XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
			else if (MI_WIN_IS_MONO(mi) || MI_NPIXELS(mi) <= 2)
				XSetForeground(display, MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
			else
				XSetForeground(display, MI_GC(mi),
				MI_PIXEL(mi, (y + x + fp->sidx) % NUMCOLORS));
			if (fp->size <= 2.0)
			  XDrawPoint(display, fp->cache, MI_GC(mi), xp, yp);
			else
			  XFillRectangle(display, fp->cache, MI_GC(mi), xp, yp,
					fp->size - 1, fp->size - 1);
			indice++;
		}
}

void
draw_flag(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	flagstruct *fp = &flags[MI_SCREEN(mi)];

	if (fp->width <= MAXW || fp->height <= MAXH) {
		fp->size = MININITSIZE;
		XCopyArea(display, fp->cache, window, MI_GC(mi), 0, 0, MINW, MINH,
			  fp->x_flag, fp->y_flag);
	} else {
		if ((fp->size + fp->inctaille) > MAXSCALE)
			fp->inctaille = -fp->inctaille;
		if ((fp->size + fp->inctaille) < MINSCALE)
			fp->inctaille = -fp->inctaille;
		fp->size += fp->inctaille;
		XCopyArea(display, fp->cache, window, MI_GC(mi),
			  0, 0, MAXW, MAXH, fp->x_flag, fp->y_flag);
	}
	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, fp->cache, MI_GC(mi), 0, 0, MAXW, MAXH);
	XFlush(display);
	affiche(mi);
	fp->sidx += 2;
	fp->sidx %= 255;
	XFlush(display);
	fp->timer++;
	if (fp->timer >= MI_CYCLES(mi))
		init_flag(mi);
}

void
release_flag(ModeInfo * mi)
{
	if (flags != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			if (flags[screen].cache)
				XFreePixmap(MI_DISPLAY(mi), flags[screen].cache);
		(void) free((void *) flags);
		flags = NULL;
	}
}
