#ifndef lint
static char sccsid[] = "@(#)xlock.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * xlock.c - X11 client to lock a display and show a screen saver.
 *
 * Copyright (c) 1988-91 by Patrick J. Naughton.
 *
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
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 * 18-Mar-96: Ron Hitchens <ron@utw.com>
 *            Implemented new ModeInfo hook calling scheme.
 *            Created mode_info() to gather and pass info to hooks.
 *            Watch for and use MI_PAUSE value if mode hook sets it.
 *            Catch SIGSEGV, SIGBUS and SIGFPE signals.  Other should
 *            be caught.  Eliminate some globals.
 * 23-Dec-95: Ron Hitchens <ron@utw.com>
 *            Rewrote event loop so as not to use signals.
 * 01-Sep-95: initPasswd function, more stuff removed to passwd.c
 * 24-Jun-95: Cut out passwd stuff to passwd.c-> getPasswd & checkPasswd
 * 17-Jun-95: Added xlockrc password compile time option.
 * 12-May-95: Added defines for SunOS's Adjunct password file from
 *            Dale A. Harris <rodmur@ecst.csuchico.edu>
 * 21-Feb-95: MANY patches from Heath A. Kehoe <hakehoe@icaen.uiowa.edu>.
 * 24-Jan-95: time_displayed fixed from Chris Ross <cross@va.pubnix.com>.
 * 18-Jan-95: Ultrix systems (at least DECstations) running enhanced
 *            security from Chris Fuhrman <cfuhrman@vt.edu> and friend.
 * 26-Oct-94: In order to use extra-long passwords with the Linux changed
 *            PASSLENGTH to 64 <slouken@virtbrew.water.ca.gov>
 * 11-Jul-94: added -inwindow option from Greg Bowering
 *            <greg@cs.adelaide.edu.au>
 * 22-Jun-94: Modified for VMS
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * 10-Jun-94: patch for BSD from Victor Langeveld <vic@mbfys.kun.nl>
 * 02-May-94: patched to work on Linux from Darren Senn's
 *            (sinster@scintilla.capitola.ca.us) xlock for Linux.
 *            Took out "bounce" since it was too buggy (maybe I will put
 *            it back later).
 * 21-Mar-94: patch to to trap Shift-Ctrl-Reset courtesy of Jamie Zawinski
 *            <jwz@netscape.com>, patched the patch (my mistake) for AIXV3
 *            and HP from <R.K.Lloyd@csc.liv.ac.uk>.
 * 01-Dec-93: added patch for AIXV3 from
 *            (Tom McConnell, tmcconne@sedona.intel.com) also added a patch
 *            for HP-UX 8.0.
 * 29-Jul-93: "hyper", "helix", "rock", and "blot" (also tips on "maze") I
 *            got courtesy of Jamie Zawinski <jwz@netscape.com>;
 *            at the time I could not get his stuff to work for the hpux 8.0,
 *            so I scrapped it but threw his stuff in xlock.
 *            "maze" and "sphere" I got courtesy of Sun Microsystems.
 *            "spline" I got courtesy of Jef Poskanzer <jef@netcom.com or
 *            jef@well.sf.ca.us>.
 *
 * Changes of Patrick J. Naughton
 * 24-Jun-91: make foreground and background color get used on mono.
 * 24-May-91: added -usefirst.
 * 16-May-91: added pyro and random modes.
 *	      ripped big comment block out of all other files.
 * 08-Jan-91: fix some problems with password entry.
 *	      removed renicing code.
 * 29-Oct-90: added cast to XFree() arg.
 *	      added volume arg to call to XBell().
 * 28-Oct-90: center prompt screen.
 *	      make sure Xlib input buffer does not use up all of swap.
 *	      make displayed text come from resource file for better I18N.
 *	      add backward compatible signal handlers for pre 4.1 machines.
 * 31-Aug-90: added blank mode.
 *	      added swarm mode.
 *	      moved usleep() and seconds() out to usleep.c.
 *	      added SVR4 defines to xlock.h
 * 29-Jul-90: added support for multiple screens to be locked by one xlock.
 *	      moved global defines to xlock.h
 *	      removed use of allowsig().
 * 07-Jul-90: reworked commandline args and resources to use Xrm.
 *	      moved resource processing out to resource.c
 * 02-Jul-90: reworked colors to not use dynamic colormap.
 * 23-May-90: added autoraise when obscured.
 * 15-Apr-90: added hostent alias searching for host authentication.
 * 18-Feb-90: added SunOS3.5 fix.
 *	      changed -mono -> -color, and -saver -> -lock.
 *	      allow non-locking screensavers to display on remote machine.
 *	      added -echokeys to disable echoing of '?'s on input.
 *	      cleaned up all of the parameters and defaults.
 * 20-Dec-89: added -xhost to allow access control list to be left alone.
 *	      added -screensaver (do not disable screen saver) for the paranoid.
 *	      Moved seconds() here from all of the display mode source files.
 *	      Fixed bug with calling XUngrabHosts() in finish().
 * 19-Dec-89: Fixed bug in GrabPointer.
 *	      Changed fontname to XLFD style.
 * 23-Sep-89: Added fix to allow local hostname:0 as a display.
 *	      Put empty case for Enter/Leave events.
 *	      Moved colormap installation later in startup.
 * 20-Sep-89: Linted and made -saver mode grab the keyboard and mouse.
 *	      Replaced SunView code for life mode with Jim Graham's version,
 *		so I could contrib it without legal problems.
 *	      Sent to expo for X11R4 contrib.
 * 19-Sep-89: Added '?'s on input.
 * 27-Mar-89: Added -qix mode.
 *	      Fixed GContext->GC.
 * 20-Mar-89: Added backup font (fixed) if XQueryLoadFont() fails.
 *	      Changed default font to lucida-sans-24.
 * 08-Mar-89: Added -nice, -mode and -display, built vector for life and hop.
 * 24-Feb-89: Replaced hopalong display with life display from SunView1.
 * 22-Feb-89: Added fix for color servers with n < 8 planes.
 * 16-Feb-89: Updated calling conventions for XCreateHsbColormap();
 *	      Added -count for number of iterations per color.
 *	      Fixed defaulting mechanism.
 *	      Ripped out VMS hacks.
 *	      Sent to expo for X11R3 contrib.
 * 15-Feb-89: Changed default font to pellucida-sans-18.
 * 20-Jan-89: Added -verbose and fixed usage message.
 * 19-Jan-89: Fixed monochrome gc bug.
 * 16-Dec-88: Added SunView style password prompting.
 * 19-Sep-88: Changed -color to -mono. (default is color on color displays).
 *	      Added -saver option. (just do display... do not lock.)
 * 31-Aug-88: Added -time option.
 *	      Removed code for fractals to separate file for modularity.
 *	      Added signal handler to restore host access.
 *	      Installs dynamic colormap with a Hue Ramp.
 *	      If grabs fail then exit.
 *	      Added VMS Hacks. (password 'iwiwuu').
 *	      Sent to expo for X11R2 contrib.
 * 08-Jun-88: Fixed root password pointer problem and changed PASSLENGTH to 20.
 * 20-May-88: Added -root to allow root to unlock.
 * 12-Apr-88: Added root password override.
 *	      Added screen saver override.
 *	      Removed XGrabServer/XUngrabServer.
 *	      Added access control handling instead.
 * 01-Apr-88: Added XGrabServer/XUngrabServer for more security.
 * 30-Mar-88: Removed startup password requirement.
 *	      Removed cursor to avoid phosphor burn.
 * 27-Mar-88: Rotate fractal by 45 degrees clockwise.
 * 24-Mar-88: Added color support. [-color]
 *	      wrote the man page.
 * 23-Mar-88: Added HOPALONG routines from Scientific American Sept. 86 p. 14.
 *	      added password requirement for invokation
 *	      removed option for command line password
 *	      added requirement for display to be "unix:0".
 * 22-Mar-88: Recieved Walter Milliken's comp.windows.x posting.
 *
 */

#include <signal.h>
#if defined( __STDC__ ) || defined( VMS ) || defined( AIXV3 )
#include <stdarg.h>
#endif
#include <errno.h>
#include <sys/types.h>
#ifdef AIXV3
#include <sys/select.h>
#else
#include <sys/time.h>
#endif
#include "xlock.h"
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#ifdef USE_VROOT
#include "vroot.h"
#endif
#ifdef SYSLOG
#include <syslog.h>
#endif
#ifdef HAS_RPLAY
#include <rplay.h>
#endif
#ifdef VMS_PLAY
#define rplay_display vms_play
#define HAS_RPLAY
#endif
#ifdef DEF_PLAY
#define rplay_display system_play
#define HAS_RPLAY
#endif
#if defined( VMS ) && !defined( OLD_EVENT_LOOP ) && ( __VMS_VER < 70000000 )
#if 0
#include "../xvmsutils/unix_types.h"
#include "../xvmsutils/unix_time.h"
#else
#include <X11/unix_types.h>
#include <X11/unix_time.h>
#endif
#endif
#ifdef DT_SAVER
#include <X11/Intrinsic.h>
#include <Dt/Saver.h>
#endif

extern char *getenv(const char *);
extern void CheckResources(void);
extern void initPasswd(void);
extern int  checkPasswd(char *);

#ifndef HAS_USLEEP
extern int  usleep(unsigned int);

#endif

#if defined( AUTO_LOGOUT ) || defined( LOGOUT_BUTTON )
extern void logoutUser();

#endif

#ifndef AIXV3
extern int  select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif
extern int  nice(int);


extern char user[PASSLENGTH];
extern float saturation;
extern float delta3d;
extern int  delay;
extern int  batchcount;
extern int  cycles;
extern int  nicelevel;
extern int  timeout;
extern int  lockdelay;
extern Bool allowaccess;
extern Bool allowroot;
extern Bool echokeys;
extern Bool enablesaver;
extern Bool grabmouse;
extern Bool inroot;
extern Bool install;
extern Bool inwindow;
extern Bool mono;
extern Bool nolock;
extern Bool timeelapsed;
extern Bool usefirst;
extern Bool use3d;
extern Bool verbose;

char       *ProgramName;	/* argv[0] */
perscreen   Scr[MAXSCREENS];
Display    *dsp = NULL;		/* server display connection */
int         screen;		/* current screen */

extern char *fontname;
extern char *background;
extern char *foreground;
extern char *text_name;
extern char *text_pass;
extern char *text_info;
extern char *text_valid;
extern char *text_invalid;
extern char *geometry;
extern char *right3d;
extern char *left3d;

#if defined( HAS_RPLAY ) || defined( VMS_PLAY ) || defined( DEF_PLAY )
extern char *locksound;
extern char *infosound;
extern char *validsound;
extern char *invalidsound;

#endif
#ifdef AUTO_LOGOUT
extern int  forceLogout;

#endif

#ifdef LOGOUT_BUTTON
extern int  enable_button;
extern char *logoutButtonLabel;
extern char *logoutButtonHelp;
extern char *logoutFailedString;

#endif

#ifdef DT_SAVER
extern Bool dtsaver;

#endif

static int  onepause = 0;


static int  screens;		/* number of screens */
static Window win[MAXSCREENS];	/* window used to cover screen */
static Window icon[MAXSCREENS];	/* window used during password typein */

#ifdef LOGOUT_BUTTON
static Window button[MAXSCREENS];

#endif
static Window root[MAXSCREENS];	/* convenience pointer to the root window */
static GC   textgc[MAXSCREENS];	/* graphics context used for text rendering */
static unsigned long fgcol[MAXSCREENS];		/* used for text rendering */
static unsigned long bgcol[MAXSCREENS];		/* background of text screen */
static long rightcol[MAXSCREENS];
static long leftcol[MAXSCREENS];
static int  iconx[MAXSCREENS];	/* location of left edge of icon */
static int  icony[MAXSCREENS];	/* location of top edge of icon */
static Cursor mycursor;		/* blank cursor */
static Pixmap lockc;
static Pixmap lockm;		/* pixmaps for cursor and mask */
static char no_bits[] =
{0};				/* dummy array for the blank cursor */
static int  passx;		/* position of the ?'s */
static int  passy;
static XFontStruct *font;
static int  sstimeout;		/* screen saver parameters */
static int  ssinterval;
static int  ssblanking;
static int  ssexposures;
static long start_time;

/* GEOMETRY STUFF */
static int  sizeconfiguremask;
static XWindowChanges fullsizeconfigure, minisizeconfigure;
static int  fullscreen = False;

#if defined( LOGOUT_BUTTON ) || defined( AUTO_LOGOUT )
static int  tried_logout = 0;

#endif

#ifdef HAS_RPLAY
static int  got_invalid = 0;

#endif

#define AllPointerEventMask \
	(ButtonPressMask | ButtonReleaseMask | \
	EnterWindowMask | LeaveWindowMask | \
	PointerMotionMask | PointerMotionHintMask | \
	Button1MotionMask | Button2MotionMask | \
	Button3MotionMask | Button4MotionMask | \
	Button5MotionMask | ButtonMotionMask | \
	KeymapStateMask)

#ifdef DEF_PLAY
void
system_play(char *string)
{
	char        progrun[BUFSIZ];

	(void) sprintf(progrun, "( %s%s.au ) 2>&1", DEF_PLAY, string);
	system(progrun);
}

#endif

/* VARARGS1 */
#if defined( __STDC__ ) || defined( VMS ) || defined( AIXV3 )
void
error(char *s1,...)
{
	char       *s2;
	va_list     vl;

	va_start(vl, s1);
	s2 = va_arg(vl, char *);

	va_end(vl);
#else
void
error(char *s1, char *s2)
{
#endif
	(void) fprintf(stderr, s1, ProgramName, s2);
	exit(1);
}

#if defined( __STDC__ ) || defined( VMS ) || defined( AIXV3 )
void
warning(char *s1,...)
{
	char       *s2, *s3;
	va_list     vl;

	va_start(vl, s1);
	s2 = va_arg(vl, char *);
	s3 = va_arg(vl, char *);

	va_end(vl);
#else
void
warning(char *s1, char *s2, char *s3)
{
#endif
	(void) fprintf(stderr, s1, ProgramName, s2, s3);
}

/* Server access control support. */

static XHostAddress *XHosts;	/* the list of "friendly" client machines */
static int  HostAccessCount;	/* the number of machines in XHosts */
static Bool HostAccessState;	/* whether or not we even look at the list */

static void
XGrabHosts(Display * display)
{
	XHosts = XListHosts(display, &HostAccessCount, &HostAccessState);
	if (XHosts)
		XRemoveHosts(display, XHosts, HostAccessCount);
	XEnableAccessControl(display);
}

static void
XUngrabHosts(Display * display)
{
	if (XHosts) {
		XAddHosts(display, XHosts, HostAccessCount);
		XFree((char *) XHosts);
	}
	if (HostAccessState == False)
		XDisableAccessControl(display);
}


#ifndef DEBUG
/* Simple wrapper to get an asynchronous grab on the keyboard and mouse. If
   either grab fails, we sleep for one second and try again since some window
   manager might have had the mouse grabbed to drive the menu choice that
   picked "Lock Screen..".  If either one fails the second time we print an
   error message and exit. */
static void
GrabKeyboardAndMouse(void)
{
	Status      status;

	status = XGrabKeyboard(dsp, win[0], True,
			       GrabModeAsync, GrabModeAsync, CurrentTime);
	if (status != GrabSuccess) {
		(void) sleep(1);
		status = XGrabKeyboard(dsp, win[0], True,
				  GrabModeAsync, GrabModeAsync, CurrentTime);

		if (status != GrabSuccess)
			error("%s: could not grab keyboard! (%d)\n", status);
	}
	status = XGrabPointer(dsp, win[0], True, AllPointerEventMask,
			      GrabModeAsync, GrabModeAsync, None, mycursor,
			      CurrentTime);
	if (status != GrabSuccess) {
		(void) sleep(1);
		status = XGrabPointer(dsp, win[0], True, AllPointerEventMask,
				GrabModeAsync, GrabModeAsync, None, mycursor,
				      CurrentTime);

		if (status != GrabSuccess)
			error("%s: could not grab pointer! (%d)\n", status);
	}
}

#endif


/* Assuming that we already have an asynch grab on the pointer, just grab it
   again with a new cursor shape and ignore the return code. */
static void
XChangeGrabbedCursor(Cursor cursor)
{
#ifndef DEBUG
	if (grabmouse && !inwindow && !inroot)
		(void) XGrabPointer(dsp, win[0], True, AllPointerEventMask,
		    GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);
#endif
}


/* 
 *    Return a pointer to an up-to-date ModeInfo struct for the given
 *      screen, window and iconic mode.  Because the number of screens
 *      is limited, and much of the information is screen-specific, this
 *      function keeps static copies of ModeInfo structs are kept for
 *      each screen.  This also eliminates redundant calls to the X server
 *      to acquire info that does change.
 */

static
ModeInfo   *
mode_info(int scrn, Window window, int iconic)
{
	XWindowAttributes xgwa;
	ModeInfo   *mi;
	static ModeInfo modeinfo[MAXSCREENS];

	mi = &modeinfo[scrn];

	if (MI_WIN_FLAG_NOT_SET(mi, WI_FLAG_INFO_INITTED)) {
		/* This stuff only needs to be set once per screen */

		(void) memset((char *) mi, 0, sizeof (ModeInfo));

		MI_DISPLAY(mi) = dsp;	/* global */
		MI_SCREEN(mi) = scrn;
		MI_REAL_SCREEN(mi) = scrn;	/* TODO, for debug */
		MI_SCREENPTR(mi) = ScreenOfDisplay(dsp, scrn);
		MI_NUM_SCREENS(mi) = ScreenCount(dsp);	/* TODO */
		MI_MAX_SCREENS(mi) = MAXSCREENS;

		MI_WIN_BLACK_PIXEL(mi) = BlackPixel(dsp, scrn);
		MI_WIN_WHITE_PIXEL(mi) = WhitePixel(dsp, scrn);

		MI_PERSCREEN(mi) = &Scr[scrn];

		/* accessing globals here */
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_MONO,
			     (mono || CellsOfScreen(MI_SCREENPTR(mi)) <= 2));
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_INWINDOW, inwindow);
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_INROOT, inroot);
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_NOLOCK, nolock);
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_INSTALL, install);
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_USE3D, use3d);
		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_VERBOSE, verbose);

		MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_INFO_INITTED, True);
	}
	if (MI_WINDOW(mi) != window) {
		MI_WINDOW(mi) = window;

		(void) XGetWindowAttributes(dsp, window, &xgwa);
		MI_WIN_WIDTH(mi) = xgwa.width;
		MI_WIN_HEIGHT(mi) = xgwa.height;
		MI_WIN_DEPTH(mi) = xgwa.depth;
		MI_VISUAL(mi) = xgwa.visual;
	}
	MI_WIN_SET_FLAG_STATE(mi, WI_FLAG_ICONIC, iconic);

	MI_DELTA3D(mi) = delta3d;
	MI_PAUSE(mi) = 0;	/* use default if mode doesn't change this */

	MI_DELAY(mi) = delay;	/* globals */
	MI_BATCHCOUNT(mi) = batchcount;
	MI_CYCLES(mi) = cycles;
	MI_SATURATION(mi) = saturation;

	return (mi);
}


/* Restore all grabs, reset screensaver, restore colormap, close connection. */
static void
finish(void)
{
#if 0
	/* this still isn't right */
	for (screen = 0; screen < screens; screen++) {
		if (win[screen] != None) {
			release_last_mode(mode_info(screen, win[screen], False));
		}
		if (icon[screen] != None) {
			release_last_mode(mode_info(screen, icon[screen], True));
		}
	}
#endif

	XSync(dsp, False);
#ifdef USE_VROOT
	if (inroot)
		XClearWindow(dsp, win[0]);
#endif
	if (!nolock && !allowaccess)
		XUngrabHosts(dsp);
	XUngrabPointer(dsp, CurrentTime);
	XUngrabKeyboard(dsp, CurrentTime);
	if (!enablesaver) {
		XSetScreenSaver(dsp, sstimeout, ssinterval,
				ssblanking, ssexposures);
	}
	XFlush(dsp);
	XCloseDisplay(dsp);
}

static int
xio_error(Display * d)
{
	exit(1);
	return 1;
}

/* Convenience function for drawing text */
static void
putText(Window w, GC gc, char *string, int bold, int left, int *px, int *py)
				/* which window */
				/* gc */
				/* text to write */
				/* 1 = make it bold */
				/* left edge of text */
				/* current x and y, input & return */
{
#define PT_BUFSZ 2048
	char        buf[PT_BUFSZ], *p, *s;
	int         x = *px, y = *py, last, len;

	(void) strncpy(buf, string, PT_BUFSZ);
	buf[PT_BUFSZ - 1] = 0;

	p = buf;
	last = 0;
	for (;;) {
		s = p;
		for (; *p; ++p)
			if (*p == '\n')
				break;
		if (!*p)
			last = 1;
		*p = 0;

		if ((len = strlen(s))) {	/* yes, "=", not "==" */
			XDrawImageString(dsp, w, gc, x, y, s, len);
			if (bold)
				XDrawString(dsp, w, gc, x + 1, y, s, len);
		}
		if (!last) {
			y += font->ascent + font->descent + 2;
			x = left;
		} else {
			if (len)
				x += XTextWidth(font, s, len);
			break;
		}
		p++;
	}
	*px = x;
	*py = y;
}


#ifndef LO_BUTTON_TIME
#define LO_BUTTON_TIME 5
#endif

static void
statusUpdate(int new, int scr)
{
	int         left, x, y, now, len;
	char        buf[1024];
	XWindowAttributes xgwa;
	static int  last_time;

#ifdef LOGOUT_BUTTON
	int         ysave;
	static      made_button, last_tried_lo = 0;
	static int  loButton = LO_BUTTON_TIME;

#endif /* LOGOUT_BUTTON */

	now = seconds();
	len = (now - start_time) / 60;

#ifdef LOGOUT_BUTTON
	if (tried_logout && !last_tried_lo) {
		last_tried_lo = 1;
		new = 1;
#ifdef AUTO_LOGOUT
		forceLogout = 0;
#endif
	}
	if (new)
		made_button = 0;
#endif /* LOGOUT_BUTTON */

	if (new || last_time != len)
		last_time = len;
	else
		return;

	(void) XGetWindowAttributes(dsp, win[scr], &xgwa);
	x = left = iconx[scr];
	y = xgwa.height - (font->descent + 2);
#ifdef AUTO_LOGOUT
	if (forceLogout)
		y -= (font->ascent + font->descent + 2);
#endif
#ifdef LOGOUT_BUTTON
	if (enable_button) {
		if (loButton > len) {
			y -= (font->ascent + font->descent + 2);
			(void) sprintf(buf,
				       "%d minute%s until public logout button appears.    \n",
			     loButton - len, loButton - len == 1 ? "" : "s");
			putText(win[scr], textgc[scr], buf, False, left, &x, &y);
		} else if (!made_button || (new && tried_logout)) {
			made_button = 1;
			ysave = y;
			y = xgwa.height * 0.6;

			XUnmapWindow(dsp, button[scr]);
			XSetForeground(dsp, Scr[scr].gc, bgcol[scr]);
			XFillRectangle(dsp, win[scr], Scr[scr].gc, left, y,
				       xgwa.width - left, xgwa.height - y);
			XSetForeground(dsp, Scr[scr].gc, fgcol[scr]);

			if (tried_logout) {
				putText(win[scr], textgc[scr], logoutFailedString,
					True, left, &x, &y);
			} else {
				XMoveWindow(dsp, button[scr], left, y);
				XMapWindow(dsp, button[scr]);
				XRaiseWindow(dsp, button[scr]);
				(void) sprintf(buf, " %s ", logoutButtonLabel);
				XSetForeground(dsp, Scr[scr].gc, WhitePixel(dsp, scr));
				XDrawString(dsp, button[scr], Scr[scr].gc, 0, font->ascent + 1,
					    buf, strlen(buf));
				XSetForeground(dsp, Scr[scr].gc, fgcol[scr]);
				y += 5 + 2 * font->ascent + font->descent;
				putText(win[scr], textgc[scr], logoutButtonHelp,
					False, left, &x, &y);
			}
			y = ysave;
			x = left;
		}
	}
#endif /* LOGOUT_BUTTON */
#ifdef AUTO_LOGOUT
	if (forceLogout) {
		(void) sprintf(buf, "%d minute%s until auto-logout.    \n",
			       forceLogout - len,
			       forceLogout - len == 1 ? "" : "s");
		putText(win[scr], textgc[scr], buf, False, left, &x, &y);
	}
#endif /* AUTO_LOGOUT */
	if (timeelapsed) {
		(void) strcpy(buf, "Display has been locked for ");
		putText(win[scr], textgc[scr], buf, False, left, &x, &y);

		if (len < 60)
			(void) sprintf(buf, "%d minute%s.    \n", len, len == 1 ? "" : "s");
		else
			(void) sprintf(buf, "%d hour%s %d minute%s.    \n",
				       len / 60, len / 60 == 1 ? "" : "s",
				       len % 60, len % 60 == 1 ? "" : "s");
		putText(win[scr], textgc[scr], buf, False, left, &x, &y);
	}
}

#ifdef AUTO_LOGOUT
static void
checkLogout(void)
{
	if (nolock || tried_logout || !forceLogout)
		return;

	if (seconds() > forceLogout * 60 + start_time) {
		tried_logout = 1;
		forceLogout = 0;
		logoutUser();
	}
}

#endif /* AUTO_LOGOUT */

#ifndef OLD_EVENT_LOOP

/* subtract timer t2 from t1, return result in t3.  Result is not allowed to
   go negative, set to zero if result underflows */

static
void
sub_timers(struct timeval *t1, struct timeval *t2, struct timeval *t3)
{
	struct timeval tmp;
	int         borrow = 0;

	if (t1->tv_usec < t2->tv_usec) {
		borrow++;
		tmp.tv_usec = (1000000 + t1->tv_usec) - t2->tv_usec;
	} else {
		tmp.tv_usec = t1->tv_usec - t2->tv_usec;
	}

	/* Be careful here, timeval fields may be unsigned.  To avoid
	   underflow in an unsigned int, add the borrow value to the
	   subtrahend for the relational test.  Same effect, but avoids the
	   possibility of a negative intermediate value. */
	if (t1->tv_sec < (t2->tv_sec + borrow)) {
		tmp.tv_usec = tmp.tv_sec = 0;
	} else {
		tmp.tv_sec = t1->tv_sec - t2->tv_sec - borrow;
	}

	*t3 = tmp;
}

/* return 0 on event recieved, -1 on timeout */
static int
runMainLoop(int maxtime, int iconscreen)
{
	static int  lastdelay = -1, lastmaxtime = -1;
	int         fd = ConnectionNumber(dsp), r;
	struct timeval sleep_time, first, repeat;
	struct timeval elapsed, tmp;
	struct fd_set reads;
	long        started;

	first.tv_sec = first.tv_usec = 0;
	elapsed.tv_sec = elapsed.tv_usec = 0;
	repeat.tv_sec = delay / 1000000;
	repeat.tv_usec = delay % 1000000;

	started = seconds();

	for (;;) {
		if (delay != lastdelay || maxtime != lastmaxtime || onepause) {
			if (!delay || (maxtime && delay / 1000000 > maxtime)) {
				repeat.tv_sec = maxtime;
				repeat.tv_usec = 0;
			} else {
				repeat.tv_sec = delay / 1000000;
				repeat.tv_usec = delay % 1000000;
			}
			if (onepause) {
				first.tv_sec = onepause / 1000000;
				first.tv_usec = onepause % 1000000;
			} else {
				first = repeat;
			}
			lastdelay = delay;
			lastmaxtime = maxtime;
			onepause = 0;
			sleep_time = first;
		} else {
			sleep_time = repeat;
		}

		/* subtract time spent doing last loop iteration */
		sub_timers(&sleep_time, &elapsed, &sleep_time);

		FD_ZERO(&reads);
		FD_SET(fd, &reads);

#ifdef DCE_PASSWD
		r = _select_sys(fd + 1,
			(fd_set *) & reads, (fd_set *) NULL, (fd_set *) NULL,
				(struct timeval *) &sleep_time);
#else
		r = select(fd + 1,
			(fd_set *) & reads, (fd_set *) NULL, (fd_set *) NULL,
			   (struct timeval *) &sleep_time);
#endif

		if (r == 1)
			return 0;
		if (r > 0 || (r == -1 && errno != EINTR))
			(void) fprintf(stderr,
				       "Unexpected select() return value: %d (errno=%d)\n", r, errno);

		(void) gettimeofday(&tmp, NULL);	/* get time before
							   calling mode proc */

		for (screen = 0; screen < screens; screen++) {
			Window      cbwin;
			Bool        iconic;
			ModeInfo   *mi;

			if (screen == iconscreen) {
				cbwin = icon[screen];
				iconic = True;
			} else {
				cbwin = win[screen];
				iconic = False;
			}
			if (cbwin != None) {
				mi = mode_info(screen, cbwin, iconic);
				call_callback_hook((LockStruct *) NULL, mi);
				onepause = MI_PAUSE(mi);
			}
		}

		XSync(dsp, False);

		/* check for events received during the XSync() */
		if (QLength(dsp)) {
			return 0;
		}
		if (maxtime && ((seconds() - started) > maxtime)) {
			return -1;
		}
		/* if (mindelay) usleep(mindelay); */

		/* get the time now, figure how long it took */
		(void) gettimeofday(&elapsed, NULL);
		sub_timers(&elapsed, &tmp, &elapsed);
	}
}

#endif /* !OLD_EVENT_LOOP */

static int
ReadXString(char *s, int slen)
{
	XEvent      event;
	char        keystr[20];
	char        c;
	int         i;
	int         bp;
	int         len;
	int         thisscreen = screen;
	char        pwbuf[PASSLENGTH];
	int         first_key = 1;

	for (screen = 0; screen < screens; screen++)
		if (thisscreen == screen) {
			call_init_hook((LockStruct *) NULL, mode_info(screen, icon[screen], True));
		} else {
			call_init_hook((LockStruct *) NULL, mode_info(screen, win[screen], False));
		}
	statusUpdate(True, thisscreen);
	bp = 0;
	*s = 0;
	for (;;) {
		unsigned long lasteventtime = seconds();

		while (!XPending(dsp)) {
#ifdef OLD_EVENT_LOOP
			for (screen = 0; screen < screens; screen++)
				if (thisscreen == screen)
					call_callback_hook(NULL, mode_info(screen,
							icon[screen], True));
				else
					call_callback_hook(NULL, mode_info(screen,
							win[screen], False));
			statusUpdate(False, thisscreen);
			XSync(dsp, False);
			(void) usleep(onepause ? onepause : delay);
			onepause = 0;
#else
			statusUpdate(False, thisscreen);
			if (runMainLoop(min(timeout, 5), thisscreen) == 0)
				break;
#endif

			if (seconds() - lasteventtime > timeout) {
				screen = thisscreen;
				return 1;
			}
		}
		screen = thisscreen;
		XNextEvent(dsp, &event);
		switch (event.type) {
			case KeyPress:
				len = XLookupString((XKeyEvent *) & event, keystr, 20, NULL, NULL);
				for (i = 0; i < len; i++) {
					c = keystr[i];
					switch (c) {
						case 8:	/* ^H */
						case 127:	/* DEL */
							if (bp > 0)
								bp--;
							break;
						case 10:	/* ^J */
						case 13:	/* ^M */
							if (first_key && usefirst)
								break;
							s[bp] = '\0';
							return 0;
						case 21:	/* ^U */
							bp = 0;
							break;
						default:
							s[bp] = c;
							if (bp < slen - 1)
								bp++;
							else
								XSync(dsp, True);	/* flush input buffer */
					}
				}
				XSetForeground(dsp, Scr[screen].gc, bgcol[screen]);
				if (echokeys) {
					(void) memset((char *) pwbuf, '?', slen);
					XFillRectangle(dsp, win[screen], Scr[screen].gc,
						 passx, passy - font->ascent,
					       XTextWidth(font, pwbuf, slen),
					       font->ascent + font->descent);
					XDrawString(dsp, win[screen], textgc[screen],
						    passx, passy, pwbuf, bp);
				}
				/* eat all events if there are more than
				   enough pending... this keeps the Xlib event
				   buffer from growing larger than all
				   available memory and crashing xlock. */
				if (XPending(dsp) > 100) {	/* 100 is arbitrarily
								   big enough */
					register Status status;

					do {
						status = XCheckMaskEvent(dsp,
									 KeyPressMask | KeyReleaseMask, &event);
					} while (status);
					XBell(dsp, 100);
				}
				break;

#ifdef MOUSE_MOTION
			case MotionNotify:
#endif
			case ButtonPress:
				if (((XButtonEvent *) & event)->window == icon[screen]) {
					return 1;
				}
#ifdef LOGOUT_BUTTON
				if (((XButtonEvent *) & event)->window == button[screen]) {
					XSetFunction(dsp, Scr[screen].gc, GXxor);
					XSetForeground(dsp, Scr[screen].gc, fgcol[screen]);
					XFillRectangle(dsp, button[screen], Scr[screen].gc,
						       0, 0, 500, 100);
					XSync(dsp, False);
					tried_logout = 1;
					logoutUser();
					XSetFunction(dsp, Scr[screen].gc, GXcopy);
				}
#endif
				break;

			case VisibilityNotify:
				if (event.xvisibility.state != VisibilityUnobscured) {
#ifndef DEBUG
					if (!inwindow)
						XRaiseWindow(dsp, win[screen]);
#endif
					call_refresh_hook((LockStruct *) NULL,
							  mode_info(screen, win[screen], False));
					s[0] = '\0';
					return 1;
				}
				break;

			case KeymapNotify:
			case KeyRelease:
			case ButtonRelease:
#ifndef MOUSE_MOTION
			case MotionNotify:
#endif
			case LeaveNotify:
			case EnterNotify:
			case NoExpose:
				break;

			default:
				(void) fprintf(stderr, "%s: unexpected event: %d\n",
					       ProgramName, event.type);
				break;
		}
		first_key = 0;
	}
}

static int
getPassword(void)
{
	XWindowAttributes xgwa;
	int         x, y, left, done;
	char        buffer[PASSLENGTH];

	if (!fullscreen)
		XConfigureWindow(dsp, win[screen], sizeconfiguremask,
				 &fullsizeconfigure);


	(void) XGetWindowAttributes(dsp, win[screen], &xgwa);

	XChangeGrabbedCursor(XCreateFontCursor(dsp, XC_left_ptr));

	XSetForeground(dsp, Scr[screen].gc, bgcol[screen]);
	XFillRectangle(dsp, win[screen], Scr[screen].gc,
		       0, 0, xgwa.width, xgwa.height);

	XMapWindow(dsp, icon[screen]);
	XRaiseWindow(dsp, icon[screen]);

	x = left = iconx[screen] + ICONW + font->max_bounds.width;
	y = icony[screen] + font->ascent;

	putText(win[screen], textgc[screen], text_name, True, left, &x, &y);
	putText(win[screen], textgc[screen], user, False, left, &x, &y);
	putText(win[screen], textgc[screen], "\n", False, left, &x, &y);

	putText(win[screen], textgc[screen], text_pass, True, left, &x, &y);
	putText(win[screen], textgc[screen], " ", False, left, &x, &y);

	passx = x;
	passy = y;

	y = icony[screen] + ICONH + font->ascent + 2;
	x = left = iconx[screen];
	putText(win[screen], textgc[screen], text_info, False, left, &x, &y);
	putText(win[screen], textgc[screen], "\n", False, left, &x, &y);

	XFlush(dsp);

	y += font->ascent + font->descent + 2;

	done = False;
	while (!done) {
#ifdef HAS_RPLAY
		if (!got_invalid)
			rplay_display(infosound);
		got_invalid = 0;
#endif
		if (ReadXString(buffer, PASSLENGTH))
			break;

		XSetForeground(dsp, Scr[screen].gc, bgcol[screen]);

		XFillRectangle(dsp, win[screen], Scr[screen].gc,
			       iconx[screen], y - font->ascent,
			XTextWidth(font, text_invalid, strlen(text_invalid)),
			       font->ascent + font->descent + 2);

		XDrawString(dsp, win[screen], textgc[screen],
			    iconx[screen], y, text_valid, strlen(text_valid));
		XFlush(dsp);

		done = checkPasswd(buffer);

		if (!done && !*buffer) {
			/* just hit return, and it was not his password */
			break;
#ifdef SYSLOG
		} else if (!done) {
			/* bad password... log it... */
			(void) printf("failed unlock attempt on user %s\n", user);
			syslog(LOG_NOTICE, "xlock: failed unlock attempt on user %s\n", user);
#endif
		}
		/* clear plaintext password so you can not grunge around
		   /dev/kmem */
		(void) memset((char *) buffer, 0, sizeof (buffer));

		if (done) {
#ifdef HAS_RPLAY
			rplay_display(validsound);
#endif
			if (!fullscreen)
				XConfigureWindow(dsp, win[screen], sizeconfiguremask,
						 &minisizeconfigure);
			return 0;
		} else {
			XSync(dsp, True);	/* flush input buffer */
			(void) sleep(1);
			XFillRectangle(dsp, win[screen], Scr[screen].gc,
				       iconx[screen], y - font->ascent,
			    XTextWidth(font, text_valid, strlen(text_valid)),
				       font->ascent + font->descent + 2);
			XDrawString(dsp, win[screen], textgc[screen],
			iconx[screen], y, text_invalid, strlen(text_invalid));
			if (echokeys)	/* erase old echo */
				XFillRectangle(dsp, win[screen], Scr[screen].gc,
					       passx, passy - font->ascent,
					       xgwa.width - passx,
					       font->ascent + font->descent);
#ifdef HAS_RPLAY
			rplay_display(invalidsound);
			got_invalid = 1;
#endif
		}
	}
	XChangeGrabbedCursor(mycursor);
	XUnmapWindow(dsp, icon[screen]);
#ifdef LOGOUT_BUTTON
	XUnmapWindow(dsp, button[screen]);
#endif
	if (!fullscreen)
		XConfigureWindow(dsp, win[screen], sizeconfiguremask,
				 &minisizeconfigure);
	return 1;
}


static void
justDisplay(void)
{
	XEvent      event;

	for (screen = 0; screen < screens; screen++) {
		call_init_hook((LockStruct *) NULL, mode_info(screen, win[screen], False));
	}

	do {
		while (!XPending(dsp)) {
#ifdef OLD_EVENT_LOOP
			(void) usleep(onepause ? onepause : delay);
			onepause = 0;
			for (screen = 0; screen < screens; screen++)
				call_callback_hook((LockStruct *) NULL, mode_info(screen,
							win[screen], False));
#ifdef AUTO_LOGOUT
			checkLogout();
#endif
			XSync(dsp, False);
#else /* !OLD_EVENT_LOOP */
#ifdef AUTO_LOGOUT
			if (runMainLoop(30, -1) == 0)
				break;
			checkLogout();
#else
			(void) runMainLoop(0, -1);
#endif
#endif /* !OLD_EVENT_LOOP */
		}
		XNextEvent(dsp, &event);
#ifndef DEBUG
		if (event.type == VisibilityNotify) {
			if (!inwindow) {
				XRaiseWindow(dsp, event.xany.window);
			}
			for (screen = 0; screen < screens; screen++) {
				call_refresh_hook((LockStruct *) NULL,
				      mode_info(screen, win[screen], False));
			}
		}
#endif
		if ((event.type == ButtonPress) &&
		    (((XButtonEvent *) & event)->button == Button2)) {
			for (screen = 0; screen < screens; screen++) {
				call_change_hook((LockStruct *) NULL,
				      mode_info(screen, win[screen], False));
			}
			event.type = 0;		/* dummy out event type */
		}
	} while (event.type != ButtonPress &&
#ifdef MOUSE_MOTION
		 event.type != MotionNotify &&
#endif
		 event.type != KeyPress);
	for (screen = 0; screen < screens; screen++)
		if (event.xbutton.root == RootWindow(dsp, screen))
			break;

	/* KLUDGE SO TVTWM AND VROOT WILL NOT MAKE XLOCK DIE */
	if (screen >= screens)
		screen = 0;

	if (usefirst)
		XPutBackEvent(dsp, &event);
}


static void
sigcatch(void)
{
	finish();
	error("%s: caught terminate signal.\nAccess control list restored.\n");
}


static void
lockDisplay(void)
{
#ifdef HAS_RPLAY
	if (!inwindow && !inroot)
		rplay_display(locksound);
#endif
	if (!allowaccess) {
#if defined( SYSV ) || defined( SVR4 )
		sigset_t    oldsigmask;
		sigset_t    newsigmask;

		(void) sigemptyset(&newsigmask);
		(void) sigaddset(&newsigmask, SIGHUP);
		(void) sigaddset(&newsigmask, SIGINT);
		(void) sigaddset(&newsigmask, SIGQUIT);
		(void) sigaddset(&newsigmask, SIGTERM);
		(void) sigprocmask(SIG_BLOCK, &newsigmask, &oldsigmask);
#else
		int         oldsigmask;

#ifndef VMS
		extern int  sigblock(int);
		extern int  sigsetmask(int);

		oldsigmask = sigblock(sigmask(SIGHUP) |
				      sigmask(SIGINT) |
				      sigmask(SIGQUIT) |
				      sigmask(SIGTERM));
#endif
#endif

		(void) signal(SIGHUP, (void (*)()) sigcatch);
		(void) signal(SIGINT, (void (*)()) sigcatch);
		(void) signal(SIGQUIT, (void (*)()) sigcatch);
		(void) signal(SIGTERM, (void (*)()) sigcatch);
		/* we should trap ALL signals, especially the deadly ones */
		(void) signal(SIGSEGV, (void (*)()) sigcatch);
		(void) signal(SIGBUS, (void (*)()) sigcatch);
		(void) signal(SIGFPE, (void (*)()) sigcatch);

		XGrabHosts(dsp);

#if defined( SYSV ) || defined( SVR4 )
		(void) sigprocmask(SIG_SETMASK, &oldsigmask, &oldsigmask);
#else
		(void) sigsetmask(oldsigmask);
#endif
	}
#if defined( __hpux ) || defined( __apollo )
	XHPDisableReset(dsp);
#endif
	do {
		justDisplay();
	} while (getPassword());
#if defined( __hpux ) || defined( __apollo )
	XHPEnableReset(dsp);
#endif
}

int
main(int argc, char **argv)
{
	XSetWindowAttributes xswa;
	XGCValues   xgcv;
	XColor      nullcolor;

	ProgramName = strrchr(argv[0], '/');
	if (ProgramName)
		ProgramName++;
	else
		ProgramName = argv[0];

#ifdef OSF1_ENH_SEC
	set_auth_parameters(argc, argv);
#endif

#ifdef SYSLOG
	(void) openlog(ProgramName, LOG_PID, LOG_AUTH);
#endif

	start_time = time((long *) 0);
	SRAND(start_time);	/* random mode needs the seed set. */

	GetResources(argc, argv);

	CheckResources();
	start_time += lockdelay;

	initPasswd();

	if (geteuid() == 0 && getuid() != 0) {
		/* revoke root privs */
		(void) setgid(getgid());
		(void) setuid(getuid());
	}
	font = XLoadQueryFont(dsp, fontname);
	if (font == NULL) {
		(void) fprintf(stderr, "%s: can not find font: %s, using %s...\n",
			       ProgramName, fontname, FALLBACK_FONTNAME);
		font = XLoadQueryFont(dsp, FALLBACK_FONTNAME);
		if (font == NULL)
			error("%s: can not even find %s!!!\n", FALLBACK_FONTNAME);
	} {
		int         flags, x, y;
		unsigned int w, h;

		if (*geometry == '\0') {
			fullscreen = True;
		} else {
			flags = XParseGeometry(geometry, &x, &y, &w, &h);
			minisizeconfigure.x = flags & XValue ? x : 0;
			minisizeconfigure.y = flags & YValue ? y : 0;
			minisizeconfigure.width = flags & WidthValue ? w : ICONW;
			minisizeconfigure.height = flags & HeightValue ? h : ICONH;
		}
	}
	screens = ScreenCount(dsp);
	if (screens > MAXSCREENS)
		error("%s: can only support %d screens.\n", MAXSCREENS);

#ifdef DT_SAVER
	/* The CDE Session Manager provides the windows for the screen saver
	   to draw into. */
	if (dtsaver) {
		Window     *saver_wins;
		int         num_wins;
		int         this_win;
		int         this_screen;
		XWindowAttributes xgwa;

		for (this_screen = 0; this_screen < screens; this_screen++)
			win[this_screen] = None;

		/* Get the list of requested windows */
		if (!DtSaverGetWindows(dsp, &saver_wins, &num_wins))
			error("%s: Unable to get screen saver info.\n");
		for (this_win = 0; this_win < num_wins; this_win++) {
			(void) XGetWindowAttributes(dsp, saver_wins[this_win], &xgwa);
			this_screen = XScreenNumberOfScreen(xgwa.screen);
			if (win[this_screen] != None)
				error("%s: Two windows on screen %d\n", this_screen);
			win[this_screen] = saver_wins[this_win];
		}

		/* Reduce to the last screen requested */
		for (this_screen = screens - 1; this_screen >= 0; this_screen--) {
			if (win[this_screen] != None)
				break;
			screens--;
		}
	}
#endif

	for (screen = 0; screen < screens; screen++) {
		Screen     *scr = ScreenOfDisplay(dsp, screen);
		Colormap    cmap = DefaultColormapOfScreen(scr);

		root[screen] = RootWindowOfScreen(scr);
		bgcol[screen] = allocpixel(cmap, background, "White");
		fgcol[screen] = allocpixel(cmap, foreground, "Black");
		rightcol[screen] = allocpixel(cmap, right3d, "Red");
		leftcol[screen] = allocpixel(cmap, left3d, "Green");
		Scr[screen].bgcol = bgcol[screen];
		Scr[screen].fgcol = fgcol[screen];
		Scr[screen].rightcol = rightcol[screen];
		Scr[screen].leftcol = leftcol[screen];
		Scr[screen].cmap = None;

		xswa.override_redirect = True;
		xswa.background_pixel = BlackPixel(dsp, screen);
		xswa.event_mask = KeyPressMask | ButtonPressMask |
#ifdef MOUSE_MOTION
			MotionNotify |
#endif
			VisibilityChangeMask;
#ifdef DEBUG
#define WIDTH WidthOfScreen(scr) - 100
#define HEIGHT HeightOfScreen(scr) - 100
#define CWMASK CWBackPixel | CWEventMask
#else
#define WIDTH (inwindow? WidthOfScreen(scr)/2 : WidthOfScreen(scr))
#define HEIGHT (inwindow? HeightOfScreen(scr)/2 : HeightOfScreen(scr))
#define CWMASK (((inwindow||inroot)? 0 : CWOverrideRedirect) | CWBackPixel | CWEventMask)
#endif
#ifdef USE_VROOT
		if (inroot) {
			win[screen] = root[screen];
			XChangeWindowAttributes(dsp, win[screen], CWBackPixel, &xswa);
		} else
#endif
#ifdef DT_SAVER
		if (!dtsaver)
#endif
		{
			if (fullscreen)
				win[screen] = XCreateWindow(dsp, root[screen], 0, 0,
							    (unsigned int) WIDTH, (unsigned int) HEIGHT, 0,
				 CopyFromParent, InputOutput, CopyFromParent,
							    CWMASK, &xswa);
			else {
				sizeconfiguremask = CWX | CWY | CWWidth | CWHeight;
				fullsizeconfigure.x = 0;
				fullsizeconfigure.y = 0;
				fullsizeconfigure.width = WIDTH;
				fullsizeconfigure.height = HEIGHT;
				win[screen] = XCreateWindow(dsp, root[screen],
						   (int) minisizeconfigure.x,
						   (int) minisizeconfigure.y,
				      (unsigned int) minisizeconfigure.width,
				     (unsigned int) minisizeconfigure.height,
							    0, CopyFromParent, InputOutput, CopyFromParent,
							    CWMASK, &xswa);
			}
		}
#ifdef DEBUG
		{
			XWMHints    xwmh;

			xwmh.flags = InputHint;
			xwmh.input = True;
			XChangeProperty(dsp, win[screen],
			       XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
					(unsigned char *) &xwmh, sizeof (xwmh) / sizeof (int));
		}
#else
		if (inwindow) {
			XWMHints    xwmh;

			xwmh.flags = InputHint;
			xwmh.input = True;
			XChangeProperty(dsp, win[screen],
			       XA_WM_HINTS, XA_WM_HINTS, 32, PropModeReplace,
					(unsigned char *) &xwmh, sizeof (xwmh) / sizeof (int));
		}
#endif

		if (mono || CellsOfScreen(scr) <= 2) {
			Scr[screen].pixels[0] = fgcol[screen];
			Scr[screen].pixels[1] = bgcol[screen];
			Scr[screen].npixels = 2;
		} else
			fix_colormap(win[screen], screen, saturation,
				     install, inroot, inwindow, verbose);

		iconx[screen] = (DisplayWidth(dsp, screen) -
			 XTextWidth(font, text_info, strlen(text_info))) / 2;

		icony[screen] = DisplayHeight(dsp, screen) / 6;

		xswa.border_pixel = WhitePixel(dsp, screen);
		xswa.background_pixel = BlackPixel(dsp, screen);
		xswa.event_mask = ButtonPressMask;
#define CIMASK CWBorderPixel | CWBackPixel | CWEventMask
		if (nolock)
			icon[screen] = None;
		else
			icon[screen] = XCreateWindow(dsp, win[screen],
						iconx[screen], icony[screen],
					     ICONW, ICONH, 1, CopyFromParent,
						 InputOutput, CopyFromParent,
						     CIMASK, &xswa);

#ifdef LOGOUT_BUTTON
		{
			char        buf[1024];
			int         w, h;

			(void) sprintf(buf, " %s ", logoutButtonLabel);
			w = XTextWidth(font, buf, strlen(buf));
			h = font->ascent + font->descent + 2;
			button[screen] = XCreateWindow(dsp, win[screen],
					       0, 0, w, h, 1, CopyFromParent,
						 InputOutput, CopyFromParent,
						       CIMASK, &xswa);
		}
#endif
		XMapWindow(dsp, win[screen]);
		XRaiseWindow(dsp, win[screen]);

#if 0
		if (install && cmap != DefaultColormapOfScreen(scr))
			set_colormap(win[screen], cmap, inwindow);
#endif

		xgcv.font = font->fid;
		xgcv.foreground = WhitePixel(dsp, screen);
		xgcv.background = BlackPixel(dsp, screen);
		Scr[screen].gc = XCreateGC(dsp, win[screen],
				GCFont | GCForeground | GCBackground, &xgcv);

		xgcv.foreground = fgcol[screen];
		xgcv.background = bgcol[screen];
		textgc[screen] = XCreateGC(dsp, win[screen],
				GCFont | GCForeground | GCBackground, &xgcv);
	}
	lockc = XCreateBitmapFromData(dsp, root[0], no_bits, 1, 1);
	lockm = XCreateBitmapFromData(dsp, root[0], no_bits, 1, 1);
	mycursor = XCreatePixmapCursor(dsp, lockc, lockm,
				       &nullcolor, &nullcolor, 0, 0);
	XFreePixmap(dsp, lockc);
	XFreePixmap(dsp, lockm);

	if (!enablesaver) {
		XGetScreenSaver(dsp, &sstimeout, &ssinterval,
				&ssblanking, &ssexposures);
		XResetScreenSaver(dsp);		/* make sure not blank now */
		XSetScreenSaver(dsp, 0, 0, 0, 0);	/* disable screen saver */
	}
#ifndef DEBUG
	if (grabmouse && !inwindow && !inroot) {
		GrabKeyboardAndMouse();
	} else
		nolock = 1;
#endif

	(void) nice(nicelevel);

	(void) XSetIOErrorHandler(xio_error);

	if (nolock)
		justDisplay();
	else if (lockdelay) {
		justDisplay();
		if (start_time < seconds())
			lockDisplay();
		else
			nolock = 1;
	} else
		lockDisplay();

#ifdef SYSLOG
	closelog();
#endif

	finish();

#ifdef VMS
	return 1;
#else
	return 0;
#endif
}
