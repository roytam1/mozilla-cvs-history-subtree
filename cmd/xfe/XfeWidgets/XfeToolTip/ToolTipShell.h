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
/* Name:		<Xfe/ToolTipShell.h>									*/
/* Description:	XfeToolTipShell widget public header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolTipShell_h_						/* start ToolTipShell.h	*/
#define _XfeToolTipShell_h_

#include <Xfe/BypassShell.h>
#include <Xfe/Button.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShell resource names										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNtoolTipCallback				"toolTipCallback"

#define XmNtoolTipLabel					"toolTipLabel"
#define XmNtoolTipPlacement				"toolTipPlacement"
#define XmNtoolTipTimeout				"toolTipTimeout"
#define XmNtoolTipType					"toolTipType"
#define XmNtoolTipHorizontalOffset		"toolTipHorizontalOffset"
#define XmNtoolTipVerticalOffset		"toolTipVerticalOffset"

#define XmCToolTipPlacement				"ToolTipPlacement"
#define XmCToolTipTimeout				"ToolTipTimeout"
#define XmCToolTipType					"ToolTipType"

#define XmRToolTipPlacement				"ToolTipPlacement"
#define XmRToolTipType					"ToolTipType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTip reasonable defaults for some resources					*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeDEFAULT_TOOL_TIP_BOTTOM_OFFSET				10
#define XfeDEFAULT_TOOL_TIP_LEFT_OFFSET					10
#define XfeDEFAULT_TOOL_TIP_RIGHT_OFFSET				10
#define XfeDEFAULT_TOOL_TIP_TOP_OFFSET					10
#define XfeDEFAULT_TOOL_TIP_TIMEOUT						200

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRToolTipType														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmTOOL_TIP_EDITABLE,						/*						*/
	XmTOOL_TIP_READ_ONLY						/*						*/
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRToolTipPlacement													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmTOOL_TIP_PLACE_BOTTOM,					/*						*/
	XmTOOL_TIP_PLACE_LEFT,						/*						*/
	XmTOOL_TIP_PLACE_RIGHT,						/*						*/
	XmTOOL_TIP_PLACE_TOP						/*						*/
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTip class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeToolTipShellWidgetClass;

typedef struct _XfeToolTipShellClassRec *		XfeToolTipShellWidgetClass;
typedef struct _XfeToolTipShellRec *			XfeToolTipShellWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTip subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsToolTipShell(w)	XtIsSubclass(w,xfeToolTipShellWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShell public methods										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateToolTipShell				(Widget		pw,
									 String		name,
									 Arg *		av,
									 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolTipShellGetLabel				(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeToolTipShellSetString			(Widget		w,
									 XmString	string);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolTipShell.h	*/
