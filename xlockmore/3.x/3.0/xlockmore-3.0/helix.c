#ifndef lint
static char sccsid[] = "@(#)helix.c	2.10 95/06/17 xlockmore";
#endif
/*
 * String art
 *
 * Copyright (c) 1992 by Jamie Zawinski
 *
 * 17-Jun-95:  removed sleep statements
 * 2-Sep-93: xlock version (David Bagley bagleyd@source.asset.com)
 * 1992:     xscreensaver version (Jamie Zawinski jwz@lucid.com)
 */

/* original copyright
 * Copyright (c) 1992 by Jamie Zawinski
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <math.h>
#include "xlock.h"

#define ANGLES 360

static double cos_array[ANGLES], sin_array[ANGLES];
typedef struct {
    int	width;
    int	height;
    int xmid, ymid;
    int color;
    int time;
} helixstruct;

static helixstruct helixes[MAXSCREENS];

static void random_helix();

void
inithelix(win)
    Window      win;
{
    XWindowAttributes xgwa;
    helixstruct *hp = &helixes[screen];
    int i;

    (void) XGetWindowAttributes(dsp, win, &xgwa);
    hp->width = xgwa.width;
    hp->height = xgwa.height;
    hp->xmid = hp->width / 2;
    hp->ymid = hp->height / 2;

    XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
    XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, hp->width, hp->height);

    for (i = 0; i < ANGLES; i++) {
      cos_array[i] = cos((((double) i) / (double) (ANGLES / 2)) * M_PI);
      sin_array[i] = sin((((double) i) / (double) (ANGLES / 2)) * M_PI);;
    }

    hp->color = 0;
    hp->time = 0;
    random_helix(win);
}

static int
gcd (a, b)
     int a, b;
{
  while (b > 0) {
      int tmp;

      tmp = a % b;
      a = b;
      b = tmp;
  }
  return (a < 0 ? -a : a);
}

static void
helix (win,
       radius1, radius2, d_angle,
       factor1, factor2, factor3, factor4)
     Window win;
     int radius1, radius2, d_angle;
     int factor1, factor2, factor3, factor4;
{
  helixstruct *hp = &helixes[screen];
  int x_1, y_1, x_2, y_2, angle, limit;
  int i;

  x_1 = hp->xmid;
  y_1 = hp->ymid + radius2;
  x_2 = hp->xmid;
  y_2 = hp->ymid + radius1;
  angle = 0;
  limit = 1 + (ANGLES / gcd (ANGLES, d_angle));

  for (i = 0; i < limit; i++)
    {
      int tmp;
#define pmod(x,y) (tmp=(x%y),(tmp>=0?tmp:tmp+y))
      x_1 = hp->xmid + (((double) radius1) *
        sin_array[pmod ((angle * factor1), ANGLES)]);
      y_1 = hp->ymid + (((double) radius2) *
        cos_array[pmod ((angle * factor2), ANGLES)]);
       if (!mono && Scr[screen].npixels > 2) {
         XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[hp->color]);
         if (++hp->color >= Scr[screen].npixels)
             hp->color = 0;
       }
      XDrawLine (dsp, win, Scr[screen].gc, x_1, y_1, x_2, y_2);
      x_2 = hp->xmid + (((double) radius2) *
        sin_array[pmod ((angle * factor3), ANGLES)]);
      y_2 = hp->ymid + (((double) radius1) *
        cos_array[pmod ((angle * factor4), ANGLES)]);
       if (!mono && Scr[screen].npixels > 2) {
         XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[hp->color]);
         if (++hp->color >= Scr[screen].npixels)
             hp->color = 0;
       }
      XDrawLine (dsp, win, Scr[screen].gc, x_1, y_1, x_2, y_2);
      angle += d_angle;
    }
}

static void
random_helix (win)
     Window win;
{
    helixstruct *hp = &helixes[screen];
  int radius, radius1, radius2, d_angle, factor1, factor2, factor3, factor4;
  double divisor;

  radius = min (hp->xmid, hp->ymid);

  d_angle = 0;
  factor1 = 2;
  factor2 = 2;
  factor3 = 2;
  factor4 = 2;

  divisor = ((LRAND() / MAXRAND * 3.0 + 1) * (((LRAND() % 1) * 2) - 1));

  if ((LRAND() & 1) == 0)
    {
      radius1 = radius;
      radius2 = radius / divisor;
    }
  else
    {
      radius2 = radius;
      radius1 = radius / divisor;
    }

  while (gcd (ANGLES, d_angle) >= 2)
    d_angle = LRAND() % ANGLES;

#define random_factor()				\
  (((LRAND() % 7) ? ((LRAND() % 1) + 1) : 3)	\
   * (((LRAND() % 1) * 2) - 1))

  while (gcd (gcd (gcd (factor1, factor2), factor3), factor4) != 1)
    {
      factor1 = random_factor();
      factor2 = random_factor();
      factor3 = random_factor();
      factor4 = random_factor();
    }
       if (!mono && Scr[screen].npixels > 2) {
         XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[hp->color]);
         if (++hp->color >= Scr[screen].npixels)
             hp->color = 0;
       } else
         XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));

  helix(win, radius1, radius2, d_angle,
	 factor1, factor2, factor3, factor4);
}

void
drawhelix(win)
    Window win;
{
  helixstruct *hp = &helixes[screen];

  if (++hp->time > cycles)
    inithelix(win);
}
