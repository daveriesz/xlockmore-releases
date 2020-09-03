#ifndef lint
static char sccsid[] = "@(#)rock.c	2.9 95/04/21 xlockmore";
#endif
/*
 * Flying through an asteroid field.  Based on TI Explorer Lisp code by
 *  John Nguyen <johnn@hx.lcs.mit.edu>
 *
 * Copyright (c) 1992 by Jamie Zawinski
 *
 * 14-Apr-95: Jeremie PETIT <petit@aurora.unice.fr> added a "move" feature.
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
#include <stdio.h>
#include "xlock.h"

#define MIN_DEPTH 2		/* rocks disappear when they get this close */
#define MAX_DEPTH 60		/* this is where rocks appear */
#define MAX_WIDTH 100		/* how big (in pixels) rocks are at depth 1 */
#define DEPTH_SCALE 100		/* how many ticks there are between depths */
#define RESOLUTION 1000
#define MAX_DEP 0.3             /* how far the displacement can be (percents)*/
#define DIRECTION_CHANGE_RATE 50 /* 0 is always */
#define MAX_DEP_SPEED 5         /* Maximum speed for movement */
#define MOVE_STYLE 0            /* Only 0 and 1. Distinguishes the fact that
				   these are the rocks that are moving (1) 
				   or the rocks source (0). */

/* there's not much point in the above being user-customizable, but those
   numbers might want to be tweaked for displays with an order of magnitude
   higher resolution or compute power.
 */
typedef struct {
  int real_size;
  int r;
  unsigned long color;
  int theta;
  int depth;
  int size, x, y;
} arock;

typedef struct {
    int	width;
    int	height;
    int midx, midy;
    int rotate_p, speed, nrocks;
    int move_p;
    int dep_x, dep_y;
    double cos_array[RESOLUTION], sin_array[RESOLUTION];
    double depths[(MAX_DEPTH + 1) * DEPTH_SCALE];
    arock *arocks;
    Pixmap pixmaps[MAX_WIDTH];
} rockstruct;

static rockstruct rocks[MAXSCREENS];

static void rock_reset(), rock_tick(), rock_compute(), rock_draw();
static void init_pixmaps(), tick_rocks();
static int compute_move();

static void
rock_reset(win, arocks)
     Window win;
     arock *arocks;
{
  arocks->real_size = MAX_WIDTH;
  arocks->r = (RESOLUTION * 0.7) + (LRAND() % (30 * RESOLUTION));
  arocks->theta = LRAND() % RESOLUTION;
  arocks->depth = MAX_DEPTH * DEPTH_SCALE;
  if (!mono && Scr[screen].npixels > 2)
    arocks->color = Scr[screen].pixels[LRAND() % Scr[screen].npixels];
  else
    arocks->color = WhitePixel(dsp, screen);
  rock_compute(arocks);
  rock_draw(win, arocks, True);
}

static void
rock_tick (win, arocks, d)
     Window win;
     arock *arocks;
     int d;
{
  rockstruct *rp = &rocks[screen];
  if (arocks->depth > 0)
    {
      rock_draw(win, arocks, False);
      arocks->depth -= rp->speed;
      if (rp->rotate_p)
	  arocks->theta = (arocks->theta + d) % RESOLUTION;
      while (arocks->theta < 0)
	arocks->theta += RESOLUTION;
      if (arocks->depth < (MIN_DEPTH * DEPTH_SCALE))
	arocks->depth = 0;
      else
	{
	  rock_compute(arocks);
	  rock_draw(win, arocks, True);
	}
    }
  else if ((LRAND() % 40) == 0)
    rock_reset(win, arocks);
}

static void
rock_compute(arocks)
     arock *arocks;
{
  rockstruct *rp = &rocks[screen];
  double factor = rp->depths [arocks->depth];
  arocks->size = (int) ((arocks->real_size * factor) + 0.5);
  arocks->x = rp->midx + (rp->cos_array[arocks->theta] * arocks->r * factor);
  arocks->y = rp->midy + (rp->sin_array[arocks->theta] * arocks->r * factor);
  if (rp->move_p) {
     double move_factor = (double)(MOVE_STYLE - (double)arocks->depth /
 			  (double)((MAX_DEPTH + 1) * (double)DEPTH_SCALE));
     /* move_factor is 0 when the rock is close, 1 when far */
     arocks->x += (double)rp->dep_x * move_factor;
     arocks->y += (double)rp->dep_y * move_factor;
  }
}

static void
rock_draw(win, arocks, draw_p)
     Window win;
     arock *arocks;
     int draw_p;
{
  rockstruct *rp = &rocks[screen];
  if (draw_p)
      XSetForeground(dsp, Scr[screen].gc, arocks->color);
  else
      XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
  if (arocks->x <= 0 || arocks->y <= 0 ||
      arocks->x >= rp->width || arocks->y >= rp->height)
    {
      /* this means that if a rock were to go off the screen at 12:00, but
	 would have been visible at 3:00, it won't come back once the observer
	 rotates around so that the rock would have been visible again.
	 Oh well.
       */
      if (!rp->move_p) arocks->depth = 0;
      return;
    }
  if (arocks->size <= 1)
    XDrawPoint (dsp, win, Scr[screen].gc, arocks->x, arocks->y);
  else if (arocks->size <= 3 || !draw_p)
    XFillRectangle(dsp, win, Scr[screen].gc,
		    arocks->x - arocks->size/2, arocks->y - arocks->size/2,
		    arocks->size, arocks->size);
  else if (arocks->size < MAX_WIDTH)
    XCopyPlane(dsp, rp->pixmaps[arocks->size], win, Scr[screen].gc,
		0, 0, arocks->size, arocks->size,
		arocks->x - arocks->size/2, arocks->y - arocks->size/2,
		1);
}

static void
init_pixmaps(win)
     Window win;
{
  rockstruct *rp = &rocks[screen];
  int i;
  XGCValues gcv;
  GC fg_gc = 0, bg_gc = 0;

  rp->pixmaps[0] = rp->pixmaps[1] = 0;
  for (i = MIN_DEPTH; i < MAX_WIDTH; i++)
    {
      int w = (1+(i/32))<<5; /* server might be faster if word-aligned */
      int h = i;

      Pixmap p = XCreatePixmap(dsp, win, w, h, 1);
      XPoint points[7];
      rp->pixmaps[i] = p;
      if (!p)
	{
	  (void) fprintf (stderr, "%s: couldn't allocate pixmaps", "xlock");
	  exit (1);
	}
      if (!fg_gc)
	{	/* must use drawable of pixmap, not window (fmh) */
	  gcv.foreground = 1;
	  fg_gc = XCreateGC (dsp, p, GCForeground, &gcv);
	  gcv.foreground = 0;
	  bg_gc = XCreateGC (dsp, p, GCForeground, &gcv);
	}
      XFillRectangle(dsp, p, bg_gc, 0, 0, w, h);
      points[0].x = i * 0.15; points[0].y = i * 0.85;
      points[1].x = i * 0.00; points[1].y = i * 0.20;
      points[2].x = i * 0.30; points[2].y = i * 0.00;
      points[3].x = i * 0.40; points[3].y = i * 0.10;
      points[4].x = i * 0.90; points[4].y = i * 0.10;
      points[5].x = i * 1.00; points[5].y = i * 0.55;
      points[6].x = i * 0.45; points[6].y = i * 1.00;
      XFillPolygon(dsp, p, fg_gc, points, 7, Nonconvex, CoordModeOrigin);
    }
  XFreeGC(dsp, fg_gc);
  XFreeGC(dsp, bg_gc);
}

void
initrock(win)
     Window win;
{
  rockstruct *rp = &rocks[screen];
  int i;
  XWindowAttributes xgwa;
  (void) XGetWindowAttributes(dsp, win, &xgwa);

      rp->width = xgwa.width;
      rp->height = xgwa.height;
      rp->midx = rp->width/2;
      rp->midy = rp->height/2;
  rp->speed = 100;
  rp->rotate_p = True;
  rp->move_p = True;
  rp->dep_x = 0;
  rp->dep_y = 0;
  if (batchcount < 1)
    batchcount = 1;
  rp->nrocks = batchcount;
  if (rp->speed < 1) rp->speed = 1;
  if (rp->speed > 100) rp->speed = 100;

  for (i = 0; i < RESOLUTION; i++)
    {
      rp->sin_array[i] = sin((((double) i) / (RESOLUTION / 2)) * M_PI);
      rp->cos_array[i] = cos((((double) i) / (RESOLUTION / 2)) * M_PI);
    }
  /* we actually only need i/speed of these, but wtf */
  for (i = 1; i < (sizeof(rp->depths) / sizeof(rp->depths[0])); i++)
    rp->depths[i] = atan(((double) 0.5) / (((double) i) / DEPTH_SCALE));
  rp->depths[0] = M_PI/2; /* avoid division by 0 */

  if (!rp->arocks)
    rp->arocks = (arock *) calloc(rp->nrocks, sizeof(arock));
  XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
  XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, rp->width, rp->height);
  init_pixmaps(win);
}

static void
tick_rocks(win, d)
     Window win;
     int d;
{
  rockstruct *rp = &rocks[screen];
  int i;
  if (rp->move_p) {
    rp->dep_x = compute_move(0);
    rp->dep_y = compute_move(1);
  }
  for (i = 0; i < rp->nrocks; i++)
    rock_tick(win, &rp->arocks[i], d);
}

static int
compute_move(axe)
     int axe; /* 0 for x, 1 for y */
{
  static int current_dep[2] = {0,0};
  static int speed[2]       = {0,0};
  static short direction[2] = {0,0};
  static int limit[2]       = {0,0};
  rockstruct *rp = &rocks[screen];
  int change = 0;
  
  limit[0] = rp->midx;
  limit[1] = rp->midy;

  current_dep[axe] += speed[axe]; /* We adjust the displacement */

  if (current_dep[axe] > (int)(limit[axe] * MAX_DEP)){
    if (current_dep[axe] > limit[axe]) current_dep[axe] = limit[axe];
    direction[axe] = -1;
  }/* This is when we reach the upper screen limit */
  if (current_dep[axe] < (int)(-limit[axe] * MAX_DEP)){
    if (current_dep[axe] < -limit[axe]) current_dep[axe] = -limit[axe];
    direction[axe] = 1;
  }/* This is when we reach the lower screen limit */

  if (direction[axe] == 1)/* We adjust the speed */
    speed[axe] += 1;
  else if (direction[axe] == -1)
    speed[axe] -= 1;

  if (speed[axe] > MAX_DEP_SPEED) speed[axe] = MAX_DEP_SPEED;
  else if (speed[axe] < -MAX_DEP_SPEED) speed[axe] = -MAX_DEP_SPEED;

  if((LRAND() % DIRECTION_CHANGE_RATE) == 0){
    /* We change direction */
    change = LRAND() % 2;
    if (change != 1)
      if (direction[axe] == 0)
	direction[axe] = change - 1; /* 0 becomes either 1 or -1 */
      else
	direction[axe] = 0; /* -1 or 1 become 0 */
  }
  return(current_dep[axe]);
}

void
drawrock(win)
    Window win;
{
  static int current_delta = 0;	/* observer Z rotation */
  static int window_tick = 50;
  static int new_delta = 0;
  static int dchange_tick = 0;

  if (window_tick++ == 50)
      window_tick = 0;
  if (current_delta != new_delta) {
    if (dchange_tick++ == 5) {
      dchange_tick = 0;
      if (current_delta < new_delta)
        current_delta++;
      else
        current_delta--;
    }
  } else {
    if ((LRAND() % 50) == 0) {
      new_delta = ((LRAND() % 11) - 5);
      if ((LRAND() % 10) == 0)
	new_delta *= 5;
    }
  }
  tick_rocks (win, current_delta);
}
