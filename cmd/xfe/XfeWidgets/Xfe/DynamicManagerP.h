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
/* Name:		<Xfe/DynamicManagerP.h>									*/
/* Description:	XfeDynamicManager widget private header file.			*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeDynamicManagerP_h_				/* start DynamicManagerP.h	*/
#define _XfeDynamicManagerP_h_

#include <Xfe/DynamicManager.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager method inheritance macros							*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeInheritAcceptDynamicChild		((XfeChildFunc)			_XtInherit)
#define XfeInheritDeleteDynamicChild		((XfeChildFunc)			_XtInherit)
#define XfeInheritInsertDynamicChild		((XfeChildFunc)			_XtInherit)
#define XfeInheritLayoutDynamicChildren		((XtWidgetProc)			_XtInherit)
#define XfeInheritGetChildDimensions		((XfeGeometryProc)		_XtInherit)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerClassPart											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	/* Dynamic children methods */
	XfeChildFunc		accept_dynamic_child;	/* accept_dynamic_child		*/
	XfeChildFunc		insert_dynamic_child;	/* insert_dynamic_child		*/
	XfeChildFunc		delete_dynamic_child;	/* delete_dynamic_child		*/
	XtWidgetProc		layout_dynamic_children;/* layout_dynamic_children	*/
	XfeGeometryProc		get_child_dimensions;	/* get_child_dimensions		*/
	XtPointer			extension;				/* extension				*/

} XfeDynamicManagerClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerClassRec											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDynamicManagerClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ConstraintClassPart			constraint_class;
	XmManagerClassPart			manager_class;
	XfeManagerClassPart			xfe_manager_class;
	XfeDynamicManagerClassPart	xfe_dynamic_manager_class;
} XfeDynamicManagerClassRec;

externalref XfeDynamicManagerClassRec xfeDynamicManagerClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDynamicManagerPart
{
	/* Callback Resources */

	/* Dynamic children resources */
	XfeLinked			dynamic_children;		/* Dynamic children			*/

	Dimension			max_dyn_width;			/* Max dyn width			*/
	Dimension			max_dyn_height;			/* Max dyn height			*/

	Dimension			min_dyn_width;			/* Min dyn width			*/
	Dimension			min_dyn_height;			/* Min dyn height			*/

	Cardinal			num_dyn_children;		/* Num dyn children			*/
	Cardinal			num_managed_dyn_children;/* Num managed dyn children*/

	Dimension			total_dyn_width;		/* Total dyn width			*/
	Dimension			total_dyn_height;		/* Total dyn height			*/

	/* Private Data Members */

} XfeDynamicManagerPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDynamicManagerRec
{
	CorePart				core;
	CompositePart			composite;
	ConstraintPart			constraint;
	XmManagerPart			manager;
	XfeManagerPart			xfe_manager;
	XfeDynamicManagerPart	xfe_dynamic_manager;
} XfeDynamicManagerRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerConstraintPart										*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDynamicManagerConstraintPart
{
    int					position_index;			/* Position Index		*/
	XfeLinkNode			link_node;				/* Link node			*/
} XfeDynamicManagerConstraintPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManagerConstraintRec										*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeDynamicManagerConstraintRec
{
	XmManagerConstraintPart			manager;
	XfeManagerConstraintPart		xfe_manager;
	XfeDynamicManagerConstraintPart	xfe_dynamic_manager;
} XfeDynamicManagerConstraintRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager Method invocation functions						*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
_XfeDynamicManagerAcceptDynamicChild		(Widget			child);
/*----------------------------------------------------------------------*/
extern Boolean
_XfeDynamicManagerInsertDynamicChild		(Widget			child);
/*----------------------------------------------------------------------*/
extern Boolean
_XfeDynamicManagerDeleteDynamicChild		(Widget			child);
/*----------------------------------------------------------------------*/
extern void
_XfeDynamicManagerLayoutDynamicChildren		(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeDynamicManagerGetChildDimensions		(Widget			child,
											 Dimension *	width_out,
											 Dimension *	height_out);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager private functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeDynamicManagerChildrenInfo				(Widget			w,
											 Dimension *	max_width_out,
											 Dimension *	max_height_out,
											 Dimension *	total_width_out,
											 Dimension *	total_height_out,
											 Cardinal *		num_managed_out,
											 Cardinal *		num_components_out);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager member access										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfemDynamicChildren(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . dynamic_children)
/*----------------------------------------------------------------------*/
#define _XfemNumDynamicChildren(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . num_dyn_children)
/*----------------------------------------------------------------------*/
#define _XfemNumManagedDynamicChildren(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . num_managed_dyn_children)
/*----------------------------------------------------------------------*/
#define _XfemMaxDynamicWidth(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . max_dyn_width)
/*----------------------------------------------------------------------*/
#define _XfemMaxDynamicHeight(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . max_dyn_height)
/*----------------------------------------------------------------------*/
#define _XfemMinDynamicWidth(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . min_dyn_width)
/*----------------------------------------------------------------------*/
#define _XfemMinDynamicHeight(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . min_dyn_height)
/*----------------------------------------------------------------------*/
#define _XfemTotalDynamicWidth(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . total_dyn_width)
/*----------------------------------------------------------------------*/
#define _XfemTotalDynamicHeight(w) \
(((XfeDynamicManagerWidget) (w))->xfe_dynamic_manager . total_dyn_height)
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Dynamic children count												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfemDynamicChildrenCount(w) \
(_XfemDynamicChildren(w) ? XfeLinkedCount(_XfemDynamicChildren(w)) : 0)
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Dynamic children indexing macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfemDynamicChildrenIndex(w,i) \
(_XfemDynamicChildren(w) ? XfeLinkedItemAtIndex(_XfemDynamicChildren(w),i) : NULL)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager child constraint part access macro					*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeDynamicManagerConstraintPart(w) \
(&(((XfeDynamicManagerConstraintRec *) _XfeConstraints(w)) -> xfe_dynamic_manager))

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDynamicManager child individual constraint resource access macro	*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeDynamicManagerPositionIndex(w) \
(_XfeDynamicManagerConstraintPart(w)) -> position_index
/*----------------------------------------------------------------------*/
#define _XfeDynamicManagerLinkNode(w) \
(_XfeDynamicManagerConstraintPart(w)) -> link_node
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif										/* end DynamicManagerP.h	*/
