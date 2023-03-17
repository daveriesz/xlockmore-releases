
/* -*- Mode: C; tab-width: 4 -*- */

#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)blank.c	4.03 97/05/10 xlockmore";

#endif

/*-
 * blank.c - blank screen for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
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
 * 10-May-97: Compatible with xscreensaver :)  OK you probably should not
 *    use this for xscreensaver but I could not resist.
 * 21-Mar-96: Ron Hitchens <ron@idiom.com>
 *		Bonehead alert.  Don't blank during password prompting.
 * 19-Mar-96: Ron Hitchens <ron@idiom.com>
 *		Changed to activate X server's native screensaver.
 *		On some devices, this will result in power saving "sleep"
 *		mode, or video blanking.
 * 31-Aug-90: Written.
 */

#ifdef STANDALONE
#define PROGCLASS            "Blank"
#define HACK_INIT            init_blank
#define HACK_DRAW            draw_blank
#define DEF_DELAY            3000000
#include "xlockmore.h"		/* in xscreensaver distribution */
#else /* STANDALONE */
#include "xlock.h"		/* in xlockmore distribution */

ModeSpecOpt blank_opts =
{0, NULL, 0, NULL, NULL};

#endif /* STANDALONE */

extern enablesaver;

void
init_blank(ModeInfo * mi)
{
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	/* Must set PreferBlanking, or XForceScreenSaver won't work */
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi) && enablesaver)
		XSetScreenSaver(MI_DISPLAY(mi), 0, 0, PreferBlanking, 0);
}

/* ARGSUSED */
void
draw_blank(ModeInfo * mi)
{
	/* Leave the lights on while user types password */
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi) && enablesaver) {
		if (MI_WIN_IS_ICONIC(mi))
			XForceScreenSaver(MI_DISPLAY(mi), ScreenSaverReset);
		else
			XForceScreenSaver(MI_DISPLAY(mi), ScreenSaverActive);
	}
}

void
release_blank(ModeInfo * mi)
{
	/* clear screensaver settings, just in case */
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi) && enablesaver) {
		XForceScreenSaver(MI_DISPLAY(mi), ScreenSaverReset);
		XSetScreenSaver(MI_DISPLAY(mi), 0, 0, 0, 0);
	}
}

void
refresh_blank(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself :) */
}
