#ifndef lint
static char sccsid[] = "@(#)marquee.c	3.10 96/07/20 xlockmore";

#endif

/*-
 * marquee.c - types a text-file to the screen
 *
 * Copyright (c) 1995 by Tobias Gloth and David Bagley
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 03-Nov-95 Many changes (hopefully good ones) by David Bagley
 * 01-Oct-95 Written by Tobias Gloth
 */

#include "xlock.h"

#define font_height(f) (f->ascent + f->descent)

ModeSpecOpt marquee_opts =
{0, NULL, NULL, NULL};

extern char *program;
extern char *messagesfile;
extern char *messagefile;
extern char *message;

extern XFontStruct *getFont(Display * display);
extern char *getWords(void);
extern int  isRibbon(void);

typedef struct {
	int         ascent;
	int         height;
	int         win_height;
	int         win_width;
	int         x;
	int         y;
	int         t;
	int         startx;
	int         nonblanks;
	int         done;
	int         color;
	int         time;
	GC          gc;
	char       *words;
	char       *wordbuf;
	char        modwords[256];
} marqueestruct;

static marqueestruct *marquees = NULL;

static XFontStruct *mode_font = None;
static int  char_width[256];

static int
font_width(XFontStruct * font, char ch)
{
	int         dummy;
	XCharStruct xcs;

	XTextExtents(font, &ch, 1, &dummy, &dummy, &dummy, &xcs);
	return xcs.width;
}

static int
text_font_width(char *string)
{
	int         n = 0, x = 0, t = 0;

	/* The following does not handle a tab or other weird junk */
	while (*string != '\0') {
		if (x > n)
			n = x;
		switch (*string) {
			case '\v':
			case '\f':
			case '\n':
				x = 0;
				t = 0;
				break;
			case '\b':
				if (t) {
					t--;
					x -= char_width[' '];
				}
				break;
			case '\t':
				x += char_width[' '] * (8 - (t % 8));
				t = ((t + 8) / 8) * 8;
				break;
			case '\r':
				break;
			default:
				t++;
				x += char_width[(int) *string];
		}
		string++;
	}
	return n;
}

static int
text_height(char *string)
{
	int         n = 0;

	while (*string != '\0') {
		if ((*string == '\n') || (*string == '\f') || (*string == '\v'))
			n++;
		string++;
	}
	return n;
}

static int
add_blanks(marqueestruct * mp)
{
	if (mp->t < 251) {
		mp->modwords[mp->t] = ' ';
		mp->t++;
		mp->modwords[mp->t] = ' ';
		mp->t++;
		mp->modwords[mp->t] = '\0';
		(void) strcat(mp->modwords, "  ");
	}
	mp->x -= 2 * char_width[' '];
	if (mp->x <= -char_width[(int) mp->modwords[0]]) {
		mp->x += char_width[(int) mp->modwords[0]];
		(void) memcpy(mp->modwords, &(mp->modwords[1]), mp->nonblanks);
		mp->nonblanks--;
	}
	return (mp->nonblanks < 0);
}

static void
add_letter(marqueestruct * mp, char letter)
{
	if (mp->t < 252) {
		mp->modwords[mp->t] = letter;
		mp->t++;
		mp->modwords[mp->t] = '\0';
		(void) strcat(mp->modwords, "  ");
	}
	mp->x -= char_width[(int) letter];
	if (mp->x <= -char_width[(int) mp->modwords[0]]) {
		mp->x += char_width[(int) mp->modwords[0]];
		(void) memcpy(mp->modwords, &(mp->modwords[1]), mp->t);
		mp->modwords[mp->t] = ' ';
		mp->t--;
	} else
		mp->nonblanks = mp->t;
}

void
init_marquee(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	marqueestruct *mp;
	XGCValues   gcv;
	int         i;

	if (marquees == NULL) {
		if ((marquees = (marqueestruct *) calloc(MI_NUM_SCREENS(mi),
					    sizeof (marqueestruct))) == NULL)
			return;
	}
	mp = &marquees[MI_SCREEN(mi)];

	mp->win_width = MI_WIN_WIDTH(mi);
	mp->win_height = MI_WIN_HEIGHT(mi);
	mp->color = 0;
	mp->time = 0;
	mp->t = 0;
	mp->nonblanks = 0;
	mp->x = 0;
	XClearWindow(display, MI_WINDOW(mi));
	if (mode_font == None)
		mode_font = getFont(display);
	if (!mp->done) {
		mp->done = 1;
		gcv.font = mode_font->fid;
		XSetFont(display, MI_GC(mi), mode_font->fid);
		gcv.graphics_exposures = False;
		gcv.foreground = MI_WIN_WHITE_PIXEL(mi);
		gcv.background = MI_WIN_BLACK_PIXEL(mi);
		mp->gc = XCreateGC(display, MI_WINDOW(mi),
				   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
		mp->ascent = mode_font->ascent;
		mp->height = font_height(mode_font);
		for (i = 0; i < 256; i++)
			if ((i >= mode_font->min_char_or_byte2) &&
			    (i <= mode_font->max_char_or_byte2))
				char_width[i] = font_width(mode_font, (char) i);
			else
				char_width[i] = font_width(mode_font, (char) mode_font->default_char);
	}
	if (mp->wordbuf)
		(void) free((void *) mp->wordbuf);
	mp->wordbuf = strdup(getWords());
	if (!mp->wordbuf)
		error("%s: out of memory.\n");
	mp->words = mp->wordbuf;
	mp->y = 0;

	if (isRibbon()) {
		mp->x = mp->win_width;
		if (mp->win_height > font_height(mode_font))
			mp->y += NRAND(mp->win_height - font_height(mode_font));
		else if (mp->win_height < font_height(mode_font))
			mp->y -= NRAND(font_height(mode_font) - mp->win_height);
	} else {
		int         text_ht = text_height(mp->words);
		int         text_font_wid = text_font_width(mp->words);

		if (mp->win_height > text_ht * font_height(mode_font))
			mp->y = NRAND(mp->win_height - text_ht * font_height(mode_font));
		if (mp->y < 0)
			mp->y = 0;
		mp->x = 0;
		if (mp->win_width > text_font_wid)
			mp->x += NRAND(mp->win_width - text_font_wid);
		/* else if (mp->win_width < text_font_wid)
		   mp->x -= NRAND(text_font_wid - mp->win_width); */
		mp->startx = mp->x;
	}
}

void
draw_marquee(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	marqueestruct *mp = &marquees[MI_SCREEN(mi)];
	char       *space = "        ";
	char       *ch;

	ch = mp->words;
	if (isRibbon()) {
		ch = mp->words;
		switch (*ch) {
			case '\0':
				if (add_blanks(mp)) {
					init_marquee(mi);
					return;
				}
				break;
			case '\b':
			case '\r':
			case '\n':
			case '\t':
			case '\v':
			case '\f':
				add_letter(mp, ' ');
				mp->words++;
				break;
			default:
				add_letter(mp, *ch);
				mp->words++;
		}
		if (MI_NPIXELS(mi) > 2) {
			XSetForeground(display, mp->gc, MI_PIXEL(mi, mp->color));
			if (++mp->color == MI_NPIXELS(mi))
				mp->color = 0;
		} else
			XSetForeground(display, mp->gc, MI_WIN_WHITE_PIXEL(mi));
		XDrawImageString(display, MI_WINDOW(mi), mp->gc,
			 mp->x, mp->y + mp->ascent, mp->modwords, mp->t + 2);
	} else {
		switch (*ch) {
			case '\0':
				if (++mp->time > 16)
					init_marquee(mi);
				return;
			case '\b':
				if (mp->t) {
					mp->t--;
					mp->x -= char_width[' '];
				}
				break;
			case '\v':
			case '\f':
			case '\n':
				mp->x = mp->startx;
				mp->t = 0;
				mp->y += mp->height;
				if (mp->y + mp->height > mp->win_height) {
					XCopyArea(display, window, window, mp->gc,
						  0, mp->height, mp->win_width, mp->y - mp->height, 0, 0);
					XSetForeground(display, mp->gc, MI_WIN_BLACK_PIXEL(mi));
					mp->y -= mp->height;
					XFillRectangle(display, window, mp->gc,
					0, mp->y, mp->win_width, mp->height);
				}
				break;
			case '\t':
				XDrawString(display, window, mp->gc, mp->x, mp->y + mp->ascent,
					    space, 8 - (mp->t % 8));
				mp->x += char_width[' '] * (8 - (mp->t % 8));
				mp->t = ((mp->t + 8) / 8) * 8;
				break;
			case '\r':
				break;
			default:
				if (MI_NPIXELS(mi) > 2) {
					XSetForeground(display, mp->gc, MI_PIXEL(mi, mp->color));
					if (++mp->color == MI_NPIXELS(mi))
						mp->color = 0;
				} else
					XSetForeground(display, mp->gc, MI_WIN_WHITE_PIXEL(mi));
				XDrawString(display, window, mp->gc, mp->x, mp->y + mp->ascent, ch, 1);
				mp->t++;
				mp->x += char_width[(int) *ch];
		}
		mp->words++;
	}
}

void
release_marquee(ModeInfo * mi)
{
	if (marquees != NULL) {
		int         screen;

		for (screen = 0; screen < MI_NUM_SCREENS(mi); screen++) {
			marqueestruct *mp = &marquees[screen];

			if (!mp->gc)
				XFreeGC(MI_DISPLAY(mi), mp->gc);

		}
		(void) free((void *) marquees);
		marquees = NULL;
	}
	if (mode_font != None) {
		XFreeFont(MI_DISPLAY(mi), mode_font);
		mode_font = None;
	}
}
