#ifndef __XLOCK_XLOCK_H__
#define __XLOCK_XLOCK_H__

/*-
 * @(#)xlock.h	3.9 96/05/25 xlockmore 
 *
 * xlock.h - external interfaces for new modes and SYSV OS defines.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 * 12-May-95: Added defines for SunOS's Adjunct password file
 *            Dale A. Harris <rodmur@ecst.csuchico.edu>
 * 18-Nov-94: Modified for QNX 4.2 w/ Metrolink X server from Brian Campbell
 *            <brianc@qnx.com>.
 * 11-Jul-94: added Bool flag: inwindow, which tells xlock to run in a
 *            window from Greg Bowering <greg@cs.adelaide.edu.au>
 * 11-Jul-94: patch for Solaris SYR4 from Chris P. Ross <cross@eng.umd.edu>
 * 28-Jun-94: Reorganized shadow stuff
 * 24-Jun-94: Reorganized
 * 22-Jun-94: Modified for VMS
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * 17-Jun-94: patched shadow passwords and bcopy and bzero for SYSV from
 *            <reggers@julian.uwo.ca>
 * 21-Mar-94: patched the patch for AIXV3 and HP from
 *            <R.K.Lloyd@csc.liv.ac.uk>.
 * 01-Dec-93: added patch for AIXV3 from
 *            (Tom McConnell, tmcconne@sedona.intel.com) also added a patch
 *            for HP-UX 8.0.
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>

#define MAXSCREENS        3
#define NUMCOLORS         64
#define PASSLENGTH        120
#define FALLBACK_FONTNAME "fixed"
#ifndef DEF_MFONT
#define DEF_MFONT "-*-times-*-*-*-*-18-*-*-*-*-*-*-*"
#endif
#ifndef DEF_PROGRAM		/* Try the -o option ;) */
#define DEF_PROGRAM "fortune -s"
#endif

#define ICONW             64
#define ICONH             64

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#if defined( VMS ) || defined( __QNX__ )
#if defined( VMS ) && ( __VMS_VER < 70000000 )
/* #define VMS_PLAY */
/* #define XVMSUTILS */
#ifndef XVMSUTILS
#define OLD_EVENT_LOOP
#endif
#include <unixlib.h>
#endif
#define M_E    2.7182818284590452354
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#endif

#if !defined( news1800 ) && !defined( sun386 )
#include <stdlib.h>
#if !defined( apollo ) && !defined( VMS )
#include <unistd.h>
#include <memory.h>
#endif
#endif
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#if !defined( VMS ) || ( __VMS_VER >= 70000000 )
#include <dirent.h>
#else
#ifdef XVMSUTILS
#if 0
#include "../xvmsutils/unix_types.h"
#include "../xvmsutils/dirent.h"
#else
#include <X11/unix_types.h>
#include <X11/dirent.h>
#endif
#endif
#endif

typedef struct {
	GC          gc;		/* graphics context for animation */
	int         npixels;	/* number of valid entries in pixels */
	Colormap    cmap;	/* current colormap */
	unsigned long pixels[NUMCOLORS];	/* pixel values in the colormap */
	unsigned long bgcol, fgcol;	/* background and foreground pixel values */
	unsigned long rightcol, leftcol;	/* 3D color pixel values */
} perscreen;

/* This stuff moved here from resource.c for the mode-specific options. */
#define t_String        0
#define t_Float         1
#define t_Int           2
#define t_Bool          3

typedef struct {
	caddr_t    *var;
	char       *name;
	char       *class;
	char       *def;
	int         type;
} argtype;

typedef struct {
	char       *opt;
	char       *desc;
} OptionStruct;

typedef struct {
	int         numopts;
	XrmOptionDescRec *opts;
	argtype    *vars;
	OptionStruct *desc;
} ModeSpecOpt;

/* End moved from resource.c */

/* this must follow definition of ModeSpecOpt */
#include "mode.h"

extern perscreen Scr[MAXSCREENS];
extern Display *dsp;
extern int  screen;

extern void GetResources(int argc, char **argv);
extern unsigned long allocpixel(Colormap cmap, char *name, char *def);
extern void set_colormap(Window window, Colormap map, Bool inwindow);
extern void fix_colormap(Window window, int screen, float saturation,
		     Bool install, Bool inroot, Bool inwindow, Bool verbose);
extern long seconds(void);

#if defined (__STDC__) || defined(VMS) || defined( AIXV3 )
extern void error(char *s1,...);
extern void warning(char *s1,...);
#else
extern void error(char *s1, char *s2);
extern void warning(char *s1, char *s2, char *s3);
#endif

extern int  readable(char *);
extern void randomImage(char *im_file);

#if defined( ultrix ) || (defined( VMS ) && ( __VMS_VER < 70000000 ))
extern char *strdup(char *);

#endif

#ifdef LESS_THAN_AIX3_2
#undef NULL
#define NULL 0
#endif /* LESS_THAN_AIX3_2 */

#if defined( __STDC__ ) && (defined( __hpux ) && defined( _PA_RISC1_1 ))
#define MATHF
#endif
#ifdef MATHF
#define SINF(n) sinf(n)
#define COSF(n) cosf(n)
#define FABSF(n) fabsf(n)
#else
#define SINF(n) ((float)sin((double)(n)))
#define COSF(n) ((float)cos((double)(n)))
#define FABSF(n) ((float)fabs((double)(n)))
#endif

/*** random number generator ***/
/* defaults */
#ifndef SRAND
extern void SetRNG(long int s);

#define SRAND(X) SetRNG((long) X)
#endif
#ifndef LRAND
extern long LongRNG(void);

#define LRAND() LongRNG()
#endif
#ifndef MAXRAND
#define MAXRAND (2147483648.0)
#endif

#define NRAND(X) ((int)(LRAND()%(X)))

#endif /* __XLOCK_XLOCK_H__ */
