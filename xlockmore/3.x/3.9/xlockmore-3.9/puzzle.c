#ifndef lint
static char sccsid[] = "@(#)puzzle.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * puzzle.c -
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 15-Mar-96: cleaned up, NUMBERED compile-time switch may be broken.
 * Feb-96: combined with rastering.  Jouk Jansen <joukj@alpha.chem.uva.nl>.
 * Feb-95: written.  Heath Rice <hrice@netcom.com>
 */

/*-
 * puzzle - Chops up the screen into squares and randomly slides them around
 * like that game where you try to rearrange the little tiles in their
 * original order.  After it scrambles the tiles for awhile, it reverses
 * itself and puts them back like they started.  This mode looks the coolest
 * if you have a picture on your background.
 */

#include <time.h>
#include <math.h>
#include "xlock.h"
#include "ras.h"
#include "image.xbm"

#ifdef HAS_XPM
#if 1
#include <X11/xpm.h>
#else
#include <xpm.h>
#endif
#include "image.xpm"
#endif

extern int  XbmReadFileToImage(char *filename,
			       int *width, int *height, unsigned char **bits);

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ABS(a) (((a)<0)?(-a):(a))

#define MAX_FIXCOUNT  500000

ModeSpecOpt puzzle_opts =
{0, NULL, NULL, NULL};

extern char *imagefile;

static XImage blogo =
{
	0, 0,			/* width, height */
	0, XYBitmap, 0,		/* xoffset, format, data */
	LSBFirst, 8,		/* byte-order, bitmap-unit */
	LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};
static XImage *logo = NULL;

typedef struct {
	int         excount;
	int        *fixbuff;
	XPoint      count, boxsize, windowsize;
	XPoint      usablewindow, offsetwindow;
	XPoint      randompos, randpos;
	GC          back_gc;
	unsigned long black;
	int         row, col, nextrow, nextcol, nextbox;
	int         incrementOfMove;
	int         lastbox;
	int         forward;
	int         prev;
	int         moves;

	/* Delta move stuff */
	int         movingBox;
	XImage     *box;
	int         cfactor, rfactor;
	int         Lp;
	int         cbs, cbw, rbs, rbw;
	int         lengthOfMove;
#ifdef NUMBERED
	XImage     *image;
	/* Font stuff */
	int         done;
	int         ascent, fontWidth, fontHeight;
#endif
} puzzlestruct;

static puzzlestruct puzzs[MAXSCREENS];

#ifdef NUMBERED
extern XFontStruct *get_font();

#define font_height(f) (f->ascent + f->descent)
static XFontStruct *mode_font;
static int  done = 0;

static int
font_width(XFontStruct * font, char ch)
{
	int         dummy;
	XCharStruct xcs;

	XTextExtents(font, &ch, 1, &dummy, &dummy, &dummy, &xcs);
	return xcs.width;
}

void
NumberScreen(ModeInfo * mi)
{
	Window      window = MI_WINDOW(mi);
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	if (!done) {
		done = 1;
		mode_font = get_font(dsp);
	}
	if (!pp->done) {
		XGCValues   gcv;

		pp->done = 1;
		gcv.font = mode_font->fid;
		XSetFont(dsp, Scr[screen].gc, mode_font->fid);
		gcv.graphics_exposures = False;
		gcv.foreground = WhitePixel(dsp, screen);
		gcv.background = BlackPixel(dsp, screen);
		pp->gc = XCreateGC(dsp, window,
				   GCForeground | GCBackground | GCGraphicsExposures | GCFont, &gcv);
		pp->ascent = mode_font->ascent;
		pp->fontHeight = font_height(mode_font);
		pp->fontWidth = font_width(mode_font, '5');
	}
	XSetForeground(dsp, pp->gc, WhitePixel(dsp, screen));

	{
		XPoint      pos, letter;
		int         count = 1, digitOffset = 1, temp, letterOffset;
		int         i, j, mult = pp->count.x * pp->count.y;
		char        buf[16];

		letter.x = pp->boxsize.x / 2 - 3;
		letter.y = pp->boxsize.y / 2 + pp->ascent / 2 - 1;
		letterOffset = pp->fontWidth / 2;
		pos.y = 0;
		for (j = 0; j < pp->count.y; j++) {
			pos.x = 0;
			for (i = 0; i < pp->count.x; i++) {
				if (count < mult) {
					if (pp->boxsize.x > 2 * pp->fontWidth &&
					    pp->boxsize.y > pp->fontHeight) {
						(void) sprintf(buf, "%d", count);
						XDrawString(dsp, window, pp->gc,
							    pos.x + letter.x - letterOffset * digitOffset +
							    pp->randompos.x,
							    pos.y + letter.y + pp->randompos.y, buf, digitOffset);
					}
					XDrawRectangle(dsp, window, pp->gc,
						       pos.x + 1 + pp->randompos.x, pos.y + 1 + pp->randompos.y,
					pp->boxsize.x - 3, pp->boxsize.y - 3);
					count++;
					digitOffset = 0;
					temp = count;
					while (temp >= 1) {
						temp /= 10;
						digitOffset++;
					}
				}
				pos.x += pp->boxsize.x;
			}
			pos.y += pp->boxsize.y;
		}
	}
}
#endif

static int
setupmove(ModeInfo * mi)
{
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	if ((pp->prev == pp->excount) && (pp->excount > 0) && (pp->forward == 1)) {
		pp->lastbox = -1;
		pp->forward = 0;
		pp->prev--;
	} else if ((pp->prev == -1) && (pp->excount > 0) && (pp->forward == 0)) {
		pp->lastbox = -1;
		pp->forward = 1;
		pp->prev++;
	}
	if (pp->forward)
		pp->nextbox = (LRAND() % 5);
	else
		pp->nextbox = pp->fixbuff[pp->prev];

	switch (pp->nextbox) {
		case 0:
			if ((pp->row == 0) || (pp->lastbox == 2))
				pp->nextbox = 99;
			else {
				pp->nextrow = pp->row - 1;
				pp->nextcol = pp->col;
			}
			break;
		case 1:
			if ((pp->col == pp->count.x - 1) || (pp->lastbox == 3))
				pp->nextbox = 99;
			else {
				pp->nextrow = pp->row;
				pp->nextcol = pp->col + 1;
			}
			break;
		case 2:
			if ((pp->row == pp->count.y - 1) || (pp->lastbox == 0))
				pp->nextbox = 99;
			else {
				pp->nextrow = pp->row + 1;
				pp->nextcol = pp->col;
			}
			break;
		case 3:
			if ((pp->col == 0) || (pp->lastbox == 1))
				pp->nextbox = 99;
			else {
				pp->nextrow = pp->row;
				pp->nextcol = pp->col - 1;
			}
			break;
		default:
			pp->nextbox = 99;
			break;
	}

	if (pp->nextbox != 99) {
		pp->lastbox = pp->nextbox;
		return True;
	} else
		return False;
}

static void
setupmovedelta(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	pp->box = XGetImage(display, window,
			    pp->nextcol * pp->boxsize.x + pp->randompos.x,
			    pp->nextrow * pp->boxsize.y + pp->randompos.y,
			    pp->boxsize.x, pp->boxsize.y,
			    AllPlanes, ZPixmap);

	if (pp->nextcol > pp->col) {
		pp->cfactor = -1;
		pp->cbs = pp->boxsize.x;
		pp->cbw = pp->incrementOfMove;
	} else if (pp->col > pp->nextcol) {
		pp->cfactor = 1;
		pp->cbs = -pp->incrementOfMove;
		pp->cbw = pp->incrementOfMove;
	} else {
		pp->cfactor = 0;
		pp->cbs = 0;
		pp->cbw = pp->boxsize.x;
	}
	if (pp->nextrow > pp->row) {
		pp->rfactor = -1;
		pp->rbs = pp->boxsize.y;
		pp->rbw = pp->incrementOfMove;
	} else if (pp->row > pp->nextrow) {
		pp->rfactor = 1;
		pp->rbs = -pp->incrementOfMove;
		pp->rbw = pp->incrementOfMove;
	} else {
		pp->rfactor = 0;
		pp->rbs = 0;
		pp->rbw = pp->boxsize.y;
	}

	if (pp->cfactor == 0)
		pp->lengthOfMove = pp->boxsize.y;
	else if (pp->rfactor == 0)
		pp->lengthOfMove = pp->boxsize.x;
	else
		pp->lengthOfMove = min(pp->boxsize.x, pp->boxsize.y);
	pp->Lp = pp->incrementOfMove;
}

static void
wrapupmove(ModeInfo * mi)
{
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	if (pp->excount) {
		if (pp->forward) {
			pp->fixbuff[pp->prev] = (pp->nextbox + 2) % 4;
			pp->prev++;
		} else
			pp->prev--;
	}
}

static void
wrapupmovedelta(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	XPutImage(display, window, pp->back_gc, pp->box, 0, 0,
		  pp->col * pp->boxsize.x + pp->randompos.x,
		  pp->row * pp->boxsize.y + pp->randompos.y,
		  pp->boxsize.x, pp->boxsize.y);
	XFlush(dsp);

	pp->row = pp->nextrow;
	pp->col = pp->nextcol;

	XDestroyImage(pp->box);
	pp->box = NULL;
}

static int
moveboxdelta(ModeInfo * mi)
{
	Window      window = MI_WINDOW(mi);
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];
	int         cf = pp->nextcol * pp->boxsize.x +
	pp->Lp * pp->cfactor + pp->randompos.x;
	int         rf = pp->nextrow * pp->boxsize.y +
	pp->Lp * pp->rfactor + pp->randompos.y;

	if (pp->Lp <= pp->lengthOfMove) {
		XPutImage(dsp, window, pp->back_gc, pp->box,
			  0, 0, cf, rf, pp->boxsize.x, pp->boxsize.y);
		XFillRectangle(dsp, window, pp->back_gc,
			       cf + pp->cbs, rf + pp->rbs, pp->cbw, pp->rbw);

		if ((pp->Lp + pp->incrementOfMove > pp->lengthOfMove) &&
		    (pp->Lp != pp->lengthOfMove))
			pp->Lp = pp->lengthOfMove - pp->incrementOfMove;
		pp->Lp += pp->incrementOfMove;
		return False;
	} else
		return True;
}

void
init_puzzle(ModeInfo * mi)
{
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];
	int         x, y, ret = -6;
	XPoint      size;

	pp->excount = MI_BATCHCOUNT(mi);
	pp->lastbox = -1;
	pp->moves = 0;
	pp->movingBox = False;

	pp->windowsize.x = MI_WIN_WIDTH(mi);
	pp->windowsize.y = MI_WIN_HEIGHT(mi);

	pp->forward = 1;
	pp->prev = 0;

	if (pp->box)
		XDestroyImage(pp->box);
	pp->box = NULL;
	if (!logo) {
		pp->black = MI_WIN_BLACK_PIXEL(mi);
		pp->back_gc = MI_GC(mi);

		randomImage(imagefile);

		if (strlen(imagefile))
			if (readable(imagefile)) {
				if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
					if (RasterSuccess == RasterFileToImage(dsp, imagefile, &logo
					/* , Scr[screen].fgcol, Scr[screen].bgcol */ ))
						ret = 2;
					if (ret == 2 && MI_WIN_IS_INSTALL(mi) && !MI_WIN_IS_INROOT(mi)) {
						Colormap    cmap;	/* Probably should be in record for deallocation */
						XGCValues   xgcv;

						cmap = XCreateColormap(display, window, MI_VISUAL(mi), AllocNone);
						SetImageColors(display, cmap);
						set_colormap(window, cmap, MI_WIN_IS_INWINDOW(mi));
						xgcv.background = pp->black = GetBlack();
						pp->back_gc = XCreateGC(display, window, GCBackground,
								      &xgcv);
					}
#ifdef HAS_XPM
					if (ret < 0)
						if (XpmSuccess == XpmReadFileToImage(dsp, imagefile, &logo,
										     (XImage **) NULL, (XpmAttributes *) NULL))
							ret = 1;
#endif
				}
				if (ret < 0)
					if (BitmapSuccess == XbmReadFileToImage(imagefile,
							&blogo.width, &blogo.height, (unsigned char **) &blogo.data)) {
						blogo.bytes_per_line = (blogo.width + 7) / 8;
						logo = &blogo;
						ret = 1;
					}
				if (ret < 0)
					(void) fprintf(stderr,
						       "\"%s\" is in an unrecognized format\n", imagefile);
			} else
				(void) fprintf(stderr,
				  "could not read file \"%s\"\n", imagefile);
#ifdef HAS_XPM
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2 && ret < 0)
			if (XpmSuccess == XpmCreateImageFromData(display, image_name,
			    &logo, (XImage **) NULL, (XpmAttributes *) NULL))
				ret = 1;
#endif
		if (ret < 0) {
			blogo.data = (char *) image_bits;
			blogo.width = image_width;
			blogo.height = image_height;
			blogo.bytes_per_line = (image_width + 7) / 8;
			logo = &blogo;
			ret = 0;
		}
	}
	XSetForeground(display, pp->back_gc, pp->black);
	XFillRectangle(display, window, pp->back_gc, 0, 0,
		       pp->windowsize.x, pp->windowsize.y);
	if (logo) {
		size.x = (logo->width < pp->windowsize.x) ?
			logo->width : pp->windowsize.x;
		size.y = (logo->height < pp->windowsize.y) ?
			logo->height : pp->windowsize.y;
	} else {
		size.x = pp->windowsize.x;
		size.y = pp->windowsize.y;
	}
	pp->boxsize.y = LRAND() % (1 + size.y / 4) + 6;
	pp->boxsize.x = LRAND() % (1 + size.x / 4) + 6;
	if ((pp->boxsize.x > 4 * pp->boxsize.y) ||
	    pp->boxsize.y > 4 * pp->boxsize.x)
		pp->boxsize.x = pp->boxsize.y = 2 * MIN(pp->boxsize.x, pp->boxsize.y);
	pp->count.x = size.x / pp->boxsize.x;
	pp->count.y = size.y / pp->boxsize.y;

	pp->usablewindow.x = pp->count.x * pp->boxsize.x;
	pp->usablewindow.y = pp->count.y * pp->boxsize.y;
	pp->offsetwindow.x = (pp->windowsize.x - pp->usablewindow.x) / 2;
	pp->offsetwindow.y = (pp->windowsize.y - pp->usablewindow.y) / 2;

	pp->incrementOfMove = MIN(pp->usablewindow.x, pp->usablewindow.y) / 20;
	pp->incrementOfMove = MAX(pp->incrementOfMove, 1);

	if (logo) {
		pp->randompos.x = LRAND() % MAX((pp->windowsize.x - logo->width),
						2 * pp->offsetwindow.x + 1);
		pp->randompos.y = LRAND() % MAX((pp->windowsize.y - logo->height),
						2 * pp->offsetwindow.y + 1);
		if (MI_WIN_IS_MONO(mi) || MI_NPIXELS(mi) <= 2)
			XSetForeground(display, pp->back_gc, MI_WIN_WHITE_PIXEL(mi));
		else
			XSetForeground(display, pp->back_gc, MI_PIXEL(mi, NRAND(MI_NPIXELS(mi))));
		XPutImage(display, window, pp->back_gc, logo,
		(int) (LRAND() % MAX(1, (logo->width - pp->usablewindow.x))),
		(int) (LRAND() % MAX(1, (logo->height - pp->usablewindow.y))),
			  pp->randompos.x, pp->randompos.y,
			  pp->usablewindow.x, pp->usablewindow.y);
		XSetForeground(display, pp->back_gc, pp->black);
		for (x = 0; x <= pp->count.x; x++) {
			int         tempx = x * pp->boxsize.x;

			XDrawLine(display, window, pp->back_gc,
				  tempx + pp->randompos.x, pp->randompos.y,
				  tempx + pp->randompos.x, pp->usablewindow.y + pp->randompos.y);
			XDrawLine(display, window, pp->back_gc,
				tempx + pp->randompos.x - 1, pp->randompos.y,
				  tempx + pp->randompos.x - 1, pp->usablewindow.y + pp->randompos.y);
		}
		for (y = 0; y <= pp->count.y; y++) {
			int         tempy = y * pp->boxsize.y;

			XDrawLine(display, window, pp->back_gc,
				  pp->randompos.x, tempy + pp->randompos.y,
				  pp->usablewindow.x + pp->randompos.x, tempy + pp->randompos.y);
			XDrawLine(display, window, pp->back_gc,
				pp->randompos.x, tempy + pp->randompos.y - 1,
				  pp->usablewindow.x + pp->randompos.x, tempy + pp->randompos.y - 1);
		}
	}
#ifdef NUMBERED
	else {
		if (pp->image)
			XDestroyImage(pp->image);
		pp->randompos.x = pp->offsetwindow.x;
		pp->randompos.y = pp->offsetwindow.y;
		NumberScreen(mi);
		pp->image = XGetImage(display, window,
				      pp->offsetwindow.x, pp->offsetwindow.y,
				      pp->usablewindow.x, pp->usablewindow.y,
				      AllPlanes,
				      (MI_WIN_IS_MONO(mi) || MI_NPIXELS(mi) <= 2) ? XYPixmap : ZPixmap);
	}

	pp->row = pp->count.y - 1;
	pp->col = pp->count.x - 1;
#else
	pp->row = NRAND(pp->count.y);
	pp->col = NRAND(pp->count.x);
#endif

	if ((pp->excount) && (pp->fixbuff == NULL))
		if ((pp->fixbuff = (int *) calloc(pp->excount, sizeof (int))) == NULL)
			            error("%s: Couldn't allocate memory for puzzle buffer\n");
}

void
draw_puzzle(ModeInfo * mi)
{
	puzzlestruct *pp = &puzzs[MI_SCREEN(mi)];

	if (pp->movingBox) {
		if (moveboxdelta(mi)) {
			wrapupmovedelta(mi);
			wrapupmove(mi);
			pp->movingBox = False;
			if (pp->moves++ > 2 * MI_BATCHCOUNT(mi))
				init_puzzle(mi);
		}
	} else {
		if (setupmove(mi)) {
			setupmovedelta(mi);
			pp->movingBox = True;
		}
	}
}
