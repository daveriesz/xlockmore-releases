
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)utils.c	4.02 97/04/01 xlockmore";

#endif

/*-
 * utils.c - various utilities - usleep, seconds, SetRNG, LongRNG, hsbramp.
 *
 * Copyright (c) 1991 by Patrick J. Naughton.
 *
 * Revision History:
 *
 * Changes of David Bagley <bagleyd@bigfoot.com>
 *  4-Apr-96: Added procedures to handle wildcards on filenames
 *            J. Jansen <joukj@crys.chem.uva.nl>
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

#ifdef USE_OLD_EVENT_LOOP
#if !defined( VMS ) || defined( XVMSUTILS ) || ( __VMS_VER >= 70000000 )
#ifdef USE_XVMSUTILS
#include <X11/unix_time.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#endif
#else
#include <starlet.h>
#endif
#if defined( SYSV ) || defined( SVR4 )
#ifdef LESS_THAN_AIX3_2
#include <sys/poll.h>
#else /* !LESS_THAN_AIX3_2 */
#include <poll.h>
#endif /* !LESS_THAN_AIX3_2 */
#endif /* defined( SYSV ) || defined( SVR4 ) */

#ifndef HAVE_USLEEP
 /* usleep should be defined */
int
usleep(unsigned long usec)
{
#if (defined( SYSV ) || defined( SVR4 )) && !defined( __hpux )
#ifdef HAVE_NANOSLEEP
	{
		struct timespec rqt;

		rqt.tv_nsec = 1000 * (usec % (unsigned long) 1000000);
		rqt.tv_sec = usec / (unsigned long) 1000000;
		return nanosleep(&rqt, NULL);
	}
#else /* !HAVE_NANOSLEEP */
	(void) poll((void *) 0, (int) 0, usec / 1000);	/* ms resolution */
#endif /* !HAVE_NANOSLEEP */
#else /* !SYSV */
#if HAVE_GETTIMEOFDAY
	struct timeval time_out;

	time_out.tv_usec = usec % (unsigned long) 1000000;
	time_out.tv_sec = usec / (unsigned long) 1000000;
	(void) select(0, (void *) 0, (void *) 0, (void *) 0, &time_out);
#else
	long        timadr[2];

	if (usec != 0) {
		timadr[0] = -usec * 10;
		timadr[1] = -1;

		sys$setimr(4, &timadr, 0, 0, 0);
		sys$waitfr(4);
	}
#endif
#endif /* !SYSV */
	return 0;
}
#endif /* !HAVE_USLEEP */
#endif /* USE_OLD_EVENT_LOOP */

/* 
 * returns the number of seconds since 01-Jan-70.
 * This is used to control rate and timeout in many of the animations.
 */
unsigned long
seconds(void)
{
#if HAVE_GETTIMEOFDAY
	struct timeval now;

	(void) gettimeofday(&now, NULL);
	return (unsigned long) now.tv_sec;
#else
	return (unsigned long) time((time_t *) 0);
#endif
}

static int
readable(char *filename)
{
	FILE       *fp;

	if ((fp = fopen(filename, "rb")) == NULL)
		return False;
	(void) fclose(fp);
	return True;
}

#if (! HAVE_STRDUP )
char       *
strdup(char *str)
{
	register char *ptr;

	ptr = (char *) malloc(strlen(str) + 1);
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
isRibbon(void)
{
	return (getwordsfrom == FROM_RESRC);
}

char       *
getWords(int screen)
{
	FILE       *pp;
	static char buf[MAXSCREENS][BUFSIZ], progerr[BUFSIZ], progrun[BUFSIZ];
	register char *p = buf[screen];
	extern int  pclose(FILE *);
	int         i;

	buf[screen][0] = '\0';

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
			/* PURIFY 3.2 on Solaris2 reports duplication of file descriptor on next line.
			   PURIFY 4.0 on SunOS4 does not report this error */
			if ((pp = (FILE *) popen(progrun, "r")) != NULL) {
				while (fgets(p, sizeof (buf[screen]) - strlen(buf[screen]), pp)) {
					if (strlen(buf[screen]) + 1 < sizeof (buf[screen]))
						p = buf[screen] + strlen(buf[screen]);
					else
						break;
				}
				(void) pclose(pp);
				p = buf[screen];
				if (!buf[screen][0])
					(void) sprintf(buf[screen], "\"%s\" produced no output!",
					 (!program) ? DEF_PROGRAM : program);
				else {
					(void) memset((char *) progerr, 0, sizeof (progerr));
					(void) strcpy(progerr, "sh: ");
					strcat_firstword(progerr, (!program) ? DEF_PROGRAM : program);
					(void) strcat(progerr, ": not found\n");
					if (!strcmp(buf[screen], progerr))
						switch (NRAND(12)) {
							case 0:
								(void) strcat(buf[screen],
									      "( Get with the program, bub. )\n");
								break;
							case 1:
								(void) strcat(buf[screen],
									      "( I blow my nose at you, you silly person! )\n");
								break;
							case 2:
								(void) strcat(buf[screen],
									      "\nThe resource you want to\nset is `program'.\n");
								break;
							case 3:
								(void) strcat(buf[screen],
									      "\nHelp!!  Help!!\nAAAAAAGGGGHHH!!  \n\n");
								break;
							case 4:
								(void) strcat(buf[screen],
									      "( Hey, who called me `Big Nose'? )\n");
								break;
							case 5:
								(void) strcat(buf[screen],
									      "( Hello?  Are you paying attention? )\n");
								break;
							case 6:
								(void) strcat(buf[screen],
									      "sh: what kind of fool do you take me for? \n");
								break;
							case 7:
								(void) strcat(buf[screen],
									      "\nRun me with -program \"fortune -o\".\n");
								break;
							case 8:
								(void) strcat(buf[screen],
									      "( Where is your fortune? )\n");
								break;
							case 9:
								(void) strcat(buf[screen],
									      "( Your fortune has not been written yet!! )");
								break;
						}
				}
				p = buf[screen];
			} else {
				perror(progrun);
				p = def_words;
			}
			break;
#endif
		case FROM_FORMATTEDFILE:
			if ((pp = fopen(messagesfile, "r")) != NULL) {
				int         len_mess_file;

				if (fscanf(pp, "%d", &len_mess_file)) {
					int         no_quote;

					if (len_mess_file <= 0)
						buf[screen][0] = '\0';
					else {
						(void) fgets(p, sizeof (buf[screen]) - strlen(buf[screen]), pp);
						/* get first '%%' (the one after the number of quotes) */
						(void) fgets(p, sizeof (buf[screen]) - strlen(buf[screen]), pp);
						no_quote = NRAND(len_mess_file);
						for (i = 0; i <= no_quote; ++i) {
							unsigned int len_cur = 0;

							buf[screen][0] = '\0';
							p = buf[screen] + strlen(buf[screen]);
							while (fgets(p,
								     sizeof (buf[screen]) - strlen(buf[screen]), pp)) {
								if (strlen(buf[screen]) + 1 < sizeof (buf[screen])) {
									/* a line with '%%' contains 3 characters */
									if ((strlen(buf[screen]) == len_cur + 3) &&
									    (p[0] == '%') && (p[1] == '%')) {
										p[0] = '\0';	/* get rid of "%%" in buf[screen] */
										break;
									} else {
										p = buf[screen] + strlen(buf[screen]);
										len_cur = strlen(buf[screen]);
									}
								} else
									break;
							}
						}
					}
					(void) fclose(pp);
					if (!buf[screen][0])
						(void) sprintf(buf[screen],
							       "file \"%s\" is empty!", messagesfile);
					p = buf[screen];
				} else {
					(void) sprintf(buf[screen],
						       "file \"%s\" not in correct format!", messagesfile);
					p = buf[screen];
				}
			} else {
				(void) sprintf(buf[screen],
				"could not read file \"%s\"!", messagesfile);
				p = buf[screen];
			}
			break;
		case FROM_FILE:
			if ((pp = fopen(messagefile, "r")) != NULL) {
				while (fgets(p, sizeof (buf[screen]) - strlen(buf[screen]), pp)) {
					if (strlen(buf[screen]) + 1 < sizeof (buf[screen]))
						p = buf[screen] + strlen(buf[screen]);
					else
						break;
				}
				(void) fclose(pp);
				if (!buf[screen][0])
					(void) sprintf(buf[screen],
					"file \"%s\" is empty!", messagefile);
				p = buf[screen];
			} else {
				(void) sprintf(buf[screen],
				 "could not read file \"%s\"!", messagefile);
				p = buf[screen];
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
getFont(Display * display)
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

#if HAVE_DIRENT_H
static void
randomImage(char *im_file)
{
	extern char directory_r[DIRBUF];
	extern struct dirent **image_list;
	extern int  num_list;
	int         num;

	if (num_list > 0) {
		num = NRAND(num_list);
		(void) strcpy(im_file, directory_r);
		(void) strcat(im_file, image_list[num]->d_name);
	}
}

#endif

#if HAVE_DIRENT_H

extern Bool debug;

/* index_dir emulation of FORTRAN's index in C. Author: J. Jansen */
static int
index_dir(char *str1, char *substr)
{
	int         i, num, l1 = strlen(str1), ls = strlen(substr), found;
	char       *str1_tmp, *substr_tmp, *substr_last;

	num = l1 - ls + 1;
	substr_last = substr + ls;
	for (i = 0; i < num; ++i) {
		str1_tmp = str1 + i;
		substr_tmp = substr;
		found = 1;
		while (substr_tmp < substr_last)
			if (*str1_tmp++ != *substr_tmp++) {
				found = 0;
				break;
			}
		if (found)
			return (i + 1);
	}
	return (0);
}

#ifdef VMS
/* Upcase string  Author: J. Jansen */
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

/* Split full path into directory and filename parts  Author: J. Jansen */
void
get_dir(char *fullpath, char *dir, char *filename)
{
	char       *ln;
	int         ip_temp = 0, ip;

#ifdef VMS
	ip = index_dir(fullpath, "]");
#else
	ln = fullpath;
	ip = 0;
	while ((ip_temp = index_dir(ln, "/"))) {
		ip = ip + ip_temp;
		ln = fullpath + ip;
	}
#endif
	if (ip == 0) {
#ifdef VMS
		(void) strcpy(dir, "[]");
#else
		(void) strcpy(dir, "./");
#endif
	} else {
		if (ip > DIRBUF - 1)
			ip_temp = DIRBUF - 1;
		else
			ip_temp = ip;
		(void) strncpy(dir, fullpath, ip_temp);
		dir[ip_temp] = '\0';
	}
	ln = fullpath + ip;
	(void) strncpy(filename, ln, MAXNAMLEN - 1);
	filename[MAXNAMLEN - 1] = '\0';
#ifdef VMS
	upcase(filename);	/* VMS knows uppercase filenames only */
#endif
	if (debug)
		(void) printf("get_dir %s %s %s\n", fullpath, dir, filename);
}

/* Procedure to select the matching filenames  Author: J. Jansen */
int
sel_image(struct dirent *name)
{
	extern char filename_r[MAXNAMLEN];
	char       *name_tmp = name->d_name;
	char       *filename_tmp = filename_r;
	static int  numfrag = -1;
	static char *frags[64];
	int         ip, i;

	if (numfrag == -1) {
		++numfrag;
		while ((ip = index_dir(filename_tmp, "*"))) {
			frags[numfrag] = (char *) malloc(ip);
			(void) strcpy(frags[numfrag], "\0");
			(void) strncat(frags[numfrag], filename_tmp, ip - 1);
			++numfrag;
			filename_tmp = filename_tmp + ip;
		}
		frags[numfrag] = (char *) malloc(strlen(filename_tmp));
		(void) strcpy(frags[numfrag], filename_tmp);
	}
	for (i = 0; i <= numfrag; ++i) {
		ip = index_dir(name_tmp, frags[i]);
		if (ip == 0)
			return (0);
		name_tmp = name_tmp + ip;
	}

	return (1);
}

/* scandir implementiation for VMS  Author: J. Jansen */
/* name changed to scan_dir to solve portablity problems */
#define _MEMBL_ 64
int
scan_dir(const char *directoryname, struct dirent ***namelist,
	 int         (*select) (struct dirent *),
	 int         (*compare) (const void *, const void *))
{
	DIR        *dirp;
	struct dirent *new_entry, **namelist_tmp;
	int         size_tmp, num_list_tmp;

	if ((dirp = opendir(directoryname)) == NULL)
		return (-1);
	size_tmp = _MEMBL_;
	namelist_tmp = (struct dirent **) malloc(size_tmp * sizeof (struct dirent *));

	if (namelist_tmp == NULL)
		return (-1);
	num_list_tmp = 0;
	while ((new_entry = readdir(dirp)) != NULL) {
#ifndef VMS
		if (!strcmp(new_entry->d_name, ".") && !strcmp(new_entry->d_name, ".."))
			continue;
#endif
		if (select != NULL && !(*select) (new_entry))
			continue;
		if (++num_list_tmp >= size_tmp) {
			size_tmp = size_tmp + _MEMBL_;
			namelist_tmp = (struct dirent **) realloc(
									 (void *) namelist_tmp, size_tmp * sizeof (struct dirent *));

			if (namelist_tmp == NULL)
				return (-1);
		}
		/* Core Dumps here for Solaris2 if files names in dir are > 14 characters */
		namelist_tmp[num_list_tmp - 1] =
			(struct dirent *) malloc(sizeof (struct dirent));

		(void) strcpy(namelist_tmp[num_list_tmp - 1]->d_name, new_entry->d_name);

		*namelist = namelist_tmp;
#ifdef SOLARIS2
		if (debug)
			(void) printf("file %d: %s\n",
				      num_list_tmp - 1, namelist_tmp[num_list_tmp - 1]->d_name);
#endif
	}
#ifdef SOLARIS2			/* Data seems to get corrupted for Solaris2 */
	if (debug) {
		(void) printf("directory: %s\n", directoryname);
		for (size_tmp = 0; size_tmp < num_list_tmp; size_tmp++)
			(void) printf("file %d: %s\n", size_tmp, namelist_tmp[size_tmp]->d_name);
	}
#endif

	(void) closedir(dirp);
	if (num_list_tmp && compare != NULL)
		(void) qsort((void *) namelist_tmp, num_list_tmp,
			     sizeof (struct dirent *), compare);

	*namelist = namelist_tmp;
	return (num_list_tmp);
}

#endif

extern int  XbmReadFileToImage(char *filename,
			       int *width, int *height, unsigned char **bits);

#if defined( USE_XPM ) || defined( USE_XPMINC )
#if USE_XPMINC
#include <xpm.h>
#else
#include <X11/xpm.h>		/* Normal spot */
#endif
#endif
#include "ras.h"

static XImage blogo =
{
	0, 0,			/* width, height */
	0, XYBitmap, 0,		/* xoffset, format, data */
	LSBFirst, 8,		/* byte-order, bitmap-unit */
	LSBFirst, 8, 1		/* bitmap-bit-order, bitmap-pad, depth */
};

void
getImage(ModeInfo * mi, XImage ** logo,
	 int width, int height, unsigned char *bits,
#if defined( USE_XPM ) || defined( USE_XPMINC )
	 char **name,
#endif
	 int *graphics_format, Colormap * ncm,
	 unsigned long *blackpix, unsigned long *whitepix)
{
	extern char *imagefile;
	Display    *display = MI_DISPLAY(mi);
	Window      window = MI_WINDOW(mi);

#if defined( USE_XPM ) || defined( USE_XPMINC )
	XpmAttributes attrib;

#endif

	if (MI_WIN_IS_INSTALL(mi) && !MI_WIN_IS_INROOT(mi))
		*ncm = XCreateColormap(display, window, MI_VISUAL(mi), AllocNone);
	else
		*ncm = None;

#if defined( USE_XPM ) || defined( USE_XPMINC )
	attrib.visual = MI_VISUAL(mi);
	if (*ncm != None)
		attrib.colormap = *ncm;
	else
		attrib.colormap = MI_COLORMAP(mi);
	attrib.depth = MI_WIN_DEPTH(mi);
	attrib.valuemask = XpmVisual | XpmColormap | XpmDepth;
#endif

	*graphics_format = 0;

#if HAVE_DIRENT_H
	randomImage(imagefile);
#endif

	if (strlen(imagefile))
		if (readable(imagefile)) {
			if (MI_NPIXELS(mi) > 2) {
				if (RasterSuccess == RasterFileToImage(display, imagefile, logo
				/* , MI_FG_COLOR(mi), MI_BG_COLOR(mi) */ )) {
					*graphics_format = IS_RASTERFILE;
					if (MI_WIN_IS_INSTALL(mi) && !MI_WIN_IS_INROOT(mi))
						SetImageColors(display, *ncm);
					*blackpix = GetBlack();
					*whitepix = GetWhite();
				}
			}
#if defined( USE_XPM ) || defined( USE_XPMINC )
#ifndef USE_MONOXPM
			if (MI_NPIXELS(mi) > 2)
#endif
			{
				if (*graphics_format <= 0) {
					if (*ncm != None)
						reserveColors(mi, *ncm, blackpix, whitepix);
					if (XpmSuccess == XpmReadFileToImage(display, imagefile, logo,
						  (XImage **) NULL, &attrib))
						*graphics_format = IS_XPMFILE;
				}
			}
#endif
			if (*graphics_format <= 0)
				if (!blogo.data) {
					if (BitmapSuccess == XbmReadFileToImage(imagefile,
										&blogo.width, &blogo.height, (unsigned char **) &blogo.data)) {
						blogo.bytes_per_line = (blogo.width + 7) / 8;
						*graphics_format = IS_XBMFILE;
						*logo = &blogo;
					}
				} else {
					*graphics_format = IS_XBMDONE;
					*logo = &blogo;
				}
			if (*graphics_format <= 0)
				(void) fprintf(stderr,
					       "\"%s\" is in an unrecognized format\n", imagefile);
		} else
			(void) fprintf(stderr,
				  "could not read file \"%s\"\n", imagefile);
#if defined( USE_XPM ) || defined( USE_XPMINC )
	if (*graphics_format <= 0)
#ifndef USE_MONOXPM
		if (MI_NPIXELS(mi) > 2)
#endif
			if (XpmSuccess == XpmCreateImageFromData(display, name,
					    logo, (XImage **) NULL, &attrib))
				*graphics_format = IS_XPM;
#endif
	if (*graphics_format <= 0) {
		if (!blogo.data) {
			blogo.data = (char *) bits;
			blogo.width = width;
			blogo.height = height;
			blogo.bytes_per_line = (blogo.width + 7) / 8;
			*graphics_format = IS_XBM;
		} else
			*graphics_format = IS_XBMDONE;
		*logo = &blogo;
	}
	if (*ncm != None && *graphics_format != IS_RASTERFILE &&
	    *graphics_format != IS_XPMFILE && *graphics_format != IS_XPM) {
		XFreeColormap(display, *ncm);
		*ncm = None;
	}
}

void
destroyImage(XImage * logo, int graphics_format)
{
	switch (graphics_format) {
#if 0
		case IS_XBM:
			blogo.data = NULL;
			break;
#endif
		case IS_XBMFILE:
			if (blogo.data) {
				(void) free((void *) blogo.data);
				blogo.data = NULL;
			}
			break;
		case IS_XPM:
		case IS_XPMFILE:
		case IS_RASTERFILE:
			if (logo)
				XDestroyImage(logo);
			break;
	}
	logo = NULL;
}

/* This stops some flashing, could be more efficient */
void
XEraseImage(Display * display, Window window, GC gc,
	    int x, int y, int xlast, int ylast, int xsize, int ysize)
{
	if (ylast < y) {
		if (y < ylast + ysize)
			XFillRectangle(display, window, gc, xlast, ylast, xsize, y - ylast);
		else
			XFillRectangle(display, window, gc, xlast, ylast, xsize, ysize);
	} else if (ylast > y) {
		if (y > ylast - ysize)
			XFillRectangle(display, window, gc, xlast, y + ysize, xsize, ylast - y);
		else
			XFillRectangle(display, window, gc, xlast, ylast, xsize, ysize);
	}
	if (xlast < x) {
		if (x < xlast + xsize)
			XFillRectangle(display, window, gc, xlast, ylast, x - xlast, ysize);
		else
			XFillRectangle(display, window, gc, xlast, ylast, xsize, ysize);
	} else if (xlast > x) {
		if (x > xlast - xsize)
			XFillRectangle(display, window, gc, x + xsize, ylast, xlast - x, ysize);
		else
			XFillRectangle(display, window, gc, xlast, ylast, xsize, ysize);
	}
}

static struct visual_class_name {
	int         class;
	char       *name;
} VisualClassName[] = {

	{
		StaticGray, "StaticGray"
	},
	{
		GrayScale, "GrayScale"
	},
	{
		StaticColor, "StaticColor"
	},
	{
		PseudoColor, "PseudoColor"
	},
	{
		TrueColor, "TrueColor"
	},
	{
		DirectColor, "DirectColor"
	},
	{
		-1, NULL
	},
};

int
visualClassFromName(char *name)
{
	int         a;
	char       *s1, *s2;
	int         class = -1;

	for (a = 0; VisualClassName[a].name; a++) {
		for (s1 = VisualClassName[a].name, s2 = name; *s1 && *s2; s1++, s2++)
			if ((isupper(*s1) ? tolower(*s1) : *s1) !=
			    (isupper(*s2) ? tolower(*s2) : *s2))
				break;

		if ((*s1 == '\0') || (*s2 == '\0')) {

			if (class != -1) {
				(void) fprintf(stderr,
					       "%s does not uniquely describe a visual class (ignored)\n", name);
				return (-1);
			}
			class = VisualClassName[a].class;
		}
	}
	if (class == -1)
		(void) fprintf(stderr, "%s is not a visual class (ignored)\n", name);
	return (class);
}

char
           *
nameOfVisualClass(int class)
{
	int         a;

	for (a = 0; VisualClassName[a].name; a++)
		if (VisualClassName[a].class == class)
			return (VisualClassName[a].name);
	return ("[Unknown Visual Class]");
}

#if defined(__cplusplus) || defined(c_plusplus)
#define DEFAULT_VIS(v)    v.c_class = DefaultVisual(display, screen)->c_class;
#define WANTED_VIS(v)   v.c_class = VisualClassWanted;
#define CHECK_WANTED_VIS(v)      ((v)->c_class == VisualClassWanted)
#define NAME_VIS(v) nameOfVisualClass(v->c_class)
#else
#define DEFAULT_VIS(v)    v.class = DefaultVisual(display, screen)->class;
#define WANTED_VIS(v)   v.class = VisualClassWanted;
#define CHECK_WANTED_VIS(v)      ((v)->class == VisualClassWanted)
#define NAME_VIS(v) nameOfVisualClass(v->class)
#endif

void
showVisualInfo(XVisualInfo * Vis)
{
	(void) fprintf(stderr, "Visual info: ");
	(void) fprintf(stderr, "screen %d, ", Vis->screen);
	(void) fprintf(stderr, "class %s, ", NAME_VIS(Vis));
	(void) fprintf(stderr, "depth %d\n", Vis->depth);
}

/*-
 * default_visual_info
 *
 * Gets a XVisualInfo structure that refers to a given visual or the default
 * visual.
 *
 * -      mi is the ModeInfo
 * -      visual is the visual to look up NULL => look up the default visual
 * -      default_info is set to point to the member of the returned list
 *          that corresponds to the default visual.
 *
 * Returns a list of XVisualInfo structures or NULL on failure. Free the list
 *   with XFree.
 */
static XVisualInfo *
defaultVisualInfo(ModeInfo * mi, Visual * visual, XVisualInfo ** default_info)
{
	Display    *display = MI_DISPLAY(mi);
	int         screen = MI_SCREEN(mi);
	XVisualInfo *info_list, vTemplate;
	int         i, n;
	extern int  VisualClassWanted;

	/* get a complete list of visuals */
	/*info_list = XGetVisualInfo(display, VisualNoMask, NULL, &n); */
	vTemplate.screen = screen;
	vTemplate.depth = MI_WIN_DEPTH(mi);
	if (VisualClassWanted == -1) {
		DEFAULT_VIS(vTemplate);
	} else {
		WANTED_VIS(vTemplate);
	}
	info_list = XGetVisualInfo(display,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
				   &vTemplate, &n);
	if (VisualClassWanted != -1 && n == 0) {
		/* Wanted visual not found so use default */
		DEFAULT_VIS(vTemplate);
		info_list = XGetVisualInfo(display,
			VisualScreenMask | VisualDepthMask | VisualClassMask,
					   &vTemplate, &n);
	}
/*-
  WANTED_VIS(vTemplate);
   if (VisualClassWanted == -1)
    info_list = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
      &vTemplate, &n);
   else
     info_list = XGetVisUalInfo(display,
       VisualScreenMask | VisualDepthMask | VisualClassMask,
       &vTemplateWant, &n);
 */
	if ((info_list == NULL) || (n == 0)) {
		if (MI_WIN_IS_VERBOSE(mi))
			(void) fprintf(stderr, "Could not get any Visuals, numvisuals = %d.\n",
				       n);
		return (NULL);
	}
	/* do we need to get the default visual? */
	if (visual == NULL)
		visual = DefaultVisual(display, screen);

	*default_info = info_list;
	if (VisualClassWanted == -1) {
		/* search through the list for the default visual */
		for (i = 0; i < n; i++, (*default_info)++)
			if ((*default_info)->visual == visual &&
			    (*default_info)->screen == screen)
				break;
	} else {
		/* search through the list for the requested visual
		 * with the greatest color depth */
		for (i = 0; i < n; i++, (*default_info)++)
			if ((*default_info)->depth >= 24 &&
			    (*default_info)->screen == screen &&
			    CHECK_WANTED_VIS(*default_info))
				break;
		if (i == n) {
			*default_info = info_list;
			for (i = 0; i < n; i++, (*default_info)++)
				if ((*default_info)->depth == 16 &&
				    (*default_info)->screen == screen &&
				    CHECK_WANTED_VIS(*default_info))
					break;
		}
		if (i == n) {
			*default_info = info_list;
			for (i = 0; i < n; i++, (*default_info)++)
				if ((*default_info)->depth == 8 &&
				    (*default_info)->screen == screen &&
				    CHECK_WANTED_VIS(*default_info))
					break;
		}
		if (i == n) {
			*default_info = info_list;
			for (i = 0; i < n; i++, (*default_info)++)
				if ((*default_info)->depth == 4 &&
				    (*default_info)->screen == screen &&
				    CHECK_WANTED_VIS(*default_info))
					break;
		}
		if (i == n) {
			*default_info = info_list;
			for (i = 0; i < n; i++, (*default_info)++)
				if ((*default_info)->screen == screen &&
				    CHECK_WANTED_VIS(*default_info))
					break;
		}
	}
	if (MI_WIN_IS_VERBOSE(mi)) {
		(void) fprintf(stderr, "Visual info: ");
		(void) fprintf(stderr, "screen %d, ", (*default_info)->screen);
		(void) fprintf(stderr, "class %s, ", NAME_VIS((*default_info)));
		(void) fprintf(stderr, "depth %d\n", (*default_info)->depth);
	}
	/* return the list (and the default info) */
	return (info_list);
}

/*-
 * setupColormap
 *
 * Create a read/write colourmap to use
 *
 */

Bool
setupColormap(ModeInfo * mi, int *colors, Bool * truecolor,
   unsigned long *redmask, unsigned long *bluemask, unsigned long *greenmask)
{
	XVisualInfo *list, *default_info;

	/* get information about the default visual */
	list = defaultVisualInfo(mi, MI_VISUAL(mi), &default_info);

	/* how many colours are there altogether? */
	*colors = MI_NPIXELS(mi);
	if (*colors > default_info->colormap_size) {
		*colors = default_info->colormap_size;
	}
	if (*colors < 2)
		*colors = 2;

#if defined(__cplusplus) || defined(c_plusplus)
	*truecolor = (default_info->c_class == TrueColor);
#else
	*truecolor = (default_info->class == TrueColor);
#endif

	*redmask = default_info->red_mask;
	*greenmask = default_info->green_mask;
	*bluemask = default_info->blue_mask;

#ifdef FORCEFIXEDCOLORS
	/* pretending a fixed colourmap */
	XFree((char *list);
	      return False;
#else
	{
		Bool        fixed;

		fixed = (
#if defined(__cplusplus) || defined(c_plusplus)
				(default_info->c_class != StaticGray) &&
				(default_info->c_class != StaticColor) &&
				(default_info->c_class != TrueColor) &&
#else
				(default_info->class != StaticGray) &&
				(default_info->class != StaticColor) &&
				(default_info->class != TrueColor) &&
#endif
				(MI_NPIXELS(mi) > 2) && !MI_WIN_IS_INROOT(mi) && MI_WIN_IS_INSTALL(mi));
		XFree((char *) list);
		return fixed;
	}
#endif
}

/*-
 * useableColors
 */
int
preserveColors(unsigned long fg, unsigned long bg,
	       unsigned long white, unsigned long black)
{
	/* how many colours should we preserve (out of white, black, fg, bg)? */
	if (((bg == black) || (bg == white)) && ((fg == black) || (fg == white)))
		return 2;
	else if ((bg == black) || (fg == black) ||
		 (bg == white) || (fg == white) || (bg == fg))
		return 3;
	else
		return 4;
}

#ifdef USE_GL

#include <GL/gl.h>
#include <GL/glx.h>

static XVisualInfo *glVis[MAXSCREENS];

int
getVisual(ModeInfo * mi, XVisualInfo * wantVis, int mono)
{
	Display    *display = MI_DISPLAY(mi);
	int         screen;
	static int  first;

	if (first) {
		for (screen = 0; screen < MAXSCREENS; screen++)
			glVis[screen] = NULL;
	}
	screen = MI_SCREEN(mi);

	if (!glVis[screen]) {
		if (mono) {
			/* Monochrome display - use color index mode */
			int         attribList[] =
			{GLX_DOUBLEBUFFER, None};

			glVis[screen] = glXChooseVisual(display, screen, attribList);
		} else {
			int         attribList[] =
			{GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 1, None};

			glVis[screen] = glXChooseVisual(display, screen, attribList);
		}
	}
	/* check if GL can render into root window. */
	if ((!glVis[screen]) || (glVis[screen]->visual != wantVis->visual)) {
		return (0);
	}
	return (1);		/* success */
}
#endif
