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
/* Name:		<Xfe/Caption.h>											*/
/* Description:	XfeCaption widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeCaption_h_							/* start Caption.h		*/
#define _XfeCaption_h_

#include <Xfe/Manager.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaption resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNcaptionLayout				"captionLayout"
#define XmNchild						"child"
#define XmNchildResize					"childResize"
#define XmNmaxChildHeight				"maxChildHeight"
#define XmNmaxChildWidth				"maxChildWidth"
#define XmNtitleDirection				"titleDirection"
#define XmNtitleHorizontalAlignment		"TitleHorizontalAlignment"
#define XmNtitleVerticalAlignment		"TitleVerticalAlignment"

#define XmCCaptionHorizontalAlignment	"CaptionHorizontalAlignment"
#define XmCCaptionLayout				"CaptionLayout"
#define XmCCaptionVerticalAlignment		"CaptionVerticalAlignment"
#define XmCChild						"Child"
#define XmCChildResize					"ChildResize"
#define XmCMaxChildHeight				"MaxChildHeight"
#define XmCMaxChildWidth				"MaxChildWidth"
#define XmCTitleDirection				"TitleDirection"

#define XmRCaptionHorizontalAlignment	"CaptionHorizontalAlignment"
#define XmRCaptionLayout				"CaptionLayout"
#define XmRCaptionVerticalAlignment		"CaptionVerticalAlignment"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRCaptionLayout														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmCAPTION_CHILD_ON_BOTTOM,
    XmCAPTION_CHILD_ON_LEFT,
    XmCAPTION_CHILD_ON_RIGHT,
    XmCAPTION_CHILD_ON_TOP
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRHorizontalAlignment												*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmCAPTION_HORIZONTAL_ALIGNMENT_LEFT,
    XmCAPTION_HORIZONTAL_ALIGNMENT_CENTER,
    XmCAPTION_HORIZONTAL_ALIGNMENT_RIGHT
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRVerticalAlignment													*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmCAPTION_VERTICAL_ALIGNMENT_BOTTOM,
    XmCAPTION_VERTICAL_ALIGNMENT_CENTER,
    XmCAPTION_VERTICAL_ALIGNMENT_TOP
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Caption max dimensions callback structure							*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
	Dimension	max_child_width;		/* Max child width				*/
	Dimension	max_child_height;		/* Max child height				*/
} XfeCaptionMaxDimensionsCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeCaptionWidgetClass;

typedef struct _XfeCaptionClassRec *		XfeCaptionWidgetClass;
typedef struct _XfeCaptionRec *				XfeCaptionWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsCaption(w)	XtIsSubclass(w,xfeCaptionWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeCaption public methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateCaption				(Widget		pw,
								 String		name,
								 Arg *		av,
								 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern Dimension
XfeCaptionMaxChildWidth			(Widget		w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Caption.h		*/
