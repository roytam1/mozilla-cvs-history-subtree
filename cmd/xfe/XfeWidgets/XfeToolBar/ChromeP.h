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
/* Name:		<Xfe/ChromeP.h>											*/
/* Description:	XfeChrome widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeChromeP_h_							/* start ChromeP.h		*/
#define _XfeChromeP_h_

#include <Xfe/Chrome.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromeClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfeChromeClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromeClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeChromeClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
    XfeChromeClassPart			xfe_chrome_class;
} XfeChromeClassRec;

externalref XfeChromeClassRec xfeChromeClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromePart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeChromePart
{
    /* Components */
	Widget				menu_bar;				/* Menu bar				*/
	Widget				tool_box;				/* Tool box				*/
	Widget				dash_board;				/* Dash board			*/

    /* View components */
	Widget				center_view;			/* Center view			*/
	Widget				top_view;				/* Top view				*/
	Widget				bottom_view;			/* Bottom view			*/
	Widget				left_view;				/* Left view			*/
	Widget				right_view;				/* Right view			*/

    /* Spacing */
    Dimension			spacing;				/* Spacing				*/

    /* Private data -- Dont even look past this comment -- */

} XfeChromePart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromeRec															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeChromeRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    XfeManagerPart		xfe_manager;
    XfeChromePart		xfe_chrome;
} XfeChromeRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromeConstraintPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeChromeConstraintPart
{
	unsigned char			chrome_child_type;
/* 	Dimension			min_width; */
/*     Dimension			min_height; */
/*     Dimension			max_width; */
/*     Dimension			max_height; */
} XfeChromeConstraintPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromeConstraintRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeChromeConstraintRec
{
    XmManagerConstraintPart			manager;
    XfeManagerConstraintPart		xfe_manager;
    XfeChromeConstraintPart			xfe_chrome;
} XfeChromeConstraintRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromePart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeChromePart(w) &(((XfeChromeWidget) w) -> xfe_chrome)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeChromePart child constraint part access macro						*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeChromeConstraintPart(w) \
(&(((XfeChromeConstraintRec *) _XfeConstraints(w)) -> xfe_chrome))

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ChromeP.h		*/

