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
/* Name:		<Xfe/ArrowP.h>											*/
/* Description:	XfeArrow widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeArrowP_h_							/* start ArrowP.h		*/
#define _XfeArrowP_h_

#include <Xfe/Arrow.h>
#include <Xfe/ButtonP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeArrowClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtPointer		extension;					/* Extension			*/
} XfeArrowClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeArrowClassRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeArrowClassRec
{
    CoreClassPart				core_class;
    XmPrimitiveClassPart		primitive_class;
    XfePrimitiveClassPart		xfe_primitive_class;
    XfeLabelClassPart			xfe_label_class;
    XfeButtonClassPart			xfe_button_class;
    XfeArrowClassPart			xfe_arrow_class;
} XfeArrowClassRec;

externalref XfeArrowClassRec xfeArrowClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeArrowPart															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeArrowPart
{
    /* Arrow resources */
	unsigned char		arrow_direction;		/* arrow_direction		*/
	Dimension			arrow_width;			/* arrow_width			*/
	Dimension			arrow_height;			/* arrow_height			*/

    /* Private data -- Dont even look past this comment -- */
	GC					arrow_insens_GC;		/* Arrow insens GC		*/

} XfeArrowPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeArrowRec															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeArrowRec
{
    CorePart				core;				/* Core Part			*/
    XmPrimitivePart			primitive;			/* XmPrimitive Part		*/
    XfePrimitivePart		xfe_primitive;		/* XfePrimitive Part	*/
    XfeLabelPart			xfe_label;			/* XfeLabel Part		*/
    XfeButtonPart			xfe_button;			/* XfeButton Part		*/
    XfeArrowPart			xfe_arrow;			/* XfeArrow Part		*/
} XfeArrowRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeArrowPart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeArrowPart(w) &(((XfeArrowWidget) w)->xfe_arrow)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ArrowP.h			*/
