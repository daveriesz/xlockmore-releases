#ifndef lint
static char sccsid[] = "@(#)clock.c	2.10 95/06/17 xlockmore";
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
 * 01-Jun-95: Written.
 */

#include "xlock.h"
#include <time.h>
#include <math.h>

#define CLOCK_SIZE     30 /* Percentage of the screen height for the clock */
#define CLOCK_BORDER    9 /* Percentage of the clock size for the border */
#define HOURS_SIZE     50 /* Percentage of the clock size for hours length */
#define MINUTES_SIZE   75 /* Idem for minutes */
#define SECONDS_SIZE   80 /* Idem for seconds */
#define HOURS_WIDTH    10 /* Percentage of the clock size for hours width */
#define MINUTES_WIDTH   5 /* Idem for minutes */
#define SECONDS_WIDTH   2 /* Idem for seconds */
#define JEWEL_POSITION 90 /* The relative position of the jewel */
#define JEWEL_SIZE      9 /* The relative size of the jewel */

typedef struct {
  int size;            /* length of the hand */
  int width;           /* width of the hand */
  unsigned long color; /* color of the hand */
} hand;

typedef struct {
  hand hours;          /* the hours hand */
  hand minutes;        /* the minutes hand */
  hand seconds;        /* the seconds hand */

  int border_width;
  unsigned long border_color;

  int jewel_size;
  int jewel_position;
  unsigned long jewel_color;

  int size;
  int radius;
  int boxsize;
  int x_pos;
  int y_pos;

  time_t d_time;
  struct tm *dd_time;
} t_clock;

static int clock_count = 50;

static t_clock myclock;

static time_t heure;           /* Current time */

static int min_x;
static int max_x;
static int min_y;
static int max_y;

static int
myrand (minimum, maximum)
     int minimum, maximum;
{
  return((int)(minimum + (LRAND()/MAXRAND)*(maximum + 1 - minimum)));
}

static double
HourToAngle() {
  return((M_PI*((double)myclock.dd_time->tm_hour +
		((double)(myclock.dd_time)->tm_min / 60) +
		((double)myclock.dd_time->tm_sec / 3600))) / 6 - M_PI_2);
}

static double
MinutesToAngle() {
  return((M_PI*((double)(myclock.dd_time->tm_min)+
		((double)(myclock.dd_time->tm_sec) / 60)))/30 - M_PI_2);
}

static double
SecondsToAngle() {
  return((M_PI*((double)(myclock.dd_time->tm_sec))/30) - M_PI_2);
}

static void
DrawDisk(win, center_x, center_y, diameter)
     Window win;
     int center_x, center_y, diameter;
{
  int radius = diameter / 2;
  (void)XFillArc(dsp, win, Scr[screen].gc,
		 center_x - radius, center_y - radius,
		 diameter,diameter,
		 0,23040);
}

static void
DrawBorder(win)
     Window win;
{
  (void) XSetForeground(dsp, Scr[screen].gc, myclock.border_color);
  (void) XSetLineAttributes(dsp, Scr[screen].gc,
			    myclock.border_width, LineSolid,
			    CapNotLast, JoinMiter);
  (void) XDrawArc(dsp, win, Scr[screen].gc,
		  myclock.x_pos - (myclock.size / 2),
		  myclock.y_pos - (myclock.size / 2),
		  myclock.size, myclock.size,
		  0,23040);
}

static void
DrawJewel(win)
     Window win;
{
  (void) XSetForeground(dsp, Scr[screen].gc, myclock.jewel_color);
  DrawDisk(win, myclock.x_pos,
	   myclock.y_pos - myclock.jewel_position,
	   myclock.jewel_size);
}

static void
DrawHand(win, h, angle, mode)
     Window win;
     hand *h;
     double angle;
     short mode;
{
  /* mode: 0 for erasing, anything else for drawing */
  if (mode == 0)
    (void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp,screen));
  else
    (void) XSetForeground(dsp, Scr[screen].gc, h->color);
  XSetLineAttributes(dsp, Scr[screen].gc, h->width, LineSolid,
		     CapNotLast, JoinMiter);
  DrawDisk(win, myclock.x_pos, myclock.y_pos, h->width);
  (void) XDrawLine(dsp, win, Scr[screen].gc,
		   myclock.x_pos, myclock.y_pos,
		   myclock.x_pos + (int)(cos(angle)*(double)(h->size)),
		   myclock.y_pos + (int)(sin(angle)*(double)(h->size)));
  DrawDisk(win,
	   myclock.x_pos + (int)(cos(angle)*(double)(h->size)),
	   myclock.y_pos + (int)(sin(angle)*(double)(h->size)),
	   h->width);
}

static void
real_draw_clock(win, mode)
     Window win;
     short mode;
{
  /* Modes: 0 for erasing, 1 for updating, 2 for drawing all. */
  if (mode == 0) {
    (void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc,
		   myclock.x_pos - myclock.radius,
		   myclock.y_pos - myclock.radius,
		   myclock.radius * 2,
		   myclock.radius * 2);
    myclock.x_pos = myrand(min_x,max_x);
    myclock.y_pos = myrand(min_y,max_y);
    if (!mono && Scr[screen].npixels > 2) {
      myclock.border_color  = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
      myclock.jewel_color   = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
      myclock.hours.color   = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
      myclock.minutes.color = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
      myclock.seconds.color = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
    }
  }
  else {
    if (mode != 1) {
      /* We redraw even the border and the jewel */
      DrawBorder(win);
      DrawJewel(win);
    }
    DrawHand(win, &myclock.hours,   HourToAngle(),    0);
    DrawHand(win, &myclock.minutes, MinutesToAngle(), 0);
    DrawHand(win, &myclock.seconds, SecondsToAngle(), 0);
    myclock.d_time  = time(0);
    myclock.dd_time = localtime(&myclock.d_time);
    DrawHand(win, &myclock.hours,   HourToAngle(),    1);
    DrawHand(win, &myclock.minutes, MinutesToAngle(), 1);
    DrawHand(win, &myclock.seconds, SecondsToAngle(), 1);
  }
}

void
initclock(win)
     Window win;
{
  XWindowAttributes xgwa;
  (void) XGetWindowAttributes(dsp, win, &xgwa);
  (void) XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp,screen));
  (void) XFillRectangle(dsp, win, Scr[screen].gc,
			0,0,xgwa.width, xgwa.height);

  if ((cycles < 50) || (cycles > 100))
    cycles = 50;
  clock_count = cycles;

  if (batchcount >= 100)
    batchcount = CLOCK_SIZE;
  myclock.size = (batchcount * xgwa.height) / 100;

  myclock.hours.size   = (myclock.size * HOURS_SIZE)   / 200;
  myclock.minutes.size = (myclock.size * MINUTES_SIZE) / 200;
  myclock.seconds.size = (myclock.size * SECONDS_SIZE) / 200;
  myclock.jewel_size   = (myclock.size * JEWEL_SIZE)   / 200;

  myclock.border_width  = (myclock.size * CLOCK_BORDER)  / 200;
  myclock.hours.width   = (myclock.size * HOURS_WIDTH)   / 200;
  myclock.minutes.width = (myclock.size * MINUTES_WIDTH) / 200;
  myclock.seconds.width = (myclock.size * SECONDS_WIDTH) / 200;

  myclock.border_color  = WhitePixel(dsp,screen);
  myclock.jewel_color   = WhitePixel(dsp,screen);
  myclock.hours.color   = WhitePixel(dsp,screen);
  myclock.minutes.color = WhitePixel(dsp,screen);
  myclock.seconds.color = WhitePixel(dsp,screen);

  myclock.jewel_position = (myclock.size * JEWEL_POSITION) /200;

  myclock.radius = (myclock.size / 2) + myclock.border_width;

  min_x = myclock.radius;
  max_x = xgwa.width - myclock.radius;
  min_y = myclock.radius;
  max_y = xgwa.height - myclock.radius;

  myclock.x_pos = myrand(min_x, max_x);
  myclock.y_pos = myrand(min_y,max_y);

  myclock.d_time  = time(0);
  myclock.dd_time = localtime(&myclock.d_time);
}

void
drawclock(win)
     Window win;
{
  heure = time(0);
  if (++clock_count >= cycles) {
    clock_count = 0;
    real_draw_clock(win, 0);
    real_draw_clock(win, 2);
  }
  else if (heure != myclock.d_time) {
    real_draw_clock(win, 1);
  }
}
