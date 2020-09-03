#ifndef lint
static char sccsid[] = "@(#)passwd.c	3.0 95/07/14 xlockmore";
#endif
/*-
 * passwd.c - passwd stuff.
 *
 * Copyright (c) 1988-91 by Patrick J. Naughton.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@source.asset.com>
 * 24-Jun-95: Extracted from xlock.c, encrypted passwords are now fetched
 *            on start-up to ensure correct operation (except Ultrix).
 */

#include <stdio.h>
#include <string.h>

#include "xlock.h"

#ifdef VMS
#define ROOT "SYSTEM"
#else
#define ROOT "root"
#endif
#if !defined(VMS) && !defined(SUNOS_ADJUNCT_PASSWD)
#include <pwd.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__hpux) || defined(SUNOS_ADJUNCT_PASSWD)
#include <sys/types.h>
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__QNX__) || defined(HPUX_SECURE_PASSWD) || defined(SOLARIS_SHADOW) || defined(SUNOS_ADJUNCT_PASSWD)
#define SECURE_PASSWD
#endif

#ifdef ultrix
#include <auth.h>
#endif

#ifdef OSF1_ENH_SEC
#include <sys/types.h>
#include <sys/security.h>
#include <prot.h>
#endif

/* System V Release 4 redefinitions of BSD functions and structures */
#if defined(SYSV) || defined(SVR4)

#ifdef LESS_THAN_AIX3_2
struct passwd {
  char  *pw_name;
  char  *pw_passwd;
  uid_t pw_uid;
  gid_t pw_gid;
  char  *pw_gecos;
  char  *pw_dir;
  char  *pw_shell;
};
#endif /* LESS_THAN_AIX3_2 */

#if defined(HAS_SHADOW) || defined(SOLARIS_SHADOW)
#define passwd spwd
#define pw_passwd sp_pwdp
#ifndef __hpux
#include <shadow.h>
#endif
#define getpwnam getspnam
#ifdef SOLARIS_SHADOW
#define pw_name sp_namp
#endif
#endif /* defined(HAS_SHADOW) || defined(SOLARIS_SHADOW) */

#ifdef SUNOS_ADJUNCT_PASSWD
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>
#define passwd passwd_adjunct
#define pw_passwd pwa_passwd
#define getpwnam(_s) getpwanam(_s)
#define pw_name pwa_name
#endif /* SUNOS_ADJUNCT_PASSWD */

#ifdef HPUX_SECURE_PASSWD
#define seteuid(_eu) setresuid(-1, _eu, -1)
#define passwd s_passwd
#define getpwnam(_s) getspwnam(_s)
#define getpwuid(_u) getspwuid(_u)
#endif /* HPUX_SECURE_PASSWD */

#else  /* defined(SYSV) || defined(SVR4) */

#if defined(HAS_SHADOW)
#include <shadow.h>
#define passwd spwd
#define pw_passwd sp_pwdp
#define pw_name sp_namp
#ifdef linux
#define getpwnam getspnam
#endif
#endif

#endif /* defined(SYSV) || defined(SVR4) */

#ifdef VMS
#include <uaidef.h>
#define VMS_PASSLENGTH 9
static short uai_salt, root_salt;
static char hash_password[VMS_PASSLENGTH], hash_system[VMS_PASSLENGTH];
static char root_password[VMS_PASSLENGTH], root_system[VMS_PASSLENGTH];
static char uai_encrypt, root_encrypt;

struct ascid {
  short len;
  char  dtype;
  char  class;
  char  *addr;
};
 
static struct ascid username, rootuser;

struct itmlst {
  short buflen;
  short code;
  long  addr;
  long  retadr;
};

#endif /* VMS */

#ifdef HP_PASSWDETC
#include <sys/wait.h>
#endif /* HP_PASSWDETC */

#ifdef AFS
#include <afs/kauth.h>
#include <afs/kautils.h>
#endif /* AFS */

#ifndef USE_XLOCKRC
static
#endif
char userpass[PASSLENGTH];
char user[PASSLENGTH];
static char rootpass[PASSLENGTH];
#ifdef VMS
static char root[] = ROOT;
#endif

extern char *crypt();

void
getUserName()
{

#ifdef VMS
  (void) strcpy(user, cuserid(NULL));
#else /* !VMS */
#ifdef HP_PASSWDETC

/*
 * The PasswdEtc libraries have replacement passwd functions that make
 * queries to DomainOS registries.  Unfortunately, these functions do
 * wierd things to the process (at minimum, signal handlers get changed,
 * there are probably other things as well) that cause xlock to become
 * unstable.
 *
 * As a (really, really sick) workaround, we'll fork() and do the getpw*()
 * calls in the child, and pass the information back through a pipe.
 */
  struct passwd *pw;
  int pipefd[2], n, total = 0, stat_loc;
  pid_t pid;

  pipe(pipefd);

  if ((pid = fork()) == 0) {
    close(pipefd[0]);
    pw = getpwuid(getuid());
    write(pipefd[1], pw->pw_name, strlen(pw->pw_name));
    close(pipefd[1]);
    _exit(0);
  }
  if (pid < 0)
    error("%s: could not get user password (fork failed)\n");
  close(pipefd[1]);

  while ((n = read(pipefd[0], &(user[total]), 50)) > 0)
    total += n;

  wait(&stat_loc);

  if (n < 0)
    error("%s: could not get user name (read failed)\n");
  user[total] = 0;

  if (total < 1)
    error("%s: could not get user name (lookups failed)\n");

#else /* !HP_PASSWDETC */
#ifdef OSF1_ENH_SEC
  struct pr_passwd *pw;

  if ((pw = getprpwuid(getuid())) == NULL)
    error("%s: could not get user name.\n");
  (void) strcpy(user, pw->ufld.fd_name);

#else /* !OSF1_ENH_SEC */

  struct passwd *pw;

#if defined(SECURE_PASSWD)
  seteuid(0); /* Need to be root now */
#endif

  if ((pw = (struct passwd *) getpwuid(getuid())) == NULL)
    error("%s: could not get user name.\n");
  (void) strcpy(user, pw->pw_name);

#if defined(SECURE_PASSWD)
  seteuid(getuid()); /* Don't need to be root anymore */
#endif

#endif /* !OSF1_ENH_SEC */
#endif /* !HP_PASSWDETC */
#endif /* !VMS */
}

void
getCryptedUserPasswd()
{

#ifdef VMS
  struct itmlst il[4];

  il[0].buflen = 2;
  il[0].code   = UAI$_SALT;
  il[0].addr   = &uai_salt;
  il[0].retadr = 0;
  il[1].buflen = 8;
  il[1].code   = UAI$_PWD;
  il[1].addr   = &hash_password;
  il[1].retadr = 0;
  il[2].buflen = 1;
  il[2].code   = UAI$_ENCRYPT;
  il[2].addr   = &uai_encrypt;
  il[2].retadr = 0;
  il[3].buflen = 0;
  il[3].code   = 0;
  il[3].addr   = 0;
  il[3].retadr = 0;

  username.len   = strlen(user);
  username.dtype = 0;
  username.class = 0;
  username.addr  = user;

  sys$getuai(0, 0, &username, &il, 0, 0, 0);
#else /* !VMS */
#ifdef HP_PASSWDETC

/*
 * still very sick, see above
 */
  struct passwd *pw;
  int pipefd[2], n, total = 0, stat_loc;
  pid_t pid;

  pipe(pipefd);

  if ((pid = fork()) == 0) {
    close(pipefd[0]);
    pw = getpwuid(getuid());
    write(pipefd[1], pw->pw_passwd, strlen(pw->pw_passwd));
    close(pipefd[1]);
    _exit(0);
  }
  if (pid < 0)
    error("%s: could not get user password (fork failed)\n");
  close(pipefd[1]);

  while ((n = read(pipefd[0], &(userpass[total]), 50)) > 0)
    total += n;

  wait(&stat_loc);

  if (n < 0)
    error("%s: could not get user password (read failed)\n");
  user[total] = 0;

  if (total < 1)
    error("%s: could not get user password (lookups failed)\n");

#else /* !HP_PASSWDETC */
#ifdef OSF1_ENH_SEC
  struct pr_passwd *pw;

  if ((pw = getprpwuid(getuid())) == NULL)
    error("%s: could not get encrypted user password.\n");
  (void) strcpy(userpass, pw->ufld.fd_encrypt);

#else /* !OSF1_ENH_SEC */

  struct passwd *pw;

#if defined(SECURE_PASSWD)
  seteuid(0); /* Need to be root now */
#endif

#ifdef SOLARIS_SHADOW
#define GETPWNAM getspnam(cuserid(user))
#else /* !SOLARIS_SHADOW */
#if defined(HAS_SHADOW) && defined(linux)
#define GETPWNAM getspnam(user)
#else /* !LINUX_SHADOW */
#define GETPWNAM getpwuid(getuid())
#endif /* !LINUX_SHADOW */
#endif /* !SOLARIS_SHADOW */
/*#define GETPWNAM getpwnam(cuserid(NULL)) real old way */

  if ((pw = (struct passwd *) GETPWNAM) == NULL)
    error("%s: could not get encrypted user password.\n");
  (void) strcpy(userpass, pw->pw_passwd);

#if defined(SECURE_PASSWD)
  seteuid(getuid()); /* Don't need to be root anymore */
#endif

#endif /* !OSF1_ENH_SEC */
#endif /* !HP_PASSWDETC */
#endif /* !VMS */
}

void
getCryptedRootPasswd()
{

#ifdef VMS
  struct itmlst il[4];

  il[0].buflen = 2;
  il[0].code   = UAI$_SALT;
  il[0].addr   = &root_salt;
  il[0].retadr = 0;
  il[1].buflen = 8;
  il[1].code   = UAI$_PWD;
  il[1].addr   = &root_password;
  il[1].retadr = 0;
  il[2].buflen = 1;
  il[2].code   = UAI$_ENCRYPT;
  il[2].addr   = &root_encrypt;
  il[2].retadr = 0;
  il[3].buflen = 0;
  il[3].code   = 0;
  il[3].addr   = 0;
  il[3].retadr = 0;

  rootuser.len   = strlen(root);
  rootuser.dtype = 0;
  rootuser.class = 0;
  rootuser.addr  = root;

  sys$getuai(0, 0, &rootuser, &il, 0, 0, 0);
#else /* !VMS */
#ifdef HP_PASSWDETC

/*
 * Still really, really sick.  See above.
 */
  struct passwd *pw;
  int pipefd[2], n, total = 0, stat_loc;
  pid_t pid;

  pipe(pipefd);

  if ((pid = fork()) == 0) {
    close(pipefd[0]);
    pw = getpwnam(ROOT);
    write(pipefd[1], pw->pw_passwd, strlen(pw->pw_passwd));
    close(pipefd[1]);
    _exit(0);
  }
  if (pid < 0)
    error("%s: could not get root password (fork failed)\n");
  close(pipefd[1]);

  while ((n = read(pipefd[0], &(rootpass[total]), 50)) > 0)
    total += n;

  wait(&stat_loc);

  if (n < 0)
    error("%s: could not get root password (read failed)\n");
  rootpass[total] = 0;

  if (total < 1)
    error("%s: could not get root password (lookups failed)\n");

#else /* !HP_PASSWDETC */
#ifdef OSF1_ENH_SEC
  struct pr_passwd *pw;

  if ((pw = getprpnam(ROOT)) == NULL)
    error("%s: could not get encrypted root password.\n");
  (void) strcpy(rootpass, pw->ufld.fd_encrypt);

#else /* !OSF1_ENH_SEC */
  struct passwd *pw;

#if defined(SECURE_PASSWD)
  seteuid(0); /* Need to be root now */
#endif

  if ((pw = (struct passwd *) getpwnam(ROOT)) == NULL)
    error("%s: could not get encrypted root password.\n");
  (void) strcpy(rootpass, pw->pw_passwd);

#if defined(SECURE_PASSWD)
  seteuid(getuid()); /* Don't need to be root anymore */
#endif

#endif /* !OSF1_ENH_SEC */
#endif /* !HP_PASSWDETC */
#endif /* !VMS */
}


/*
 * we don't allow for root to have no password, but we handle the case
 * where the user has no password correctly; they have to hit return
 * only
 */
int
checkPasswd(buffer)
char *buffer;
{
  int done;

#ifdef VMS
  struct ascid password;

  password.len   = strlen(buffer);
  password.dtype = 0;
  password.class = 0;
  password.addr  = buffer;
 
  str$upcase(&password, &password);
 
  sys$hash_password(&password, uai_encrypt, uai_salt,
                    &username, &hash_system);

  hash_password[VMS_PASSLENGTH - 1] = 0;
  hash_system[VMS_PASSLENGTH - 1]   = 0;

  done = !strcmp(hash_password, hash_system);
  if (!done && allowroot) {
    sys$hash_password(&password, root_encrypt, root_salt,
                      &rootuser, &root_system);
    root_password[VMS_PASSLENGTH - 1] = 0;
    root_system[VMS_PASSLENGTH - 1]   = 0;
    done = !strcmp(root_password, root_system);
  }
#else /* !VMS */

#ifdef ultrix
  struct passwd *pw;

  done = ((authenticate_user(pw, buffer, NULL) >= 0) ||
          (allowroot && (authenticate_user((struct passwd *)
            getpwnam(ROOT), buffer, NULL) >= 0)));
#else /* !ultrix */

#if defined(HAS_SHADOW) && defined(linux) && !defined(__QNX__)
#define crypt pw_encrypt
#endif

#ifdef AFS
  char *reason;

  /* check afs passwd first, then local, then root */
  done = !ka_UserAuthenticate(user, "", 0, buffer, 0, &reason);
  if (!done)
#endif /* !AFS */
  done = !((strcmp((char *) crypt(buffer, userpass), userpass)) &&
           (!allowroot || strcmp((char *) crypt(buffer, rootpass), rootpass)));

#endif /* !ultrix */
#endif /* !VMS */

#if !defined (ultrix) && !defined (VMS)
  /* userpass is used */
  if (!*userpass && *buffer)
    /*
     * the user has no password, but something was typed anyway.
     * sounds fishy: don't let him in...
     */
    done = False;
#endif
  return done;
}
