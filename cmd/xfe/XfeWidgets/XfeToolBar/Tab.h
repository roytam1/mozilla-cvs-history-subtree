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
/* Name:		<Xfe/Tab.h>												*/
/* Description:	XfeTab widget public header file.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeTab_h_								/* start Tab.h			*/
#define _XfeTab_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTab resource names												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNbottomPixmap					"bottomPixmap"
#define XmNbottomRaisedPixmap			"bottomRaisedPixmap"
#define XmNhorizontalPixmap				"horizontalPixmap"
#define XmNhorizontalRaisedPixmap		"horizontalRaisedPixmap"
#define XmNleftPixmap					"leftPixmap"
#define XmNleftRaisedPixmap				"leftRaisedPixmap"
#define XmNrightPixmap					"rightPixmap"
#define XmNrightRaisedPixmap			"rightRaisedPixmap"
#define XmNtopPixmap					"topPixmap"
#define XmNtopRaisedPixmap				"topRaisedPixmap"
#define XmNverticalPixmap				"verticalPixmap"
#define XmNverticalRaisedPixmap			"verticalRaisedPixmap"

#define XmCBottomPixmap					"BottomPixmap"
#define XmCBottomRaisedPixmap			"BottomRaisedPixmap"
#define XmCHorizontalPixmap				"HorizontalPixmap"
#define XmCHorizontalRaisedPixmap		"HorizontalRaisedPixmap"
#define XmCLeftPixmap					"LeftPixmap"
#define XmCLeftRaisedPixmap				"LeftRaisedPixmap"
#define XmCRightPixmap					"RightPixmap"
#define XmCRightRaisedPixmap			"RightRaisedPixmap"
#define XmCTopPixmap					"TopPixmap"
#define XmCTopRaisedPixmap				"TopRaisedPixmap"
#define XmCVerticalPixmap				"VerticalPixmap"
#define XmCVerticalRaisedPixmap			"VerticalRaisedPixmap"

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTab class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeTabWidgetClass;
    
typedef struct _XfeTabClassRec *	XfeTabWidgetClass;
typedef struct _XfeTabRec *			XfeTabWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTab subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsTab(w)	XtIsSubclass(w,xfeTabWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTab public functions												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateTab				(Widget		pw,
							 String		name,
							 Arg *		av,
							 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern void
XfeTabDrawRaised			(Widget		w,
							 Boolean	raised);
/*----------------------------------------------------------------------*/
extern unsigned char
XfeTabGetOrientation		(Widget		w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Tab.h		*/
