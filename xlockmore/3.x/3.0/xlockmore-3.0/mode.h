/*-
 * @(#)mode.h  3.0 95/07/14 xlockmore
 *
 * mode.h - mode management for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *
 * Changes of David Bagley (bagleyd@source.asset.com)
 * 02-Jun-95: Extracted out of resource.c  .
 *
 */

/*
 * Declare external interface routines for supported screen savers.
 */
 
extern void initbat();
extern void drawbat();

extern void initbounce();
extern void drawbounce();

extern void initblot();
extern void drawblot();

extern void initclock();
extern void drawclock();

extern void initflame();
extern void drawflame();

extern void initforest();
extern void drawforest();

extern void initgalaxy();
extern void drawgalaxy();
 
extern void initgeometry();
extern void drawgeometry();

extern void initgrav();
extern void drawgrav();

extern void inithelix();
extern void drawhelix();

extern void inithop();
extern void drawhop();

extern void inithyper();
extern void drawhyper();

extern void initimage();
extern void drawimage();

extern void initkaleid();
extern void drawkaleid();

extern void initlaser();
extern void drawlaser();

extern void initlife();
extern void drawlife();

extern void initlife3d();
extern void drawlife3d();

extern void initmaze();
extern void drawmaze();

extern void initmountain();
extern void drawmountain();

extern void initpyro();
extern void drawpyro();

extern void initqix();
extern void drawqix();

extern void initrect();
extern void drawrect();

extern void initrock();
extern void drawrock();

extern void initrotor();
extern void drawrotor();

extern void initsphere();
extern void drawsphere();

extern void initspiral();
extern void drawspiral();

extern void initspline();
extern void drawspline();

extern void initswarm();
extern void drawswarm();

extern void initwator();
extern void drawwator();

extern void initworld();
extern void drawworld();

extern void initworm();
extern void drawworm();
 
extern void initswirl();
extern void drawswirl();

extern void initblank();
extern void drawblank();

typedef struct {
  char  *cmdline_arg;
  void  (*lp_init) ();
  void  (*lp_callback) ();
  int   def_delay;
  int   def_batchcount;
  int   def_cycles;
  float def_saturation;
  char  *desc;
} LockStruct;

#define NUMSPECIAL 2 /* # of Special Screens */

static LockStruct LockProcs[] = {
  {"bat", initbat, drawbat, 100000, 6, 20, 1.0,
     "Flying Bats"},
  {"blot", initblot, drawblot, 100000, 6, 30, 0.4,
     "Rorschach's ink blot test"},
  {"bounce", initbounce, drawbounce, 10000, 10, 20, 1.0,
     "Bouncing ball"},
  {"clock", initclock, drawclock, 1000, 30, 50, 1.0,
     "Clock"},
  {"flame", initflame, drawflame, 10000, 20, 10000, 1.0,
     "Cosmic Flame Fractals"},
  {"forest", initforest, drawforest, 100000, 100, 200, 1.0,
     "Fractal Forest"},
  {"galaxy", initgalaxy, drawgalaxy, 100, 3, 20, 1.0,
     "Spinning galaxies"},
  {"geometry", initgeometry, drawgeometry, 2000, 8, 20, 1.0,
     "Complete morphing"},
  {"grav", initgrav, drawgrav, 10000, 10, 20, 1.0,
     "Orbiting planets"},
  {"helix", inithelix, drawhelix, 10000, 1, 100, 1.0,
     "Helix"},
  {"hop", inithop, drawhop, 10000, 1000, 2500, 1.0,
     "Hopalong iterated fractals"},
  {"hyper", inithyper, drawhyper, 10000, 1, 300, 1.0,
     "Spinning Tesseract"},
  {"image", initimage, drawimage, 2000000, 1, 20, 1.0,
     "Random Bouncing Image"},
  {"kaleid", initkaleid, drawkaleid, 2000, 4, 700, 1.0,
     "Kaleidoscope"},
  {"laser", initlaser, drawlaser, 5000, 10, 200, 1.0,
     "Laser ray"},
  {"life", initlife, drawlife, 750000, 40, 140, 1.0,
     "Conway's game of Life"},
  {"life3d", initlife3d, drawlife3d, 1000000, 35, 85, 1.0,
     "Bays' game of 3D Life"},
  {"mountain", initmountain, drawmountain, 10000, 30, 100, 1.0,
     "Papo's Mountains"},
  {"maze", initmaze, drawmaze, 10000, 40, 300, 1.0,
     "aMAZEing"},
  {"pyro", initpyro, drawpyro, 15000, 40, 20, 1.0,
     "Fireworks"},
  {"qix", initqix, drawqix, 30000, 100, 64, 1.0,
     "Spinning lines a la Qix(tm)"},
  {"rect", initrect, drawrect, 10000, 100, 256, 1.0,
     "Greynetic rectangles"},
  {"rock", initrock, drawrock, 20000, 100, 20, 0.2,
     "Asteroid field"},
  {"rotor", initrotor, drawrotor, 10000, 4, 20, 0.4,
     "Tom's Roto-Rooter"},
  {"sphere", initsphere, drawsphere, 10000, 1, 20, 1.0,
     "Shaded spheres"},
  {"spiral", initspiral, drawspiral, 5000, 6, 350, 1.0,
     "Helix-ish spiral"},
  {"spline", initspline, drawspline, 30000, 100, 20, 0.4,
     "Moving Splines"},
  {"swarm", initswarm, drawswarm, 10000, 100, 20, 1.0,
     "Swarm of bees"},
   {"wator", initwator, drawwator, 750000, 4, 32767, 1.0,
    "Dewdney's Planet Wa-Tor"},
  {"world", initworld, drawworld, 100000, 8, 20, 0.3,
     "Random Spinning Earths"},
  {"worm", initworm, drawworm, 10000, 20, 20, 1.0,
     "Wiggly Worms"},
  {"swirl", initswirl, drawswirl, 5000, 5, 20, 1.0,
     "Animated swirling patterns"},
  {"blank", initblank, drawblank, 4000000, 1, 20, 1.0,
     "Blank screen"},
  {"random", NULL, NULL, 0, 0, 0, 0.0,
     "Random mode"},
};
