
#ifndef lint
static char sccsid[] = "@(#)mode.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * mode.c - Modes for xlock. 
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 23-Feb-96: Extensive revision to implement new mode hooks and stuff
 *		Ron Hitchens <ron@utw.com>
 * 04-Sep-95: Moved over from mode.h (previously resource.h) with new
 *            "&*_opts" by Heath A. Kehoe <hakehoe@icaen.uiowa.edu>.
 *
 */

#include <ctype.h>
#include "xlock.h"

/* -------------------------------------------------------------------- */

LockStruct  LockProcs[] =
{
	{"ant", init_ant, draw_ant, release_ant, NULL, NULL, NULL,
	 &ant_opts, 1000, -3, 40000, 1.0,
	 "Langton and Turk's generalized ants", 0, NULL},
	{"bat", init_bat, draw_bat, release_bat, NULL, NULL, NULL,
	 &bat_opts, 100000, -8, 20, 1.0,
	 "Flying Bats", 0, NULL},
	{"blot", init_blot, draw_blot, release_blot, NULL, NULL, NULL,
	 &blot_opts, 100000, 6, 30, 0.4,
	 "Rorschach's ink blot test", 0, NULL},
{"bouboule", init_bouboule, draw_bouboule, release_bouboule, NULL, NULL, NULL,
 &bouboule_opts, 10000, 100, 15, 1.0,
 "Moving stars Mimi's bouboule", 0, NULL},
	{"bounce", init_bounce, draw_bounce, release_bounce, NULL, NULL, NULL,
	 &bounce_opts, 10000, -10, 20, 1.0,
	 "Bouncing ball", 0, NULL},
	{"braid", init_braid, draw_braid, release_braid, NULL, NULL, NULL,
	 &braid_opts, 1000, 15, 30, 1.0,
	 "Random braids", 0, NULL},
	{"bug", init_bug, draw_bug, release_bug, NULL, NULL, NULL,
	 &bug_opts, 75000, 10, 32767, 1.0,
	 "Palmiter's bug evolution", 0, NULL},
	{"clock", init_clock, draw_clock, release_clock, NULL, NULL, NULL,
	 &clock_opts, 100000, 30, 200, 1.0,
	 "Clock", 0, NULL},
	{"demon", init_demon, draw_demon, release_demon, NULL, NULL, NULL,
	 &demon_opts, 50000, 16, 1000, 1.0,
	 "Griffeath's cellular automata", 0, NULL},
	{"eyes", init_eyes, draw_eyes, release_eyes, refresh_eyes, NULL, NULL,
	 &eyes_opts, 20000, -8, 5, 1.0,
	 "Someone's Watching You", 0, NULL},
	{"flag", init_flag, draw_flag, release_flag, NULL, NULL, NULL,
	 &flag_opts, 10000, -7, 1000, 1.0,
	 "Flying Flag", 0, NULL},
	{"flame", init_flame, draw_flame, release_flame, NULL, NULL, NULL,
	 &flame_opts, 750000, 20, 10000, 1.0,
	 "Cosmic Flame Fractals", 0, NULL},
	{"forest", init_forest, draw_forest, release_forest, NULL, NULL, NULL,
	 &forest_opts, 400000, 100, 200, 1.0,
	 "Fractal Forest", 0, NULL},
	{"galaxy", init_galaxy, draw_galaxy, release_galaxy, NULL, NULL, NULL,
	 &galaxy_opts, 100, -5, 250, 1.0,
	 "Spinning galaxies", 0, NULL},
{"geometry", init_geometry, draw_geometry, release_geometry, NULL, NULL, NULL,
 &geometry_opts, 2000, -10, 20, 1.0,
 "Complete morphing", 0, NULL},
	{"grav", init_grav, draw_grav, release_grav, NULL, NULL, NULL,
	 &grav_opts, 10000, -12, 20, 1.0,
	 "Orbiting planets", 0, NULL},
	{"helix", init_helix, draw_helix, release_helix, NULL, NULL, NULL,
	 &helix_opts, 10000, 1, 100, 1.0,
	 "Helix", 0, NULL},
	{"hop", init_hop, draw_hop, release_hop, NULL, NULL, NULL,
	 &hop_opts, 10000, 1000, 2500, 1.0,
	 "Hopalong iterated fractals", 0, NULL},
	{"hyper", init_hyper, draw_hyper, release_hyper, NULL, NULL, NULL,
	 &hyper_opts, 10000, 1, 300, 1.0,
	 "Spinning Tesseract", 0, NULL},
	{"image", init_image, draw_image, release_image, NULL, NULL, NULL,
	 &image_opts, 2000000, -10, 20, 1.0,
	 "Random Bouncing Image", 0, NULL},
	{"kaleid", init_kaleid, draw_kaleid, release_kaleid, NULL, NULL, NULL,
	 &kaleid_opts, 20000, 4, 700, 1.0,
	 "Kaleidoscope", 0, NULL},
	{"laser", init_laser, draw_laser, release_laser, NULL, NULL, NULL,
	 &laser_opts, 5000, -10, 200, 1.0,
	 "Laser ray", 0, NULL},
	{"life", init_life, draw_life, release_life, NULL, NULL, NULL,
	 &life_opts, 750000, 40, 140, 1.0,
	 "Conway's game of Life", 0, NULL},
	{"life1d", init_life1d, draw_life1d, release_life1d, NULL, NULL, NULL,
	 &life1d_opts, 2500000, 10, 10, 1.0,
	 "Wolfram's game of 1D Life", 0, NULL},
	{"life3d", init_life3d, draw_life3d, release_life3d, NULL, NULL, NULL,
	 &life3d_opts, 1000000, 35, 85, 1.0,
	 "Bays' game of 3D Life", 0, NULL},
 {"lightning", init_lightning, draw_lightning, release_lightning, NULL, NULL,
  NULL, &lightning_opts, 1, 1, 1, 0.6,
  "Keith's Lightning", 0, NULL},
	{"lissie", init_lissie, draw_lissie, release_lissie, NULL, NULL, NULL,
	 &lissie_opts, 10000, 1, 2000, 0.6,
	 "The Lissajous worm", 0, NULL},
	{"marquee", init_marquee, draw_marquee, release_marquee, NULL, NULL,
	 NULL, &marquee_opts, 100000, 10, 20, 1.0,
	 "Text printer", 0, NULL},
	{"maze", init_maze, draw_maze, release_maze, NULL, NULL, NULL,
	 &maze_opts, 1000, -40, 300, 1.0,
	 "aMAZEing", 0, NULL},
  {"mountain", init_mountain, draw_mountain, release_mountain, NULL, NULL,
   NULL, &mountain_opts, 300, 30, 100, 1.0,
   "Papo's Mountains", 0, NULL},
	{"nose", init_nose, draw_nose, release_nose, NULL, NULL, NULL,
	 &nose_opts, 100000, 10, 20, 1.0,
	 "Nose guy", 0, NULL},
	{"petal", init_petal, draw_petal, release_petal, NULL, NULL, NULL,
	 &petal_opts, 10000, -500, 400, 1.0,
	 "Flowers", 0, NULL},
	{"puzzle", init_puzzle, draw_puzzle, release_puzzle, NULL, NULL, NULL,
	 &puzzle_opts, 10000, 250, 100, 1.0,
	 "Puzzle", 0, NULL},
	{"pyro", init_pyro, draw_pyro, release_pyro, NULL, NULL, NULL,
	 &pyro_opts, 15000, 40, 75, 1.0,
	 "Fireworks", 0, NULL},
	{"qix", init_qix, draw_qix, release_qix, NULL, NULL, NULL,
	 &qix_opts, 30000, 100, 64, 1.0,
	 "Spinning lines a la Qix(tm)", 0, NULL},
	{"rock", init_rock, draw_rock, release_rock, NULL, NULL, NULL,
	 &rock_opts, 20000, 100, 100, 0.2,
	 "Asteroid field", 0, NULL},
	{"rotor", init_rotor, draw_rotor, release_rotor, NULL, NULL, NULL,
	 &rotor_opts, 10000, 4, 20, 0.4,
	 "Tom's Roto-Rooter", 0, NULL},
	{"shape", init_shape, draw_shape, release_shape, NULL, NULL, NULL,
	 &shape_opts, 10000, 100, 256, 1.0,
	 "Greynetic shapes", 0, NULL},
	{"slip", init_slip, draw_slip, release_slip, NULL, NULL, NULL,
	 &slip_opts, 50000, 35, 50, 1.0,
	 "Slip", 0, NULL},
	{"sphere", init_sphere, draw_sphere, release_sphere, NULL, NULL, NULL,
	 &sphere_opts, 10000, 1, 20, 1.0,
	 "Shaded spheres", 0, NULL},
	{"spiral", init_spiral, draw_spiral, release_spiral, NULL, NULL, NULL,
	 &spiral_opts, 5000, -40, 350, 1.0,
	 "Helix-ish spiral", 0, NULL},
	{"spline", init_spline, draw_spline, release_spline, NULL, NULL, NULL,
	 &spline_opts, 30000, -6, 20, 0.4,
	 "Moving Splines", 0, NULL},
	{"swarm", init_swarm, draw_swarm, release_swarm, NULL, NULL, NULL,
	 &swarm_opts, 10000, 100, 20, 1.0,
	 "Swarm of bees", 0, NULL},
	{"swirl", init_swirl, draw_swirl, release_swirl, NULL, NULL, NULL,
	 &swirl_opts, 5000, 5, 20, 1.0,
	 "Animated swirling patterns", 0, NULL},
  {"triangle", init_triangle, draw_triangle, release_triangle, NULL, NULL,
   NULL, &triangle_opts, 10000, 100, 20, 1.0,
   "Draws a triangle-mountain", 0, NULL},
	{"wator", init_wator, draw_wator, release_wator, NULL, NULL, NULL,
	 &wator_opts, 750000, 4, 32767, 1.0,
	 "Dewdney's Planet Wa-Tor", 0, NULL},
	{"wire", init_wire, draw_wire, release_wire, NULL, NULL, NULL,
	 &wire_opts, 500000, 1000, 150, 1.0,
	 "wire world", 0, NULL},
	{"world", init_world, draw_world, release_world, NULL, NULL, NULL,
	 &world_opts, 100000, -16, 20, 0.3,
	 "Random Spinning Earths", 0, NULL},
	{"worm", init_worm, draw_worm, release_worm, NULL, NULL, NULL,
	 &worm_opts, 17000, -20, 10, 1.0,
	 "Wiggly Worms", 0, NULL},
	{"blank", init_blank, draw_blank, release_blank, NULL, NULL, NULL,
	 &blank_opts, 3000000, 1, 20, 1.0,
	 "Blank screen", 0, NULL},
  {"random", init_random, draw_random, NULL, refresh_random, change_random,
   NULL, &random_opts, 1, 0, 0, 1.0,
   "Random mode", 0, NULL},
};

int         numprocs = sizeof (LockProcs) / sizeof (LockProcs[0]);

/* -------------------------------------------------------------------- */

static LockStruct *last_initted_mode = NULL;
static LockStruct *default_mode = NULL;

/* -------------------------------------------------------------------- */

void
set_default_mode(LockStruct * ls)
{
	default_mode = ls;
}

/* -------------------------------------------------------------------- */

static
void
set_window_title(ModeInfo * mi)
{
	XTextProperty prop;
	char        buf[512];
	char       *ptr = buf;

	(void) sprintf(buf, "%s: %s", MI_NAME(mi), MI_DESC(mi));

	XStringListToTextProperty(&ptr, 1, &prop);
	XSetWMName(MI_DISPLAY(mi), MI_WINDOW(mi), &prop);
}

/* -------------------------------------------------------------------- */

/* 
 *    This hook is called prior to calling the init hook of a
 *      different mode.  It is to inform the mode that it is losing
 *      control, and should therefore release any dynamically created
 *      resources.
 */

static
void
call_release_hook(LockStruct * ls, ModeInfo * mi)
{
	if (ls == NULL) {
		return;
	}
	MI_LOCKSTRUCT(mi) = ls;

	if (ls->release_hook != NULL) {
		ls->release_hook(mi);
	}
	ls->flags &= ~(LS_FLAG_INITED);

	last_initted_mode = NULL;
}

void
release_last_mode(ModeInfo * mi)
{
	if (last_initted_mode == NULL) {
		return;
	}
	call_release_hook(last_initted_mode, mi);

	last_initted_mode = NULL;
}

/* -------------------------------------------------------------------- */

/* 
 *    Call the init hook for a mode.  If this mode is not the same
 *      as the last one, call the release proc for the previous mode
 *      so that it will surrender its dynamic resources.
 *      A mode's init hook may be called multiple times, without
 *      intervening release calls.
 */

void
call_init_hook(LockStruct * ls, ModeInfo * mi)
{
	if (ls == NULL) {
		if (default_mode == NULL) {
			return;
		} else {
			ls = default_mode;
		}
	}
	if (ls != last_initted_mode) {
		call_release_hook(last_initted_mode, mi);
	}
	MI_LOCKSTRUCT(mi) = ls;

	set_window_title(mi);

	ls->init_hook(mi);

	ls->flags |= LS_FLAG_INITED;
	MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_JUST_INITTED, True);

	last_initted_mode = ls;
}

/* -------------------------------------------------------------------- */

/* 
 *    Call the callback hook for a mode.  This hook is called repeatedly,
 *      at (approximately) constant time intervals.  The time between calls
 *      is controlled by the -delay command line option, which is mapped
 *      to the variable named delay.
 */

void
call_callback_hook(LockStruct * ls, ModeInfo * mi)
{
	if (ls == NULL) {
		if (default_mode == NULL) {
			return;
		} else {
			ls = default_mode;
		}
	}
	MI_LOCKSTRUCT(mi) = ls;

	ls->callback_hook(mi);

	MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_JUST_INITTED, False);
}

/* -------------------------------------------------------------------- */

/* 
 *    Window damage has occurred.  If the mode has been initted and
 *      supplied a refresh proc, call that.  Otherwise call its init
 *      hook again.
 */

#define JUST_INITTED(mi)	(MI_WIN_FLAG_IS_SET(mi, WI_FLAG_JUST_INITTED))

void
call_refresh_hook(LockStruct * ls, ModeInfo * mi)
{
	if (ls == NULL) {
		if (default_mode == NULL) {
			return;
		} else {
			ls = default_mode;
		}
	}
	MI_LOCKSTRUCT(mi) = ls;

	if (ls->refresh_hook == NULL) {
		/*
		 * No refresh hook supplied.  If the mode has been
		 * initialized, and the callback has been called at least
		 * once, then call the init hook to do the refresh.
		 * Note that two flags are examined here.  The first
		 * indicates if the mode has ever had its init hook called,
		 * the second is a per-screen flag which indicates
		 * if the draw (callback) hook has been called since the
		 * init hook was called for that screen.
		 * This second test is a hack.  A mode should gracefully
		 * deal with its init hook being called twice in a row.
		 * Once all the modes have been updated, this hack should
		 * be removed.
		 */
		if (MODE_NOT_INITED(ls) || JUST_INITTED(mi)) {
			return;
		}
		call_init_hook(ls, mi);
	} else {
		ls->refresh_hook(mi);
	}
}

/* -------------------------------------------------------------------- */

/* 
 *    The user has requested a change, by pressing the middle mouse
 *      button.  Let the mode know about it.
 */

void
call_change_hook(LockStruct * ls, ModeInfo * mi)
{
	if (ls == NULL) {
		if (default_mode == NULL) {
			return;
		} else {
			ls = default_mode;
		}
	}
	MI_LOCKSTRUCT(mi) = ls;

	if (ls->change_hook != NULL) {
		ls->change_hook(mi);
	}
}

/* -------------------------------------------------------------------- */
