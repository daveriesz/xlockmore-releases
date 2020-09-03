
#if !defined( lint ) && !defined( SABER )
static const char sccsid[] = "@(#)option.c	4.00 97/01/01 xlockmore";

#endif

/*-
 * option.c - option stuff for xmlock, the gui interface to xlock.
 *
 * Copyright (c) 1996 by Charles Vidal
 *
 * See xlock.c for copying information.
 *
 * Revision History:
 * Nov-96: written.
 *
 */

/* if you want add some options ,so it's not really difficult * 1: Learn Motif 




   ... but only if you want :) * 2: add a name of widget after
   geometry,font,program * 3: add a string ( char * ) after
   *inval=NULL,*geo=NULL * 4: add a callback like f_username with your new
   string * 5: copy and paste after 
   geometry=XtVaCreateManagedWidget("geometry",xmPushButtonGadgetClass,options,NULL);
   XtAddCallback(geometry,XmNactivateCallback,f_geometry,NULL); change geometry by *
   your name of widget  , and f_geometry by the name of your new callback * 6: go to
   the church , call god for a miracle :) * GOOD LUCK */

#include <X11/cursorfont.h>
/* #include <Xm/XmAll.h> Does not work on my version of Lesstif */
#include <Xm/RowColumn.h>
#include <Xm/SelectioB.h>
#include <Xm/CascadeB.h>
#include <Xm/PushBG.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>

#define OPTIONS 8
#define NBPROGNAME 3
#define MAXNAMES 10000
Widget      Menuoption;
static Widget Options[OPTIONS];

/* extern variables */
extern Widget toplevel;

/* Widget */
static Widget PromptDialog, FontSelectionDialog, ProgramSelectionDialog;

/* strings */
char       *c_Options[OPTIONS];
extern char *r_Options[OPTIONS];

static char *prompts[] =
{
	"Enter the user name.",
	"Enter the password message.",
	"Enter the info message.",
	"Enter the valid message.",
	"Enter the invalid message.",
	"Enter the geometry mxn."
};

static char *prognames[NBPROGNAME] =
{"fortune", "finger", "\"echo hello world\""};
static int  loadfont = 0;

/* string temp */
static char **whichone;


static void
managePrompt(char *s)
{
	int         ac;
	Arg         args[3];
	XmString    label_str1, label_str2;

	ac = 0;
	label_str1 = XmStringCreateSimple(s);
	XtSetArg(args[ac], XmNselectionLabelString, label_str1);
	ac++;
	if (*whichone != NULL) {
		label_str2 = XmStringCreateSimple(*whichone);
		XtSetArg(args[ac], XmNtextString, label_str2);
		ac++;
	} else {
		label_str2 = XmStringCreateSimple("");
		XtSetArg(args[ac], XmNtextString, label_str2);
		ac++;
	}

	XtSetValues(PromptDialog, args, ac);
	XtManageChild(PromptDialog);
	XmStringFree(label_str1);
	XmStringFree(label_str2);
}

/* CallBack */
static void
f_option(Widget w, XtPointer client_data, XtPointer call_data)
{
	switch ((int) client_data) {
		case 6:
			{
				int         numdirnames, i;
				char      **dirnames;
				XmString    label_str;
				Widget      listtmp;
				Cursor      tmp;

				whichone = &c_Options[6];
				if (!loadfont) {
					tmp = XCreateFontCursor(XtDisplay(toplevel), XC_watch);
					XDefineCursor(XtDisplay(toplevel), XtWindow(toplevel), tmp);
					dirnames = XListFonts(XtDisplay(toplevel), "*", MAXNAMES, &numdirnames);
					listtmp = XmSelectionBoxGetChild(FontSelectionDialog, XmDIALOG_LIST);
					for (i = 0; i < numdirnames; i++) {
						label_str = XmStringCreateSimple(dirnames[i]);
						XmListAddItemUnselected(listtmp, label_str, 0);
						XmStringFree(label_str);
					}
					tmp = XCreateFontCursor(XtDisplay(toplevel), XC_left_ptr);
					XDefineCursor(XtDisplay(toplevel), XtWindow(toplevel), tmp);
					loadfont = 1;
				}
				XtManageChild(FontSelectionDialog);
			}
			break;

		case 7:
			whichone = &c_Options[7];
			XtManageChild(ProgramSelectionDialog);
			break;

		default:
			whichone = &c_Options[(int) client_data];
			managePrompt(prompts[(int) client_data]);
	}
}

static void
f_Dialog(Widget w, XtPointer client_data, XtPointer call_data)
{
	static char *text = NULL;
	XmSelectionBoxCallbackStruct *scb =
	(XmSelectionBoxCallbackStruct *) call_data;

	if (whichone != NULL)
		XtFree(*whichone);
	XmStringGetLtoR(scb->value, XmSTRING_DEFAULT_CHARSET, &text);
	*whichone = text;
}

/* Setup Widget */
void
Setup_Option(Widget MenuBar)
{
	Arg         args[15];
	int         i, ac = 0;
	XmString    label_str;
	Widget      listtmp, pulldownmenu;

	pulldownmenu = XmCreatePulldownMenu(MenuBar, "PopupOptions", NULL, 0);
	label_str = XmStringCreateSimple("Options");
	XtVaCreateManagedWidget("Options", xmCascadeButtonWidgetClass, MenuBar,
		XmNlabelString, label_str, XmNsubMenuId, pulldownmenu, NULL);

	for (i = 0; i < OPTIONS; i++) {
		Options[i] = XtVaCreateManagedWidget(r_Options[i],
				xmPushButtonGadgetClass, pulldownmenu, NULL);
		XtAddCallback(Options[i], XmNactivateCallback, f_option, (XtPointer) i);
	}
	PromptDialog = XmCreatePromptDialog(toplevel, "PromptDialog", args, ac);
	XtAddCallback(PromptDialog, XmNokCallback, f_Dialog, NULL);

	FontSelectionDialog = XmCreateSelectionDialog(toplevel,
					     "FontSelectionDialog", NULL, 0);
	XtAddCallback(FontSelectionDialog, XmNokCallback, f_Dialog, NULL);

	ProgramSelectionDialog = XmCreateSelectionDialog(toplevel,
					  "ProgramSelectionDialog", NULL, 0);
	XtAddCallback(ProgramSelectionDialog, XmNokCallback, f_Dialog, NULL);
	listtmp = XmSelectionBoxGetChild(ProgramSelectionDialog, XmDIALOG_LIST);
	for (i = 0; i < NBPROGNAME; i++) {
		label_str = XmStringCreateSimple(prognames[i]);
		XmListAddItemUnselected(listtmp, label_str, 0);
		XmStringFree(label_str);
	}
}
