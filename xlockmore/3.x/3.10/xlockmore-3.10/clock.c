
#ifndef lint
static char sccsid[] = "@(#)clock.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * clock.c - displays an analog clock moving around the screen.
 * The design was highly inspirated from 'oclock' but not the code,
 * which is all mine.
 *
 * Copyright (c) 1995 by Jeremie PETIT <petit@aurora.unice.fr>
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 24-Dec-95: Ron Hitchens <ron@utw.com> cycles range and defaults
 *            changed.
 * 01-Dec-95: Clock size changes each time it is displayed.
 * 01-Jun-95: Written.
 */

#include "xlock.h"
#include <time.h>
#include <math.h>

#define MIN_CYCLES     50	/* shortest possible time before moving the clock */
#define MAX_CYCLES   1000	/* longest possible time before moving the clock */
#define CLOCK_MAX_SIZE 50	/* Percentage of the screen height for the clock */
#define CLOCK_MIN_SIZE  5	/* size, maximum and minimum */
#define CLOCK_BORDER    9	/* Percentage of the clock size for the border */
#define HOURS_SIZE     50	/* Percentage of the clock size for hours length */
#define MINUTES_SIZE   75	/* Idem for minutes */
#define SECONDS_SIZE   80	/* Idem for seconds */
#define HOURS_WIDTH    10	/* Percentage of the clock size for hours width */
#define MINUTES_WIDTH   5	/* Idem for minutes */
#define SECONDS_WIDTH   2	/* Idem for seconds */
#define JEWEL_POSITION 90	/* The relative position of the jewel */
#define JEWEL_SIZE      9	/* The relative size of the jewel */

ModeSpecOpt clock_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         size;	/* length of the hand */
	int         width;	/* width of the hand */
	unsigned long color;	/* color of the hand */
} hand;

typedef struct {
	hand        hours;	/* the hours hand */
	hand        minutes;	/* the minutes hand */
	hand        seconds;	/* the seconds hand */

	int         border_width;
	unsigned long border_color;

	int         jewel_size;
	int         jewel_position;
	unsigned long jewel_color;

	int         size;
	int         radius;
	int         boxsize;
	int         x_pos;
	int         y_pos;

	long        d_time;
	struct tm   dd_time;
	int         clock_count;
	int         width, height;
} clockstruct;

static clockstruct *oclock = NULL;

static int  min_x;
static int  max_x;
static int  min_y;
static int  max_y;
static int  min_size;
static int  max_size;

static int
myrand(int minimum, int maximum)
{
	return ((int) (minimum + (LRAND() / MAXRAND) * (maximum + 1 - minimum)));
}

static double
HourToAngle(clockstruct * cp)
{
	return ((M_PI * ((double) cp->dd_time.tm_hour +
			 ((double) cp->dd_time.tm_min / 60) +
			 ((double) cp->dd_time.tm_sec / 3600))) / 6 - M_PI_2);
}

static double
MinutesToAngle(clockstruct * cp)
{
	return ((M_PI * ((double) cp->dd_time.tm_min +
			 ((double) cp->dd_time.tm_sec / 60))) / 30 - M_PI_2);
}

static double
SecondsToAngle(clockstruct * cp)
{
	return ((M_PI * ((double) cp->dd_time.tm_sec) / 30) - M_PI_2);
}

static void
DrawDisk(Display * display, Window window, GC gc,
	 int center_x, int center_y, int diameter)
{
	int         radius = diameter / 2;

	(void) XFillArc(display, window, gc,
			center_x - radius, center_y - radius,
			diameter, diameter,
			0, 23040);
}

static void
DrawBorder(ModeInfo * mi, short int mode)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	clockstruct *cp = &oclock[MI_SCREEN(mi)];


	if (mode == 0)
		(void) XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	else
		(void) XSetForeground(display, gc, cp->border_color);

	(void) XSetLineAttributes(display, gc, cp->border_width,
				  LineSolid, CapNotLast, JoinMiter);
	(void) XDrawArc(display, MI_WINDOW(mi), gc,
			cp->x_pos - cp->size / 2, cp->y_pos - cp->size / 2, cp->size, cp->size,
			0, 23040);
}

static void
DrawJewel(ModeInfo * mi, short int mode)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	clockstruct *cp = &oclock[MI_SCREEN(mi)];

	XSetLineAttributes(display, gc, 1, LineSolid, CapNotLast, JoinMiter);
	if (mode == 0)
		(void) XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	else
		(void) XSetForeground(display, gc, cp->jewel_color);
	DrawDisk(display, MI_WINDOW(mi), gc,
		 cp->x_pos, cp->y_pos - cp->jewel_position, cp->jewel_size);
}

static void
DrawHand(ModeInfo * mi, hand * h, double angle, short int mode)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	clockstruct *cp = &oclock[MI_SCREEN(mi)];
	int         cosi, sinj;

	/* mode: 0 for erasing, anything else for drawing */
	if (mode == 0)
		(void) XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
	else
		(void) XSetForeground(display, gc, h->color);
	XSetLineAttributes(display, gc, h->width, LineSolid, CapNotLast, JoinMiter);
	DrawDisk(display, window, gc, cp->x_pos, cp->y_pos, h->width);
	cosi = (int) (cos(angle) * (double) (h->size));
	sinj = (int) (sin(angle) * (double) (h->size));
	XDrawLine(display, window, gc,
		  cp->x_pos, cp->y_pos, cp->x_pos + cosi, cp->y_pos + sinj);
	DrawDisk(display, window, gc,
		 cp->x_pos + cosi, cp->y_pos + sinj, h->width);
}

static void
real_draw_clock(ModeInfo * mi, short int mode)
{
	clockstruct *cp = &oclock[MI_SCREEN(mi)];

	DrawBorder(mi, mode);
	DrawJewel(mi, mode);
	DrawHand(mi, &cp->hours, HourToAngle(cp), mode);
	DrawHand(mi, &cp->minutes, MinutesToAngle(cp), mode);
	DrawHand(mi, &cp->seconds, SecondsToAngle(cp), mode);
}

static void
new_clock_state(ModeInfo * mi)
{
	clockstruct *cp = &oclock[MI_SCREEN(mi)];

	/* We change the clock size */
	cp->size = myrand(min_size, max_size);

	/* We must compute new attributes for the clock because its size changes */
	cp->hours.size = (cp->size * HOURS_SIZE) / 200;
	cp->minutes.size = (cp->size * MINUTES_SIZE) / 200;
	cp->seconds.size = (cp->size * SECONDS_SIZE) / 200;
	cp->jewel_size = (cp->size * JEWEL_SIZE) / 200;

	cp->border_width = (cp->size * CLOCK_BORDER) / 200;
	cp->hours.width = (cp->size * HOURS_WIDTH) / 200;
	cp->minutes.width = (cp->size * MINUTES_WIDTH) / 200;
	cp->seconds.width = (cp->size * SECONDS_WIDTH) / 200;

	cp->jewel_position = (cp->size * JEWEL_POSITION) / 200;

	cp->radius =
		(cp->size / 2) + cp->border_width + 1;

	min_x = cp->radius;
	max_x = cp->width - cp->radius;
	min_y = cp->radius;
	max_y = cp->height - cp->radius;

	/* A new position for the clock */
	cp->x_pos = myrand(min_x, max_x);
	cp->y_pos = myrand(min_y, max_y);

	/* Here we set new values for the next clock to be displayed */
	if (MI_NPIXELS(mi) > 2) {
		/* New colors */
		cp->border_color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
		cp->jewel_color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
		cp->hours.color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
		cp->minutes.color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
		cp->seconds.color = MI_PIXEL(mi, NRAND(MI_NPIXELS(mi)));
	}
}

static void
update_clock(ModeInfo * mi)
{
	clockstruct *cp = &oclock[MI_SCREEN(mi)];

	DrawHand(mi, &cp->hours, HourToAngle(cp), 0);
	DrawHand(mi, &cp->minutes, MinutesToAngle(cp), 0);
	DrawHand(mi, &cp->seconds, SecondsToAngle(cp), 0);
	cp->d_time = time((time_t *) 0);
	(void) memcpy((char *) &cp->dd_time,
		      (char *) localtime(&cp->d_time),
		      sizeof (cp->dd_time));
	DrawHand(mi, &cp->hours, HourToAngle(cp), 1);
	DrawHand(mi, &cp->minutes, MinutesToAngle(cp), 1);
	DrawHand(mi, &cp->seconds, SecondsToAngle(cp), 1);
}

void
init_clock(ModeInfo * mi)
{
	clockstruct *cp;

	if (oclock == NULL) {
		if ((oclock = (clockstruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (clockstruct))) == NULL)
			return;
	}
	cp = &oclock[MI_SCREEN(mi)];

	cp->width = MI_WIN_WIDTH(mi);
	cp->height = MI_WIN_HEIGHT(mi);

	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

	min_size = (CLOCK_MIN_SIZE *
		    (cp->width + cp->height)) / 200;
	max_size = (CLOCK_MAX_SIZE *
		    (cp->width + cp->height)) / 200;

	cp->clock_count = MI_CYCLES(mi);
	if (cp->clock_count < MIN_CYCLES)
		cp->clock_count = MIN_CYCLES;
	if (cp->clock_count > MAX_CYCLES)
		cp->clock_count = MAX_CYCLES;

	if (MI_BATCHCOUNT(mi) >= 100)
		cp->size = (CLOCK_MAX_SIZE * cp->height) / 100;
	else
		cp->size = (MI_BATCHCOUNT(mi) * cp->height) / 100;

	/* By default, we set the clock's colors to white */
	cp->border_color = MI_WIN_WHITE_PIXEL(mi);
	cp->jewel_color = MI_WIN_WHITE_PIXEL(mi);
	cp->hours.color = MI_WIN_WHITE_PIXEL(mi);
	cp->minutes.color = MI_WIN_WHITE_PIXEL(mi);
	cp->seconds.color = MI_WIN_WHITE_PIXEL(mi);

	cp->d_time = time((time_t *) 0);
	(void) memcpy((char *) &cp->dd_time,
		      (char *) localtime(&cp->d_time),
		      sizeof (cp->dd_time));
	new_clock_state(mi);
}

void
draw_clock(ModeInfo * mi)
{
	clockstruct *cp = &oclock[MI_SCREEN(mi)];

	if (++cp->clock_count >= MI_CYCLES(mi)) {
		cp->clock_count = 0;
		real_draw_clock(mi, 0);
		new_clock_state(mi);
		real_draw_clock(mi, 1);
	} else if (cp->d_time != time((time_t *) 0))
		update_clock(mi);
}

void
release_clock(ModeInfo * mi)
{
	if (oclock != NULL) {
		(void) free((void *) oclock);
		oclock = NULL;
	}
}
