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
/* Name:		<Xfe/ToolTip.h>											*/
/* Description:	XfeToolTip - TipString / DocString support.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolTip_h_							/* start ToolTip.h		*/
#define _XfeToolTip_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTip resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNdocumentationString			"documentationString"
#define XmNtipString					"tipString"

#define XmCDocumentationString			"DocumentationString"
#define XmCTipString					"TipString"

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTipStringObtainCallback											*/
/*																		*/
/* This callback is invoked when an item that has tip string support is	*/
/* ready to post a tooltip.  An item is ready to post a tooltip right	*/
/* after the following two events occur in am immediate sequence:		*/
/*																		*/
/* 1.  The pointer Enters the item.										*/
/* 2.  A timeout expires without an intervening cancellation.			*/
/*																		*/
/* A cancellation occurs when:											*/
/*																		*/
/* 1.  The pointer leaves the item before the timeout expires.			*/
/* 2.  The item receives a Button or KetPress event.					*/
/*																		*/
/* This callback should return the following:							*/
/*																		*/ 
/* An XmString in 'string_return'										*/
/* 	   	   	   															*/
/* Should that string should be freed after use in 'need_to_free_string'*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef void
(*XfeTipStringObtainCallback)	(Widget				w,
								 XtPointer			client_data,
								 XmString *			string_return,
								 Boolean *			need_to_free_string);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Callback Reasons														*/
/*																		*/
/* The 'reason' given to the XfeDocStringCallback callaback below.		*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XfeDOC_STRING_CLEAR,							/* Clear (leave)	*/
	XfeDOC_STRING_SET								/* Set (enter)		*/
};

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDocStringCallback													*/
/*																		*/
/* This callback is invoked when the pointer enters an item that has	*/
/* doc string support and a valid doc string is obtained.				*/
/*																		*/
/* The XfeTipStringObtainCallback callback can be installed by			*/
/* XfeDocStringSetObtainCallback().										*/
/*																		*/
/* You can use XfeDocStringGetFromAppDefaults() in your callback to 	*/
/* obtain a string for a resource named XmNdocumentationString.			*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef void
(*XfeDocStringCallback)			(Widget					w,
								 XtPointer				client_data,
								 unsigned char			reason,
								 XmString				string);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* TipString public methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeTipStringAdd						(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeTipStringRemove					(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeTipStringSetEnabledState			(Widget		w,
									 Boolean	state);
/*----------------------------------------------------------------------*/
extern Boolean
XfeTipStringGetEnabledState			(Widget		w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* TipString callback functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeTipStringSetObtainCallback	(Widget							w,
								 XfeTipStringObtainCallback		callback,
								 XtPointer						client_data);
/*----------------------------------------------------------------------*/
extern void
XfeTipStringClearObtainCallback	(Widget							w);

/*----------------------------------------------------------------------*/
/*																		*/
/* TipString global enabled / disable functions							*/
/*																		*/
/* Enable and disable tip strings on a global basis.  You can use these	*/
/* functions to diable tip strings everywhere.  The individual enabled	*/
/* state of items with tip string support is not affected by these		*/
/* functions.															*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void			
XfeTipStringGlobalSetEnabledState		(Boolean state);
/*----------------------------------------------------------------------*/
extern Boolean
XfeTipStringGlobalGetEnabledState		(void);

/*----------------------------------------------------------------------*/
/*																		*/
/* Check whether the global tooltip is showing							*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
XfeTipStringIsShowing					(void);

/*----------------------------------------------------------------------*/
/*																		*/
/* DocString public methods												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeDocStringAdd						(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeDocStringRemove					(Widget		w);
/*----------------------------------------------------------------------*/
extern void
XfeDocStringSetEnabledState			(Widget		w,
									 Boolean	state);
/*----------------------------------------------------------------------*/
extern Boolean
XfeDocStringGetEnabledState			(Widget		w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* DocString callback functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeDocStringSetObtainCallback		(Widget						w,
									 XfeTipStringObtainCallback	callback,
									 XtPointer					client_data);
/*----------------------------------------------------------------------*/
extern void
XfeDocStringClearObtainCallback		(Widget						w);
/*----------------------------------------------------------------------*/
extern void
XfeDocStringSetCallback				(Widget						w,
									 XfeDocStringCallback		callback,
									 XtPointer					client_data);
/*----------------------------------------------------------------------*/
extern void
XfeDocStringClearCallback			(Widget						w);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* DocString global enabled / disable functions							*/
/*																		*/
/* Enable and disable doc strings on a global basis.  You can use these	*/
/* functions to diable doc strings everywhere.  The individual enabled	*/
/* state of items with doc string support is not affected by these		*/
/* functions.															*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void			
XfeDocStringGlobalSetEnabledState	(Boolean state);
/*----------------------------------------------------------------------*/
extern Boolean
XfeDocStringGlobalGetEnabledState	(void);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDocStringGetFromAppDefaults()										*/
/*																		*/
/* Obtain an XmString from application defaults for the resource named	*/
/* "documentationString"												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern XmString
XfeDocStringGetFromAppDefaults		(Widget					w);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeTipStringGetFromAppDefaults()										*/
/*																		*/
/* Obtain an XmString from application defaults for the resource named	*/
/* "tipString"															*/
/*																		*/
/*----------------------------------------------------------------------*/
extern XmString
XfeTipStringGetFromAppDefaults		(Widget					w);

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolTip.h		*/
