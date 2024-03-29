
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)fadeplot.c  4.00 97/01/17 xlockmore";

#endif

/*-
 * fadeplot.c - fadeplot for xlock, the X Window System lockscreen.
 *
 * Some easy plotting stuff, by Bas van Gaalen, Holland, PD
 *
 * Converted for xlock by Charles Vidal
 * See xlock.c for copying information.
 */

/*-
 1) Not random enough, i.e. always same starting position.
 2) Needs to be less flashy
 */

#include "xlock.h"

ModeSpecOpt fadeplot_opts =
{0, NULL, 0, NULL, NULL};

#define MINSTEPS 1
#define ANGLES 1000

typedef struct {
	XPoint      speed, step, factor, st;
	int         temps, maxpts, nbstep;
	int         min;
	int         width, height;
	int         pix;
	int         stab[ANGLES];
	XPoint     *pts;
} fadeplotstruct;

static fadeplotstruct *fadeplots = NULL;

static void
initSintab(ModeInfo * mi)
{
	fadeplotstruct *fp = &fadeplots[MI_SCREEN(mi)];
	int         i;
	float       x;

	for (i = 0; i < ANGLES; i++) {
		x = SINF(i * 2 * M_PI / ANGLES);
		fp->stab[i] = (int) (x * ABS(x) * fp->min) + fp->min;
	}
}

void
init_fadeplot(ModeInfo * mi)
{
	fadeplotstruct *fp;

	if (fadeplots == NULL) {
		if ((fadeplots = (fadeplotstruct *) calloc(MI_NUM_SCREENS(mi),
					   sizeof (fadeplotstruct))) == NULL)
			return;
	}
	fp = &fadeplots[MI_SCREEN(mi)];

	fp->width = MI_WIN_WIDTH(mi);
	fp->height = MI_WIN_HEIGHT(mi);
  fp->min = MAX(MIN(fp->width, fp->height) / 2, 1);

	fp->speed.x = 8;
	fp->speed.y = 10;
	fp->step.x = 1;
	fp->step.y = 1;
	fp->temps = 0;
	fp->factor.x = MAX(fp->width / (2 * fp->min), 1);
	fp->factor.y = MAX(fp->height / (2 * fp->min), 1);

	fp->nbstep = MI_BATCHCOUNT(mi);
	if (fp->nbstep < -MINSTEPS) {
		fp->nbstep = NRAND(-fp->nbstep - MINSTEPS + 1) + MINSTEPS;
	} else if (fp->nbstep < MINSTEPS)
		fp->nbstep = MINSTEPS;

  fp->maxpts = MI_CYCLES(mi);
  if (fp->maxpts < 1)
    fp->maxpts = 1;

	if (fp->pts == NULL)
		fp->pts = (XPoint *) calloc(fp->maxpts, sizeof (XPoint));
	if (MI_NPIXELS(mi) > 2)
		fp->pix = NRAND(MI_NPIXELS(mi));

	initSintab(mi);

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
draw_fadeplot(ModeInfo * mi)
{
	fadeplotstruct *fp = &fadeplots[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	int         i, j;
	long        temp;

	XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	XDrawPoints(display, window, gc, fp->pts, fp->maxpts, CoordModeOrigin);

	if (MI_NPIXELS(mi) > 2) {
		XSetForeground(display, gc, MI_PIXEL(mi, fp->pix));
		if (++fp->pix >= MI_NPIXELS(mi))
			fp->pix = 0;
	} else
		XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));

		for (temp = fp->nbstep - 1; temp >= 0; temp--) {
			j = temp;
			for (i = 0; i < fp->maxpts / fp->nbstep; i++) {
				fp->pts[temp * i + i].x =
					fp->stab[(fp->st.x + fp->speed.x * j + i * fp->step.x) % ANGLES] *
					fp->factor.x + fp->width / 2 - fp->min;
				fp->pts[temp * i + i].y =
					fp->stab[(fp->st.y + fp->speed.y * j + i * fp->step.y) % ANGLES] *
					fp->factor.y + fp->height / 2 - fp->min;
			}
		}
	XDrawPoints(display, window, gc, fp->pts, fp->maxpts, CoordModeOrigin);
	XFlush(display);
	fp->st.x = (fp->st.x + fp->speed.x) % ANGLES;
	fp->st.y = (fp->st.y + fp->speed.y) % ANGLES;
	fp->temps++;
	if ((fp->temps % (ANGLES / 2)) == 0) {
		fp->temps = fp->temps % ANGLES * 5;
		if ((fp->temps % (ANGLES)) == 0)
			fp->speed.y = (fp->speed.y++) % 30 + 1;
		if ((fp->temps % (ANGLES * 2)) == 0)
			fp->speed.x = (fp->speed.x) % 20;
		if ((fp->temps % (ANGLES * 3)) == 0)
			fp->step.y = (fp->step.y++) % 2 + 1;
		XClearWindow(display, window);
	}
}
void
refresh_fadeplot(ModeInfo * mi)
{

}

void
release_fadeplot(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
