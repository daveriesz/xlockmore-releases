#if 0

static const char sccsid[] = "@(#)xmlock.c	4.08 98/02/18 xlockmore";

#endif

/*-
 * xmlock.c - main file for xmlock, the gui interface to xlock.
 *
 * Copyright (c) 1996 by Charles Vidal
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Jun-03: Code added from Xlockup by Thad Phetteplace
 *     tdphette@dexter.glaci.com
 *   Sun -- works (both timeouts seem connected to the same device)
 *   Linux -- mouse timeout works, keyboard timeout does not work
 *   Cygwin -- mouse timeout does not work, keyboard timeout does not work
 *   Alpha -- "does something"  ;)
 *   Since keyboard timeout does not seem to help, it was removed.
 * Nov-96: Continual minor improvements by Charles Vidal and David Bagley.
 * Oct-96: written.
 */

/*-
  XmLock Problems
  1. Allowing only one in -inroot.  Need a way to kill it.
  2. XLock resources need to be read and used to set initial values.
  3. Integer and floating point and string input.
 */

/*
 * 01/2001
 * I'm confused because the source is old and really not well programming
 * I put boolean option and option in the menubar
 * I have had the option.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef __FreeBSD__
#include <signal.h>
#endif
#include <sys/signal.h>
#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define HAVE_UNISTD_H 1
#endif /* HAVE_CONFIG_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef VMS
#include "vms_x_fix.h"
#include <descrip.h>
#include <lib$routines.h>
#endif

#ifdef USE_MB
#include <X11/Xlocale.h>
#endif

#ifdef HAVE_MOTIF
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#elif defined(HAVE_XAW3D)
#include <X11/StringDefs.h>
#include <X11/Xaw3d/Form.h>
#include <X11/Xaw3d/Label.h>
#include <X11/Xaw3d/Toggle.h>
#include <X11/Xaw3d/Box.h>
#include <X11/Xaw3d/Scrollbar.h>
#include <X11/Xaw3d/SimpleMenu.h>
#include <X11/Xaw3d/MenuButton.h>
#include <X11/Xaw3d/SmeBSB.h>
#include <X11/Xaw3d/Command.h>
#include <X11/Xaw3d/Dialog.h>
#define HAVE_ATHENA 1
#elif defined(HAVE_ATHENA)
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw3d/Command.h>
#include <X11/Xaw/Dialog.h>
/*#include <X11/Xaw/AsciiText.h>*/
#endif

#if USE_XMU
#include <X11/Xmu/Editres.h>
#endif

#include "option.h"

#ifdef SOLARIS2
extern int  kill(pid_t, int);
extern pid_t  wait(int *);
#endif

#include "bitmaps/m-xlock.xbm"	/* icon bitmap */

#define SKIPDELAY 60000 /* 1 minute */
#define MAXMIN 120

/* like an enum */
enum ACTION {
  LAUNCH=0,
  ROOT,
  WINDOW
	};

/* extern variable */
extern Widget Menuoption;
extern Widget Toggles[TOGGLES];
extern OptionStruct Opt[];

extern char *r_Toggles[];

static pid_t numberprocess = -1;	/* PID of xlock */

/*************************
 * To know the number of element
 * in the Opt Array
**************************/
extern int getNumberofElementofOpt(void);

/* Widget */
Widget      topLevel;
static Widget Labelxlock;
static Widget      timeoutSlider;

static time_t timenow;

/* If this worked on all systems it would be 30 min or 1800 */
static long timeout = 0;

static Widget ScrolledListModes, pushButtons[PUSHBUTTONS];

 /*Resource string */
#include "modes.h"

#if 0
static XmStringCharSet char_set = (XmStringCharSet) XmSTRING_DEFAULT_CHARSET;
#endif

/* some resources of buttons and toggles not really good programming :( */

static char *labelPushButtons[PUSHBUTTONS] =
{
	(char *) "Launch",
	(char *) "In Root",
	(char *) "In Window",
};

static int  numberinlist = 0;

static void checkTime(void);
/* CallBack */

void exitcallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	if (numberprocess != -1) {
		(void) kill(numberprocess, SIGKILL);
		/*(void) wait(&n);*/
	}
	exit(0);
}

static void
callLocker(int choice)
{
	int         i;
	char        command[500];

#ifdef VMS
	int         mask = 17;
	struct dsc$descriptor_d vms_image;

#endif

	(void) sprintf(command, "%s ", XLOCK);

	/* booleans (+/-) options */

#ifdef HAVE_MOTIF
	for (i = 0; i < TOGGLES; i++) {
		if (XmToggleButtonGetState(Toggles[i])) {
			(void) strcat(command, "-");
			(void) strcat(command, r_Toggles[i]);
			(void) strcat(command, " ");
		}
	}
	/*
		sizeof(Opt)/sizeof(OptionStruct) is not good to
		know the number of element in Opt
		so I have made a function getNumberofElementofOpt
	*/
	for (i = 0; i < getNumberofElementofOpt(); i++)
		if (Opt[i].userdata != NULL) {
			(void) strcat(command, "-");
			(void) strcat(command, Opt[i].cmd);
			(void) strcat(command, " ");
			(void) strcat(command, Opt[i].userdata);
			(void) strcat(command, " ");
		}
#elif defined(HAVE_ATHENA)
#endif

	switch (choice) {
		case LAUNCH:
			/* the default value then nothing to do */
			break;
		case WINDOW:
			(void) strcat(command, "-inwindow ");
			break;
		case ROOT:
			(void) strcat(command, "-inroot ");
			break;
	}
	(void) strcat(command, "-mode ");
	(void) strcat(command, LockProcs[numberinlist].cmdline_arg);
#ifdef VMS
	vms_image.dsc$w_length = strlen(command);
	vms_image.dsc$a_pointer = command;
	vms_image.dsc$b_class = DSC$K_CLASS_S;
	vms_image.dsc$b_dtype = DSC$K_DTYPE_T;
	(void) printf("%s\n", command);
	(void) lib$spawn(&vms_image, 0, 0, &mask);
#else
	if (choice != LAUNCH)
		(void) strcat(command, " & ");
	(void) printf("%s\n", command);
	(void) system(command);
#endif
	if (choice == LAUNCH)
		(void) XtAppAddTimeOut(XtWidgetToApplicationContext(topLevel),
			SKIPDELAY, (XtTimerCallbackProc) checkTime, topLevel);
}

/*----------------------------------------------------------*/
/* Code taken from Xlockup by Thad Phetteplace              */
/*    tdphette@dexter.glaci.com used by permission          */
/* CHECKTIME: This routine is called periodically by the     */
/*    openwin event handler.  It checks the last access     */
/*    time/date stamp on the mouse and keyboard devices     */
/*    and compares it to the timeout value.                 */
/*----------------------------------------------------------*/

static void checkTime(void)
{
  struct stat statmouse;
#if 0
  struct stat statkeybd;
#endif

  (void) time( &timenow );                   /* get current time */
  /* next does not work on Cygwin */
  (void) stat("/dev/mouse", &statmouse );   /* get mouse status */
#if 0
  /* does not seem to help on Solaris and hinders on Linux */
  (void) stat("/dev/kbd", &statkeybd );     /* get keyboard status */
#endif

  /* check if last access time was larger than timeout */
  if (timeout > 0 && (timenow-statmouse.st_atime >= timeout) /* &&
      (timenow-statkeybd.st_atime >= timeout)*/) {
     callLocker(LAUNCH);
  }
#ifdef DEBUG
  printf("timeout %ld,  timenow - amouse %ld\n",
    timeout, timenow - statmouse.st_atime);
  printf("timeout %ld,  timenow - akeybd %ld\n",
    timeout, timenow - statkeybd.st_atime);
#endif
  (void) XtAppAddTimeOut(XtWidgetToApplicationContext(topLevel),
	SKIPDELAY, (XtTimerCallbackProc) checkTime, topLevel);
}

static void
listenerPushButtons(Widget w, XtPointer client_data, XtPointer call_data)
{
	callLocker((long) client_data);
}

static void
f_ScrolledListModes(Widget w, XtPointer client_data, XtPointer call_data)
{
	char        numberwidget[50];
	char        str[50];
	int         n;

#ifdef HAVE_MOTIF
	numberinlist = ((XmListCallbackStruct *) call_data)->item_position - 1;
#elif defined(HAVE_ATHENA)
	numberinlist = ((size_t) call_data);
printf("%d\n",numberinlist);
numberinlist=2;
#endif
	(void) sprintf(numberwidget, "%ld", XtWindow(Labelxlock));
	(void) sprintf(str, "%s", LockProcs[numberinlist].cmdline_arg);
	if (numberprocess != -1) {
		(void) kill(numberprocess, SIGKILL);
		(void) wait(&n);
	}
#ifdef VMS
#define FORK vfork
#else
#define FORK fork
#endif
	if ((numberprocess = FORK()) == -1)
		(void) fprintf(stderr, "Fork error\n");
	else if (numberprocess == 0) {
		(void) execlp(XLOCK, XLOCK, "-parent", numberwidget,
			"-mode", str, "-geometry", WINDOW_GEOMETRY,
			"-delay", "100000",
			"-nolock", "-inwindow", "+install", NULL);
	}
}

#ifdef HAVE_MOTIF
static void
TimeoutSlider(Widget w, XtPointer clientData, XmScaleCallbackStruct * cbs)
{
	timeout = cbs->value * 60;
}
#endif
/* Setup Form */

/**********************/
static void
setupForm(Widget father)
{
	Arg         args[15];
	int         i, ac;
	Widget      pushButtonRow, exitwidgetB;
	char        string[160];
	XtPointer   iptr = 0;
#ifdef HAVE_MOTIF
	XmString    label_str;
	XmString    TabXmStr[numprocs];

/* buttons in the bottom */
	ac = 0;
	XtSetArg(args[ac], XmNorientation, XmHORIZONTAL);
	ac++;
	XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM);
	ac++;

	Menuoption = XmCreateMenuBar(father, (char *) "MenuBar", args,ac);
	XtManageChild(Menuoption);

	ac = 0;
	XtSetArg(args[ac], XmNwidth, WINDOW_WIDTH);
	ac++;
	XtSetArg(args[ac], XmNheight, WINDOW_HEIGHT);
	ac++;

	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
	XtSetArg(args[ac], XmNtopWidget, Menuoption); ac++;

#if 0
	int ModeLabel;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM); ac++;

	XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;

	XtSetArg(args[ac], XmNleftWidget, ModeLabel); ac++;
#endif

	Labelxlock = XmCreateLabel(father, (char *) "Window", args, ac);
	XtManageChild(Labelxlock);

	/* buttons in the bottom */
	 ac = 0;

	XtSetArg(args[ac], XmNorientation, XmHORIZONTAL);
	ac++;
	XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_FORM);
	ac++;
	pushButtonRow = XmCreateRowColumn(father, (char *) "pushButtonRow", args, ac);

	for (i = 0; i < PUSHBUTTONS; i++) {
		ac = 0;
#ifndef USE_MB
		label_str = XmStringCreate(labelPushButtons[i], XmSTRING_DEFAULT_CHARSET);
		XtSetArg(args[ac], XmNlabelString, label_str);
		ac++;
#endif
		PushButtons[i] = XmCreatePushButton(pushButtonRow,
			 labelPushButtons[i], args, ac);
#ifndef USE_MB
		XmStringFree(label_str);
#endif
		XtAddCallback(pushButtons[i],
			XmNactivateCallback, listenerPushButtons, iptr++);
		XtManageChild(pushButtons[i]);
	}
/*
 *  For the exit button
 */
	ac = 0;
#ifndef USE_MB
	label_str = XmStringCreate((char *) "Exit",
		XmSTRING_DEFAULT_CHARSET);
	XtSetArg(args[ac], XmNlabelString, label_str);
	ac++;
#endif
	exitwidgetB = XmCreatePushButton(pushButtonRow,
		(char *) "Exit", args, ac);
#ifndef USE_MB
	XmStringFree(label_str);
#endif
	XtAddCallback(exitwidgetB, XmNactivateCallback, exitcallback,
		(XtPointer) NULL);
	XtManageChild(exitwidgetB);
	timeoutSlider = XtVaCreateManagedWidget("timeout",
		xmScaleWidgetClass, pushButtonRow,
		XtVaTypedArg, XmNtitleString, XmRString, "Timeout", 8,
		XmNminimum, 0,
		XmNmaximum, MAXMIN,
		XmNvalue, timeout / 60,
		XmNshowValue, True,
		XmNorientation, XmHORIZONTAL, NULL);
	XtAddCallback(timeoutSlider, XmNvalueChangedCallback,
		(XtCallbackProc) TimeoutSlider, (XtPointer) NULL);

	XtManageChild(pushButtonRow);

/* list and toggles in row like that (row(list)(TogglesRow(toggles...))) */
	ac = 0;
	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET);
	ac++;
	XtSetArg(args[ac], XmNtopWidget, Labelxlock);
	ac++;
	XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_FORM);
	ac++;
	XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET);
	ac++;
	XtSetArg(args[ac], XmNbottomWidget, pushButtonRow);
	ac++;
	XtSetArg(args[ac], XmNitems, TabXmStr);
	ac++;
	XtSetArg(args[ac], XmNitemCount, numprocs);
	ac++;
	XtSetArg(args[ac], XmNvisibleItemCount, 10);
	ac++;


	for (i = 0; i < (int) numprocs; i++) {
		(void) sprintf(string, "%-14s%s", LockProcs[i].cmdline_arg,
			       LockProcs[i].desc);
		TabXmStr[i] = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET);
	}
		ScrolledListModes = XmCreateScrolledList(father, (char *) "ScrolledListModes",
						 args, ac);
	XtAddCallback(ScrolledListModes, XmNbrowseSelectionCallback,
		      f_ScrolledListModes, NULL);
	XtManageChild(ScrolledListModes);

#if 0
	int TogglesRow, Row;
	TogglesRow = XmCreateRowColumn(Row, (char *) "TogglesRow", NULL, 0);
	for (i = 0; i < TOGGLES; i++) {
#ifndef USE_MB
		ac = 0;
		label_str = XmStringCreate(r_Toggles[i], XmSTRING_DEFAULT_CHARSET);
		XtSetArg(args[ac], XmNlabelString, label_str);
		ac++;
#endif
		Toggles[i] = XmCreateToggleButton(TogglesRow, r_Toggles[i], args, ac);
#ifndef USE_MB
		XmStringFree(label_str);
#endif
		XtManageChild(Toggles[i]);
	}
	XtManageChild(TogglesRow);

	XtManageChild(Row);
#endif
	for (i = 0; i < (int) numprocs; i++) {
		XmStringFree(TabXmStr[i]);
	}
#elif defined(HAVE_ATHENA)
	pushButtonRow = XtVaCreateManagedWidget("pushButtonRow",
                formWidgetClass, father,
		XtNborderWidth, 0, NULL);
	for (i = 0; i < PUSHBUTTONS; i++) {
		pushButtons[i] = XtVaCreateManagedWidget(labelPushButtons[i],
               		toggleWidgetClass, pushButtonRow,
			NULL);
		if (i > 0)
			XtVaSetValues(pushButtons[i],
				XtNfromHoriz, pushButtons[i - 1], NULL);
printf("%s\n", labelPushButtons[i]);
		XtAddCallback(pushButtons[i],
			XtNcallback, listenerPushButtons, iptr++);
	}
#endif
}

#ifdef MOTIF
extern void setup_Option(Widget MenuBar);
#elif defined(HAVE_ATHENA)
#endif

int
main(int argc, char **argv)
{
	Widget      form;
	Arg         args[15];

#ifdef USE_MB
	setlocale(LC_ALL, "");
	XtSetLanguageProc(NULL, NULL, NULL);
#endif

/* PURIFY 4.0.1 on Solaris 2 reports an uninitialized memory read on the next
   line. */
	topLevel = XtInitialize(argv[0], "XmLock", (XrmOptionDescRec*) NULL, 0,
		&argc, argv);
	XtSetArg(args[0], XtNiconPixmap,
		 XCreateBitmapFromData(XtDisplay(topLevel),
			RootWindowOfScreen(XtScreen(topLevel)),
			(char *) image_bits, image_width, image_height));
	XtSetValues(topLevel, args, 1);
	/* creation Widget */
/* PURIFY 4.0.1 on Solaris 2 reports an uninitialized memory read on the next
   line. */
#ifdef HAVE_MOTIF
	form = XmCreateForm(topLevel, (char *) "Form", (Arg *) NULL, 0);
#elif defined(HAVE_ATHENA)
	form = XtVaCreateManagedWidget("Form",
                formWidgetClass, topLevel,
                XtNborderWidth, 0, NULL);
#endif
	setupForm(form);
#ifdef HAVE_MOTIF
	setup_Option(Menuoption);
#elif defined(HAVE_ATHENA)
#endif
	XtManageChild(form);
	XtRealizeWidget(topLevel);
	(void) XtAppAddTimeOut(XtWidgetToApplicationContext(topLevel),
		SKIPDELAY, (XtTimerCallbackProc) checkTime, topLevel);
#if USE_XMU
	/* With this handler you can use editres */
	XtAddEventHandler(topLevel, (EventMask) 0, TRUE,
		(XtEventHandler) _XEditResCheckMessages, NULL);
#endif
	XtMainLoop();
#ifdef VMS
	return 1;
#else
	return 0;
#endif
}
