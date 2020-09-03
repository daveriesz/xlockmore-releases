#ifndef lint
static char sccsid[] = "@(#)nose.c	3.9 96/05/25 xlockmore";

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
 *		Ron Hitchens <ron@utw.com>
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

ModeSpecOpt nose_opts =
{0, NULL, NULL, NULL};

extern char *program;
extern char *messagesfile;
extern char *messagefile;
extern char *message;

extern XFontStruct *get_font(Display * display);
extern char *get_words(void);
extern int  is_ribbon(void);

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
	int         done;
	int         xs, ys;
	int         width, height;
	GC          text_fg_gc, text_bg_gc;
	char       *words;
	int         x, y;
	int         mode;	/* walking or getting passwd */
	int         length, dir, lastdir;
	int         up;
	int         frame;
	void        (*next_fn) ();
} nosestruct;

static nosestruct noses[MAXSCREENS];

static void init_images(void), walk(Window win, register int dir), talk(Window win, int force_erase),
            freeze(void);
static int  think(Window win);
static unsigned long look(Window win);
static int  done = 0;
static XFontStruct *mode_font;
static long nose_pause = 0;	/* make a private variable */

static unsigned char *bits[] =
{
	nose_0_left_bits, nose_1_left_bits, nose_0_right_bits,
	nose_1_right_bits, nose_left_front_bits, nose_right_front_bits,
	nose_front_bits, nose_down_bits
};

static void
init_images(void)
{
	int         i;

	for (i = 0; i < BITMAPS; i++) {
		bimages[i].data = (char *) bits[i];
		bimages[i].width = nose_front_width;
		bimages[i].height = nose_front_height;
		bimages[i].bytes_per_line = (nose_front_width + 7) / 8;
	}
}

#define LEFT 001
#define RIGHT 002
#define DOWN 004
#define UP 010
#define FRONT 020
#define X_INCR 3
#define Y_INCR 2

static void
move(Window win)
{
	nosestruct *np = &noses[screen];

	if (!np->length) {
		register int tries = 0;

		np->dir = 0;
		if ((LRAND() & 1) && think(win)) {
			talk(win, 0);	/* sets timeout to itself */
			return;
		}
		if (!(LRAND() % 3) && (nose_pause = look(win))) {
			np->next_fn = move;
			return;
		}
		nose_pause = 2000 + 10 * (LRAND() % 100);
		do {
			if (!tries)
				np->length = np->width / 100 + LRAND() % 90, tries = 8;
			else
				tries--;
			switch (LRAND() % 8) {
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
	walk(win, np->dir);
	--np->length;
	np->next_fn = move;
}

static void
walk(Window win, register int dir)
{
	nosestruct *np = &noses[screen];
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
		XPutImage(dsp, win, Scr[screen].gc, &(bimages[F]),
			  0, 0, np->x, np->y -= Y_INCR, np->xs, np->ys);
	} else if (dir == DOWN)
		XPutImage(dsp, win, Scr[screen].gc, &(bimages[D]),
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
		XPutImage(dsp, win, Scr[screen].gc, &(bimages[np->frame]),
			  0, 0, np->x, np->y, np->xs, np->ys);
	}
	if (dir & LEFT)
		while (--incr >= 0) {
			XPutImage(dsp, win, Scr[screen].gc, &(bimages[np->frame]),
			      0, 0, --np->x, np->y + np->up, np->xs, np->ys);
			XFlush(dsp);
	} else if (dir & RIGHT)
		while (++incr <= 0) {
			XPutImage(dsp, win, Scr[screen].gc, &(bimages[np->frame]),
			      0, 0, ++np->x, np->y + np->up, np->xs, np->ys);
			XFlush(dsp);
		}
	np->lastdir = dir;
}

static int
think(Window win)
{
	nosestruct *np = &noses[screen];

	if (LRAND() & 1)
		walk(win, FRONT);
	if (LRAND() & 1) {
		np->words = get_words();
		return 1;
	}
	return 0;
}

#define MAXLINES 40

static void
talk(Window win, int force_erase)
{
	nosestruct *np = &noses[screen];
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
		XFillRectangle(dsp, win, np->text_bg_gc, s_rect.x - 5, s_rect.y - 5,
			       s_rect.width + 10, s_rect.height + 10);
		talking = 0;
		if (!force_erase)
			np->next_fn = move;
		{
			/* might as well check the window for size changes now... */
			XWindowAttributes xgwa;

			(void) XGetWindowAttributes(dsp, win, &xgwa);
			np->width = xgwa.width + 2;
			np->height = xgwa.height + 2;
		}
		return;
	}
	talking = 1;
	walk(win, FRONT);
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
	 * Figure out the height and width in pixels (height, width) extend the
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

	XFillRectangle(dsp, win, np->text_bg_gc,
		       s_rect.x, s_rect.y, s_rect.width, s_rect.height);

	/* make a box that's 5 pixels thick. Then add a thin box inside it */
	XSetLineAttributes(dsp, np->text_fg_gc, 5, 0, 0, 0);
	XDrawRectangle(dsp, win, np->text_fg_gc,
		    s_rect.x, s_rect.y, s_rect.width - 1, s_rect.height - 1);
	XSetLineAttributes(dsp, np->text_fg_gc, 0, 0, 0, 0);
	XDrawRectangle(dsp, win, np->text_fg_gc,
	  s_rect.x + 7, s_rect.y + 7, s_rect.width - 15, s_rect.height - 15);

	X = 15;
	Y = 15 + font_height(mode_font);

	/* now print each string in reverse order (start at bottom of box) */
	for (Z = 0; Z < height; Z++) {
		XDrawString(dsp, win, np->text_fg_gc, s_rect.x + X, s_rect.y + Y,
			    args[Z], strlen(args[Z]));
		Y += font_height(mode_font);
	}
	nose_pause = (total / 15) * 1000000;
	if (nose_pause < 3000000)
		nose_pause = 3000000;
	np->next_fn = talk;
}

static void
freeze(void)
{
}

static unsigned long
look(Window win)
{
	nosestruct *np = &noses[screen];

	if (LRAND() % 3) {
		XPutImage(dsp, win, Scr[screen].gc,
			  &(bimages[(LRAND() & 1) ? D : F]),
			  0, 0, np->x, np->y, np->xs, np->ys);
		return 100000L;
	}
	if (!(LRAND() % 5))
		return 0;
	if (LRAND() % 3) {
		XPutImage(dsp, win, Scr[screen].gc,
			  &(bimages[(LRAND() & 1) ? LF : RF]),
			  0, 0, np->x, np->y, np->xs, np->ys);
		return 100000L;
	}
	if (!(LRAND() % 5))
		return 0;
	XPutImage(dsp, win, Scr[screen].gc,
		  &(bimages[(LRAND() & 1) ? L0 : R0]),
		  0, 0, np->x, np->y, np->xs, np->ys);
	return 100000L;
}

void
init_nose(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	nosestruct *np = &noses[screen];
	XGCValues   gcv;

	np->width = MI_WIN_WIDTH(mi) + 2;
	np->height = MI_WIN_HEIGHT(mi) + 2;
	np->mode =
		(np->width + np->height < 2 * (nose_front_width + nose_front_height));
	np->xs = nose_front_width;
	np->ys = nose_front_height;

	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, np->width, np->height);
	XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
	if (!done) {
		done = 1;
		mode_font = get_font(dsp);
		init_images();
	}
	np->words = get_words();
	if (!np->done) {
		np->done = 1;
		gcv.font = mode_font->fid;
		XSetFont(dsp, Scr[screen].gc, mode_font->fid);
		gcv.graphics_exposures = False;
		gcv.foreground = WhitePixel(dsp, screen);
		gcv.background = BlackPixel(dsp, screen);
		np->text_fg_gc = XCreateGC(dsp, win,
					   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
		gcv.foreground = BlackPixel(dsp, screen);
		gcv.background = WhitePixel(dsp, screen);
		np->text_bg_gc = XCreateGC(dsp, win,
					   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
	}
	np->up = 1;
	if (np->mode) {
		np->x = 0;
		np->y = 0;
		XPutImage(dsp, win, Scr[screen].gc, &(bimages[R0]),
			  0, 0, np->x, np->y, np->xs, np->ys);
		np->next_fn = freeze;
	} else {
		np->x = np->width / 2;
		np->y = np->height / 2;
		np->next_fn = move;
	}
}

void
draw_nose(ModeInfo * mi)
{
	nosestruct *np = &noses[screen];

	nose_pause = 0;		/* use default if not changed here */
	np->next_fn(MI_WINDOW(mi), 0);
	MI_PAUSE(mi) = nose_pause;	/* pass back desired pause time */
}
