/*
 * @(#)xlock.h 1.6 90/10/28 XLOCK SMI
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#define MAXSCREENS 3
#define NUMCOLORS 64

typedef struct {
 GC gc; /* graphics context for animation */
 int npixels; /* number of valid entries in pixels */
 u_long pixels[NUMCOLORS]; /* pixel values in the colormap */
} perscreen;

extern perscreen Scr[MAXSCREENS];
extern Display *dsp;
extern int screen;

extern char *ProgramName;
extern char *display;
extern char *mode;
extern char *fontname;
extern char *background;
extern char *foreground;
extern char *text_name;
extern char *text_pass;
extern char *text_info;
extern char *text_valid;
extern char *text_invalid;
extern float saturation;
extern int nicelevel;
extern int delay;
extern int batchcount;
extern int reinittime;
extern int timeout;
extern Bool mono;
extern Bool nolock;
extern Bool allowroot;
extern Bool enablesaver;
extern Bool allowaccess;
extern Bool echokeys;
extern Bool verbose;
extern void (*callback) ();
extern void (*init) ();

extern void GetResources();
extern void hsbramp();
extern void error();
extern long seconds();
extern void usage();

/* System V Release 4 redefinitions of BSD functions and structures */

#ifdef SYSV

#include <sys/time.h>
#include <poll.h>
#include <shadow.h>
#define srandom srand
#define random rand
#define USERNAME "LOGNAME"
#define passwd spwd
#define pw_passwd sp_pwdp
#define getpwnam getspnam

#else

#define USERNAME "USER"

#endif