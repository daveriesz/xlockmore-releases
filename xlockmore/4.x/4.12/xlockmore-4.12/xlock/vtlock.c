#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)vtlock.c	1.00 98/07/01 xlockmore";

#endif

/* Copyright (c) E. Lassauge/ R. Cohen-Scali, 1998. */

/*
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
 * The original code for was written by R. Cohen-Scali 
 * (remi.cohenscali@pobox.com) for a command line vtswich control tool.
 *
 * My e-mail address is lassauge@sagem.fr
 *
 */


#include "xlock.h"


#ifdef USE_VTLOCK
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

#define CONSOLE "/dev/console"

int         vtlocked = 0;

static uid_t ruid;

static void
getrootprivs()
{
	ruid = getuid();

	(void) seteuid(0);
}

/* revoke root privs, if there were any */
static void
revokerootprivs()
{
	(void) seteuid(ruid);
}

static void
vtlock(Bool lock)
{
	int         consfd = -1;
	struct stat consstat;

	if (stat(CONSOLE, &consstat) == -1) {
		return;
	}
	getrootprivs();

	if (ruid != consstat.st_uid) {
		revokerootprivs();
		return;
	}
	/* Open console */
	if ((consfd = open(CONSOLE, O_RDWR)) == -1) {
		revokerootprivs();
		return;
	}
	/* Do it */
	if (ioctl(consfd, lock ? VT_LOCKSWITCH : VT_UNLOCKSWITCH) == -1) {
		revokerootprivs();
		(void) close(consfd);
		return;
	}
	/* Terminate */
	(void) close(consfd);
	vtlocked = lock;
	revokerootprivs();
}

void
dovtlock()
{
	vtlock(True);
}

void
dounvtlock()
{
	vtlock(False);
}

#endif
