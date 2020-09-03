
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)gears.c	4.02 97/04/01 xlockmore";

#endif

/*-
 * gears.c - 3D gear wheels
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 22-Mar-97: Added support for -mono mode, and monochrome X servers.
 *              Ed Mackey, emackey@early.com
 * 13-Mar-97: Memory leak fix by Tom Schmidt <tschmidt@micron.com>
 * 1996: "written" by Danny Sung <dannys@ucla.edu>
 *       Based on 3-D gear wheels by Brian Paul which is in the public domain.
 *
 * Brian Paul
 */

/*-
   It actually took almost no work on my part to port to xlock.
   The gears code was taken from Mesa's example programs gears.
   Obviously you will need MesaGL (or OpenGL) to get this to work.
   It may not really worth running on a 486-66.
 */

/*-
 * PURIFY 3.0a on SunOS4 reports an unitialized memory read on each of
 * the glCallList() functions below when using MesaGL 2.1.  This has
 * been fixed in MesaGL 2.2 and later releases.
 */

#include "xlock.h"

#ifdef USE_GL
#include <X11/Intrinsic.h>
#include <GL/xmesa.h>
#include <GL/gl.h>
#include <GL/glx.h>

ModeSpecOpt gears_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	GLfloat     view_rotx, view_roty, view_rotz;
	GLuint       gear1, gear2, gear3;
	GLfloat     angle;
	int         mono;
	GLXContext  glx_context;
#if 0
  Window win;
#endif
} gearsstruct;

static gearsstruct *gears = NULL;

/* 
 * Draw a gear wheel.  You'll probably want to call this function when
 * building a display list since we do a lot of trig here.
 *
 * Input:  inner_radius - radius of hole at center
 *         outer_radius - radius at center of teeth
 *         width - width of gear
 *         teeth - number of teeth
 *         tooth_depth - depth of tooth
 *         wire - true for wireframe mode
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth, Bool wire)
{
	GLint       i;
	GLfloat     r0, r1, r2;
	GLfloat     angle, da;
	GLfloat     u, v, len;

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;

	da = 2.0 * M_PI / teeth / 4.0;

	glShadeModel(GL_FLAT);

	/* This subroutine got kind of messy when I added all the checks
	 * for wireframe mode.  A much cleaner solution that I sometimes
	 * use is to have a variable hold the value GL_LINE_LOOP when
	 * in wireframe mode, or hold the value GL_POLYGON otherwise.
	 * Then I just call glBegin(that_variable), give my polygon
	 * coordinates, and glEnd().  Pretty neat eh?  Too bad I couldn't
	 * integrate that trick here.
	 *                                  --Ed.
	 */

	if (!wire)
		glNormal3f(0.0, 0.0, 1.0);

	/* draw front face */
	if (!wire)
		glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		if (wire)
			glBegin(GL_LINES);
		angle = i * 2.0 * M_PI / teeth;
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		if (!wire) {
			glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
		} else {
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
			glVertex3f(r1 * cos(angle + 4 * da), r1 * sin(angle + 4 * da), width * 0.5);
			glEnd();
		}
	}
	if (!wire)
		glEnd();

	/* draw front sides of teeth */
	if (!wire)
		glBegin(GL_QUADS);
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		if (wire)
			glBegin(GL_LINE_LOOP);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
		if (wire)
			glEnd();
	}
	if (!wire)
		glEnd();


	if (!wire)
		glNormal3f(0.0, 0.0, -1.0);

	/* draw back face */
	if (!wire)
		glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		if (wire)
			glBegin(GL_LINES);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		if (!wire) {
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
			glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		} else {
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
			glVertex3f(r1 * cos(angle + 4 * da), r1 * sin(angle + 4 * da), -width * 0.5);
			glEnd();
		}
	}
	if (!wire)
		glEnd();

	/* draw back sides of teeth */
	if (!wire)
		glBegin(GL_QUADS);
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		if (wire)
			glBegin(GL_LINE_LOOP);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		if (wire)
			glEnd();
	}
	if (!wire)
		glEnd();


	/* draw outward faces of teeth */
	if (!wire)
		glBegin(GL_QUAD_STRIP);
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		if (wire)
			glBegin(GL_LINES);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		u = r2 * cos(angle + da) - r1 * cos(angle);
		v = r2 * sin(angle + da) - r1 * sin(angle);
		len = sqrt(u * u + v * v);
		u /= len;
		v /= len;
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glNormal3f(v, -u, 0.0);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
		u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
		v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glNormal3f(v, -u, 0.0);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
		if (wire)
			glEnd();
	}

	if (!wire) {
		glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
		glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);
		glEnd();
	}
	if (!wire)
		glShadeModel(GL_SMOOTH);

	/* draw inside radius cylinder */
	if (!wire)
		glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		if (wire)
			glBegin(GL_LINES);
		glNormal3f(-cos(angle), -sin(angle), 0.0);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
		if (wire) {
			glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
			glVertex3f(r0 * cos(angle + 4 * da), r0 * sin(angle + 4 * da), -width * 0.5);
			glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
			glVertex3f(r0 * cos(angle + 4 * da), r0 * sin(angle + 4 * da), width * 0.5);
			glEnd();
		}
	}
	if (!wire)
		glEnd();

}

static void
draw(ModeInfo * mi)
{
	gearsstruct *gp = &gears[MI_SCREEN(mi)];
	int         wire = MI_WIN_IS_WIREFRAME(mi) || gp->mono;

	if (!wire) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glPushMatrix();
	glRotatef(gp->view_rotx, 1.0, 0.0, 0.0);
	glRotatef(gp->view_roty, 0.0, 1.0, 0.0);
	glRotatef(gp->view_rotz, 0.0, 0.0, 1.0);

	glPushMatrix();
	glTranslatef(-3.0, -2.0, 0.0);
	glRotatef(gp->angle, 0.0, 0.0, 1.0);
	glCallList(gp->gear1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.1, -2.0, 0.0);
	glRotatef(-2.0 * gp->angle - 9.0, 0.0, 0.0, 1.0);
	glCallList(gp->gear2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.1, 4.2, 0.0);
	glRotatef(-2.0 * gp->angle - 25.0, 0.0, 0.0, 1.0);
	glCallList(gp->gear3);
	glPopMatrix();

	glPopMatrix();
}



/* new window size or exposure */
static void
reshape(int width, int height)
{
	GLfloat     h = (GLfloat) height / (GLfloat) width;

	glViewport(0, 0, (GLint) width, (GLint) height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -40.0);

	/* The depth buffer will be cleared, if needed, before the
	 * next frame.  Right now we just want to black the screen.
	 */
	glClear(GL_COLOR_BUFFER_BIT);

}


static void
pinit(ModeInfo * mi)
{
	gearsstruct *gp = &gears[MI_SCREEN(mi)];
	static GLfloat pos[4] =
	{5.0, 5.0, 10.0, 1.0};
	static GLfloat red[4] =
	{0.8, 0.1, 0.0, 1.0};
	static GLfloat green[4] =
	{0.0, 0.8, 0.2, 1.0};
	static GLfloat blue[4] =
	{0.2, 0.2, 1.0, 1.0};
	int         wire = MI_WIN_IS_WIREFRAME(mi) || gp->mono;

	if (!wire) {
		glLightfv(GL_LIGHT0, GL_POSITION, pos);
		glEnable(GL_CULL_FACE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_DEPTH_TEST);
	}

#if 0
/*-
 * Messes up on multiscreen Pseudocolor:0 StaticGray(monochrome):1
 * 2nd time mode is run it is Grayscale on PseudoColor.
 * The code below forces monochrome on TrueColor.
 */
  if (MI_WIN_IS_MONO(mi)) {
              red[0] = red[1] = red[2] = 1.0;
              green[0] = green[1] = green[2] = 1.0;
              blue[0] = blue[1] = blue[2] = 1.0;
  }
#endif

	/* make the gears */
	if (glIsList(gp->gear1))
		glDeleteLists(gp->gear1, 1);
	gp->gear1 = glGenLists(1);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
	glNewList(gp->gear1, GL_COMPILE);
	if (!wire)
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
	else
		glColor4fv(red);
	gear(1.0, 4.0, 1.0, 20, 0.7, wire);
	glEndList();

	if (glIsList(gp->gear2))
		glDeleteLists(gp->gear2, 1);
	gp->gear2 = glGenLists(1);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
	glNewList(gp->gear2, GL_COMPILE);
	if (!wire)
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
	else
		glColor4fv(green);
	gear(0.5, 2.0, 2.0, 10, 0.7, wire);
	glEndList();

	if (glIsList(gp->gear3))
		glDeleteLists(gp->gear3, 1);
	gp->gear3 = glGenLists(1);
/* PURIFY 3.0a on SunOS4 reports a 2000 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
	glNewList(gp->gear3, GL_COMPILE);
	if (!wire)
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
	else
		glColor4fv(blue);
	gear(1.3, 2.0, 0.5, 10, 0.7, wire);
	glEndList();
	if (!wire)
		glEnable(GL_NORMALIZE);
}

void
init_gears(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         screen = MI_SCREEN(mi);

	/*Colormap    cmap; */
	/* Boolean     rgba, doublebuffer, cmap_installed; */
	gearsstruct *gp;

	if (gears == NULL) {
		if ((gears = (gearsstruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (gearsstruct))) == NULL)
			return;
	}
	gp = &gears[screen];

#if 0
	gp->win = window;
#endif
	gp->view_rotx = NRAND(360);
	gp->view_roty = NRAND(360);
	gp->view_rotz = NRAND(360);
	gp->angle = NRAND(360);
	gp->mono = (MI_WIN_IS_MONO(mi) ? 1 : 0);

	if (gp->glx_context) {
		glXDestroyContext(display, gp->glx_context);
		gp->glx_context = NULL;
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
		if (!getVisual(mi, wantVis, gp->mono)) {
			if (!getVisual(mi, wantVis, !gp->mono)) {
				(void) fprintf(stderr, "GL can not render with root visual\n");
				return;
			}
		}
		if (MI_WIN_IS_VERBOSE(mi))
			showVisualInfo(wantVis);
/* PURIFY 3.0a on SunOS4 reports a 104 byte memory leak on the next line each
 * time that gears mode is run in random mode. */
		gp->glx_context = glXCreateContext(display, wantVis, 0, GL_TRUE);
    XFree((char *) wantVis);
	}

	glXMakeCurrent(display, window, gp->glx_context);
	reshape(MI_WIN_WIDTH(mi), MI_WIN_HEIGHT(mi));
	if (gp->mono) {
		glIndexi(WhitePixel(display, screen));
		glClearIndex(BlackPixel(display, screen));
	}
	pinit(mi);
}

void
draw_gears(ModeInfo * mi)
{
	gearsstruct *gp = &gears[MI_SCREEN(mi)];
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	glXMakeCurrent(display, window, gp->glx_context);
	draw(mi);

	/* let's do something so we don't get bored */
	gp->angle += MI_CYCLES(mi) ? MI_CYCLES(mi) : 2;

	gp->view_rotx += MI_BATCHCOUNT(mi) ? MI_BATCHCOUNT(mi) : 1;
	gp->view_roty += MI_BATCHCOUNT(mi) ? MI_BATCHCOUNT(mi) : 1;
	gp->view_rotz += MI_BATCHCOUNT(mi) ? MI_BATCHCOUNT(mi) : 1;

	glFinish();
	glXSwapBuffers(display, window);
}

void
release_gears(ModeInfo * mi)
{
	if (gears != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			gearsstruct *gp = &gears[screen];

			if (gp->glx_context)
				glXDestroyContext(MI_DISPLAY(mi), gp->glx_context);
			if (glIsList(gp->gear1))
				glDeleteLists(gp->gear1, 1);
			if (glIsList(gp->gear2))
				glDeleteLists(gp->gear2, 1);
			if (glIsList(gp->gear3))
				glDeleteLists(gp->gear3, 1);
#if 0
#ifdef GLX_MESA_release_buffers
			glXReleaseBuffersMESA(MI_DISPLAY(mi), gp->win);
#endif
#endif
		}
		(void) free((void *) gears);
		gears = NULL;
	}
}


/*********************************************************/

#endif
