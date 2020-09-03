
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)life.c	4.00 97/01/01 xlockmore";

#endif

/*-
 * life.c - Conway's game of Life for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@bigfoot.com>
 * 07-Jan-95: life now has a random soup pattern.
 * 07-Dec-94: life now has new organisms.  They are now better centered.
 *            Some of the nonperiodic forms were removed. New life forms
 *            were taken from xlife (an AMAZING collection of life forms).
 *            life's gliders now come from the edge of the screen except
 *            when generated by a life form.
 * 23-Nov-94: Bug fix for different iconified window sizes  
 * 21-Jul-94: Took out bzero & bcopy since memset & memcpy is more portable
 * 10-Jun-94: Changed name of function 'kill', which is a libc function on
 *            many systems from Victor Langeveld <vic@mbfys.kun.nl>
 *
 * Changes of Patrick J. Naughton
 * 24-May-91: Added wraparound code from johnson@bugs.comm.mot.com.
 *	      Made old cells stay blue.
 *	      Made batchcount control the number of generations until restart.
 * 29-Jul-90: support for multiple screens.
 * 07-Feb-90: remove bogus semi-colon after #include line.
 * 15-Dec-89: Fix for proper skipping of {White,Black}Pixel() in colors.
 * 08-Oct-89: Moved seconds() to an extern.
 * 20-Sep-89: Written (life algorithm courtesy of Jim Graham, flar@sun.com).
 */

#include "xlock.h"

/* aliases for vars defined in the bitmap file */
#define CELL_WIDTH   image_width
#define CELL_HEIGHT    image_height
#define CELL_BITS    image_bits

#include "life.xbm"

#define DEF_RULE  "2333"

static int  rule;

static XrmOptionDescRec opts[] =
{
	{"-rule", ".life.rule", XrmoptionSepArg, (caddr_t) NULL},
};
static argtype vars[] =
{
	{(caddr_t *) & rule, "rule", "Rule", DEF_RULE, t_Int}
};
static OptionStruct desc[] =
{
   {"-rule num", "<living_min><living_max><birth_min><birth_max> parameters"}
};

ModeSpecOpt life_opts =
{1, opts, 1, vars, desc};

static XImage logo =
{
	0, 0,			/* width, height */
	0, XYBitmap, 0,		/* xoffset, format, data */
	LSBFirst, 8,		/* byte-order, bitmap-unit */
	LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};

#define BORDER 2
#define MINGRIDSIZE 20
#define MINSIZE 1

typedef struct {
	int         pattern;
	int         pixelmode;
	int         xs, ys, xb, yb;	/* cell size, grid border */
	int         generation;
	int         nrows;
	int         ncols;
	int         width;
	int         height;
	unsigned char *buffer;
	unsigned char *tempbuf;
	unsigned char *lastbuf;
	unsigned char *agebuf;
} lifestruct;

static int  living_min, living_max, birth_min, birth_max;

static lifestruct *lifes = NULL;

/* 
 * Buffer stores the data for each cell. Each cell is stored as 8 bits
 * representing the presence of a critter in each of it's surrounding 8
 * cells. There is an empty row and column around the whole array to allow
 * stores without bounds checking as well as an extra row at the end for the 
 * fetches into tempbuf.  Tempbuf stores the data for the next two rows so
 * that we know the state of those critter before he was modified by the
 * fate of the critters that have already been processed.  Agebuf stores
 * the age of each critter.
 */

#define	UPLT	0x01
#define UP	0x02
#define UPRT	0x04
#define LT	0x08
#define RT	0x10
#define DNLT	0x20
#define DN	0x40
#define DNRT	0x80

/* 
 * Fates is a lookup table for the fate of a critter. The 256 * entries
 * represent the 256 possible combinations of the 8 * neighbor cells.
 */
#define NUMPTS  63
static unsigned char birth[256];
static unsigned char living[256];

#define LIFE_2333 0

/* 
 * Patterns have < NUMPTS pts (and should have a size of <= 32x32,
 * the Gun is an exception)
 */
static int  patterns_2333[][2 * NUMPTS + 1] =
{
	{			/* GLIDER GUN */
		6, -4,
		5, -3, 6, -3,
		-6, -2, -5, -2, 8, -2, 9, -2, 16, -2,
		-7, -1, 8, -1, 9, -1, 10, -1, 16, -1, 17, -1,
		-18, 0, -17, 0, -8, 0, 8, 0, 9, 1,
		-17, 1, -8, 1, 5, 1, 6, 1,
		-8, 2, 6, 2,
		-7, 3,
		-6, 4, -5, 4,
		127
	},
	{			/* SHOWER TUB (PART OF OSCILATORS) */
		-3, -6, -2, -6, 2, -6, 3, -6,
		-4, -5, -2, -5, 2, -5, 4, -5,
		-4, -4, 4, -4,
		-7, -3, -6, -3, -4, -3, -3, -3, 3, -3, 4, -3, 6, -3, 7, -3,
		-7, -2, -6, -2, -4, -2, 0, -2, 4, -2, 6, -2, 7, -2,
		-4, -1, -2, -1, 2, -1, 4, -1,
		-4, 0, -2, 0, 2, 0, 4, 0,
		-5, 1, -4, 1, -2, 1, 2, 1, 4, 1, 5, 1,
		-4, 2, -1, 2, 0, 2, 1, 2, 4, 2,
		-4, 3, 4, 3, 6, 3,
		-3, 4, -2, 4, -1, 4, 5, 4, 6, 4,
		-1, 5,
		127
	},
	{			/* P4 LARGE TOASTER */
		-5, -3, -4, -3, -2, -3, 2, -3, 4, -3, 5, -3,
		-5, -2, 5, -2,
		-4, -1, -3, -1, 3, -1, 4, -1,
       -7, 0, -6, 0, -5, 0, -2, 0, -1, 0, 0, 0, 1, 0, 2, 0, 5, 0, 6, 0, 7, 0,
		-7, 1, -4, 1, 4, 1, 7, 1,
		-6, 2, -5, 2, 5, 2, 6, 2,
		127
	},
	{			/* STARGATE REPEATER P3 */
		0, -6,
		-1, -5, 1, -5,
		-2, -4, 0, -4, 2, -4,
		-2, -3, 2, -3,
		-4, -2, -3, -2, 0, -2, 3, -2, 4, -2,
		-5, -1, 0, -1, 5, -1,
		-6, 0, -4, 0, -2, 0, -1, 0, 1, 0, 2, 0, 4, 0, 6, 0,
		-5, 1, 0, 1, 5, 1,
		-4, 2, -3, 2, 0, 2, 3, 2, 4, 2,
		-2, 3, 2, 3,
		-2, 4, 0, 4, 2, 4,
		-1, 5, 1, 5,
		0, 6,
		127
	},
	{			/* OSCILLATOR 7 (P8) */
		-4, -2, -3, -2, -2, -2, -1, -2, 4, -2, 5, -2, 6, -2, 7, -2,
		-9, -1, -8, -1, 0, -1, 8, -1,
		-9, 0, -8, 0, -5, 0, -4, 0, 0, 0, 3, 0, 4, 0, 8, 0,
		-5, 1, -4, 1, -1, 1, 3, 1, 4, 1, 7, 1,
		127
	},
	{			/* P144 */
		-14, -9, -13, -9, 12, -9, 13, -9,
		-14, -8, -13, -8, 12, -8, 13, -8,
		4, -7, 5, -7,
		3, -6, 6, -6,
		4, -5, 5, -5,
		-1, -3, 0, -3, 1, -3,
		-1, -2, 1, -2,
		-1, -1, 0, -1, 1, -1,
		-1, 0, 0, 0,
		-2, 1, -1, 1, 0, 1,
		-2, 2, 0, 2,
		-2, 3, -1, 3, 0, 3,
		-6, 5, -5, 5,
		-7, 6, -4, 6,
		-6, 7, -5, 7,
		-14, 8, -13, 8, 12, 8, 13, 8,
		-14, 9, -13, 9, 12, 9, 13, 9,
		127
	},
	{			/* FIGURE EIGHT */
		-3, -3, -2, -3, -1, -3,
		-3, -2, -2, -2, -1, -2,
		-3, -1, -2, -1, -1, -1,
		0, 0, 1, 0, 2, 0,
		0, 1, 1, 1, 2, 1,
		0, 2, 1, 2, 2, 2,
		127
	},
	{			/* PULSAR 18-22-20 */
		0, -4, 1, -4, 2, -4,
		-1, -2, 4, -2,
		-2, -1, 0, -1, 4, -1,
		-4, 0, -3, 0, -2, 0, 1, 0, 4, 0,
		2, 1,
		0, 2, 1, 2,
		0, 3,
		0, 4,
		127
	},
	{			/* PULSAR 48-56-72 */
		-2, -1, -1, -1, 0, -1, 1, -1, 2, -1,
		-2, 0, 2, 0,
		127
	},
	{			/* BARBER POLE P2 */
		-6, -6, -5, -6,
		-6, -5, -4, -5,
		-4, -3, -2, -3,
		-2, -1, 0, -1,
		0, 1, 2, 1,
		2, 3, 4, 3,
		5, 4,
		4, 5, 5, 5,
		127
	},
	{			/* ACHIM P5 */
		-6, -6, -5, -6,
		-6, -5,
		-4, -4,
		-4, -3, -2, -3,
		-2, -1, 0, -1,
		0, 1, 2, 1,
		2, 3, 3, 3,
		5, 4,
		4, 5, 5, 5,
		127
	},
	{			/* HERTZ P4 */
		-2, -5, -1, -5,
		-2, -4, -1, -4,
		-7, -2, -6, -2, -2, -2, -1, -2, 0, -2, 1, -2, 5, -2, 6, -2,
		-7, -1, -5, -1, -3, -1, 2, -1, 4, -1, 6, -1,
		-5, 0, -3, 0, -2, 0, 2, 0, 4, 0,
		-6, 1, -5, 1, -3, 1, 2, 1, 4, 1, 5, 1,
		-2, 2, -1, 2, 0, 2, 1, 2,
		-2, 4, -1, 4,
		-2, 5, -1, 5,
		127
	},
	{			/* PUMP (TUMBLER, P14) */
		-2, -3, -1, -3, 1, -3, 2, -3,
		-2, -2, -1, -2, 1, -2, 2, -2,
		-1, -1, 1, -1,
		-3, 0, -1, 0, 1, 0, 3, 0,
		-3, 1, -1, 1, 1, 1, 3, 1,
		-3, 2, -2, 2, 2, 2, 3, 2,
		127
	},
	{			/* PULSE1 P4 */
		0, -3, 1, -3,
		-2, -2, 0, -2,
		-3, -1, 3, -1,
		-2, 0, 2, 0, 3, 0,
		0, 2, 2, 2,
		1, 3,
		127
	},
	{			/* SHINING FLOWER P5 */
		-1, -4, 0, -4,
		-2, -3, 1, -3,
		-3, -2, 2, -2,
		-4, -1, 3, -1,
		-4, 0, 3, 0,
		-3, 1, 2, 1,
		-2, 2, 1, 2,
		-1, 3, 0, 3,
		127
	},
	{			/* PULSE2 P6 */
		0, -4, 1, -4,
		-4, -3, -3, -3, -1, -3,
		-4, -2, -3, -2, 0, -2, 3, -2,
		1, -1, 3, -1,
		2, 0,
		1, 2, 2, 2,
		1, 3, 2, 3,
		127
	},
	{			/* PINWHEEL P4 */
		-2, -6, -1, -6,
		-2, -5, -1, -5,
		-2, -3, -1, -3, 0, -3, 1, -3,
		-3, -2, -1, -2, 2, -2, 4, -2, 5, -2,
		-3, -1, 1, -1, 2, -1, 4, -1, 5, -1,
		-6, 0, -5, 0, -3, 0, 0, 0, 2, 0,
		-6, 1, -5, 1, -3, 1, 2, 1,
		-2, 2, -1, 2, 0, 2, 1, 2,
		0, 4, 1, 4,
		0, 5, 1, 5,
		127
	},
	{			/* CLOCK P4 */
		-2, -6, -1, -6,
		-2, -5, -1, -5,
		-2, -3, -1, -3, 0, -3, 1, -3,
		-6, -2, -5, -2, -3, -2, 0, -2, 2, -2,
		-6, -1, -5, -1, -3, -1, -1, -1, 2, -1,
		-3, 0, -1, 0, 2, 0, 4, 0, 5, 0,
		-3, 1, 2, 1, 4, 1, 5, 1,
		-2, 2, -1, 2, 0, 2, 1, 2,
		0, 4, 1, 4,
		0, 5, 1, 5,
		127
	},
	{			/* CROSS P3 */
		-2, -4, -1, -4, 0, -4, 1, -4,
		-2, -3, 1, -3,
		-4, -2, -3, -2, -2, -2, 1, -2, 2, -2, 3, -2,
		-4, -1, 3, -1,
		-4, 0, 3, 0,
		-4, 1, -3, 1, -2, 1, 1, 1, 2, 1, 3, 1,
		-2, 2, 1, 2,
		-2, 3, -1, 3, 0, 3, 1, 3,
		127
	},
	{			/* BIG CROSS P3 */
		0, -5,
		-1, -4, 0, -4, 1, -4,
		-3, -3, -2, -3, -1, -3, 1, -3, 2, -3, 3, -3,
		-3, -2, 3, -2,
		-4, -1, -3, -1, 3, -1, 4, -1,
		-5, 0, -4, 0, 4, 0, 5, 0,
		-4, 1, -3, 1, 3, 1, 4, 1,
		-3, 2, 3, 2,
		-3, 3, -2, 3, -1, 3, 1, 3, 2, 3, 3, 3,
		-1, 4, 0, 4, 1, 4,
		0, 5,
		127
	},
	{			/* P4 DIAG SYM */
		-2, -4, 0, -4,
		-2, -3, 0, -3, 2, -3, 3, -3,
		-4, -2, -3, -2, 2, -2,
		0, -1, 1, -1, 2, -1,
		-4, 0, -3, 0, -1, 0, 2, 0,
		-1, 1, 2, 1,
		-3, 2, -2, 2, -1, 2, 0, 2, 1, 2,
		-3, 3,
		127
	},
	{			/* P4 ASYM */
		-4, -4, -2, -4,
		-4, -3, -1, -3,
		-1, -2,
		-2, -1, -1, -1, 0, -1, 3, -1, 4, -1, 5, -1,
		-5, 0, -4, 0, -3, 0, 0, 0, 1, 0, 2, 0,
		1, 1,
		1, 2, 4, 2,
		2, 3, 4, 3,
		127
	},
	{			/* P4 ASYM 2 */
		-3, -3, -1, -3, 2, -3, 4, -3, 5, -3, 6, -3,
		-4, -2, -1, -2, 1, -2, 3, -2, 5, -2,
		-4, -1,
		3, 0,
		-6, 1, -4, 1, -2, 1, 0, 1, 3, 1,
		-7, 2, -6, 2, -5, 2, -3, 2, 0, 2, 2, 2,
		127
	},
	{			/* P4 SYM */
		-6, -2, -5, -2, 4, -2, 5, -2,
		-6, -1, -5, -1, -3, -1, -2, -1, 1, -1, 2, -1, 4, -1, 5, -1,
		-5, 0, -2, 0, 1, 0, 4, 0,
		-5, 1, -4, 1, -2, 1, -1, 1, 0, 1, 1, 1, 3, 1, 4, 1,
		127
	},
	{			/* WHIRLY THING P12 */
		-5, -6,
		-5, -5, -4, -5, -3, -5, 5, -5, 6, -5,
		-2, -4, 5, -4,
		-3, -3, -2, -3, 3, -3, 5, -3,
		3, -2, 4, -2,
		0, -1, 1, -1,
		0, 0, 1, 0,
		0, 1, 1, 1,
		-4, 2, -3, 2,
		-5, 3, -3, 3, 2, 3, 3, 3,
		-5, 4, 2, 4,
		-6, 5, -5, 5, 3, 5, 4, 5, 5, 5,
		5, 6,
		127
	},
	{			/* PENTADECATHOLON P15 */
	     -5, 0, -4, 0, -3, 0, -2, 0, -1, 0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0,
		127
	},
	{			/* BALLOON P5 */
		-1, -3, 0, -3,
		-3, -2, 2, -2,
		-3, -1, 2, -1,
		-3, 0, 2, 0,
		-2, 1, 1, 1,
		-4, 2, -2, 2, 1, 2, 3, 2,
		-4, 3, -3, 3, 2, 3, 3, 3,
		127
	},
	{			/* FENCEPOST P12 */
		-11, -3, -9, -3, -7, -3,
	 -11, -2, -9, -2, -7, -2, 5, -2, 6, -2, 7, -2, 9, -2, 10, -2, 11, -2,
		-11, -1, -7, -1, -3, -1, 1, -1, 8, -1,
	  -10, 0, -8, 0, -3, 0, -2, 0, -1, 0, 1, 0, 5, 0, 6, 0, 10, 0, 11, 0,
		-11, 1, -7, 1, -3, 1, 1, 1, 8, 1,
		-11, 2, -9, 2, -7, 2, 5, 2, 6, 2, 7, 2, 9, 2, 10, 2, 11, 2,
		-11, 3, -9, 3, -7, 3,
		127
	},
	{			/* PISTON (SHUTTLE) P30 */
		1, -3, 2, -3,
		0, -2,
		-10, -1, -1, -1,
		-11, 0, -10, 0, -1, 0, 9, 0, 10, 0,
		-1, 1, 9, 1,
		0, 2,
		1, 3, 2, 3,
		127
	},
	{			/* P30 */
		-8, -5, 7, -5,
		-9, -4, -7, -4, 1, -4, 2, -4, 6, -4, 8, -4,
		-8, -3, 0, -3, 1, -3, 2, -3, 7, -3,
		1, -2, 2, -2,
		1, 2, 2, 2,
		-8, 3, 0, 3, 1, 3, 2, 3, 7, 3,
		-9, 4, -7, 4, 1, 4, 2, 4, 6, 4, 8, 4,
		-8, 5, 7, 5,
		127
	},
	{			/* PISTON2 P46 */
		-3, -5,
		-14, -4, -13, -4, -4, -4, -3, -4, 13, -4, 14, -4,
		-14, -3, -13, -3, -5, -3, -4, -3, 13, -3, 14, -3,
		-4, -2, -3, -2, 0, -2, 1, -2,
		-4, 2, -3, 2, 0, 2, 1, 2,
		-14, 3, -13, 3, -5, 3, -4, 3, 13, 3, 14, 3,
		-14, 4, -13, 4, -4, 4, -3, 4, 13, 4, 14, 4,
		-3, 5,
		127
	},
	{			/* GEARS (gear, flywheel, blinker) P2 */
		-1, -4,
		-1, -3, 1, -3,
		-3, -2,
		2, -1, 3, -1,
		-4, 0, -3, 0,
		2, 1,
		-2, 2, 0, 2,
		0, 3,

		5, 3,
		3, 4, 4, 4,
		5, 5, 6, 5,
		4, 6,

		8, 0,
		8, 1,
		8, 2,
		127
	},
	{			/* TURBINE8, KOK'S GALAXY */
		-4, -4, -3, -4, -2, -4, -1, -4, 0, -4, 1, -4, 3, -4, 4, -4,
		-4, -3, -3, -3, -2, -3, -1, -3, 0, -3, 1, -3, 3, -3, 4, -3,
		3, -2, 4, -2,
		-4, -1, -3, -1, 3, -1, 4, -1,
		-4, 0, -3, 0, 3, 0, 4, 0,
		-4, 1, -3, 1, 3, 1, 4, 1,
		-4, 2, -3, 2,
		-4, 3, -3, 3, -1, 3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3,
		-4, 4, -3, 4, -1, 4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4,
		127
	},
	{			/* P16 */
		-3, -6, 1, -6, 2, -6,
		-3, -5, 0, -5, 3, -5,
		3, -4,
		-5, -3, -4, -3, 1, -3, 2, -3, 5, -3, 6, -3,
		-6, -2, -3, -2,
		-6, -1, -3, -1,
		-5, 0, 5, 0,
		3, 1, 6, 1,
		3, 2, 6, 2,
		-6, 3, -5, 3, -2, 3, -1, 3, 4, 3, 5, 3,
		-3, 4,
		-3, 5, 0, 5, 3, 5,
		-2, 6, -1, 6, 3, 6,
		127
	},
	{			/* P28 (FLUTTER) */
		-9, -7, -7, -7, 7, -7, 9, -7,
		-6, -6, 6, -6,
		-10, -5, -7, -5, 7, -5, 10, -5,
		-11, -4, -9, -4, -7, -4, 7, -4, 9, -4, 11, -4,
		-11, -3, -8, -3, 8, -3, 11, -3,
		-10, -2, -9, -2, -4, -2, -3, -2, -2, -2,
		2, -2, 3, -2, 4, -2, 9, -2, 10, -2,
		-3, -1, 3, -1,
		-10, 1, -9, 1, 9, 1, 10, 1,
		-11, 2, -8, 2, 8, 2, 11, 2,
		-11, 3, -9, 3, -6, 3, 6, 3, 9, 3, 11, 3,
		-10, 4, 10, 4,
		-9, 5, -8, 5, -6, 5, 6, 5, 8, 5, 9, 5,
		-7, 6, 7, 6,
		127
	},
	{			/* P54 (PISTON3) */
		-14, -8, -13, -8, 13, -8, 14, -8,
		-13, -7, 13, -7,
		-13, -6, -11, -6, 11, -6, 13, -6,
		-12, -5, -11, -5, -1, -5, 11, -5, 12, -5,
		0, -4,
		-6, -3, -5, -3, 1, -3,
		-6, -2, -5, -2, -2, -2, 0, -2,
		-1, -1,
		-1, 1,
		-6, 2, -5, 2, -2, 2, 0, 2,
		-6, 3, -5, 3, 1, 3,
		0, 4,
		-12, 5, -11, 5, -1, 5, 11, 5, 12, 5,
		-13, 6, -11, 6, 11, 6, 13, 6,
		-13, 7, 13, 7,
		-14, 8, -13, 8, 13, 8, 14, 8,
		127
	},
	{			/* SWITCH ENGINE */
		-12, -3, -10, -3,
		-13, -2,
		-12, -1, -9, -1,
		-10, 0, -9, 0, -8, 0,
		13, 2, 14, 2,
		13, 3,
		127
	},
	{			/* PUFFER TRAIN */
		1, -9,
		2, -8,
		-2, -7, 2, -7,
		-1, -6, 0, -6, 1, -6, 2, -6,
		-2, -2,
		-1, -1, 0, -1,
		0, 0,
		0, 1,
		-1, 2,
		1, 5,
		2, 6,
		-2, 7, 2, 7,
		-1, 8, 0, 8, 1, 8, 2, 8,
		127
	},
	{			/* SCHOOL OF FISH (ESCORT) */
		3, -8,
		4, -7,
		-2, -6, 4, -6,
		-1, -5, 0, -5, 1, -5, 2, -5, 3, -5, 4, -5,
		-5, -1, -4, -1, -3, -1, -2, -1, -1, -1, 0, -1,
		1, -1, 2, -1, 3, -1, 4, -1, 5, -1, 6, -1,
		-6, 0, 6, 0,
		6, 1,
		5, 2,
		3, 4,
		4, 5,
		-2, 6, 4, 6,
		-1, 7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7,

		127
	},
	{			/* DART SPEED 1/3 */
		3, -7,
		2, -6, 4, -6,
		1, -5, 2, -5,
		4, -4,
		0, -3, 4, -3,
		-3, -2, 0, -2,
		-4, -1, -2, -1, 1, -1, 2, -1, 3, -1, 4, -1,
		-5, 0, -2, 0,
		-4, 1, -2, 1, 1, 1, 2, 1, 3, 1, 4, 1,
		-3, 2, 0, 2,
		0, 3, 4, 3,
		4, 4,
		1, 5, 2, 5,
		2, 6, 4, 6,
		3, 7,
		127
	},
	{			/* PERIOD 4 SPEED 1/2 */
		-3, -5,
		-4, -4, -3, -4, -2, -4, -1, -4, 0, -4,
		-5, -3, -4, -3, 0, -3, 1, -3, 3, -3,
		-4, -2, 4, -2,
		-3, -1, -2, -1, 1, -1, 3, -1,
		-3, 1, -2, 1, 1, 1, 3, 1,
		-4, 2, 4, 2,
		-5, 3, -4, 3, 0, 3, 1, 3, 3, 3,
		-4, 4, -3, 4, -2, 4, -1, 4, 0, 4,
		-3, 5,
		127
	},
	{			/* ANOTHER PERIOD 4 SPEED 1/2 */
		-4, -7, -3, -7, -1, -7, 0, -7, 1, -7, 2, -7, 3, -7, 4, -7,
		-5, -6, -4, -6, -3, -6, -2, -6, 5, -6,
		-6, -5, -5, -5,
		-5, -4, 5, -4,
		-4, -3, -3, -3, -2, -3, 0, -3,
		-2, -2,
		-2, -1,
		-1, 0,
		-2, 1,
		-2, 2,
		-4, 3, -3, 3, -2, 3, 0, 3,
		-5, 4, 5, 4,
		-6, 5, -5, 5,
		-5, 6, -4, 6, -3, 6, -2, 6, 5, 6,
		-4, 7, -3, 7, -1, 7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7,
		127
	},
	{			/* SMALLEST KNOWN PERIOD 3 SPACESHIP SPEED 1/3 */
		0, -8,
		-1, -7, 1, -7,
		-1, -6, 1, -6,
		-1, -5,
		-2, -3, -1, -3,
		-1, -2, 1, -2,
		-2, -1, 0, -1,
		-2, 0, -1, 0, 0, 0,
		-1, 2, 1, 2,
		-1, 3, 0, 3,
		0, 4,
		0, 5, 2, 5,
		0, 6, 2, 6,
		1, 7,
		127
	},
	{			/* TURTLE SPEED 1/3 */
		-4, -5, -3, -5, -2, -5, 6, -5,
		-4, -4, -3, -4, 0, -4, 2, -4, 3, -4, 5, -4, 6, -4,
		-2, -3, -1, -3, 0, -3, 5, -3,
		-4, -2, -1, -2, 1, -2, 5, -2,
		-5, -1, 0, -1, 5, -1,
		-5, 0, 0, 0, 5, 0,
		-4, 1, -1, 1, 1, 1, 5, 1,
		-2, 2, -1, 2, 0, 2, 5, 2,
		-4, 3, -3, 3, 0, 3, 2, 3, 3, 3, 5, 3, 6, 3,
		-4, 4, -3, 4, -2, 4, 6, 4,
		127
	},
	{			/* SMALLEST KNOWN PERIOD 5 SPEED 2/5 */
		1, -7, 3, -7,
		-2, -6, 3, -6,
		-3, -5, -2, -5, -1, -5, 4, -5,
		-4, -4, -2, -4,
		-5, -3, -4, -3, -1, -3, 0, -3, 5, -3,
		-4, -2, -3, -2, 0, -2, 1, -2, 2, -2, 3, -2, 4, -2,
		-4, 2, -3, 2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2,
		-5, 3, -4, 3, -1, 3, 0, 3, 5, 3,
		-4, 4, -2, 4,
		-3, 5, -2, 5, -1, 5, 4, 5,
		-2, 6, 3, 6,
		1, 7, 3, 7,
		127
	},
	{			/* SYM PUFFER */
		1, -4, 2, -4, 3, -4, 4, -4,
		0, -3, 4, -3,
		4, -2,
		-4, -1, -3, -1, 0, -1, 3, -1,
		-4, 0, -3, 0, -2, 0,
		-4, 1, -3, 1, 0, 1, 3, 1,
		4, 2,
		0, 3, 4, 3,
		1, 4, 2, 4, 3, 4, 4, 4,
		127
	},
	{			/* RAKE P20 BACKWARDS */
		0, -10, 1, -10, 10, -10,
		-1, -9, 0, -9, 1, -9, 2, -9, 11, -9,
		-1, -8, 0, -8, 2, -8, 3, -8, 7, -8, 11, -8,
		1, -7, 2, -7, 8, -7, 9, -7, 10, -7, 11, -7,
		6, -3, 7, -3,
		5, -2, 6, -2, 8, -2, 9, -2,
		6, -1, 9, -1,
		6, 0, 9, 0,
		7, 1, 8, 1,
		10, 4,
		11, 5,
		-8, 6, 7, 6, 11, 6,
		-7, 7, 8, 7, 9, 7, 10, 7, 11, 7,
		-11, 8, -7, 8,
		-10, 9, -9, 9, -8, 9, -7, 9,
		127
	},
	{			/* RAKE P20 FORWARDS */
		0, -10, 1, -10, 10, -10,
		-1, -9, 0, -9, 1, -9, 2, -9, 11, -9,
		-1, -8, 0, -8, 2, -8, 3, -8, 7, -8, 11, -8,
		1, -7, 2, -7, 8, -7, 9, -7, 10, -7, 11, -7,
		6, -3, 7, -3,
		5, -2, 6, -2, 8, -2, 9, -2,
		6, -1, 9, -1,
		6, 0, 9, 0,
		7, 1, 8, 1,
		10, 4,
		11, 5,
		7, 6, 11, 6,
		-9, 7, -8, 7, -7, 7, -6, 7, 8, 7, 9, 7, 10, 7, 11, 7,
		-10, 8, -6, 8,
		-6, 9,
		-7, 10,
		127
	},
	{			/* RAKE P24 BACKWARDS */
		-5, -10,
		-4, -9,
		-10, -8, -4, -8,
		-9, -7, -8, -7, -7, -7, -6, -7, -5, -7, -4, -7,
		6, -6, 7, -6,
		5, -5, 6, -5, 7, -5, 8, -5,
		5, -4, 6, -4, 8, -4, 9, -4,
		7, -3, 8, -3,
		0, -2, 2, -2,
		-1, -1, 2, -1, 3, -1,
		0, 0, 2, 0,
		7, 1, 8, 1,
		5, 2, 6, 2, 8, 2, 9, 2,
		5, 3, 6, 3, 7, 3, 8, 3,
		6, 4, 7, 4,
		-5, 6, -4, 6,
		-9, 7, -8, 7, -7, 7, -6, 7, -4, 7, -3, 7,
		-9, 8, -8, 8, -7, 8, -6, 8, -5, 8, -4, 8,
		-8, 9, -7, 9, -6, 9, -5, 9,
		127
	},
	{			/* BIG GLIDER 1 */
		-4, -7, -3, -7,
		-4, -6, -2, -6,
		-4, -5,
		-7, -4, -6, -4, -4, -4,
		-7, -3, -2, -3,
		-7, -2, -5, -2, -4, -2, 3, -2, 4, -2, 5, -2,
		-2, -1, -1, -1, 0, -1, 5, -1, 6, -1,
		-1, 0, 0, 0, 1, 0, 3, 0, 5, 0,
		6, 1,
		-1, 2, 1, 2,
		-2, 3, -1, 3, 1, 3,
		-1, 4,
		-3, 5, -2, 5, 0, 5,
		0, 6,
		-2, 7, -1, 7,
		127
	},
	{			/* BIG GLIDER 2 */
		0, -9, 1, -9,
		0, -8, 2, -8,
		0, -7,
		1, -6, 2, -6, 3, -6,
		1, -5, 3, -5, 4, -5, 5, -5,
		1, -4, 4, -4, 5, -4,
		3, -3, 5, -3, 7, -3, 8, -3,
		2, -2, 3, -2, 5, -2, 7, -2,
		1, -1, 2, -1, 7, -1,
		-9, 0, -8, 0, -7, 0,
		-9, 1, -6, 1, -5, 1, -4, 1, -1, 1, 3, 1, 4, 1,
		-8, 2, -6, 2, -2, 2, -1, 2,
		-6, 3, -5, 3, -3, 3, -2, 3, 1, 3,
		-5, 4, -4, 4, 1, 4,
		-5, 5, -4, 5, -3, 5, -2, 5,
		-3, 7, -2, 7, -1, 7,
		-3, 8,
		127
	},
	{			/* BIG GLIDER 3 */
		-1, -8,
		-2, -7, -1, -7,
		-2, -6, 0, -6,
		0, -4, 2, -4,
		-2, -3, 0, -3, 2, -3,
		-7, -2, -3, -2, -2, -2, 0, -2, 2, -2, 3, -2,
		-8, -1, -7, -1, -6, -1, -5, -1, -2, -1, 4, -1,
		-8, 0, -5, 0, -4, 0, -2, 0, -1, 0, 3, 0, 4, 0,
		-6, 1, -5, 1, 0, 1,
		-4, 2, -3, 2, 1, 2,
		-3, 3, -2, 3, -1, 3, 2, 3, 4, 3, 5, 3, 6, 3, 7, 3,
		-3, 4, 0, 4, 3, 4, 4, 4, 5, 4,
		-1, 5, 0, 5, 1, 5,
		1, 6, 2, 6, 3, 6,
		2, 7, 3, 7,
		127
	},
	{			/* PI HEPTOMINO (], NEAR SHIP) */
		-2, -1, -1, -1, 0, -1,
		1, 0,
		-2, 1, -1, 1, 0, 1,
		127
	},
	{			/* R PENTOMINO */
		0, -1, 1, -1,
		-1, 0, 0, 0,
		0, 1,
		127
	},
	{			/* BUNNIES */
		-4, -2, 2, -2,
		-2, -1, 2, -1,
		-2, 0, 1, 0, 3, 0,
		-3, 1, -1, 1,
		127
	}
};

#define NPATS_2333	(sizeof patterns_2333 / sizeof patterns_2333[0])


static void
drawcell(ModeInfo * mi, int row, int col)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	lifestruct *lp = &lifes[MI_SCREEN(mi)];

	XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	if (MI_NPIXELS(mi) > 2) {
		unsigned char *loc = lp->buffer + ((row + 1) * (lp->ncols + BORDER)) +
		col + 1;
		unsigned char *ageptr = lp->agebuf + (loc - lp->buffer);
		unsigned char age = *ageptr;

		/* if we aren't up to blue yet, then keep aging the cell. */
		if (age < (unsigned char) (MI_NPIXELS(mi) * 0.7))
			++age;

		XSetForeground(display, gc, MI_PIXEL(mi, age));
		*ageptr = age;
	}
	if (lp->pixelmode)
		XFillRectangle(display, MI_WINDOW(mi), gc,
		lp->xb + lp->xs * col, lp->yb + lp->ys * row, lp->xs, lp->ys);
	else
		XPutImage(display, MI_WINDOW(mi), gc, &logo,
			  0, 0, lp->xb + lp->xs * col, lp->yb + lp->ys * row,
			  logo.width, logo.height);
}


static void
erasecell(ModeInfo * mi, int row, int col)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];

	XSetForeground(MI_DISPLAY(mi), MI_GC(mi), MI_WIN_BLACK_PIXEL(mi));
	XFillRectangle(MI_DISPLAY(mi), MI_WINDOW(mi), MI_GC(mi),
	       lp->xb + lp->xs * col, lp->yb + lp->ys * row, lp->xs, lp->ys);
}


static void
spawn(lifestruct * lp, unsigned char *loc)
{
	unsigned char *ulloc, *ucloc, *urloc, *clloc, *crloc, *llloc, *lcloc,
	           *lrloc, *arloc;
	int         off, row, col, lastrow;

	lastrow = (lp->nrows) * (lp->ncols + BORDER);
	off = loc - lp->buffer;
	col = off % (lp->ncols + BORDER);
	row = (off - col) / (lp->ncols + BORDER);
	ulloc = loc - lp->ncols - 3;
	ucloc = loc - lp->ncols - 2;
	urloc = loc - lp->ncols - 1;
	clloc = loc - 1;
	crloc = loc + 1;
	arloc = loc + 1;
	llloc = loc + lp->ncols + 1;
	lcloc = loc + lp->ncols + 2;
	lrloc = loc + lp->ncols + 3;
	if (row == 1) {
		ulloc += lastrow;
		ucloc += lastrow;
		urloc += lastrow;
	}
	if (row == lp->nrows) {
		llloc -= lastrow;
		lcloc -= lastrow;
		lrloc -= lastrow;
	}
	if (col == 1) {
		ulloc += lp->ncols;
		clloc += lp->ncols;
		llloc += lp->ncols;
	}
	if (col == lp->ncols) {
		urloc -= lp->ncols;
		crloc -= lp->ncols;
		lrloc -= lp->ncols;
	}
	*ulloc |= UPLT;
	*ucloc |= UP;
	*urloc |= UPRT;
	*clloc |= LT;
	*crloc |= RT;
	*arloc |= RT;
	*llloc |= DNLT;
	*lcloc |= DN;
	*lrloc |= DNRT;

	*(lp->agebuf + (loc - lp->buffer)) = 0;
}


static void
ckill(lifestruct * lp, unsigned char *loc)
{
	unsigned char *ulloc, *ucloc, *urloc, *clloc, *crloc, *llloc, *lcloc,
	           *lrloc, *arloc;
	int         off, row, col, lastrow;

	lastrow = (lp->nrows) * (lp->ncols + BORDER);
	off = loc - lp->buffer;
	row = off / (lp->ncols + BORDER);
	col = off % (lp->ncols + BORDER);
	row = (off - col) / (lp->ncols + BORDER);
	ulloc = loc - lp->ncols - 3;
	ucloc = loc - lp->ncols - 2;
	urloc = loc - lp->ncols - 1;
	clloc = loc - 1;
	crloc = loc + 1;
	arloc = loc + 1;
	llloc = loc + lp->ncols + 1;
	lcloc = loc + lp->ncols + 2;
	lrloc = loc + lp->ncols + 3;
	if (row == 1) {
		ulloc += lastrow;
		ucloc += lastrow;
		urloc += lastrow;
	}
	if (row == lp->nrows) {
		llloc -= lastrow;
		lcloc -= lastrow;
		lrloc -= lastrow;
	}
	if (col == 1) {
		ulloc += lp->ncols;
		clloc += lp->ncols;
		llloc += lp->ncols;
	}
	if (col == lp->ncols) {
		urloc -= lp->ncols;
		crloc -= lp->ncols;
		lrloc -= lp->ncols;
	}
	*ulloc &= ~UPLT;
	*ucloc &= ~UP;
	*urloc &= ~UPRT;
	*clloc &= ~LT;
	*crloc &= ~RT;
	*arloc &= ~RT;
	*llloc &= ~DNLT;
	*lcloc &= ~DN;
	*lrloc &= ~DNRT;
}


static void
setcell(ModeInfo * mi, int row, int col)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	unsigned char *loc;

	loc = lp->buffer + ((row + 1) * (lp->ncols + BORDER)) + col + 1;
	spawn(lp, loc);
	drawcell(mi, row, col);
}

static void
alloccells(lifestruct * lp)
{
	lp->buffer = (unsigned char *) calloc(((lp->ncols + BORDER) * lp->nrows) +
		       3 * (lp->ncols + BORDER) - 1, sizeof (unsigned char));
	lp->tempbuf = (unsigned char *) calloc(2 * lp->ncols, sizeof (unsigned char));
	lp->lastbuf = (unsigned char *) calloc(lp->ncols, sizeof (unsigned char));

	lp->agebuf = (unsigned char *) calloc((lp->ncols + BORDER) *
			       (lp->nrows + BORDER), sizeof (unsigned char));
}

static void
freecells(lifestruct * lp)
{
	if (lp->buffer != NULL)
		(void) free((void *) lp->buffer);
	if (lp->tempbuf != NULL)
		(void) free((void *) lp->tempbuf);
	if (lp->lastbuf != NULL)
		(void) free((void *) lp->lastbuf);
	if (lp->agebuf != NULL)
		(void) free((void *) lp->agebuf);
}

static void
init_fates(void)
{
	int         i, bits, neighbors;

	for (i = 0; i < 256; i++) {
		neighbors = 0;
		for (bits = i; bits; bits &= (bits - 1))
			neighbors++;
		living[i] = (neighbors >= living_min && neighbors <= living_max);
		birth[i] = (neighbors >= birth_min && neighbors <= birth_max);
	}
}

static void
RandomSoup(ModeInfo * mi, int n, int v)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	int         row, col;

	v /= 2;
	if (v < 1)
		v = 1;
	for (row = lp->nrows / 2 - v; row < lp->nrows / 2 + v; ++row)
		for (col = lp->ncols / 2 - v; col < lp->ncols / 2 + v; ++col)
			if (NRAND(100) < n && col >= 0 && row >= 0 &&
			    col < lp->ncols && row < lp->nrows)
				setcell(mi, row, col);
	if (MI_WIN_IS_VERBOSE(mi))
		(void) printf("random pattern\n");
}

static void
GetPattern(ModeInfo * mi, int i)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	int         row, col;
	int        *patptr;

	patptr = &patterns_2333[i][0];
	while ((col = *patptr++) != 127) {
		row = *patptr++;
		col += lp->ncols / 2;
		row += lp->nrows / 2;
		if (col >= 0 && row >= 0 && col < lp->ncols && row < lp->nrows)
			setcell(mi, row, col);
	}
	if (MI_WIN_IS_VERBOSE(mi))
		(void) printf("table number %d\n", i);
}

static void
shooter(ModeInfo * mi)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	int         hsp, vsp, hoff = 1, voff = 1;

	/* Generate the glider at the edge of the screen */
	if (LRAND() & 1) {
		hsp = (LRAND() & 1) ? 0 : lp->ncols - 1;
		vsp = NRAND(lp->nrows);
	} else {
		vsp = (LRAND() & 1) ? 0 : lp->nrows - 1;
		hsp = NRAND(lp->ncols);
	}
	if (vsp > lp->nrows / 2)
		voff = -1;
	if (hsp > lp->ncols / 2)
		hoff = -1;
	if (rule == 2333) {
		setcell(mi, vsp + 0 * voff, hsp + 2 * hoff);
		setcell(mi, vsp + 1 * voff, hsp + 2 * hoff);
		setcell(mi, vsp + 2 * voff, hsp + 2 * hoff);
		setcell(mi, vsp + 2 * voff, hsp + 1 * hoff);
		setcell(mi, vsp + 1 * voff, hsp + 0 * hoff);
	}
}

void
init_life(ModeInfo * mi)
{
	int         size = MI_SIZE(mi);
	lifestruct *lp;
	int         npats;

	if (lifes == NULL) {
		if ((lifes = (lifestruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (lifestruct))) == NULL)
			return;
	}
	lp = &lifes[MI_SCREEN(mi)];

	lp->generation = 0;

	if (!logo.data) {
		if (MI_WIN_IS_VERBOSE(mi))
			(void) printf("rule %d\n", rule);
		/*
		 * Rules should not have to be contiguous ...  that is, it should be
		 * possible to have rules where cells will live if there are 2 or 4
		 * (not 3) cells around them.   These rules are not general enough
		 * to allow for that.
		 */
		living_min = rule / 1000;
		living_max = (rule / 100) % 10;
		birth_min = (rule / 10) % 10;
		birth_max = rule % 10;
		init_fates();
		logo.data = (char *) CELL_BITS;
		logo.width = CELL_WIDTH;
		logo.height = CELL_HEIGHT;
		logo.bytes_per_line = (logo.width + 7) / 8;
	}
	lp->width = MI_WIN_WIDTH(mi);
	lp->height = MI_WIN_HEIGHT(mi);
	if (lp->width < 2)
		lp->width = 2;
	if (lp->height < 2)
		lp->height = 2;
	if (size == 0 ||
	 MINGRIDSIZE * size > lp->width || MINGRIDSIZE * size > lp->height) {
		if (lp->width > MINGRIDSIZE * logo.width &&
		    lp->height > MINGRIDSIZE * logo.height) {
			lp->pixelmode = False;
			lp->xs = logo.width;
			lp->ys = logo.height;
		} else {
			lp->pixelmode = True;
			lp->xs = lp->ys = MAX(MINSIZE, MIN(lp->width, lp->height) / MINGRIDSIZE);
		}
	} else {
		lp->pixelmode = True;
		if (size < -MINSIZE)
			lp->ys = NRAND(MIN(-size, MAX(MINSIZE, MIN(lp->width, lp->height) /
				      MINGRIDSIZE)) - MINSIZE + 1) + MINSIZE;
		else if (size < MINSIZE)
			lp->ys = MINSIZE;
		else
			lp->ys = MIN(size, MAX(MINSIZE, MIN(lp->width, lp->height) /
					       MINGRIDSIZE));
		lp->xs = lp->ys;
	}
	lp->ncols = MAX(lp->width / lp->xs, 2);
	lp->nrows = MAX(lp->height / lp->ys, 2);
	lp->xb = (lp->width - lp->xs * lp->ncols) / 2;
	lp->yb = (lp->height - lp->ys * lp->nrows) / 2;

	freecells(lp);
	alloccells(lp);
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

	if (rule == 2333)
		npats = NPATS_2333;
	else
		npats = 0;
	lp->pattern = NRAND(npats + 2);
	if (lp->pattern >= npats)
		RandomSoup(mi, 30, 15);
	else
		GetPattern(mi, lp->pattern);
}

void
draw_life(ModeInfo * mi)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	unsigned char *loc, *temploc, *lastloc;
	int         row, col;
	unsigned char living_now, birth_now;

	loc = lp->buffer + lp->ncols + BORDER + 1;
	temploc = lp->tempbuf;
	/* copy the first 2 rows to the tempbuf */
	(void) memcpy((char *) temploc, (char *) loc, lp->ncols);
	(void) memcpy((char *) (temploc + lp->ncols),
		      (char *) (loc + lp->ncols + BORDER), lp->ncols);

	lastloc = lp->lastbuf;
	/* copy the last row to another buffer for wraparound */
	(void) memcpy((char *) lastloc,
	 (char *) loc + ((lp->nrows - 1) * (lp->ncols + BORDER)), lp->ncols);
	for (row = 0; row < lp->nrows; ++row) {
		for (col = 0; col < lp->ncols; ++col) {
			birth_now = birth[*temploc];
			living_now = living[*temploc];
/* PURIFY 4.0 on SunOS4 reports an array bounds read on the next line */
			*temploc = (row == (lp->nrows - BORDER - 1)) ?
				*(lastloc + col) : *(loc + (lp->ncols + BORDER) * 2);
			if (*(loc + 1) & RT) {
				if (living_now)
					drawcell(mi, row, col);		/* Draw because the cell ages */
				else {
					ckill(lp, loc);
					erasecell(mi, row, col);
				}
			} else {
				if (birth_now) {
					spawn(lp, loc);
					drawcell(mi, row, col);
				}
			}
			loc++;
			temploc++;
		}
		loc += BORDER;
		if (temploc >= lp->tempbuf + lp->ncols * 2)
			temploc = lp->tempbuf;
	}

	if (++lp->generation > MI_CYCLES(mi))
		init_life(mi);

	/*
	 * generate a randomized shooter aimed roughly toward the center of the
	 * screen after batchcount.
	 */

	if (lp->generation && lp->generation %
	    ((MI_BATCHCOUNT(mi) < 0) ? 1 : MI_BATCHCOUNT(mi)) == 0)
		shooter(mi);
}

void
release_life(ModeInfo * mi)
{
	if (lifes != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++)
			freecells(&lifes[screen]);
		(void) free((void *) lifes);
		lifes = NULL;
	}
}

void
refresh_life(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}


void
change_life(ModeInfo * mi)
{
	lifestruct *lp = &lifes[MI_SCREEN(mi)];
	int         npats;

	lp->generation = 0;
	freecells(lp);
	alloccells(lp);
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));

	lp->pattern = lp->pattern + 1;
	if (rule == 2333)
		npats = NPATS_2333;
	else
		npats = 0;
	if (lp->pattern >= npats + 2)
		lp->pattern = 0;
	if (lp->pattern >= npats)
		RandomSoup(mi, 30, 15);
	else
		GetPattern(mi, lp->pattern);
}
