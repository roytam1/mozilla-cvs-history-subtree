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
/* Name:		<Xfe/ToolItemP.h>										*/
/* Description:	XfeToolItem widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolItemP_h_						/* start ToolItemP.h	*/
#define _XfeToolItemP_h_

#include <Xfe/ToolItem.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolItemClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer		extension;					/* Extension			*/ 
} XfeToolItemClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolItemClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolItemClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ConstraintClassPart			constraint_class;
	XmManagerClassPart			manager_class;
	XfeManagerClassPart			xfe_manager_class;
	XfeToolItemClassPart		xfe_tool_item_class;
} XfeToolItemClassRec;

externalref XfeToolItemClassRec xfeToolItemClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolItemPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolItemPart
{
    /* Resources */
	Widget				item;					/* Item					*/
    Widget				logo;					/* Logo					*/
    Dimension			spacing;				/* Spacing				*/

} XfeToolItemPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolItemRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolItemRec
{
   CorePart				core;
   CompositePart		composite;
   ConstraintPart		constraint;
   XmManagerPart		manager;
   XfeManagerPart		xfe_manager;
   XfeToolItemPart		xfe_tool_item;
} XfeToolItemRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolItemPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolItemPart(w) &(((XfeToolItemWidget) w) -> xfe_tool_item)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolItemP.h		*/

