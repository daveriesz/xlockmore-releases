/* -*- Mode: C; tab-width: 4 -*- */
/* pyraminx --- Shows an auto-solving Pyraminx */

#if 0
static const char sccsid[] = "@(#)pyraminx.c	5.72 2023/08/19 xlockmore";
#endif

#undef DEBUG_LISTS

/*-
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * This mode shows an auto-solving Pyraminx tetrahedron.
 *
 * Thanks goes also to Brian Paul for making it possible and inexpensive
 * to use OpenGL at home.
 *
 * Based on rubik.c by Marcelo F. Vianna
 *
 * Revision History:
 * 02-Feb-2024: Started writing, using xlock dino.c and PyraminxGL.c from
		xpuzzles.  Available are sizes 2-4 for period 3 turning.
 */


/* Blue Red Yellow Green */
/*-
 * Color labels mapping:
 * =====================
 *
 * 	        +
 *	       /|\
 *  	      / | \
 *           /  |  \
 *          /   |   \
 *         /(0) | (1)\
 *        /BLUE | RED \
 *       /  TOP | RIGHT\
 *      /       |     3 \
 *     /2     --+--    1 \
 *    /0 1 3-- (2) ---2 0 \
 *   /  ---  YELLOW   ---  \
 *  /2--      LEFT       -- \
 * /0 1 3                    \
 * +--------------------------+
 * Flip side: BOTTOM (3) GREEN
 */


#ifdef VMS
#include <types.h>
#endif

#ifdef STANDALONE
# define MODE_pyraminx
# define DEFAULTS	"*delay: 100000 \n" \
			"*count: -30 \n" \
			"*cycles: 5 \n" \

# define free_pyraminx 0
# include "xlockmore.h"		/* from the xscreensaver distribution */
# include "gltrackball.h"

#else /* !STANDALONE */
#include "xlock.h"		/* from the xlockmore distribution */
#include "visgl.h"
#endif /* !STANDALONE */

#ifdef MODE_pyraminx
#define glTranslate(x,y,z) glTranslatef((float) x, (float) y, (float) z)

#include <GL/gl.h>
#define DEF_HIDESHUFFLING     "False"
#define DEF_STICKY     "False"
#define DEF_PERIOD2     "False"

static Bool hideshuffling;
static Bool sticky;
/*static Bool period2;*/

static XrmOptionDescRec opts[] =
{
	{(char *) "-hideshuffling", (char *) ".pyraminx.hideshuffling", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+hideshuffling", (char *) ".pyraminx.hideshuffling", XrmoptionNoArg, (caddr_t) "off"},
	{(char *) "-sticky", (char *) ".pyraminx.sicky", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+sticky", (char *) ".pyraminx.sticky", XrmoptionNoArg, (caddr_t) "off"}
	/*{(char *) "-period2", (char *) ".pyraminx.period2", XrmoptionNoArg, (caddr_t) "on"},
	{(char *) "+period2", (char *) ".pyraminx.period2", XrmoptionNoArg, (caddr_t) "off"}*/
};

static argtype vars[] =
{
	{(void *) & hideshuffling, (char *) "hideshuffling", (char *) "Hideshuffling", (char *) DEF_HIDESHUFFLING, t_Bool},
	{(void *) & sticky, (char *) "sticky", (char *) "Sticky", (char *) DEF_STICKY, t_Bool}
	/*{(void *) & period2, (char *) "period2", (char *) "Period2", (char *) DEF_PERIOD2, t_Bool}*/
};

static OptionStruct desc[] =
{
	{(char *) "-/+hideshuffling", (char *) "turn on/off hidden shuffle phase"},
	{(char *) "-/+sticky", (char *) "turn on/off sticky mode"}
	/*{(char *) "-/+period2", (char *) "turn on/off period 2 turns"}*/
};

ENTRYPOINT ModeSpecOpt pyraminx_opts =
{sizeof opts / sizeof opts[0], opts, sizeof vars / sizeof vars[0], vars, desc};

#ifdef USE_MODULES
ModStruct   pyraminx_description =
{"pyraminx", "init_pyraminx", "draw_pyraminx", "release_pyraminx",
 "draw_pyraminx", "change_pyraminx", "(char *) NULL", &pyraminx_opts,
 100000, -30, 5, -4, 64, 1.0, "",
 "Shows an auto-solving Pyraminx puzzle", 0, NULL};

#endif

#define ACTION_SOLVE    1
#define ACTION_SHUFFLE  0

#define DELAY_AFTER_SHUFFLING  5
#define DELAY_AFTER_SOLVING   20

/*************************************************************************/

#define MINSIZE 2

#define Scale4Window	       (0.9/3.0)
#define Scale4Iconic	       (2.1/3.0)

/* Directions relative to the face of a piece */
#define TOP 0
#define TR 1
#define RIGHT 2
#define BOTTOM 3
#define BL 4
#define LEFT 5
#define COORD 6
#define CW 7
#define BR 9
#define TL 10
#define CCW 11

#define PERIOD2 2
#define PERIOD3 3
#define BOTH 4

#define DOWN 0
#define UP 1
#define MAX_FACES 4
#define MAX_VIEWS 2
#define MAX_SIDES (MAX_FACES/MAX_VIEWS)
#define MAX_ORIENT (3*MAX_SIDES)
#define MAX_ROTATE 3

#define TOP_FACE 0
#define RIGHT_FACE 1
#define LEFT_FACE 2
#define BOTTOM_FACE 3
#define NO_FACE (MAX_FACES+1)
#define NO_POSITION (-1)
#define NO_ROTATION (2*MAX_ORIENT)

#define FACET_LENGTH (float) 0.5
#define SELECT_LENGTH2 (float) 0.5
#define STICKER_LENGTH2 (float) 0.485

#define FACET_TETRA (FACET_LENGTH-(float) 0.02)
#define STICKER_TETRA2 (FACET_TETRA-(float) 0.04)
#define STICKER_TETRA3 (STICKER_TETRA2-(float) 0.04)
#define SELECT_TETRA2 (STICKER_TETRA2-(float) 0.04)
#define SELECT_TETRA3 (STICKER_TETRA3-(float) 0.06)

#define BAR_WIDTH (FACET_LENGTH*(float) 0.05)
#define GAP_WIDTH (FACET_LENGTH*(float) 0.15)
#define GAP_WIDTH2 (FACET_LENGTH*(float) 0.2)

#define FACET_OCTA (FACET_LENGTH*(float) 1.90)
#define STICKER_OCTA2 (FACET_LENGTH*(float) 1.85)
#define SELECT_OCTA2 (FACET_LENGTH*(float) 1.81)

#define STICKER_LONG (FACET_TETRA-(float) 0.05)
#define STICKER_SHORT (STICKER_LONG-(float) 0.05)
#define STICKER_HALF ((STICKER_SHORT)*(float) 0.5)
#define STICKER_DEPTH (FACET_LENGTH+(float) 0.01)
#define SELECT_LONG (float) (4.5*FACET_TETRA/5.0-0.05)
#define SELECT_SHORT (float) (3.5*STICKER_LONG/5.0-0.05)
#define CUT (float) 0.04
#define CUT2 (float) 0.02
#define CUT_DEPTH (STICKER_DEPTH+(float) 0.001)
#define SELECT_DEPTH (STICKER_DEPTH+(float) 0.002)

#define ObjCubit	0
#define MaxObj	  1

typedef struct _PyraminxLoc {
	int	 face;
	int	 rotation;  /* Not used yet */
} PyraminxLoc;

typedef struct _PyraminxLocPos {
	int	 face, position, direction;
} PyraminxLocPos;

typedef struct _PyraminxMove {
	int	 face, direction;
	int	 position;
	int	 style;
	int	 control; /* Not used yet */
} PyraminxMove;

typedef struct {
#ifdef WIN32
	HGLRC       glx_context;
#else
	GLXContext *glx_context;
#endif
	GLint       WindH, WindW;
	GLfloat     step;
	PyraminxMove  *moves;
	int	 storedmoves;
	int      degreeTurn;
	int	 shufflingmoves;
	int      size;
	int	 action;
	int	 done;
	GLfloat     anglestep;
	PyraminxLoc *facetLoc[MAX_FACES];
	PyraminxLoc *faceLoc[MAX_SIDES];
	PyraminxLoc *rowLoc[3][MAX_SIDES];
	PyraminxMove   movement;
	GLfloat     rotateStep;
	GLfloat     PX, PY, VX, VY;
	Bool	AreObjectsDefined[2];
	Bool    period2, sticky;
#ifdef STANDALONE
	Bool button_down_p;
	trackball_state *trackball;
#endif
} pyraminxstruct;

static float front_shininess[] =
{60.0};
static float front_specular[] =
{0.7, 0.7, 0.7, 1.0};
static float ambient[] =
{0.0, 0.0, 0.0, 1.0};
static float diffuse[] =
{1.0, 1.0, 1.0, 1.0};
static float position0[] =
{1.0, 1.0, 1.0, 0.0};
static float position1[] =
{-1.0, -1.0, 1.0, 0.0};
static float lmodel_ambient[] =
{0.5, 0.5, 0.5, 1.0};
static float lmodel_twoside[] =
{GL_TRUE};

static float MaterialRed[] =
{0.5, 0.0, 0.0, 1.0};
static float MaterialBlue[] =
{0.0, 0.0, 0.5, 1.0};
static float MaterialGreen[] =
{0.0, 0.5, 0.0, 1.0};
static float MaterialYellow[] =
{0.7, 0.7, 0.0, 1.0};

static float MaterialGray[] =
{0.2, 0.2, 0.2, 1.0};
static float MaterialGray3[] =
{0.3, 0.3, 0.3, 1.0};
static float MaterialGray4[] =
{0.4, 0.4, 0.4, 1.0};
static float MaterialGray5[] =
{0.5, 0.5, 0.5, 1.0};
static float MaterialGray7[] =
{0.7, 0.7, 0.7, 1.0};

static pyraminxstruct *pyraminx = (pyraminxstruct *) NULL;

static void
pickColor(int c, int mono)
{
	switch (c) {
		case TOP_FACE:
			if (mono)
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray3);
			else
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialBlue);
			break;
		case RIGHT_FACE:
			if (mono)
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray4);
			else
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialRed);
			break;
		case LEFT_FACE:
			if (mono)
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray5);
			else
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialYellow);
			break;
		case BOTTOM_FACE:
			if (mono)
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray7);
			else
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGreen);
			break;
	}
}

static void
drawStickerlessTetra(void)
{
	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray);
	glBegin(GL_TRIANGLES);
	/* OPP GREEN FRONT */
	glNormal3f(-1.0, 1.0, 1.0);
	glVertex3f(-FACET_TETRA, FACET_LENGTH, FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_LENGTH, FACET_TETRA);
	glVertex3f(-FACET_LENGTH, FACET_TETRA, FACET_LENGTH);
	/* OPP RED LEFT */
	glNormal3f(-1.0, -1.0, -1.0);
	glVertex3f(-FACET_LENGTH, -FACET_TETRA, -FACET_LENGTH);
	glVertex3f(-FACET_TETRA, -FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, -FACET_LENGTH, -FACET_TETRA);
	/* OPP YELLOW TOP */
	glNormal3f(1.0, 1.0, -1.0);
	glVertex3f(FACET_TETRA, FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(FACET_LENGTH, FACET_LENGTH, -FACET_TETRA);
	glVertex3f(FACET_LENGTH, FACET_TETRA, -FACET_LENGTH);
	/* OPP BLUE RIGHT */
	glNormal3f(1.0, -1.0, 1.0);
	glVertex3f(FACET_LENGTH, -FACET_TETRA, FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_LENGTH, FACET_TETRA);
	glVertex3f(FACET_TETRA, -FACET_LENGTH, FACET_LENGTH);

	/* TOP_FACE */
	glNormal3f(-1.0, 1.0, -1.0);
	glVertex3f(-FACET_LENGTH, -FACET_TETRA, -FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_LENGTH, FACET_TETRA);
	glVertex3f(FACET_TETRA, FACET_LENGTH, -FACET_LENGTH);
	/* RIGHT_FACE */
	glNormal3f(1.0, 1.0, 1.0);
	glVertex3f(FACET_LENGTH, FACET_LENGTH, -FACET_TETRA);
	glVertex3f(-FACET_TETRA, FACET_LENGTH, FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_TETRA, FACET_LENGTH);
	/* LEFT_FACE */
	glNormal3f(-1.0, -1.0, 1.0);
	glVertex3f(FACET_TETRA, -FACET_LENGTH, FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_TETRA, FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, -FACET_LENGTH, -FACET_TETRA);
	/* BOTTOM_FACE */
	glNormal3f(1.0, -1.0, -1.0);
	glVertex3f(FACET_LENGTH, FACET_TETRA, -FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_LENGTH, FACET_TETRA);
	glVertex3f(-FACET_TETRA, -FACET_LENGTH, -FACET_LENGTH);
	glEnd();

	glBegin(GL_QUADS);
	/* LEFT BAR */
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-FACET_LENGTH, -FACET_TETRA, -FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, -FACET_LENGTH, -FACET_TETRA);
	glVertex3f(-FACET_LENGTH, FACET_TETRA, FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_LENGTH, FACET_TETRA);
	/* TOP BAR */
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(FACET_LENGTH, FACET_LENGTH, -FACET_TETRA);
	glVertex3f(FACET_TETRA, FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_LENGTH, FACET_TETRA);
	glVertex3f(-FACET_TETRA, FACET_LENGTH, FACET_LENGTH);
	/* RIGHT BAR */
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(FACET_TETRA, -FACET_LENGTH, FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_TETRA, FACET_LENGTH);
	glVertex3f(-FACET_TETRA, FACET_LENGTH, FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, FACET_TETRA, FACET_LENGTH);
	/* FAR LEFT BAR */
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(-FACET_TETRA, -FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(-FACET_LENGTH, -FACET_TETRA, -FACET_LENGTH);
	glVertex3f(FACET_TETRA, FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(FACET_LENGTH, FACET_TETRA, -FACET_LENGTH);
	/* FAR RIGHT BAR */
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(FACET_LENGTH, FACET_TETRA, -FACET_LENGTH);
	glVertex3f(FACET_LENGTH, FACET_LENGTH, -FACET_TETRA);
	glVertex3f(FACET_LENGTH, -FACET_TETRA, FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_LENGTH, FACET_TETRA);
	/* BOTTOM BAR */
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(-FACET_LENGTH, -FACET_LENGTH, -FACET_TETRA);
	glVertex3f(-FACET_TETRA, -FACET_LENGTH, -FACET_LENGTH);
	glVertex3f(FACET_LENGTH, -FACET_LENGTH, FACET_TETRA);
	glVertex3f(FACET_TETRA, -FACET_LENGTH, FACET_LENGTH);
	glEnd();
	glPopMatrix();
}

static void
drawStickerlessOcta(void)
{
	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialGray);
	glBegin(GL_QUADS);
	glNormal3f(-1.0, 1.0, 0.0);
	glVertex3f(-FACET_OCTA, CUT, BAR_WIDTH); /* LEFT BAR */
	glVertex3f(-CUT, FACET_OCTA, BAR_WIDTH);
	glVertex3f(-CUT, FACET_OCTA, -BAR_WIDTH);
	glVertex3f(-FACET_OCTA, CUT, -BAR_WIDTH);
	glNormal3f(0.0, 1.0, -1.0);
	glVertex3f(BAR_WIDTH, CUT, -FACET_OCTA); /* UPPER LEFT BAR */
	glVertex3f(-BAR_WIDTH, CUT, -FACET_OCTA);
	glVertex3f(-BAR_WIDTH, FACET_OCTA, -CUT);
	glVertex3f(BAR_WIDTH, FACET_OCTA, -CUT);
	glNormal3f(0.0, 1.0, 1.0);
	glVertex3f(-BAR_WIDTH, CUT, FACET_OCTA); /* RIGHT BAR */
	glVertex3f(BAR_WIDTH, CUT, FACET_OCTA);
	glVertex3f(BAR_WIDTH, FACET_OCTA, CUT);
	glVertex3f(-BAR_WIDTH, FACET_OCTA, CUT);
	glNormal3f(1.0, 1.0, 0.0);
	glVertex3f(CUT, FACET_OCTA, BAR_WIDTH); /* UPPER RIGHT BAR */
	glVertex3f(FACET_OCTA, CUT, BAR_WIDTH);
	glVertex3f(FACET_OCTA, CUT, -BAR_WIDTH);
	glVertex3f(CUT, FACET_OCTA, -BAR_WIDTH);
	glNormal3f(-1.0, 0.0, 1.0);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, CUT); /* BOTTOM BAR */
	glVertex3f(-CUT, -BAR_WIDTH, FACET_OCTA);
	glVertex3f(-CUT, BAR_WIDTH, FACET_OCTA);
	glVertex3f(-FACET_OCTA, BAR_WIDTH, CUT);
	glNormal3f(1.0, 0.0, -1.0);
	glVertex3f(FACET_OCTA, BAR_WIDTH, -CUT); /* BACK BOTTOM BAR */
	glVertex3f(FACET_OCTA, -BAR_WIDTH, -CUT);
	glVertex3f(CUT, -BAR_WIDTH, -FACET_OCTA);
	glVertex3f(CUT, BAR_WIDTH, -FACET_OCTA);
	glNormal3f(-1.0, -1.0, 0.0);
	glVertex3f(-CUT, -FACET_OCTA, -BAR_WIDTH); /* BACK UPPER LEFT BAR */
	glVertex3f(-CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -CUT, -BAR_WIDTH);
	glNormal3f(0.0, -1.0, -1.0);
	glVertex3f(BAR_WIDTH, -CUT, -FACET_OCTA); /* LEFT BAR */
	glVertex3f(BAR_WIDTH, -FACET_OCTA, -CUT);
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, -CUT);
	glVertex3f(-BAR_WIDTH, -CUT, -FACET_OCTA);
	glNormal3f(0.0, -1.0, 1.0);
	glVertex3f(-BAR_WIDTH, -CUT, FACET_OCTA); /* BACK UPPER RIGHT BAR */
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(BAR_WIDTH, -CUT, FACET_OCTA);
	glNormal3f(1.0, -1.0, 0.0);
	glVertex3f(FACET_OCTA, -CUT, -BAR_WIDTH); /* BACK RIGHT BAR */
	glVertex3f(FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(CUT, -FACET_OCTA, -BAR_WIDTH);
	glNormal3f(-1.0, 0.0, -1.0);
	glVertex3f(-FACET_OCTA, BAR_WIDTH, -CUT); /* FAR LEFT BAR */
	glVertex3f(-CUT, BAR_WIDTH, -FACET_OCTA);
	glVertex3f(-CUT, -BAR_WIDTH, -FACET_OCTA);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, -CUT);
	glNormal3f(1.0, 0.0, 1.0);
	glVertex3f(FACET_OCTA, -BAR_WIDTH, CUT); /* FAR RIGHT BAR */
	glVertex3f(FACET_OCTA, BAR_WIDTH, CUT);
	glVertex3f(CUT, BAR_WIDTH, FACET_OCTA);
	glVertex3f(CUT, -BAR_WIDTH, FACET_OCTA);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, -CUT); /* LEFT */
	glVertex3f(-FACET_OCTA, -CUT, -BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, CUT);
	glVertex3f(-FACET_OCTA, BAR_WIDTH, CUT);
	glVertex3f(-FACET_OCTA, CUT, BAR_WIDTH);
	glVertex3f(-FACET_OCTA, CUT, -BAR_WIDTH);
	glVertex3f(-FACET_OCTA, BAR_WIDTH, -CUT);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(FACET_OCTA, -BAR_WIDTH, -CUT); /* BACK RIGHT */
	glVertex3f(FACET_OCTA, BAR_WIDTH, -CUT);
	glVertex3f(FACET_OCTA, CUT, -BAR_WIDTH);
	glVertex3f(FACET_OCTA, CUT, BAR_WIDTH);
	glVertex3f(FACET_OCTA, BAR_WIDTH, CUT);
	glVertex3f(FACET_OCTA, -BAR_WIDTH, CUT);
	glVertex3f(FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(FACET_OCTA, -CUT, -BAR_WIDTH);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-BAR_WIDTH, FACET_OCTA, -CUT); /* TOP */
	glVertex3f(-CUT, FACET_OCTA, -BAR_WIDTH);
	glVertex3f(-CUT, FACET_OCTA, BAR_WIDTH);
	glVertex3f(-BAR_WIDTH, FACET_OCTA, CUT);
	glVertex3f(BAR_WIDTH, FACET_OCTA, CUT);
	glVertex3f(CUT, FACET_OCTA, BAR_WIDTH);
	glVertex3f(CUT, FACET_OCTA, -BAR_WIDTH);
	glVertex3f(BAR_WIDTH, FACET_OCTA, -CUT);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, -CUT); /* BOTTOM */
	glVertex3f(BAR_WIDTH, -FACET_OCTA, -CUT);
	glVertex3f(CUT, -FACET_OCTA, -BAR_WIDTH);
	glVertex3f(CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(-CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(-CUT, -FACET_OCTA, -BAR_WIDTH);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(-BAR_WIDTH, -CUT, FACET_OCTA); /* RIGHT */
	glVertex3f(BAR_WIDTH, -CUT, FACET_OCTA);
	glVertex3f(CUT, -BAR_WIDTH, FACET_OCTA);
	glVertex3f(CUT, BAR_WIDTH, FACET_OCTA);
	glVertex3f(BAR_WIDTH, CUT, FACET_OCTA);
	glVertex3f(-BAR_WIDTH, CUT, FACET_OCTA);
	glVertex3f(-CUT, BAR_WIDTH, FACET_OCTA);
	glVertex3f(-CUT, -BAR_WIDTH, FACET_OCTA);
	glEnd();
	glBegin(GL_POLYGON);
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(-BAR_WIDTH, -CUT, -FACET_OCTA); /* BACK LEFT */
	glVertex3f(-CUT, -BAR_WIDTH, -FACET_OCTA);
	glVertex3f(-CUT, BAR_WIDTH, -FACET_OCTA);
	glVertex3f(-BAR_WIDTH, CUT, -FACET_OCTA);
	glVertex3f(BAR_WIDTH, CUT, -FACET_OCTA);
	glVertex3f(CUT, BAR_WIDTH, -FACET_OCTA);
	glVertex3f(CUT, -BAR_WIDTH, -FACET_OCTA);
	glVertex3f(BAR_WIDTH, -CUT, -FACET_OCTA);
	glEnd();
	/* BOTTOM LEFT */
	glBegin(GL_POLYGON);
	glNormal3f(-1.0, -1.0, -1.0);
	glVertex3f(-CUT, -BAR_WIDTH, -FACET_OCTA);
	glVertex3f(-BAR_WIDTH, -CUT, -FACET_OCTA);
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, -CUT);
	glVertex3f(-CUT, -FACET_OCTA, -BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -CUT, -BAR_WIDTH);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, -CUT);
	glEnd();
	/* BOTTOM */
	glBegin(GL_POLYGON);
	glNormal3f(-1.0, -1.0, 1.0);
	glVertex3f(-FACET_OCTA, -BAR_WIDTH, CUT);
	glVertex3f(-FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(-CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(-BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(-BAR_WIDTH, -CUT, FACET_OCTA);
	glVertex3f(-CUT, -BAR_WIDTH, FACET_OCTA);
	glEnd();
	/* TOP LEFT */
	glBegin(GL_POLYGON);
	glNormal3f(-1.0, 1.0, -1.0);
	glVertex3f(-FACET_OCTA, BAR_WIDTH, -CUT); /* LEFT BOTTOM */
	glVertex3f(-FACET_OCTA, CUT, -BAR_WIDTH);
	glVertex3f(-CUT, FACET_OCTA, -BAR_WIDTH); /* TOP */
	glVertex3f(-BAR_WIDTH, FACET_OCTA, -CUT);
	glVertex3f(-BAR_WIDTH, CUT, -FACET_OCTA); /* FAR LEFT */
	glVertex3f(-CUT, BAR_WIDTH, -FACET_OCTA);
	glEnd();
	/* FRONT */
	glBegin(GL_POLYGON);
	glNormal3f(-1.0, 1.0, 1.0);
	glVertex3f(-CUT, BAR_WIDTH, FACET_OCTA); /* RIGHT */
	glVertex3f(-BAR_WIDTH, CUT, FACET_OCTA);
	glVertex3f(-BAR_WIDTH, FACET_OCTA, CUT); /* TOP */
	glVertex3f(-CUT, FACET_OCTA, BAR_WIDTH);
	glVertex3f(-FACET_OCTA, CUT, BAR_WIDTH); /* LEFT */
	glVertex3f(-FACET_OCTA, BAR_WIDTH, CUT);
	glEnd();
	/* BACK */
	glBegin(GL_POLYGON);
	glNormal3f(1.0, -1.0, -1.0);
	glVertex3f(FACET_OCTA, -BAR_WIDTH, -CUT);
	glVertex3f(FACET_OCTA, -CUT, -BAR_WIDTH);
	glVertex3f(CUT, -FACET_OCTA, -BAR_WIDTH);
	glVertex3f(BAR_WIDTH, -FACET_OCTA, -CUT);
	glVertex3f(BAR_WIDTH, -CUT, -FACET_OCTA);
	glVertex3f(CUT, -BAR_WIDTH, -FACET_OCTA);
	glEnd();
	/* BOTTOM RIGHT */
	glBegin(GL_POLYGON);
	glNormal3f(1.0, -1.0, 1.0);
	glVertex3f(CUT, -BAR_WIDTH, FACET_OCTA);
	glVertex3f(BAR_WIDTH, -CUT, FACET_OCTA);
	glVertex3f(BAR_WIDTH, -FACET_OCTA, CUT);
	glVertex3f(CUT, -FACET_OCTA, BAR_WIDTH);
	glVertex3f(FACET_OCTA, -CUT, BAR_WIDTH);
	glVertex3f(FACET_OCTA, -BAR_WIDTH, CUT);
	glEnd();
	/* TOP BEHIND */
	glBegin(GL_POLYGON);
	glNormal3f(1.0, 1.0, -1.0);
	glVertex3f(CUT, BAR_WIDTH, -FACET_OCTA);
	glVertex3f(BAR_WIDTH, CUT, -FACET_OCTA);
	glVertex3f(BAR_WIDTH, FACET_OCTA, -CUT);
	glVertex3f(CUT, FACET_OCTA, -BAR_WIDTH);
	glVertex3f(FACET_OCTA, CUT, -BAR_WIDTH);
	glVertex3f(FACET_OCTA, BAR_WIDTH, -CUT);
	glEnd();
	/* TOP RIGHT */
	glBegin(GL_POLYGON);
	glNormal3f(1.0, 1.0, 1.0);
	glVertex3f(FACET_OCTA, BAR_WIDTH, CUT);
	glVertex3f(FACET_OCTA, CUT, BAR_WIDTH);
	glVertex3f(CUT, FACET_OCTA, BAR_WIDTH);
	glVertex3f(BAR_WIDTH, FACET_OCTA, CUT);
	glVertex3f(BAR_WIDTH, CUT, FACET_OCTA);
	glVertex3f(CUT, BAR_WIDTH, FACET_OCTA);
	glEnd();
	glPopMatrix();
}

/* This helps to Map sizes for the piece faces */
static int truncUnit[MAX_FACES][6][3] =
{
	{ /* 0 Blue */
		{-2, 3, -1},
		{-1, 3, -2},
		{-1, 2, -3},
		{-2, 1, -3},
		{-3, 1, -2},
		{-3, 2, -1}
	}, { /* 1 Red */
		{1, 2, 3},
		{2, 1, 3},
		{3, 1, 2},
		{3, 2, 1},
		{2, 3, 1},
		{1, 3, 2}
	}, { /* 2 Yellow */
		{-3, -1, 2},
		{-3, -2, 1},
		{-2, -3, 1},
		{-1, -3, 2},
		{-1, -2, 3},
		{-2, -1, 3}
	}, { /* 3 Green */
		{1, -2, -3},
		{2, -1, -3},
		{3, -1, -2},
		{3, -2, -1},
		{2, -3, -1},
		{1, -3, -2}
	}
};

static float stickerCoord[6][3];

static void setStickerCoord(int face, float a, float b, float c)
{
	int i, j, value;
	for (i = 0; i < 6; i++) {
		for (j = 0; j < 3; j++) {
			value = truncUnit[face][i][j];
			switch(value) {
			case -1:
				stickerCoord[i][j] = -a;
				break;
			case 1:
				stickerCoord[i][j] = a;
				break;
			case -2:
				stickerCoord[i][j] = -b;
				break;
			case 2:
				stickerCoord[i][j] = b;
				break;
			case -3:
				stickerCoord[i][j] = -c;
				break;
			case 3:
				stickerCoord[i][j] = c;
				break;
			}
		}
	}
}

static void
drawSticker(ModeInfo * mi, int face, int position, Bool tetra)
{
	int mono = MI_IS_MONO(mi);
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];
	int i;

	glBegin(GL_POLYGON);
	pickColor(pp->facetLoc[face][position].face, mono);
	if (tetra)
		setStickerCoord(face, -STICKER_TETRA3, STICKER_TETRA2, STICKER_LENGTH2);
	else
		setStickerCoord(face, CUT, GAP_WIDTH, STICKER_OCTA2);
	for (i = 0; i < 6; i++) {
		glVertex3f(stickerCoord[i][0], stickerCoord[i][1], stickerCoord[i][2]);
	}
	glEnd();
}

static void
drawTetraFacet(ModeInfo * mi, int top, int right, int left, int bottom)
{
	drawStickerlessTetra();
	glPushMatrix();
	/* TOP LEFT */
	if (top != NO_POSITION) { /* BLUE */
		glNormal3f(-1.0, 1.0, -1.0);
		drawSticker(mi, TOP_FACE, top, True);
	}
	/* TOP RIGHT */
	if (right != NO_POSITION) { /* RED */
		glNormal3f(1.0, 1.0, 1.0);
		drawSticker(mi, RIGHT_FACE, right, True);
	}
	/* BOTTOM */
	if (left != NO_POSITION) { /* YELLOW */
		glNormal3f(-1.0, -1.0, 1.0);
		drawSticker(mi, LEFT_FACE, left, True);
	}
	/* BACK */
	if (bottom != NO_POSITION) { /* GREEN */
		glNormal3f(1.0, -1.0, -1.0);
		drawSticker(mi, BOTTOM_FACE, bottom, True);
	}
	glPopMatrix();
}

/* compatible with tetra drawing */
static void
drawOctaFacet(ModeInfo * mi, int top, int right, int left, int bottom)
{
	drawStickerlessOcta();
	glPushMatrix();
	/* TOP LEFT */
	if (top != NO_POSITION) { /* BLUE */
		glNormal3f(-1.0, 1.0, -1.0);
		drawSticker(mi, TOP_FACE, top, False);
	}
	/* TOP RIGHT */
	if (right != NO_POSITION) { /* RED */
		glNormal3f(1.0, 1.0, 1.0);
		drawSticker(mi, RIGHT_FACE, right, False);
	}
	/* BOTTOM */
	if (left != NO_POSITION) { /* YELLOW */
		glNormal3f(-1.0, -1.0, 1.0);
		drawSticker(mi, LEFT_FACE, left, False);
	}
	/* BACK */
	if (bottom != NO_POSITION) { /* GREEN */
		glNormal3f(1.0, -1.0, -1.0);
		drawSticker(mi, BOTTOM_FACE, bottom, False);
	}
	glPopMatrix();
}

static void
drawFacet(ModeInfo * mi, int top, int right, int left, int bottom,
		Bool tetra)
{
	if (tetra)
		drawTetraFacet(mi, top, right, left, bottom);
	else
		drawOctaFacet(mi, top, right, left, bottom);
}

typedef struct _CRD {
	int column, row, diagonal;
} CRD;

/* This is fast for small i, a -1 is returned for negative i. */
static int
SQRT(int i)
{
	int j = 0;

	while (j * j <= i)
		j++;
	return (j - 1);
}

static void
toCRD(pyraminxstruct * pp, int face, int position, CRD * crd)
{
	int diag, diag2;

	diag = SQRT(position);
	diag2 = diag * diag;
	if ((face & 1) == UP) {
		crd->column = pp->size - 1 - (position - diag2) / 2;
		crd->row = pp->size - 1 - (diag2 + 2 * diag - position) / 2;
		crd->diagonal = 2 * pp->size - 1 - diag;
	} else {
		crd->column = (position - diag2) / 2;
		crd->row = (diag2 + 2 * diag - position) / 2;
		crd->diagonal = diag;
	}
}

typedef struct _FaceDepth {
	 int face, depth;
} FaceDepth;

static void
faceDepth(pyraminxstruct * pp, int face, int position, int direction, FaceDepth *fd)
{
	CRD crd;

	toCRD(pp, face, position, &crd);
	if (direction == TR || direction == BL) {
		if (face % 2 == 1)
			fd->face = face - 1;
		else
			fd->face = face + 1;
		if (crd.diagonal >= pp->size)
			fd->depth = 2 * pp->size - 1 - crd.diagonal;
		else
			fd->depth = crd.diagonal;
	} else if (direction == TOP || direction == BOTTOM) {
		fd->face = (face + 2) % 4;
		if (face % 2 == 1)
			fd->depth = crd.column;
		else
			fd->depth = pp->size - 1 - crd.column;
	} else /* (direction == LEFT || direction == RIGHT) */ {
		fd->face = MAX_FACES - 1 - face;
		if (face % 2 == 1)
			fd->depth = crd.row;
		else
			fd->depth = pp->size - 1 - crd.row;
	}
}

static void
drawFaces(ModeInfo * mi, int face, int position, int direction,
		Bool use, Bool all)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];
	int s1, s2;
	int i, j, k, lc, rc, corner;
	int size = pp->size;
	int dir = direction % 3;
	Bool tetra;
	CRD crd;
	FaceDepth fd;

	faceDepth(pp, face, position, direction, &fd);
	toCRD(pp, face, position, &crd);
	lc = (size - 1) * (size - 1);
	rc = size * size - 1;
	switch (dir) {
	case 0: /* row */
		corner = rc;
		break;
	case 1: /* diagonal */
		corner = 0;
		break;
	case 2: /* column */
		corner = lc;
		break;
	default:
		corner = 0;
		break;
	}
	if (size == 1) {
		drawTetraFacet(mi, 0, 0, 0, 0);
	} else if (size == 2) {
		if (all || (use && corner != position)
				|| (!use && corner == position))
			drawOctaFacet(mi, 2, 2, 2, 2);
	} else if (pp->sticky) {
		int s3;
		int a, b, c, d, e, f;
		/* CORNERS */
		s1 = size - 1;
		j = lc;
		k = rc;
		if (all || (use && fd.face == 0 && fd.depth != size - 1)
				|| (use && fd.face != 0 && fd.depth == size - 1)
				|| (!use && fd.face != 0 && fd.depth != size - 1)
				|| (!use && fd.face == 0 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, -s1 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawTetraFacet(mi, -1, 0, k, j); /* RIGHT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth != size - 1)
				|| (use && fd.face != 1 && fd.depth == size - 1)
				|| (!use && fd.face != 1 && fd.depth != size - 1)
				|| (!use && fd.face == 1 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, -s1 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawTetraFacet(mi, 0, -1, j, k); /* LEFT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth != size - 1)
				|| (use && fd.face != 2 && fd.depth == size - 1)
				|| (!use && fd.face != 2 && fd.depth != size - 1)
				|| (!use && fd.face == 2 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, s1 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawTetraFacet(mi, k, j, -1, 0); /* TOP */
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth != size - 1)
				|| (use && fd.face != 3 && fd.depth == size - 1)
				|| (!use && fd.face != 3 && fd.depth != size - 1)
				|| (!use && fd.face == 3 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, s1 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawTetraFacet(mi, j, k, 0, -1); /* FRONT */
			glPopMatrix();
		}
		/* CORNERS - 1 */
		s2 = size - 2;
		j = (size - 1) * (size - 1) + 1;
		k = size * size - 2;
		if (all || (use && fd.face == 0 && fd.depth != size - 1)
				|| (use && fd.face != 0 && fd.depth == size - 1)
				|| (!use && fd.face != 0 && fd.depth != size - 1)
				|| (!use && fd.face == 0 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(s2 * FACET_LENGTH, -s2 * FACET_LENGTH, s2 * FACET_LENGTH);
			drawOctaFacet(mi, -1, 2, k, j); /* RIGHT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth != size - 1)
				|| (use && fd.face != 1 && fd.depth == size - 1)
				|| (!use && fd.face != 1 && fd.depth != size - 1)
				|| (!use && fd.face == 1 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(-s2 * FACET_LENGTH, -s2 * FACET_LENGTH, -s2 * FACET_LENGTH);
			drawOctaFacet(mi, 2, -1, j, k); /* LEFT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth != size - 1)
				|| (use && fd.face != 2 && fd.depth == size - 1)
				|| (!use && fd.face != 2 && fd.depth != size - 1)
				|| (!use && fd.face == 2 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(s2 * FACET_LENGTH, s2 * FACET_LENGTH, -s2 * FACET_LENGTH);
			drawOctaFacet(mi, k, j, -1, 2); /* TOP */
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth != size - 1)
				|| (use && fd.face != 3 && fd.depth == size - 1)
				|| (!use && fd.face != 3 && fd.depth != size - 1)
				|| (!use && fd.face == 3 && fd.depth == size - 1)) {
			glPushMatrix();
			glTranslate(-s2 * FACET_LENGTH, s2 * FACET_LENGTH, s2 * FACET_LENGTH);
			drawOctaFacet(mi, j, k, 2, -1); /* FRONT */
			glPopMatrix();
		}
		/* MIDDLE EDGES */ /* 1, 3, 6; 5, 7, 12; 4, 8, 20*/
		tetra = (size % 2 == 1);
		s1 = size - 1 - ((tetra) ? 0 : 1);
		i = (size / 2) * (size / 2) + ((tetra) ? 0: 1);
		j = (size / 2 + 1) * (size / 2 + 1) - 1 - ((tetra) ? 0 : 1);
		k = size * size - size;
		if (all || (use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, 0, s1 * FACET_LENGTH);
			drawFacet(mi, -1, j, j, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, s1 * FACET_LENGTH, 0);
			drawFacet(mi, k, k, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)
				|| (use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, 0, 0);
			drawFacet(mi, -1, i, -1, i, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, 0, -s1 * FACET_LENGTH);
			drawFacet(mi, j, -1, -1, j, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)
				|| (use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, -s1 * FACET_LENGTH, 0);
			drawFacet(mi, -1, -1, k, k, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, 0, 0);
			drawFacet(mi, i, -1, i, -1, tetra);
			glPopMatrix();
		}
		/* CENTER */
		tetra = (size % 3 == 1); /* 2, 6, 12, 20, 30 */
		if (tetra) {
			s1 = ((size - 1) / 3) * 2;
		} else {
			s1 = ((size - 1) / 3) * 2 + 1;
		}
		k = s1 * s1 + s1;
		i = s1 - 1 - ((tetra) ? 0 : 1);
		if (all || (use && fd.face == 0 && fd.depth == size - 1) 
				|| (use && fd.face != 0 && fd.depth != size - 1)
				|| (!use && fd.face != 0 && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-i * FACET_LENGTH, i * FACET_LENGTH, -i * FACET_LENGTH);
			drawFacet(mi, k, -1, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == size - 1) 
				|| (use && fd.face != 1 && fd.depth != size - 1)
				|| (!use && fd.face != 1 && fd.depth == size - 1)
				|| (!use && fd.face == 1 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(i * FACET_LENGTH, i * FACET_LENGTH, i * FACET_LENGTH);
			drawFacet(mi, -1, k, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == size - 1) 
				|| (use && fd.face != 2 && fd.depth != size - 1)
				|| (!use && fd.face != 2 && fd.depth == size - 1)
				|| (!use && fd.face == 2 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-i * FACET_LENGTH, -i * FACET_LENGTH, i * FACET_LENGTH);
			drawFacet(mi, -1, -1, k, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth == size - 1) 
				|| (use && fd.face != 3 && fd.depth != size - 1)
				|| (!use && fd.face != 3 && fd.depth == size - 1)
				|| (!use && fd.face == 3 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(i * FACET_LENGTH, -i * FACET_LENGTH, -i * FACET_LENGTH);
			drawFacet(mi, -1, -1, -1, k, tetra);
			glPopMatrix();
		}
		/* MIDDLE MIDDLE SIDES */
		tetra = (size % 2 == 0);
		s1 = size - 1;
		s3 = size - 3;
		a = 1;
		b = 3;
		c = 4;
		d = 8;
		e = 11;
		f = 13;
		if (all || (use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, -s3 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, a, -1, c, -1, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, s3 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, c, -1, a, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, -s3 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawFacet(mi, b, -1, -1, d, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, s3 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawFacet(mi, d, -1, -1, b, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, s1 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, e, f, -1, -1, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, s1 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, f, e, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)
				|| (use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, -s3 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, -1, a, -1, c, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, s3 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, -1, c, -1, a, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)
				|| (use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, s3 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawFacet(mi, -1, d, b, -1, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, -s3 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawFacet(mi, -1, b, d, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)
				|| (use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, -s1 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, -1, -1, f, e, tetra);
			glPopMatrix();
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, -s1 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, -1, -1, e, f, tetra);
			glPopMatrix();
		}
		return;
	}
	if (size > 1) { /* CORNERS */
		s1 = size - 1;
		j = lc;
		k = rc;
		if (all || (use && fd.face == 0 && fd.depth == 0)
				|| (use && fd.face != 0 && fd.depth == size - 1)
				|| (!use && fd.face != 0 && fd.depth != size - 1)
				|| (!use && fd.face == 0 && fd.depth != 0)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, -s1 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawTetraFacet(mi, -1, 0, k, j); /* RIGHT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == 0)
				|| (use && fd.face != 1 && fd.depth == size - 1)
				|| (!use && fd.face != 1 && fd.depth != size - 1)
				|| (!use && fd.face == 1 && fd.depth != 0)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, -s1 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawTetraFacet(mi, 0, -1, j, k); /* LEFT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == 0)
				|| (use && fd.face != 2 && fd.depth == size - 1)
				|| (!use && fd.face != 2 && fd.depth != size - 1)
				|| (!use && fd.face == 2 && fd.depth != 0)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, s1 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawTetraFacet(mi, k, j, -1, 0); /* TOP */
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth == 0)
				|| (use && fd.face != 3 && fd.depth == size - 1)
				|| (!use && fd.face != 3 && fd.depth != size - 1)
				|| (!use && fd.face == 3 && fd.depth != 0)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, s1 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawTetraFacet(mi, j, k, 0, -1); /* FRONT */
			glPopMatrix();
		}
	}
	if (size > 2) { /* CORNERS - 1 */
		s2 = size - 2;
		j = (size - 1) * (size - 1) + 1;
		k = size * size - 2;
		if (all || (use && fd.face == 0 && fd.depth == 1)
				|| (use && fd.face != 0 && fd.depth == size - 1)
				|| (!use && fd.face != 0 && fd.depth != size - 1)
				|| (!use && fd.face == 0 && fd.depth != 1)) {
			glPushMatrix();
			glTranslate(s2 * FACET_LENGTH, -s2 * FACET_LENGTH, s2 * FACET_LENGTH);
			drawOctaFacet(mi, -1, 2, k, j); /* RIGHT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == 1)
				|| (use && fd.face != 1 && fd.depth == size - 1)
				|| (!use && fd.face != 1 && fd.depth != size - 1)
				|| (!use && fd.face == 1 && fd.depth != 1)) {
			glPushMatrix();
			glTranslate(-s2 * FACET_LENGTH, -s2 * FACET_LENGTH, -s2 * FACET_LENGTH);
			drawOctaFacet(mi, 2, -1, j, k); /* LEFT */
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == 1)
				|| (use && fd.face != 2 && fd.depth == size - 1)
				|| (!use && fd.face != 2 && fd.depth != size - 1)
				|| (!use && fd.face == 2 && fd.depth != 1)) {
			glPushMatrix();
			glTranslate(s2 * FACET_LENGTH, s2 * FACET_LENGTH, -s2 * FACET_LENGTH);
			drawOctaFacet(mi, k, j, -1, 2); /* TOP */
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth == 1)
				|| (use && fd.face != 3 && fd.depth == size - 1)
				|| (!use && fd.face != 3 && fd.depth != size - 1)
				|| (!use && fd.face == 3 && fd.depth != 1)) {
			glPushMatrix();
			glTranslate(-s2 * FACET_LENGTH, s2 * FACET_LENGTH, s2 * FACET_LENGTH);
			drawOctaFacet(mi, j, k, 2, -1); /* FRONT */
			glPopMatrix();
		}
	}
	if (size > 2) { /* MIDDLE SIDE */ /* 1, 3, 6; 5, 7, 12; 4, 8, 20*/
		int middle = size / 2;
		tetra = (size % 2 == 1);
		s1 = size - 1 - ((tetra) ? 0 : 1);
		i = (size / 2) * (size / 2) + ((tetra) ? 0: 1);
		j = (size / 2 + 1) * (size / 2 + 1) - 1 - ((tetra) ? 0 : 1);
		k = size * size - size;
		if (all || (use && (fd.face == 0 || fd.face == 3) && fd.depth == middle)
				|| (use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != middle)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, 0, s1 * FACET_LENGTH);
			drawFacet(mi, -1, j, j, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 2 || fd.face == 3) && fd.depth == middle)
				|| (use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != middle)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, s1 * FACET_LENGTH, 0);
			drawFacet(mi, k, k, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 2) && fd.depth == middle)
				|| (use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != middle)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, 0, 0);
			drawFacet(mi, -1, i, -1, i, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 1 || fd.face == 2) && fd.depth == middle)
				|| (use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != middle)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, 0, -s1 * FACET_LENGTH);
			drawFacet(mi, j, -1, -1, j, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 0 || fd.face == 1) && fd.depth == middle)
				|| (use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != middle)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(0, -s1 * FACET_LENGTH, 0);
			drawFacet(mi, -1, -1, k, k, tetra);
			glPopMatrix();
		}
		if (all || (use && (fd.face == 1 || fd.face == 3) && fd.depth == middle)
				|| (use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != middle)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, 0, 0);
			drawFacet(mi, i, -1, i, -1, tetra);
			glPopMatrix();
		}
	}
	if (size > 3 && size % 3 != 0) { /* CENTER */
		tetra = (size % 3 == 1); /* 2, 6, 12, 20, 30 */
		if (tetra) {
			s1 = ((size - 1) / 3) * 2;
		} else {
			s1 = ((size - 1) / 3) * 2 + 1;
		}
		k = s1 * s1 + s1;
		i = s1 - 1 - ((tetra) ? 0 : 1);
		if (all || (use && fd.face == 0 && fd.depth == size - 1) 
				|| (use && fd.face != 0 && fd.depth == s1)
				|| (!use && fd.face != 0 && fd.depth != s1)
				|| (!use && fd.face == 0 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-i * FACET_LENGTH, i * FACET_LENGTH, -i * FACET_LENGTH);
			drawFacet(mi, k, -1, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == size - 1) 
				|| (use && fd.face != 1 && fd.depth == s1)
				|| (!use && fd.face != 1 && fd.depth != s1)
				|| (!use && fd.face == 1 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(i * FACET_LENGTH, i * FACET_LENGTH, i * FACET_LENGTH);
			drawFacet(mi, -1, k, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == size - 1) 
				|| (use && fd.face != 2 && fd.depth == s1)
				|| (!use && fd.face != 2 && fd.depth != s1)
				|| (!use && fd.face == 2 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-i * FACET_LENGTH, -i * FACET_LENGTH, i * FACET_LENGTH);
			drawFacet(mi, -1, -1, k, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 3 && fd.depth == size - 1) 
				|| (use && fd.face != 3 && fd.depth == s1)
				|| (!use && fd.face != 3 && fd.depth != s1)
				|| (!use && fd.face == 3 && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(i * FACET_LENGTH, -i * FACET_LENGTH, -i * FACET_LENGTH);
			drawFacet(mi, -1, -1, -1, k, tetra);
			glPopMatrix();
		}
	}
	if (size > 3 && size < 6) { /* MIDDLE MIDDLE SIDES */
		int s3, m1, m2;
		int a, b, c, d, e, f;
		tetra = (size % 3 == 1);
		s1 = size - 1 - ((tetra) ? 0 : 1);
		s3 = size - 3 - ((tetra) ? 0 : 1);
		tetra = (size % 2 == 0);
		/* Facet colors and depth hard coded for size 4 and 5 FIXME */
		a = (tetra) ? 1 : 5;
		b = (tetra) ? 3 : 7;
		c = (tetra) ? 4 : 10;
		d = (tetra) ? 8 : 14;
		e = (tetra) ? 11 : 19;
		f = (tetra) ? 13 : 21;
		m1 = (tetra) ? 1 : 2;
		m2 = (tetra) ? 2 : 3;
		if (all || (use && fd.face == 1 && fd.depth == m1)
				|| (use && fd.face == 3 && fd.depth == m2)
				|| (use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && fd.face == 1 && fd.depth != m1)
				|| (!use && fd.face == 3 && fd.depth != m2)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, -s3 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, a, -1, c, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == m1)
				|| (use && fd.face == 2 && fd.depth == m2)
				|| (use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 1 && fd.depth != m1)
				|| (!use && fd.face == 2 && fd.depth != m2)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, -s3 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawFacet(mi, b, -1, -1, d, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == m2)
				|| (use && fd.face == 3 && fd.depth == m1)
				|| (use && (fd.face == 0 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && fd.face == 1 && fd.depth != m2)
				|| (!use && fd.face == 3 && fd.depth != m1)
				|| (!use && (fd.face == 0 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s1 * FACET_LENGTH, s3 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, c, -1, a, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 1 && fd.depth == m2)
				|| (use && fd.face == 2 && fd.depth == m1)
				|| (use && (fd.face == 0 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 1 && fd.depth != m2)
				|| (!use && fd.face == 2 && fd.depth != m1)
				|| (!use && (fd.face == 0 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, s3 * FACET_LENGTH, -s1 * FACET_LENGTH);
			drawFacet(mi, d, -1, -1, b, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == m2)
				|| (use && fd.face == 3 && fd.depth == m1)
				|| (use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && fd.face == 2 && fd.depth != m2)
				|| (!use && fd.face == 3 && fd.depth != m1)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, s1 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, e, f, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 2 && fd.depth == m1)
				|| (use && fd.face == 3 && fd.depth == m2)
				|| (use && (fd.face == 0 || fd.face == 1) && fd.depth == size - 1)
				|| (!use && fd.face == 2 && fd.depth != m1)
				|| (!use && fd.face == 3 && fd.depth != m2)
				|| (!use && (fd.face == 0 || fd.face == 1) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, s1 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, f, e, -1, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m1)
				|| (use && fd.face == 2 && fd.depth == m2)
				|| (use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m1)
				|| (!use && fd.face == 2 && fd.depth != m2)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, -s3 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, -1, a, -1, c, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m2)
				|| (use && fd.face == 2 && fd.depth == m1)
				|| (use && (fd.face == 1 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m2)
				|| (!use && fd.face == 2 && fd.depth != m1)
				|| (!use && (fd.face == 1 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s1 * FACET_LENGTH, s3 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, -1, c, -1, a, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m2)
				|| (use && fd.face == 3 && fd.depth == m1)
				|| (use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m2)
				|| (!use && fd.face == 3 && fd.depth != m1)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, s3 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawFacet(mi, -1, d, b, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m1)
				|| (use && fd.face == 3 && fd.depth == m2)
				|| (use && (fd.face == 1 || fd.face == 2) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m1)
				|| (!use && fd.face == 3 && fd.depth != m2)
				|| (!use && (fd.face == 1 || fd.face == 2) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, -s3 * FACET_LENGTH, s1 * FACET_LENGTH);
			drawFacet(mi, -1, b, d, -1, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m1)
				|| (use && fd.face == 1 && fd.depth == m2)
				|| (use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m1)
				|| (!use && fd.face == 1 && fd.depth != m2)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(s3 * FACET_LENGTH, -s1 * FACET_LENGTH, s3 * FACET_LENGTH);
			drawFacet(mi, -1, -1, f, e, tetra);
			glPopMatrix();
		}
		if (all || (use && fd.face == 0 && fd.depth == m2)
				|| (use && fd.face == 1 && fd.depth == m1)
				|| (use && (fd.face == 2 || fd.face == 3) && fd.depth == size - 1)
				|| (!use && fd.face == 0 && fd.depth != m2)
				|| (!use && fd.face == 1 && fd.depth != m1)
				|| (!use && (fd.face == 2 || fd.face == 3) && fd.depth != size - 1)) {
			glPushMatrix();
			glTranslate(-s3 * FACET_LENGTH, -s1 * FACET_LENGTH, -s3 * FACET_LENGTH);
			drawFacet(mi, -1, -1, e, f, tetra);
			glPopMatrix();
		}
	}
}

/* From David Bagley's xpyraminx.  Used by permission. ;)  */
typedef struct _RowNextP3 {
	int viewChanged, face, direction, reverse;
} RowNextP3;
static PyraminxLoc slideNextRowP2[MAX_SIDES][3] =
{
	{
		{2, 0},
		{1, 3},
		{2, 3}
	},
	{
		{2, 0},
		{0, 3},
		{2, 3}
	}
};
static RowNextP3 slideNextRowP3[MAX_SIDES][MAX_ORIENT] =
{
	{
		{True, UP, TR, False},
		{True, UP, TOP, False},
		{False, UP, BOTTOM, True},
		{False, UP, RIGHT, True},
		{True, DOWN, RIGHT, True},
		{True, DOWN, TR, True}
	},
	{
		{False, DOWN, LEFT, True},
		{True, UP, LEFT, True},
		{True, UP, BL, True},
		{True, DOWN, BL, False},
		{True, DOWN, BOTTOM, False},
		{False, DOWN, TOP, True}
	}
};
static int rotOrientRowP3[3][MAX_ORIENT] =
/* current orient, rotation */
{
	{1, 5, 1, 5, 4, 2},
	{2, 0, 2, 0, 5, 3},
	{3, 1, 3, 1, 0, 4}
};


static int
toPosition(pyraminxstruct * pp, CRD crd)
{
	int diag;

	if (crd.diagonal < pp->size)
		return (crd.diagonal * crd.diagonal + crd.diagonal +
			crd.column - crd.row);
	diag = 2 * pp->size - 1 - crd.diagonal;
	return (diag * diag + diag + crd.row - crd.column);
}

static int
positionCRD(int dir, int i, int j, int side)
{
	if (dir == TOP || dir == BOTTOM)
		return (i);
	else if (dir == RIGHT || dir == LEFT)
		return (j);
	else	/* dir == TR || dir == BL */
		return (i + j + side);
}

static int
length(pyraminxstruct * pp, int face, int dir, int h)
{
	if (dir == TR || dir == BL)
		return ((face == UP) ? 2 * pp->size - 1 - h : h);
	else
		return ((face == UP) ? h : pp->size - 1 - h);
}

static void
rotateFace(pyraminxstruct * pp, int view, int faceOnView, int direction)
{
	int g, h, side, face, position;
	CRD crd;

	/* Read Face */
	for (g = 0; g < pp->size; g++)
		for (h = 0; h < pp->size - g; h++)
			for (side = 0; side < MAX_SIDES; side++)
				if (g + h + side < pp->size) {
					if (faceOnView == DOWN) {
						crd.column = h;
						crd.row = g;
						crd.diagonal = h + g + side;
						face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
						position = toPosition(pp, crd);
						pp->faceLoc[side][h + g * pp->size] =
							pp->facetLoc[face][position];
					} else {	/* faceOnView == UP */
						crd.column = pp->size - 1 - h;
						crd.row = pp->size - 1 - g;
						crd.diagonal = crd.column + crd.row + !side;
						face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
						position = toPosition(pp, crd);
						pp->faceLoc[side][h + g * pp->size] =
							pp->facetLoc[face][position];
					}
				}
	/* Write Face */
	if (faceOnView == DOWN) {
		for (g = 0; g < pp->size; g++)
			for (h = 0; h < pp->size - g; h++)
				for (side = 0; side < MAX_SIDES; side++)
					if (g + h + side < pp->size) {
						crd.column = h;
						crd.row = g;
						crd.diagonal = h + g + side;
						face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
						position = toPosition(pp, crd);
						if (direction == CCW)
							pp->facetLoc[face][position] =
								pp->faceLoc[side][pp->size - 1 - g - h - side +
							h * pp->size];
						else	/* direction == CW */
							pp->facetLoc[face][position] =
								pp->faceLoc[side][g +
											(pp->size - 1 - g - h - side) * pp->size];
						pp->facetLoc[face][position].rotation =
							(direction == CW) ?
							(pp->facetLoc[face][position].rotation + 2) %
							MAX_ORIENT :
							(pp->facetLoc[face][position].rotation + 4) %
							MAX_ORIENT;
						/* DrawTriangle(face, position); */
					}
	} else {		/* faceOnView == UP */
		for (g = pp->size - 1; g >= 0; g--)
			for (h = pp->size - 1; h >= pp->size - 1 - g; h--)
				for (side = 1; side >= 0; side--)
					if (g + h + side >= pp->size) {
						crd.column = h;
						crd.row = g;
						crd.diagonal = h + g + side;
						face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
						position = toPosition(pp, crd);
						if (direction == CCW)
							pp->facetLoc[face][position] =
								pp->faceLoc[!side][g + h - pp->size + 1 - !side
											+ (pp->size - 1 - h) * pp->size];
						else	/* (direction == CW) */
							pp->facetLoc[face][position] =
								pp->faceLoc[!side][pp->size - 1 - g +
											(g + h - pp->size + 1 - !side) * pp->size];
						pp->facetLoc[face][position].rotation =
							(direction == CW) ?
							(pp->facetLoc[face][position].rotation + 2) %
							MAX_ORIENT :
							(pp->facetLoc[face][position].rotation + 4) %
							MAX_ORIENT;
						/* DrawTriangle(face, position); */
					}
	}
}

static void
readCRD2(pyraminxstruct * pp, int view, int dir, int h, int orient)
{
	int g, i, j, side, faceOnView, s, face, position, v = view;
	CRD crd;

	if (dir == TOP || dir == BOTTOM) {
		for (g = 0; g < pp->size; g++)
			for (side = 0; side < MAX_SIDES; side++) {
				crd.column = h;
				crd.row = g;
				crd.diagonal = h + g + side;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->rowLoc[orient][side][g] =
					pp->facetLoc[face][position];
			}
	} else if (dir == RIGHT || dir == LEFT) {
		for (g = 0; g < pp->size; g++)
			for (side = 0; side < MAX_SIDES; side++) {
				crd.column = g;
				crd.row = h;
				crd.diagonal = h + g + side;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->rowLoc[orient][side][g] =
					pp->facetLoc[face][position];
			}
	} else {		/* dir == TR || dir == BL */
		faceOnView = (h < pp->size) ? DOWN : UP;
		i = (faceOnView == UP) ? pp->size - 1 : 0;
		j = h % pp->size;
		for (g = 0; g < pp->size; g++) {
			for (side = 0; side < MAX_SIDES; side++) {
				s = (((side == UP) && (faceOnView == UP)) ||
					((side == DOWN) && (faceOnView == DOWN)))
					? DOWN : UP;
				crd.column = i;
				crd.row = j;
				crd.diagonal = i + j + s;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->rowLoc[orient][side][g] =
					pp->facetLoc[face][position];
				if (side == DOWN) {
					if (faceOnView == UP) {
						if (j == pp->size - 1)
							v = (v == UP) ? DOWN : UP;
						j = (j + 1) % pp->size;
					} else {	/* faceOnView == DOWN */
						if (!j)
							v = (v == UP) ? DOWN : UP;
						j = (j - 1 + pp->size) % pp->size;
					}
				}
			}
			i = (faceOnView == UP) ? i - 1 : i + 1;
		}
	}
}

static void
readCRD3(pyraminxstruct * pp, int view, int faceOnView, int dir, int h, int len, int orient)
{
	int g, i, j, side, s, face, position, faceView = faceOnView;
	CRD crd;

	if (dir == TOP || dir == BOTTOM) {
		for (g = 0; g <= len; g++)
			for (side = 0; side < MAX_SIDES; side++)
				if ((side == DOWN) || g < len) {
					crd.column = h;
					crd.row = (faceView == UP) ? pp->size - 1 - g : g;
					crd.diagonal = h + crd.row +
						((((side == UP) && (faceView == DOWN)) ||
						((side == DOWN) && (faceView == UP)))
						? UP : DOWN);
					face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->rowLoc[orient][side][g] =
						pp->facetLoc[face][position];
				}
	} else if (dir == RIGHT || dir == LEFT) {
		for (g = 0; g <= len; g++)
			for (side = 0; side < MAX_SIDES; side++)
				if ((side == DOWN) || g < len) {
					crd.column = (faceView == UP) ? pp->size - 1 - g : g;
					crd.row = h;
					crd.diagonal = h + crd.column +
						((((side == UP) && (faceView == DOWN)) ||
						((side == DOWN) && (faceView == UP)))
						? UP : DOWN);
					face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->rowLoc[orient][side][g] =
						pp->facetLoc[face][position];
				}
	} else {	/* dir == TR || dir == BL */
		faceView = (h < pp->size) ? DOWN : UP;
		i = (faceView == UP) ? pp->size - 1 : 0;
		j = h % pp->size;
		for (g = 0; g <= len; g++) {
			for (side = 0; side < MAX_SIDES; side++) {
				if (side == DOWN || g < len) {
					s = (((side == UP) && (faceView == UP)) ||
						((side == DOWN) && (faceView == DOWN)))
						? DOWN : UP;
					crd.column = i;
					crd.row = j;
					crd.diagonal = i + j + s;
					face = view * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->rowLoc[orient][side][g] =
						pp->facetLoc[face][position];
					if (side == DOWN)
						j = (faceView == UP) ? j + 1 : j - 1;
				}
			}
			i = (faceView == UP) ? i - 1 : i + 1;
		}
	}
}

static void
rotateCRD2(pyraminxstruct * pp, int rotate, int orient)
{
	int g, side;

	for (g = 0; g < pp->size; g++)
		for (side = 0; side < MAX_SIDES; side++)
			pp->rowLoc[orient][side][g].rotation =
				(pp->rowLoc[orient][side][g].rotation + rotate) % MAX_ORIENT;
}

static void
rotateCRD3(pyraminxstruct *pp, int faceOnView, int dir, int len, int orient)
{
	int g, side, direction, facetOrient;

	for (g = 0; g <= len; g++)
		for (side = 0; side < MAX_SIDES; side++)
			if (side == DOWN || g < len) {
				direction = (faceOnView == UP) ? (dir + 3) % MAX_ORIENT : dir;
				facetOrient = pp->rowLoc[orient][side][g].rotation;
				pp->rowLoc[orient][side][g].rotation =
					(facetOrient >= 3) ?
					(rotOrientRowP3[facetOrient - 3][direction] + 3) % MAX_ORIENT :
					rotOrientRowP3[facetOrient][direction];
			}
}

static void
reverseCRD2(pyraminxstruct * pp, int orient)
{
	PyraminxLoc temp;
	int g;

	for (g = 0; g < pp->size; g++) {
		temp = pp->rowLoc[orient][g & 1][g / 2];
		pp->rowLoc[orient][g & 1][g / 2] =
			pp->rowLoc[orient][((g & 1) == 1) ? 0 : 1][pp->size - 1 - g / 2];
		pp->rowLoc[orient][((g & 1) == 1) ? 0 : 1][pp->size - 1 - g / 2] =
			temp;
	}
}

static void
reverseCRD3(pyraminxstruct * pp, int len, int orient)
{
	PyraminxLoc temp;
	int g;

	for (g = 0; g < len; g++) {
		temp = pp->rowLoc[orient][g & 1][len - ((g + 1) / 2)];
		pp->rowLoc[orient][g & 1][len - ((g + 1) / 2)] =
			pp->rowLoc[orient][g & 1][g / 2];
		pp->rowLoc[orient][g & 1][g / 2] = temp;
	}
}

static void
writeCRD2(pyraminxstruct * pp, int view, int dir, int h, int orient)
{
	int g, side, i, j, s, faceOnView, face, position, v = view;
	CRD crd;

	if (dir == TOP || dir == BOTTOM) {
		for (g = 0; g < pp->size; g++)
			for (side = 0; side < MAX_SIDES; side++) {
				crd.column = h;
				crd.row = g;
				crd.diagonal = h + g + side;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->facetLoc[face][position] =
					pp->rowLoc[orient][side][g];
				/* DrawTriangle(v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN),
					toPosition(pp, crd)); */
			}
	} else if (dir == RIGHT || dir == LEFT) {
		for (g = 0; g < pp->size; g++)
			for (side = 0; side < MAX_SIDES; side++) {
				crd.column = g;
				crd.row = h;
				crd.diagonal = h + g + side;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->facetLoc[face][position] =
					pp->rowLoc[orient][side][g];
				/* DrawTriangle(face, position); */
			}
	} else {	/* dir == TR || dir == BL */
		faceOnView = (h < pp->size) ? DOWN : UP;
		i = (faceOnView == UP) ? pp->size - 1 : 0;
		j = h % pp->size;
		for (g = 0; g < pp->size; g++) {
			for (side = 0; side < MAX_SIDES; side++) {
				s = (((side == UP) && (faceOnView == UP)) ||
					((side == DOWN) && (faceOnView == DOWN)))
					? DOWN : UP;
				crd.column = i;
				crd.row = j;
				crd.diagonal = i + j + s;
				face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
				position = toPosition(pp, crd);
				pp->facetLoc[face][position] =
					pp->rowLoc[orient][side][g];
				/* DrawTriangle(face, position); */
				if (side == DOWN) {
					if (faceOnView == UP) {
						if (j == pp->size - 1) {
							v = (v == UP) ? DOWN : UP;
							j = 0;
						} else
							++j;
					} else {	/* FACE == DOWN */
						if (j == 0) {
							v = (v == UP) ? DOWN : UP;
							j = pp->size - 1;
						} else
							--j;
					}
				}
			}
			if (faceOnView == UP)
				--i;
			else	/* faceOnView == DOWN */
				++i;
		}
	}
}

static void
writeCRD3(pyraminxstruct * pp, int view, int faceOnView, int dir, int h, int len, int orient)
{
	int g, side, i, j, k, s, face, position, v = view, faceView = faceOnView;
	CRD crd;

	if (dir == TOP || dir == BOTTOM) {
		for (k = 0; k <= len; k++)
			for (side = 0; side < MAX_SIDES; side++)
				if ((side == DOWN) || k < len) {
					g = (faceView == UP) ? pp->size - 1 - k : k;
					s = (((side == UP) && (faceView == DOWN)) ||
						((side == DOWN) && (faceView == UP)))
						? UP : DOWN;
					crd.column = h;
					crd.row = g;
					crd.diagonal = h + g + s;
					face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->facetLoc[face][position] =
						pp->rowLoc[orient][side][k];
					/* DrawTriangle(face, position); */
				}
	} else if (dir == RIGHT || dir == LEFT) {
		for (k = 0; k <= len; k++)
			for (side = 0; side < MAX_SIDES; side++)
				if ((side == DOWN) || k < len) {
					g = (faceView == UP) ? pp->size - 1 - k : k;
					s = (((side == UP) && (faceView == DOWN)) ||
						((side == DOWN) && (faceView == UP)))
						? UP : DOWN;
					crd.column = g;
					crd.row = h;
					crd.diagonal = h + g + s;
					face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->facetLoc[face][position] =
						pp->rowLoc[orient][side][k];
					/* DrawTriangle(face, position); */
				}
	} else {		/* dir == TR || dir == BL */
		faceView = (h < pp->size) ? DOWN : UP;
		i = (faceView == UP) ? pp->size - 1 : 0;
		j = h % pp->size;
		for (k = 0; k <= len; k++) {
			for (side = 0; side < MAX_SIDES; side++)
				if ((side == DOWN) || k < len) {
					s = (((side == UP) && (faceView == UP)) ||
						((side == DOWN) && (faceView == DOWN)))
						? DOWN : UP;
					crd.column = i;
					crd.row = j;
					crd.diagonal = i + j + s;
					face = v * MAX_SIDES + ((crd.diagonal >= pp->size) ? UP : DOWN);
					position = toPosition(pp, crd);
					pp->facetLoc[face][position] =
						pp->rowLoc[orient][side][k];
					/* DrawTriangle(face, position); */
					if (side == DOWN) {
						if (faceView == UP) {
							if (j == pp->size - 1) {
								v = (v == UP) ? DOWN : UP;
								j = 0;
							} else
								++j;
						} else {	/* FACE == DOWN */
							if (j == 0) {
								v = (v == UP) ? DOWN : UP;
								j = pp->size - 1;
							} else
								--j;
						}
					}
				}
			if (faceView == UP)
				--i;
			else	/* faceView == DOWN */
				++i;
		}
	}
}

static void
movePieces(pyraminxstruct *pp, int face, CRD crd, int direction, int style,
		Bool control)
{
	Bool reverse, bound;
	int view, side;
	int newSide, newView, rotate, h, k, newH, faceOnView, len;
	int newFace, newDirection, l = 0;
	int dir = direction;

#if 0
	if (!control) {
		movePiecesGL(pp, face, crd, direction,
			style, control);
	}
#endif
	view = face / MAX_SIDES;
	side = crd.diagonal - crd.column - crd.row;
	if (style == PERIOD2) {
		/* Period 2 Slide rows */
		h = positionCRD(dir, crd.column, crd.row, side);
		if (pp->sticky && (h % 4 == 1 || h % 4 == 2)) {
			bound = True;
			l = 0;
			if (h % 4 == 2)
				h = h - 1;
		} else
			bound = False;
		do {
			readCRD2(pp, view, dir, h, 0);
			for (k = 1; k <= 2; k++) {
				rotate = slideNextRowP2[side][dir % 3].rotation;
				if (dir == TR || dir == BL) {
					newView = view;
					newSide = !side;
					newH = 2 * pp->size - 1 - h;
					reverse = False;
				} else if (!rotate) {
					newView = !view;
					newSide = side;
					newH = h;
					reverse = False;
				} else {	/* rotate == 3 */
					newView = !view;
					newSide = !side;
					newH = pp->size - 1 - h;
					reverse = True;
				}
				if (k != 2)
					readCRD2(pp, newView, dir, newH, k);
				rotateCRD2(pp, rotate, k - 1);
				if (reverse)
					reverseCRD2(pp, k - 1);
				writeCRD2(pp, newView, dir, newH, k - 1);
				view = newView;
				h = newH;
				side = newSide;
			}
			l++;
			h++;
		} while (bound && l < 2);
	} else {		/* style == PERIOD3 */
		faceOnView = (crd.diagonal >= pp->size) ? UP : DOWN;
		h = positionCRD(dir, crd.column, crd.row, side);
		bound = False;
		if (dir == TR || dir == BL) {
			if (h == pp->size)
				rotateFace(pp, view, DOWN, (dir == TR) ? CCW : CW);
			else if (h == pp->size - 1)
				rotateFace(pp, view, UP, (dir == TR) ? CW : CCW);
			else if (pp->sticky)
				bound = True;
		} else if (!crd.column && faceOnView == DOWN &&
				(dir == TOP || dir == BOTTOM))
			rotateFace(pp, (view == UP) ? DOWN : UP, DOWN,
			 (dir == TOP || dir == LEFT) ? CCW : CW);
		else if (crd.column == pp->size - 1 && faceOnView &&
			 (dir == TOP || dir == BOTTOM))
			rotateFace(pp, (view == UP) ? DOWN : UP, UP,
				(dir == BOTTOM || dir == RIGHT) ? CCW : CW);
		else if (crd.row == 0 && faceOnView == DOWN &&
			 (dir == RIGHT || dir == LEFT))
			rotateFace(pp, (view == UP) ? DOWN : UP, UP,
				(dir == BOTTOM || dir == RIGHT) ? CCW : CW);
		else if (crd.row == pp->size - 1 && faceOnView == UP &&
			 (dir == RIGHT || dir == LEFT))
			rotateFace(pp, (view == UP) ? DOWN : UP, DOWN,
			 (dir == TOP || dir == LEFT) ? CCW : CW);
		else if (pp->sticky)
			bound = True;
		/* Slide rows */
		if (bound) {
			l = 0;
			if (dir == TR || dir == BL)
				h = (faceOnView == UP) ? pp->size + 1 : 0;
			else
				h = (faceOnView == UP) ? 0 : 1;
		}
		do {
			len = length(pp, faceOnView, dir, h);
			readCRD3(pp, view, faceOnView, dir, h, len, 0);
			for (k = 1; k <= 3; k++) {
				newView = (slideNextRowP3[faceOnView][dir].viewChanged)
					? ((view == UP) ? DOWN : UP) : view;
				newFace = slideNextRowP3[faceOnView][dir].face;
				newDirection = slideNextRowP3[faceOnView][dir].direction;
				reverse = slideNextRowP3[faceOnView][dir].reverse;
				newH = pp->size - 1 - h;
				if (faceOnView == UP) {
					if (dir == TR || dir == RIGHT)
						newH = 2 * pp->size - 1 - h;
					else if (dir == BOTTOM)
						newH = h;
					else if (dir == BL)
						newH = h - pp->size;
				} else {
					if (dir == TOP)
						newH = pp->size + h;
					else if (dir == TR)
						newH = h;
				}
				if (k != 3)
					readCRD3(pp, newView, newFace, newDirection, newH, len, k);
				rotateCRD3(pp, faceOnView, dir, len, k - 1);
				if (reverse)
					reverseCRD3(pp, len, k - 1);
				writeCRD3(pp, newView, newFace, newDirection, newH, len, k - 1);
				view = newView;
				faceOnView = newFace;
				dir = newDirection;
				h = newH;
			}
			h++;
			l++;
		} while (bound && l < pp->size - 1);
	}
}

static void
moveControlCb(pyraminxstruct * pp, int face, int direction, int style)
{
	int i, faceOnView;
	CRD crd;

	crd.column = 0;
	crd.row = 0;
	crd.diagonal = crd.row;
#if 0
	movePiecesGL(pp, face, crd, direction,
		style, True);
#endif
	faceOnView = face % MAX_SIDES;
	if (pp->sticky) {
		if (style == PERIOD2)
			for (i = 0; i < 3; i++) {
				if (direction == TR || direction == BL) {
					crd.column = 0;
					crd.row = 3 * i / 2;
					crd.diagonal = crd.row + i % 2;
				} else {
					crd.column = 3 * i / 2;
					crd.row = 3 * i / 2;
					crd.diagonal = crd.row + crd.column;
				}
				movePieces(pp, face, crd, direction, style, True);
			}
		else	/* (style == PERIOD3) */
			for (i = 0; i < 2; i++) {
				if (direction == TR || direction == BL) {
					crd.column = 1 + faceOnView;
					crd.row = 1 + faceOnView;
					crd.diagonal = crd.row + crd.column + i;
				} else {
					crd.column = i + 2 * faceOnView;
					crd.row = i + 2 * faceOnView;
					crd.diagonal = crd.row + crd.column + faceOnView;
				}
				movePieces(pp, face, crd, direction, style, True);
			}
	} else {
		for (i = 0; i < pp->size; i++) {
			if (direction == TR || direction == BL) {
				if (style == PERIOD2) {
					crd.column = 0;
					crd.row = i;
					crd.diagonal = crd.row;
				} else {
					crd.column = faceOnView * (pp->size - 1);
					crd.row = i;
					crd.diagonal = crd.column + crd.row + faceOnView;
				}
			} else {
				crd.column = i;
				crd.row = pp->size - 1 - i;
				crd.diagonal = crd.column + crd.row + faceOnView;
			}
			movePieces(pp, face, crd, direction, style, True);
		}
	}
}

#if 0
static Bool
moveAllPieces(pyraminxstruct * pp, const int face, const int position,
		const int direction, const int style, const Bool control)
{
	int size;
	Bool newControl;
	CRD crd;

	size = pp->size;
	toCRD(pp, face, position, &crd);
	pp->degreeTurn = 360 / style;
	if (control || size == 1) {
		newControl = True;
#if 0
		int k, newPosition;
		for (k = 0; k < size; k++) {
			newPosition = k * k; /* FIXME */
			toCRD(pp, face, newPosition, &crd);
			movePieces(pp, face, crd, direction, style,
				newControl, newFast);
		}
#endif
	} else {
		newControl = False; 
		movePieces(pp, face, crd, direction, style, control);
	}
	return newControl;
}
#endif

static void
movePyraminx(pyraminxstruct * pp, int face, int position, int direction,
		int style, int control)
{
	if (control) {
		moveControlCb(pp, face, direction, style);
	} else {
		CRD crd;

		toCRD(pp, face, position, &crd);
		movePieces(pp, face, crd, direction, style, False);
	}
}

#ifdef DEBUG
static void
printTetrahedron(pyraminxstruct * pp)
{
	(void) printf("FIXME\n");
}

#endif

/*-
 * This rotates whole tetrahedron.
 */
static void
rotateAllFaces(ModeInfo * mi, GLfloat rotateStep,
		GLfloat x, GLfloat y, GLfloat z)
{
	glPushMatrix();
	glRotatef(rotateStep, x, y, z);
	drawFaces(mi, -1, 0, 0, False, True);
	glPopMatrix();
}

static void
rotateFaceSlice(ModeInfo * mi, GLfloat rotateStep,
		GLfloat x, GLfloat y, GLfloat z,
		int face, int position, int direction)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];
	glPushMatrix();
	glPushMatrix();
	glRotatef(rotateStep, x, y, z);
	if (pp->movement.control) 
		drawFaces(mi, face, 0, direction, False, True);
	else
		drawFaces(mi, face, position, direction, True, False);
	glPopMatrix();
	if (!pp->movement.control) 
		drawFaces(mi, face, position, direction, False, False);
	glPopMatrix();
}

static void
draw_tetra(ModeInfo * mi)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];
	int face = pp->movement.face;
	int position = pp->movement.position;
	int direction = pp->movement.direction;
	int style = PERIOD3; /* pp->movement.style; */
	int control = 0; /* pp->movement.control; */

	pp->movement.control = control;
	if (direction == -1) {
		rotateAllFaces(mi, 0.0f, 0, 0, 0);
/* 6 edge */
/* 0 1 0 rotate front edge CCW <=> 0 BL */
/* 0 -1 0 rotate front edge CW <=> 0 TR */
/* 1 0 0 rotate left edge CW <=> 0 BOTTOM */
/* -1 0 0 rotate left edge CCW <=> 0 TOP */
/* 0 0 1 rotate right edge CCW <=> 0 LEFT */
/* 0 0 -1 rotate right edge CW <=> 0 RIGHT */
	} else if (style == 2) {
		/*if ((face == 1 || face == 2) && (direction == TR || direction == BL))
			direction = (direction + MAX_ORIENT / 2) % MAX_ORIENT;*/
		switch (direction) {
		case TOP:
			rotateAllFaces(mi, pp->rotateStep, -1, 0, 0);
			break;
		case TR:
			rotateAllFaces(mi, pp->rotateStep, 0, -1, 0);
			break;
		case RIGHT:
			rotateAllFaces(mi, pp->rotateStep, 0, 0, -1);
			break;
		case BOTTOM:
			rotateAllFaces(mi, pp->rotateStep, 1, 0, 0);
			break;
		case BL:
			rotateAllFaces(mi, pp->rotateStep, 0, 1, 0);
			break;
		case LEFT:
			rotateAllFaces(mi, pp->rotateStep, 0, 0, 1);
			break;
		default:
			rotateAllFaces(mi, 0.0f, 0, 0, 0);
		}
	} else if (style == 3) {
/* 8 face/corner rotate */
/*	  1-1-1  */
/*	111      */
/* 1 -1 -1 rotate front faces CCW <=> 0 LEFT */
/* -1 1 -1 rotate right face CCW <=> 0 BL */
/* -1 -1 1 rotate top face CCW <=> 1 TR */
/* 1 1 1 rotate left face CCW <=> 0 TOP */
/* -1 1 1 rotate front faces CW <=> 0 RIGHT */
/* 1 -1 1 rotate right face CW <=> 0 TR */
/* 1 1 -1 rotate top face CW <=> 1 BL */
/* -1 -1 -1 rotate left face CW <=> BOTTOM */
		switch (pp->movement.face) {
		case 0:
			switch (direction) {
			case TOP:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, 1, face, position, direction);
				break;
			case TR:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, -1, face, position, direction);
				break;
			case RIGHT:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, -1, face, position, direction);
				break;
			case BOTTOM:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, -1, face, position, direction);
				break;
			case BL:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, 1, face, position, direction);
				break;
			case LEFT:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, 1, face, position, direction);
				break;
			default:
				rotateAllFaces(mi, 0.0f, 0, 0, 0);
			}
			break;
		case 1:
			switch (direction) {
			case TOP:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, 1, face, position, direction);
				break;
			case TR:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, -1, face, position, direction);
				break;
			case RIGHT:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, -1, face, position, direction);
				break;
			case BOTTOM:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, -1, face, position, direction);
				break;
			case BL:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, 1, face, position, direction);
				break;
			case LEFT:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, 1, face, position, direction);
				break;
			default:
				rotateAllFaces(mi, 0.0f, 0, 0, 0);
			}
			break;
		case 2:
			switch (direction) {
			case TOP:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, -1, face, position, direction);
				break;
			case TR:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, 1, face, position, direction);
				break;
			case RIGHT:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, 1, face, position, direction);
				break;
			case BOTTOM:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, 1, face, position, direction);
				break;
			case BL:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, -1, face, position, direction);
				break;
			case LEFT:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, -1, face, position, direction);
				break;
			default:
				rotateAllFaces(mi, 0.0f, 0, 0, 0);
			}
			break;
		case 3:
			switch (direction) {
			case TOP:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, -1, face, position, direction);
				break;
			case TR:
				rotateFaceSlice(mi, pp->rotateStep, -1, -1, 1, face, position, direction);
				break;
			case RIGHT:
				rotateFaceSlice(mi, pp->rotateStep, 1, -1, 1, face, position, direction);
				break;
			case BOTTOM:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, 1, face, position, direction);
				break;
			case BL:
				rotateFaceSlice(mi, pp->rotateStep, 1, 1, -1, face, position, direction);
				break;
			case LEFT:
				rotateFaceSlice(mi, pp->rotateStep, -1, 1, -1, face, position, direction);
				break;
			default:
				rotateAllFaces(mi, 0.0f, 0, 0, 0);
			}
			break;
		default:
			rotateAllFaces(mi, 0.0f, 0, 0, 0);
		}
	} else {
		rotateAllFaces(mi, 0.0f, 0, 0, 0);
	}
}

static void
evalmovement(ModeInfo * mi, PyraminxMove movement)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];

#ifdef DEBUG
	printTetrahedron(pp);
#endif
	if (movement.face < 0 || movement.face >= MAX_FACES)
		return;
	movePyraminx(pp, movement.face, movement.position, movement.direction,
		movement.style, False);

}

#ifdef HACK
static      Bool
compare_moves(pyraminxstruct * pp, PyraminxMove move1, PyraminxMove move2, Bool opp)
{
#ifdef FIXME
	PyraminxLoc  slice1, slice2;

	convertMove(pp, move1, &slice1);
	convertMove(pp, move2, &slice2);
	if (slice1.face == slice2.face) {
		if (slice1.rotation == slice2.rotation) { /* CW or CCW */
			if (!opp)
				return True;
		} else {
			if (opp)
				return True;
		}
	}
#endif
	return False;
}
#endif

static Bool
shuffle(ModeInfo * mi)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];
	int	 i, face, position, orient, side;
	PyraminxMove   move;

	i = MI_SIZE(mi);
	if (i < -MINSIZE) {
		i = NRAND(-i - MINSIZE + 1) + MINSIZE;
		if (i == MINSIZE && NRAND(4) != 0) {
			i = 4;
			pp->sticky = True;
		}
	} else if (i < MINSIZE)
		 i = MINSIZE;
	pp->size = i;
	for (face = 0; face < MAX_FACES; face++) {
		if (pp->facetLoc[face] != NULL)
			free(pp->facetLoc[face]);
		if ((pp->facetLoc[face] = (PyraminxLoc *) malloc(pp->size * pp->size *
				sizeof (PyraminxLoc))) == NULL) {
			return False;
		}
		for (position = 0; position < pp->size * pp->size; position++) {
			pp->facetLoc[face][position].face = face;
			pp->facetLoc[face][position].rotation = TOP;
		}
	}
	for (orient = 0; orient < 3; orient++)
		for (side = 0; side < MAX_SIDES; side++) {
			if (pp->rowLoc[orient][side] != NULL)
				free(pp->rowLoc[orient][side]);
			if ((pp->rowLoc[orient][side] = (PyraminxLoc *) malloc(pp->size *
					sizeof (PyraminxLoc))) == NULL) {
				return False;
			}
		}
	for (side = 0; side < MAX_SIDES; side++) {
	       	if (pp->faceLoc[side] != NULL)
		       	free(pp->faceLoc[side]);
	       	if ((pp->faceLoc[side] = (PyraminxLoc *) malloc(pp->size * pp->size *
				sizeof (PyraminxLoc))) == NULL) {
			return False;
		}
	}
	pp->storedmoves = MI_COUNT(mi);
	if (pp->storedmoves < 0) {
		if (pp->moves != NULL)
			free(pp->moves);
		pp->moves = (PyraminxMove *) NULL;
		pp->storedmoves = NRAND(-pp->storedmoves) + 1;
	}
	if ((pp->storedmoves) && (pp->moves == NULL))
		if ((pp->moves = (PyraminxMove *) calloc(pp->storedmoves + 1,
				sizeof (PyraminxMove))) == NULL) {
			return False;
		}
	if (MI_CYCLES(mi) <= 1) {
		pp->anglestep = pp->degreeTurn;
	} else {
		pp->anglestep = pp->degreeTurn / (GLfloat) (MI_CYCLES(mi));
	}

	for (i = 0; i < pp->storedmoves; i++) {
		Bool condition;

		do {
			move.face = NRAND(MAX_FACES);
			move.position = NRAND(pp->size * pp->size);
			move.direction = ((move.position +
				((NRAND(2) == 0) ? 1 : 3)) % MAX_ORIENT);
			move.style = pp->period2;
			condition = True;
			/*
			 * Some silly moves being made, weed out later....
			 */
		} while (!condition);
		if (hideshuffling)
			evalmovement(mi, move);
		pp->moves[i] = move;
	}
	pp->VX = 0.05;
	if (NRAND(100) < 50)
		pp->VX *= -1;
	pp->VY = 0.05;
	if (NRAND(100) < 50)
		pp->VY *= -1;
	pp->movement.face = NO_FACE;
	pp->rotateStep = 0;
	pp->action = hideshuffling ? ACTION_SOLVE : ACTION_SHUFFLE;
	pp->shufflingmoves = 0;
	pp->done = 0;
	return True;
}

static void
reshape_pyraminx(ModeInfo * mi, int width, int height)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];

	glViewport(0, 0, pp->WindW = (GLint) width, pp->WindH = (GLint) height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 15.0);
	glMatrixMode(GL_MODELVIEW);

	pp->AreObjectsDefined[ObjCubit] = False;
}

#ifdef STANDALONE
ENTRYPOINT Bool
pyraminx_handle_event (ModeInfo *mi, XEvent *event)
{
	pyraminxstruct *pp = &pyraminx[MI_SCREEN(mi)];

	if (gltrackball_event_handler (event, pp->trackball,
			MI_WIDTH (mi), MI_HEIGHT (mi),
			&pp->button_down_p))
		return True;
	else if (screenhack_event_helper (MI_DISPLAY(mi), MI_WINDOW(mi), event)) {
		pp->done = 1;
		return True;
	}
	return False;
}
#endif

static Bool
pinit(ModeInfo * mi)
{
	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glColor3f(1.0, 1.0, 1.0);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glEnable(GL_CULL_FACE);

	glShadeModel(GL_FLAT);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_shininess);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_specular);

	return (shuffle(mi));
}

static void
free_pyraminx_screen(pyraminxstruct *pp)
{
	int face, orient, side;

	if (pp == NULL) {
		return;
	}
#ifdef STANDALONE
	if (pp->trackball)
		gltrackball_free (pp->trackball);
#endif
	for (face = 0; face < MAX_FACES; face++)
		if (pp->facetLoc[face] != NULL) {
			free(pp->facetLoc[face]);
			pp->facetLoc[face] = (PyraminxLoc *) NULL;
		}
	for (orient = 0; orient < 3; orient++)
		for (side = 0; side < MAX_SIDES; side++)
			if (pp->rowLoc[orient][side] != NULL) {
				free(pp->rowLoc[orient][side]);
				pp->rowLoc[orient][side] = (PyraminxLoc *) NULL;
			}
	for (side = 0; side < MAX_SIDES; side++)
	       	if (pp->faceLoc[side] != NULL) {
		       	free(pp->faceLoc[side]);
			pp->faceLoc[side] = (PyraminxLoc *) NULL;
		}
	if (pp->moves != NULL) {
		free(pp->moves);
		pp->moves = (PyraminxMove *) NULL;
	}
	pp = NULL;
}

ENTRYPOINT void
release_pyraminx(ModeInfo * mi)
{
	if (pyraminx != NULL) {
		int	 screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			free_pyraminx_screen(&pyraminx[screen]);
		free(pyraminx);
		pyraminx = (pyraminxstruct *) NULL;
	}
	FreeAllGL(mi);
}

ENTRYPOINT void
init_pyraminx(ModeInfo * mi)
{
	pyraminxstruct *pp;

	MI_INIT(mi, pyraminx);
	pp = &pyraminx[MI_SCREEN(mi)];

	pp->PX = ((float) LRAND() / (float) MAXRAND) * 2.0 - 1.0;
	pp->PY = ((float) LRAND() / (float) MAXRAND) * 2.0 - 1.0;

#if 0
	/* Period 2 needs work on the turning */
	if (MI_IS_FULLRANDOM(mi)) {
		pp->period2 = (Bool) (LRAND() & 1);
	} else {
		pp->period2 = period2;
	}
#else
	pp->period2 = False;
#endif
	if (pp->period2) {
		pp->degreeTurn = 180;
		pp->movement.style = PERIOD2;
	} else {
		pp->degreeTurn = 120;
		pp->movement.style = PERIOD3;
	}
	pp->step = NRAND(pp->degreeTurn);

#ifdef STANDALONE
	pp->trackball = gltrackball_init (True);
#endif

	if ((pp->glx_context = init_GL(mi)) != NULL) {
		reshape_pyraminx(mi, MI_WIDTH(mi), MI_HEIGHT(mi));
		glDrawBuffer(GL_BACK);
		if (!pinit(mi)) {
			free_pyraminx_screen(pp);
			if (MI_IS_VERBOSE(mi)) {
				 (void) fprintf(stderr,
					"Could not allocate memory for pyraminx\n");
			}
		}
	} else {
		MI_CLEARWINDOW(mi);
	}
}

ENTRYPOINT void
draw_pyraminx(ModeInfo * mi)
{
	Bool bounced = False;
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	pyraminxstruct *pp;

	if (pyraminx == NULL)
		return;
	pp = &pyraminx[MI_SCREEN(mi)];
	if ((pp->storedmoves) && (pp->moves == NULL))
		return;
	if (!pp->glx_context)
		return;
	MI_IS_DRAWN(mi) = True;
#ifdef WIN32
	wglMakeCurrent(hdc, pp->glx_context);
#else
	glXMakeCurrent(display, window, *(pp->glx_context));
#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	glTranslatef(0.0, 0.0, -10.0);

	pp->PX += pp->VX;
	pp->PY += pp->VY;

	if (pp->PY < -1) {
		pp->PY += (-1) - (pp->PY);
		pp->VY = -pp->VY;
		bounced = True;
	}
	if (pp->PY > 1) {
		pp->PY -= (pp->PY) - 1;
		pp->VY = -pp->VY;
		bounced = True;
	}
	if (pp->PX < -1) {
		pp->PX += (-1) - (pp->PX);
		pp->VX = -pp->VX;
		bounced = True;
	}
	if (pp->PX > 1) {
		pp->PX -= (pp->PX) - 1;
		pp->VX = -pp->VX;
		bounced = True;
	}
	if (bounced) {
		pp->VX += ((float) LRAND() / (float) MAXRAND) * 0.02 - 0.01;
		pp->VX += ((float) LRAND() / (float) MAXRAND) * 0.02 - 0.01;
		if (pp->VX > 0.06)
			pp->VX = 0.06;
		if (pp->VY > 0.06)
			pp->VY = 0.06;
		if (pp->VX < -0.06)
			pp->VX = -0.06;
		if (pp->VY < -0.06)
			pp->VY = -0.06;
	}
	if (!MI_IS_ICONIC(mi)) {
		glTranslatef(pp->PX, pp->PY, 0);
		glScalef(Scale4Window * pp->WindH / pp->WindW,
			Scale4Window, Scale4Window);
	} else {
		glScalef(Scale4Iconic * pp->WindH / pp->WindW,
			Scale4Iconic, Scale4Iconic);
	}

# ifdef HAVE_MOBILE     /* Keep it the same relative size when rotated. */
	{
		GLfloat h = MI_HEIGHT(mi) / (GLfloat) MI_WIDTH(mi);
		int o = (int) current_device_rotation();
		if (o != 0 && o != 180 && o != -180) {
			glScalef (1/h, h, 1); /* #### not quite right */
			h = 1.8;
			glScalef (h, h, h);
		}
	}
# endif

#ifdef STANDALONE
	gltrackball_rotate (pp->trackball);
#endif

	glRotatef(pp->step * 100, 1, 0, 0);
	glRotatef(pp->step * 95, 0, 1, 0);
	glRotatef(pp->step * 90, 0, 0, 1);
	draw_tetra(mi);
	if (MI_IS_FPS(mi)) {
		do_fps (mi);
	}
	glXSwapBuffers(display, window);
	if (pp->action == ACTION_SHUFFLE) {
		if (pp->done) {
			if (++pp->rotateStep > DELAY_AFTER_SHUFFLING) {
				pp->movement.face = NO_FACE;
				pp->rotateStep = 0;
				pp->action = ACTION_SOLVE;
				pp->done = 0;
			}
		} else {
			if (pp->movement.face == NO_FACE) {
				if (pp->shufflingmoves < pp->storedmoves) {
					pp->rotateStep = 0;
					pp->movement = pp->moves[pp->shufflingmoves];
				} else {
					pp->rotateStep = 0;
					pp->done = 1;
				}
			} else {
				if (pp->rotateStep == 0) {
					;
				}
				pp->rotateStep += pp->anglestep;
				if (pp->rotateStep > pp->degreeTurn) {
					evalmovement(mi, pp->movement);
					pp->shufflingmoves++;
					pp->movement.face = NO_FACE;
				}
			}
		}
	} else {
		if (pp->done) {
			if (++pp->rotateStep > DELAY_AFTER_SOLVING)
				if (!shuffle(mi)) {
					free_pyraminx_screen(pp);
					if (MI_IS_VERBOSE(mi)) {
						 (void) fprintf(stderr,
							"Could not allocate memory for pyraminx\n");
					}
				}
		} else {
			if (pp->movement.face == NO_FACE) {
				if (pp->storedmoves > 0) {
					pp->rotateStep = 0;
					pp->movement = pp->moves[pp->storedmoves - 1];
					pp->movement.direction = (pp->movement.direction +
						(MAX_ORIENT / 2)) % MAX_ORIENT;
				} else {
					pp->rotateStep = 0;
					pp->done = 1;
				}
			} else {
				if (pp->rotateStep == 0) {
					;
				}
				pp->rotateStep += pp->anglestep;
				if (pp->rotateStep > pp->degreeTurn) {
					evalmovement(mi, pp->movement);
					pp->storedmoves--;
					pp->movement.face = NO_FACE;
				}
			}
		}
	}

	glPopMatrix();

	glFlush();

	pp->step += 0.05;
}

#ifndef STANDALONE
ENTRYPOINT void
change_pyraminx(ModeInfo * mi)
{
	pyraminxstruct *pp;

	if (pyraminx == NULL)
		return;
	pp = &pyraminx[MI_SCREEN(mi)];

	if (!pp->glx_context)
		return;
	if (!pinit(mi)) {
		free_pyraminx_screen(pp);
		if (MI_IS_VERBOSE(mi)) {
			 (void) fprintf(stderr,
				"Could not allocate memory for pyraminx\n");
		}
	}
}
#endif

XSCREENSAVER_MODULE ("Pyraminx", pyraminx)

#endif /* MODE_pyraminx */
