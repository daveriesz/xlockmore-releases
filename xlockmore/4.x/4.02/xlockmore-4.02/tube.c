
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)tube.c 4.02 97/04/01 xlockmore";

#endif

/*-
 * tube.c - animated tube for xlock
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 4-Mar-97: Memory leak fix by Tom Schmidt <tschmidt@micron.com>
 * 7-Feb-97: Written by Dan Stromberg <strombrg@nis.acs.uci.edu>
 */
#include "xlock.h"

#define MINSIZE 3

ModeSpecOpt tube_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	int         dx1, dy1;
	int         x1, y1;
	int         width;
	int         height;
	unsigned int cur_color;
	Colormap    cmap;
	XColor     *colors;
	XColor      top;
	XColor      bottom;
	int         counter;
} tubestruct;

static tubestruct *tubes = NULL;

void
init_tube(ModeInfo * mi)
{
	tubestruct *tubep;
	int         screen_width, screen_height;
	int         size = MI_SIZE(mi);

	if (tubes == NULL) {
		if ((tubes = (tubestruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (tubestruct))) == NULL)
			return;
	}
	tubep = &tubes[MI_SCREEN(mi)];

	screen_width = MI_WIN_WIDTH(mi);
	screen_height = MI_WIN_HEIGHT(mi);

	if (size < -MINSIZE) {
		tubep->width = NRAND(MIN(-size, MAX(MINSIZE, screen_width / 2)) -
				     MINSIZE + 1) + MINSIZE;
		tubep->height = NRAND(MIN(-size, MAX(MINSIZE, screen_height / 2)) -
				      MINSIZE + 1) + MINSIZE;
	} else if (size < MINSIZE) {
		if (!size) {
			tubep->width = MAX(MINSIZE, screen_width / 2);
			tubep->height = MAX(MINSIZE, screen_height / 2);
		} else {
			tubep->width = MINSIZE;
			tubep->height = MINSIZE;
		}
	} else {
		tubep->width = MIN(size, MAX(MINSIZE, screen_width / 2));
		tubep->height = MIN(size, MAX(MINSIZE, screen_height / 2));
	}

	tubep->dx1 = NRAND(2);
	if (tubep->dx1 == 0)
		tubep->dx1 = -1;
	tubep->dy1 = NRAND(2);
	if (tubep->dy1 == 0)
		tubep->dy1 = -1;
	tubep->x1 = NRAND(screen_width - tubep->width);
	tubep->y1 = NRAND(screen_height - tubep->height);
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	tubep->counter = 0;
	if (!MI_WIN_IS_INROOT(mi) && MI_WIN_IS_INSTALL(mi) && MI_NPIXELS(mi) > 2) {
		/* color in "the bottom third */
		tubep->bottom.red = NRAND(65536 / 3);
		tubep->bottom.blue = NRAND(65536 / 3);
		tubep->bottom.green = NRAND(65536 / 3);
		/* color in "the top third */
		tubep->top.red = NRAND(65536 / 3) + 65536 * 2 / 3;
		tubep->top.blue = NRAND(65536 / 3) + 65536 * 2 / 3;
		tubep->top.green = NRAND(65536 / 3) + 65536 * 2 / 3;

		/* allocate colormap, if needed */
		if (tubep->colors == NULL)
			tubep->colors = (XColor *) malloc(sizeof (XColor) * MI_NPIXELS(mi));
		if (tubep->cmap == None)
			tubep->cmap = XCreateColormap(MI_DISPLAY(mi),
				     MI_WINDOW(mi), MI_VISUAL(mi), AllocAll);
	}
}

void
draw_tube(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	tubestruct *tubep = &tubes[MI_SCREEN(mi)];
	unsigned int i, j;
	int         k;

	/* advance drawing color */
	tubep->cur_color = (tubep->cur_color + 1) % MI_NPIXELS(mi);
	if (!MI_WIN_IS_INROOT(mi) && MI_WIN_IS_INSTALL(mi) && MI_NPIXELS(mi) > 2) {
		while (tubep->cur_color == MI_WIN_WHITE_PIXEL(mi) ||
		       tubep->cur_color == MI_WIN_BLACK_PIXEL(mi)) {
			tubep->cur_color = (tubep->cur_color + 1) % MI_NPIXELS(mi);
		}
	}
	XSetForeground(display, gc, tubep->cur_color);

	/* move rectangle forward, horiz */
	tubep->x1 += tubep->dx1;
	if (tubep->x1 < 0) {
		tubep->x1 = 0;
		tubep->dx1 = -tubep->dx1;
	}
	if (tubep->x1 + tubep->width >= MI_WIN_WIDTH(mi)) {
		tubep->x1 = MI_WIN_WIDTH(mi) - tubep->width - 1;
		tubep->dx1 = -tubep->dx1;
	}
	/* move rectange forward, vert */
	tubep->y1 += tubep->dy1;
	if (tubep->y1 < 0) {
		tubep->y1 = 0;
		tubep->dy1 = -tubep->dy1;
	}
	if (tubep->y1 + tubep->height >= MI_WIN_HEIGHT(mi)) {
		tubep->y1 = MI_WIN_HEIGHT(mi) - tubep->height - 1;
		tubep->dy1 = -tubep->dy1;
	}
	/* draw the rectangle */
	XDrawLine(display, MI_WINDOW(mi), gc,
		  tubep->x1, tubep->y1,
		  tubep->x1, tubep->y1 + tubep->height);
	XDrawLine(display, MI_WINDOW(mi), gc,
		  tubep->x1, tubep->y1 + tubep->height,
		  tubep->x1 + tubep->width, tubep->y1 + tubep->height);
	XDrawLine(display, MI_WINDOW(mi), gc,
		  tubep->x1 + tubep->width, tubep->y1 + tubep->height,
		  tubep->x1 + tubep->width, tubep->y1);
	XDrawLine(display, MI_WINDOW(mi), gc,
		  tubep->x1 + tubep->width, tubep->y1,
		  tubep->x1, tubep->y1);

	/* advance colormap */
	if (!MI_WIN_IS_INROOT(mi) && MI_WIN_IS_INSTALL(mi) && MI_NPIXELS(mi) > 2) {
		for (i = 0, j = tubep->cur_color, k = 0;
		     i < (unsigned int) MI_NPIXELS(mi); i++) {
			if (i == MI_WIN_WHITE_PIXEL(mi)) {
				tubep->colors[i].pixel = i;
				tubep->colors[i].red = 65535;
				tubep->colors[i].blue = 65535;
				tubep->colors[i].green = 65535;
				tubep->colors[i].flags = DoRed | DoGreen | DoBlue;
			} else if (i == MI_WIN_BLACK_PIXEL(mi)) {
				tubep->colors[i].pixel = i;
				tubep->colors[i].red = 0;
				tubep->colors[i].blue = 0;
				tubep->colors[i].green = 0;
				tubep->colors[i].flags = DoRed | DoGreen | DoBlue;
			} else {
				double      range;

				tubep->colors[i].pixel = i;
				range = ((double) (tubep->top.red - tubep->bottom.red)) /
					MI_NPIXELS(mi);
				tubep->colors[i].red = (short unsigned int) (range * j +
							  tubep->bottom.red);
				range = ((double) (tubep->top.green - tubep->bottom.green)) /
					MI_NPIXELS(mi);
				tubep->colors[i].green = (short unsigned int) (range * j +
							tubep->bottom.green);
				range = ((double) (tubep->top.blue - tubep->bottom.blue)) /
					MI_NPIXELS(mi);
				tubep->colors[i].blue = (short unsigned int) (range * j +
							 tubep->bottom.blue);
				tubep->colors[i].flags = DoRed | DoGreen | DoBlue;
				j = (j + 1) % MI_NPIXELS(mi);
				k++;
			}
		}
		/* make the entire tube move forward */
		XStoreColors(display, tubep->cmap, tubep->colors, MI_NPIXELS(mi));
		setColormap(display, MI_WINDOW(mi), tubep->cmap, MI_WIN_IS_INWINDOW(mi));
	}
	tubep->counter++;
	if (tubep->counter > MI_CYCLES(mi)) {
		init_tube(mi);
	}
}

void
release_tube(ModeInfo * mi)
{
	if (tubes != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			tubestruct *tubep = &tubes[screen];

			if (tubep->cmap != None)
				XFreeColormap(MI_DISPLAY(mi), tubep->cmap);
			if (tubep->colors != NULL)
				XFree((void *) tubep->colors);
		}
		(void) free((void *) tubes);
		tubes = NULL;
	}
}
