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
/* Name:		<Xfe/Button.h>											*/
/* Description:	XfeButton widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeButton_h_							/* start Button.h		*/
#define _XfeButton_h_

#include <Xfe/Label.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton resource names												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNbutton3DownCallback				"button3DownCallback"
#define XmNbutton3UpCallback				"button3UpCallback"

#define XmNarmBackground					"armBackground"
#define XmNarmForeground					"armForeground"
#define XmNarmOffset						"armOffset"
#define XmNarmed							"armed"
#define XmNarmedPixmap						"armedPixmap"
#define XmNarmedPixmapMask					"armedPixmapMask"
#define XmNbuttonLayout						"buttonLayout"
#define XmNbuttonTrigger					"buttonTrigger"
#define XmNdeterminate						"determinate"
#define XmNemulateMotif						"emulateMotif"
#define XmNfillOnEnter						"fillOnEnter"
#define XmNinsensitivePixmap				"insensitivePixmap"
#define XmNinsensitivePixmapMask			"insensitivePixmapMask"
#define XmNpixmapMask						"pixmapMask"
#define XmNraiseBackground					"raiseBackground"
#define XmNraiseForeground					"raiseForeground"
#define XmNraiseOffset						"raiseOffset"
#define XmNraiseOnEnter						"raiseOnEnter"
#define XmNraised							"raised"
#define XmNraisedPixmap						"raisedPixmap"
#define XmNraisedPixmapMask					"raisedPixmapMask"
#define XmNtransparentCursor				"transparentCursor"

#define XmCArmBackground					"ArmBackground"
#define XmCArmForeground					"ArmForeground"
#define XmCArmOffset						"ArmOffset"
#define XmCArmed							"Armed"
#define XmCArmedPixmap						"ArmedPixmap"
#define XmCArmedPixmapMask					"ArmedPixmapMask"
#define XmCButtonLayout						"ButtonLayout"
#define XmCButtonTrigger					"ButtonTrigger"
#define XmCDeterminate						"Determinate"
#define XmCDragButton						"DragButton"
#define XmCEmulateMotif						"EmulateMotif"
#define XmCFillOnEnter						"FillOnEnter"
#define XmCInsensitivePixmap				"InsensitivePixmap"
#define XmCInsensitivePixmapMask			"InsensitivePixmapMask"
#define XmCPixmapMask						"PixmapMask"
#define XmCRaiseBackground					"RaiseBackground"
#define XmCRaiseForeground					"RaiseForeground"
#define XmCRaiseOnEnter						"RaiseOnEnter"
#define XmCRaised							"Raised"
#define XmCRaisedPixmap						"RaisedPixmap"
#define XmCRaisedPixmapMask					"RaisedPixmapMask"
#define XmCUnderlineThickness				"UnderlineThickness"

#define XmRButtonLayout						"ButtonLayout"
#define XmRButtonTrigger					"ButtonTrigger"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRButtonType														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmBUTTON_NONE,								/*						*/
    XmBUTTON_PUSH,								/*						*/
    XmBUTTON_TOGGLE								/*						*/
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRButtonLayout														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmBUTTON_LABEL_ONLY,
    XmBUTTON_LABEL_ON_BOTTOM,
    XmBUTTON_LABEL_ON_LEFT,
    XmBUTTON_LABEL_ON_RIGHT,
    XmBUTTON_LABEL_ON_TOP,
    XmBUTTON_PIXMAP_ONLY
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRButtonTrigger														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
	XmBUTTON_TRIGGER_ANYWHERE,
    XmBUTTON_TRIGGER_LABEL,
    XmBUTTON_TRIGGER_PIXMAP,
    XmBUTTON_TRIGGER_EITHER,
    XmBUTTON_TRIGGER_NEITHER
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Button callback structure											*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
    int			reason;					/* Reason why CB was invoked	*/
    XEvent *	event;					/* Event that triggered CB		*/
    Boolean		armed;					/* Button armed ?				*/
    Boolean		raised;					/* Button raised ?				*/
} XfeButtonCallbackStruct;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton class names												*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeButtonWidgetClass;
    
typedef struct _XfeButtonClassRec *	XfeButtonWidgetClass;
typedef struct _XfeButtonRec *		XfeButtonWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton subclass test macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsButton(w)	XtIsSubclass(w,xfeButtonWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton public functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateButton				(Widget		parent,
							 String		name,
							 Arg *		args,
							 Cardinal	num_args);
/*----------------------------------------------------------------------*/
extern void
XfeButtonPreferredGeometry	(Widget				w,
							 unsigned char		layout,
							 Dimension *		width_out,
							 Dimension *		height_out);
/*----------------------------------------------------------------------*/
extern Boolean
XfeButtonAcceptXY			(Widget				w,
							 int				x,
							 int				y);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeButton Rep type registration function								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeButtonRegisterRepTypes		(void);

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Button.h			*/
