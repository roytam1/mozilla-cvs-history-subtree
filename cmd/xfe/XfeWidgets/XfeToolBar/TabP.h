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
/* Name:		<Xfe/TabP.h>											*/
/* Description:	XfeTab widget private header file.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeTabP_h_								/* start TabP.h			*/
#define _XfeTabP_h_

#include <Xfe/Tab.h>
#include <Xfe/ButtonP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTabClassPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtPointer		extension;					/* Extension			*/
} XfeTabClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTabClassRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeTabClassRec
{
    CoreClassPart				core_class;
    XmPrimitiveClassPart		primitive_class;
    XfePrimitiveClassPart		xfe_primitive_class;
    XfeLabelClassPart			xfe_label_class;
    XfeButtonClassPart			xfe_button_class;
    XfeTabClassPart				xfe_tab_class;
} XfeTabClassRec;

externalref XfeTabClassRec xfeTabClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTabPart															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeTabPart
{
    /* Resources */
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

	unsigned char		orientation;			/* Orientation			*/

    /* Private Data Members */
	Dimension			bottom_width;			/* Bottom width			*/
	Dimension			bottom_height;			/* Bottom height		*/
	Dimension			horizontal_width;		/* Horizontal width		*/
	Dimension			horizontal_height;		/* Horizontal height	*/
	Dimension			left_width;				/* Left width			*/
	Dimension			left_height;			/* Left height			*/
	Dimension			right_width;			/* Right width			*/
	Dimension			right_height;			/* Right height			*/
	Dimension			top_width;				/* Top width			*/
	Dimension			top_height;				/* Top height			*/
	Dimension			vertical_width;			/* Vertical width		*/
	Dimension			vertical_height;		/* Vertical height		*/

} XfeTabPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTabRec															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeTabRec
{
    CorePart				core;				/* Core Part			*/
    XmPrimitivePart			primitive;			/* XmPrimitive Part		*/
    XfePrimitivePart		xfe_primitive;		/* XfePrimitive Part	*/
    XfeLabelPart			xfe_label;			/* XfeLabel Part		*/
    XfeButtonPart			xfe_button;			/* XfeButton Part		*/
    XfeTabPart				xfe_tab;			/* XfeTab Part			*/
} XfeTabRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTabPart Access Macro												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeTabPart(w) &(((XfeTabWidget) w)->xfe_tab)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTab - superclass = XfeButton										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XFE_PREPARE_TAB_BOTTOM_PIXMAP				XfePrepare6
#define _XFE_PREPARE_TAB_HORIZONTAL_PIXMAP			XfePrepare7
#define _XFE_PREPARE_TAB_LEFT_PIXMAP				XfePrepare8
#define _XFE_PREPARE_TAB_RIGHT_PIXMAP				XfePrepare9
#define _XFE_PREPARE_TAB_TOP_PIXMAP					XfePrepare10
#define _XFE_PREPARE_TAB_VERTICAL_PIXMAP			XfePrepare11
#define _XFE_PREPARE_TAB_BOTTOM_RAISED_PIXMAP		XfePrepare12
#define _XFE_PREPARE_TAB_HORIZONTAL_RAISED_PIXMAP	XfePrepare13
#define _XFE_PREPARE_TAB_LEFT_RAISED_PIXMAP			XfePrepare14
#define _XFE_PREPARE_TAB_RIGHT_RAISED_PIXMAP		XfePrepare15
#define _XFE_PREPARE_TAB_TOP_RAISED_PIXMAP			XfePrepare16
#define _XFE_PREPARE_TAB_VERTICAL_RAISED_PIXMAP		XfePrepare17

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end TabP.h		*/
