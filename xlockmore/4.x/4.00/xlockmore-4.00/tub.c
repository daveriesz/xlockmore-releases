

/* 
   ** Written Fri Feb  7 16:30:06 PST 1997. */
#include "xlock.h"

ModeSpecOpt tube_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	int         dx1, dy1;
	int         x1, y1;
	int         width;
	int         height;
	int         cur_color;
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
	int         tube_width, tube_height;

	if (tubes == NULL) {
		if ((tubes = (tubestruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (tubestruct))) == NULL)
			return;
	}
	tubep = &tubes[MI_SCREEN(mi)];

	screen_width = MI_WIN_WIDTH(mi);
	screen_height = MI_WIN_HEIGHT(mi);
	tubep->width = NRAND((int) (screen_width * 0.1)) + 5;
	tubep->height = NRAND((int) (screen_height * 0.1)) + 5;

	tubep->dx1 = NRAND(2);
	if (tubep->dx1 == 0)
		tubep->dx1 = -1;
	tubep->dy1 = NRAND(2);
	if (tubep->dy1 == 0)
		tubep->dy1 = -1;
	tubep->x1 = NRAND(screen_width - tubep->width);
	tubep->y1 = NRAND(screen_height - tubep->height);
	tubep->colors = NULL;
	tubep->cmap = 0;
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	tubep->counter = 0;
	/* color in "the bottom third */
	tubep->bottom.red = NRAND(65536 / 3);
	tubep->bottom.blue = NRAND(65536 / 3);
	tubep->bottom.green = NRAND(65536 / 3);
	/* color in "the top third */
	tubep->top.red = NRAND(65536 / 3) + 65536 * 2 / 3;
	tubep->top.blue = NRAND(65536 / 3) + 65536 * 2 / 3;
	tubep->top.green = NRAND(65536 / 3) + 65536 * 2 / 3;
}

void
draw_tube(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	tubestruct *tubep = &tubes[MI_SCREEN(mi)];
	int         i, j, k;

	/* advance drawing color */
	tubep->cur_color = (tubep->cur_color + 1) % MI_NPIXELS(mi);
	if (MI_NPIXELS(mi) > 2) {
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

	/* allocate colormap, if needed */
	if (tubep->colors == 0) {
		tubep->colors = malloc(sizeof (XColor) * MI_NPIXELS(mi));
		if (!MI_WIN_IS_INROOT(mi)) {
			tubep->cmap = XCreateColormap(
							     display,
							     MI_WINDOW(mi),
							     MI_VISUAL(mi),
							     AllocAll);
		}
	}
	/* advance colormap */
	for (i = 0, j = tubep->cur_color, k = 0; i < MI_NPIXELS(mi); i++) {
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
			range = ((double) (tubep->top.red - tubep->bottom.red)) / MI_NPIXELS(mi);
			tubep->colors[i].red = range * j + tubep->bottom.red;
			range = ((double) (tubep->top.green - tubep->bottom.green)) / MI_NPIXELS(mi);
			tubep->colors[i].green = range * j + tubep->bottom.green;
			range = ((double) (tubep->top.blue - tubep->bottom.blue)) / MI_NPIXELS(mi);
			tubep->colors[i].blue = range * j + tubep->bottom.blue;
			tubep->colors[i].flags = DoRed | DoGreen | DoBlue;
			j = (j + 1) % MI_NPIXELS(mi);
			k++;
		}
	}
	/* make the entire tube move forward */
	if (!MI_WIN_IS_INROOT(mi)) {
		XStoreColors(display, tubep->cmap, tubep->colors, MI_NPIXELS(mi));
		setColormap(display, MI_WINDOW(mi), tubep->cmap, MI_WIN_IS_INWINDOW(mi));
	}
	tubep->counter++;
	if (tubep->counter > 20000) {
		init_tube(mi);
	}
}

void
release_tube(ModeInfo * mi)
{
	if (tubes != NULL) {
		int         screen;

		if (!MI_WIN_IS_INROOT(mi)) {
			for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
				tubestruct *tubep = &tubes[MI_SCREEN(mi)];

				if (tubep->cmap != 0)
					XFreeColormap(MI_DISPLAY(mi), tubep->cmap);
			}
		}
		(void) free((void *) tubes);
		tubes = NULL;
	}
}
