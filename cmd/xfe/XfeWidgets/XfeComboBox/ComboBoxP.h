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
/* Name:		<Xfe/ComboBoxP.h>										*/
/* Description:	XfeComboBox widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeComboBoxP_h_						/* start ComboBoxP.h	*/
#define _XfeComboBoxP_h_

#include <Xfe/ComboBox.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBox method inheritance macros								*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeInheritLayoutTitle				((XtWidgetProc)			_XtInherit)
#define XfeInheritLayoutArrow				((XtWidgetProc)			_XtInherit)
#define XfeInheritDrawHighlight				((XfeExposeProc)		_XtInherit)
#define XfeInheritDrawTitleShadow			((XfeExposeProc)		_XtInherit)
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBoxClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtWidgetProc			layout_title;		/* layout_title			*/
    XtWidgetProc			layout_arrow;		/* layout_arrow			*/
    XfeExposeProc			draw_highlight;		/* draw_highlight		*/
    XfeExposeProc			draw_title_shadow;	/* draw_title_shadow	*/
	XtPointer				extension;			/* extension			*/
} XfeComboBoxClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBoxClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeComboBoxClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
    XfeComboBoxClassPart		xfe_combo_box_class;
} XfeComboBoxClassRec;

externalref XfeComboBoxClassRec xfeComboBoxClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBoxPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeComboBoxPart
{
	/* Callback Resources */
	XtCallbackList		text_activate_callback;	/* Text activate cb		*/

    /* XmTextField manipulation resources */
	XfeComboBoxSetTextProc	set_text_proc;		/* Set text proc		*/
	XfeComboBoxGetTextFunc	get_text_func;		/* Get text func		*/

    /* Title resources */
    unsigned char		combo_box_type;			/* Combo box type		*/
	Widget				title;					/* Title				*/
    Dimension			spacing;				/* Spacing				*/
	XmFontList			title_font_list;		/* Title font list		*/
    Dimension			title_shadow_thickness;	/* Title shadow thickness*/
    unsigned char		title_shadow_type;		/* Title shadow type	*/

	/* List resources */
	Widget				list;					/* List					*/
    XmStringTable		items;					/* Items				*/
    int					item_count;				/* Item count			*/
	XmFontList			list_font_list;			/* List font list		*/
    Dimension			list_margin_height;		/* List margin height	*/
    Dimension			list_margin_width;		/* List margin width	*/
    Dimension			list_spacing;			/* List spacing			*/
	int					top_item_position;		/* Top item position	*/
	int					visible_item_count;		/* Visible item count	*/

	/* Arrow resources */
	Widget				arrow;					/* Arrow				*/

	/* Shell resources */
	Boolean				share_shell;			/* Share shell			*/
	Widget				shell;					/* Shell				*/
	Boolean				popped_up;				/* Popped up ?			*/

	/* Selected resources */
	XmString			selected_item;			/* Selected				*/
	int					selected_position;		/* Selected position	*/

	/* Traversal resources */
    Dimension			highlight_thickness;	/* Highlight thickness	*/
    Boolean				traversal_on;			/* Traversal on ?		*/

    /* Private data -- Dont even look past this comment -- */
    Boolean				highlighted;			/* Highlighted ?		*/
    Boolean				remain_popped_up;		/* Remain popped up ?	*/
	XtIntervalId		delay_timer_id;			/* Delay timer id		*/

} XfeComboBoxPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBoxRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeComboBoxRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    XfeManagerPart		xfe_manager;
    XfeComboBoxPart		xfe_combo_box;
} XfeComboBoxRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBoxPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeComboBoxPart(w) &(((XfeComboBoxWidget) w) -> xfe_combo_box)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBox method invocation functions								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeComboBoxLayoutTitle			(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeComboBoxLayoutArrow			(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeComboBoxDrawHighlight		(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/
extern void
_XfeComboBoxDrawTitleShadow		(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeComboBox - superclass = XfeManager								*/
/*																		*/
/* Component preparation macros.										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XFE_PREPARE_ARROW							XfePrepare1

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ComboBoxP.h		*/

