#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)tri.c	4.00 97/01/01 xlockmore";

#endif

/* 
 * tri.c - Sierpinski triangle fractal for xlock,
 *   the X Window System lockscreen.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 05-Sep-96: Desmond Daignault Datatimes Incorporated
 *            <tekdd@dtol.datatimes.com> .
 */
#include "xlock.h"

typedef struct {
	int         width, height;
	int         time;
	int         px, py;
	int         total_npoints;
	int         npoints[3];
	unsigned long colors[3];
	XPoint     *pointBuffer[3];
	XPoint      vertex[3];
} tristruct;

ModeSpecOpt tri_opts =
{0, NULL, 0, NULL, NULL};

static tristruct *tris = NULL;

static void
startover(ModeInfo * mi)
{
	int         j;
	tristruct  *tp = &tris[MI_SCREEN(mi)];

	if (MI_NPIXELS(mi) > 2) {
		tp->colors[0] = (NRAND(MI_NPIXELS(mi)));
		tp->colors[1] = (tp->colors[0] + MI_NPIXELS(mi) / 7 +
			     NRAND(2 * MI_NPIXELS(mi) / 7)) % MI_NPIXELS(mi);
		tp->colors[2] = (tp->colors[0] + 4 * MI_NPIXELS(mi) / 7 +
			     NRAND(2 * MI_NPIXELS(mi) / 7)) % MI_NPIXELS(mi);
	}
	for (j = 0; j < 3; j++) {
		tp->vertex[j].x = NRAND(tp->width);
		tp->vertex[j].y = NRAND(tp->height);
	}
	tp->px = NRAND(tp->width);
	tp->py = NRAND(tp->height);
	tp->time = 0;
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
init_tri(ModeInfo * mi)
{
	tristruct  *tp;
	int         i;

	if (tris == NULL) {
		if ((tris = (tristruct *) calloc(MI_NUM_SCREENS(mi),
						 sizeof (tristruct))) == NULL)
			return;
	}
	tp = &tris[MI_SCREEN(mi)];

	tp->width = MI_WIN_WIDTH(mi);
	tp->height = MI_WIN_HEIGHT(mi);

	tp->total_npoints = MI_BATCHCOUNT(mi);
	if (tp->total_npoints < 1)
		tp->total_npoints = 1;
	for (i = 0; i < 3; i++) {
		if (!tp->pointBuffer[i])
			tp->pointBuffer[i] = (XPoint *) malloc(tp->total_npoints *
							    sizeof (XPoint));
	}
	startover(mi);
}

void
draw_tri(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	tristruct  *tp = &tris[MI_SCREEN(mi)];
	XPoint     *xp[3];
	int         i = 0, v;

	if (MI_NPIXELS(mi) <= 2)
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	for (i = 0; i < 3; i++)
		xp[i] = tp->pointBuffer[i];
	for (i = 0; i < tp->total_npoints; i++) {
		v = NRAND(3);
		tp->px = (tp->px + tp->vertex[v].x) / 2;
		tp->py = (tp->py + tp->vertex[v].y) / 2;
		xp[v]->x = tp->px;
		xp[v]->y = tp->py;
		xp[v]++;
		tp->npoints[v]++;
	}
	for (i = 0; i < 3; i++) {
		if (MI_NPIXELS(mi) > 2)
			XSetForeground(display, gc, MI_PIXEL(mi, tp->colors[i]));
		XDrawPoints(display, MI_WINDOW(mi), gc, tp->pointBuffer[i], tp->npoints[i],
			    CoordModeOrigin);
		tp->npoints[i] = 0;
	}
	if (++tp->time >= MI_CYCLES(mi))
		startover(mi);
}

void
release_tri(ModeInfo * mi)
{
	if (tris != NULL) {
		int         screen, i;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			for (i = 0; i < 3; i++)
				if (tris[screen].pointBuffer[i] != NULL) {
					(void) free((void *) tris[screen].pointBuffer[i]);
				}
		}
		(void) free((void *) tris);
		tris = NULL;
	}
}

void
refresh_tri(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
