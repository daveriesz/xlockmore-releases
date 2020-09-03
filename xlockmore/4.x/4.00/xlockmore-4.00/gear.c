
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)gear.c	4.00 97/01/01 xlockmore";

#endif

/*-
 * gear.c - 3D gear wheels
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 1996: "written" by Danny Sung <dannys@ucla.edu>
 *       Based on 3-D gear wheels by Brian Paul which is in the public domain.
 *
 * Brian Paul
 */

/*-
   It actually took almost no work on my part to port to xlock.
   The gear code was taken from Mesa's example programs gears.
   Obviously you will need MesaGL (or OpenGL) to get this to work.
   It may not really worth running on a 486-66.
 */

#include "xlock.h"

#ifdef USE_GL
#include <stdarg.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <GL/xmesa.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/GLwDrawA.h>
/* #include "gltk.h" */

ModeSpecOpt gear_opts =
{0, NULL, 0, NULL, NULL};

typedef struct {
	GLfloat     view_rotx, view_roty, view_rotz;
	GLint       gear1, gear2, gear3;
	GLfloat     angle;
	GLuint      limit;
	GLuint      count;
	GLXContext  glx_context;
} gearstruct;

static gearstruct *gears = NULL;


#if 0
static GLint Black, Red, Green, Blue;
static XtAppContext AppContext;
static Display *display;
static Widget mesa;

static GLint alloc_color(Widget w, Colormap cmap, int red, int green, int blue);
static void translate_pixels(Widget to, Widget from,...);

#endif


/* 
 * Draw a gear wheel.  You'll probably want to call this function when
 * building a display list since we do a lot of trig here.
 *
 * Input:  inner_radius - radius of hole at center
 *         outer_radius - radius at center of teeth
 *         width - width of gear
 *         teeth - number of teeth
 *         tooth_depth - depth of tooth
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
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

	glNormal3f(0.0, 0.0, 1.0);

	/* draw front face */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
	}
	glEnd();

	/* draw front sides of teeth */
	glBegin(GL_QUADS);
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
	}
	glEnd();


	glNormal3f(0.0, 0.0, -1.0);

	/* draw back face */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
	}
	glEnd();

	/* draw back sides of teeth */
	glBegin(GL_QUADS);
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
	}
	glEnd();


	/* draw outward faces of teeth */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		u = r2 * cos(angle + da) - r1 * cos(angle);
		v = r2 * sin(angle + da) - r1 * sin(angle);
		len = sqrt(u * u + v * v);
		u /= len;
		v /= len;
		glNormal3f(v, -u, 0.0);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da), -width * 0.5);
		u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
		v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
		glNormal3f(v, -u, 0.0);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da), -width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
	}

	glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
	glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

	glEnd();


	glShadeModel(GL_SMOOTH);

	/* draw inside radius cylinder */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glNormal3f(-cos(angle), -sin(angle), 0.0);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	}
	glEnd();

}

static void
draw(ModeInfo * mi)
{
	gearstruct *gp = &gears[MI_SCREEN(mi)];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	/*tkSwapBuffers(); */

	gp->count++;
	if (gp->count == gp->limit) {
		exit(0);
	}
}



/* static void idle( void ) { angle += 2.0; } */



#if 0
/* change view angle, exit upon ESC */
static      GLenum
key(int k, GLenum mask)
{
	switch (k) {
		case TK_UP:
			gp->view_rotx += 5.0;
			return GL_TRUE;
		case TK_DOWN:
			gp->view_rotx -= 5.0;
			return GL_TRUE;
		case TK_LEFT:
			gp->view_roty += 5.0;
			return
				GL_TRUE;
		case TK_RIGHT:
			gp->view_roty -= 5.0;
			return GL_TRUE;
		case TK_z:
			gp->view_rotz += 5.0;
			return GL_TRUE;
		case TK_Z:
			gp->view_rotz -= 5.0;
			return
				GL_TRUE;
		case TK_ESCAPE:
			tkQuit();
	}
	return GL_FALSE;
}
#endif

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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}


static void
pinit(ModeInfo * mi)
{
	gearstruct *gp = &gears[MI_SCREEN(mi)];
	static GLfloat pos[4] =
	{5.0, 5.0, 10.0, 1.0};
	static GLfloat red[4] =
	{0.8, 0.1, 0.0, 1.0};
	static GLfloat green[4] =
	{0.0, 0.8, 0.2, 1.0};
	static GLfloat blue[4] =
	{0.2, 0.2, 1.0, 1.0};

	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	/* make the gears */
	if (gp->gear1) {
		glDeleteLists(gp->gear1, 1);
		gp->gear1 = None;
	}
	gp->gear1 = glGenLists(1);
	glNewList(gp->gear1, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
	gear(1.0, 4.0, 1.0, 20, 0.7);
	glEndList();

	if (gp->gear2) {
		glDeleteLists(gp->gear2, 1);
		gp->gear2 = None;
	}
	gp->gear2 = glGenLists(1);
	glNewList(gp->gear2, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
	gear(0.5, 2.0, 2.0, 10, 0.7);
	glEndList();

	if (gp->gear3) {
		glDeleteLists(gp->gear3, 1);
		gp->gear3 = None;
	}
	gp->gear3 = glGenLists(1);
	glNewList(gp->gear3, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
	gear(1.3, 2.0, 0.5, 10, 0.7);
	glEndList();
}


void
init_gear(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         screen = MI_SCREEN(mi);
	XVisualInfo *vi;

	/*Colormap    cmap; */
	/* Boolean     rgba, doublebuffer, cmap_installed; */
	int         attribList[] =
	{GLX_RGBA, GLX_DOUBLEBUFFER, None};
	gearstruct *gp;

	if (gears == NULL) {
		if ((gears = (gearstruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (gearstruct))) == NULL)
			return;
	}
	gp = &gears[MI_SCREEN(mi)];

	gp->view_rotx = 20.0, gp->view_roty = 30.0, gp->view_rotz = 0.0;
	gp->angle = 0;
	gp->count = 1;
	gp->limit = 0;

	if (gp->glx_context) {
		glXDestroyContext(display, gp->glx_context);
		gp->glx_context = NULL;
	}
	vi = glXChooseVisual(display, screen, attribList);
	gp->glx_context = glXCreateContext(display, vi, 0, GL_FALSE);

	/*cmap = XCreateColormap(display, RootWindow(display, screen), vi->visual,
	   AllocNone); */
	/*XMapWindow(display, window); */
	glXMakeCurrent(display, window, gp->glx_context);

	reshape(MI_WIN_WIDTH(mi), MI_WIN_HEIGHT(mi));
	pinit(mi);
}

void
draw_gear(ModeInfo * mi)
{
	gearstruct *gp = &gears[MI_SCREEN(mi)];

	draw(mi);

	/* let's do something so we don't get bored */
	gp->angle += MI_CYCLES(mi) ? MI_CYCLES(mi) : 2;

	gp->view_rotx += 1.0;
	gp->view_roty += 1.0;
	gp->view_rotz += 1.0;

	glFinish();
	glXSwapBuffers(MI_DISPLAY(mi), MI_WINDOW(mi));
}

void
release_gear(ModeInfo * mi)
{
	if (gears != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			gearstruct *gp = &gears[screen];

			if (gp->glx_context)
				glXDestroyContext(MI_DISPLAY(mi), gp->glx_context);
			if (gp->gear1)
				glDeleteLists(gp->gear1, 1);
			if (gp->gear2)
				glDeleteLists(gp->gear2, 1);
			if (gp->gear3)
				glDeleteLists(gp->gear3, 1);
		}
		(void) free((void *) gears);
		gears = NULL;
	}
}


/*********************************************************/

#endif
