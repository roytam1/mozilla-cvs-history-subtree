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
/* Name:		<Xfe/ProgressBar.h>										*/
/* Description:	XfeProgressBar widget public header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeProgressBar_h_						/* start ProgressBar.h	*/
#define _XfeProgressBar_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeProgressBar resource names										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNbarColor						"barColor"
#define XmNcylonInterval				"cylonInterval"
#define XmNcylonOffset					"cylonOffset"
#define XmNcylonRunning					"cylonRunning"
#define XmNendPercent					"endPercent"
#define XmNstartPercent					"startPercent"
#define XmNcylonWidth					"cylonWidth"

#define XmCBarColor						"BarColor"
#define XmCCylonInterval				"CylonInterval"
#define XmCCylonOffset					"CylonOffset"
#define XmCCylonWidth					"CylonWidth"
#define XmCEndPercent					"EndPercent"
#define XmCStartPercent					"StartPercent"

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeProgressBar class names											*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeProgressBarWidgetClass;
    
typedef struct _XfeProgressBarClassRec *	XfeProgressBarWidgetClass;
typedef struct _XfeProgressBarRec *			XfeProgressBarWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeProgressBar subclass test macro									*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsProgressBar(w)	XtIsSubclass(w,xfeProgressBarWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeProgressBar public functions										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget	
XfeCreateProgressBar			(Widget		w,
								 String		name,
								 ArgList	args,
								 Cardinal	nargs);
/*----------------------------------------------------------------------*/
extern void	
XfeProgressBarSetPercentages	(Widget		w,
								 int		start,
								 int		end);
/*----------------------------------------------------------------------*/
extern void	
XfeProgressBarSetComponents		(Widget		w,
								 XmString	xm_label,
								 int		start,
								 int		end);
/*----------------------------------------------------------------------*/
extern void
XfeProgressBarCylonStart		(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeProgressBarCylonStop			(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeProgressBarCylonReset		(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeProgressBarCylonTick			(Widget		w);
/*----------------------------------------------------------------------*/


XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ProgressBar.h	*/
