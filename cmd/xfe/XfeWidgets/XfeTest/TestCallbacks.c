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
/* Name:		<XfeTest/TestCallbacks.c>								*/
/* Description:	Xfe widget tests callbacks.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

/*----------------------------------------------------------------------*/
/*extern */ void
XfeActivateCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Activate(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeArmCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Arm(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeDisarmCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Disarm(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeLowerCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Lower(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeRaiseCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Raise(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeEnterCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Enter(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeLeaveCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Leave(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeMapCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Map(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeUnmapCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Unmap(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeDockCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Dock(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeUndockCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Undock(%s)\n\n",XtName(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeExitCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	printf("Exit(%s)\n\n",XtName(w));

	XtCloseDisplay(XtDisplay(w));

	XtDestroyWidget(XfeAncestorFindApplicationShell(w));

	exit(0);
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeDestroyCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	Widget target = (Widget) client_data;

	printf("Destroy(%s)\n\n",XtName(target));

	XtDestroyWidget(target);
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeResizeCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	assert( XfeIsAlive(w) );

	printf("Resize(%s) to (%d,%d)\n\n",XtName(w),XfeWidth(w),XfeHeight(w));
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeLabelSelectionChangedCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	XfeLabelSelectionChangedCallbackStruct * data = 
		(XfeLabelSelectionChangedCallbackStruct *) call_data;

	assert( XfeIsAlive(w) );

	printf("SelectionChanged(%s,%s)\n\n",
		   XtName(w),data->selected ? "True" : "False");
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeFreeDataCallback(Widget w,XtPointer client_data,XtPointer call_data)
{
	if (client_data)
	{
		XtFree((char *) client_data);
	}
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeToolBoxOpenCallback(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;
	
	printf("XfeToolBoxOpenCallback(tb = %s,item = %s,i = %d)\n",
		   XtName(w),
		   XtName(cbs->item),
		   cbs->index);
}
/*----------------------------------------------------------------------*/
/*extern */ void
XfeToolBoxCloseCallback(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;
	
	printf("XfeToolBoxCloseCallback(tb = %s,item = %s,i = %d)\n",
		   XtName(w),
		   XtName(cbs->item),
		   cbs->index);
}
/*----------------------------------------------------------------------*/
