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
/* Name:		<Xfe/Manager.h>											*/
/* Description:	XfeManager widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeManager_h_							/* start Manager.h		*/
#define _XfeManager_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNchangeManagedCallback			"changeManagedCallback"
#define XmNlayoutCallback					"layoutCallback"

#define XmNbusy								"busy"
#define XmNbusyCursor						"busyCursor"
#define XmNbusyCursorOn						"busyCursorOn"
#define XmNcomponentChildren				"componentChildren"
#define XmNlayoutFrozen						"layoutFrozen"
#define XmNmanagerChildType					"managerChildType"
#define XmNmaxComponentChildrenHeight		"maxComponentChildrenHeight"
#define XmNmaxComponentChildrenWidth		"maxComponentChildrenWidth"
#define XmNmaxStaticChildrenHeight			"maxStaticChildrenHeight"
#define XmNmaxStaticChildrenWidth			"maxStaticChildrenWidth"
#define XmNnumComponentChildren				"numComponentChildren"
#define XmNnumManagedComponentChildren		"numManagedComponentChildren"
#define XmNnumManagedStaticChildren			"numManagedStaticChildren"
#define XmNnumStaticChildren				"numStaticChildren"
#define XmNstaticChildren					"staticChildren"
#define XmNtotalComponentChildrenHeight		"totalComponentChildrenHeight"
#define XmNtotalComponentChildrenWidth		"totalComponentChildrenWidth"
#define XmNtotalStaticChildrenHeight		"totalStaticChildrenHeight"
#define XmNtotalStaticChildrenWidth			"totalStaticChildrenWidth"

#define XmCBusy								"Busy"
#define XmCBusyCursor						"BusyCursor"
#define XmCBusyCursorOn						"BusyCursorOn"
#define XmCLayoutFrozen						"LayoutFrozen"
#define XmRLinked							XmRPointer
#define XmRLinkedChildren					"LinkedChildren"
#define XmRManagerChildType					"ManagerChildType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRManagerChildType													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmMANAGER_COMPONENT_INVALID,			/*						*/
	XmMANAGER_COMPONENT_CHILD,				/*						*/
	XmMANAGER_DYNAMIC_CHILD,				/*						*/
	XmMANAGER_STATIC_CHILD					/*						*/
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager children info/apply mask bits								*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeCHILDREN_INFO_NONE		(0)
#define XfeCHILDREN_INFO_ALIVE		(1 << 0)
#define XfeCHILDREN_INFO_MANAGED	(1 << 1)
#define XfeCHILDREN_INFO_REALIZED	(1 << 2)
#define XfeCHILDREN_INFO_ANY		(~(0))

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager class names												*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeManagerWidgetClass;

typedef struct _XfeManagerClassRec *	XfeManagerWidgetClass;
typedef struct _XfeManagerRec *			XfeManagerWidget;

#define XfeIsManager(w)	XtIsSubclass(w,xfeManagerWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* Manager apply function type											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef void		(*XfeManagerApplyProc)	(Widget		w,
											 Widget		child,
											 XtPointer	client_data);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager public methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeManagerLayout				(Widget					w);
/*----------------------------------------------------------------------*/
extern void
XfeManagerSetChildrenValues		(Widget					w,
								 ArgList				args,
								 Cardinal				n,
								 Boolean				only_managed);
/*----------------------------------------------------------------------*/
extern void
XfeManagerResizeChildren		(Widget					w,
								 Boolean				set_width,
								 Boolean				set_height,
								 Boolean				only_managed);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeManager public children apply functions							*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeManagerApply					(Widget					w,
								 int					mask,
								 XfeManagerApplyProc	proc,
								 XtPointer				data,
								 Boolean				private_components,
								 Boolean				freeze_layout);
/*----------------------------------------------------------------------*/
extern void
XfeManagerApplyLinked			(Widget					w,
								 unsigned char			child_type,
								 int					mask,
								 XfeManagerApplyProc	proc,
								 XtPointer				data,
								 Boolean				freeze_layout);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif						/* end Manager.h	*/
