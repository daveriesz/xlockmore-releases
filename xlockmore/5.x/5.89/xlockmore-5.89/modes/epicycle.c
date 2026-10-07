/* -*- Mode: C; tab-width: 4 -*- */

#if 0
static const char sccsid[] = "@(#)epicycle.c     5.88 2026/01/17 xlockmore";

#endif

/* epicycle --- The motion of a body with epicycles, as in the pre-Copernican
 * cosmologies.
 *
 * Copyright (c) 1998  James Youngman <jay@gnu.org>
 * 
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/* Standard C headers; screenhack.h assumes that these have already
 * been included if required -- for example, it defines M_PI if not
 * already defined.
 */
#include <float.h>
#include <math.h>

#ifdef STANDALONE
#define MODE_epicycle
#define DEFAULTS "*delay: 10000 \n" \
        "*count: -10 \n" \
        "*cycles: 20000 \n" \
        "*ncolors: 64 \n" \
        "*fpsSolid: True \n" \
        "*ignoreRotation: True \n" \

# define free_epicycle 0
#define SMOOTH_COLORS
#include "xlockmore.h"          /* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"              /* in xlockmore distribution */
#endif /* STANDALONE */

#ifdef MODE_epicycle

/*
#include "screenhack.h"
#include "erase.h"
*/



/*********************************************************/
/******************** MAGIC CONSTANTS ********************/
/*********************************************************/
#define MIN_RADIUS (5)		/* smallest allowable circle radius */
#define FILL_PROPORTION (0.9)	/* proportion of screen to fill by scaling. */
/*********************************************************/
/***************** END OF MAGIC CONSTANTS ****************/
/*********************************************************/


#define FULLCIRCLE (2.0 * M_PI)	/* radians in a circle. */

#define FLOATRAND ((double) LRAND() / ((double) MAXRAND))

#define MIN_CIRCLES 2

/* Some of these resource values here are hand-tuned to give a
 * pleasing variety of interesting shapes.  These are not the only
 * good settings, but you may find you need to change some as a group
 * to get pleasing figures.
 */
#define DEF_HOLD_TIME "2" /* hold after complete */
#define DEF_LINE_WIDTH "4"
#define DEF_HARMONICS "8"
#define DEF_DIVISOR_POISSON "0.4"
#define DEF_SIZE_FACTOR_MIN "1.05"
#define DEF_SIZE_FACTOR_MAX "2.05"

static int holdTime;
static int harmonics;
static float divisorPoisson;
static float sizeFactorMin;
static float sizeFactorMax;

static XrmOptionDescRec opts[] =
{
  {(char *) "-holdTime", (char *) ".epicycle.holdTime", XrmoptionSepArg, (caddr_t) NULL},
  {(char *) "-harmonics", (char *) ".epicycles.harmonics", XrmoptionSepArg, (caddr_t) NULL},
  {(char *) "-divisorPoisson", (char *) ".epicycles.divisorPoisson", XrmoptionSepArg, (caddr_t) NULL},  
  {(char *) "-sizeFactorMin", (char *) ".epicycles.sizeFactorMin", XrmoptionSepArg, (caddr_t) NULL},  
  {(char *) "-sizeFactorMax", (char *) ".epicycles.sizeFactorMax", XrmoptionSepArg, (caddr_t) NULL}  
}
;
static argtype vars[] =
{
  {(void *) & holdTime, (char *) "holdTime", (char *) "HoldTime", (char *) DEF_HOLD_TIME, t_Int},
  {(void *) & harmonics, (char *) "harmonics", (char *) "Harmonics", (char *) DEF_HARMONICS, t_Int},
  {(void *) & divisorPoisson, (char *) "divisorPoisson", (char *) "DivisorPoisson", (char *) DEF_DIVISOR_POISSON, t_Float},
  {(void *) & sizeFactorMin, (char *) "sizeFactorMin", (char *) "SizeFactorMin", (char *) DEF_SIZE_FACTOR_MIN, t_Float},
  {(void *) & sizeFactorMax, (char *) "sizeFactorMax", (char *) "SizeFactorMax", (char *) DEF_SIZE_FACTOR_MAX, t_Float}
};
static OptionStruct desc[] =
{
    {(char *) "-hold num", (char *) "hold time"},
    {(char *) "-line num", (char *) "line width"},
    {(char *) "-circles num", (char *) "max cirles"},
    {(char *) "-harmonics num", (char *) "harmonics"},
    {(char *) "-divisor num", (char *) "divisor poisson"},
    {(char *) "-minfactor num", (char *) "min factor"},
    {(char *) "-maxfactor num", (char *) "max factor"}
};

ENTRYPOINT ModeSpecOpt epicycle_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
const ModStruct epicycle_description =
{"epicycle", "init_epicycle", "draw_epicycle", "release_epicycle",
 "init_epicycle", "init_epicycle", (char *) NULL, &epicycle_opts,
 20000, -10, 50000, 1, 64, 1.0, "",
 "Shows points moving around a circles recursively", 0, NULL};

#endif

/* Each circle is centered on a point on the rim of another circle.
 */
struct tagCircle
{
  long radius;			/* in pixels */
  double w;			/* position (radians ccw from x-axis) */
  double initial_w;		/* starting position */
  double wdot;			/* rotation rate (change in w per iteration) */
  int divisor;

  struct tagCircle *pchild;
};
typedef struct tagCircle Circle;


struct tagBody			/* a body that moves on a system of circles. */
{
  int x_origin, y_origin;
  int x, y;
  int old_x, old_y;
  int current_color;		/* pixel index into colors[] */
  Circle *epicycles;		/* system of circles on which it moves. */
  struct tagBody *next;		/* next in list. */
};
typedef struct tagBody Body;

typedef struct {
   Window window;
   int ncolors;
   int width, height;
   int x_offset, y_offset;
   int line_width;
   int unit_pixels;
   int restart;
   double wdot_max;
   int color_shift_pos;	/* how far we are towards that. */
   double colour_cycle_rate;
   int harmonics;
   double divisorPoisson;
   double sizeFactorMin;
   double sizeFactorMax;
   int maxCircles;

   Bool done;

   double T, timestep, circle, timestep_coarse;
   int delay;
   int uncleared;
   int holdtime;
   int xmax, xmin, ymax, ymin;
   Body *pb0;
   double xtime;
#ifdef STANDALONE
   eraser_state *eraser;
#endif
} epicyclestruct;

static epicyclestruct *epicycles = (epicyclestruct *) NULL;


/* Determine the GCD of two numbers using Euclid's method.  The other
 * possible algorithm is Stein's method, but it's probably only going
 * to be much faster on machines with no divide instruction, like the
 * ARM and the Z80.  The former is very fast anyway and the latter
 * probably won't run X clients; in any case, this calculation is not
 * the bulk of the computational expense of the program.  I originally
 * tried using Stein's method, but I wanted to remove the gotos.  Not
 * wanting to introduce possible bugs, I plumped for Euclid's method
 * instead.  Lastly, Euclid's algorithm is preferred to the
 * generalisation for N inputs.
 *
 * See Knuth, section 4.5.2.
 */
static int
gcd(int u, int v)		/* Euclid's Method */
{
  /* If either operand of % is negative, the sign of the result is
   * implementation-defined.  See section 6.3.5 "Multiplicative
   * Operators" of the ANSI C Standard (page 46 [LEFT HAND PAGE!] of
   * "Annotated C Standard", Osborne, ISBN 0-07-881952-0).
   */
  if (u < 0)
    u = -u;
  if (v < 0)
    v = -v;
  
  while (0 != v) {
    int r;
    r = u % v;
    u = v;
    v = r;
  }
  return u;
}

/* Determine the Lowest Common Multiple of two integers, using
 * Euclid's Proposition 34, as explained in Knuth's The Art of
 * Computer Programming, Vol 2, section 4.5.2.
 */
static int
lcm(int u, int v)
{
  return u / gcd(u,v) * v;
}

static long 
random_radius(epicyclestruct *st, double scale)	
{
  long r;

  r = FLOATRAND * scale * st->unit_pixels / 2;
  if (r < MIN_RADIUS)
    r = MIN_RADIUS;
  return r;
}


static long
random_divisor(epicyclestruct *st)
{
  int divisor = 1;
  int sign;

  while (FLOATRAND < st->divisorPoisson && divisor <= st->harmonics) {
    ++divisor;
  }
  sign = (FLOATRAND < 0.5) ? +1 : -1;
  return sign * divisor;
}


/* Construct a circle or die.
 */
static Circle *
new_circle(epicyclestruct *st, double scale)
{
  Circle *p = malloc(sizeof(Circle));
  
  p->radius = random_radius(st, scale);
  p->w = p->initial_w = 0.0;
  p->divisor = random_divisor(st);
  p->wdot = st->wdot_max / p->divisor;
  p->pchild = NULL;
  
  return p;
}

static void delete_circle(Circle *p)
{
  free(p);
}

static void 
delete_circle_chain(Circle *p)
{
  while (p) {
    Circle *q = p->pchild;
    delete_circle(p);
    p = q;
  }
}

static Circle *
new_circle_chain(epicyclestruct *st)
{
  Circle *head;
  double scale = 1.0, factor;
  int n;

  /* Parent circles are larger than their children by a factor of at
   * least FACTOR_MIN and at most FACTOR_MAX.
   */
  factor = st->sizeFactorMin + FLOATRAND * (st->sizeFactorMax - st->sizeFactorMin);
  
  /* There are between minCircles and maxCircles in each figure.
   */
  if (st->maxCircles < -MIN_CIRCLES)
    n = NRAND(-st->maxCircles - MIN_CIRCLES) + MIN_CIRCLES;
  else if (st->maxCircles < MIN_CIRCLES)
    n = MIN_CIRCLES;
  else
    n = st->maxCircles;

  head = NULL;
  while (n--) {
    Circle *p = new_circle(st, scale);
    p->pchild = head;
    head = p;

    scale /= factor;
  }
  return head;
}

static void
assign_random_common_w(Circle *p)
{
  double w_common = FLOATRAND * FULLCIRCLE;	/* anywhere on the circle */
  while (p) {
    p->initial_w = w_common;
    p = p->pchild;
  }
}

static Body *
new_body(epicyclestruct *st)
{
  Body *p = malloc(sizeof(Body));
  if (!p) abort();
  p->epicycles = new_circle_chain(st);
  p->current_color = 0;		/* ?? start them all on different colors? */
  p->next = NULL;
  p->x = p->y = 0;
  p->old_x = p->old_y = 0;
  p->x_origin = p->y_origin = 0;

  /* Start all the epicycles at the same w value to make it easier to
   * figure out at what T value the cycle is closed.   We don't just fix
   * the initial W value because that makes all the patterns tend to 
   * be symmetrical about the X axis.
   */
  assign_random_common_w(p->epicycles);
  return p;
}

static long
compute_divisor_lcm(Circle *p)
{
  long l = 1;
  
  while (p) {
    l = lcm(l, p->divisor);
    p = p->pchild;
  }
  return l;
}

	      
/* move_body()
 *
 * Calculate the position for the body at time T.  We work in double 
 * rather than int to avoid the cumulative errors that would be caused
 * by the rounding implicit in an assignment to int.
 */
static void
move_body(Body *pb, double t)
{
  Circle *p;
  double x, y;

  pb->old_x = pb->x;
  pb->old_y = pb->y;
  
  x = pb->x_origin;
  y = pb->y_origin;
  
  for (p = pb->epicycles; NULL != p; p = p->pchild) {
    /* angular pos = initial_pos + time * angular speed */
    /* but this is an angular position, so modulo FULLCIRCLE. */
    p->w = fmod(p->initial_w + (t * p->wdot), FULLCIRCLE);
    
    x += (p->radius * cos(p->w));
    y += (p->radius * sin(p->w));
  }
  
  pb->x = (int)x;
  pb->y = (int)y;
}

static void
setup(ModeInfo *mi, epicyclestruct *st)
{
  Display *display = MI_DISPLAY(mi);
  Window window = MI_WINDOW(mi);
  GC gc = MI_GC(mi);
  XWindowAttributes xgwa;
 
  XGetWindowAttributes (display, window, &xgwa);

  st->width = xgwa.width;
  st->height = xgwa.height;
  st->x_offset = st->width / 2;
  st->y_offset = st->height / 2;
  st->unit_pixels = st->width < st->height ? st->width : st->height;
  st->line_width = st->unit_pixels / 256 + 1;
  XSetLineAttributes(display, gc, st->line_width,
    LineSolid, CapRound, JoinRound);

  {
    if (!st->done) {
      st->done = True;
    }
  }
}


static void
color_step(ModeInfo *mi, epicyclestruct *st, Body *pb, double frac)
{
  if (st->ncolors > 2) {
    int newshift = st->ncolors * fmod(frac * st->colour_cycle_rate, 1.0);
    if (newshift != st->color_shift_pos) {
      pb->current_color = newshift;
      XSetForeground (MI_DISPLAY(mi), MI_GC(mi), MI_PIXEL(mi, pb->current_color));
      st->color_shift_pos = newshift;
    }
  } else
   XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WHITE_PIXEL(mi));
}


#if 0
static long
distance(long x1, long y1, long x2, long y2)
{
  long dx, dy;

  dx = x2 - x1;
  dy = y2 - y1;
  return dx*dx + dy*dy;
}

static int poisson_irand(double p)
{
  int r = 1;
  while (fabs(FLOATRAND) < p)
    ++r;
  return r < 1 ? 1 : r;
}
#endif

static void
precalculate_figure(Body *pb,
  double this_xtime, double step,
  int *x_max, int *y_max,
  int *x_min, int *y_min)
{
  double t;

  move_body(pb, 0.0); /* move once to avoid initial line from origin */
  *x_min = *x_max = pb->x;
  *y_min = *y_max = pb->y;
  
  for (t = 0.0; t < this_xtime; t += step) {
    move_body(pb, t); /* move once to avoid initial line from origin */
    if (pb->x > *x_max)
      *x_max = pb->x;
    if (pb->x < *x_min)
      *x_min = pb->x;
    if (pb->y > *y_max)
      *y_max = pb->y;
    if (pb->y < *y_min)
      *y_min = pb->y;
  }
}

static int i_max(int a, int b)
{
  return (a>b) ? a : b;
}

static void rescale_circles(epicyclestruct *st, Body *pb,
			    int x_max, int y_max,
			    int x_min, int y_min)
{
  double xscale, yscale, scale;
  double xm, ym;
  
  x_max -= st->x_offset;
  x_min -= st->x_offset;
  y_max -= st->y_offset;
  y_min -= st->y_offset;

  x_max = i_max(x_max, -x_min);
  y_max = i_max(y_max, -y_min);


  xm = st->width / 2.0;
  ym = st->height / 2.0;
  if (x_max > xm)
    xscale = xm / x_max;
  else
    xscale = 1.0;
  if (y_max > ym)
    yscale = ym / y_max;
  else
    yscale = 1.0;

  if (xscale < yscale)		/* wider than tall */
    scale = xscale;		/* ensure width fits onscreen */
  else
    scale = yscale;		/* ensure height fits onscreen */


  scale *= FILL_PROPORTION;	/* only fill FILL_PROPORTION of screen */
  if (scale < 1.0) {		/* only reduce, don't enlarge. */
    Circle *p;
    for (p = pb->epicycles; p; p = p->pchild) {
      p->radius *= scale;
    }
  } else {
    printf("enlarge by x%.2f skipped...\n", scale);
  }

  if (st->width > st->height * 5 ||  /* window has weird aspect */
      st->height > st->width * 5) {
    Circle *p;
    double r = (st->width > st->height
      ? st->width / (double) st->height
      : st->height / (double) st->width);
    for (p = pb->epicycles; p; p = p->pchild)
      p->radius *= r;
  }
}


/* angular speeds of the circles are harmonics of a fundamental
 * value.  That should please the Pythagoreans among you... :-)
 */
static double 
random_wdot_max(epicyclestruct *st)
{
  /* Maximum and minimum values for the choice of wdot_max.  Possible
   * epicycle speeds vary from wdot_max to (wdot_max * harmonics).
   */
  double minspeed, maxspeed;
  minspeed = 0.003;
  maxspeed = 0.005;
  return st->harmonics * (minspeed + FULLCIRCLE * FLOATRAND * (maxspeed - minspeed));
}


ENTRYPOINT void
init_epicycle(ModeInfo * mi)
{
  Display *display = MI_DISPLAY(mi);
  Window window = MI_WINDOW(mi);
  epicyclestruct *st;

  MI_INIT(mi, epicycles);
  st = &epicycles[MI_SCREEN(mi)];
  st->ncolors = MI_NPIXELS(mi);
  st->holdtime = holdTime;

  st->circle = FULLCIRCLE;
  
  XClearWindow(display, window);
  st->uncleared = 0;
  st->restart = 1;
 
   
  st->harmonics = harmonics;
  st->divisorPoisson = divisorPoisson;
  st->timestep = 1.0; 
  st->timestep_coarse = 1.0; 
  st->sizeFactorMin = sizeFactorMin;
  st->sizeFactorMax = sizeFactorMax;

  st->maxCircles = MI_COUNT(mi);

  st->xtime = 0; /* is this right? */
}

ENTRYPOINT void
draw_epicycle(ModeInfo * mi)
{
  Display *display = MI_DISPLAY(mi);
  Window window = MI_WINDOW(mi);
  GC gc = MI_GC(mi);
  epicyclestruct *st;

  if (epicycles == NULL)
    return;
  st = &epicycles[MI_SCREEN(mi)];

#ifdef STANDALONE
  if (st->eraser) {
    st->eraser = erase_window (display, window, st->eraser);
    return;
  }
#endif

  if (st->restart) {
    long L;
    setup(mi, st);
    st->restart = 0;

    /* Flush any outstanding events; this has the side effect of
     * reducing the number of "false restarts"; resdtarts caused by
     * one event (e.g. ConfigureNotify) followed by another
     * (e.g. Expose).
     */
  
    MI_CLEARWINDOW(mi);
    st->wdot_max = random_wdot_max(st);
  
    if (st->pb0) {
      delete_circle_chain(st->pb0->epicycles);
      free(st->pb0);
      st->pb0 = NULL;
    }
    st->pb0 = new_body(st);
    st->pb0->x_origin = st->pb0->x = st->x_offset;
    st->pb0->y_origin = st->pb0->y = st->y_offset;

#ifdef STANDALONE
    if (st->uncleared) {
      st->eraser = erase_window (display, window, st->eraser);
      st->uncleared = 0;
    }
#endif

    precalculate_figure(st->pb0, st->xtime, st->timestep_coarse,
      &st->xmax, &st->ymax, &st->xmin, &st->ymin);

    rescale_circles(st, st->pb0, st->xmax, st->ymax, st->xmin, st->ymin);
      
    move_body(st->pb0, 0.0); /* move once to avoid initial line from origin */
    move_body(st->pb0, 0.0); /* move once to avoid initial line from origin */

      
    st->T = 0.0; /* start at time zero. */

    L = compute_divisor_lcm(st->pb0->epicycles);
      
    st->colour_cycle_rate = labs(L);
      
    st->xtime = fabs(L * st->circle / st->wdot_max);

    if (st->ncolors > 2) /* (colors==NULL) if mono_p */
      XSetForeground (display, gc, MI_PIXEL(mi, st->pb0->current_color));
  }

  color_step(mi, st, st->pb0, st->T / st->xtime );
  /* draw_body */
  XDrawLine(display, window, gc, st->pb0->old_x, st->pb0->old_y, st->pb0->x, st->pb0->y);
  st->uncleared = 1;

	  
  /* Check if the figure is complete...*/
  if (st->T > st->xtime + holdTime * 100) {
    /*this_delay = st->holdtime * 1000000;*/
    st->restart = 1;	/* begin new figure. */
  }

  st->T += st->timestep;
  move_body(st->pb0, st->T);
}

#ifdef STANDALONE
ENTRYPOINT void
reshape_epicycle(ModeInfo *mi, int width, int height)
{
  epicyclestruct *st;

  if (epicycles == NULL)
    return;
  st = &epicycles[MI_SCREEN(mi)];
  st->restart = 1;
}

ENTRYPOINT Bool
epicycle_handle_event(ModeInfo *mi, XEvent *event)
{
  epicyclestruct *st = &epicycles[MI_SCREEN(mi)];
  if (screenhack_event_helper(MI_DISPLAY(mi), MI_WINDOW(mi), event)) {
    /*reset_epicycle(st);
    MI_CLEARWINDOW(mi);*/
    st->restart = 1;
    return True;
  }
  return False;
}
#endif

static void
free_epicycle_screen(epicyclestruct *st)
{
  if (st == NULL)
    return;
#ifdef STANDALONE
  if (st->eraser)
    eraser_free(st->eraser);  
#endif
  if (st->pb0) {
    delete_circle_chain(st->pb0->epicycles);
    free(st->pb0);
   }
}

ENTRYPOINT void
release_epicycle(ModeInfo * mi)
{
  if (epicycles != NULL) {
    int screen;

    for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
      free_epicycle_screen(&epicycles[screen]);
    }
    free(epicycles);
    epicycles = (epicyclestruct *) NULL;
  }
}

XSCREENSAVER_MODULE ("Epicycle", epicycle)

#endif /* MODE_epicycle */
