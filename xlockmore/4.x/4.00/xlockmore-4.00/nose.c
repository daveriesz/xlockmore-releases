
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)nose.c	4.00 97/01/01 xlockmore";

#endif

/*-
 * nose.c - Based on xnlock a little guy with a big nose and a hat wander
 * around the screen, spewing out messages.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 27-Feb-96: Added new ModeInfo arg to init and callback hooks.  Removed
 *		references to onepause, now uses MI_PAUSE(mi) interface.
 *		Ron Hitchens <ron@idiom.com>
 * 10-Oct-95: A better way of handling fortunes from a file, thanks to
 *            Jouk Jansen <joukj@alpha.chem.uva.nl>.
 * 21-Sep-95: font option added, debugged for multiscreens
 * 12-Aug-95: xlock version
 * 1992: xscreensaver version, noseguy (Jamie Zawinski <jwz@netscape.com>)
 * 1990: X11 version, xnlock (Dan Heller <argv@sun.com>)
 */

/* xscreensaver, Copyright (c) 1992 Jamie Zawinski <jwz@mcom.com> * *
   Permission to use, copy, modify, distribute, and sell this software and its
   * documentation for any purpose is hereby granted without fee, provided that
   * the above copyright notice appear in all copies and that both that *
   copyright notice and this permission notice appear in supporting *
   documentation.  No representations are made about the suitability of this *
   software for any purpose.  It is provided "as is" without express or  *
   implied warranty. */

#include "xlock.h"

#include "bitmaps/nose-0l.xbm"
#include "bitmaps/nose-1l.xbm"
#include "bitmaps/nose-0r.xbm"
#include "bitmaps/nose-1r.xbm"
#include "bitmaps/nose-lf.xbm"
#include "bitmaps/nose-rf.xbm"
#include "bitmaps/nose-f.xbm"
#include "bitmaps/nose-d.xbm"

#define font_height(f) (f->ascent + f->descent)

#define L0 0
#define L1 1
#define R0 2
#define R1 3
#define LF 4
#define RF 5
#define F 6
#define D 7
#define BITMAPS 8

#define MOVE 0
#define TALK 1
#define FREEZE 2

ModeSpecOpt nose_opts =
{0, NULL, 0, NULL, NULL};

extern char *program;
extern char *messagesfile;
extern char *messagefile;
extern char *message;

extern XFontStruct *getFont(Display * display);
extern char *getWords(int screen);
extern int  isRibbon(void);

static XImage bimages[] =
{
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
	{0, 0, 0, XYBitmap, 0, LSBFirst, 8, LSBFirst, 8, 1},
};

typedef struct {
	int         xs, ys;
	int         width, height;
	GC          text_fg_gc, text_bg_gc;
	char       *words;
	int         x, y;
	int         tinymode;	/* walking or getting passwd */
	int         length, dir, lastdir;
	int         up;
	int         frame;
	long        nose_pause;
	int         state;
} nosestruct;

static nosestruct *noses = NULL;

static void walk(ModeInfo * mi, register int dir);
static void talk(ModeInfo * mi, int force_erase);
static int  think(ModeInfo * mi);
static unsigned long look(ModeInfo * mi);
static XFontStruct *mode_font = None;
static int  init_images = 0;

static unsigned char *bits[] =
{
	nose_0_left_bits, nose_1_left_bits, nose_0_right_bits,
	nose_1_right_bits, nose_left_front_bits, nose_right_front_bits,
	nose_front_bits, nose_down_bits
};

#define LEFT 001
#define RIGHT 002
#define DOWN 004
#define UP 010
#define FRONT 020
#define X_INCR 3
#define Y_INCR 2

static void
move(ModeInfo * mi)
{
	nosestruct *np = &noses[MI_SCREEN(mi)];

	if (!np->length) {
		register int tries = 0;

		np->dir = 0;
		if ((LRAND() & 1) && think(mi)) {
			talk(mi, 0);	/* sets timeout to itself */
			return;
		}
		if (!NRAND(3) && (np->nose_pause = look(mi))) {
			np->state = MOVE;
			return;
		}
		np->nose_pause = 2000 + 10 * NRAND(100);
		do {
			if (!tries)
				np->length = np->width / 100 + NRAND(90), tries = 8;
			else
				tries--;
			switch (NRAND(8)) {
				case 0:
					if (np->x - X_INCR * np->length >= 5)
						np->dir = LEFT;
					break;
				case 1:
					if (np->x + X_INCR * np->length <= np->width - np->xs - 6)
						np->dir = RIGHT;
					break;
				case 2:
					if (np->y - (Y_INCR * np->length) >= 5)
						np->dir = UP;
					break;
				case 3:
					if (np->y + Y_INCR * np->length <= np->height - np->ys - 6)
						np->dir = DOWN;
					break;
				case 4:
					if (np->x - X_INCR * np->length >= 5 &&
					  np->y - (Y_INCR * np->length) >= 5)
						np->dir = (LEFT | UP);
					break;
				case 5:
					if (np->x + X_INCR * np->length <= np->width - np->xs - 6 &&
					    np->y - Y_INCR * np->length >= 5)
						np->dir = (RIGHT | UP);
					break;
				case 6:
					if (np->x - X_INCR * np->length >= 5 &&
					    np->y + Y_INCR * np->length <= np->height - np->ys - 6)
						np->dir = (LEFT | DOWN);
					break;
				case 7:
					if (np->x + X_INCR * np->length <= np->width - np->xs - 6 &&
					    np->y + Y_INCR * np->length <= np->height - np->ys - 6)
						np->dir = (RIGHT | DOWN);
					break;
				default:
					/* No Defaults */
					break;
			}
		} while (!np->dir);
	}
	walk(mi, np->dir);
	--np->length;
	np->state = MOVE;
}

static void
walk(ModeInfo * mi, register int dir)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	nosestruct *np = &noses[MI_SCREEN(mi)];
	register int incr = 0;

	if (dir & (LEFT | RIGHT)) {	/* left/right movement (mabye up/down too) */
		np->up = -np->up;	/* bouncing effect (even if hit a wall) */
		if (dir & LEFT) {
			incr = X_INCR;
			np->frame = (np->up < 0) ? L0 : L1;
		} else {
			incr = -X_INCR;
			np->frame = (np->up < 0) ? R0 : R1;
		}
		/* note that maybe neither UP nor DOWN is set! */
		if (dir & UP && np->y > Y_INCR)
			np->y -= Y_INCR;
		else if (dir & DOWN && np->y < np->height - np->ys)
			np->y += Y_INCR;
	} else if (dir == UP) {	/* Explicit up/down movement only (no left/right) */
		XPutImage(display, window, gc, &(bimages[F]),
			  0, 0, np->x, np->y -= Y_INCR, np->xs, np->ys);
	} else if (dir == DOWN)
		XPutImage(display, window, gc, &(bimages[D]),
			  0, 0, np->x, np->y += Y_INCR, np->xs, np->ys);
	else if (dir == FRONT && np->frame != F) {
		if (np->up > 0)
			np->up = -np->up;
		if (np->lastdir & LEFT)
			np->frame = LF;
		else if (np->lastdir & RIGHT)
			np->frame = RF;
		else
			np->frame = F;
		XPutImage(display, window, gc, &(bimages[np->frame]),
			  0, 0, np->x, np->y, np->xs, np->ys);
	}
	if (dir & LEFT)
		while (--incr >= 0) {
			XPutImage(display, window, gc, &(bimages[np->frame]),
			      0, 0, --np->x, np->y + np->up, np->xs, np->ys);
			XFlush(display);
	} else if (dir & RIGHT)
		while (++incr <= 0) {
			XPutImage(display, window, gc, &(bimages[np->frame]),
			      0, 0, ++np->x, np->y + np->up, np->xs, np->ys);
			XFlush(display);
		}
	np->lastdir = dir;
}

static int
think(ModeInfo * mi)
{
	nosestruct *np = &noses[MI_SCREEN(mi)];

	if (LRAND() & 1)
		walk(mi, FRONT);
	if (LRAND() & 1) {
		np->words = getWords(MI_SCREEN(mi));
		return 1;
	}
	return 0;
}

#define MAXLINES 40

static void
talk(ModeInfo * mi, int force_erase)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	nosestruct *np = &noses[MI_SCREEN(mi)];
	int         width = 0, height, Z, total = 0;
	static int  X, Y, talking;
	static struct {
		int         x, y, width, height;
	} s_rect;
	register char *p, *p2;
	char        buf[BUFSIZ], args[MAXLINES][256];

	/* clear what we've written */
	if (talking || force_erase) {
		if (!talking)
			return;
		XFillRectangle(display, window, np->text_bg_gc, s_rect.x - 5, s_rect.y - 5,
			       s_rect.width + 10, s_rect.height + 10);
		talking = 0;
		if (!force_erase)
			np->state = MOVE;
		return;
	}
	talking = 1;
	walk(mi, FRONT);
	p = strcpy(buf, np->words);

	if (!(p2 = (char *) strchr(p, '\n')) || !p2[1]) {
		total = strlen(np->words);
		(void) strcpy(args[0], np->words);
		width = XTextWidth(mode_font, np->words, total);
		height = 0;
	} else
		/* p2 now points to the first '\n' */
		for (height = 0; p; height++) {
			int         w;

			*p2 = 0;
			if ((w = XTextWidth(mode_font, p, p2 - p)) > width)
				width = w;
			total += p2 - p;	/* total chars; count to determine reading time */
			(void) strcpy(args[height], p);
			if (height == MAXLINES - 1) {
				(void) puts("Message too long!");
				break;
			}
			p = p2 + 1;
			if (!(p2 = (char *) strchr(p, '\n')))
				break;
		}
	height++;

	/*
	 * Figure out the height and width in imagepixels (height, width) extend the
	 * new box by 15 pixels on the sides (30 total) top and bottom.
	 */
	s_rect.width = width + 30;
	s_rect.height = height * font_height(mode_font) + 30;
	if (np->x - s_rect.width - 10 < 5)
		s_rect.x = 5;
	else if ((s_rect.x = np->x + 32 - (s_rect.width + 15) / 2)
		 + s_rect.width + 15 > np->width - 5)
		s_rect.x = np->width - 15 - s_rect.width;
	if (np->y - s_rect.height - 10 < 5)
		s_rect.y = np->y + np->ys + 5;
	else
		s_rect.y = np->y - 5 - s_rect.height;

	XFillRectangle(display, window, np->text_bg_gc,
		       s_rect.x, s_rect.y, s_rect.width, s_rect.height);

	/* make a box that's 5 pixels thick. Then add a thin box inside it */
	XSetLineAttributes(display, np->text_fg_gc, 5, 0, 0, 0);
	XDrawRectangle(display, window, np->text_fg_gc,
		    s_rect.x, s_rect.y, s_rect.width - 1, s_rect.height - 1);
	XSetLineAttributes(display, np->text_fg_gc, 0, 0, 0, 0);
	XDrawRectangle(display, window, np->text_fg_gc,
	  s_rect.x + 7, s_rect.y + 7, s_rect.width - 15, s_rect.height - 15);

	X = 15;
	Y = 15 + font_height(mode_font);

	/* now print each string in reverse order (start at bottom of box) */
	for (Z = 0; Z < height; Z++) {
		XDrawString(display, window, np->text_fg_gc, s_rect.x + X, s_rect.y + Y,
			    args[Z], strlen(args[Z]));
		Y += font_height(mode_font);
	}
	np->nose_pause = (total / 15) * 1000000;
	if (np->nose_pause < 3000000)
		np->nose_pause = 3000000;
	np->state = TALK;
}

static unsigned long
look(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	nosestruct *np = &noses[MI_SCREEN(mi)];

	if (NRAND(3)) {
		XPutImage(display, window, gc,
			  &(bimages[(LRAND() & 1) ? D : F]),
			  0, 0, np->x, np->y, np->xs, np->ys);
		return 100000L;
	}
	if (!NRAND(5))
		return 0;
	if (NRAND(3)) {
		XPutImage(display, window, gc,
			  &(bimages[(LRAND() & 1) ? LF : RF]),
			  0, 0, np->x, np->y, np->xs, np->ys);
		return 100000L;
	}
	if (!NRAND(5))
		return 0;
	XPutImage(display, window, gc,
		  &(bimages[(LRAND() & 1) ? L0 : R0]),
		  0, 0, np->x, np->y, np->xs, np->ys);
	return 100000L;
}

void
init_nose(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	GC          gc = MI_GC(mi);
	nosestruct *np;
	XGCValues   gcv;

	if (noses == NULL) {
		if ((noses = (nosestruct *) calloc(MI_NUM_SCREENS(mi),
					       sizeof (nosestruct))) == NULL)
			return;
	}
	np = &noses[MI_SCREEN(mi)];

	np->width = MI_WIN_WIDTH(mi) + 2;
	np->height = MI_WIN_HEIGHT(mi) + 2;
	np->tinymode =
		(np->width + np->height < 2 * (nose_front_width + nose_front_height));
	np->xs = nose_front_width;
	np->ys = nose_front_height;

	XClearWindow(display, window);
	XSetForeground(display, gc, MI_WIN_WHITE_PIXEL(mi));
	if (mode_font == None)
		mode_font = getFont(display);
	if (!init_images)
		for (init_images = 0; init_images < BITMAPS; init_images++) {
			bimages[init_images].data = (char *) bits[init_images];
			bimages[init_images].width = nose_front_width;
			bimages[init_images].height = nose_front_height;
			bimages[init_images].bytes_per_line = (nose_front_width + 7) / 8;
		}
	np->words = getWords(MI_SCREEN(mi));
	if (np->text_fg_gc == NULL) {
		gcv.font = mode_font->fid;
		XSetFont(display, gc, mode_font->fid);
		gcv.graphics_exposures = False;
		gcv.foreground = MI_WIN_WHITE_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);
		np->text_fg_gc = XCreateGC(display, window,
					   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
		gcv.foreground = MI_WIN_BLACK_PIXEL(mi);
		gcv.background = MI_WIN_WHITE_PIXEL(mi);
		np->text_bg_gc = XCreateGC(display, window,
					   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
	}
	np->up = 1;
	if (np->tinymode) {
		np->x = 0;
		np->y = 0;
		XPutImage(display, window, gc, &(bimages[R0]),
			  0, 0,
			  (np->width - nose_front_width) / 2,
			  (np->height - nose_front_height) / 2,
			  np->xs, np->ys);
		np->state = FREEZE;
	} else {
		np->x = np->width / 2;
		np->y = np->height / 2;
		np->state = MOVE;
	}
}

void
draw_nose(ModeInfo * mi)
{
	nosestruct *np = &noses[MI_SCREEN(mi)];

	np->nose_pause = 0;	/* use default if not changed here */
	switch (np->state) {
		case MOVE:
			move(mi);
			break;
		case TALK:
			talk(mi, 0);
			break;
	}
	/* TODO: This is wrong for multi-screens *** */
	MI_PAUSE(mi) = np->nose_pause;	/* pass back desired pause time */
}

void
release_nose(ModeInfo * mi)
{
	if (noses != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			nosestruct *np = &noses[screen];

			if (np->text_fg_gc != NULL)
				XFreeGC(MI_DISPLAY(mi), np->text_fg_gc);
			if (np->text_bg_gc != NULL)
				XFreeGC(MI_DISPLAY(mi), np->text_bg_gc);

		}
		(void) free((void *) noses);
		noses = NULL;
	}
	if (mode_font != None) {
		XFreeFont(MI_DISPLAY(mi), mode_font);
		mode_font = None;
	}
}

void
refresh_nose(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself */
}
