/*-
 * @(#)xlock.h	3.0 95/06/24 xlockmore 
 *
 * xlock.h - external interfaces for new modes and SYSV OS defines.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@source.asset.com>
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

#define MAXSCREENS        3
#define NUMCOLORS         64
#define PASSLENGTH        64
#define FALLBACK_FONTNAME "fixed"
#define ICONW             64
#define ICONH             64

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#if defined VMS || defined __QNX__
#ifdef VMS
#include <unixlib.h>
#endif
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.78539816339744830962
#endif

#if !defined (news1800) && !defined (sun386)
#include <stdlib.h>
#if !defined (apollo) && !defined (VMS)
#include <unistd.h>
#endif
#endif
 
typedef struct {
  GC            gc;                /* graphics context for animation */
  int           npixels;           /* number of valid entries in pixels */
  unsigned long pixels[NUMCOLORS]; /* pixel values in the colormap */
  unsigned long bgcol, fgcol;      /* background and foreground pixel values */
} perscreen;

extern perscreen Scr[MAXSCREENS];
extern Display *dsp;
extern int  screen;

extern char  *ProgramName;
extern char  *fontname;
extern char  *background;
extern char  *foreground;
extern char  *text_name;
extern char  *text_pass;
extern char  *text_info;
extern char  *text_valid;
extern char  *text_invalid;
extern char  *geometry;
extern float saturation;
extern int   nicelevel;
extern int   delay;
extern int   batchcount;
extern int   cycles;
extern int   timeout;
#ifdef AUTO_LOGOUT
extern int   forceLogout;
#endif
#ifdef LOGOUT_BUTTON
extern int   enable_button;
extern char  *logoutButtonLabel;
extern char  *logoutButtonHelp;
extern char  *logoutFailedString;
#endif
extern Bool  usefirst;
extern Bool  mono;
extern Bool  nolock;
extern Bool  allowroot;
extern Bool  enablesaver;
extern Bool  allowaccess;
extern Bool  grabmouse;
extern Bool  echokeys;
extern Bool  verbose;
extern Bool  inwindow;
extern Bool  inroot;
extern Bool  timeelapsed;
extern Bool  install;
extern void  (*callback) ();
extern void  (*init) ();

extern void GetResources();
extern void set_colormap();
#ifdef __STDC__
extern void error(char *, ...);
#else
extern void error();
#endif

#ifdef LESS_THAN_AIX3_2
#undef NULL
#define NULL 0
#endif /* LESS_THAN_AIX3_2 */

/*** random number generator ***/
/* insert your favorite */
extern void SetRNG();
extern long LongRNG();
#define SRAND(X) SetRNG((long) X)
#define LRAND() LongRNG()

#define NRAND(X) ((int)(LRAND()%(X)))
#define MAXRAND (2147483648.0)
