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
/* Name:		<Xfe/ToolBoxP.h>										*/
/* Description:	XfeToolBox widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolBoxP_h_							/* start ToolBoxP.h		*/
#define _XfeToolBoxP_h_

#include <Xfe/ToolBox.h>
#include <Xfe/DynamicManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer		extension;					/* Extension			*/ 
} XfeToolBoxClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolBoxClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ConstraintClassPart			constraint_class;
	XmManagerClassPart			manager_class;
	XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
	XfeToolBoxClassPart			xfe_tool_box_class;
} XfeToolBoxClassRec;

externalref XfeToolBoxClassRec xfeToolBoxClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolBoxPart
{
    /* Callbacks */
	XtCallbackList		new_item_callback;		/* New item callback	*/
	XtCallbackList		swap_callback;			/* Swap callback		*/
    XtCallbackList		close_callback;			/* Close callback		*/
    XtCallbackList		drag_allow_callback;	/* Drag allow callback	*/
    XtCallbackList		drag_end_callback;		/* Drag end callback	*/
    XtCallbackList		drag_motion_callback;	/* Drag motion callback	*/
    XtCallbackList		drag_start_callback;	/* Drag start callback	*/
    XtCallbackList		open_callback;			/* Open callback		*/
    XtCallbackList		snap_callback;			/* Snap callback		*/

    /* Item resources */
	WidgetList			items;					/* Items				*/
	Cardinal			item_count;				/* Item count			*/

    /* Tab resources */
	WidgetList			closed_tabs;			/* Closed tabs			*/
	WidgetList			opened_tabs;			/* Opened tabs			*/
	Dimension			tab_offset;				/* Tab offset			*/

    /* Spacing resources */
	Dimension			vertical_spacing;		/* Vertical spacing		*/
	Dimension			horizontal_spacing;		/* Horizontal spacing	*/

    /* Drag resources */
	Dimension			drag_threshold;			/* Drag threshold		*/ 
	Cursor				drag_cursor;			/* Drag cursor			*/
	int					drag_button;			/* Drag button			*/

    /* Swap resources */
	Dimension			swap_threshold;			/* Swap threshold		*/ 
    
    /* Pixmap resources */
	Pixmap				bottom_pixmap;			/* Bottom pixmap		*/
	Pixmap				horizontal_pixmap;		/* Horizontal pixmap	*/
	Pixmap				left_pixmap;			/* Left pixmap			*/
	Pixmap				right_pixmap;			/* Right pixmap			*/
	Pixmap				top_pixmap;				/* Top pixmap			*/
	Pixmap				vertical_pixmap;		/* Vertical pixmap		*/
	Pixmap				bottom_raised_pixmap;	/* Bottom Raised pixmap	*/
	Pixmap				horizontal_raised_pixmap;/* Hor Raised pixmap	*/
	Pixmap				left_raised_pixmap;		/* Left Raised pixmap	*/
	Pixmap				right_raised_pixmap;	/* Right Raised pixmap	*/
	Pixmap				top_raised_pixmap;		/* Top Raised pixmap	*/
	Pixmap				vertical_raised_pixmap;	/* Ver Raised pixmap	*/

	/* Private data -- Dont even look past this comment -- */
	int					last_y;					/* Last y				*/
	int					original_y;				/* Origian y			*/
	int					last_drag_y;			/* Last drag y			*/
	int					start_drag_y;			/* Start drag y			*/

	Widget				last_moved_item;		/* Last moved item		*/

	int drag_direction;

	Boolean				dragging;				/* Dragging ?			*/
	Boolean				dragging_tab;			/* Dragging Tab ?		*/
	Boolean				clicking_tab;			/* Clicking Tab ?		*/

} XfeToolBoxPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolBoxRec
{
	CorePart				core;
	CompositePart			composite;
	ConstraintPart			constraint;
	XmManagerPart			manager;
	XfeManagerPart			xfe_manager;
	XfeDynamicManagerPart	xfe_dynamic_manager;
	XfeToolBoxPart			xfe_tool_box;
} XfeToolBoxRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxConstraintPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolBoxConstraintPart
{
	Boolean 		open;				/* open			*/
} XfeToolBoxConstraintPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxConstraintRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolBoxConstraintRec
{
    XmManagerConstraintPart			manager;
    XfeManagerConstraintPart		xfe_manager;
    XfeDynamicManagerConstraintPart	xfe_dynamic_manager;
    XfeToolBoxConstraintPart		xfe_tool_box;
} XfeToolBoxConstraintRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolBoxPart(w) &(((XfeToolBoxWidget) w) -> xfe_tool_box)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxPart child constraint part access macro					*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolBoxConstraintPart(w) \
(&(((XfeToolBoxConstraintRec *) _XfeConstraints(w)) -> xfe_tool_box))

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolBoxPart child constraint open access macro					*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolBoxChildOpen(child) \
((_XfeToolBoxConstraintPart(child)) -> open)
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolBoxP.h	*/

