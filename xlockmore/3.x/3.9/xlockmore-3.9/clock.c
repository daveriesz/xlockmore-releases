#ifndef lint
static char sccsid[] = "@(#)clock.c	3.9 96/05/25 xlockmore";

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
} t_clock;

static t_clock myclock[MAXSCREENS];

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
HourToAngle(void)
{
	return ((M_PI * ((double) myclock[screen].dd_time.tm_hour +
			 ((double) (myclock[screen].dd_time).tm_min / 60) +
		     ((double) myclock[screen].dd_time.tm_sec / 3600))) / 6 -
		M_PI_2);
}

static double
MinutesToAngle(void)
{
	return ((M_PI * ((double) (myclock[screen].dd_time.tm_min) +
		    ((double) (myclock[screen].dd_time.tm_sec) / 60))) / 30 -
		M_PI_2);
}

static double
SecondsToAngle(void)
{
	return ((M_PI * ((double) (myclock[screen].dd_time.tm_sec)) / 30) - M_PI_2);
}

static void
DrawDisk(Window win, int center_x, int center_y, int diameter)
{
	int         radius = diameter / 2;

	(void) XFillArc(dsp, win, Scr[screen].gc,
			center_x - radius, center_y - radius,
			diameter, diameter,
			0, 23040);
}

static void
DrawBorder(Window win, short int mode)
{
	if (mode == 0)
		(void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	else
		(void) XSetForeground(dsp, Scr[screen].gc, myclock[screen].border_color);

	(void) XSetLineAttributes(dsp, Scr[screen].gc,
				  myclock[screen].border_width, LineSolid,
				  CapNotLast, JoinMiter);
	(void) XDrawArc(dsp, win, Scr[screen].gc,
			myclock[screen].x_pos - (myclock[screen].size / 2),
			myclock[screen].y_pos - (myclock[screen].size / 2),
			myclock[screen].size, myclock[screen].size,
			0, 23040);
}

static void
DrawJewel(Window win, short int mode)
{
	XSetLineAttributes(dsp, Scr[screen].gc, 1, LineSolid,
			   CapNotLast, JoinMiter);
	if (mode == 0)
		(void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	else
		(void) XSetForeground(dsp, Scr[screen].gc, myclock[screen].jewel_color);
	DrawDisk(win, myclock[screen].x_pos,
		 myclock[screen].y_pos - myclock[screen].jewel_position,
		 myclock[screen].jewel_size);
}

static void
DrawHand(Window win, hand * h, double angle, short int mode)
{
	/* mode: 0 for erasing, anything else for drawing */
	if (mode == 0)
		(void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	else
		(void) XSetForeground(dsp, Scr[screen].gc, h->color);
	XSetLineAttributes(dsp, Scr[screen].gc, h->width, LineSolid,
			   CapNotLast, JoinMiter);
	DrawDisk(win, myclock[screen].x_pos, myclock[screen].y_pos, h->width);
	(void) XDrawLine(dsp, win, Scr[screen].gc,
			 myclock[screen].x_pos, myclock[screen].y_pos,
	     myclock[screen].x_pos + (int) (cos(angle) * (double) (h->size)),
	    myclock[screen].y_pos + (int) (sin(angle) * (double) (h->size)));
	DrawDisk(win,
	     myclock[screen].x_pos + (int) (cos(angle) * (double) (h->size)),
	     myclock[screen].y_pos + (int) (sin(angle) * (double) (h->size)),
		 h->width);
}

static void
real_draw_clock(Window win, short int mode)
{
	DrawBorder(win, mode);
	DrawJewel(win, mode);
	DrawHand(win, &myclock[screen].hours, HourToAngle(), mode);
	DrawHand(win, &myclock[screen].minutes, MinutesToAngle(), mode);
	DrawHand(win, &myclock[screen].seconds, SecondsToAngle(), mode);
}

static void
new_clock_state(ModeInfo * mi)
{
	/* We change the clock size */
	myclock[screen].size = myrand(min_size, max_size);

	/* We must compute new attributes for the clock because its size changes */
	myclock[screen].hours.size = (myclock[screen].size * HOURS_SIZE) / 200;
	myclock[screen].minutes.size = (myclock[screen].size * MINUTES_SIZE) / 200;
	myclock[screen].seconds.size = (myclock[screen].size * SECONDS_SIZE) / 200;
	myclock[screen].jewel_size = (myclock[screen].size * JEWEL_SIZE) / 200;

	myclock[screen].border_width = (myclock[screen].size * CLOCK_BORDER) / 200;
	myclock[screen].hours.width = (myclock[screen].size * HOURS_WIDTH) / 200;
	myclock[screen].minutes.width = (myclock[screen].size * MINUTES_WIDTH) / 200;
	myclock[screen].seconds.width = (myclock[screen].size * SECONDS_WIDTH) / 200;

	myclock[screen].jewel_position = (myclock[screen].size * JEWEL_POSITION) / 200;

	myclock[screen].radius =
		(myclock[screen].size / 2) + myclock[screen].border_width + 1;

	min_x = myclock[screen].radius;
	max_x = myclock[screen].width - myclock[screen].radius;
	min_y = myclock[screen].radius;
	max_y = myclock[screen].height - myclock[screen].radius;

	/* A new position for the clock */
	myclock[screen].x_pos = myrand(min_x, max_x);
	myclock[screen].y_pos = myrand(min_y, max_y);

	/* Here we set new values for the next clock to be displayed */
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		/* New colors */
		myclock[screen].border_color =
			Scr[screen].pixels[LRAND() % Scr[screen].npixels];
		myclock[screen].jewel_color =
			Scr[screen].pixels[LRAND() % Scr[screen].npixels];
		myclock[screen].hours.color =
			Scr[screen].pixels[LRAND() % Scr[screen].npixels];
		myclock[screen].minutes.color =
			Scr[screen].pixels[LRAND() % Scr[screen].npixels];
		myclock[screen].seconds.color =
			Scr[screen].pixels[LRAND() % Scr[screen].npixels];
	}
}

static void
update_clock(Window win)
{
	DrawHand(win, &myclock[screen].hours, HourToAngle(), 0);
	DrawHand(win, &myclock[screen].minutes, MinutesToAngle(), 0);
	DrawHand(win, &myclock[screen].seconds, SecondsToAngle(), 0);
	myclock[screen].d_time = time((time_t *) 0);
	(void) memcpy((char *) &myclock[screen].dd_time,
		      (char *) localtime(&myclock[screen].d_time),
		      sizeof (myclock[screen].dd_time));
	DrawHand(win, &myclock[screen].hours, HourToAngle(), 1);
	DrawHand(win, &myclock[screen].minutes, MinutesToAngle(), 1);
	DrawHand(win, &myclock[screen].seconds, SecondsToAngle(), 1);
}

void
init_clock(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);

	myclock[screen].width = MI_WIN_WIDTH(mi);
	myclock[screen].height = MI_WIN_HEIGHT(mi);
	(void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	(void) XFillRectangle(dsp, win, Scr[screen].gc,
			0, 0, myclock[screen].width, myclock[screen].height);

	min_size = (CLOCK_MIN_SIZE *
		    (myclock[screen].width + myclock[screen].height)) / 200;
	max_size = (CLOCK_MAX_SIZE *
		    (myclock[screen].width + myclock[screen].height)) / 200;

	myclock[screen].clock_count = MI_CYCLES(mi);
	if (myclock[screen].clock_count < MIN_CYCLES)
		myclock[screen].clock_count = MIN_CYCLES;
	if (myclock[screen].clock_count > MAX_CYCLES)
		myclock[screen].clock_count = MAX_CYCLES;

	if (MI_BATCHCOUNT(mi) >= 100)
		myclock[screen].size = (CLOCK_MAX_SIZE * myclock[screen].height) / 100;
	else
		myclock[screen].size = (MI_BATCHCOUNT(mi) * myclock[screen].height) / 100;

	/* By default, we set the clock's colors to white */
	myclock[screen].border_color = WhitePixel(dsp, screen);
	myclock[screen].jewel_color = WhitePixel(dsp, screen);
	myclock[screen].hours.color = WhitePixel(dsp, screen);
	myclock[screen].minutes.color = WhitePixel(dsp, screen);
	myclock[screen].seconds.color = WhitePixel(dsp, screen);

	myclock[screen].d_time = time((time_t *) 0);
	(void) memcpy((char *) &myclock[screen].dd_time,
		      (char *) localtime(&myclock[screen].d_time),
		      sizeof (myclock[screen].dd_time));
	new_clock_state(mi);
}

void
draw_clock(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);

	if (++myclock[screen].clock_count >= MI_CYCLES(mi)) {
		myclock[screen].clock_count = 0;
		real_draw_clock(win, 0);
		new_clock_state(mi);
		real_draw_clock(win, 1);
	} else if (myclock[screen].d_time != time((time_t *) 0))
		update_clock(win);
}
