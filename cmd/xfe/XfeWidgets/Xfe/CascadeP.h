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
/* Name:		<Xfe/CascadeP.h>										*/
/* Description:	XfeCascade widget private header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeCascadeP_h_							/* start CascadeP.h		*/
#define _XfeCascadeP_h_

#include <Xfe/Cascade.h>
#include <Xfe/ButtonP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
   
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadeClassPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    XtPointer		extension;					/* Extension			*/
} XfeCascadeClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadeClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCascadeClassRec
{
    CoreClassPart				core_class;
    XmPrimitiveClassPart		primitive_class;
    XfePrimitiveClassPart		xfe_primitive_class;
    XfeLabelClassPart			xfe_label_class;
    XfeButtonClassPart			xfe_button_class;
    XfeCascadeClassPart			xfe_cascade_class;
} XfeCascadeClassRec;

externalref XfeCascadeClassRec xfeCascadeClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadePart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCascadePart
{
    /* Callback resources */
    XtCallbackList		popdown_callback;		/* Popdown cb			*/
    XtCallbackList		popup_callback;			/* Popup cb				*/
    XtCallbackList		cascading_callback;		/* Cascading cb			*/
    XtCallbackList		submenu_tear_callback;	/* Tear submenu cb		*/

    /* Sub menu resources */
	Widget				sub_menu_id;			/* Sub menu id			*/
	int					mapping_delay;			/* Mapping delay		*/
    unsigned char		sub_menu_alignment;		/* Sub menu alignment	*/
    unsigned char		sub_menu_location;		/* Sub menu location	*/
	Boolean				popped_up;				/* Popped up ?			*/
	Boolean				match_sub_menu_width;	/* Match sub menu width?*/

	/* Cascade arrow resources */
	unsigned char		cascade_arrow_direction;/* Arrow direction		*/
	unsigned char		cascade_arrow_location;	/* Arrow location		*/
	Boolean				draw_cascade_arrow;		/* Draw cascade arrow ? */

	/* Tear resources */
	Boolean				torn;					/* Torn ?				*/
	Boolean				allow_tear_off;			/* Allow tear off		*/
	String				torn_shell_title;		/* Torn shell title		*/

    /* Private Data Members */
	XtIntervalId		delay_timer_id;			/* Delay timer id		*/
	Cursor				default_menu_cursor;	/* Default menu cursor	*/
	XRectangle			cascade_arrow_rect;		/* Cascade arrow rect	*/
} XfeCascadePart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadeRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeCascadeRec
{
    CorePart				core;				/* Core Part			*/
    XmPrimitivePart			primitive;			/* XmPrimitive Part		*/
    XfePrimitivePart		xfe_primitive;		/* XfePrimitive Part	*/
    XfeLabelPart			xfe_label;			/* XfeLabel Part		*/
    XfeButtonPart			xfe_button;			/* XfeButton Part		*/
    XfeCascadePart			xfe_cascade;		/* XfeCascade Part		*/
} XfeCascadeRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCascadePart Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeCascadePart(w) &(((XfeCascadeWidget) w)->xfe_cascade)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end CascadeP.h		*/
