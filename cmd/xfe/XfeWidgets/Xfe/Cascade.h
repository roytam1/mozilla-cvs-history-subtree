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
/* Name:		<Xfe/Cascade.h>											*/
/* Description:	XfeCascade widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeCascade_h_							/* start Cascade.h		*/
#define _XfeCascade_h_

#include <Xfe/Button.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascade resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNsubmenuTearCallback				"submenuTearCallback"

#define XmNallowTearOff					"allowTearOff"
#define XmNcascadeArrowDirection		"cascadeArrowDirection"
#define XmNcascadeArrowLocation			"cascadeArrowLocation"
#define XmNcascadeArrowHeight			"cascadeArrowHeight"
#define XmNcascadeArrowWidth			"cascadeArrowWidth"
#define XmNdrawCascadeArrow				"drawCascadeArrow"
#define XmNtorn							"torn"
#define XmNmatchSubMenuWidth			"matchSubMenuWidth"
#define XmNsubMenuAlignment				"subMenuAlignment"
#define XmNsubMenuLocation				"subMenuLocation"
#define XmNtornShellTitle				"tornShellTitle"

#define XmCCascadeArrowDirection		"CascadeArrowDirection"
#define XmCCascadeArrowHeight			"CascadeArrowHeight"
#define XmCCascadeArrowLocation			"CascadeArrowLocation"
#define XmCSubMenuAlignment				"SubMenuAlignment"
#define XmCCascadeArrowWidth			"CascadeArrowWidth"
#define XmCDrawCascadeArrow				"DrawCascadeArrow"
#define XmCMatchSubMenuWidth			"MatchSubMenuWidth"
#define XmCSubMenuLocation				"SubMenuLocation"
#define XmCTornShellTitle				"TornShellTitle"

#define XmRLocationType					"LocationType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRLocationType														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmLOCATION_EAST,
	XmLOCATION_NORTH,
	XmLOCATION_NORTH_EAST,
	XmLOCATION_NORTH_WEST,
	XmLOCATION_SOUTH,
	XmLOCATION_SOUTH_EAST,
	XmLOCATION_SOUTH_WEST,
	XmLOCATION_WEST
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Cascade tear submenu callback structure								*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
    Boolean		torn;					/* Cascade torn ?				*/
} XfeSubmenuTearCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascade class names												*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeCascadeWidgetClass;
    
typedef struct _XfeCascadeClassRec *	XfeCascadeWidgetClass;
typedef struct _XfeCascadeRec *			XfeCascadeWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascade subclass test macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsCascade(w)	XtIsSubclass(w,xfeCascadeWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascade public functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateCascade			(Widget		parent,
							 String		name,
							 Arg *		args,
							 Cardinal	num_args);
/*----------------------------------------------------------------------*/
extern void
XfeCascadeDestroyChildren	(Widget		w);
/*----------------------------------------------------------------------*/
extern Boolean
XfeCascadeArmAndPost		(Widget		w,
							 XEvent *	event);
/*----------------------------------------------------------------------*/
extern Boolean
XfeCascadeDisarmAndUnpost	(Widget		w,
							 XEvent *	event);
/*----------------------------------------------------------------------*/
extern void
XfeCascadeGetChildren		(Widget			w,
							 WidgetList *	children,
							 Cardinal *		num_children);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Cascade.h		*/
