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
/* Name:		<Xfe/ChildrenUtil.h>									*/
/* Description:	Children misc utilities header.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeChildrenUtil_h_						/* start ChildrenUtil.h	*/
#define _XfeChildrenUtil_h_

#include <X11/Intrinsic.h>						/* Xt public defs		*/

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* Children access														*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Cardinal		XfeNumChildren			(Widget w);
extern WidgetList	XfeChildren				(Widget w);
extern Widget		XfeChildrenIndex		(Widget w,Cardinal i);
extern Cardinal		XfeChildrenCountAlive	(Widget w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Popup children access												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Cardinal		XfeNumPopups			(Widget w);
extern WidgetList	XfePopupList			(Widget w);
extern Widget		XfePopupListIndex		(Widget w,Cardinal i);
extern Cardinal		XfePopupListCountAlive	(Widget w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Children access														*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeChildrenGet				(Widget			w,
							 WidgetList *	children,
							 Cardinal *		num_children);
/*----------------------------------------------------------------------*/
extern Cardinal
XfeChildrenGetNumManaged	(Widget			w);
/*----------------------------------------------------------------------*/
extern Widget
XfeChildrenGetLast			(Widget			w);
/*----------------------------------------------------------------------*/
extern int
XfeChildGetIndex			(Widget			w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Destroy																*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeChildrenDestroy			(Widget			w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Callbacks / Event handlers											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeChildrenAddCallback		(Widget			w,
							 String			callback_name,
							 XtCallbackProc	callback,
							 XtPointer		data);
/*----------------------------------------------------------------------*/
extern void
XfeChildrenAddEventHandler	(Widget			w,
							 EventMask		event_mask,
							 Boolean		nonmaskable,
							 XtEventHandler	proc,
							 XtPointer		data);
/*----------------------------------------------------------------------*/
extern void
XfeChildrenRemoveCallback	(Widget			w,
							 String			callback_name,
							 XtCallbackProc	callback,
							 XtPointer		data);
/*----------------------------------------------------------------------*/
extern void
XfeChildrenRemoveEventHandler(Widget			w,
							  EventMask			event_mask,
							  Boolean			nonmaskable,
							  XtEventHandler	proc,
							  XtPointer			data);
/*----------------------------------------------------------------------*/



XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ChildrenUtil.h	*/
