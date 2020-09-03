
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)flag.c	4.02 97/04/01 xlockmore";

#endif

/*-
 * flag.c - PEtite demo X11 de charles vidal 15 05 96
 *          <vidalc@univ-mlv.fr>
 *          tourne sous Linux et SOLARIS
 *          thank's to Bas van Gaalen, Holland, PD, for his sources
 *          in pascal vous devez rajouter une ligne dans mode.c
 *
 * See xlock.c for copying information.
 *
 * Revision History: 
 * 01-May-96: written.
 */

#include "xlock.h"
#include "flag.h"

#define MINSIZE 1
#define MAXSCALE 8
#define MINSCALE 2
#define MAXINITSIZE 6
#define MININITSIZE 2
#define MINAMP 5
#define MAXAMP 20
#define MAXW(s)		(MAXSCALE * flag_width + 2 * MAXAMP + s)
#define MAXH(s)		(MAXSCALE * flag_height + 2 * MAXAMP + s)
#define MINW(s)		(MINSCALE * flag_width + 2 * MINAMP + s)
#define MINH(s)		(MINSCALE * flag_height + 2 * MINAMP + s)
#define ANGLES		360

ModeSpecOpt flag_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	int         samp;
	int         sofs;
	int         sidx;
	int         x_flag, y_flag;
	int         timer;
	int         initialized;
	int         stab[ANGLES];
	Pixmap      cache;
	int         width, height;
	int         pointsize;
	double      size;
	double      inctaille;
	int         startcolor;
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

	for (i = 0; i < ANGLES; i++)
		fp->stab[i] = (int) (sin(i * 4 * M_PI / ANGLES) * fp->samp) + fp->sofs;
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
				fp->stab[(fp->sidx + x + y) % ANGLES];
			yp = (int) (fp->size * (double) y) +
				fp->stab[(fp->sidx + 4 * x + y + y) % ANGLES];
#ifdef INVERSE
			if (flag_bits[indice])
#else
			if (!flag_bits[indice])
#endif
				XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
			else if (MI_NPIXELS(mi) <= 2)
				XSetForeground(display, MI_GC(mi), MI_WIN_WHITE_PIXEL(mi));
			else
				XSetForeground(display, MI_GC(mi),
					       MI_PIXEL(mi, (y + x + fp->sidx + fp->startcolor) % MI_NPIXELS(mi)));
			if (fp->pointsize <= 1)
				XDrawPoint(display, fp->cache, MI_GC(mi), xp, yp);
			else
				XFillRectangle(display, fp->cache, MI_GC(mi), xp, yp,
					       fp->pointsize, fp->pointsize);
			indice++;
		}
}

void
init_flag(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	int         size = MI_SIZE(mi);
	flagstruct *fp;

	if (flags == NULL) {
		if ((flags = (flagstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (flagstruct))) == NULL)
			return;
	}
	fp = &flags[MI_SCREEN(mi)];

	fp->width = MI_WIN_WIDTH(mi);
	fp->height = MI_WIN_HEIGHT(mi);

	fp->samp = MAXAMP;	/* Amplitude */
	fp->sofs = 20;		/* ???????? */
	fp->pointsize = size;
	if (size < -MINSIZE)
		fp->pointsize = NRAND(-size - MINSIZE + 1) + MINSIZE;
	if (fp->pointsize < MINSIZE ||
	fp->width <= MAXW(fp->pointsize) || fp->height <= MAXH(fp->pointsize))
		fp->pointsize = MINSIZE;
	fp->size = MAXINITSIZE;	/* Initial distance between pts */
	fp->inctaille = 0.05;
	fp->timer = 0;
	fp->sidx = fp->x_flag = fp->y_flag = 0;

	if (!fp->initialized) {
		fp->initialized = True;
		if (!(fp->cache = XCreatePixmap(display, MI_WINDOW(mi),
		MAXW(fp->pointsize), MAXH(fp->pointsize), MI_WIN_DEPTH(mi))))
			error("%s: catastrophe memoire\n");
	}
	XSetForeground(display, MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, fp->cache, MI_GC(mi),
		       0, 0, MAXW(fp->pointsize), MAXH(fp->pointsize));
	/* don't want any exposure events from XCopyArea */
	XSetGraphicsExposures(display, MI_GC(mi), False);
	if (MI_NPIXELS(mi) > 2)
		fp->startcolor = NRAND(MI_NPIXELS(mi));
	if (fp->width <= MAXW(fp->pointsize) || fp->height <= MAXH(fp->pointsize)) {
		fp->samp = MINAMP;
		fp->sofs = 0;
		fp->x_flag = random_num(fp->width - MINW(fp->pointsize));
		fp->y_flag = random_num(fp->height - MINH(fp->pointsize));
	} else {
		fp->samp = MAXAMP;
		fp->sofs = 20;
		fp->x_flag = random_num(fp->width - MAXW(fp->pointsize));
		fp->y_flag = random_num(fp->height - MAXH(fp->pointsize));
	}

	initSintab(mi);

	XClearWindow(display, MI_WINDOW(mi));
}

void
draw_flag(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	flagstruct *fp = &flags[MI_SCREEN(mi)];

	if (fp->width <= MAXW(fp->pointsize) || fp->height <= MAXH(fp->pointsize)) {
		fp->size = MININITSIZE;
		/* fp->pointsize = MINPOINTSIZE; */
		XCopyArea(display, fp->cache, window, MI_GC(mi),
			  0, 0, MINW(fp->pointsize), MINH(fp->pointsize), fp->x_flag, fp->y_flag);
	} else {
		if ((fp->size + fp->inctaille) > MAXSCALE)
			fp->inctaille = -fp->inctaille;
		if ((fp->size + fp->inctaille) < MINSCALE)
			fp->inctaille = -fp->inctaille;
		fp->size += fp->inctaille;
		XCopyArea(display, fp->cache, window, MI_GC(mi),
			  0, 0, MAXW(fp->pointsize), MAXH(fp->pointsize), fp->x_flag, fp->y_flag);
	}
	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(display, fp->cache, MI_GC(mi),
		       0, 0, MAXW(fp->pointsize), MAXH(fp->pointsize));
	XFlush(display);
	affiche(mi);
	fp->sidx += 2;
	fp->sidx %= (ANGLES * MI_NPIXELS(mi));
	XFlush(display);
	fp->timer++;
	if ((MI_CYCLES(mi) > 0) && (fp->timer >= MI_CYCLES(mi)))
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

void
refresh_flag(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
