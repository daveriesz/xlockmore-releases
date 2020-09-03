#ifndef lint
static char sccsid[] = "@(#)utils.c	3.9 96/05/25 xlockmore";

#endif

/*-
 * utils.c - various utilities - usleep, seconds, SetRNG, LongRNG, hsbramp.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@hertz.njit.edu>
 *  4-Apr-96: Added procedures to handle wildcards on filenames
 *            J.Jansen <joukj@crys.chem.uva.nl>
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

#ifdef OLD_EVENT_LOOP
#if defined( SYSV ) || defined( SVR4 )
#include <sys/time.h>
#ifdef LESS_THAN_AIX3_2
#include <sys/poll.h>
#else /* !LESS_THAN_AIX3_2 */
#include <poll.h>
#endif /* !LESS_THAN_AIX3_2 */

#endif /* defined( SYSV ) || defined( SVR4 ) */

#ifndef HAS_USLEEP
 /* usleep should be defined */
int
usleep(unsigned long usec)
{
#if (defined( SYSV ) || defined( SVR4 )) && !defined( __hpux )
#ifdef HAS_NANOSLEEP
	{
		struct timespec rqt;

		rqt.tv_nsec = 1000 * (usec % (unsigned long) 1000000);
		rqt.tv_sec = usec / (unsigned long) 1000000;
		return nanosleep(&rqt, NULL);
	}
#else
	(void) poll((void *) 0, (int) 0, usec / 1000);	/* ms resolution */
#endif
#else
#if defined( VMS ) && ( __VMS_VER < 70000000 )
	long        timadr[2];

	if (usec != 0) {
		timadr[0] = -usec * 10;
		timadr[1] = -1;

		sys$setimr(4, &timadr, 0, 0, 0);
		sys$waitfr(4);
	}
#else
	struct timeval time_out;
	extern int  select();

	time_out.tv_usec = usec % (unsigned long) 1000000;
	time_out.tv_sec = usec / (unsigned long) 1000000;
	(void) select(0, (void *) 0, (void *) 0, (void *) 0, &time_out);
#endif
#endif
	return 0;
}
#endif
#endif

/* 
 * returns the number of seconds since 01-Jan-70.
 * This is used to control rate and timeout in many of the animations.
 */
long
seconds(void)
{
#if defined( VMS ) && ( __VMS_VER < 70000000 )
	return (long) time(NULL);
#else
	struct timeval now;

	(void) gettimeofday(&now, NULL);
	return now.tv_sec;
#endif
}

int
readable(char *filename)
{
	FILE       *file;

	if (!(file = fopen(filename, "rb")))
		return False;
	(void) fclose(file);
	return True;
}

#if defined( ultrix ) || (defined( VMS ) && ( __VMS_VER < 70000000 ))
char       *
strdup(char *str)
{
	register char *ptr;

	ptr = malloc(strlen(str) + 1);
	(void) strcpy(ptr, str);
	return ptr;
}
#endif

/* 
   Dr. Park's algorithm published in the Oct. '88 ACM  "Random Number
   Generators: Good Ones Are Hard To Find" His version available at
   ftp://cs.wm.edu/pub/rngs.tar Present form by many authors. */

static int  Seed = 1;		/* This is required to be 32 bits long */

/* 
 *      Given an integer, this routine initializes the RNG seed.
 */
void
SetRNG(long int s)
{
	Seed = (int) s;
}

/* 
 *      Returns an integer between 0 and 2147483647, inclusive.
 */
long
LongRNG(void)
{
	if ((Seed = Seed % 44488 * 48271 - Seed / 44488 * 3399) < 0)
		Seed += 2147483647;
	return (long) (Seed - 1);
}


#include <ctype.h>
#define FROM_PROGRAM 1
#define FROM_FORMATTEDFILE    2
#define FROM_FILE    3
#define FROM_RESRC   4

char       *program, *messagesfile, *messagefile, *message, *mfont;

static char *def_words = "I'm out running around.";
static int  getwordsfrom;

static void
strcat_firstword(char *fword, char *words)
{
	while (*fword)
		fword++;
	while (*words && !(isspace(*words)))
		*fword++ = *words++;
}

int
is_ribbon(void)
{
	return (getwordsfrom == FROM_RESRC);
}

char       *
get_words(void)
{
	FILE       *pp;
	static char buf[BUFSIZ], progerr[BUFSIZ], progrun[BUFSIZ];
	register char *p = buf;
	extern int  pclose(FILE *);
	int         i;

	buf[0] = '\0';

	if (strlen(message))
		getwordsfrom = FROM_RESRC;
	else if (strlen(messagefile)) {
		getwordsfrom = FROM_FILE;
		if (!messagefile)
			error("%s: No file specified.\n");
	} else if (strlen(messagesfile)) {
		getwordsfrom = FROM_FORMATTEDFILE;
		if (!messagesfile)
			error("%s: No file specified.\n");
	} else {
		getwordsfrom = FROM_PROGRAM;
		(void) sprintf(progrun, "( %s ) 2>&1",
			       (!program) ? DEF_PROGRAM : program);
	}

	switch (getwordsfrom) {
#ifndef VMS
		case FROM_PROGRAM:
			if ((pp = popen(progrun, "r"))) {
				while (fgets(p, sizeof (buf) - strlen(buf), pp)) {
					if (strlen(buf) + 1 < sizeof (buf))
						p = buf + strlen(buf);
					else
						break;
				}
				(void) pclose(pp);
				p = buf;
				if (!buf[0])
					(void) sprintf(buf, "\"%s\" produced no output!",
					 (!program) ? DEF_PROGRAM : program);
				else {
					(void) strcpy(progerr, "sh: ");
					strcat_firstword(progerr, (!program) ? DEF_PROGRAM : program);
					(void) strcat(progerr, ": not found\n");
					if (!strcmp(buf, progerr))
						switch (LRAND() % 7) {
							case 0:
								(void) strcat(buf, "( Get with the program, bub. )\n");
								break;
							case 1:
								(void) strcat(buf,
									      "( I blow my nose at you, you silly person! )\n");
								break;
							case 2:
								(void) strcat(buf,
									      "\nThe resource you want to\nset is `program'\n");
								break;
							case 3:
								(void) strcat(buf, "\nHelp!!  Help!!\nAAAAAAGGGGHHH!!  \n\n");
								break;
							case 4:
								(void) strcat(buf, " Hey, who called me `Big Nose'? \n");
								break;
							case 5:
								(void) strcat(buf, "( Hello?  Are you paying attention? )\n");
								break;
							case 6:
								(void) strcat(buf,
									      "sh: what kind of fool do you take me for? \n");
								break;
						}
				}
				p = buf;
			} else {
				perror(progrun);
				p = def_words;
			}
			break;
#endif
		case FROM_FORMATTEDFILE:
			if ((pp = fopen(messagesfile, "r"))) {
				int         len_mess_file;

				if (fscanf(pp, "%d", &len_mess_file)) {
					int         no_quote;

					if (len_mess_file <= 0)
						buf[0] = '\0';
					else {
						(void) fgets(p, sizeof (buf) - strlen(buf), pp);
						/* get first '%%' (the one after the number of quotes) */
						(void) fgets(p, sizeof (buf) - strlen(buf), pp);
						no_quote = LRAND() % len_mess_file;
						for (i = 0; i <= no_quote; ++i) {
							int         len_cur = 0;

							buf[0] = '\0';
							p = buf + strlen(buf);
							while (fgets(p, sizeof (buf) - strlen(buf), pp)) {
								if (strlen(buf) + 1 < sizeof (buf)) {
									/* a line with '%%' contains 3 characters */
									if ((strlen(buf) == len_cur + 3) &&
									    (p[0] == '%') && (p[1] == '%')) {
										p[0] = '\0';	/* get rid of "%%" in buf */
										break;
									} else {
										p = buf + strlen(buf);
										len_cur = strlen(buf);
									}
								} else
									break;
							}
						}
					}
					(void) fclose(pp);
					if (!buf[0])
						(void) sprintf(buf, "file \"%s\" is empty!", messagesfile);
					p = buf;
				} else {
					(void) sprintf(buf, "file \"%s\" not in correct format!",
						       messagesfile);
					p = buf;
				}
			} else {
				(void) sprintf(buf, "could not read file \"%s\"!", messagesfile);
				p = buf;
			}
			break;
		case FROM_FILE:
			if ((pp = fopen(messagefile, "r"))) {
				while (fgets(p, sizeof (buf) - strlen(buf), pp)) {
					if (strlen(buf) + 1 < sizeof (buf))
						p = buf + strlen(buf);
					else
						break;
				}
				(void) fclose(pp);
				if (!buf[0])
					(void) sprintf(buf, "file \"%s\" is empty!", messagefile);
				p = buf;
			} else {
				(void) sprintf(buf, "could not read file \"%s\"!", messagefile);
				p = buf;
			}
			break;
		case FROM_RESRC:
			p = message;
			break;
		default:
			p = def_words;
			break;
	}

	if (!p || *p == '\0')
		p = def_words;
	return p;
}

XFontStruct *
get_font(Display * display)
{
	XFontStruct *mode_font;

	if (!(mode_font = XLoadQueryFont(display, (mfont) ? mfont : DEF_MFONT))) {
		if (mfont) {
			warning("%s: can't find font: %s, using %s...\n", mfont, DEF_MFONT);
			mode_font = XLoadQueryFont(display, DEF_MFONT);
		}
		if (!(mode_font)) {
			warning("%s: can't find font: %s, using %s...\n", DEF_MFONT,
				FALLBACK_FONTNAME);
			mode_font = XLoadQueryFont(display, FALLBACK_FONTNAME);
			if (!mode_font)
				error("%s: can't even find %s!!!\n", FALLBACK_FONTNAME);
		}
	}
	return mode_font;
}

void
randomImage(char *im_file)
{
#if !defined( VMS ) || defined( XVMSUTILS ) || ( __VMS_VER >= 70000000 )
  extern char directory_[512];
  extern struct dirent **image_list;
  extern int  num_list;
  int         num;
 
  if (num_list > 0) {
    num = LRAND() % num_list;
    (void) strcpy(im_file, directory_);
    (void) strcat(im_file, image_list[num]->d_name);
  }
#endif
}

#if !defined( VMS ) || defined( XVMSUTILS ) ||  ( __VMS_VER >= 70000000 )

/* index_ emulation of FORTRAN's index in C. * Author : J.Jansen */
static int
index_(char *str1, char *substr)
{
	int         i, num, l1 = strlen(str1), ls = strlen(substr);
	char       *str1_, *substr_, *substr_last;

	num = l1 - ls + 1;
	substr_last = substr + ls;
	for (i = 0; i < num; ++i) {
		str1_ = str1 + i;
		substr_ = substr;
		while (substr_ < substr_last)
			if (*str1_++ != *substr_++)
				goto not_found;
		return (i + 1);
	      not_found:;
	}
	return (0);
}

#ifdef VMS
/* Upcase string * Author J.Jansen */
void
upcase(char *s)
{
	int         i;
	char        c[1];

	for (i = 0; s[i] != '\0'; i++) {
		if (s[i] >= 'a' && s[i] <= 'z') {
			*c = s[i];
			s[i] = (char) (*c - 32);
		}
	}
}

#endif

/* Split full path into directory and filename parts * author : J.Jansen */
void
get_direc(char *fullpath, char *dir, char *filn_)
{
	char       *ln;
	int         ip_ = 0, ip;

#ifdef VMS
	ip = index_(fullpath, "]");
#else
	ln = fullpath;
	ip = 0;
	while ((ip_ = index_(ln, "/"))) {
		ip = ip + ip_;
		ln = fullpath + ip;
	}
#endif
	if (ip == 0) {
		(void) strcpy(filn_, fullpath);
#ifdef VMS
		(void) strcpy(dir, "[]");
#else
		(void) strcpy(dir, "./");
#endif
	} else {
		(void) strcpy(dir, "\0");
		(void) strncat(dir, fullpath, ip);
	}
	ln = fullpath + ip;
	(void) strcpy(filn_, ln);
#ifdef VMS
	upcase(filn_);		/* VMS knows uppercase filenames only */
#endif
}

/* Procedure to select the matching filenames * author : J.Jansen */
int
sel_image(struct dirent *name)
{
	extern char filenam_[256];
	char       *nam_ = name->d_name;
	char       *fil_ = filenam_;
	static int  numfrag = -1;
	static char *frags[64];
	int         ip, i;

	if (numfrag == -1) {
		++numfrag;
		while ((ip = index_(fil_, "*"))) {
			frags[numfrag] = (char *) malloc(ip);
			(void) strcpy(frags[numfrag], "\0");
			(void) strncat(frags[numfrag], fil_, ip - 1);
			++numfrag;
			fil_ = fil_ + ip;
		}
		frags[numfrag] = (char *) malloc(strlen(fil_));
		(void) strcpy(frags[numfrag], fil_);
	}
	for (i = 0; i <= numfrag; ++i) {
		ip = index_(nam_, frags[i]);
		if (ip == 0)
			return (0);
		nam_ = nam_ + ip;
	}

	return (1);
}

/* scandir implementiation for VMS * Author : J.Jansen */
/* name changed to scan_dir to solve portablity problems */
#define _MEMBL_ 64
int
scan_dir(const char *directoryname, struct dirent ***namelist,
 int         (*select) (void *), int (*compare) (const void *, const void *))
{
	DIR        *dir_;
	struct dirent *new_entry, **namelist_;
	int         size_, num_list_;

	if ((dir_ = opendir(directoryname)) == NULL)
		return (-1);
	size_ = _MEMBL_;
	namelist_ = (struct dirent **) malloc(size_ * sizeof (struct dirent *));

	if (namelist_ == NULL)
		return (-1);
	num_list_ = 0;
	while ((new_entry = readdir(dir_)) != NULL) {
#ifndef VMS
		if (!strcmp(new_entry->d_name, ".."))
			continue;
#endif

		if (select != NULL && !(*select) (new_entry))
			continue;
		if (++num_list_ >= size_) {
			size_ = size_ + _MEMBL_;
			namelist_ = (struct dirent **) realloc((void *) namelist_, size_ *
						   sizeof (struct dirent *));

			if (namelist_ == NULL)
				return (-1);
		}
		namelist_[num_list_ - 1] = (struct dirent *) malloc(sizeof (struct
								    dirent));

		(void) strcpy(namelist_[num_list_ - 1]->d_name, new_entry->d_name);
	}
	(void) closedir(dir_);
	if (num_list_ && compare != NULL)
		(void) qsort((void *) namelist_, num_list_,
			     sizeof (struct dirent *), compare);

	*namelist = namelist_;
	return (num_list_);
}

#endif
