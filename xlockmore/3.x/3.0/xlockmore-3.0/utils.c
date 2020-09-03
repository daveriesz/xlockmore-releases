#ifndef lint
static char sccsid[] = "@(#)utils.c	2.10 95/05/14 xlockmore";
#endif
/*-
 * utils.c - various utilities - usleep, seconds, SetRNG, LongRNG, hsbramp.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@source.asset.com>
 * 15-May-95: random number generator added, moved hsbramp.c to utils.c .
 *            Also renamed file from usleep.c to utils.c .
 * 14-Mar-95: patches for rand and seconds for VMS
 * 27-Feb-95: fixed nanosleep for times >= 1 second
 * 05-Jan-95: nanosleep for Solaris 2.3 and greater Greg Onufer
 *            <Greg.Onufer@Eng.Sun.COM>
 * 22-Jun-94: Fudged for VMS by Anthony Clarke
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * 10-Jun-94: patch for BSD from Victor Langeveld <vic@mbfys.kun.nl>
 * 02-May-94: patch for Linux, got ideas from Darren Senn's xlock
 *            <sinster@scintilla.capitola.ca.us>
 * 21-Mar-94: patch fix for HP from <R.K.Lloyd@csc.liv.ac.uk> 
 * 01-Dec-93: added patch for HP
 *
 * Changes of Patrick J. Naughton
 * 30-Aug-90: written.
 *
 */

#include "xlock.h"

#if defined(SYSV) || defined(SVR4)
#include <sys/time.h>
#ifdef LESS_THAN_AIX3_2 
#include <sys/poll.h>
#else /* !LESS_THAN_AIX3_2 */
#include <poll.h>
#endif /* !LESS_THAN_AIX3_2 */
 
#endif /* defined(SYSV) || defined(SVR4) */

#if !defined(HAS_USLEEP)
 /* usleep should be defined */
int
usleep(usec)
    unsigned long usec;
{
#if (defined (SYSV) || defined(SVR4)) && !defined(__hpux)
#if defined(HAS_NANOSLEEP)
{
    struct timespec rqt;

    rqt.tv_nsec = 1000 * (usec % (unsigned long) 1000000);
    rqt.tv_sec = usec / (unsigned long) 1000000;
    return nanosleep(&rqt, NULL);
}
#else
    (void) poll((void *) 0, (size_t) 0, usec / 1000);	/* ms resolution */
#endif
#else
#ifdef VMS
    long timadr[2];
 
    if (usec !=0) {
      timadr[0] = -usec*10;
      timadr[1] = -1;
 
      sys$setimr(4,&timadr,0,0,0);
      sys$waitfr(4);
    }
#else
    struct timeval time_out;
    time_out.tv_usec = usec % (unsigned long) 1000000;
    time_out.tv_sec = usec / (unsigned long) 1000000;
    (void) select(0, (void *) 0, (void *) 0, (void *) 0, &time_out);
#endif
#endif
    return 0;
}
#endif

/*
 * returns the number of seconds since 01-Jan-70.
 * This is used to control rate and timeout in many of the animations.
 */
long
seconds()
{
#ifdef VMS
    return (long) time(NULL);
#else
    struct timeval now;

    (void) gettimeofday(&now, (struct timezone *) 0);
    return now.tv_sec;
#endif
}

/*
Dr. Park's algorithm published in the Oct. '88 ACM 
"Random Number Generators: Good Ones Are Hard To Find"
His version available at ftp://cs.wm.edu/pub/rngs.tar
Present form by many authors.
*/

static long Seed = 1;       /* This is required to be 32 bits long */

/*
 *      Given an integer, this routine initializes the RNG seed.
 */
void SetRNG(s)
long s;
{
  Seed = s;
}

/*
 *      Returns an integer between 0 and 2147483647, inclusive.
 */
long LongRNG()
{
  if ((Seed = Seed % 44488 * 48271 - Seed / 44488 * 3399) < 0)
    Seed += 2147483647;
  return Seed - 1;
}

/*-
 * Create an HSB ramp.
 *
 * Revision History:
 * Changes of David Bagley <bagleyd@source.asset.com>
 * 22-Jun-94: Modified for VMS
 *            <Anthony.D.Clarke@Support.Hatfield.Raytheon.bae.eurokom.ie>
 * Changes of Patrick J. Naughton
 * 29-Jul-90: renamed hsbramp.c from HSBmap.c
 *	      minor optimizations.
 * 01-Sep-88: Written.
 */

#include <sys/types.h>
#include <math.h>

static void
hsb2rgb(H, S, B, r, g, b)
    double H, S, B;
    unsigned char *r, *g, *b;
{
    int i;
    double f, bb;
    unsigned char p, q, t;

    H -= floor(H);		/* remove anything over 1 */
    H *= 6.0;
    i = floor(H);		/* 0..5 */
    f = H - (float) i;		/* f = fractional part of H */
    bb = 255.0 * B;
    p = (unsigned char) (bb * (1.0 - S));
    q = (unsigned char) (bb * (1.0 - (S * f)));
    t = (unsigned char) (bb * (1.0 - (S * (1.0 - f))));
    switch (i) {
    case 0:
	*r = (unsigned char) bb;
	*g = t;
	*b = p;
	break;
    case 1:
	*r = q;
	*g = (unsigned char) bb;
	*b = p;
	break;
    case 2:
	*r = p;
	*g = (unsigned char) bb;
	*b = t;
	break;
    case 3:
	*r = p;
	*g = q;
	*b = (unsigned char) bb;
	break;
    case 4:
	*r = t;
	*g = p;
	*b = (unsigned char) bb;
	break;
    case 5:
	*r = (unsigned char) bb;
	*g = p;
	*b = q;
	break;
    }
}


/*
 * Input is two points in HSB color space and a count
 * of how many discreet rgb space values the caller wants.
 *
 * Output is that many rgb triples which describe a linear
 * interpolate ramp between the two input colors.
 */

void
hsbramp(h1, s1, b1, h2, s2, b2, count, red, green, blue)
    double h1, s1, b1, h2, s2, b2;
    int count;
    unsigned char *red, *green, *blue;
{
    double dh, ds, db;

    dh = (h2 - h1) / count;
    ds = (s2 - s1) / count;
    db = (b2 - b1) / count;
    while (count--) {
	hsb2rgb(h1, s1, b1, red++, green++, blue++);
	h1 += dh;
	s1 += ds;
	b1 += db;
    }
}
