/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 * 
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 * 
 * The Original Code is the Xfe Widgets.
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

/*----------------------------------------------------------------------*/
/*																		*/
/* Name:		DashBoardTest.c											*/
/* Description:	Test for XfeDashBoard widget.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static Widget	create_frame_shell			(Widget,String);

int
main(int argc,char *argv[])
{
    Widget	frame1;
    Widget	frame2;
    Widget	frame3;
    Widget	frame4;
    
	XfeAppCreate("DashBoardTest",&argc,argv);
    
	frame1 = create_frame_shell(XfeAppShell(),"Frame1");

	frame2 = create_frame_shell(XfeAppShell(),"Frame2");

	frame3 = create_frame_shell(XfeAppShell(),"Frame3");

	frame4 = create_frame_shell(XfeAppShell(),"Frame4");

	XtPopup(frame1,XtGrabNone);

	XtPopup(frame2,XtGrabNone);

	XtPopup(frame3,XtGrabNone);

	XtPopup(frame4,XtGrabNone);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static Widget
create_frame_shell(Widget parent,String name)
{
    Widget	frame;
	Widget	form;
    Widget	docked_task_bar;
    Widget	dash_board;

	static Widget	floating_shell = NULL;
	static Boolean	first_instance = True;

    frame = XtVaCreatePopupShell(name,
								 xfeFrameShellWidgetClass,
								 parent,
								 NULL);

	XfeAddEditresSupport(frame);

    form = XtVaCreateManagedWidget("Form",
								   xmFormWidgetClass,
								   frame,
								   NULL);

	if (first_instance)
	{
		int			mask;
		int			func;
		
		
		Widget floating_task_bar;
		
		first_instance = False;
		
		mask = MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU;
		func = MWM_FUNC_CLOSE | MWM_FUNC_MOVE;

		floating_shell =
			XtVaCreatePopupShell("FloatingShell",
								 xmDialogShellWidgetClass,
								 XfeAncestorFindApplicationShell(parent),
								 XmNvisual,				XfeVisual(parent),
								 XmNcolormap,			XfeColormap(parent),
								 XmNdepth,				XfeDepth(parent),
								 XmNmwmDecorations,		mask,
								 XmNmwmFunctions,		func,
								 XmNallowShellResize,	True,
								 XmNdeleteResponse,		XmDO_NOTHING,
								 NULL);

		
		floating_task_bar = XfeCreateLoadedTaskBar(floating_shell,
												   "FloatingTaskBar",
												   True,
												   "Task_",
												   NULL,
												   4,
												   NULL);
	}

    dash_board = XtVaCreateManagedWidget("DashBoard",
										 xfeDashBoardWidgetClass,
										 form,
										 NULL);

	XtAddCallback(dash_board,XmNdockCallback,XfeDockCallback,NULL);
	XtAddCallback(dash_board,XmNundockCallback,XfeUndockCallback,NULL);

    XtVaSetValues(dash_board,
				  XmNfloatingShell,	floating_shell,
				  NULL);
	
	/* Create the progress bar */
	XtVaCreateManagedWidget("ProgressBar",
							xfeProgressBarWidgetClass,
							dash_board,
							XmNusePreferredHeight,		True,
							XmNusePreferredWidth,		True,
							XmNshadowThickness,			1,
							XmNshadowType,				XmSHADOW_IN,
							XmNhighlightThickness,		0,
							XmNtraversalOn,				False,
							NULL);

    /* Create the status bar */
	XtVaCreateWidget("StatusBar",
					 xfeLabelWidgetClass,
					 dash_board,
					 XmNusePreferredHeight,		False,
					 XmNusePreferredWidth,		True,
					 XmNshadowThickness,		2,
					 XmNshadowType,				XmSHADOW_IN,
					 XmNhighlightThickness,		0,
					 XmNtraversalOn,			False,
					 NULL);
	
    /* Create the task bar */
	docked_task_bar = XfeCreateLoadedTaskBar(dash_board,
											 "DockedTaskBar",
											 False,
											 "T",
											 NULL,
											 4,
											 NULL);

	XtVaSetValues(docked_task_bar,
				  XmNorientation,			XmHORIZONTAL,
				  XmNusePreferredWidth,		True,
				  XmNusePreferredHeight,	True,
				  XmNhighlightThickness,	0,
				  XmNshadowThickness,		0,
				  XmNtraversalOn,			False,
				  NULL);
	
	return frame;
}
/*----------------------------------------------------------------------*/
