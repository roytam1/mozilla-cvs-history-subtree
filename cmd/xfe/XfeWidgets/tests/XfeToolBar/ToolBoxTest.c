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
/* Name:		ToolBoxTest.c											*/
/* Description:	Test for XfeToolBox widget.								*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static Widget	create_tool_item		(Widget,String,Cardinal);
static void		close_cb				(Widget,XtPointer,XtPointer);
static void		open_cb				(Widget,XtPointer,XtPointer);
static void		new_item_cb				(Widget,XtPointer,XtPointer);
static void		snap_cb					(Widget,XtPointer,XtPointer);
static void		swap_cb					(Widget,XtPointer,XtPointer);

static void		button_cb				(Widget,XtPointer,XtPointer);


static String tool_names[] = 
{
	"red",
	"yellow",
	"green",
#if 1
	"blue",
	"orange",
	"purple",
	"pink",
	"aliceblue"
#endif
};


static int tool_heights[] = 
{
	2,
	4,
	6,
	8,
	10,
	12,
	10,
	8
};

#define NUM_TOOLS XtNumber(tool_names)

static Widget tool_widgets[NUM_TOOLS];

/*----------------------------------------------------------------------*/
int
main(int argc,char *argv[])
{
	Widget	form;
	Widget	frame;

    Widget		tb;
	Cardinal	i;

	XfeAppCreateSimple("ToolBoxTest",&argc,argv,"MainFrame",&frame,&form);
    
    tb = XfeCreateLoadedToolBox(form,"ToolBox",NULL,0);

	for (i = 0; i < NUM_TOOLS; i++)
	{
		tool_widgets[i] = create_tool_item(tb,
										   tool_names[i],
										   tool_heights[i]);
	}

	XtPopup(frame,XtGrabNone);

    XfeAppMainLoop();

	return 0;
}
/*----------------------------------------------------------------------*/
static Widget
create_tool_item(Widget parent,String name,Cardinal nitems)
{
    Widget		tool_item;
    Widget		tool_bar;
	Widget		logo;

    tool_item = 
		XtVaCreateManagedWidget(name,
								xfeToolItemWidgetClass,
								parent,
								XmNusePreferredHeight,	True,
								XmNusePreferredWidth,	False,
								XmNbackground,			XfeGetPixel(name),
								NULL);

    logo = 
		XtVaCreateManagedWidget("Logo",
								xfeLogoWidgetClass,
								tool_item,
								XmNbackground,			XfeGetPixel(name),
								NULL);
	
    tool_bar = XfeCreateLoadedToolBar(tool_item,
									  "ToolBar",
									  "Item",
									  nitems,
									  nitems / 10,
									  XfeArmCallback,
									  XfeDisarmCallback,
									  XfeActivateCallback,
									  NULL);

	XtVaSetValues(tool_bar,XmNbackground,XfeGetPixel(name),NULL);
	
	XfeToolBoxAddDragDescendant(parent,tool_item);
	XfeToolBoxAddDragDescendant(parent,tool_bar);
	XfeToolBoxAddDragDescendant(parent,logo);

    return tool_item;
}
/*----------------------------------------------------------------------*/
static void
snap_cb(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;

	printf("snap      (%s at %d)\n",XtName(cbs->item),cbs->index);
}
/*----------------------------------------------------------------------*/
static void
new_item_cb(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;

	printf("new_item  (%s at %d)\n",XtName(cbs->item),cbs->index);
}
/*----------------------------------------------------------------------*/
static void
close_cb(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;

	printf("close  (%s at %d)\n",XtName(cbs->item),cbs->index);
}
/*----------------------------------------------------------------------*/
static void
open_cb(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxCallbackStruct *	cbs = (XfeToolBoxCallbackStruct *) call_data;

	printf("open    (%s at %d)\n",XtName(cbs->item),cbs->index);
}
/*----------------------------------------------------------------------*/
static void
swap_cb(Widget w,XtPointer clien_data,XtPointer call_data)
{
	XfeToolBoxSwapCallbackStruct *	cbs = 
		(XfeToolBoxSwapCallbackStruct *) call_data;
	
	printf("swap      (%s with %s from %d to %d)\n",
		   XtName(cbs->swapped),XtName(cbs->displaced),
		   cbs->from_index,cbs->to_index);
}
/*----------------------------------------------------------------------*/
