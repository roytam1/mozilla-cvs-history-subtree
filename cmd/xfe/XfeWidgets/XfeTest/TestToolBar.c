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
/* Name:		<XfeTest/TestToolBar.c>									*/
/* Description:	Xfe widget toolbar test funcs.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

static WidgetClass	type_to_class			(XfeMenuItemType item_type);
static String		type_to_action_cb_name	(XfeMenuItemType item_type);

/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeToolBarItemCreate(Widget pw,XfeMenuItem data,XtPointer client_data)
{
	Widget			item = NULL;
	WidgetClass		wc = NULL;

	assert( XfeIsAlive(pw) );
	assert( XfeIsToolBar(pw) );
	assert( data != NULL );
	assert( data->name != NULL );

	wc = type_to_class(data->type);

	assert( wc != NULL );

	if (client_data)
	{
		data->client_data = client_data;
	}

	if (data->type == XfeMENU_PANE)
	{
		XfeMenuPaneRec pane_data;

		pane_data.name = data->name;
		pane_data.items = data->pane_items;

		item = XfeToolBarPaneCreate(pw,&pane_data,client_data);
	}
	else
	{
		item = XtVaCreateManagedWidget(data->name,wc,pw,NULL);

		if (data->type == XfeMENU_SEP)
		{
			XtVaSetValues(item,
						  XmNshadowThickness,	0,
						  XmNwidth,				20,
						  XmNheight,			5,
						  NULL);
		}
		else if (data->type == XfeMENU_PUSH)
		{
			XtVaSetValues(item,
						  XmNbuttonType,		XmBUTTON_PUSH,
						  NULL);
		}
		else if (data->type == XfeMENU_TOGGLE)
		{
			XtVaSetValues(item,
						  XmNbuttonType,		XmBUTTON_TOGGLE,
						  NULL);
		}

		if (data->action_cb)
		{
			String cb_name = type_to_action_cb_name(data->type);
		
			XtAddCallback(item,cb_name,data->action_cb,data->client_data);
		}
	}

	return item;
}
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeToolBarPaneCreate(Widget pw,XfeMenuPane data,XtPointer client_data)
{
	Widget			cascade = NULL;
	Widget			pane = NULL;
	
	assert( XfeIsAlive(pw) );
	assert( XfeIsToolBar(pw) );
	assert( data != NULL );

	cascade = XtVaCreateManagedWidget(data->name,
									  xfeCascadeWidgetClass,
									  pw,
									  XmNbuttonType,		XmBUTTON_PUSH,
									  XmNmappingDelay,		0,
									  NULL);

	XtVaGetValues(cascade,XmNsubMenuId,&pane,NULL);

	if (data->items)
	{
		XfeMenuItem item = data->items;

		while(item && item->name)
		{
			XfeMenuItemCreate(pane,item,client_data);
			
			item++;
		}
	}
	
	return cascade;
}
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeToolBarCreate(Widget				pw,
				 String				name,
				 XfeMenuPaneRec *	items,
				 XtPointer			client_data)
{
	Widget tool_bar = NULL;
	
	assert( XfeIsAlive(pw) );
	assert( name != NULL );

	tool_bar = XfeCreateToolBar(pw,name,NULL,0);
	
	if (items)
	{
		XfeMenuPane pane = items;

		while(pane && pane->name)
		{
			XfeToolBarPaneCreate(tool_bar,pane,client_data);

			pane++;
		}
	}

	XtManageChild(tool_bar);

	return tool_bar;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
static WidgetClass
type_to_class(XfeMenuItemType item_type)
{
	WidgetClass wc = NULL;

	switch(item_type)
	{
	case XfeMENU_PANE:			wc = xfeCascadeWidgetClass;			break;
	case XfeMENU_PUSH:			wc = xfeButtonWidgetClass;			break;
	case XfeMENU_SEP:			wc = xmSeparatorWidgetClass;		break;
	case XfeMENU_TOGGLE:		wc = xfeButtonWidgetClass;			break;
	default:					wc = NULL;							break;
	}

	return wc;
}
/*----------------------------------------------------------------------*/
static String
type_to_action_cb_name(XfeMenuItemType item_type)
{
	String name = NULL;

	switch(item_type)
	{
	case XfeMENU_PANE:			name = XmNactivateCallback;		break;
	case XfeMENU_PUSH:			name = XmNactivateCallback;		break;
	case XfeMENU_SEP:			name = NULL;					break;
	case XfeMENU_TOGGLE:		name = XmNactivateCallback;		break;
	default:					name = NULL;					break;
	}

	return name;
}
/*----------------------------------------------------------------------*/
