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
/* Name:		<Xfe/Pane.h>											*/
/* Description:	XfePane widget public header file.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfePane_h_								/* start Pane.h			*/
#define _XfePane_h_

#include <Xfe/Xfe.h>
#include <Xfe/Oriented.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePane resource names												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNallowExpand					"allowExpand"
#define XmNalwaysVisible				"alwaysVisible"
#define XmNattachmentOneBottom			"attachmentOneBottom"
#define XmNattachmentOneLeft			"attachmentOneLeft"
#define XmNattachmentOneRight			"attachmentOneRight"
#define XmNattachmentOneTop				"attachmentOneTop"
#define XmNattachmentTwoBottom			"attachmentTwoBottom"
#define XmNattachmentTwoLeft			"attachmentTwoLeft"
#define XmNpaneChildAttachment			"paneChildAttachment"
#define XmNpaneChildType				"paneChildType"
#define XmNpaneSashType					"paneSashType"
#define XmNattachmentTwoRight			"attachmentTwoRight"
#define XmNsashColor					"sashColor"
#define XmNsashOffset					"sashOffset"
#define XmNsashPosition					"sashPosition"
#define XmNsashSpacing					"sashSpacing"
#define XmNsashThickness				"sashThickness"
#define XmNchildOne						"childOne"
#define XmNchildTwo						"childTwo"
#define XmNattachmentTwoTop				"attachmentTwoTop"
#define XmNpaneDragMode					"paneDragMode"
#define XmNsashAlwaysVisible			"sashAlwaysVisible"
#define XmNsashShadowType				"sashShadowType"

#define XmCAlwaysVisible				"AlwaysVisible"
#define XmCAttachmentOneBottom			"AttachmentOneBottom"
#define XmCAttachmentOneLeft			"AttachmentOneLeft"
#define XmCAttachmentOneRight			"AttachmentOneRight"
#define XmCAttachmentOneTop				"AttachmentOneTop"
#define XmCAttachmentTwoBottom			"AttachmentTwoBottom"
#define XmCAttachmentTwoLeft			"AttachmentTwoLeft"
#define XmCAttachmentTwoRight			"AttachmentTwoRight"
#define XmCAttachmentTwoTop				"AttachmentTwoTop"
#define XmCChildOne						"ChildOne"
#define XmCChildTwo						"ChildTwo"
#define XmCSashColor					"SashColor"
#define XmCPaneChildAttachment			"PaneChildAttachment"
#define XmCPaneChildType				"PaneChildType"
#define XmCPaneDragMode					"PaneDragMode"
#define XmCPaneSashType					"PaneSashType"
#define XmCSashAlwaysVisible			"SashAlwaysVisible"
#define XmCSashSpacing					"SashSpacing"
#define XmCSashThickness				"SashThickness"

#define XmRPaneChildAttachment			"PaneChildAttachment"
#define XmRPaneChildType				"PaneChildType"
#define XmRPaneDragMode					"PaneDragMode"
#define XmRPaneSashType					"PaneSashType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRPaneChildType														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmPANE_CHILD_NONE,
	XmPANE_CHILD_ATTACHMENT_ONE,
	XmPANE_CHILD_ATTACHMENT_TWO,
	XmPANE_CHILD_WORK_AREA_ONE,
	XmPANE_CHILD_WORK_AREA_TWO
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRPaneAttachmentType												*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmPANE_CHILD_ATTACH_NONE,
	XmPANE_CHILD_ATTACH_BOTTOM,
	XmPANE_CHILD_ATTACH_LEFT,
	XmPANE_CHILD_ATTACH_RIGHT,
	XmPANE_CHILD_ATTACH_TOP
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRPaneDragModeType													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmPANE_DRAG_PRESERVE_ONE,
	XmPANE_DRAG_PRESERVE_TWO,
	XmPANE_DRAG_PRESERVE_RATIO
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRSashType															*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmPANE_SASH_DOUBLE_LINE,
	XmPANE_SASH_FILLED_RECTANGLE,
	XmPANE_SASH_LIVE,
	XmPANE_SASH_RECTANGLE,
	XmPANE_SASH_SINGLE_LINE
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfePaneWidgetClass;

typedef struct _XfePaneClassRec *			XfePaneWidgetClass;
typedef struct _XfePaneRec *				XfePaneWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsPane(w)	XtIsSubclass(w,xfePaneWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePane public methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreatePane					(Widget		pw,
								 String		name,
								 Arg *		av,
								 Cardinal	ac);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Pane.h			*/
