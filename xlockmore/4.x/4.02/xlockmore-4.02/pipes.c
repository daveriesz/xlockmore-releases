#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)pipes.c   	4.02 97/04/01 xlockmore";

#endif

/*-
 * pipes.c - Shows 3D selfbuiding pipe system (xlock Version)
 *
 * This program was inspired on a WindowsNT(R)'s screen saver. It was written 
 * from scratch and it was not based on any other source code.
 *
 * ==========================================================================
 * The routine myElbow is derivated from the doughnut routine from the MesaGL
 * library (more especifically the Mesaaux library) written by Brian Paul.
 * ==========================================================================
 *
 * Thanks goes to Brian Paul for making it possible and inexpensive to use
 * OpenGL at home.
 *
 * Since I'm not a native english speaker, my apologies for any gramatical
 * mistake.
 *
 * My e-mail addresses are
 *
 * vianna@cat.cbpf.br 
 *         and
 * marcelo@venus.rdc.puc-rio.br
 *
 * Marcelo F. Vianna (Apr-09-1997)
 */

#include "xlock.h"

#ifdef USE_GL

#include <X11/Intrinsic.h>
#include <GL/xmesa.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

ModeSpecOpt pipes_opts =
{0, NULL, 0, NULL, NULL};

#define Scale4Window               0.1
#define Scale4Iconic               0.07

#define one_third                  0.3333333333333333333

#define dirNone -1
#define dirUP 0
#define dirDOWN 1
#define dirLEFT 2
#define dirRIGHT 3
#define dirNEAR 4
#define dirFAR 5

#define HCELLS 33
#define VCELLS 25
#define DEFINEDCOLORS 8
#define elbowradius 0.5

/*************************************************************************/

typedef struct {
	GLint       WindH, WindW;
	int         Cells[HCELLS][VCELLS][HCELLS];
	int         usedcolors[DEFINEDCOLORS];
	int         directions[6];
	int         ndirections;
	int         flip;
	int         nowdir, olddir;
	int         system_number;
	int         counter;
	int         PX, PY, PZ;
	int         number_of_systems;
	int         system_type;
	int         system_length;
	float      *MaterialColor;
	GLXContext  glx_context;
} pipesstruct;


static float front_shininess[] =
{60.0};
static float front_specular[] =
{0.7, 0.7, 0.7, 1.0};
static float ambient0[] =
{0.4, 0.4, 0.4, 1.0};
static float diffuse0[] =
{1.0, 1.0, 1.0, 1.0};
static float ambient1[] =
{0.2, 0.2, 0.2, 1.0};
static float diffuse1[] =
{0.5, 0.5, 0.5, 1.0};
static float position0[] =
{1.0, 1.0, 1.0, 0.0};
static float position1[] =
{-1.0, -1.0, 1.0, 0.0};
static float lmodel_ambient[] =
{0.5, 0.5, 0.5, 1.0};
static float lmodel_twoside[] =
{GL_TRUE};

static float MaterialRed[] =
{0.7, 0.0, 0.0, 1.0};
static float MaterialGreen[] =
{0.1, 0.5, 0.2, 1.0};
static float MaterialBlue[] =
{0.0, 0.0, 0.7, 1.0};
static float MaterialCyan[] =
{0.2, 0.5, 0.7, 1.0};
static float MaterialYellow[] =
{0.7, 0.7, 0.0, 1.0};
static float MaterialMagenta[] =
{0.6, 0.2, 0.5, 1.0};
static float MaterialWhite[] =
{0.7, 0.7, 0.7, 1.0};
static float MaterialGray[] =
{0.2, 0.2, 0.2, 1.0};

static pipesstruct *pipes = NULL;


static void
MakeTube(int direction)
{
	float       an;

	switch (direction) {
		case dirUP:
		case dirDOWN:
			glRotatef(90.0, 1.0, 0.0, 0.0);
			break;
		case dirLEFT:
		case dirRIGHT:
			glRotatef(90.0, 0.0, 1.0, 0.0);
			break;
	}

	glBegin(GL_QUAD_STRIP);
	for (an = 0.0; an <= 2.0 * M_PI; an += M_PI / 12.0) {
		glNormal3f(cos(an) / 3.0, sin(an) / 3.0, 0.0);
		glVertex3f(cos(an) / 3.0, sin(an) / 3.0, one_third);

		glNormal3f(cos(an) / 3.0, sin(an) / 3.0, 0.0);
		glVertex3f(cos(an) / 3.0, sin(an) / 3.0, -one_third);
	}
/* PURIFY 3.0a on SunOS4 reports a memory leak on the next line */
	glEnd();
}

static void
mySphere(float radius)
{
	GLUquadricObj *quadObj;

	quadObj = gluNewQuadric();
/* Sun CC reports a warning: enum type mismatch: arg #2 on the next line */
	gluQuadricDrawStyle(quadObj, GLU_FILL);
/* PURIFY 3.0a on SunOS4 reports a memory leak on the next line */
	gluSphere(quadObj, radius, 16, 16);
	gluDeleteQuadric(quadObj);
}

static void
myElbow()
{
#define nsides 25
#define rings 25
#define r one_third
#define R one_third

	int         i, j;
	GLdouble    p0[03], p1[3], p2[3], p3[3];
	GLdouble    n0[3], n1[3], n2[3], n3[3];

	for (i = 0; i <= rings / 4; i++) {
		GLdouble    theta, theta1;
		theta = (GLdouble) i *2.0 * M_PI / rings;

		theta1 = (GLdouble) (i + 1) * 2.0 * M_PI / rings;
		for (j = 0; j < nsides; j++) {
			GLdouble    phi, phi1;
			phi = (GLdouble) j *2.0 * M_PI / nsides;

			phi1 = (GLdouble) (j + 1) * 2.0 * M_PI / nsides;

			p0[0] = cos(theta) * (R + r * cos(phi));
			p0[1] = -sin(theta) * (R + r * cos(phi));
			p0[2] = r * sin(phi);

			p1[0] = cos(theta1) * (R + r * cos(phi));
			p1[1] = -sin(theta1) * (R + r * cos(phi));
			p1[2] = r * sin(phi);

			p2[0] = cos(theta1) * (R + r * cos(phi1));
			p2[1] = -sin(theta1) * (R + r * cos(phi1));
			p2[2] = r * sin(phi1);

			p3[0] = cos(theta) * (R + r * cos(phi1));
			p3[1] = -sin(theta) * (R + r * cos(phi1));
			p3[2] = r * sin(phi1);

			n0[0] = cos(theta) * (cos(phi));
			n0[1] = -sin(theta) * (cos(phi));
			n0[2] = sin(phi);

			n1[0] = cos(theta1) * (cos(phi));
			n1[1] = -sin(theta1) * (cos(phi));
			n1[2] = sin(phi);

			n2[0] = cos(theta1) * (cos(phi1));
			n2[1] = -sin(theta1) * (cos(phi1));
			n2[2] = sin(phi1);

			n3[0] = cos(theta) * (cos(phi1));
			n3[1] = -sin(theta) * (cos(phi1));
			n3[2] = sin(phi1);

			glBegin(GL_QUADS);
			glNormal3dv(n3);
			glVertex3dv(p3);
			glNormal3dv(n2);
			glVertex3dv(p2);
			glNormal3dv(n1);
			glVertex3dv(p1);
			glNormal3dv(n0);
			glVertex3dv(p0);
/* PURIFY 3.0a on SunOS4 reports a memory leak on the next line */
			glEnd();
		}
	}

#undef r
#undef R
#undef nsides
#undef rings
}

static void
FindNeighbors(ModeInfo * mi)
{
	pipesstruct *pp = &pipes[MI_SCREEN(mi)];

	pp->ndirections = 0;
	pp->directions[dirUP] = (!pp->Cells[pp->PX][pp->PY + 1][pp->PZ]) ? 1 : 0;
	pp->ndirections += pp->directions[dirUP];
	pp->directions[dirDOWN] = (!pp->Cells[pp->PX][pp->PY - 1][pp->PZ]) ? 1 : 0;
	pp->ndirections += pp->directions[dirDOWN];
	pp->directions[dirLEFT] = (!pp->Cells[pp->PX - 1][pp->PY][pp->PZ]) ? 1 : 0;
	pp->ndirections += pp->directions[dirLEFT];
	pp->directions[dirRIGHT] = (!pp->Cells[pp->PX + 1][pp->PY][pp->PZ]) ? 1 : 0;
	pp->ndirections += pp->directions[dirRIGHT];
	pp->directions[dirFAR] = (!pp->Cells[pp->PX][pp->PY][pp->PZ - 1]) ? 1 : 0;
	pp->ndirections += pp->directions[dirFAR];
	pp->directions[dirNEAR] = (!pp->Cells[pp->PX][pp->PY][pp->PZ + 1]) ? 1 : 0;
	pp->ndirections += pp->directions[dirNEAR];
}

static int
SelectNeighbor(ModeInfo * mi)
{
	pipesstruct *pp = &pipes[MI_SCREEN(mi)];
	int         dirlist[6];
	int         i, j;

	for (i = 0, j = 0; i < 6; i++) {
		if (pp->directions[i]) {
			dirlist[j] = i;
			j++;
		}
	}

	i = NRAND(pp->ndirections);
	return dirlist[i];
}

static void pinit(ModeInfo * mi, int zera);

void
draw_pipes(ModeInfo * mi)
{
	pipesstruct *pp = &pipes[MI_SCREEN(mi)];

	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

	int         newdir;
	int         OPX, OPY, OPZ;

	glXMakeCurrent(display, window, pp->glx_context);

	if (MI_WIN_IS_INROOT(mi)) {
		glDrawBuffer(GL_BACK);
	} else {
		glDrawBuffer(GL_FRONT);
	}

	glPushMatrix();

	glTranslatef(0.0, 0.0, -10.0);
	if (!MI_WIN_IS_ICONIC(mi)) {
		glScalef(Scale4Window * pp->WindH / pp->WindW, Scale4Window, Scale4Window);
	} else {
		glScalef(Scale4Iconic * pp->WindH / pp->WindW, Scale4Iconic, Scale4Iconic);
	}

	FindNeighbors(mi);

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pp->MaterialColor);

	/* If it's the begining of a system, draw a sphere */
	if (pp->olddir == dirNone) {
		glPushMatrix();
		glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0);
		mySphere(0.6);
		glPopMatrix();
	}
	/* Check for stop conditions */
	if (pp->ndirections == 0 || pp->counter > pp->system_length) {
		glPushMatrix();
		glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0);
		/* Finish the system with another sphere */
		mySphere(0.6);
		if (MI_WIN_IS_INROOT(mi))
			glXSwapBuffers(display, window);
		glPopMatrix();

		/* If the maximum number of system was drawn, restart (clearing the screen), */
		/* else start a new system. */
		if (++pp->system_number > pp->number_of_systems) {
			sleep(1);
			pinit(mi, 1);
		} else {
			pinit(mi, 0);
		}

		glPopMatrix();
		return;
	}
	pp->counter++;

	/* Do will the direction change? if so, determine the new one */
	newdir = pp->nowdir;
	if (!pp->directions[newdir]) {	/* cannot proceed in the current direction */
		newdir = SelectNeighbor(mi);
	} else {
		if ((pp->counter > 1) && (NRAND(100) < 20)) {	/* random change (20% chance) */
			newdir = SelectNeighbor(mi);
		}
	}

	/* Have the direction changed? */
	if (newdir == pp->nowdir) {
		/* If not, draw the cell's center pipe */
		glPushMatrix();
		glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0);
		MakeTube(newdir);
		glPopMatrix();
	} else {
		/* If so, draw the cell's center elbow/sphere */
		glPushMatrix();
		if (pp->system_type == 1 || (pp->system_type == 3 && (pp->system_number & 1))) {
			glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0);
			mySphere(elbowradius);
		} else {
			switch (pp->nowdir) {
				case dirUP:
					switch (newdir) {
						case dirLEFT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							break;
						case dirRIGHT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirFAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							glRotatef(180.0, 0.0, 0.0, 1.0);
							break;
						case dirNEAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							break;
					}
					break;
				case dirDOWN:
					switch (newdir) {
						case dirLEFT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							break;
						case dirRIGHT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirFAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 0.0, 1.0, 0.0);
							break;
						case dirNEAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							break;
					}
					break;
				case dirLEFT:
					switch (newdir) {
						case dirUP:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirDOWN:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirFAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirNEAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 0.0, 1.0);
							break;
					}
					break;
				case dirRIGHT:
					switch (newdir) {
						case dirUP:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							break;
						case dirDOWN:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							break;
						case dirFAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							break;
						case dirNEAR:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 1.0, 0.0, 0.0);
							break;
					}
					break;
				case dirNEAR:
					switch (newdir) {
						case dirLEFT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							break;
						case dirRIGHT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 1.0, 0.0);
							break;
						case dirUP:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(270.0, 0.0, 1.0, 0.0);
							break;
						case dirDOWN:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0 - (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							glRotatef(180.0, 0.0, 0.0, 1.0);
							break;
					}
					break;
				case dirFAR:
					switch (newdir) {
						case dirUP:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 + (one_third), (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							break;
						case dirDOWN:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0, (pp->PY - 12) / 3.0 * 4.0 - (one_third), (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 0.0, 1.0, 0.0);
							glRotatef(180.0, 1.0, 0.0, 0.0);
							break;
						case dirLEFT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 - (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(90.0, 1.0, 0.0, 0.0);
							break;
						case dirRIGHT:
							glTranslatef((pp->PX - 16) / 3.0 * 4.0 + (one_third), (pp->PY - 12) / 3.0 * 4.0, (pp->PZ - 16) / 3.0 * 4.0 + (one_third));
							glRotatef(270.0, 1.0, 0.0, 0.0);
							glRotatef(180.0, 0.0, 0.0, 1.0);
							break;
					}
					break;
			}
			myElbow();
		}
		glPopMatrix();
	}

	OPX = pp->PX;
	OPY = pp->PY;
	OPZ = pp->PZ;
	pp->olddir = pp->nowdir;
	pp->nowdir = newdir;
	switch (pp->nowdir) {
		case dirUP:
			pp->PY++;
			break;
		case dirDOWN:
			pp->PY--;
			break;
		case dirLEFT:
			pp->PX--;
			break;
		case dirRIGHT:
			pp->PX++;
			break;
		case dirNEAR:
			pp->PZ++;
			break;
		case dirFAR:
			pp->PZ--;
			break;
	}
	pp->Cells[pp->PX][pp->PY][pp->PZ] = 1;

	/* Cells'face pipe */
	glTranslatef(((pp->PX + OPX) / 2.0 - 16) / 3.0 * 4.0, ((pp->PY + OPY) / 2.0 - 12) / 3.0 * 4.0, ((pp->PZ + OPZ) / 2.0 - 16) / 3.0 * 4.0);
	MakeTube(newdir);

	glPopMatrix();

	glFlush();

	pp->flip = !pp->flip;
	if (pp->flip) {
		if (MI_WIN_IS_INROOT(mi))
			glXSwapBuffers(display, window);
	}
	/* This is needed since other modes that needs to draw in backbuffer */
	/* may be unexpectedly switched in by the random mode. Better than */
	/* changing them all... */
	glDrawBuffer(GL_BACK);
}

static void
reshape(ModeInfo * mi, int width, int height)
{
	pipesstruct *pp = &pipes[MI_SCREEN(mi)];

	glViewport(0, 0, pp->WindW = (GLint) width, pp->WindH = (GLint) height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 15.0);
	glMatrixMode(GL_MODELVIEW);
}

static void
pinit(ModeInfo * mi, int zera)
{
	pipesstruct *pp = &pipes[MI_SCREEN(mi)];
	int         X, Y, Z;

	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glColor3f(1.0, 1.0, 1.0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse1);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	glShadeModel(GL_SMOOTH);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_specular);

	if (zera) {
		pp->system_number = 1;
		glDrawBuffer(GL_FRONT_AND_BACK);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		memset(pp->Cells, 0, sizeof (pp->Cells));
		for (X = 0; X < HCELLS; X++) {
			for (Y = 0; Y < VCELLS; Y++) {
				pp->Cells[X][Y][0] = 1;
				pp->Cells[X][Y][HCELLS - 1] = 1;
				pp->Cells[0][Y][X] = 1;
				pp->Cells[HCELLS - 1][Y][X] = 1;
			}
		}
		for (X = 0; X < HCELLS; X++) {
			for (Z = 0; Z < HCELLS; Z++) {
				pp->Cells[X][0][Z] = 1;
				pp->Cells[X][VCELLS - 1][Z] = 1;
			}
		}
		memset(pp->usedcolors, 0, sizeof (pp->usedcolors));
	}
	pp->counter = 0;

	if (!MI_WIN_IS_MONO(mi)) {
		int         collist[DEFINEDCOLORS];
		int         i, j, lower = 1000;

		/* Avoid repeating colors on the same screen unless necessary */
		for (i = 0; i < DEFINEDCOLORS; i++) {
			if (lower > pp->usedcolors[i])
				lower = pp->usedcolors[i];
		}
		for (i = 0, j = 0; i < DEFINEDCOLORS; i++) {
			if (pp->usedcolors[i] == lower) {
				collist[j] = i;
				j++;
			}
		}
		i = collist[NRAND(j)];
		pp->usedcolors[i]++;
		switch (i) {
			case 0:
				pp->MaterialColor = MaterialRed;
				break;
			case 1:
				pp->MaterialColor = MaterialGreen;
				break;
			case 2:
				pp->MaterialColor = MaterialBlue;
				break;
			case 3:
				pp->MaterialColor = MaterialCyan;
				break;
			case 4:
				pp->MaterialColor = MaterialYellow;
				break;
			case 5:
				pp->MaterialColor = MaterialMagenta;
				break;
			case 6:
				pp->MaterialColor = MaterialGray;
				break;
			case 7:
				pp->MaterialColor = MaterialWhite;
				break;
		}
	} else {
		pp->MaterialColor = MaterialGray;
	}

	do {
		pp->PX = NRAND((HCELLS - 1)) + 1;
		pp->PY = NRAND((VCELLS - 1)) + 1;
		pp->PZ = NRAND((HCELLS - 1)) + 1;
	} while (pp->Cells[pp->PX][pp->PY][pp->PZ] ||
		 (pp->Cells[pp->PX + 1][pp->PY][pp->PZ] && pp->Cells[pp->PX - 1][pp->PY][pp->PZ] &&
		  pp->Cells[pp->PX][pp->PY + 1][pp->PZ] && pp->Cells[pp->PX][pp->PY - 1][pp->PZ] &&
		  pp->Cells[pp->PX][pp->PY][pp->PZ + 1] && pp->Cells[pp->PX][pp->PY][pp->PZ - 1]));
	pp->Cells[pp->PX][pp->PY][pp->PZ] = 1;
	pp->olddir = dirNone;

	FindNeighbors(mi);

	pp->nowdir = SelectNeighbor(mi);
}

void
init_pipes(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	int         screen = MI_SCREEN(mi);
	int         mono = (MI_WIN_IS_MONO(mi) ? 1 : 0);
	pipesstruct *pp;

	if (pipes == NULL) {
		if ((pipes = (pipesstruct *) calloc(MI_NUM_SCREENS(mi),
					      sizeof (pipesstruct))) == NULL)
			return;
	}
	pp = &pipes[screen];


	if (pp->glx_context) {
		glXDestroyContext(display, pp->glx_context);
		pp->glx_context = NULL;
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
		if (!getVisual(mi, wantVis, mono)) {
			if (!getVisual(mi, wantVis, !mono)) {
				(void) fprintf(stderr, "GL can not render with root visual\n");
				return;
			}
		}
		if (MI_WIN_IS_VERBOSE(mi))
			showVisualInfo(wantVis);
/* PURIFY 3.0a on SunOS4 reports a 104 byte memory leak on the next line each 
 * time that pipes mode is run in random mode. */ 
		pp->glx_context = glXCreateContext(display, wantVis, 0, True);
		XFree((char *) wantVis);
	}

	glXMakeCurrent(display, window, pp->glx_context);

	if (mono) {
		glIndexi(WhitePixel(display, screen));
		glClearIndex(BlackPixel(display, screen));
	}
	reshape(mi, MI_WIN_WIDTH(mi), MI_WIN_HEIGHT(mi));
	pinit(mi, 1);

	if (MI_BATCHCOUNT(mi) < 1 || MI_BATCHCOUNT(mi) > 3) {
		pp->system_type = NRAND(3) + 1;
	} else {
		pp->system_type = MI_BATCHCOUNT(mi);
	}

	if (MI_CYCLES(mi) > 0 && MI_CYCLES(mi) < 11) {
		pp->number_of_systems = MI_CYCLES(mi);
	} else {
		pp->number_of_systems = 5;
	}

	if (MI_SIZE(mi) < 10) {
		pp->system_length = 10;
	} else if (MI_SIZE(mi) > 1000) {
		pp->system_length = 1000;
	} else {
		pp->system_length = MI_SIZE(mi);
	}

}

void
change_pipes(ModeInfo * mi)
{
	pinit(mi, 1);
}

void
release_pipes(ModeInfo * mi)
{
	if (pipes != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			pipesstruct *pp = &pipes[screen];

			if (pp->glx_context)
				glXDestroyContext(MI_DISPLAY(mi), pp->glx_context);
#if 0 /* This is wrong for multiscreens anyway */
#ifdef GLX_MESA_release_buffers
			glXReleaseBuffersMESA(MI_DISPLAY(mi), MI_WINDOW(mi));
#endif
#endif
		}
		(void) free((void *) pipes);
		pipes = NULL;
	}
}

#endif
