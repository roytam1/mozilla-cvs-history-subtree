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
/* Name:		<Xfe/ClientData.h>										*/
/* Description:	Manager of Arbitrary Client Data for widgets & gadget.	*/
/*																		*/
/*				API for associating arbitrary data with widget and 		*/
/*				gadgets.  The widget/gadget realized, managed or alive	*/
/*				state does not matter.  The association can occur at  	*/
/*				any stage in the widget/gadget life via                 */
/*				XfeClientDataRemove().									*/
/*																		*/
/*				The associated client data will be valid until the 		*/
/*				widget/gadget is destroyed or it is removed via			*/
/*				XfeClientDataRemove().									*/
/*																		*/
/*				A simple garbage collection mechaism is provided via	*/
/*				a callback that is invoked when the widget/gadget is	*/
/*				destroyed (XfeClientDataFreeFunc)						*/
/*																		*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeClientData_h_						/* start ClientData.h	*/
#define _XfeClientData_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeClientDataFreeFunc												*/
/*																		*/
/* This function is invoked when an item is being freed.  This usually	*/
/* happens when the associated widget or gadget is destroyed.			*/
/*																		*/
/* This function can be use to free any memory allocated by the client	*/
/* and stored in the client_data.										*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef void
(*XfeClientDataFreeFunc)		(Widget				w,
								 XtPointer			client_data);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeClientDataKey														*/
/*																		*/
/* All XfeClientData operations are performed with the help of a unique */
/* key.  Use XfeClientGetUniqueKey() to obtain a unique key.            */
/*																		*/
/*----------------------------------------------------------------------*/
typedef int XfeClientDataKey;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeClientDataAdd														*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeClientDataAdd				(Widget					w,
								 XfeClientDataKey		key,
								 XtPointer				client_data,
								 XfeClientDataFreeFunc	free_func);
/*----------------------------------------------------------------------*/
extern XtPointer
XfeClientDataRemove				(Widget					w,
								 XfeClientDataKey		key);
/*----------------------------------------------------------------------*/
extern XtPointer
XfeClientDataGet				(Widget					w,
								 XfeClientDataKey		key);
/*----------------------------------------------------------------------*/
extern void
XfeClientDataSet				(Widget					w,
								 XfeClientDataKey		key,
								 XtPointer				client_data,
								 XfeClientDataFreeFunc	free_func);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeClientGetUniqueKey												*/
/*																		*/
/* Obtain a unique key to use with XfeClientData operations.            */
/*																		*/
/*----------------------------------------------------------------------*/
extern XfeClientDataKey
XfeClientGetUniqueKey			(void);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ClientData.h		*/
