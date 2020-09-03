#ifndef lint
static char sccsid[] = "@(#)opt.c	2.10 95/06/17 xlockmore";
#endif
/*
 * opt.c - optional stuff: xlockrc passwd and handle logout
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * 07-Jun-95: xlockrc passwd code, author unknown.
 *            Moved logout.c into opt.c .
 * 24-Feb-95: fullLock rewritten to handle non-default group names from
 *            Dale A. Harris <rodmur@ecst.csuchico.edu>
 * 13-Feb-95: Heath A. Kehoe <hakehoe@icaen.uiowa.edu>.
 *            Mostly taken from bomb.c
 * 1994:      bomb.c written.  Copyright (c) 1994 Dave Shield
 *            Liverpool Computer Science
 */

#include "xlock.h"

#ifdef USE_XLOCKRC
#undef passwd
#include <pwd.h>

void
gpass() {
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#define CPASSLENGTH 14
#define CPASSCHARS 64
  extern char userpass[PASSLENGTH];
  extern char *cpasswd;
  static char saltchars[CPASSCHARS + 1] =
    "abcdefghijklmnopwrstuvwxyzABCDEFGHIJKLMNOPWRSTUVWXYZ1234567890./";
  char xlockrc[MAXPATHLEN], *home, *getpass();
  FILE *fp;
 
  if (cpasswd[0] == '\0') {
    /*
     * No password given on command line or from database, get from
     * $HOME/.xlockrc instead.
     */
    if ((home = getenv("HOME")) == 0) {
      struct passwd *p = getpwuid(getuid());
      if (p == 0) {
        error("%s: Who are you?\n");
        exit(1);
      }
      home = p->pw_dir;
    }
 
    (void) strncpy(xlockrc, home, MAXPATHLEN);
    (void) strncat(xlockrc, "/.xlockrc", MAXPATHLEN);
    if ((fp = fopen(xlockrc, "r")) == NULL) {
      char pw[9], salt[2], *pw2;

      if ((fp = fopen(xlockrc, "w")) != NULL)
        (void) fchmod(fileno(fp), 0600);
      while(1) {
        (void) strncpy(pw, getpass("Key: "), sizeof pw);
        pw2 = getpass("Again:");
        if (strcmp(pw, pw2) != 0)
          error("%s: Mismatch, try again\n");
        else
          break;
      }
      /* grab a random printable character that isn't a colon */
      salt[0] = saltchars[LRAND() & (CPASSCHARS - 1)];
      salt[1] = saltchars[LRAND() & (CPASSCHARS - 1)];
#if defined(HAS_SHADOW) && defined(linux) && !defined(__QNX__)
#define crypt pw_encrypt
#endif
      (void) strncpy(userpass, (char *) crypt(pw, salt), CPASSLENGTH);
      if (fp)
        (void) fprintf(fp, "%s\n", userpass);
    } else {
       char buf[BUFSIZ];

       (void) fchmod(fileno(fp), 0600);
       buf[0] = '\0';
       if ((fgets(buf, sizeof buf, fp) == NULL) ||
           (!(strlen(buf) == CPASSLENGTH - 1 ||
             (strlen(buf) == CPASSLENGTH && buf[CPASSLENGTH - 1] == '\n')))) {
         (void) fprintf(stderr, "%s: %s crypted password %s\n", xlockrc,
           buf[0] == '\0' ? "null" : "bad", buf);
         exit(1);
       }
       buf[CPASSLENGTH - 1] = '\0';
       (void) strncpy(userpass, buf, CPASSLENGTH);
     }
     if (fp)
       (void) fclose(fp);
  } else {
     if (strlen(cpasswd) != CPASSLENGTH - 1) {
       error("%s: bad crypted password %s\n", cpasswd);
       exit(1);
     }
     else
       (void) strncpy(userpass, cpasswd, CPASSLENGTH);
  }
}
#endif

#if defined(AUTO_LOGOUT) || defined (LOGOUT_BUTTON)

#include <stdio.h>
#include <stdlib.h>
#ifdef SYSLOG
#include <syslog.h>
#endif
#include <sys/signal.h>

/*
 * This file contains a function called logoutUser() that, when called,
 * will (try) to log out the user.
 *
 * A portable way to do this is to simply kill all of the user's processes,
 * but this is a really ugly way to do it (it kills background jobs that
 * users may want to remain running after they log out).
 *
 * If your system provides for a cleaner/easier way to log out a user,
 * you may implement it here.
 *
 * For example, on some systems, one may define for the users an environment
 * variable named XSESSION that contains the pid of the X session leader
 * process.  So, to log the user out, we just need to kill that process,
 * and the X session will end.  Of course, a user can defeat that by
 * changing the value of XSESSION; so we can fall back on the ugly_logout()
 * method.
 *
 * If you can't log the user out (and you don't want to use the brute
 * force method) simply return from logoutUser(), and xlock will continue
 * on it's merry way (only applies if AUTO_LOGOUT or LOGOUT_BUTTON is
 * defined.)
 */

#define NAP_TIME        5               /* Sleep between shutdown attempts */

void ugly_logout()
{

#ifndef KILL_ALL_OTHERS
#define KILL_ALL_OTHERS -1
    (void) signal(SIGHUP, SIG_IGN);
    (void) signal(SIGTERM, SIG_IGN);
#endif

    (void) kill( KILL_ALL_OTHERS, SIGHUP );
    (void) sleep(NAP_TIME);
    (void) kill( KILL_ALL_OTHERS, SIGTERM );
    (void) sleep(NAP_TIME);

#ifdef SYSLOG
    syslog(LOG_NOTICE, "xlock failed to exit - sending kill (uid %d)\n",
    getuid());
#endif

    (void) kill( KILL_ALL_OTHERS, SIGKILL );
    (void) sleep(NAP_TIME);

#ifdef SYSLOG
    syslog(LOG_WARNING, "xlock still won't exit - suicide (uid %d)\n",
    getuid());
#endif

    (void) kill(getpid(), SIGKILL);
    exit(-1);

}

/* Decide which routine you want with a 1 or 0 */
#if 1

void logoutUser()
{

#ifdef SYSLOG
    syslog(LOG_INFO, "xlock expired. closing down (uid %d) on %s\n",
		getuid(), getenv("DISPLAY"));
#endif

    ugly_logout();
}

#else

/* sample logoutUser (described above) */

void logoutUser()
{
    char *pidstr;
    int pid;

#ifdef SYSLOG
    syslog(LOG_INFO, "xlock expired. closing down (uid %d) on %s\n",
              getuid(), getenv("DISPLAY"));
#endif

    pidstr = getenv("XSESSION");
    if(pidstr) {
	pid = atoi(pidstr);
	kill(pid, SIGTERM);
	sleep(5);
    }
/*    ugly_logout(); */
    (void) sleep(1);
}

#endif


        /*
         *  Determine whether to "fully" lock the terminal, or
         *    whether the time-limited version should be imposed.
         *
         *  Policy:
         *      Members of staff can fully lock
         *        (hard-wired user/group names + file read at run time)
         *      Students (i.e. everyone else)
         *          are forced to use the time-limit version.
         *
         *  An alternative policy could be based on display location
         */
#define FULL_LOCK       1
#define TEMP_LOCK       0
 
/* assuming only staff file is needed */
#ifndef STAFF_FILE
#define STAFF_FILE      "/usr/remote/etc/xlock.staff"
#endif
 
#undef passwd
#undef pw_name
#undef getpwnam
#undef getpwuid
#include <pwd.h>
#include <grp.h>
#include <sys/param.h>
#include <errno.h>
#ifndef NGROUPS
#include <limits.h> /* or <sys/param.h> */
#define NGROUPS NGROUPS_MAX
#endif

int fullLock()
{
    uid_t uid;
    int ngroups = NGROUPS;
    gid_t mygidset[NGROUPS];
    int ngrps, i;
    struct passwd *pwp;
    struct group *gp;
    FILE *fp;
    char buf[BUFSIZ];

    if ((uid = getuid()) == 0)
        return(FULL_LOCK);              /* root */
    if (inwindow || inroot || nolock) 
        return(FULL_LOCK);  /* (mostly) harmless user */

    pwp=getpwuid(uid);
    if ((ngrps = getgroups(ngroups, mygidset)) == -1)
        perror(ProgramName);

    if ((fp=fopen( STAFF_FILE, "r")) == NULL )
        return(TEMP_LOCK);

    while ((fgets( buf, BUFSIZ, fp )) != NULL ) {
          char    *cp;

          if ((cp = (char *) strchr(buf, '\n')) != NULL )
                *cp = '\0';
            if (!strcmp(buf, pwp->pw_name))
              return(FULL_LOCK);
            if ((gp=getgrnam(buf)) != NULL) /* check all of user's groups */
                for (i=0; i < ngrps; ++i)
                   if (gp->gr_gid == mygidset[i])
                       return(FULL_LOCK);
    }
 
    return(TEMP_LOCK);
}
#endif
