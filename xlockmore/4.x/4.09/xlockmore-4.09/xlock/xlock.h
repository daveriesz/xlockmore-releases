#ifndef __XLOCK_XLOCK_H__
#define __XLOCK_XLOCK_H__

/*-
 * @(#)xlock.h	4.00 97/01/01 xlockmore 
 *
 * xlock.h - external interfaces for new modes and SYSV OS defines.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *
 * Changes maintained by David Bagley <bagleyd@bigfoot.com>
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
 * 01-Dec-93: added patch for AIXV3 from Tom McConnell
 *            <tmcconne@sedona.intel.com> also added a patch for HP-UX 8.0.
 *
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xresource.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#ifdef USE_MB
#ifdef __linux__
#define X_LOCALE
#endif
#include <X11/Xlocale.h>
#endif
#else /* HAVE_CONFIG_H */

/* THIS MAY SOON BE DEFUNCT, SHOULD WORK NOW THOUGH FOR IMAKE */
#define inline
#ifdef AIXV3
#define HAVE_SYS_SELECT_H 1
#else
#define HAVE_SYS_TIME_H 1
#endif
#if !defined( __hpux ) && !defined( apollo ) && !defined( VMS )
#define HAVE_SETEUID
#endif
#define HAVE_FCNTL_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SYSLOG_H 1
#define HAVE_GETHOSTNAME 1
#define HAVE_UNISTD_H 1
#ifdef VMS
#if ( __VMS_VER < 70000000 )
#ifdef USE_XVMSUTILS
#define HAVE_DIRENT_H 1
#define HAVE_GETTIMEOFDAY 1
#define GETTIMEOFDAY_TWO_ARGS 1
#else /* !USE_XVMSUTILS */
#ifndef USE_OLD_EVENT_LOOP
#define USE_OLD_EVENT_LOOP
#endif /* USE_OLD_EVENT_LOOP */
#define HAVE_DIRENT_H 0
#define HAVE_GETTIMEOFDAY 0
#endif /* !USE_XVMSUTILS */
#define HAVE_STRDUP 0
#else /* __VMS_VER >= 70000000 */
#define HAVE_DIRENT_H 1
#define HAVE_GETTIMEOFDAY 1
#define GETTIMEOFDAY_TWO_ARGS 1
#define HAVE_STRDUP 1
#endif /* __VMS_VER >= 70000000 */
#else /* !VMS */
#define HAVE_DIRENT_H 1
#define HAVE_MEMORY_H 1
#define HAVE_GETTIMEOFDAY 1
#define GETTIMEOFDAY_TWO_ARGS 1
#ifdef ultrix
#define HAVE_STRDUP 0
#else /* !ultrix */
#define HAVE_STRDUP 1
#endif /* !ultrix */
#endif /* !VMS */
#endif /* !HAVE_CONF_H */

#ifndef MESSAGELINES
#define MESSAGELINES      40
#endif
#define PASSLENGTH        120
#define FALLBACK_FONTNAME "fixed"
#ifndef DEF_MFONT
#define DEF_MFONT "-*-times-*-*-*-*-18-*-*-*-*-*-*-*"
#endif
#ifndef DEF_PROGRAM		/* Try the -o option ;) */
#define DEF_PROGRAM "fortune -s"
#endif

#ifndef DEF_NONE3D
#define DEF_NONE3D "Black"
#endif
#ifndef DEF_RIGHT3D
#define DEF_RIGHT3D "Red"
#endif
#ifndef DEF_LEFT3D
#define DEF_LEFT3D "Blue"
#endif
#ifndef DEF_BOTH3D
#define DEF_BOTH3D "Magenta"
#endif

#ifndef DEF_ICONW
#define DEF_ICONW         64	/* Age old default */
#endif
#ifndef DEF_ICONH
#define DEF_ICONH         64
#endif

#ifndef DEF_GLW
#define DEF_GLW         640
#endif
#ifndef DEF_GLH
#define DEF_GLH         480
#endif

#define MINICONW          1	/* Will not see much */
#define MINICONH          1

#ifndef MAXICONW
#define MAXICONW          256	/* Want users to know the screen is locked */
#endif
#ifndef MAXICONH
#define MAXICONH          256	/* by a particular user */
#endif

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#ifndef ABS
#define ABS(a)  ((a<0)?(-(a)):(a))
#endif

#include <math.h>
#ifndef M_E
#define M_E    2.7182818284590452354
#endif
#ifndef M_PI
#define M_PI   3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif
#if VMS
#include <unixlib.h>
#endif
#include <sys/types.h>
#if 0
#ifndef uid_t
#define uid_t int
#endif
#ifndef gid_t
#define gid_t int
#endif
#ifndef pid_t
#define pid_t int
#endif
#ifndef size_t
#define size_t unsigned
#endif
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if (defined( SYSV ) || defined( SVR4 )) && defined( SOLARIS2 ) && !defined( LESS_THAN_SOLARIS2_5 )
struct hostent {
	char       *h_name;	/* official name of host */
	char      **h_aliases;	/* alias list */
	int         h_addrtype;	/* host address type */
	int         h_length;	/* length of address */
	char      **h_addr_list;	/* list of addresses from name server */
};

#else
#include <netdb.h>		/* Gives problems on Solaris 2.6 with gcc */
#endif
#include <ctype.h>
#if HAVE_DIRENT_H
#ifdef USE_XVMSUTILS
#if 0
#include "../xvmsutils/unix_types.h"
#include "../xvmsutils/dirent.h"
#else
#include <X11/unix_types.h>
#include <X11/dirent.h>
#endif
#else /* !USE_XVMSUTILS */
#include <dirent.h>
#endif /* !USE_XVMSUTILS */
#else
#define dirent direct
#define NAMELEN(dirent) (dirent)->d_namelen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#if HAVE_GETTIMEOFDAY
#ifdef GETTIMEOFDAY_TWO_ARGS
#define GETTIMEOFDAY(t) (void)gettimeofday(t,NULL);
#else
#define GETTIMEOFDAY(t) (void)gettimeofday(t);
#endif
#endif
#if defined(_SYSTYPE_SVR4) && !defined(SVR4)	/* For SGI */
#define SVR4
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64	/* SunOS 3.5 does not define this */
#endif

#ifndef MAXNAMLEN
#define MAXNAMLEN       256	/* maximum filename length */
#endif
#ifndef DIRBUF
#define DIRBUF          512	/* buffer size for fs-indep. dirs */
#endif

#if defined(__cplusplus) || defined(c_plusplus)
#define CLASS c_class
#else
#define CLASS class
#endif

#if (!defined( AFS ) && defined( HAVE_SHADOW ) && !defined( OSF1_ENH_SEC ) && !defined( HP_PASSWDETC ) && !defined( VMS ) && !defined( PAM ))
#define FALLBACK_XLOCKRC
#endif

#define t_String        0
#define t_Float         1
#define t_Int           2
#define t_Bool          3

typedef struct {
	caddr_t    *var;
	char       *name;
	char       *classname;
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
	int         numvarsdesc;
	argtype    *vars;
	OptionStruct *desc;
} ModeSpecOpt;

#ifdef USE_MODULES
typedef struct {
	char       *cmdline_arg;	/* mode name */
	char       *init_name;	/* name of init a mode */
	char       *callback_name;	/* name of run (tick) a mode */
	char       *release_name;	/* name of shutdown of a mode */
	char       *refresh_name;	/* name of mode to repaint */
	char       *change_name;	/* name of mode to change */
	char       *unused_name;	/* name for future expansion */
	ModeSpecOpt *msopt;	/* this mode's def resources */
	int         def_delay;	/* default delay for mode */
	int         def_batchcount;
	int         def_cycles;
	int         def_size;
	int         def_ncolors;
	float       def_saturation;
	char       *def_imagefile;
	char       *desc;	/* text description of mode */
	unsigned int flags;	/* state flags for this mode */
	void       *userdata;	/* for use by the mode */
} ModStruct;

#endif

/* this must follow definition of ModeSpecOpt */
#include "mode.h"

#define IS_NONE 0
#define IS_XBMDONE 1		/* Only need one mono image */
#define IS_XBM 2
#define IS_XBMFILE 3
#define IS_XPM 4
#define IS_XPMFILE 5
#define IS_RASTERFILE 6

/* There is some overlap so it can be made more efficient */
#define ERASE_IMAGE(d,w,g,x,y,xl,yl,xs,ys) \
if (yl<y) \
(y<(yl+ys))?XFillRectangle(d,w,g,xl,yl,xs,y-(yl)): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
else if (yl>y) \
(y>(yl-(ys)))?XFillRectangle(d,w,g,xl,y+ys,xs,yl-(y)): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
if (xl<x) \
(x<(xl+xs))?XFillRectangle(d,w,g,xl,yl,x-(xl),ys): \
XFillRectangle(d,w,g,xl,yl,xs,ys); \
else if (xl>x) \
(x>(xl-(xs)))?XFillRectangle(d,w,g,x+xs,yl,xl-(x),ys): \
XFillRectangle(d,w,g,xl,yl,xs,ys)

extern void getResources(int argc, char **argv);
extern unsigned long allocPixel(Display * display, Colormap cmap,
				char *name, char *def);

extern void setColormap(Display * display, Window window, Colormap map,
			Bool inwindow);
extern void fixColormap(Display * display, Window window,
			int screen, int ncolors, float saturation,
	  Bool mono, Bool install, Bool inroot, Bool inwindow, Bool verbose);
extern int  preserveColors(unsigned long black, unsigned long white,
			   unsigned long bg, unsigned long fg);
extern Bool setupColormap(ModeInfo * mi, int *colors, Bool * truecolor,
  unsigned long *redmask, unsigned long *bluemask, unsigned long *greenmask);
extern int  visualClassFromName(char *name);
extern char *nameOfVisualClass(int visualclass);

extern Bool fixedColors(ModeInfo * mi);

extern void getImage(ModeInfo * mi, XImage ** logo,
	  int default_width, int default_height, unsigned char *default_bits,
#if defined( USE_XPM ) || defined( USE_XPMINC )
		     int default_xpm, char **name,
#endif
		     int *graphics_format, Colormap * newcolormap,
		     unsigned long *black);
extern void destroyImage(XImage ** logo, int *graphics_format);
extern void getPixmap(Display * display, Drawable drawable,
	  int default_width, int default_height, unsigned char *default_bits,
		      int *width, int *height, Pixmap * pixmap,
		      int *graphics_format);

#if defined( USE_XPM ) || defined( USE_XPMINC )
extern void reserveColors(ModeInfo * mi, Colormap cmap,
			  unsigned long *black);

#endif

#ifdef USE_GL
#include <GL/gl.h>
#include <GL/glx.h>

extern GLXContext *init_GL(ModeInfo * mi);
extern void FreeAllGL(ModeInfo * mi);

#endif

#ifdef OPENGL_MESA_INCLUDES
/* Allow OPEN GL using MESA includes */
#undef MESA
#endif

extern XPoint hexagonUnit[6];
extern XPoint triangleUnit[2][3];

#define NUMSTIPPLES 11
#define STIPPLESIZE 8
extern unsigned char stipples[NUMSTIPPLES][STIPPLESIZE];

extern unsigned long seconds(void);
extern void finish(Display * display, Bool closeDisplay);
extern void error(char *buf);

extern FILE *my_fopen(char *, char *);

#if (! HAVE_STRDUP )
extern char *strdup(char *);

#endif

#ifdef VMS
#define FORK vfork
#else
#define FORK fork
#endif

#ifdef LESS_THAN_AIX3_2
#undef NULL
#define NULL 0
#endif /* LESS_THAN_AIX3_2 */

#ifndef SYSLOG_WARNING
#define SYSLOG_WARNING LOG_WARNING
#endif
#ifndef SYSLOG_NOTICE
#define SYSLOG_NOTICE LOG_NOTICE
#endif
#ifndef SYSLOG_INFO
#define SYSLOG_INFO LOG_INFO
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
#if HAVE_RAND48
#define SRAND srand48
#define LRAND lrand48
#define MAXRAND (2147483648.0)
#else /* HAVE_RAND48 */
#if HAVE_RANDOM
#define SRAND srand48
#define LRAND lrand48
#define MAXRAND (2147483648.0)
#else /* HAVE_RANDOM */
#if HAVE_RAND
#define SRAND srand48
#define LRAND lrand48
#ifdef AIXV3
#define MAXRAND (2147483648.0)
#else
#define MAXRAND (32768.0)
#endif
#endif /* HAVE_RAND */
#endif /* HAVE_RANDOM */
#endif /* HAVE_RAND48 */

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

#if (defined( USE_RPLAY ) || defined( USE_NAS ) || defined( USE_VMSPLAY ) || defined( DEF_PLAY ))
#define USE_SOUND
#endif

#ifdef USE_MB
extern XFontSet fontset;

#define XTextWidth(font,string,length) \
		XmbTextEscapement(fontset,string,length)
#define XDrawString(display,window,gc,x,y,string,length) \
		XmbDrawString(display,window,fontset,gc,x,y,string,length)
#define XDrawImageString(display,window,gc,x,y,string,length) \
		XmbDrawImageString(display,window,fontset,gc,x,y,string,length)
#endif
#endif /* __XLOCK_XLOCK_H__ */
