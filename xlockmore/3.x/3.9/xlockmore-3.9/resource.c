
#ifndef lint
static char sccsid[] = "@(#)resource.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * resource.c - resource management for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 *
 * Changes of David Bagley (bagleyd@hertz.njit.edu)
 *  3-Apr-96: Jouk Jansen <joukj@alpha.chem.uva.nl>.
 *            Supply for wildcards for filenames for random selection
 * 18-Mar-96: Ron Hitchens <ron@utw.com>
 *            Setup chosen mode with set_default_mode() for new hook scheme.
 *  6-Mar-96: Jouk Jansen <joukj@alpha.chem.uva.nl>
 *            Remote node checking for VMS fixed
 * 20-Dec-95: Ron Hitchens <ron@utw.com>
 *            Resource parsing fixed for "nolock".
 * 02-Aug-95: Patch to use default delay, etc., from mode.h thanks to
 *            Roland Bock <exp120@physik.uni-kiel.d400.de>
 * 17-Jun-95: Split out mode.h from resource.c .
 * 29-Mar-95: Added -cycles for more control over a lockscreen similar to
 *            -delay, -batchcount, and -saturation.
 * 21-Feb-95: MANY patches from Heath A. Kehoe <hakehoe@icaen.uiowa.edu>.
 * 21-Dec-94: patch for -delay, -batchcount and -saturation for X11R5+
 *            from Patrick D Sullivan <pds@bss.com>. 
 * 18-Dec-94: -inroot option added from Bill Woodward <wpwood@pencom.com>.
 * 20-Sep-94: added bat mode from Lorenzo Patocchi <patol@info.isbiel.ch>.
 * 11-Jul-94: added grav mode, and inwindow option from Greg Bowering
 *            <greg@cs.adelaide.edu.au>
 * 22-Jun-94: Modified for VMS
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * 17-Jun-94: default changed from life to blank
 * 21-Mar-94: patch fix for AIXV3 from <R.K.Lloyd@csc.liv.ac.uk>
 * 01-Dec-93: added patch for AIXV3 from Tom McConnell
 *            <tmcconne@sedona.intel.com>
 * 29-Jun-93: added spline, maze, sphere, hyper, helix, rock, and blot mode.
 *
 * Changes of Patrick J. Naughton
 * 25-Sep-91: added worm mode.
 * 24-Jun-91: changed name to username.
 * 06-Jun-91: Added flame mode.
 * 24-May-91: Added -name and -usefirst and -resources.
 * 16-May-91: Added random mode and pyro mode.
 * 26-Mar-91: CheckResources: delay must be >= 0.
 * 29-Oct-90: Added #include <ctype.h> for missing isupper() on some OS revs.
 *	      moved -mode option, reordered Xrm database evaluation.
 * 28-Oct-90: Added text strings.
 * 26-Oct-90: Fix bug in mode specific options.
 * 31-Jul-90: Fix ':' handling in parsefilepath
 * 07-Jul-90: Created from resource work in xlock.c
 *
 */

#include "xlock.h"
#include "mode.h"
#include <netdb.h>
#include <math.h>
#include <ctype.h>

#ifndef offsetof
#define offsetof(s,m) ((char*)(&((s *)0)->m)-(char*)0)
#endif

#if defined( AUTO_LOGOUT ) || defined( LOGOUT_BUTTON )
extern int  fullLock();

#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64	/* SunOS 3.5 does not define this */
#endif

extern char *getenv(const char *);

#ifndef DEF_FILESEARCHPATH
#ifdef VMS
#include <descrip>
#include <iodef>
#include <ssdef>
#include <stsdef>

#define DEF_FILESEARCHPATH "DECW$SYSTEM_DEFAULTS:DECW$%N.DAT%S"
#define BUFSIZE 132
#define DECW$C_WS_DSP_TRANSPORT 2	/*  taken from wsdriver.lis */
#define IO$M_WS_DISPLAY 0x00000040	/* taken from wsdriver */

struct descriptor_t {		/* descriptor structure         */
	unsigned short len;
	unsigned char type;
	unsigned char class;
	char       *ptr;
};
typedef struct descriptor_t dsc;

/* $dsc creates a descriptor for a predefined string */

#define $dsc(name,string) dsc name = { sizeof(string)-1,14,1,string}

/* $dscp creates a descriptor pointing to a buffer allocated elsewhere */

#define $dscp(name,size,addr) dsc name = { size,14,1,addr }

static int  descr();

#else
#define DEF_FILESEARCHPATH "/usr/lib/X11/%T/%N%S"
#endif
#endif
#ifndef DEF_MODE
#define DEF_MODE	"blank"
#endif
#ifndef DEF_FONT
#ifdef AIXV3
#define DEF_FONT	"fixed"
#else /* !AIXV3 */
#define DEF_FONT	"-b&h-lucida-medium-r-normal-sans-24-*-*-*-*-*-iso8859-1"
#endif /* !AIXV3 */
#endif
#define DEF_BG		"White"
#define DEF_FG		"Black"
#define DEF_NAME	"Name: "
#define DEF_PASS	"Password: "
#define DEF_INFO	"Enter password to unlock; select icon to lock."
#define DEF_VALID	"Validating login..."
#define DEF_INVALID	"Invalid login."
#define DEF_TIMEOUT	"30"	/* secs until password entry times out */
#define DEF_LOCKDELAY	"0"	/* secs until lock */
#define DEF_DELAY	"200000"	/* microseconds between batches */
#define DEF_BATCHCOUNT	"100"	/* vectors (or whatever) per batch */
#define DEF_CYCLES	"1000"	/* timeout in cycles for a batch */
#define DEF_SATURATION	"1.0"	/* color ramp saturation 0->1 */
#define DEF_DELTA3D	"1.5"	/* space between things in 3d mode relative to their size */
#define DEF_RIGHT3D	"rgb:ffff/0/0"
#define DEF_LEFT3D	"rgb:0/0/ffff"
#define DEF_NICE	"10"	/* xlock process nicelevel */
#define DEF_CLASSNAME	"XLock"
#define DEF_GEOMETRY	""
#ifndef DEF_MESSAGESFILE
#define DEF_MESSAGESFILE ""
#endif
#ifndef DEF_MESSAGEFILE
#define DEF_MESSAGEFILE ""
#endif
#ifndef DEF_MESSAGE
/* #define DEF_MESSAGE "I am out running around." */
#define DEF_MESSAGE ""
#endif
#ifndef DEF_IMAGEFILE
#define DEF_IMAGEFILE ""
#endif
#define DEF_GRIDSIZE  "170"
/*-
  Grid     Number of Neigbors
  ----     ------------------
  Square   4 (or 8)      <- 8 is not implemented
  Hexagon  6
  Triangle (3 or 12)     <- 3 and 12 are not implemented
*/
#define DEF_NEIGHBORS  "4"

#if defined( HAS_RPLAY ) || defined( DEF_PLAY )
#define DEF_LOCKSOUND	"thank-you"
#define DEF_INFOSOUND	"identify-please"
#define DEF_VALIDSOUND	"complete"
#define DEF_INVALIDSOUND	"not-programmed"
#ifndef HAS_RPLAY
#define HAS_RPLAY
#endif
#else
#ifdef VMS_PLAY
#define DEF_LOCKSOUND	"[]thank-you.au"
#define DEF_INFOSOUND	"[]identify-please.au"
#define DEF_VALIDSOUND	"[]complete.au"
#define DEF_INVALIDSOUND	"[]not-programmed.au"
#define HAS_RPLAY
#endif
#endif

#ifdef AUTO_LOGOUT
#define DEF_LOGOUT	"30"	/* minutes until auto-logout */
#endif

#ifdef LOGOUT_BUTTON

#define DEF_BTN_LABEL	"Logout"	/* string that appears in logout button */

/* this string appears immediately below logout button */
#define DEF_BTN_HELP	"Click here to logout"

/* this string appears in place of the logout button if user could not be
   logged out */
#define DEF_FAIL	"Auto-logout failed"

#endif

extern char *ProgramName;
extern Display *dsp;
extern char *program;

/* For modes with text, marquee & nose */
extern char *messagesfile;
extern char *messagefile;
extern char *message;
extern char *mfont;

/* For modes with images, image & puzzle */
extern char *imagefile;

/* For modes automata modes ant & demon */
int         gridsize;
int         neighbors;

static char *mode;
static char *displayname = NULL;
static char *classname;
static char modename[1024];
static char modeclass[1024];
static Bool remote;

#if !defined( VMS ) || defined( XVMSUTILS ) || ( __VMS_VER >= 70000000)
static struct dirent ***images_list;
int         num_list;
struct dirent **image_list;
char        filenam_[256];
char        directory_[512];

#endif

static XrmOptionDescRec genTable[] =
{
	{"-mode", ".mode", XrmoptionSepArg, (caddr_t) NULL},
	{"-nolock", ".nolock", XrmoptionNoArg, (caddr_t) "on"},
	{"+nolock", ".nolock", XrmoptionNoArg, (caddr_t) "off"},
	{"-remote", ".remote", XrmoptionNoArg, (caddr_t) "on"},
	{"+remote", ".remote", XrmoptionNoArg, (caddr_t) "off"},
	{"-mono", ".mono", XrmoptionNoArg, (caddr_t) "on"},
	{"+mono", ".mono", XrmoptionNoArg, (caddr_t) "off"},
	{"-allowroot", ".allowroot", XrmoptionNoArg, (caddr_t) "on"},
	{"+allowroot", ".allowroot", XrmoptionNoArg, (caddr_t) "off"},
	{"-enablesaver", ".enablesaver", XrmoptionNoArg, (caddr_t) "on"},
	{"+enablesaver", ".enablesaver", XrmoptionNoArg, (caddr_t) "off"},
	{"-allowaccess", ".allowaccess", XrmoptionNoArg, (caddr_t) "on"},
	{"+allowaccess", ".allowaccess", XrmoptionNoArg, (caddr_t) "off"},
	{"-grabmouse", ".grabmouse", XrmoptionNoArg, (caddr_t) "on"},
	{"+grabmouse", ".grabmouse", XrmoptionNoArg, (caddr_t) "off"},
	{"-echokeys", ".echokeys", XrmoptionNoArg, (caddr_t) "on"},
	{"+echokeys", ".echokeys", XrmoptionNoArg, (caddr_t) "off"},
	{"-usefirst", ".usefirst", XrmoptionNoArg, (caddr_t) "on"},
	{"+usefirst", ".usefirst", XrmoptionNoArg, (caddr_t) "off"},
	{"-v", ".verbose", XrmoptionNoArg, (caddr_t) "on"},
	{"+v", ".verbose", XrmoptionNoArg, (caddr_t) "off"},
	{"-inwindow", ".inwindow", XrmoptionNoArg, (caddr_t) "on"},
	{"+inwindow", ".inwindow", XrmoptionNoArg, (caddr_t) "off"},
	{"-inroot", ".inroot", XrmoptionNoArg, (caddr_t) "on"},
	{"+inroot", ".inroot", XrmoptionNoArg, (caddr_t) "off"},
	{"-timeelapsed", ".timeelapsed", XrmoptionNoArg, (caddr_t) "on"},
	{"+timeelapsed", ".timeelapsed", XrmoptionNoArg, (caddr_t) "off"},
	{"-install", ".install", XrmoptionNoArg, (caddr_t) "on"},
	{"+install", ".install", XrmoptionNoArg, (caddr_t) "off"},
	{"-nice", ".nice", XrmoptionSepArg, (caddr_t) NULL},
	{"-timeout", ".timeout", XrmoptionSepArg, (caddr_t) NULL},
	{"-lockdelay", ".lockdelay", XrmoptionSepArg, (caddr_t) NULL},
	{"-font", ".font", XrmoptionSepArg, (caddr_t) NULL},
	{"-bg", ".background", XrmoptionSepArg, (caddr_t) NULL},
	{"-fg", ".foreground", XrmoptionSepArg, (caddr_t) NULL},
	{"-background", ".background", XrmoptionSepArg, (caddr_t) NULL},
	{"-foreground", ".foreground", XrmoptionSepArg, (caddr_t) NULL},
	{"-username", ".username", XrmoptionSepArg, (caddr_t) NULL},
	{"-password", ".password", XrmoptionSepArg, (caddr_t) NULL},
	{"-info", ".info", XrmoptionSepArg, (caddr_t) NULL},
	{"-validate", ".validate", XrmoptionSepArg, (caddr_t) NULL},
	{"-invalid", ".invalid", XrmoptionSepArg, (caddr_t) NULL},

	{"-use3d", ".use3d", XrmoptionNoArg, (caddr_t) "on"},
	{"+use3d", ".use3d", XrmoptionNoArg, (caddr_t) "off"},
	{"-delta3d", ".delta3d", XrmoptionSepArg, (caddr_t) NULL},
	{"-right3d", ".right3d", XrmoptionSepArg, (caddr_t) NULL},
	{"-left3d", ".left3d", XrmoptionSepArg, (caddr_t) NULL},

    /* For modes with text, marquee & nose */
	{"-program", ".program", XrmoptionSepArg, (caddr_t) NULL},
	{"-messagesfile", ".messagesfile", XrmoptionSepArg, (caddr_t) NULL},
	{"-messagefile", ".messagefile", XrmoptionSepArg, (caddr_t) NULL},
	{"-message", ".message", XrmoptionSepArg, (caddr_t) NULL},
	{"-mfont", ".mfont", XrmoptionSepArg, (caddr_t) NULL},
    /* For modes with images, image & puzzle */
	{"-imagefile", ".imagefile", XrmoptionSepArg, (caddr_t) NULL},
    /* For automata modes ant & demon */
	{"-gridsize", ".gridsize", XrmoptionSepArg, (caddr_t) NULL},
	{"-neighbors", ".neighbors", XrmoptionSepArg, (caddr_t) NULL},

#ifdef DT_SAVER
	{"-dtsaver", ".dtsaver", XrmoptionNoArg, (caddr_t) "on"},
	{"+dtsaver", ".dtsaver", XrmoptionNoArg, (caddr_t) "off"},
#endif
#ifdef HAS_RPLAY
	{"-locksound", ".locksound", XrmoptionSepArg, (caddr_t) NULL},
	{"-infosound", ".infosound", XrmoptionSepArg, (caddr_t) NULL},
	{"-validsound", ".validsound", XrmoptionSepArg, (caddr_t) NULL},
	{"-invalidsound", ".invalidsound", XrmoptionSepArg, (caddr_t) NULL},
#endif
#ifdef USE_XLOCKRC
	{"-cpasswd", ".cpasswd", XrmoptionSepArg, (caddr_t) NULL},
#endif
	{"-geometry", ".geometry", XrmoptionSepArg, (caddr_t) NULL},
#ifdef AUTO_LOGOUT
	{"-forceLogout", ".forceLogout", XrmoptionSepArg, (caddr_t) NULL},
#endif
#ifdef LOGOUT_BUTTON
{"-logoutButtonLabel", ".logoutButtonLabel", XrmoptionSepArg, (caddr_t) NULL},
 {"-logoutButtonHelp", ".logoutButtonHelp", XrmoptionSepArg, (caddr_t) NULL},
	{"-logoutFailedString", ".logoutFailedString", XrmoptionSepArg, (caddr_t) NULL},
#endif

};

#define genEntries (sizeof genTable / sizeof genTable[0])

static XrmOptionDescRec modeTable[] =
{
	{"-delay", "*delay", XrmoptionSepArg, (caddr_t) NULL},
	{"-batchcount", "*batchcount", XrmoptionSepArg, (caddr_t) NULL},
	{"-cycles", "*cycles", XrmoptionSepArg, (caddr_t) NULL},
	{"-saturation", "*saturation", XrmoptionSepArg, (caddr_t) NULL},
};

#define modeEntries (sizeof modeTable / sizeof modeTable[0])

static XrmOptionDescRec cmdlineTable[] =
{
	{"-display", ".display", XrmoptionSepArg, (caddr_t) NULL},
	{"-nolock", ".nolock", XrmoptionNoArg, (caddr_t) "on"},
	{"+nolock", ".nolock", XrmoptionNoArg, (caddr_t) "off"},
	{"-remote", ".remote", XrmoptionNoArg, (caddr_t) "on"},
	{"+remote", ".remote", XrmoptionNoArg, (caddr_t) "off"},
	{"-inwindow", ".inwindow", XrmoptionNoArg, (caddr_t) "on"},
	{"+inwindow", ".inwindow", XrmoptionNoArg, (caddr_t) "off"},
	{"-inroot", ".inroot", XrmoptionNoArg, (caddr_t) "on"},
	{"+inroot", ".inroot", XrmoptionNoArg, (caddr_t) "off"},
#ifdef DT_SAVER
	{"-dtsaver", ".dtsaver", XrmoptionNoArg, (caddr_t) "on"},
	{"+dtsaver", ".dtsaver", XrmoptionNoArg, (caddr_t) "off"},
#endif
	{"-xrm", NULL, XrmoptionResArg, (caddr_t) NULL}
};

#define cmdlineEntries (sizeof cmdlineTable / sizeof cmdlineTable[0])

static XrmOptionDescRec nameTable[] =
{
	{"-name", ".name", XrmoptionSepArg, (caddr_t) NULL},
};

static OptionStruct opDesc[] =
{
	{"-help", "print out this message"},
	{"-resources", "print default resource file to standard output"},
	{"-display displayname", "X server to contact"},
{"-name resourcename", "class name to use for resources (default is XLock)"},
	{"-/+mono", "turn on/off monochrome override"},
	{"-/+nolock", "turn on/off no password required mode"},
	{"-/+remote", "turn on/off remote host access"},
#ifndef ALWAYS_ALLOW_ROOT
	{"-/+allowroot", "turn on/off allow root password mode"},
#else
	{"-/+allowroot", "turn on/off allow root password mode (ignored)"},
#endif
	{"-/+enablesaver", "turn on/off enable X server screen saver"},
	{"-/+allowaccess", "turn on/off allow new clients to connect"},
	{"-/+grabmouse", "turn on/off grabbing of mouse and keyboard"},
	{"-/+echokeys", "turn on/off echo '?' for each password key"},
	{"-/+usefirst", "turn on/off using the first char typed in password"},
	{"-/+v", "turn on/off verbose mode"},
	{"-/+inwindow", "turn on/off making xlock run in a window"},
	{"-/+inroot", "turn on/off making xlock run in the root window"},
	{"-/+timeelapsed", "turn on/off clock"},
	{"-/+install", "whether to use private colormap if needed (yes/no)"},
	{"-delay usecs", "microsecond delay between screen updates"},
	{"-batchcount num", "number of things per batch"},
	{"-cycles num", "number of cycles per batch"},
	{"-saturation value", "saturation of color ramp"},
	{"-nice level", "nice level for xlock process"},
	{"-timeout seconds", "number of seconds before password times out"},
	{"-lockdelay seconds", "number of seconds until lock"},
	{"-font fontname", "font to use for password prompt"},
	{"-bg color", "background color to use for password prompt"},
	{"-fg color", "foreground color to use for password prompt"},
	{"-username string", "text string to use for Name prompt"},
	{"-password string", "text string to use for Password prompt"},
	{"-info string", "text string to use for instructions"},
  {"-validate string", "text string to use for validating password message"},
      {"-invalid string", "text string to use for invalid password message"},
	{"-geometry geom", "geometry for non-full screen lock"},

	{"-/+use3d", "turn on/off 3d view"},
 {"-delta3d value", "space between rocks in 3d mode relative to their size"},
	{"-right3d color", "colour to be used for the right eye in 3d mode"},
	{"-left3d color", "colour to be used for the left eye in 3d mode"},

    /* For modes with text, marquee & nose */
   {"-program programname", "program to get messages from, usually fortune"},
	{"-messagesfile filename", "formatted file message to say"},
	{"-messagefile filename", "file message to say"},
	{"-message string", "message to say"},
	{"-mfont fontname", "font for a specific mode"},
    /* For modes with image files, image & puzzle */
	{"-imagefile filename", "image file"},
    /* For modes with automata modes ant & demon */
	{"-gridsize", "size of maximum grid"},
	{"-neighbors", "squares 4, hexagons 6"},

#ifdef DT_SAVER
	{"-/+dtsaver", "turn on/off CDE Saver Mode"},
#endif
#ifdef HAS_RPLAY
	{"-locksound string", "sound to use at locktime"},
	{"-infosound string", "sound to use for information"},
	{"-validsound string", "sound to use when password is valid"},
	{"-invalidsound string", "sound to use when password is invalid"},
#endif
};

#define opDescEntries (sizeof opDesc / sizeof opDesc[0])

char       *fontname;
char       *background;
char       *foreground;
char       *text_name;
char       *text_pass;
char       *text_info;
char       *text_valid;
char       *text_invalid;

#ifdef USE_XLOCKRC
char       *cpasswd;

#endif
char       *geometry;
int         nicelevel;
int         delay;
int         batchcount;
float       saturation;
int         cycles;
int         timeout;
int         lockdelay;

Bool        use3d;
float       delta3d;
char       *right3d;
char       *left3d;

#ifdef AUTO_LOGOUT
int         forceLogout;

#endif
#ifdef LOGOUT_BUTTON
int         enable_button = 1;
char       *logoutButtonLabel;
char       *logoutButtonHelp;
char       *logoutFailedString;

#endif
Bool        mono;
Bool        nolock;

#ifdef ALWAYS_ALLOW_ROOT
Bool        allowroot = 1;

#else
Bool        allowroot;

#endif
Bool        enablesaver;
Bool        allowaccess;
Bool        grabmouse;
Bool        echokeys;
Bool        usefirst;
Bool        verbose;
Bool        inwindow;
Bool        inroot;
Bool        timeelapsed;
Bool        install;

#ifdef DT_SAVER
Bool        dtsaver;

#endif
#ifdef HAS_RPLAY
char       *locksound;
char       *infosound;
char       *validsound;
char       *invalidsound;

#endif

static argtype genvars[] =
{
	{(caddr_t *) & fontname, "font", "Font", DEF_FONT, t_String},
    {(caddr_t *) & background, "background", "Background", DEF_BG, t_String},
    {(caddr_t *) & foreground, "foreground", "Foreground", DEF_FG, t_String},
	{(caddr_t *) & text_name, "username", "Username", DEF_NAME, t_String},
	{(caddr_t *) & text_pass, "password", "Password", DEF_PASS, t_String},
	{(caddr_t *) & text_info, "info", "Info", DEF_INFO, t_String},
     {(caddr_t *) & text_valid, "validate", "Validate", DEF_VALID, t_String},
   {(caddr_t *) & text_invalid, "invalid", "Invalid", DEF_INVALID, t_String},
#ifdef USE_XLOCKRC
	{(caddr_t *) & cpasswd, "cpasswd", "cpasswd", "", t_String},
#endif
    {(caddr_t *) & geometry, "geometry", "Geometry", DEF_GEOMETRY, t_String},
	{(caddr_t *) & nicelevel, "nice", "Nice", DEF_NICE, t_Int},
	{(caddr_t *) & timeout, "timeout", "Timeout", DEF_TIMEOUT, t_Int},
   {(caddr_t *) & lockdelay, "lockdelay", "LockDelay", DEF_LOCKDELAY, t_Int},
	{(caddr_t *) & mono, "mono", "Mono", "off", t_Bool},
#ifndef ALWAYS_ALLOW_ROOT
	{(caddr_t *) & allowroot, "allowroot", "AllowRoot", "off", t_Bool},
#endif
    {(caddr_t *) & enablesaver, "enablesaver", "EnableSaver", "off", t_Bool},
    {(caddr_t *) & allowaccess, "allowaccess", "AllowAccess", "off", t_Bool},
	{(caddr_t *) & grabmouse, "grabmouse", "GrabMouse", "on", t_Bool},
	{(caddr_t *) & echokeys, "echokeys", "EchoKeys", "off", t_Bool},
	{(caddr_t *) & usefirst, "usefirst", "Usefirst", "off", t_Bool},
	{(caddr_t *) & verbose, "verbose", "Verbose", "off", t_Bool},
    {(caddr_t *) & timeelapsed, "timeelapsed", "TimeElapsed", "off", t_Bool},
	{(caddr_t *) & install, "install", "Install", "off", t_Bool},

	{(caddr_t *) & use3d, "use3d", "Use3D", "off", t_Bool},
	{(caddr_t *) & delta3d, "delta3d", "Delta3D", DEF_DELTA3D, t_Float},
	{(caddr_t *) & right3d, "right3d", "Right3D", DEF_RIGHT3D, t_String},
	{(caddr_t *) & left3d, "left3d", "Left3D", DEF_LEFT3D, t_String},

	{(caddr_t *) & program, "program", "Program", DEF_PROGRAM, t_String},
	{(caddr_t *) & messagesfile, "messagesfile", "Messagesfile", DEF_MESSAGESFILE, t_String},
	{(caddr_t *) & messagefile, "messagefile", "Messagefile", DEF_MESSAGEFILE, t_String},
	{(caddr_t *) & message, "message", "Message", DEF_MESSAGE, t_String},
	{(caddr_t *) & mfont, "mfont", "MFont", DEF_MFONT, t_String},
{(caddr_t *) & imagefile, "imagefile", "Imagefile", DEF_IMAGEFILE, t_String},
	{(caddr_t *) & gridsize, "gridsize", "GridSize", DEF_GRIDSIZE, t_Int},
   {(caddr_t *) & neighbors, "neighbors", "Neighbors", DEF_NEIGHBORS, t_Int},

#ifdef HAS_RPLAY
{(caddr_t *) & locksound, "locksound", "LockSound", DEF_LOCKSOUND, t_String},
{(caddr_t *) & infosound, "infosound", "InfoSound", DEF_INFOSOUND, t_String},
	{(caddr_t *) & validsound, "validsound", "ValidSound", DEF_VALIDSOUND, t_String},
	{(caddr_t *) & invalidsound, "invalidsound", "InvalidSound", DEF_INVALIDSOUND, t_String},
#endif
#ifdef AUTO_LOGOUT
{(caddr_t *) & forceLogout, "forceLogout", "ForceLogout", DEF_LOGOUT, t_Int},
#endif
#ifdef LOGOUT_BUTTON
	{(caddr_t *) & logoutButtonLabel, "logoutButtonLabel",
	 "LogoutButtonLabel", DEF_BTN_LABEL, t_String},
	{(caddr_t *) & logoutButtonHelp, "logoutButtonHelp",
	 "LogoutButtonHelp", DEF_BTN_HELP, t_String},
	{(caddr_t *) & logoutFailedString, "logoutFailedString",
	 "LogoutFailedString", DEF_FAIL, t_String},
#endif
#if 0
    /* These resources require special handling.  They must be examined
     * before the display is opened.  They are evaluated by individual
     * calls to GetResource(), so they should not be evaluated again here.
     * For example, X-terminals need this special treatment.
     */
	{(caddr_t *) & nolock, "nolock", "NoLock", "off", t_Bool},
	{(caddr_t *) & inwindow, "inwindow", "InWindow", "off", t_Bool},
	{(caddr_t *) & inroot, "inroot", "InRoot", "off", t_Bool},
	{(caddr_t *) & remote, "remote", "Remote", "off", t_Bool},
#endif
};

#define NGENARGS (sizeof genvars / sizeof genvars[0])

static argtype modevars[] =
{
	{(caddr_t *) & delay, "delay", "Delay", DEF_DELAY, t_Int},
{(caddr_t *) & batchcount, "batchcount", "BatchCount", DEF_BATCHCOUNT, t_Int},
	{(caddr_t *) & cycles, "cycles", "Cycles", DEF_CYCLES, t_Int},
	{(caddr_t *) & saturation, "saturation", "Saturation", DEF_SATURATION, t_Float},
};

#define NMODEARGS (sizeof modevars / sizeof modevars[0])

static int  modevaroffs[NMODEARGS] =
{
	offsetof(LockStruct, def_delay),
	offsetof(LockStruct, def_batchcount),
	offsetof(LockStruct, def_cycles),
	offsetof(LockStruct, def_saturation)
};

#ifdef VMS
static char *
stripname(char *string)
{
	char       *characters;

	while (string && *string++ != ']');
	characters = string;
	while (characters)
		if (*characters == '.') {
			*characters = '\0';
			return string;
		} else
			characters++;
	return string;
}
#endif

static void
Syntax(char *badOption)
{
	int         col, len, i;

	(void) fprintf(stderr, "%s:  bad command line option \"%s\"\n\n",
		       ProgramName, badOption);

	(void) fprintf(stderr, "usage:  %s", ProgramName);
	col = 8 + strlen(ProgramName);
	for (i = 0; i < opDescEntries; i++) {
		len = 3 + strlen(opDesc[i].opt);	/* space [ string ] */
		if (col + len > 79) {
			(void) fprintf(stderr, "\n   ");	/* 3 spaces */
			col = 3;
		}
		(void) fprintf(stderr, " [%s]", opDesc[i].opt);
		col += len;
	}

	len = 8 + strlen(LockProcs[0].cmdline_arg);
	if (col + len > 79) {
		(void) fprintf(stderr, "\n   ");	/* 3 spaces */
		col = 3;
	}
	(void) fprintf(stderr, " [-mode %s", LockProcs[0].cmdline_arg);
	col += len;
	for (i = 1; i < numprocs; i++) {
		len = 3 + strlen(LockProcs[i].cmdline_arg);
		if (col + len > 79) {
			(void) fprintf(stderr, "\n   ");	/* 3 spaces */
			col = 3;
		}
		(void) fprintf(stderr, " | %s", LockProcs[i].cmdline_arg);
		col += len;
	}
	(void) fprintf(stderr, "]\n");

	(void) fprintf(stderr, "\nType %s -help for a full description.\n\n",
		       ProgramName);
	exit(1);
}

static void
Help(void)
{
	int         i;

	(void) fprintf(stderr, "usage:\n        %s [-options ...]\n\n",
		       ProgramName);
	(void) fprintf(stderr, "where options include:\n");
	for (i = 0; i < opDescEntries; i++) {
		(void) fprintf(stderr, "    %-28s %s\n",
			       opDesc[i].opt, opDesc[i].desc);
	}

	(void) fprintf(stderr, "    %-28s %s\n", "-mode mode", "animation mode");
	(void) fprintf(stderr, "    where mode is one of:\n");
	for (i = 0; i < numprocs; i++) {
		(void) fprintf(stderr, "          %-23s %s\n",
			       LockProcs[i].cmdline_arg, LockProcs[i].desc);
	}
	(void) putc('\n', stderr);

	exit(0);
}

static void
DumpResources(void)
{
	int         i;

	(void) printf("%s.mode: %s\n", classname, DEF_MODE);

	for (i = 0; i < NGENARGS; i++)
		(void) printf("%s.%s: %s\n",
			      classname, genvars[i].name, genvars[i].def);

	for (i = 0; i < numprocs; i++) {
		(void) printf("%s.%s.%s: %d\n", classname, LockProcs[i].cmdline_arg,
			      "delay", LockProcs[i].def_delay);
		(void) printf("%s.%s.%s: %d\n", classname, LockProcs[i].cmdline_arg,
			      "batchcount", LockProcs[i].def_batchcount);
		(void) printf("%s.%s.%s: %d\n", classname, LockProcs[i].cmdline_arg,
			      "cycles", LockProcs[i].def_cycles);
		(void) printf("%s.%s.%s: %g\n", classname, LockProcs[i].cmdline_arg,
			      "saturation", LockProcs[i].def_saturation);
	}
	exit(0);
}


static void
LowerString(char *s)
{

	while (*s) {
		if (isupper(*s))
			*s += ('a' - 'A');
		s++;
	}
}

static void
GetResource(XrmDatabase database, char *parentname, char *parentclass, char *name, char *class, int valueType, char *def, caddr_t * valuep)
{
	char       *type;
	XrmValue    value;
	char       *string;
	char        buffer[1024];
	char        fullname[1024];
	char        fullclass[1024];
	int         len;

	(void) sprintf(fullname, "%s.%s", parentname, name);
	(void) sprintf(fullclass, "%s.%s", parentclass, class);
	if (XrmGetResource(database, fullname, fullclass, &type, &value)) {
		string = value.addr;
		len = value.size;
	} else {
		string = def;
		if (string == NULL) {
			*valuep = NULL;
			return;
		}
		len = strlen(string);
	}
	(void) strncpy(buffer, string, sizeof (buffer));
	buffer[sizeof (buffer) - 1] = '\0';

	switch (valueType) {
		case t_String:
			{
				char       *s = (char *) malloc(len + 1);

				if (s == (char *) NULL)
					error("%s: GetResource - could not allocate memory");
				(void) strncpy(s, string, len);
				s[len] = '\0';
				*((char **) valuep) = s;
			}
			break;
		case t_Bool:
			LowerString(buffer);
			*((int *) valuep) = (!strcmp(buffer, "true") ||
					     !strcmp(buffer, "on") ||
					     !strcmp(buffer, "enabled") ||
				      !strcmp(buffer, "yes")) ? True : False;
			break;
		case t_Int:
			*((int *) valuep) = atoi(buffer);
			break;
		case t_Float:
			*((float *) valuep) = (float) atof(buffer);
			break;
	}
}


static      XrmDatabase
parsefilepath(char *xfilesearchpath, char *TypeName, char *ClassName)
{
	XrmDatabase database = NULL;
	char        appdefaults[1024];
	char       *src;
	char       *dst;

	src = xfilesearchpath;
	appdefaults[0] = '\0';
	dst = appdefaults;
	for (;;) {
		if (*src == '%') {
			src++;
			switch (*src) {
				case '%':
				case ':':
					*dst++ = *src++;
					*dst = '\0';
					break;
				case 'T':
					(void) strcat(dst, TypeName);
					src++;
					dst += strlen(TypeName);
					break;
				case 'N':
					(void) strcat(dst, ClassName);
					src++;
					dst += strlen(ClassName);
					break;
				case 'S':
					src++;
					break;
				default:
					src++;
					break;
			}
#ifdef VMS
		} else if (*src == '#') {	/* Colons required in VMS use # */
#else
		} else if (*src == ':') {
#endif
			database = XrmGetFileDatabase(appdefaults);
			if (database == NULL) {
				dst = appdefaults;
				src++;
			} else
				break;
		} else if (*src == '\0') {
			database = XrmGetFileDatabase(appdefaults);
			break;
		} else {
			*dst++ = *src++;
			*dst = '\0';
		}
	}
	return database;
}


static void
open_display(void)
{
	if (!(dsp = XOpenDisplay(displayname)))
		error("%s: unable to open display %s.\n", displayname);

	displayname = DisplayString(dsp);

	/*
	 * only restrict access to other displays if we are locking and if the
	 * Remote resource is not set.
	 */
	if (nolock || inwindow || inroot)
		remote = True;

#ifdef VMS
	if (!remote && ((displayname[0] == '_') |
			((displayname[0] == 'W') &
			 (displayname[1] == 'S') &
			 (displayname[2] == 'A')))) {	/* this implies a v5.4 system. The return
							   value is a device name, which must be
							   interrogated to find the real information */
		unsigned long chan;
		unsigned int status;
		unsigned short len;
		char        server_transport[100];

		status = sys$assign(descr(displayname), &chan, 0, 0);
		if (!(status & 1))
			displayname = " ";
		else {
			status = get_info(chan, DECW$C_WS_DSP_TRANSPORT,
					  server_transport, &len);
			if (!(status & 1))
				exit(status);

			if (strcmp(server_transport, "LOCAL")) {
				strcat(displayname, "'s display via ");
				strncat(displayname, server_transport, len);
				error("%s: can not lock %s\n", displayname);
			}
		}
	} else {
#endif /* VMS */

		if (displayname != NULL) {
			char       *colon = (char *) strchr(displayname, ':');
			int         n = colon - displayname;

			if (colon == NULL)
				error("%s: Malformed -display argument, \"%s\"\n", displayname);

			if (!remote && n
			    && strncmp(displayname, "unix", n)
			    && strncmp(displayname, "localhost", n)) {
				char        hostname[MAXHOSTNAMELEN];
				struct hostent *host;
				char      **hp;
				int         badhost = 1;
				extern int  gethostname(char *, size_t);

				if (gethostname(hostname, MAXHOSTNAMELEN))
					error("%s: Can not get local hostname.\n");

				if (!(host = gethostbyname(hostname)))
					error("%s: Can not get hostbyname.\n");

				if (strncmp(displayname, host->h_name, n)) {
					for (hp = host->h_aliases; *hp; hp++) {
						if (!strncmp(displayname, *hp, n)) {
							badhost = 0;
							break;
						}
					}
					if (badhost) {
						*colon = (char) 0;
						error("%s: can not lock %s's display\n", displayname);
					}
				}
			}
		}
#ifdef VMS
	}
#endif
}

static void
printvar(char *class, argtype var)
{
	switch (var.type) {
		case t_String:
			(void) fprintf(stderr, "%s.%s: %s\n",
				       class, var.name, *((char **) var.var));
			break;
		case t_Bool:
			(void) fprintf(stderr, "%s.%s: %s\n",
				       class, var.name, *((int *) var.var)
				       ? "True" : "False");
			break;
		case t_Int:
			(void) fprintf(stderr, "%s.%s: %d\n",
				       class, var.name, *((int *) var.var));
			break;
		case t_Float:
			(void) fprintf(stderr, "%s.%s: %g\n",
				       class, var.name, *((float *) var.var));
			break;
	}
}


void
GetResources(int argc, char **argv)
{
	XrmDatabase RDB = NULL;
	XrmDatabase nameDB = NULL;
	XrmDatabase modeDB = NULL;
	XrmDatabase cmdlineDB = NULL;
	XrmDatabase generalDB = NULL;
	XrmDatabase homeDB = NULL;
	XrmDatabase applicationDB = NULL;
	XrmDatabase serverDB = NULL;
	XrmDatabase userDB = NULL;
	char        userfile[1024];
	char       *homeenv;
	char       *userpath;
	char       *env;
	char       *serverString;
	int         i, j;
	extern Display *dsp;

	XrmInitialize();

	for (i = 0; i < argc; i++) {
		if (!strncmp(argv[i], "-help", strlen(argv[i])))
			Help();
		/* NOTREACHED */
	}

	/*
	 * get -name arg from command line so you can have different resource
	 * files for different configurations/machines etc...
	 */
#ifdef VMS
	/*Strip off directory and .exe; parts */
	ProgramName = stripname(ProgramName);
#endif
	XrmParseCommand(&nameDB, nameTable, 1, ProgramName,
			&argc, argv);
	GetResource(nameDB, ProgramName, "*", "name", "Name", t_String,
		    DEF_CLASSNAME, &classname);


	homeenv = getenv("HOME");
	if (!homeenv)
		homeenv = "";

	env = getenv("XFILESEARCHPATH");
	applicationDB = parsefilepath(env ? env : DEF_FILESEARCHPATH,
				      "app-defaults", classname);

	XrmParseCommand(&cmdlineDB, cmdlineTable, cmdlineEntries, ProgramName,
			&argc, argv);

	userpath = getenv("XUSERFILESEARCHPATH");
	if (!userpath) {
		env = getenv("XAPPLRESDIR");
		if (env)
			(void) sprintf(userfile, "%s/%%N:%s/%%N", env, homeenv);
		else
#ifdef VMS
			(void) sprintf(userfile, "%sDECW$%%N.DAT#%sDECW$XDEFAULTS.DAT",
				       homeenv, homeenv);
#else
			(void) sprintf(userfile, "%s/%%N", homeenv);
#endif
		userpath = userfile;
	}
	userDB = parsefilepath(userpath, "app-defaults", classname);

	(void) XrmMergeDatabases(applicationDB, &RDB);
	(void) XrmMergeDatabases(userDB, &RDB);
	(void) XrmMergeDatabases(cmdlineDB, &RDB);

	GetResource(RDB, ProgramName, classname, "display", "Display", t_String,
		    "", &displayname);
	GetResource(RDB, ProgramName, classname, "nolock", "NoLock", t_Bool,
		    "off", (caddr_t *) & nolock);
	GetResource(RDB, ProgramName, classname, "remote", "Remote", t_Bool,
		    "off", (caddr_t *) & remote);
	GetResource(RDB, ProgramName, classname, "inwindow", "InWindow", t_Bool,
		    "off", (caddr_t *) & inwindow);
	GetResource(RDB, ProgramName, classname, "inroot", "InRoot", t_Bool,
		    "off", (caddr_t *) & inroot);
	GetResource(RDB, ProgramName, classname, "mono", "Mono", t_Bool,
		    "off", (caddr_t *) & mono);
#ifdef DT_SAVER
	GetResource(RDB, ProgramName, classname, "dtsaver", "DtSaver", t_Bool,
		    "off", (caddr_t *) & dtsaver);
	if (dtsaver) {
		inroot = False;
		inwindow = True;
		nolock = True;
	}
#endif

	open_display();
	serverString = XResourceManagerString(dsp);
	if (serverString) {
		serverDB = XrmGetStringDatabase(serverString);
		(void) XrmMergeDatabases(serverDB, &RDB);
	} else {
		char        buf[1024];

		(void) sprintf(buf, "%s/.Xdefaults", homeenv);
		homeDB = XrmGetFileDatabase(buf);
		(void) XrmMergeDatabases(homeDB, &RDB);
	}

	XrmParseCommand(&generalDB, genTable, genEntries, ProgramName, &argc, argv);
	(void) XrmMergeDatabases(generalDB, &RDB);

	GetResource(RDB, ProgramName, classname, "mode", "Mode", t_String,
		    DEF_MODE, (caddr_t *) & mode);

	XrmParseCommand(&modeDB, modeTable, modeEntries, ProgramName, &argc, argv);
	(void) XrmMergeDatabases(modeDB, &RDB);

	for (i = 0; i < numprocs; i++) {
		XrmDatabase optDB = NULL;
		ModeSpecOpt *ms = LockProcs[i].msopt;

		if (!ms->numopts)
			continue;
		XrmParseCommand(&optDB, ms->opts, ms->numopts,
				ProgramName, &argc, argv);
		(void) XrmMergeDatabases(optDB, &RDB);
	}

	/* the RDB is set, now query load the variables from the database */

	for (i = 0; i < NGENARGS; i++)
		GetResource(RDB, ProgramName, classname,
			    genvars[i].name, genvars[i].class,
			    genvars[i].type, genvars[i].def, genvars[i].var);

	for (i = 0; i < numprocs; i++) {
		argtype    *v;
		ModeSpecOpt *ms = LockProcs[i].msopt;

		(void) sprintf(modename, "%s.%s", ProgramName, LockProcs[i].cmdline_arg);
		(void) sprintf(modeclass, "%s.%s", classname, LockProcs[i].cmdline_arg);
		for (j = 0; j < NMODEARGS; j++) {
			char        buf[16];
			void       *p = (void *) ((char *) (&LockProcs[i]) + modevaroffs[j]);

			if (modevars[j].type == t_Int)
				(void) sprintf(buf, "%d", *(int *) p);
			else
				(void) sprintf(buf, "%g", *(float *) p);
			GetResource(RDB, modename, modeclass,
				    modevars[j].name, modevars[j].class,
				    modevars[j].type, buf, (caddr_t *) p);
			if (!strcmp(mode, LockProcs[i].cmdline_arg)) {
				GetResource(RDB, modename, modeclass,
					 modevars[j].name, modevars[j].class,
				     modevars[j].type, buf, modevars[j].var);
			}
		}
		if (!ms->numopts)
			continue;
		v = ms->vars;
		for (j = 0; j < ms->numopts; j++)
			GetResource(RDB, modename, modeclass, v[j].name, v[j].class,
				    v[j].type, v[j].def, v[j].var);
	}

	/*XrmPutFileDatabase(RDB, "/tmp/xlock.rsrc.out"); */
	(void) XrmDestroyDatabase(RDB);

	/* Parse the rest of the command line */
	for (argc--, argv++; argc > 0; argc--, argv++) {
		if (**argv != '-')
			Syntax(*argv);
		switch (argv[0][1]) {
			case 'r':
				DumpResources();
				/* NOTREACHED */
			default:
				Syntax(*argv);
				/* NOTREACHED */
		}
	}

#if defined( AUTO_LOGOUT ) || defined( LOGOUT_BUTTON )
	if (fullLock()) {
#ifdef AUTO_LOGOUT
		forceLogout = 0;
#endif
#ifdef LOGOUT_BUTTON
		enable_button = 0;
#endif
	}
#endif
#ifdef DT_SAVER
	if (dtsaver) {
		enablesaver = True;
		geometry = DEF_GEOMETRY;
		grabmouse = False;
		inroot = False;
		install = False;
		inwindow = True;
		lockdelay = 0;
		nolock = True;
	}
#endif

	if (verbose) {
		for (i = 0; i < NGENARGS; i++)
			printvar(classname, genvars[i]);
		for (i = 0; i < NMODEARGS; i++)
			printvar(modename, modevars[i]);
	}
#if !defined( VMS ) || defined( XVMSUTILS ) ||  ( __VMS_VER >= 70000000)
	/* Evaluate imagefile */
	if (strcmp(imagefile, DEF_IMAGEFILE)) {
		extern void get_direc(char *fullpath, char *dir, char *filn_);
		extern int  sel_image(struct dirent *name);
		extern int  scan_dir(const char *directoryname, struct dirent ***namelist,
				     int         (*select) (struct dirent *),
			int         (*compare) (const void *, const void *));

		get_direc(imagefile, directory_, filenam_);
		images_list = (struct dirent ***) malloc(sizeof (struct dirent **));

		num_list = scan_dir(directory_, images_list, sel_image, NULL);
		image_list = *images_list;
#ifdef DEBUG
		for (i = 0; i < num_list; i++)
			(void) printf("File no %d : %s\n", i, image_list[i]->d_name);
#endif
	} else
		num_list = 0;
#endif
}


void
CheckResources(void)
{
	int         i;

	if (delay < 0)
		Syntax("-delay argument must be positive.");
	if (batchcount < 0)
		Syntax("-batchcount argument must be positive.");
	if (cycles < 0)
		Syntax("-cycles argument must be positive.");
	if (saturation < 0.0 || saturation > 1.0)
		Syntax("-saturation argument must be between 0.0 and 1.0.");
	if (delta3d < 0.0 || delta3d > 20.0)
		Syntax("-delta3d argument must be between 0.0 and 20.0.");

	/* in case they have a 'xlock*mode: ' empty resource */
	if (!mode || *mode == '\0')
		mode = DEF_MODE;

	for (i = 0; i < numprocs; i++) {
		if (!strncmp(LockProcs[i].cmdline_arg, mode, strlen(mode))) {
			set_default_mode(&LockProcs[i]);
			break;
		}
	}
	if (i == numprocs) {
		(void) fprintf(stderr, "Unknown mode: ");
		Syntax(mode);
	}
}

#ifdef VMS
/* 
 * FUNCTIONAL DESCRIPTION:
 * int get_info (chan, item, ret_str, ret_len)
 * Fetch a single characteristics from the pseudo-workstation
 * device and return the information.
 * (Taken and modified from the v5.4 fiche. PPL/SG 2/10/91
 * FORMAL PARAMETERS:
 *      chan:   the device channel
 *      item:   the characteristic to show
 *      ret_str: str pointer to information
 * ret_len: length of above string
 *  IMPLICIT INPUTS:
 *      none
 *  IMPLICIT OUTPUTS:
 *      none
 *  COMPLETION CODES:
 * errors returned by SYS$QIO
 *  SIDE EFFECTS:
 *      none
 * Hacked from Steve Garrett's xservername (as posted to INFO-VAX)
 */

int
get_info(unsigned long chan, unsigned long item,
	 char *ret_str, unsigned long *ret_len)
{
	unsigned long iosb[2];
	int         status;
	char        itembuf[BUFSIZE];
	struct dsc$descriptor itemval;

	itemval.dsc$w_length = BUFSIZE;
	itemval.dsc$b_dtype = 0;
	itemval.dsc$b_class = 0;
	itemval.dsc$a_pointer = &itembuf[0];

	status = sys$qiow(0, chan, IO$_SENSEMODE | IO$M_WS_DISPLAY, &iosb, 0, 0,
			  itemval.dsc$a_pointer,
			  itemval.dsc$w_length,
			  item, 0, 0, 0);
	if (status != SS$_NORMAL)
		return (status);
	if (iosb[0] != SS$_NORMAL)
		return (iosb[0]);

	itemval.dsc$w_length = iosb[1];
	*ret_len = iosb[1];
	itembuf[*ret_len] = 0;
	(void) strcpy(ret_str, &itembuf[0]);

	return (status);
}


/* routine that will return descripter of asciz string */

static int
descr(char *name)
{
	static      $dscp(d1, 0, 0);
	static      $dscp(d2, 0, 0);
	static      $dscp(d3, 0, 0);
	static      $dscp(d4, 0, 0);
	static      $dscp(d5, 0, 0);
	static      $dscp(d6, 0, 0);
	static dsc *tbl[] =
	{&d1, &d2, &d3, &d4, &d5, &d6};
	static int  didx = 0;

	if (didx == 6)
		didx = 0;

	tbl[didx]->len = strlen(name);
	tbl[didx]->ptr = name;

	return tbl[didx++];
}
#endif
