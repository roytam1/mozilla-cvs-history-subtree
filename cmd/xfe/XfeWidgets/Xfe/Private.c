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
/* Name:		<Xfe/Private.c>											*/
/* Description:	Xfe widgets private utilities.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include <Xfe/XfeP.h>

#define MESSAGE1 "%s needs to have the same depth as the widget."
#define MESSAGE2 "Cannot obtain geometry info for %s."

/*----------------------------------------------------------------------*/
/*																		*/
/* Callbacks															*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeInvokeCallbacks(Widget			w,
					XtCallbackList	list,
					int				reason,
					XEvent *		event,
					Boolean			flush_display)
{
	/* Make sure widget is alive and callback list is not NULL */
	if (_XfeIsAlive(w) && list)
	{
		XmAnyCallbackStruct cbs;
		
		cbs.event 	= event;
		cbs.reason	= reason;

		/* Flush the display before invoking callback if needed */
		if (flush_display)
		{
			XFlush(XtDisplay(w));
		}

		/* Invoke the Callback List */
		XtCallCallbackList(w,list,&cbs);
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Warnings																*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeSimpleWarning(Widget w,String warning)
{
	static char buf[2048];

	sprintf(buf,
			"\n\t%s: %s\n\t%s: %s\n\t%s\n",
			"Name",XtName(w),
			"Class",w->core.widget_class->core_class.class_name,
			warning);

	XtAppWarning(XtWidgetToApplicationContext(w),buf);
}
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeExtraWarning(Widget		w,
				 String		warning,
				 String		filename,
				 Cardinal	lineno)
{
	static char buf[2048];
	static char line_buf[32];
	static char window_buf[32];

	if (lineno)
	{
		sprintf(line_buf,"%d",lineno);
	}
	else
	{
		sprintf(line_buf,"%s","unknown");
	}

	if (XtIsRealized(w))
	{
		Widget windowed = XfeWindowedWidget(w);

		if (_XfeWindow(windowed) && (_XfeWindow(windowed) != None))
		{
			if (w == windowed)
			{
				sprintf(window_buf,"0x%x",(int) _XfeWindow(windowed));
			}
			else
			{
				sprintf(window_buf,"0x%x (parent's)",(int) _XfeWindow(windowed));
			}
		}
		else
		{
			sprintf(window_buf,"%s","invalid");
		}

		
	}
	else
	{
		sprintf(window_buf,"%s","unrealized");
	}

	sprintf(buf,
			"\n  %-14s %s\n  %-14s %s\n  %-14s %s\n  %-14s %s\n  %-14s %s\n  %s",
			"Filename:",		filename ? filename : "unknown",
			"Line Number:",		line_buf,
			"Widget Name:",		XtName(w),
			"Widget Class:",	w->core.widget_class->core_class.class_name,
			"Widget Window:",	window_buf,
			warning);

	XtAppWarning(XtWidgetToApplicationContext(w),buf);
}
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeArgumentWarning(Widget		w,
					String		format,
					XtPointer	argument,
					String		filename,
					Cardinal	lineno)
{
	static char buf[2048];

	sprintf(buf,format,argument);

	_XfeExtraWarning(w,buf,filename,lineno);
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Actions																*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XtActionProc
_XfeGetActionProc(WidgetClass wc,String name)
{
	XtActionProc	proc = NULL;
	XtActionsRec *	action_list = NULL;
	Cardinal		num_actions;
	Cardinal		i;

	assert( wc != NULL );

	XtGetActionList(wc,&action_list,&num_actions);

	if (action_list && num_actions)
	{
		for(i = 0; i < num_actions; i++)
		{
			if (strcmp(action_list[i].string,name) == 0)
			{
				proc = action_list[i].proc;
			}
		}

		XtFree((char *) action_list);
	}

	return proc;
}
/*----------------------------------------------------------------------*/
