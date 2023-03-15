/*-
 * @(#)modes.h	4.01 2000/01/28 xlockmore
 *
 * modes.h - mode management for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 10-Oct-1999: Generated by genlauncher.
 *              Adapted by Eric Lassauge <lassauge AT users.sourceforge.net>
 * 08-Jul-1998: Adapted to xglock from mode.h by Remi Cohen-Scali
 *              <remi.cohenscali@pobox.com>
 *
 */

/*-
 * Declare external interface routines for supported screen savers.
 */

#ifndef _LMODE_H_
#define _LMODE_H_

/* -------------------------------------------------------------------- */

typedef struct LockStruct_s {
	gchar       *cmdline_arg;	/* mode name */
	gint         def_delay;		/* default delay for mode */
	gint         def_batchcount;
	gint         def_cycles;
	gint         def_size;
	gfloat       def_saturation;
	gchar       *desc;		/* text description of mode */
	void        *userdata;		/* for use by the mode */
} LockStruct;

LockStruct  LockProcs[] =
{
	{"anemone",
	 50000, 1, 1, 1, 1.00,
	 "Shows wiggling tentacles", (void *) NULL},
	{"ant",
	 1000, -3, 40000, -7, 1.00,
	 "Shows Langton's and Turk's generalized ants", (void *) NULL},
	{"ant3d",
	 5000, -3, 100000, 1, 1.00,
	 "Shows 3D ants", (void *) NULL},
	{"apollonian",
	 1000000, 64, 20, 1, 1.00,
	 "Shows Apollonian Circles", (void *) NULL},
#ifdef USE_GL
	{"atlantis",
	 25000, 4, 100, 6000, 1.00,
	 "Shows moving sharks/whales/dolphin", (void *) NULL},
#endif
#ifdef USE_GL
	{"atunnels",
	 25000, 1, 1, 0, 1.00,
	 "Shows an OpenGL advanced tunnel screensaver", (void *) NULL},
#endif
	{"ball",
	 10000, 10, 20, -100, 1.00,
	 "Shows bouncing balls", (void *) NULL},
	{"bat",
	 100000, -8, 1, 0, 1.00,
	 "Shows bouncing flying bats", (void *) NULL},
	{"blot",
	 200000, 6, 30, 1, 0.30,
	 "Shows Rorschach's ink blot test", (void *) NULL},
	{"bouboule",
	 10000, 100, 1, 15, 1.00,
	 "Shows Mimi's bouboule of moving stars", (void *) NULL},
	{"bounce",
	 5000, -10, 1, 0, 1.00,
	 "Shows bouncing footballs", (void *) NULL},
#ifdef USE_GL
	{"boxed",
	 1000, 1, 2, 1, 1.00,
	 "Shows GL's boxed balls", (void *) NULL},
#endif
	{"braid",
	 1000, 15, 100, -7, 1.00,
	 "Shows random braids and knots", (void *) NULL},
	{"bubble",
	 100000, 25, 1, 100, 0.30,
	 "Shows popping bubbles", (void *) NULL},
#if defined( USE_GL ) && defined( HAVE_CXX )
	{"bubble3d",
	 20000, 1, 2, 1, 1.00,
	 "Richard Jones's GL bubbles", (void *) NULL},
#endif
	{"bug",
	 75000, 10, 32767, -4, 1.00,
	 "Shows Palmiter's bug evolution and garden of Eden", (void *) NULL},
#ifdef USE_GL
	{"cage",
	 80000, 1, 1, 1, 1.00,
	 "Shows the Impossible Cage, an Escher-like GL scene", (void *) NULL},
#endif
	{"clock",
	 100000, -16, 200, -200, 1.00,
	 "Shows Packard's clock", (void *) NULL},
	{"coral",
	 60000, -3, 1, 35, 0.60,
	 "Shows a coral reef", (void *) NULL},
	{"crystal",
	 60000, -500, 200, -15, 1.00,
	 "Shows polygons in 2D plane groups", (void *) NULL},
	{"daisy",
	 100000, 300, 350, 1, 1.00,
	 "Shows a meadow of daisies", (void *) NULL},
	{"dclock",
	 10000, 1, 10000, 1, 0.30,
	 "Shows a floating digital clock or message", (void *) NULL},
	{"decay",
	 200000, 6, 30, 1, 0.30,
	 "Shows a decaying screen", (void *) NULL},
	{"deco",
	 1000000, -30, 2, -10, 0.60,
	 "Shows art as ugly as sin", (void *) NULL},
	{"demon",
	 50000, 0, 1000, -7, 1.00,
	 "Shows Griffeath's cellular automata", (void *) NULL},
	{"dilemma",
	 200000, -2, 1000, 0, 1.00,
	 "Shows Lloyd's Prisoner's Dilemma simulation", (void *) NULL},
	{"discrete",
	 1000, 4096, 2500, 1, 1.00,
	 "Shows various discrete maps", (void *) NULL},
	{"dragon",
	 2000000, 1, 16, -24, 1.00,
	 "Shows Deventer's Hexagonal Dragons Maze", (void *) NULL},
	{"drift",
	 10000, 30, 1, 1, 1.00,
	 "Shows cosmic drifting flame fractals", (void *) NULL},
	{"euler2d",
	 1000, 1024, 3000, 1, 1.00,
	 "Shows a simulation of 2D incompressible inviscid fluid", (void *) NULL},
	{"eyes",
	 20000, -8, 5, 1, 1.00,
	 "Shows eyes following a bouncing grelb", (void *) NULL},
	{"fadeplot",
	 30000, 10, 1500, 1, 0.60,
	 "Shows a fading plot of sine squared", (void *) NULL},
	{"fiberlamp",
	 10000, 500, 10000, 0, 1.00,
	 "Shows a Fiber Optic Lamp", (void *) NULL},
#ifdef USE_GL
	{"fire",
	 10000, 800, 1, 0, 1.00,
	 "Shows a 3D fire-like image", (void *) NULL},
#endif
	{"flag",
	 50000, 1, 1000, -7, 1.00,
	 "Shows a waving flag image", (void *) NULL},
	{"flame",
	 750000, 20, 10000, 1, 1.00,
	 "Shows cosmic flame fractals", (void *) NULL},
	{"flow",
	 1000, 1024, 3000, -10, 1.00,
	 "Shows dynamic strange attractors", (void *) NULL},
	{"forest",
	 400000, 100, 200, 1, 1.00,
	 "Shows binary trees of a fractal forest", (void *) NULL},
	{"fzort",
	 10000, 1, 1, 1, 1.00,
	 "Shows a metalic-looking fzort", (void *) NULL},
	{"galaxy",
	 100, -5, 250, -3, 1.00,
	 "Shows crashing spiral galaxies", (void *) NULL},
#ifdef USE_GL
	{"gears",
	 50000, 1, 2, 1, 1.00,
	 "Shows GL's gears", (void *) NULL},
#endif
#ifdef USE_GL
	{"glplanet",
	 15000, 1, 2, 1, 1.00,
	 "Animates texture mapped sphere (planet)", (void *) NULL},
#endif
	{"goop",
	 10000, -12, 1, 1, 1.00,
	 "Shows goop from a lava lamp", (void *) NULL},
	{"grav",
	 10000, -12, 1, 1, 1.00,
	 "Shows orbiting planets", (void *) NULL},
	{"helix",
	 25000, 1, 100, 1, 1.00,
	 "Shows string art", (void *) NULL},
	{"hop",
	 10000, 1000, 2500, 1, 1.00,
	 "Shows real plane iterated fractals", (void *) NULL},
	{"hyper",
	 100000, -6, 300, 1, 1.00,
	 "Shows spinning n-dimensional hypercubes", (void *) NULL},
	{"ico",
	 100000, 0, 400, 0, 1.00,
	 "Shows a bouncing polyhedron", (void *) NULL},
	{"ifs",
	 1000, 1, 1, 1, 1.00,
	 "Shows a modified iterated function system", (void *) NULL},
	{"image",
	 3000000, -20, 1, 1, 1.00,
	 "Shows randomly appearing logos", (void *) NULL},
#if defined( USE_GL ) && defined( HAVE_CXX )
	{"invert",
	 80000, 1, 1, 1, 1.00,
	 "Shows a sphere inverted without wrinkles", (void *) NULL},
#endif
	{"juggle",
	 10000, 200, 1000, 1, 1.00,
	 "Shows a Juggler, juggling", (void *) NULL},
#ifdef USE_GL
	{"juggler3d",
	 10000, 200, 1000, 1, 1.00,
	 "Shows a 3D Juggler, juggling", (void *) NULL},
#endif
	{"julia",
	 10000, 1000, 20, 1, 1.00,
	 "Shows the Julia set", (void *) NULL},
	{"kaleid",
	 80000, 4, 40, -9, 0.60,
	 "Shows a kaleidoscope", (void *) NULL},
	{"kumppa",
	 10000, 1, 1, 1, 1.00,
	 "Shows kumppa", (void *) NULL},
#ifdef USE_GL
	{"lament",
	 10000, 1, 1, 1, 1.00,
	 "Shows Lemarchand's Box", (void *) NULL},
#endif
	{"laser",
	 20000, -10, 200, 1, 1.00,
	 "Shows spinning lasers", (void *) NULL},
	{"life",
	 750000, 40, 140, 0, 1.00,
	 "Shows Conway's game of Life", (void *) NULL},
	{"life1d",
	 10000, 1, 10, 0, 1.00,
	 "Shows Wolfram's game of 1D Life", (void *) NULL},
	{"life3d",
	 1000000, 35, 85, 1, 1.00,
	 "Shows Bays' game of 3D Life", (void *) NULL},
	{"lightning",
	 10000, 1, 1, 1, 0.60,
	 "Shows Keith's fractal lightning bolts", (void *) NULL},
	{"lisa",
	 25000, 1, 256, -1, 1.00,
	 "Shows animated lisajous loops", (void *) NULL},
	{"lissie",
	 10000, 1, 2000, -200, 0.60,
	 "Shows lissajous worms", (void *) NULL},
	{"loop",
	 100000, -5, 1600, -12, 1.00,
	 "Shows Langton's self-producing loops", (void *) NULL},
	{"lyapunov",
	 25000, 600, 1, 1, 1.00,
	 "Shows lyapunov space", (void *) NULL},
	{"mandelbrot",
	 25000, -8, 20000, 1, 1.00,
	 "Shows mandelbrot sets", (void *) NULL},
	{"marquee",
	 100000, 1, 1, 1, 1.00,
	 "Shows messages", (void *) NULL},
	{"matrix",
	 100, 1, 1, 1, 1.00,
	 "Shows the matrix", (void *) NULL},
	{"maze",
	 1000, 1, 3000, -40, 1.00,
	 "Shows a random maze and a depth first search solution", (void *) NULL},
#ifdef USE_GL
	{"maze3d",
	 20000, 1, 2, 1, 1.00,
	 "Shows a 3D maze", (void *) NULL},
#endif
#ifdef USE_GL
	{"moebius",
	 30000, 1, 1, 1, 1.00,
	 "Shows Moebius Strip II, an Escher-like GL scene with ants", (void *) NULL},
#endif
#ifdef USE_GL
	{"molecule",
	 50000, 1, 20, 1, 1.00,
	 "Draws molecules", (void *) NULL},
#endif
#ifdef USE_GL
	{"morph3d",
	 40000, 0, 1, 1, 1.00,
	 "Shows GL morphing polyhedra", (void *) NULL},
#endif
	{"mountain",
	 1000, 30, 4000, 1, 1.00,
	 "Shows Papo's mountain range", (void *) NULL},
	{"munch",
	 5000, 1, 7, 1, 1.00,
	 "Shows munching squares", (void *) NULL},
#ifdef USE_GL
	{"noof",
	 1000, 1, 1, 1, 1.00,
	 "Shows SGI Diatoms", (void *) NULL},
#endif
	{"nose",
	 100000, 1, 1, 1, 1.00,
	 "Shows a man with a big nose runs around spewing out messages", (void *) NULL},
	{"pacman",
	 10000, 10, 1, 0, 1.00,
	 "Shows Pacman(tm)", (void *) NULL},
	{"penrose",
	 10000, 1, 1, -40, 1.00,
	 "Shows Penrose's quasiperiodic tilings", (void *) NULL},
	{"petal",
	 10000, -500, 400, 1, 1.00,
	 "Shows various GCD Flowers", (void *) NULL},
	{"petri",
	 10000, 1, 1, 4, 1.00,
	 "Shows a mold simulation in a petri dish", (void *) NULL},
#ifdef USE_GL
	{"pipes",
	 1000, 2, 5, 500, 1.00,
	 "Shows a selfbuilding pipe system", (void *) NULL},
#endif
	{"polyominoes",
	 6000, 1, 8192, 1, 1.00,
	 "Shows attempts to place polyominoes into a rectangle", (void *) NULL},
	{"puzzle",
	 10000, 250, 1, 1, 1.00,
	 "Shows a puzzle being scrambled and then solved", (void *) NULL},
	{"pyro",
	 15000, 100, 1, -3, 1.00,
	 "Shows fireworks", (void *) NULL},
	{"pyro2",
	 15000, 1, 1, 1, 1.00,
	 "Shows other fireworks", (void *) NULL},
	{"qix",
	 30000, -5, 32, 1, 1.00,
	 "Shows spinning lines a la Qix(tm)", (void *) NULL},
	{"rain",
	 35000, 1, 1, 1, 0.30,
	 "Shows rain", (void *) NULL},
	{"roll",
	 100000, 25, 1, -64, 0.60,
	 "Shows a rolling ball", (void *) NULL},
	{"rotor",
	 100, 4, 100, -6, 0.30,
	 "Shows Tom's Roto-Rooter", (void *) NULL},
#ifdef USE_GL
	{"rubik",
	 100000, -30, 5, -6, 1.00,
	 "Shows an auto-solving Rubik's Cube", (void *) NULL},
#endif
#ifdef USE_GL
	{"sballs",
	 40000, 0, 10, 0, 1.00,
	 "Balls spinning like crazy in GL", (void *) NULL},
#endif
	{"scooter",
	 20000, 24, 3, 100, 1.00,
	 "Shows a journey through space tunnel and stars", (void *) NULL},
	{"shape",
	 10000, 100, 256, 1, 1.00,
	 "Shows stippled rectangles, ellipses, and triangles", (void *) NULL},
	{"sierpinski",
	 400000, 2000, 100, 1, 1.00,
	 "Shows Sierpinski's triangle", (void *) NULL},
#ifdef USE_GL
	{"sierpinski3d",
	 15000, 1, 2, 1, 1.00,
	 "Shows GL's Sierpinski gasket", (void *) NULL},
#endif
#ifdef USE_GL
	{"skewb",
	 100000, -30, 5, 1, 1.00,
	 "Shows an auto-solving Skewb", (void *) NULL},
#endif
	{"slip",
	 50000, 35, 50, 1, 1.00,
	 "Shows slipping blits", (void *) NULL},
#ifdef HAVE_CXX
	{"solitaire",
	 2000000, 1, 1, 1, 1.00,
	 "Shows Klondike's game of solitaire", (void *) NULL},
#endif
#ifdef USE_UNSTABLE
	{"space",
	 10000, 100, 1, 15, 1.00,
	 "Shows a journey into deep space", (void *) NULL},
#endif
	{"sphere",
	 5000, 1, 20, 0, 1.00,
	 "Shows a bunch of shaded spheres", (void *) NULL},
	{"spiral",
	 5000, -40, 350, 1, 1.00,
	 "Shows a helical locus of points", (void *) NULL},
	{"spline",
	 30000, -6, 2048, 1, 0.30,
	 "Shows colorful moving splines", (void *) NULL},
#ifdef USE_GL
	{"sproingies",
	 80000, 5, 0, 0, 1.00,
	 "Shows Sproingies!  Nontoxic.  Safe for pets and small children", (void *) NULL},
#endif
#ifdef USE_GL
	{"stairs",
	 200000, 0, 1, 1, 1.00,
	 "Shows some Infinite Stairs, an Escher-like scene", (void *) NULL},
#endif
	{"star",
	 75000, 100, 1, 100, 0.30,
	 "Shows a star field with a twist", (void *) NULL},
	{"starfish",
	 10000, 1, 1, 1, 1.00,
	 "Shows starfish", (void *) NULL},
	{"strange",
	 1000, 1, 1, 1, 1.00,
	 "Shows strange attractors", (void *) NULL},
#ifdef USE_GL
	{"superquadrics",
	 40000, 25, 40, 1, 1.00,
	 "Shows 3D mathematical shapes", (void *) NULL},
#endif
	{"swarm",
	 15000, -100, 1, -10, 1.00,
	 "Shows a swarm of bees following a wasp", (void *) NULL},
	{"swirl",
	 5000, 5, 1, 1, 1.00,
	 "Shows animated swirling patterns", (void *) NULL},
	{"t3d",
	 250000, 1000, 60000, 0, 1.00,
	 "Shows a Flying Balls Clock Demo", (void *) NULL},
	{"tetris",
	 50000, 1, 1, -100, 1.00,
	 "Shows an autoplaying tetris game", (void *) NULL},
#if defined(USE_GL) && defined(HAVE_CXX) && defined( HAVE_TTF ) && defined( HAVE_GLTT )
	{"text3d",
	 100000, 1, 10, 1, 1.00,
	 "Shows 3D text", (void *) NULL},
#endif
#if defined(USE_GL) && defined(HAVE_CXX) && defined( HAVE_FREETYPE ) && defined( HAVE_FTGL )
	{"text3d2",
	 100000, 1, 10, 1, 1.00,
	 "Shows 3D text", (void *) NULL},
#endif
	{"thornbird",
	 1000, 800, 16, 1, 1.00,
	 "Shows an animated bird in a thorn bush fractal map", (void *) NULL},
	{"tik_tak",
	 60000, -20, 200, -1000, 1.00,
	 "Shows rotating polygons", (void *) NULL},
	{"toneclock",
	 60000, -20, 200, -1000, 1.00,
	 "Shows Peter Schat's toneclock", (void *) NULL},
	{"triangle",
	 10000, 1, 1, 1, 1.00,
	 "Shows a triangle mountain range", (void *) NULL},
	{"tube",
	 25000, -9, 20000, -200, 1.00,
	 "Shows an animated tube", (void *) NULL},
	{"turtle",
	 1000000, 1, 20, 1, 1.00,
	 "Shows turtle fractals", (void *) NULL},
	{"vines",
	 200000, 0, 1, 1, 1.00,
	 "Shows fractals", (void *) NULL},
	{"voters",
	 1000, 0, 327670, 0, 1.00,
	 "Shows Dewdney's Voters", (void *) NULL},
	{"wator",
	 750000, 1, 32767, 0, 1.00,
	 "Shows Dewdney's Water-Torus planet of fish and sharks", (void *) NULL},
	{"wire",
	 500000, 1000, 150, -8, 1.00,
	 "Shows a random circuit with 2 electrons", (void *) NULL},
	{"world",
	 100000, -16, 1, 1, 0.30,
	 "Shows spinning Earths", (void *) NULL},
	{"worm",
	 17000, -20, 10, -3, 1.00,
	 "Shows wiggly worms", (void *) NULL},
	{"xcl",
	 20000, -3, 1, 1, 1.00,
	 "Shows a control line combat model race", (void *) NULL},
	{"xjack",
	 50000, 1, 1, 1, 1.00,
	 "Shows Jack having one of those days", (void *) NULL},
	{"blank",
	 3000000, 1, 1, 1, 1.00,
	 "Shows nothing but a black screen", (void *) NULL},
#ifdef USE_BOMB
	{"bomb",
	 100000, 10, 20, 1, 1.0,
	 "Shows a bomb and will autologout after a time", (void *) NULL},
	{"random",
	 1, 1, 1, 1, 1.0,
	 "Shows a random mode from above except blank and bomb", (void *) NULL},
#else
	{"random",
	 1, 1, 1, 1, 1.0,
	 "Shows a random mode from above except blank", (void *) NULL},
#endif
};

/* Number of modes (set in main) */
guint nb_mode = 0;

/* Defaults values available */
#define DEF_DELAY		0
#define DEF_BATCHCOUNT		1
#define DEF_CYCLES		2
#define DEF_SIZE		3
#define DEF_SATURATION		4
#define NB_DEFAULTED_OPTIONS 	5

/* Default values options names */
static gchar *defaulted_options[NB_DEFAULTED_OPTIONS] = {
  "delay",
  "batchcount",
  "cycles",
  "size",
  "saturation"};

/* Number of default values options */
static guint nb_defaultedOptions = NB_DEFAULTED_OPTIONS;

/*------------------------------------*/
/*       Boolean xlock options        */
/*------------------------------------*/

/* boolean option entry */
typedef struct struct_option_bool_s {
	gchar       *cmdarg;
	gchar       *label;
	gchar       *desc;
	gchar	       defval;
	gchar 	     value;
} struct_option_bool;

/* Description of the boolean options */
struct_option_bool BoolOpt[] =
{
  {"mono", "mono", "The mono option causes xlock to display monochrome", '\000', '\000'},
  {"nolock", "nolock", "The nolock option causes xlock to only draw the patterns and not lock the display", '\000', '\000'},
  {"remote", "remote", "The remote option causes xlock to not stop you from locking remote X11 server", '\000', '\000'},
  {"allowroot", "allowroot", "The allowroot option allow the root password to unlock the server", '\000', '\000'},
  {"enablesaver", "enablesaver", "This option enables the use of the normal Xserver screen saver", '\000', '\000'},
  {"resetsaver", "resetsaver", "This option enables the call of XResetScreenSaver", '\000', '\000'},
  {"allowaccess", "allowaccess", "For servers not allowing clients to modify host access, left the X11 server open", '\000', '\000'},
#ifdef USE_VTLOCK
  {"lockvt", "lockvt", "This option control the VT switch locking", '\000', '\000'},
#endif
  {"mousemotion", "mousemotion", "Allows to turn on/off the mouse sensitivity to bring up pass window", '\001', '\000'},
  {"grabmouse", "grabmouse", "This option causes xlock to grab mouse and keyboard", '\001', '\000'},
  {"grabserver", "grabserver", "The grabserver option causes xlock to grab the server", '\001', '\000'},
  {"echokeys", "echokeys", "This option causes xlock to echo a question mark for each typed character", '\000', '\000'},
  {"usefirst", "usefirst", "This option enables xlock to use the first keystroke in the password", '\000', '\000'},
  {"verbose", "verbose", "verbose launch", '\000', '\000'},
  {"debug", "debug", "This option allows xlock to be locked in a window", '\000', '\000'},
  {"wireframe", "wireframe", "This option turns on wireframe rendering mode mainly for GL", '\000', '\000'},
#ifdef USE_GL
  {"showfps", "showfps", "This option turns on frame per sec display for GL", '\000', '\000'},
  {"fpstop", "fpstop", "This option turns on top fps display for GL", '\000', '\000'},
#endif
  {"install", "install", "Allows xlock to install its own colormap if xlock runs out of colors", '\000', '\000'},
  {"sound", "sound", "Allows you to turn on and off sound if installed with the capability", '\000', '\000'},
  {"timeelapsed", "timeelapsed", "Allows you to find out how long a machine is locked", '\000', '\000'},
  {"fullrandom", "fullrandom", "Turn on/off randomness options within modes", '\000', '\000'},
  {"use3d", "use3d", "Turn on/off 3d view, available on bouboule, pyro, star, and worm", '\000', '\000'},
  {"trackmouse", "trackmouse", "Turn on and off mouse interaction in eyes, julia, and swarm", '\000', '\000'},
#if 0
  {"dtsaver", "dtsaver", "Turn on/off CDE Saver Mode. Only available if CDE support was compiled in", '\000', '\000'},
#endif
};

/* Number of boolean options (set in main) */
guint nb_boolOpt = 0;

/* Boolean option dialog callback struct */
typedef struct struct_option_bool_callback_s {
    GtkWidget		*boolopt_dialog;
} struct_option_bool_callback;

/*------------------------------------*/
/*          General options           */
/*------------------------------------*/

/* Gen option entry struct */
typedef struct struct_option_gen_s {
    gchar      *cmdarg;
    gchar      *label;
    gchar      *desc;
    gchar      *help_anchor;
    GtkWidget  *text_widget;
} struct_option_gen;

/* Description of the general option */
struct_option_gen generalOpt[] =
{
  {"username", "username", "text string to use for Name prompt", "opt_gen_", (GtkWidget *)NULL},
  {"password", "password", "text string to use for Password prompt", "gen_opt_", (GtkWidget *)NULL},
  {"info", "info", "text string to use for instruction", "gen_opt_", (GtkWidget *)NULL},
  {"validate", "validate", "text string to use for validating password message", "gen_opt_", (GtkWidget *)NULL},
  {"invalid", "invalidate", "text string to use for invalid password message", "gen_opt_", (GtkWidget *)NULL},
  {"message", "message", "message to say", "gen_opt_", (GtkWidget *)NULL},
  {"delay", "delay", "The delay option sets the speed at which a mode will operate", "gen_opt_", (GtkWidget *)NULL},
  {"batchcount", "batchcount", "This option sets number of things to do per batch to num", "gen_opt_", (GtkWidget *)NULL},
  {"cycles", "cycles", "This option delay is used for some mode as parameter", "gen_opt_", (GtkWidget *)NULL},
  {"ncolors", "ncolors", "This option delay is used for some mode as parameter", "gen_opt_", (GtkWidget *)NULL},
  {"size", "size", "This option delay is used for some mode as parameter", "gen_opt_", (GtkWidget *)NULL},
  {"saturation", "saturation", "This option delay is used for some GL mode as parameter", "gen_opt_", (GtkWidget *)NULL},
  {"nice", "nice", "This option sets system nicelevel of the xlock process", "gen_opt_", (GtkWidget *)NULL},
  {"lockdelay", "lockdelay", "This option set the delay between launch and lock", "gen_opt_", (GtkWidget *)NULL},
  {"timeout", "timeout", "The timeout option sets the password screen timeout", "gen_opt_", (GtkWidget *)NULL},
  {"geometry", "geometry", "geometry of mode window", "gen_opt_", (GtkWidget *)NULL},
  {"icongeometry", "icongeometry", "geometry of mode icon window", "gen_opt_", (GtkWidget *)NULL},
  {"glgeometry", "geometry", "geometry of GL mode window", "gen_opt_", (GtkWidget *)NULL},
  {"delta3d", "delta3d", "Turn on/off 3d view, available on bouboule, pyro, star, and worm", "gen_opt_", (GtkWidget *)NULL},
  {"neighbors", "neighbors", "Sets number of neighbors of cell (3, 4, 6, 9, 12) for automata modes", "gen_opt_", (GtkWidget *)NULL},
  {"cpasswd", "cpasswd", "Sets the key to be this text string to unlock", "gen_opt_", (GtkWidget *)NULL},
  {"program", "program", "program used as a fortune generator", "gen_opt_", (GtkWidget *)NULL},
#ifdef USE_AUTO_LOGOUT
  {"forceLogout", "forceLogout", "This option sets minutes to auto-logout", "gen_opt_", (GtkWidget *)NULL},
#endif
#ifdef USE_BUTTON_LOGOUT
  {"logoutButtonHelp", "logoutButtonHelp", "Text string is a message shown outside logout", "gen_opt_", (GtkWidget *)NULL},
  {"logoutButtonLabel", "logoutButtonLabel", "Text string is a message shown inside logout button", "gen_opt_", (GtkWidget *)NULL},
#endif
#if defined( USE_AUTO_LOGOUT ) || defined( USE_BUTTON_LOGOUT )
  {"logoutFailedString", "logoutFailedString", "Text string is shown when a logout is attempted and fails", "gen_opt_", (GtkWidget *)NULL},
#endif
  {"startCmd", "startCmd", "Command to execute when the screen is locked", "gen_opt_", (GtkWidget *)NULL},
  {"endCmd", "endCmd", "Command to execute when the screen is unlocked", "gen_opt_", (GtkWidget *)NULL},
  {"pipepassCmd", "pipepassCmd", "Command into which to pipe the password when the screen is unlocked", "gen_opt_", (GtkWidget *)NULL},
#if defined( USE_AUTO_LOGOUT ) || defined( USE_BUTTON_LOGOUT )
  {"logoutCmd", "logoutCmd", "Command to execute when the user is logged out", "gen_opt_", (GtkWidget *)NULL},
#endif
  {"mailCmd", "mailCmd", "Command to execute when mails are checked", "gen_opt_", (GtkWidget *)NULL},
#ifdef USE_DPMS
  {"dpmsstandby", "dpmsstandby", "Allows one to set DPMS Standby for monitor (0 is infinite)", "gen_opt_", (GtkWidget *)NULL},
  {"dpmssuspend", "dpmssuspend", "Allows one to set DPMS Suspend for monitor (0 is infinite)", "gen_opt_", (GtkWidget *)NULL},
  {"dpmsoff", "dpmsoff", "Allows one to set DPMS Power Off for monitor (0 is infinite)", "gen_opt_", (GtkWidget *)NULL},
#endif
};

/* Number of general options (set in main) */
guint nb_genOpt = 0;

/* General option dialog callback struct */
typedef struct struct_option_gen_callback_s {
    GtkWidget  *gen_dialog;
    GtkWidget  *text_widget;
} struct_option_gen_callback;

/*------------------------------------*/
/*      Font/File/Color options       */
/*------------------------------------*/

/* Option type (font/color/file) */
typedef enum enum_type_option_fntcol_e {
    TFNTCOL_FONT = 0,
    TFNTCOL_COLOR,
    TFNTCOL_FILE
} enum_type_option_fntcol;

/* Font/Color/File option entry struct */
typedef struct struct_option_fntcol_s {
    enum_type_option_fntcol 	type;
    gchar       	       *cmdarg;
    gchar       	       *label;
    gchar       	       *desc;
    gchar		       *help_anchor;
    GtkWidget		       *entry;
    GtkWidget		       *drawing_area;
} struct_option_fntcol;

/* Description of the font and color option */
struct_option_fntcol fntcolorOpt[] =
{
  {TFNTCOL_COLOR, "bg", "background", "background color to use for password prompt", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_COLOR, "fg", "foreground", "foreground color to use for password prompt", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_COLOR, "none3d", "none3d", "color used for empty size in 3d mode", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_COLOR, "right3d", "right3d", "color used for right eye in 3d mode", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_COLOR, "left3d", "left3d", "color used for left eye in 3d mode", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_COLOR, "both3d", "both3d", "color used for overlapping images for left and right eye in 3d mode", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FONT, "font", "font", "font to use for password prompt", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FONT, "messagefont", "msgfont", "font to use for message", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FONT, "planfont", "planfont", "font to use for lower part of password screen", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
#ifdef USE_GL
  {TFNTCOL_FONT, "fpsfont", "fpsfont", "font to use for FPS display in GL mode", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
#endif
  {TFNTCOL_FILE, "messagesfile", "messagesfile", "file to be used as the fortune generator", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FILE, "messagefile", "messagefile", "file whose contents are displayed", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FILE, "bitmap", "bitmap", "sets the xbm, xpm, or ras file to be displayed with some modes", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FILE, "lifefile", "lifefile", "sets the lifeform (only one format: #P xlife format)", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
  {TFNTCOL_FILE, "life3dfile", "life3dfile", "sets the lifeform (only one format similar to #P xlife format)", "opt_fntcol_",
     (GtkWidget *)NULL, (GtkWidget *)NULL},
};

/* Number of font/color/file options (set in main) */
guint nb_fntColorOpt = 0;

/* Font/Color/File option dialog callback struct */
typedef struct struct_option_fntcol_callback_s {
    GtkWidget  		       *fntcol_dialog;
    GtkWidget		       *entry;
    GtkWidget		       *drawing_area;
} struct_option_fntcol_callback;

/* Colors handling */
GdkVisual *gdkvisual;
GdkColormap *gdkcolormap;

#endif /* !_LMODE_H_ */
