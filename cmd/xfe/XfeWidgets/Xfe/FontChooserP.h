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
/* Name:		<Xfe/FontChooserP.h>									*/
/* Description:	XfeFontChooser widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeFontChooserP_h_						/* start FontChooserP.h	*/
#define _XfeFontChooserP_h_

#include <Xfe/FontChooser.h>
#include <Xfe/CascadeP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadeClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtPointer		extension;					/* Extension			*/
} XfeFontChooserClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFontChooserClassRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFontChooserClassRec
{
    CoreClassPart				core_class;
    XmPrimitiveClassPart		primitive_class;
    XfePrimitiveClassPart		xfe_primitive_class;
    XfeLabelClassPart			xfe_label_class;
    XfeButtonClassPart			xfe_button_class;
    XfeCascadeClassPart			xfe_cascade_class;
    XfeFontChooserClassPart		xfe_font_chooser;
} XfeFontChooserClassRec;

externalref XfeFontChooserClassRec xfeFontChooserClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFontChooserPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFontChooserPart
{
    /* Callback resources */
    XtCallbackList	selection_changed_callback;	/* Selection cb			*/
	
    /* Font item resources */
	Cardinal			num_font_items;			/* Num font items		*/
	XmString *			font_item_labels;		/* Font item labels		*/
	XmFontList *		font_item_fonts;		/* Font item fonts		*/

    /* Private Data Members */
} XfeFontChooserPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFontChooserRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFontChooserRec
{
    CorePart				core;				/* Core Part			*/
    XmPrimitivePart			primitive;			/* XmPrimitive Part		*/
    XfePrimitivePart		xfe_primitive;		/* XfePrimitive Part	*/
    XfeLabelPart			xfe_label;			/* XfeLabel Part		*/
    XfeButtonPart			xfe_button;			/* XfeButton Part		*/
    XfeCascadePart			xfe_cascade;		/* XfeCascade Part		*/
    XfeFontChooserPart		xfe_font_chooser;	/* XfeFontChooser Part	*/
} XfeFontChooserRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFontChooserPart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeFontChooserPart(w) &(((XfeFontChooserWidget) w)->xfe_font_chooser)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end FontChooserP.h	*/
