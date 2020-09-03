#ifndef lint
static char sccsid[] = "@(#)kaleid.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * kaleid.c
 *
 * Based on a kaleidoscope algorithm from a PC-based program by:
 * Judson D. McClendon (Compuserve: [74415,1003])
 *
 * KALEIDOSCOPE (X11 Version)
 * By Nathan Meyers, nathanm@hp-pcd.hp.com.
 *
 * Modified by Laurent JULLIARD, laurentj@hpgnse2.grenoble.hp.com
 * for LINUX 0.96 and X11 v1.0 shared library
 *
 * Revision History:
 * 23-Oct-94: Ported to xlock by David Bagley <bagleyd@hertz.njit.edu>
 */

#include "xlock.h"

#define INTRAND(min,max) (LRAND()%((max+1)-(min))+(min))

ModeSpecOpt kaleid_opts =
{0, NULL, NULL, NULL};

typedef struct {
	int         x;
	int         y;
} point;

typedef struct {
	int         pix;
	int         cx, cy, m;
	int         ox, oy;
	int         x1, y1, x2, y2, xv1, yv1, xv2, yv2;
	int         xa, ya, xb, yb, xc, yc, xd, yd;
	int         width;
	int         height;
	int         itercount;
} kaleidstruct;

static kaleidstruct kaleids[MAXSCREENS];

void
init_kaleid(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	kaleidstruct *kp = &kaleids[screen];

	kp->width = MI_WIN_WIDTH(mi);
	kp->height = MI_WIN_HEIGHT(mi);

	kp->pix = 0;
	kp->cx = kp->width / 2;
	kp->cy = kp->height / 2;
	kp->m = (kp->cx > kp->cy) ? kp->cx : kp->cy;
	kp->m = kp->m ? kp->m : 1;
	kp->x1 = LRAND() % kp->m + 1;
	kp->x2 = LRAND() % kp->m + 1;
	kp->y1 = LRAND() % kp->x1 + 1;
	kp->y2 = LRAND() % kp->x2 + 1;
	kp->xv1 = LRAND() % 7 - 3;
	kp->xv2 = LRAND() % 7 - 3;
	kp->yv1 = LRAND() % 7 - 3;
	kp->yv2 = LRAND() % 7 - 3;
	kp->itercount = 0;
	XSetForeground(dsp, Scr[screen].gc, BlackPixel(dsp, screen));
	XFillRectangle(dsp, win, Scr[screen].gc, 0, 0, kp->width, kp->height);
}

void
draw_kaleid(ModeInfo * mi)
{
	Window      win = MI_WINDOW(mi);
	XSegment    segs[8];
	kaleidstruct *kp = &kaleids[screen];

	if (!(LRAND() % 50)) {
		kp->x1 = LRAND() % kp->m + 1;
		kp->x2 = LRAND() % kp->m + 1;
		kp->y1 = LRAND() % kp->x1;
		kp->y2 = LRAND() % kp->x2;
	}
	if (!(LRAND() % 10)) {
		kp->xv1 = LRAND() % 7 - 3;
		kp->xv2 = LRAND() % 7 - 3;
		kp->yv1 = LRAND() % 7 - 3;
		kp->yv2 = LRAND() % 7 - 3;
		if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
			XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[kp->pix]);
			if (++kp->pix >= Scr[screen].npixels)
				kp->pix = 0;
		}
	}
	if (kp->cx < kp->cy) {
		kp->xa = kp->x1 * kp->cx / kp->cy;
		kp->ya = kp->y1 * kp->cx / kp->cy;
		kp->xb = kp->x1;
		kp->yb = kp->y1;
		kp->xc = kp->x2 * kp->cx / kp->cy;
		kp->yc = kp->y2 * kp->cx / kp->cy;
		kp->xd = kp->x2;
		kp->yd = kp->y2;
	} else {
		kp->xa = kp->x1;
		kp->ya = kp->y1;
		kp->xb = kp->x1 * kp->cy / kp->cx;
		kp->yb = kp->y1 * kp->cy / kp->cx;
		kp->xc = kp->x2;
		kp->yc = kp->y2;
		kp->xd = kp->x2 * kp->cy / kp->cx;
		kp->yd = kp->y2 * kp->cy / kp->cx;
	}
	segs[0].x1 = kp->cx + kp->xa + kp->ox;
	segs[0].y1 = kp->cy - kp->yb + kp->oy;
	segs[0].x2 = kp->cx + kp->xc + kp->ox;
	segs[0].y2 = kp->cy - kp->yd + kp->oy;
	segs[1].x1 = kp->cx - kp->ya + kp->ox;
	segs[1].y1 = kp->cy + kp->xb + kp->oy;
	segs[1].x2 = kp->cx - kp->yc + kp->ox;
	segs[1].y2 = kp->cy + kp->xd + kp->oy;
	segs[2].x1 = kp->cx - kp->xa + kp->ox;
	segs[2].y1 = kp->cy - kp->yb + kp->oy;
	segs[2].x2 = kp->cx - kp->xc + kp->ox;
	segs[2].y2 = kp->cy - kp->yd + kp->oy;
	segs[3].x1 = kp->cx - kp->ya + kp->ox;
	segs[3].y1 = kp->cy - kp->xb + kp->oy;
	segs[3].x2 = kp->cx - kp->yc + kp->ox;
	segs[3].y2 = kp->cy - kp->xd + kp->oy;
	segs[4].x1 = kp->cx - kp->xa + kp->ox;
	segs[4].y1 = kp->cy + kp->yb + kp->oy;
	segs[4].x2 = kp->cx - kp->xc + kp->ox;
	segs[4].y2 = kp->cy + kp->yd + kp->oy;
	segs[5].x1 = kp->cx + kp->ya + kp->ox;
	segs[5].y1 = kp->cy - kp->xb + kp->oy;
	segs[5].x2 = kp->cx + kp->yc + kp->ox;
	segs[5].y2 = kp->cy - kp->xd + kp->oy;
	segs[6].x1 = kp->cx + kp->xa + kp->ox;
	segs[6].y1 = kp->cy + kp->yb + kp->oy;
	segs[6].x2 = kp->cx + kp->xc + kp->ox;
	segs[6].y2 = kp->cy + kp->yd + kp->oy;
	segs[7].x1 = kp->cx + kp->ya + kp->ox;
	segs[7].y1 = kp->cy + kp->xb + kp->oy;
	segs[7].x2 = kp->cx + kp->yc + kp->ox;
	segs[7].y2 = kp->cy + kp->xd + kp->oy;
	XDrawSegments(dsp, win, Scr[screen].gc, segs, 8);
	kp->x1 = (kp->x1 + kp->xv1) % kp->m;
	kp->y1 = (kp->y1 + kp->yv1) % kp->m;
	kp->x2 = (kp->x2 + kp->xv2) % kp->m;
	kp->y2 = (kp->y2 + kp->yv2) % kp->m;
	kp->itercount++;

	if (kp->itercount > MI_CYCLES(mi)) {
		XClearWindow(dsp, win);
		kp->itercount = 0;
	}
	if (!MI_WIN_IS_MONO(mi) && MI_NPIXELS(mi) > 2) {
		XSetForeground(dsp, Scr[screen].gc, Scr[screen].pixels[kp->pix]);
		if (++kp->pix >= Scr[screen].npixels)
			kp->pix = 0;
	} else
		XSetForeground(dsp, Scr[screen].gc, WhitePixel(dsp, screen));
}
