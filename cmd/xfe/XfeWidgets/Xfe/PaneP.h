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
/* Name:		<Xfe/PaneP.h>											*/
/* Description:	XfePane widget private header file.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfePaneP_h_							/* start PaneP.h		*/
#define _XfePaneP_h_

#include <Xfe/Pane.h>
#include <Xfe/OrientedP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfePaneClassPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfePaneClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePaneClassRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePaneClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
    XfeOrientedClassPart		xfe_oriented_class;
    XfePaneClassPart			xfe_pane_class;
} XfePaneClassRec;

externalref XfePaneClassRec xfePaneClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePanePart															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePanePart
{
	/* Sash resources */
	Boolean				sash_always_visible;	/* Sash always visible	*/
	Pixel				sash_color;				/* Pane color			*/
	Dimension			sash_offset;			/* Sash offset			*/
	Position			sash_position;			/* Sash position		*/
	Dimension			sash_shadow_thickness;	/* Sash shadow thickness*/
    unsigned char		sash_shadow_type;		/* Sash shadow type		*/
	Dimension			sash_spacing;			/* Sash spacing			*/
	Dimension			sash_thickness;			/* Sash thickness		*/

	/* Drag resources */
    unsigned char		pane_sash_type;			/* Pane sash type		*/
    unsigned char		pane_drag_mode;			/* Pane drag mode		*/
	Dimension			drag_threshold;			/* Drag threshold		*/
	/* Child one resources */
	Widget				child_one;				/* Child one			*/
	Widget				attachment_one_bottom;	/* Child one bottom		*/
	Widget				attachment_one_left;	/* Child one left		*/
	Widget				attachment_one_right;	/* Child one right		*/
	Widget				attachment_one_top;		/* Child one top		*/

	/* Child two resources */
	Widget				child_two;				/* Child two			*/
	Widget				attachment_two_bottom;	/* Child two bottom		*/
	Widget				attachment_two_left;	/* Child two left		*/
	Widget				attachment_two_right;	/* Child two right		*/
	Widget				attachment_two_top;		/* Child two top		*/

    /* Private data -- Dont even look past this comment -- */
	GC					sash_GC;				/* Sash GC				*/
	XRectangle			sash_rect;				/* Sash rect			*/
	XRectangle			sash_dragging_rect;		/* Sash dragging rect	*/
	int					sash_dragging_last;		/* Sash dragging last	*/
	Position			sash_original_position;	/* Sash original pos	*/

	Dimension			old_width;
	Dimension			old_height;
	Position			old_sash_position;		/* Old sash position	*/

} XfePanePart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePaneRec															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePaneRec
{
    CorePart				core;
    CompositePart			composite;
    ConstraintPart			constraint;
    XmManagerPart			manager;
    XfeManagerPart			xfe_manager;
    XfeDynamicManagerPart	xfe_dynamic_manager;
    XfeOrientedPart			xfe_oriented;
    XfePanePart				xfe_pane;
} XfePaneRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePaneConstraintPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePaneConstraintPart
{
	Dimension			pane_minimum;
    Dimension			pane_maximum;
	unsigned char		pane_child_attachment;
	unsigned char		pane_child_type;
	Boolean				allow_resize;
	Boolean				allow_expand;
	Boolean				always_visible;
} XfePaneConstraintPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePaneConstraintRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePaneConstraintRec
{
    XmManagerConstraintPart			manager;
    XfeManagerConstraintPart		xfe_manager;
	XfeDynamicManagerConstraintPart	xfe_dynamic_manager;
    XfeOrientedConstraintPart		xfe_oriented;
    XfePaneConstraintPart			xfe_pane;
} XfePaneConstraintRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePanePart Access Macro												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfePanePart(w) &(((XfePaneWidget) w) -> xfe_pane)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePanePart child constraint part access macro						*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfePaneConstraintPart(w) \
(&(((XfePaneConstraintRec *) _XfeConstraints(w)) -> xfe_pane))

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end PaneP.h			*/

