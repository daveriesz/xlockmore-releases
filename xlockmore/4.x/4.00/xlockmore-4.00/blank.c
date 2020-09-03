
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)blank.c	4.0 97/01/01 xlockmore";

#endif

/*-
 * blank.c - blank screen for xlock, the X Window System lockscreen.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 21-Mar-96: Ron Hitchens <ron@idiom.com>
 *		Bonehead alert.  Don't blank during password prompting.
 * 19-Mar-96: Ron Hitchens <ron@idiom.com>
 *		Changed to activate X server's native screensaver.
 *		On some devices, this will result in power saving "sleep"
 *		mode, or video blanking.
 * 31-Aug-90: Written.
 */

#include "xlock.h"

ModeSpecOpt blank_opts =
{0, NULL, 0, NULL, NULL};

void
init_blank(ModeInfo * mi)
{
	XClearWindow(MI_DISPLAY(mi), MI_WINDOW(mi));
	/* Must set PreferBlanking, or XForceScreenSaver won't work */
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi))
		XSetScreenSaver(MI_DISPLAY(mi), 0, 0, PreferBlanking, 0);
}

/* ARGSUSED */
void
draw_blank(ModeInfo * mi)
{
	/* Leave the lights on while user types password */
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi)) {
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
	if (!MI_WIN_IS_INWINDOW(mi) && !MI_WIN_IS_INROOT(mi)) {
		XForceScreenSaver(MI_DISPLAY(mi), ScreenSaverReset);
		XSetScreenSaver(MI_DISPLAY(mi), 0, 0, 0, 0);
	}
}

void
refresh_blank(ModeInfo * mi)
{
	/* Do nothing, it will refresh by itself :) */
}
