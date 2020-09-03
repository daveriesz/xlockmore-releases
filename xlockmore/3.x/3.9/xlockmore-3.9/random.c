#ifndef lint
static char sccsid[] = "@(#)random.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * random.c - Run random modes for a certain duration.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 18-Mar-96: Ron Hitchens <ron@utw.com>
 *		Re-coded for the ModeInfo calling scheme.  Added the
 *		change hook.  Get ready for 3.8 release.
 * 23-Dec-95: Ron Hitchens <ron@utw.com>
 *		Re-coded pickMode() to keep track of the modes, so
 *		that all modes are tried before there are any repeats.
 *		Also prevent a mode from being picked twice in a row
 *		(could happen as first pick after refreshing the list).
 * 04-Sep-95: Written by Heath A. Kehoe <hakehoe@icaen.uiowa.edu>.
 *
 */

#include "xlock.h"


#define NUMSPECIAL	2	/* blank,random */

#define GC_SAVE_VALUES (GCFunction|GCLineWidth|GCLineStyle|GCCapStyle|GCJoinStyle|GCGraphicsExposures|GCFont|GCSubwindowMode)

#define DEF_DURATION	"0"	/* 0 == infinite duration */
#define DEF_MODELIST	""

extern float saturation;
extern int  delay;
extern int  batchcount;
extern int  cycles;

static int  duration;
static char *modelist;

static XrmOptionDescRec opts[] =
{
	{"-duration", ".random.duration", XrmoptionSepArg, (caddr_t) NULL},
	{"-modelist", ".random.modelist", XrmoptionSepArg, (caddr_t) NULL}
};
static argtype vars[] =
{
	{(caddr_t *) & duration, "duration", "Duration", DEF_DURATION, t_Int},
     {(caddr_t *) & modelist, "modelist", "Modelist", DEF_MODELIST, t_String}
};
static OptionStruct desc[] =
{
	{"-duration", "how long a mode runs before changing to another"},
	{"-modelist", "list of modes to randomly choose from"}
};

ModeSpecOpt random_opts =
{2, opts, vars, desc};

typedef struct {
	int         width, height;
	XGCValues   gcvs;
} randomstruct;

static int  currentmode = -1;
static int  starttime;
static int *modes;
static int  nmodes;
static Bool change_now = False;

static randomstruct randoms[MAXSCREENS];

static
void
setMode(int newmode, ModeInfo * mi)
{
	randomstruct *rp = &randoms[MI_SCREEN(mi)];
	int         num_screens = ScreenCount(dsp);
	int         i;

	currentmode = newmode;

/* FIX THIS GLOBAL ACCESS */
	delay = MI_DELAY(mi) = LockProcs[currentmode].def_delay;
	batchcount = MI_BATCHCOUNT(mi) = LockProcs[currentmode].def_batchcount;
	cycles = MI_CYCLES(mi) = LockProcs[currentmode].def_cycles;
	saturation = MI_SATURATION(mi) = LockProcs[currentmode].def_saturation;

	for (i = 0; i < num_screens; i++) {
		XChangeGC(MI_DISPLAY(mi), MI_GC(mi),
			  GC_SAVE_VALUES, &(rp->gcvs));
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2)
			fix_colormap(MI_WINDOW(mi), i, MI_SATURATION(mi),
				     MI_WIN_IS_INSTALL(mi), MI_WIN_IS_INROOT(mi), MI_WIN_IS_INWINDOW(mi),
				     MI_WIN_IS_VERBOSE(mi));
	}
	if (MI_WIN_IS_VERBOSE(mi))
		(void) printf("mode: %s\n", LockProcs[currentmode].cmdline_arg);
}

static
int
pickMode(void)
{
	static int *mode_indexes = NULL;
	static int  mode_count = 0;
	static int  last_mode = 0;
	int         mode, i;

	if (mode_indexes == NULL) {
		if ((mode_indexes = (int *) calloc(nmodes, sizeof (int))) ==
		    NULL) {
			return modes[NRAND(nmodes)];
		}
	}
	if (mode_count == 0) {
		for (i = 0; i < nmodes; i++) {
			mode_indexes[i] = modes[i];
		}
		mode_count = nmodes;
	}
	if (mode_count == 1) {
		/* only one left, let's use that one */
		return (last_mode = mode_indexes[--mode_count]);
	} else {
		/* pick a random slot in the list, check for last */
		while (mode_indexes[i = NRAND(mode_count)] == last_mode)
			/* empty */ ;
	}

	mode = mode_indexes[i];	/* copy out chosen mode */
	/* move mode at end of list to slot vacated by chosen mode, dec count */
	mode_indexes[i] = mode_indexes[--mode_count];
	return (last_mode = mode);	/* remember last mode picked */
}

static
char       *
strpmtok(int *sign, char *str)
{
	static int  nextsign = 0;
	static char *loc;
	char       *p, *r;

	if (str)
		loc = str;
	if (nextsign) {
		*sign = nextsign;
		nextsign = 0;
	}
	p = loc - 1;
	for (;;) {
		switch (*++p) {
			case '+':
				*sign = 1;
				continue;
			case '-':
				*sign = -1;
				continue;
			case ' ':
			case ',':
			case '\t':
			case '\n':
				continue;
			case 0:
				loc = p;
				return NULL;
		}
		break;
	}
	r = p;

	for (;;) {
		switch (*++p) {
			case '+':
				nextsign = 1;
				break;
			case '-':
				nextsign = -1;
				break;
			case ' ':
			case ',':
			case '\t':
			case '\n':
			case 0:
				break;
			default:
				continue;
		}
		break;
	}
	if (*p) {
		*p = 0;
		loc = p + 1;
	} else
		loc = p;

	return r;
}

static
void
parsemodelist(void)
{
	int         i, sign = 1;
	char       *p;

	modes = (int *) calloc(numprocs - 1, sizeof (int));

	p = strpmtok(&sign, modelist);

	while (p) {
		if (!strcmp(p, "all"))
			for (i = 0; i < numprocs - 1; i++)
				modes[i] = (i < numprocs - NUMSPECIAL) * (sign > 0);
		else {
			for (i = 0; i < numprocs - 1; i++)
				if (!strcmp(p, LockProcs[i].cmdline_arg))
					break;
			if (i < numprocs - 1)
				modes[i] = (sign > 0);
			else
				warning("%s: unrecognized mode \"%s\"\n", p);
		}
		p = strpmtok(&sign, (char *) NULL);
	}

	nmodes = 0;
	for (i = 0; i < numprocs - 1; i++)
		if (modes[i])
			modes[nmodes++] = i;
	if (!nmodes) {		/* empty list */
		for (i = 0; i < numprocs - NUMSPECIAL; i++)
			modes[i] = i;
		nmodes = i;
	}
/* 
   for (i = 0; i < nmodes; i++) (void) printf("%d ", modes[i]); (void) (void)
   printf("\n"); */
}

void
init_random(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	randomstruct *rp = &randoms[MI_SCREEN(mi)];
	int         i;

	rp->width = MI_WIN_WIDTH(mi);
	rp->height = MI_WIN_HEIGHT(mi);

	if (currentmode < 0) {
		parsemodelist();

		for (i = 0; i < MI_NUM_SCREENS(mi); i++)
			(void) XGetGCValues(display, MI_GC(mi), GC_SAVE_VALUES, &(rp->gcvs));
		setMode(pickMode(), mi);
		starttime = seconds();
		if (duration < 0)
			duration = 0;
	}
	call_init_hook(&LockProcs[currentmode], mi);
}

void
draw_random(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	GC          gc = MI_GC(mi);
	int         scrn = MI_SCREEN(mi);
	randomstruct *rp = &randoms[scrn];
	int         newmode, now = seconds();
	int         has_run = (duration == 0) ? 0 : (now - starttime);
	static int  do_init = 0;

	if ((scrn == 0) && do_init) {
		do_init = 0;
	}
	if (change_now || ((scrn == 0) && (has_run > duration))) {
		newmode = pickMode();

		XSetForeground(display, gc, MI_WIN_BLACK_PIXEL(mi));
		XFillRectangle(display, MI_WINDOW(mi), gc,
			       0, 0, rp->width, rp->height);

		setMode(newmode, mi);
		starttime = now;
		do_init = 1;
		change_now = False;
	}
	if (do_init) {
		call_init_hook(&LockProcs[currentmode], mi);
	}
	call_callback_hook(&LockProcs[currentmode], mi);
}

void
refresh_random(ModeInfo * mi)
{
	call_refresh_hook(&LockProcs[currentmode], mi);
}


void
change_random(ModeInfo * mi)
{
	change_now = True;	/* force a change on next draw callback */

	draw_random(mi);
}
