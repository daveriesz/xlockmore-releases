#ifndef lint
static char sccsid[] = "@(#)forest.c	2.10 95/05/18 xlockmore";
#endif
/*-
 * forest.c - forest for xlockmore
 *
 * Copyright (c) 1995 Pascal Pensa <pensa@aurora.unice.fr>
 *
 * Original idea : Guillaume Ramey <ramey@aurora.unice.fr>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include "xlock.h"
#include <math.h>

#define MINHEIGHT  20  /* Tree height range */
#define MAXHEIGHT  40

#define MINANGLE   15  /* (degree) angle between soon */
#define MAXANGLE   35
#define RANDANGLE  15  /* (degree) Max random angle from default */

#define REDUCE     90  /* Height % from father */

#define ITERLEVEL  10  /* Tree iteration */

#define COLORSPEED  2  /* Color increment */

/* degree to radian */
#define DEGTORAD(x) (((float)(x)) * 0.017453293)

#define PIONTWO 1.5707963

#define MAX_RAND(max) (LRAND() % (max))
#define RANGE_RAND(min,max) ((min) + LRAND() % ((max) - (min)))

typedef struct {
  int		width;
  int		height;
  int		time;           /* up time */
} foreststruct;

static foreststruct forests[MAXSCREENS];

static void
drawtree(win,x,y,len,a,as,c,level)
     Window win;
     short    x,y;   /* Father's end */
     short    len;   /* Length */
     short    c;     /* color */
     short    level; /* Height level */
     float    a;     /* Father's angle */
     float    as;    /* Father's angle step */
{
  short x1,y1,x2,y2;
  float a1,a2;

  /* left */

  a1 = a + as + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);
  
  x1 = x + (short)(cos(a1) * ((float)len));
  y1 = y + (short)(sin(a1) * ((float)len));
  
  /* right */
  
  a2 = a - as + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);
  
  x2 = x + (short)(cos(a2) * ((float)len));
  y2 = y + (short)(sin(a2) * ((float)len));

  XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[c]);

  XDrawLine(dsp, win, Scr[screen].gc, x, y, x1, y1);
  XDrawLine(dsp, win, Scr[screen].gc, x, y, x2, y2);

  if(level < 2){
    XDrawLine(dsp, win, Scr[screen].gc, x+1, y, x1+1, y1);
    XDrawLine(dsp, win, Scr[screen].gc, x+1, y, x2+1, y2);
  }

  len = (len * REDUCE * 10) / 1000;
  c = (c + COLORSPEED) % Scr[screen].npixels;

  if(level < ITERLEVEL){
    drawtree(win, x1, y1, len, a1, as, c, level + 1);
    drawtree(win, x2, y2, len, a2, as, c, level + 1);
  }
}

void
initforest(win)
     Window      win;
{
  XWindowAttributes xgwa;
  foreststruct *fp = &forests[screen];

  (void) XGetWindowAttributes(dsp, win, &xgwa);
  fp->width = xgwa.width;
  fp->height = xgwa.height;
  fp->time = 0;

  XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
  XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, fp->width, fp->height);
}

void
drawforest(win)
     Window      win;
{
  short x,y,x2,y2,len,c;
  float a,as;
  foreststruct *fp = &forests[screen];

  if (fp->time < batchcount) {

    x   = RANGE_RAND(0,fp->width);
    y   = RANGE_RAND(0,fp->height + MAXHEIGHT);
    a   = - PIONTWO + DEGTORAD(MAX_RAND(2 * RANDANGLE) - RANDANGLE);
    as  = DEGTORAD(RANGE_RAND(MINANGLE,MAXANGLE));
    len = ((RANGE_RAND(MINHEIGHT,MAXHEIGHT) * (fp->width / 20)) / 50) + 2;

    c = MAX_RAND(Scr[screen].npixels);

    XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[c]);
    
    x2 = x + (short)(cos(a) * ((float)len));
    y2 = y + (short)(sin(a) * ((float)len));

    c = (c + COLORSPEED) % Scr[screen].npixels;

    XDrawLine(dsp, win, Scr[screen].gc, x, y, x2, y2);
    XDrawLine(dsp, win, Scr[screen].gc, x+1, y, x2+1, y2);

    drawtree(win, x2, y2, (len * REDUCE) / 100, a, as, c, 1);
  }

  if (++fp->time > cycles)
    initforest(win);
}


