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
/* Name:		<Xfe/DividerP.h>										*/
/* Description:	XfeDivider widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeDividerP_h_							/* start DividerP.h		*/
#define _XfeDividerP_h_

#include <Xfe/Divider.h>
#include <Xfe/OrientedP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDividerClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfeDividerClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDividerClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDividerClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
    XfeOrientedClassPart		xfe_oriented_class;
    XfeDividerClassPart			xfe_divider_class;
} XfeDividerClassRec;

externalref XfeDividerClassRec xfeDividerClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDividerPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDividerPart
{
	/* Divider resources */
    Dimension			divider_fixed_size;		/* Divider fixed size	*/
    int					divider_percentage;		/* Divider percentage	*/
    Cardinal			divider_target;			/* Divider target		*/
    unsigned char		divider_type;			/* Sash shadow type		*/

    /* Private data -- Dont even look past this comment -- */
} XfeDividerPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDividerRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDividerRec
{
    CorePart				core;
    CompositePart			composite;
    ConstraintPart			constraint;
    XmManagerPart			manager;
    XfeManagerPart			xfe_manager;
    XfeDynamicManagerPart	xfe_dynamic_manager;
    XfeOrientedPart			xfe_oriented;
    XfeDividerPart			xfe_divider;
} XfeDividerRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDividerPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeDividerPart(w) &(((XfeDividerWidget) w) -> xfe_divider)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end DividerP.h		*/

