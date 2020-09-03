
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)sproingiewrap.c	4.2 97/03/22 xlockmore";

#endif

/*-
 * sproingiewrap.c - Copyright 1996 Sproingie Technologies Incorporated.
 *                   Source and binary freely distributable under the
 *                   terms in xlock.c
 *
 *    Programming:  Ed Mackey, http://www.early.com/~emackey/
 *    Sproingie 3D objects modeled by:  Al Mackey, al@iam.com
 *       (using MetaNURBS in NewTek's Lightwave 3D v5).
 *
 * Revision History:
 * 28-Mar-97: Added size support.
 * 22-Mar-97: Updated to use glX interface instead of xmesa one.
 *              Also, support for multiscreens added.
 * 20-Mar-97: Updated for xlockmore v4.02alpha7 and higher, using
 *              xlockmore's built-in Mesa/OpenGL support instead of
 *              my own.  Submitted for inclusion in xlockmore.
 * 09-Dec-96: Written.
 *
 * Ed Mackey
 */

#include "xlock.h"

#if defined( USE_GL ) && defined( USE_SPROINGIES )

#define MINSIZE 32

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/xmesa.h>		/* Do we need this?? */
#include <time.h>

ModeSpecOpt sproingies_opts =
{0, NULL, 0, NULL, NULL};

void        NextSproingie(int screen);
void        NextSproingieDisplay(int screen);
void        DisplaySproingies(int screen);
void        ReshapeSproingies(int w, int h);
void        CleanupSproingies(int screen);
void        InitSproingies(int wfmode, int grnd, int mspr, int screen, int numscreens);

typedef struct {
	GLfloat     view_rotx, view_roty, view_rotz;
	GLint       gear1, gear2, gear3;
	GLfloat     angle;
	GLuint      limit;
	GLuint      count;
	GLXContext  glx_context;
	int         mono;
} sproingiesstruct;

static sproingiesstruct *sproingies = NULL;

static Display *swap_display;
static Window swap_window;

void
SproingieSwap(void)
{
	glFinish();
	glXSwapBuffers(swap_display, swap_window);
}


/* ARGSUSED */
void
draw_sproingies(ModeInfo * mi)
{
	sproingiesstruct *sp = &sproingies[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	glXMakeCurrent(display, window, sp->glx_context);

	swap_display = display;
	swap_window = window;

	NextSproingieDisplay(MI_SCREEN(mi));	/* It will swap. */
}

void
refresh_sproingies(ModeInfo * mi)
{
	/* No need to do anything here... The whole screen is updated
	 * every frame anyway.  Otherwise this would be just like
	 * draw_sproingies, above, but replace NextSproingieDisplay(...)
	 * with DisplaySproingies(...).
	 */
}

void
release_sproingies(ModeInfo * mi)
{
	if (sproingies != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			sproingiesstruct *sp = &sproingies[screen];

			CleanupSproingies(MI_SCREEN(mi));

			if (sp->glx_context)
				glXDestroyContext(MI_DISPLAY(mi), sp->glx_context);
#if 0 /* This is wrong for multiscreens anyway */
#ifdef GLX_MESA_release_buffers
			glXReleaseBuffersMESA(MI_DISPLAY(mi), MI_WINDOW(mi));
#endif
#endif
		}
		(void) free((void *) sproingies);
		sproingies = NULL;
	}
}

void
init_sproingies(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         screen = MI_SCREEN(mi);

	int         cycles = MI_CYCLES(mi);
	int         batchcount = MI_BATCHCOUNT(mi);
	int         size = MI_SIZE(mi);

	sproingiesstruct *sp;
	int         wfmode = 0, grnd, mspr, w, h;

	if (sproingies == NULL) {
		if ((sproingies = (sproingiesstruct *) calloc(MI_NUM_SCREENS(mi),
					 sizeof (sproingiesstruct))) == NULL)
			return;
	}
	sp = &sproingies[screen];

	sp->mono = (MI_WIN_IS_MONO(mi) ? 1 : 0);

	if (sp->glx_context) {
		glXDestroyContext(display, sp->glx_context);
		sp->glx_context = NULL;
	} {
		int         n;
		XVisualInfo *wantVis, vTemplate;
		extern int  VisualClassWanted;

		vTemplate.screen = screen;
		vTemplate.depth = MI_WIN_DEPTH(mi);
		if (VisualClassWanted == -1) {
			vTemplate.class = DefaultVisual(display, screen)->class;
		} else {
			vTemplate.class = VisualClassWanted;
		}
		wantVis = XGetVisualInfo(display,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
					 &vTemplate, &n);
		if (VisualClassWanted != -1 && n == 0) {
			/* Wanted visual not found so use default */
			vTemplate.class = DefaultVisual(display, screen)->class;
			wantVis = XGetVisualInfo(display,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
						 &vTemplate, &n);
		}
		/* if User asked for color, try that first, then try mono */
		/* if User asked for mono.  Might fail on 16/24 bit displays,
		   so fall back on color, but keep the mono "look & feel". */
		if (!getVisual(mi, wantVis, sp->mono)) {
			if (!getVisual(mi, wantVis, !sp->mono)) {
				(void) fprintf(stderr, "GL can not render with root visual\n");
				return;
			}
		}
		if (MI_WIN_IS_VERBOSE(mi))
			showVisualInfo(wantVis);
/* PURIFY 3.0a on SunOS4 reports a 104 byte memory leak on the next line each 
 * time that sproingies mode is run in random mode. */ 
		sp->glx_context = glXCreateContext(display, wantVis, 0, True);
		XFree((char *) wantVis);
	}

	glXMakeCurrent(display, window, sp->glx_context);

	if ((cycles & 1) || MI_WIN_IS_WIREFRAME(mi) || sp->mono)
		wfmode = 1;
	grnd = (cycles >> 1);
	if (grnd > 2)
		grnd = 2;

	mspr = batchcount;
	if (mspr > 10)
		mspr = 10;

	if (sp->mono) {
		glIndexi(WhitePixel(display, screen));
		glClearIndex(BlackPixel(display, screen));
	}
	/* wireframe, ground, maxsproingies */
	InitSproingies(wfmode, grnd, mspr, MI_SCREEN(mi), MI_NUM_SCREENS(mi));

	/* Default viewport size is 1/16 of screen, full size in password screen */
	if (MI_WIN_IS_ICONIC(mi)) {
		w = MI_WIN_WIDTH(mi);
		h = MI_WIN_HEIGHT(mi);
	} else {
		w = MI_WIN_WIDTH(mi) / 4;
		h = MI_WIN_HEIGHT(mi) / 4;
	}
	/* Viewport is specified size if size >= MINSIZE && size < screensize */
	if (size >= MINSIZE && size <= MI_WIN_WIDTH(mi) && size <= MI_WIN_HEIGHT(mi)) {
		w = size;
		h = size;
	}
	/* Viewport is screensize / size if size < MINSIZE and size > 0 */
	if (size < MINSIZE && size > 0) {
		w = MI_WIN_WIDTH(mi) / size;
		h = MI_WIN_HEIGHT(mi) / size;
		if (MI_WIN_IS_ICONIC(mi)) {
			if (w > MI_WIN_WIDTH(mi))
				w = MI_WIN_WIDTH(mi);
			if (h > MI_WIN_HEIGHT(mi))
				h = MI_WIN_HEIGHT(mi);
		}
	}
	if (w < MINSIZE)
		w = MINSIZE;	/* Smallest allowed */
	if (h < MINSIZE)
		h = MINSIZE;

	glViewport((MI_WIN_WIDTH(mi) - w) / 2, (MI_WIN_HEIGHT(mi) - h) / 2, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65.0, (GLfloat) w / (GLfloat) h, 0.1, 2000.0);	/* was 200000.0 */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	swap_display = display;
	swap_window = window;
	DisplaySproingies(MI_SCREEN(mi));
}

#endif

/* End of sproingiewrap.c */
