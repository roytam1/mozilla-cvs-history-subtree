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
/* Name:		<Xfe/DashBoard.h>										*/
/* Description:	XfeDashBoard widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeDashBoard_h_						/* start DashBoard.h	*/
#define _XfeDashBoard_h_

#include <Xfe/Xfe.h>
#include <Xfe/Button.h>
#include <Xfe/Label.h>
#include <Xfe/ProgressBar.h>
/* #include <Xfe/TaskBar.h> */
#include <Xfe/ToolBar.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDashBoard resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNdockCallback						"dockCallback"
#define XmNfloatingMapCallback				"floatingMapCallback"
#define XmNfloatingUnmapCallback			"floatingUnmapCallback"
#define XmNundockCallback					"undockCallback"

#define XmNdocked							"docked"
#define XmNdockedTaskBar					"dockedTaskBar"
#define XmNfloatingShell					"floatingShell"
#define XmNfloatingTarget					"floatingTarget"
#define XmNfloatingTaskBar					"floatingTaskBar"
#define XmNprogressBar						"progressBar"
#define XmNshowDockedTaskBar				"showDockedTaskBar"
#define XmNstatusBar						"statusBar"
#define XmNtoolBar							"toolBar"
#define XmNundockPixmap						"undockPixmap"

#define XmCDocked							"Docked"
#define XmCShowDockedTaskBar				"ShowDockedTaskBar"
#define XmCUndockPixmap						"UndockPixmap"

#define XmCR_XFE_LAST_REASON 1

/*----------------------------------------------------------------------*/
/*																		*/
/* Callback Reasons														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmCR_DOCK = XmCR_XFE_LAST_REASON + 99,		/* Task bar dock		*/
    XmCR_UNDOCK,								/* Task bar undock		*/ 
    XmCR_FLOATING_MAP,							/* Floating map			*/
    XmCR_FLOATING_UNMAP							/* Floating unmap		*/
};

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDashBoard components												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef enum
{
	XfeDASH_BOARD_DOCKED_TASK_BAR,
	XfeDASH_BOARD_FLOATING_SHELL,
	XfeDASH_BOARD_FLOATING_TASK_BAR,
	XfeDASH_BOARD_PROGRESS_BAR,
	XfeDASH_BOARD_STATUS_BAR,
	XfeDASH_BOARD_TOOL_BAR
} XfeDashBoardComponent;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeDashBoardWidgetClass;

typedef struct _XfeDashBoardClassRec *		XfeDashBoardWidgetClass;
typedef struct _XfeDashBoardRec *			XfeDashBoardWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsDashBoard(w)	XtIsSubclass(w,xfeDashBoardWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDashBoard Public Methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateDashBoard				(Widget		parent,
								 String		name,
								 Arg *		args,
								 Cardinal	num_args);
/*----------------------------------------------------------------------*/
extern Widget
XfeDashBoardGetComponent		(Widget					dashboard,
								 XfeDashBoardComponent	component);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end DashBoard.h		*/
