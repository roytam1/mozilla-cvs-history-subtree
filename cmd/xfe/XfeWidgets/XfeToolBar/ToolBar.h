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
/* Name:		<Xfe/ToolBar.h>											*/
/* Description:	XfeToolBar widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolBar_h_							/* start ToolBar.h		*/
#define _XfeToolBar_h_

#include <Xfe/Oriented.h>
#include <Xfe/Cascade.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBar resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/

#define XmNforceDimensionCallback			"forceDimensionCallback"

#define XmNactiveButton						"activeButton"
#define XmNallowWrap						"allowWrap"
#define XmNdynamicIndicator					"dynamicIndicator"
#define XmNforceDimensionToMax				"forceDimensionToMax"
#define XmNindicatorLocation				"indicatorLocation"
#define XmNindicatorPosition				"indicatorPosition"
#define XmNindicatorThreshold				"indicatorThreshold"
#define XmNmaxNumColumns					"maxNumColumns"
#define XmNmaxNumRows						"maxNumRows"
#define XmNnumRows							"numRows"
#define XmNselectedButton					"selectedButton"
#define XmNseparatorThickness				"separatorThickness"
#define XmNtoggleBehavior					"toggleBehavior"
#define XmNtoolBar							"toolBar"

#define XmCActiveButton						"ActiveButton"
#define XmCDynamicIndicator					"DynamicIndicator"
#define XmCForceDimensionToMax				"ForceDimensionToMax"
#define XmCIndicatorLocation				"IndicatorLocation"
#define XmCIndicatorPosition				"IndicatorPosition"
#define XmCIndicatorThreshold				"IndicatorThreshold"
#define XmCMaxNumColumns					"MaxNumColumns"
#define XmCMaxNumRows						"MaxNumRows"
#define XmCNumRows							"NumRows"
#define XmCSelectedButton					"SelectedButton"
#define XmCSeparatorThickness				"SeparatorThickness"
#define XmCToggleBehavior					"ToggleBehavior"

#define XmRToolBarIndicatorLocation			"ToolBarIndicatorLocation"
#define XmRToolBarSelectionPolicy			"ToolBarSelectionPolicy"
#define XmRToolBarToggleBehavior			"ToolBarToggleBehavior"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRToolBarSelectionType												*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmTOOL_BAR_SELECT_NONE,
	XmTOOL_BAR_SELECT_SINGLE,
	XmTOOL_BAR_SELECT_MULTIPLE
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRToolBarIndicatorLocation											*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmINDICATOR_LOCATION_NONE,
	XmINDICATOR_LOCATION_BEGINNING,
	XmINDICATOR_LOCATION_END,
	XmINDICATOR_LOCATION_MIDDLE
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRToolBarToggleBehavior												*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmTOOL_BAR_TOGGLE_ONE_OR_MORE,
	XmTOOL_BAR_TOGGLE_ONLY_ONE,
	XmTOOL_BAR_TOGGLE_ZERO_OR_MORE,
	XmTOOL_BAR_TOGGLE_ZERO_OR_ONE
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Callback Reasons														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmCR_TOOL_BAR_SELECTION_CHANGED = XmCR_XFE_LAST_REASON + 99, /* ToolBar selection changed */
	XmCR_TOOL_BAR_VALUE_CHANGED,			/* ToolBar value changed	*/
	XmCR_FORCE_DIMENSION					/* ToolBar force dimensions	*/
};

/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* XmINDICATOR_DONT_SHOW - for XmNindicatorPosition to hide indicator.	*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmINDICATOR_DONT_SHOW -2

/*----------------------------------------------------------------------*/
/*																		*/
/* ToolBar callback structure											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
	Widget		button;					/* Button that invoked callback	*/
    Boolean		armed;					/* Button armed ?				*/
    Boolean		selected;				/* Button selected ?			*/
} XfeToolBarCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* ToolBar force dimensions callback structure							*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
	Widget		child;					/* Child that invoked callback	*/
	Boolean		allow_change;			/* Allow the change ?			*/
} XfeToolBarForceDimensionCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeToolBarWidgetClass;

typedef struct _XfeToolBarClassRec *		XfeToolBarWidgetClass;
typedef struct _XfeToolBarRec *				XfeToolBarWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsToolBar(w)	XtIsSubclass(w,xfeToolBarWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBar Public Methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateToolBar				(Widget		pw,
								 String		name,
								 Arg *		av,
								 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern Boolean
XfeToolBarSetActiveButton		(Widget		w,
								 Widget		button);
/*----------------------------------------------------------------------*/
extern Boolean
XfeToolBarSetSelectedButton		(Widget		w,
								 Widget		button);
/*----------------------------------------------------------------------*/
extern unsigned char
XfeToolBarXYToIndicatorLocation	(Widget		w,
								 Widget		item,
								 int		x,
								 int		y);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBarGetFirstItem			(Widget		w);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBarGetLastItem			(Widget		w);
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBarGetIndicatorItem		(Widget		w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBar item editting functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeToolBarGetEditText			(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeToolBarEditItem				(Widget		w,
								 Widget		item,
								 int		label_x,
								 int		label_y,
								 int		label_width,
								 int		label_height);
/*----------------------------------------------------------------------*/
	
XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolBar.h		*/
