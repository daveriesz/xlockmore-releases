/* squiral --- squiraling worms */

#if 0
static const char sccsid[] = "@(#)squiral.c  5.87 2026/01/03 xlockmore";
#endif

/*-
 * squiral, by "Jeff Epler" <jepler AT inetnebr.com>, 18-mar-1999.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * Revision History:
 * 05-Jan-2026: ported to xlock from xscreensaver, updated for readability as
 *		wanted to add features.  Fixed horizontal curtains and added
 *		hexagons and diamonds and vertical curtains.
 *
 */

#ifdef STANDALONE
#define MODE_squiral
#define DEFAULTS "*delay:	10000 \n" \
  "*fpsSolid:	true  \n" \
  "*count:	0 \n" \
  "*ncolors:	100 \n" \

#define free_squiral 0
#define UNIFORM_COLORS
#include "xlockmore.h"	/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"	/* in xlockmore distribution */
#include "color.h"
#include "iostuff.h"
#endif /* STANDALONE */
#include "automata.h"

#ifdef MODE_squiral

#define FLOATRAND ((double) LRAND() / ((double) MAXRAND))
#define R(x)  NRAND(x)
#define PROB(x) (FLOATRAND < (x))

/* square */
/* 0- 3 left-winding  */
/* 4- 7 right-winding */

#define DEF_DISORDER "0.005"
#define DEF_HANDEDNESS  "0.5" /* no preference */
#define DEF_SCALE  "1" /* smallest possible */
#define DEF_CYCLE "False" /* color cycle */
#define DEF_FILL  "0.75"
#define DEF_SHAPES  "False" /* squares or diamonds and hexagons */
#define DEF_VERTICAL  "False" /* what the curtains? */

static float disorder;
static float handedness;
static int  scale;
static float fill;
static Bool cycle;
static Bool shapes;
static Bool vertical;

static XrmOptionDescRec opts[] =
{
	{(char *) "-disorder", (char *) ".squiral.disorder", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-handedness", (char *) ".squiral.handedness", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-scale", (char *) ".squiral.scale", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-fill", (char *) ".squiral.fill", XrmoptionSepArg, (caddr_t) NULL},
	{(char *) "-cycle", (char *) ".squiral.cycle", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+cycle", (char *) ".squiral.cycle", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-shapes", (char *) ".squiral.shapes", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+shapes", (char *) ".squiral.shapes", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-vertical", (char *) ".squiral.vertical", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+vertical", (char *) ".squiral.vertical", XrmoptionNoArg, (caddr_t) "off"}
};
static argtype vars[] =
{
	{(void *) & disorder, (char *) "disorder", (char *) "Disorder", (char *) DEF_DISORDER, t_Float},
	{(void *) & handedness, (char *) "handedness", (char *) "Handedness", (char *) DEF_HANDEDNESS, t_Float},
	{(void *) & scale, (char *) "scale", (char *) "Scale", (char *) DEF_SCALE, t_Int},
	{(void *) & fill, (char *) "fill", (char *) "Fill", (char *) DEF_FILL, t_Float},
	{(void *) & cycle, (char *) "cycle", (char *) "Cycle", (char *) DEF_CYCLE, t_Bool},
	{(void *) & shapes, (char *) "shapes", (char *) "Shapes", (char *) DEF_SHAPES, t_Bool},
	{(void *) & vertical, (char *) "vertical", (char *) "Vertical", (char *) DEF_VERTICAL, t_Bool}
};
static OptionStruct desc[] =
{
	{(char *) "-disorder num", (char *) "chance of disorder"},
	{(char *) "-handedness num", (char *) "prefer left or right"},
	{(char *) "-scale num", (char *) "size of boxes"},
	{(char *) "-/+cycle", (char *) "cycle the colors"},
	{(char *) "-/+shapes", (char *) "squares or diamonds and hexagons"},
	{(char *) "-/+vertical", (char *) "curtains"}
};

ENTRYPOINT ModeSpecOpt squiral_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
const ModStruct squiral_description =
{"squiral", "init_squiral", "draw_squiral", "release_squiral",
 "init_squiral", "init_squiral", (char *) NULL, &squiral_opts,
 10000, 0, 50000, -3, 64, 1.0, "",
 "Shows spiral-producing automata", 0, NULL};

#endif

typedef struct {
	int horiz;
	int vert;
	int state;
	int color;
	int cycleColor;
	int neighbors;
	int shape;
} wormstruct;

typedef struct {
	int width, height, count, cycle;
	double frac, disorder, handedness;
	int ncolors;

	int cov;
	int dirHoriz[4][6];
	int dirVert[4][6];

	int *fill;

	wormstruct *worms;
	int inClear;
	int scale;
	Bool shapes;
	Bool vertical;
} squiralstruct;

static squiralstruct *squirals = (squiralstruct *) NULL;


#define CLEAR1(x,y) (!st->fill[((st->vertical)?(((x)%st->width)*st->height+(y)%st->height):(((y)%st->height)*st->width+(x)%st->width))])
#define MOVE1(x,y) (st->fill[((st->vertical)?(((x)%st->width)*st->height+(y)%st->height):(((y)%st->height)*st->width+(x)%st->width))]=1, \
	XFillRectangle(display, window, MI_GC(mi), \
		((x)%st->width)*st->scale, \
		((y)%st->height)*st->scale, \
		st->scale, st->scale), \
		st->cov++)

#define CLEARDXY(x,y,dx,dy) CLEAR1(x+dx, y+dy) && CLEAR1(x+dx*2, y+dy*2)
#define MOVEDXY(x,y,dx,dy) MOVE1(x+dx, y+dy), MOVE1(x+dx*2, y+dy*2)

#define CLEAR(d) CLEARDXY(w->horiz,w->vert, st->dirHoriz[w->shape][d],st->dirVert[w->shape][d])
#define MOVE(d) (MOVEDXY(w->horiz,w->vert, st->dirHoriz[w->shape][d],st->dirVert[w->shape][d]), \
	w->horiz=w->horiz+st->dirHoriz[w->shape][d]*2, \
	w->vert=w->vert+st->dirVert[w->shape][d]*2, dir=d)

#define RANDOM (void) (w->horiz=R(st->width), \
	w->vert=R(st->height), \
	w->color=R(st->ncolors), \
	handType=R(2), \
	w->neighbors=((!st->shapes)?4:((NRAND(3)==0)?4:6)), \
	dir=R(w->neighbors), \
	(st->cycle && (w->cycleColor=R(3)+st->ncolors)))

#define SUCC(x) ((x+1)%w->neighbors)
#define PRED(x) ((x+w->neighbors-1)%w->neighbors)
#define CCW	PRED(dir)
#define CW	SUCC(dir)

/* neighbors = 6 only */
#define SUCC2(x) ((x+2)%w->neighbors)
#define PRED2(x) ((x+w->neighbors-2)%w->neighbors)
#define CCW2	PRED2(dir)
#define CW2	SUCC2(dir)

#define REV	((dir+w->neighbors/2)%w->neighbors)
#define STR	(dir)
#define TRY(x)	if (CLEAR(x)) { MOVE(x); break; }

#define SQUARE 0
#define DIAMOND 1
/*#define SQUARE2 2*/ /* will overlap */
#define HORIZ_HEXAGON 2
#define VERT_HEXAGON 3

static void
fixOffset(squiralstruct *st, wormstruct *w)
{
	if ((st->shapes) && ((w->vert + w->horiz) % 2 != 0)) {
		if (w->horiz > w->vert)
			w->horiz--;
		else
			w->vert--;
	}
}

static void
doWorm(ModeInfo * mi, squiralstruct *st, wormstruct *w)
{
	Display	*display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	int handType = w->state / w->neighbors;
	int dir = w->state % w->neighbors;

	w->color = (w->color + w->cycleColor) % st->ncolors;

	if (PROB(st->disorder))
		handType = PROB(st->handedness);
	if (MI_NPIXELS(mi) >= 2) {
		XSetForeground(display, MI_GC(mi), MI_PIXEL(mi, w->color));
	} else {
		XSetForeground(display, MI_GC(mi), MI_WHITE_PIXEL(mi));
	}
	switch(handType) {
	case 0: /* CCW */
 		/*TRY(CCW2)*/
 		TRY(CCW)
		TRY(STR)
		TRY(CW)
		RANDOM;
		if (!st->shapes)
			w->shape = 0;
		else if (w->neighbors == 4)
			w->shape = 1;
		else
			w->shape = NRAND(2) + 2;
		fixOffset(st, w);
		break;
	case 1: /* CW */
 		/*TRY(CW2)*/
		TRY(CW)
		TRY(STR)
		TRY(CCW)
		RANDOM;
		if (!st->shapes)
			w->shape = 0;
		else if (w->neighbors == 4)
			w->shape = 1;
		else
			w->shape = NRAND(2) + 2;
		fixOffset(st, w);
		break;
	default:
		(void) printf("should not happen\n");
	}
	w->state = handType * w->neighbors + dir;
	w->horiz = w->horiz % st->width;
	w->vert = w->vert % st->height;
}

static void
reset_squiral(squiralstruct *st)
{
	int i;
	if (st->worms)
		free(st->worms);
	if (st->fill)
		free(st->fill);

	st->worms = calloc(st->count, sizeof(wormstruct));

	st->fill = calloc(st->width * st->height, sizeof(int));
	for (i = 0; i < st->count; i++) {
		wormstruct *w = &st->worms[i];
		w->horiz = R(st->width);
		w->vert = R(st->height);
		w->neighbors = ((!st->shapes) ? 4 : ((NRAND(3) == 0) ? 4 : 6));
		w->state = R(w->neighbors) + w->neighbors * PROB(st->handedness);
		w->color = R(st->ncolors);
		if (st->cycle)
			w->cycleColor = R(3) + st->ncolors;
		else
			w->cycleColor = 0;
		if (!st->shapes)
			w->shape = 0;
		else if (w->neighbors == 4)
			w->shape = 1;
		else
			w->shape = NRAND(2) + 2;
		fixOffset(st, w);
	}
}

static void
curtains(ModeInfo * mi, squiralstruct *st)
{
	Display	*display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	GC gc = MI_GC(mi);
	if (st->vertical) {
		if (st->inClear < st->width) {
			XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
			XFillRectangle(display, window, gc,
				st->inClear * st->scale,
				0,
				st->scale,
				st->height * st->scale);
			memset(&st->fill[st->inClear * st->height], 0,
				sizeof(int) * st->height);
			XFillRectangle(display, window, gc,
				(st->width - st->inClear - 1) * st->scale,
				0,
				st->scale,
				st->height * st->scale);
			memset(&st->fill[(st->width - st->inClear - 1) * st->height], 0,
				sizeof(int) * st->height);
			st->inClear++;
			XFillRectangle(display, window, gc,
				st->inClear * st->scale,
				0,
				st->scale,
				st->height * st->scale);
			if (st->inClear < st->width)
				memset(&st->fill[st->inClear * st->height], 0,
					sizeof(int) * st->height);
			XFillRectangle(display, window, gc,
				(st->width - st->inClear - 1) * st->scale,
				0,
				st->scale,
				st->height * st->scale);
			if (st->width - st->inClear >= 1)
				memset(&st->fill[(st->width - st->inClear - 1) * st->height], 0,
					sizeof(int) * st->height);
			st->inClear++;
			if (st->inClear > st->width / 2)
				st->inClear = st->width;
		} else if (st->cov > (st->frac * st->width * st->height)) {
			st->inClear = 0;
			st->cov = 0;
		}
	} else {
		if (st->inClear < st->height) {
			XSetForeground(display, gc, MI_BLACK_PIXEL(mi));
			XFillRectangle(display, window, gc,
				0,
				st->inClear * st->scale,
				st->width * st->scale,
				st->scale);
			memset(&st->fill[st->inClear * st->width], 0,
				sizeof(int) * st->width);
			XFillRectangle(display, window, gc,
				0,
				(st->height - st->inClear - 1) * st->scale,
				st->width * st->scale,
				st->scale);
			memset(&st->fill[(st->height - st->inClear - 1) * st->width], 0,
				sizeof(int) * st->width);
			st->inClear++;
			XFillRectangle(display, window, gc,
				0,
				st->inClear * st->scale,
				st->width * st->scale,
				st->scale);
			if (st->inClear < st->height)
				memset(&st->fill[st->inClear * st->width], 0,
					sizeof(int) * st->width);
			XFillRectangle(display, window, gc,
				0,
				(st->height - st->inClear - 1) * st->scale,
				st->width * st->scale, 
				st->scale);
			if (st->height - st->inClear >= 1)
				memset(&st->fill[(st->height - st->inClear - 1) * st->width], 0,
					sizeof(int) * st->width);
			st->inClear++;
			if (st->inClear > st->height / 2)
				st->inClear = st->height;
		} else if (st->cov > (st->frac * st->width * st->height)) {
			st->inClear = 0;
			st->cov = 0;
		}
	}
}

static void
free_squiral_screen(squiralstruct *st)
{
	if (st == NULL)
		return;
	if (st->worms)
		free(st->worms);
	if (st->fill)
		free(st->fill);
	st = NULL;
}

ENTRYPOINT void
init_squiral(ModeInfo * mi)
{
	Display	*display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	squiralstruct *st;
	XWindowAttributes xgwa;

	MI_INIT(mi, squirals);
	st = &squirals[MI_SCREEN(mi)];

	MI_CLEARWINDOW(mi);
	XGetWindowAttributes(display, window, &xgwa);

	st->scale = scale;
	if (xgwa.width > 2560 || xgwa.height > 2560)
		st->scale *= 3; /* Retina displays */

	st->width = xgwa.width / st->scale;
	st->height = xgwa.height / st->scale;

	if (st->width < 4)
		st->width = 4;
	if (st->height < 4)
		st->height = 4;
	st->frac = fill;
	if (MI_IS_FULLRANDOM(mi)) {
		st->shapes = (NRAND(2) == 1);
		st->cycle = (NRAND(6) == 5);
		st->vertical = (NRAND(2) == 1);
	} else {
		st->shapes = shapes;
		st->cycle = cycle;
		st->vertical = vertical;
	}
	if (st->shapes) {
		st->frac /= 3;
		if ((st->width % 2) == 1)
			st->width--;
		if ((st->height % 2) == 1)
			st->height--;
	}
	st->dirHoriz[SQUARE][0] = 0;
	st->dirHoriz[SQUARE][1] = 1;
	st->dirHoriz[SQUARE][2] = 0;
	st->dirHoriz[SQUARE][3] = st->width - 1;
	st->dirVert[SQUARE][0] = st->height - 1;
	st->dirVert[SQUARE][1] = 0;
	st->dirVert[SQUARE][2] = 1;
	st->dirVert[SQUARE][3] = 0;
	st->dirHoriz[DIAMOND][0] = 1;
	st->dirHoriz[DIAMOND][1] = 1;
	st->dirHoriz[DIAMOND][2] = st->width - 1;
	st->dirHoriz[DIAMOND][3] = st->width - 1;
	st->dirVert[DIAMOND][0] = st->height- 1;
	st->dirVert[DIAMOND][1] = 1;
	st->dirVert[DIAMOND][2] = 1;
	st->dirVert[DIAMOND][3] = st->height - 1;
	/*st->dirHoriz[SQUARE2][0] = 0;
	st->dirHoriz[SQUARE2][1] = 2;
	st->dirHoriz[SQUARE2][2] = 0;
	st->dirHoriz[SQUARE2][3] = st->width - 2;
	st->dirVert[SQUARE2][0] = st->height - 2;
	st->dirVert[SQUARE2][1] = 0;
	st->dirVert[SQUARE2][2] = 2;
	st->dirVert[SQUARE2][3] = 0;*/
	st->dirHoriz[HORIZ_HEXAGON][0] = 1;
	st->dirHoriz[HORIZ_HEXAGON][1] = 2;
	st->dirHoriz[HORIZ_HEXAGON][2] = 1;
	st->dirHoriz[HORIZ_HEXAGON][3] = st->width - 1;
	st->dirHoriz[HORIZ_HEXAGON][4] = st->width - 2;
	st->dirHoriz[HORIZ_HEXAGON][5] = st->width - 1;
	st->dirVert[HORIZ_HEXAGON][0] = st->height - 1;
	st->dirVert[HORIZ_HEXAGON][1] = 0;
	st->dirVert[HORIZ_HEXAGON][2] = 1;
	st->dirVert[HORIZ_HEXAGON][3] = 1;
	st->dirVert[HORIZ_HEXAGON][4] = 0;
	st->dirVert[HORIZ_HEXAGON][5] = st->height - 1;
	st->dirHoriz[VERT_HEXAGON][0] = 0;
	st->dirHoriz[VERT_HEXAGON][1] = 1;
	st->dirHoriz[VERT_HEXAGON][2] = 1;
	st->dirHoriz[VERT_HEXAGON][3] = 0;
	st->dirHoriz[VERT_HEXAGON][4] = st->width - 1;
	st->dirHoriz[VERT_HEXAGON][5] = st->width - 1;
	st->dirVert[VERT_HEXAGON][0] = st->height - 2;
	st->dirVert[VERT_HEXAGON][1] = st->height - 1;
	st->dirVert[VERT_HEXAGON][2] = 1;
	st->dirVert[VERT_HEXAGON][3] = 2;
	st->dirVert[VERT_HEXAGON][4] = 1;
	st->dirVert[VERT_HEXAGON][5] = st->height - 1;
	st->count = MI_COUNT(mi);
	st->disorder = disorder;
	st->handedness = handedness;
	st->ncolors = MI_NPIXELS(mi);

	if (st->frac < 0.01)
		st->frac = 0.01;
	if (st->frac > 0.99)
		st->frac = 0.99;
	if (st->count == 0)
		st->count = (st->width + st->height) / 64;
	if (st->count < 1)
		st->count = 1;
	if (st->count > 1000)
		st->count = 1000;

	st->worms = NULL;
	st->fill = NULL;

	reset_squiral(st);
}

ENTRYPOINT void
draw_squiral(ModeInfo * mi)
{
	squiralstruct *st;
	int i;

	if (squirals == NULL)
		return;
	st = &squirals[MI_SCREEN(mi)];

	curtains(mi, st);
	for (i = 0; i < st->count; i++)
		doWorm(mi, st, &st->worms[i]);
}

#ifdef STANDALONE
ENTRYPOINT void
reshape_squiral(ModeInfo *mi, int width, int height)
{
	Display	*display = MI_DISPLAY(mi);
	Window window = MI_WINDOW(mi);
	squiralstruct *st;

	if (squirals == NULL)
		return;
	st = &squirals[MI_SCREEN(mi)];

	st->scale = scale;
	if (width > 2560 || height > 2560)
		st->scale *= 3; /* Retina displays */

	st->width = width / st->scale;
	st->height = height / st->scale;
	reset_squiral(st);
	XClearWindow(display, window);
}

ENTRYPOINT Bool
squiral_handle_event(ModeInfo *mi, XEvent *event)
{
	squiralstruct *st = &squirals[MI_SCREEN(mi)];
	if (screenhack_event_helper(MI_DISPLAY(mi), MI_WINDOW(mi), event)) {
		reset_squiral(st);
		MI_CLEARWINDOW(mi);
		return True;
	}
	return False;
}
#endif

ENTRYPOINT void
release_squiral(ModeInfo * mi)
{
	if (squirals != NULL) {
		int screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			free_squiral_screen(&squirals[screen]);
		}
		free(squirals);
		squirals = (squiralstruct *) NULL;
	}
}

XSCREENSAVER_MODULE ("Squiral", squiral)

#endif /* MODE_squiral */

