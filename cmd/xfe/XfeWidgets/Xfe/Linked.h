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
/* Name:		<Xfe/Linked.h>											*/
/* Description:	XfeLinked object tweaked to be used in widgets.			*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeLinked_h_							/* start Linked.h		*/
#define _XfeLinked_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLinkedApplyProc type												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef void	(*XfeLinkedApplyProc)		(XtPointer		item,
											 XtPointer		client_data);

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLinkedTestFunc type												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef Boolean	(*XfeLinkedTestFunc)		(XtPointer		item,
											 XtPointer		client_data);
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeLinkedCompareFunc													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef int	(*XfeLinkedCompareFunc)			(XtPointer		one,
											 XtPointer		two,
											 XtPointer		client_data);


typedef struct _XfeLinkedRec *			XfeLinked;
typedef struct _XfeLinkNodeRec *		XfeLinkNode;

/*----------------------------------------------------------------------*/
/*																		*/
/* Public Linked List Functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Cardinal			
XfeLinkedCount					(XfeLinked					list);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedHead					(XfeLinked					list);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedTail					(XfeLinked					list);
/*----------------------------------------------------------------------*/
extern XfeLinked	
XfeLinkedConstruct				(void);
/*----------------------------------------------------------------------*/
extern void				
XfeLinkedDestroy				(XfeLinked					list,
								 XfeLinkedApplyProc			proc,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern void				
XfeLinkedApply					(XfeLinked					list,
								 XfeLinkedApplyProc			proc,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedInsertAfter			(XfeLinked					list,
								 XfeLinkNode				node,
								 XtPointer					item);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedInsertBefore			(XfeLinked					list,
								 XfeLinkNode				node,
								 XtPointer					item);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedInsertAtTail			(XfeLinked					list,
								 XtPointer					item);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedInsertAtHead			(XfeLinked					list,
								 XtPointer					item);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedInsertOrdered			(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XtPointer		
XfeLinkedRemoveNode				(XfeLinked					list,
								 XfeLinkNode				node);
/*----------------------------------------------------------------------*/
extern void				
XfeLinkedClear					(XfeLinked					list,
								 XfeLinkedApplyProc			proc,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindLT					(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindGT					(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindLE					(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindGE					(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindEQ					(XfeLinked					list,
								 XfeLinkedCompareFunc		func,
								 XtPointer					item,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFind					(XfeLinked					list,
								 XfeLinkedTestFunc			func,
								 XtPointer					data);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedFindNodeByItem			(XfeLinked					list,
								 XtPointer					item);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkedNodeAtIndex			(XfeLinked					list,
								 Cardinal					i);
/*----------------------------------------------------------------------*/
extern Boolean			
XfeLinkedPosition				(XfeLinked					list,
								 XtPointer					item,
								 Cardinal *					pos);
/*----------------------------------------------------------------------*/
extern XtPointer
XfeLinkedItemAtIndex			(XfeLinked					list,
								 Cardinal					i);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Public Link Node Functions											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkNodeNext					(XfeLinkNode				node);
/*----------------------------------------------------------------------*/
extern XfeLinkNode		
XfeLinkNodePrev					(XfeLinkNode				node);
/*----------------------------------------------------------------------*/
extern XtPointer		
XfeLinkNodeItem					(XfeLinkNode				node);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Linked.h			*/
