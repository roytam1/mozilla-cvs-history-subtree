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
/* Name:		<Xfe/CaptionP.h>										*/
/* Description:	XfeCaption widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeCaptionP_h_							/* start CaptionP.h		*/
#define _XfeCaptionP_h_

#include <Xfe/Caption.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaption Widget method inheritance macros							*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeInheritCaptionActivate		((XtWidgetProc)			_XtInherit)
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaptionClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtWidgetProc		activate;					/* activate			*/
	XtPointer			extension;					/* extension		*/ 
} XfeCaptionClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaptionClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCaptionClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
    XfeCaptionClassPart			xfe_caption_class;
} XfeCaptionClassRec;

externalref XfeCaptionClassRec xfeCaptionClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaptionPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCaptionPart
{
	/* Layout resources */
	unsigned char		caption_layout;			/* Caption layout		*/
	Dimension			spacing;				/* Spacing				*/

	/* Title resources */
	Widget				title;					/* Title				*/
	unsigned char		title_hor_alignment;	/* Title hor alignment	*/
	unsigned char		title_ver_alignment;	/* Title ver alignment	*/
    unsigned char		title_direction;		/* Title direction		*/
	XmFontList			title_font_list;		/* Title font list		*/
	XmString			title_string;			/* Title string			*/

	/* Child resources */
	Widget				child;					/* Child				*/
	unsigned char		child_hor_alignment;	/* Child hor alignment	*/
	unsigned char		child_ver_alignment;	/* Child ver alignment	*/
	Boolean				child_resize;			/* Child Resize			*/

	/* Dimension resources */
	unsigned char		max_child_method;		/* Max child method		*/
	Dimension			max_child_width;		/* Max child width		*/
	Dimension			max_child_height;		/* Max child height		*/

    /* Private data -- Dont even look past this comment -- */

} XfeCaptionPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaptionRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCaptionRec
{
    CorePart				core;
    CompositePart			composite;
    ConstraintPart			constraint;
    XmManagerPart			manager;
    XfeManagerPart			xfe_manager;
    XfeCaptionPart			xfe_caption;
} XfeCaptionRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaptionPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeCaptionPart(w) &(((XfeCaptionWidget) w) -> xfe_caption)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaption Method invocation functions								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeCaptionActivate				(Widget			w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end CaptionP.h		*/

