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
/* Name:		<XfeTest/TestApp.c>										*/
/* Description:	Xfe widget tests stuff.									*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>
#include <Xfe/Chrome.h>

#include <X11/Xmu/Editres.h>
#include <X11/Xmu/Converters.h>

#include <X11/IntrinsicP.h>

static XtConvertArgRec parentCvtArg[] = 
{
	{
		XtWidgetBaseOffset,
		(XtPointer) XtOffsetOf(WidgetRec , core . parent),
		sizeof(Widget)
	}
};

/*----------------------------------------------------------------------*/
static XtAppContext		_xfe_app_context = NULL;
static Widget			_xfe_app_shell = NULL;
/*----------------------------------------------------------------------*/

extern char * fallback_resources[];

/*----------------------------------------------------------------------*/
/* extern */ void
XfeAppCreate(char * app_name,int * argc,String * argv)
{
	char * pc;

	assert( _xfe_app_context == NULL );
	assert( _xfe_app_shell == NULL );

	pc = strstr(argv[0],".shared");

	if (!pc)
	{
		pc = strstr(argv[0],".static");
	}

	if (pc != NULL)
	{
		*pc = '\0';
	}

    _xfe_app_shell = XtAppInitialize(&_xfe_app_context,
									 app_name,
									 NULL,0,
									 argc,argv,
									 fallback_resources,
									 NULL,0);

	XtVaSetValues(_xfe_app_shell,
  				  XmNmappedWhenManaged,		False,
				  XmNx,						XfeScreenWidth(_xfe_app_shell)/2,
				  XmNy,						XfeScreenHeight(_xfe_app_shell)/2,
				  XmNwidth,					1,
				  XmNheight,				1,
				  NULL);

    XtRealizeWidget(_xfe_app_shell);

/*  	XfeAddEditresSupport(_xfe_app_shell); */

	XfeRegisterStringToWidgetCvt();
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeAppCreateSimple(char *		app_name,
				   int *		argc,
				   String *		argv,
				   char *		frame_name,
				   Widget *		frame_out,
				   Widget *		form_out)
{
	assert( _xfe_app_context == NULL );
	assert( _xfe_app_shell == NULL );
	assert( frame_out != NULL );
	assert( form_out != NULL );

	XfeAppCreate(app_name,argc,argv);

	*frame_out = XfeFrameCreate(frame_name,NULL,0);

    *form_out = XfeDescendantFindByName(*frame_out,
										"MainForm",
										XfeFIND_ANY,False);
}
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeFrameCreate(char * frame_name,ArgList args,Cardinal num_args)
{
	Widget frame;
	Widget main_form;

	assert( _xfe_app_context != NULL );
	assert( XfeIsAlive(_xfe_app_shell) );

	frame = XtVaCreatePopupShell(frame_name,
								 xfeFrameShellWidgetClass,
								 _xfe_app_shell,
								 NULL);
	
	main_form = XfeCreateManagedForm(frame,"MainForm",NULL,0);

  	XfeAddEditresSupport(frame);

	return frame;
}
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeFrameCreateWithChrome(char * frame_name,ArgList args,Cardinal num_args)
{
	Widget frame;
	Widget chrome;

	assert( _xfe_app_context != NULL );
	assert( XfeIsAlive(_xfe_app_shell) );

	frame = XtVaCreatePopupShell(frame_name,
								 xfeFrameShellWidgetClass,
								 _xfe_app_shell,
								 NULL);
	
    chrome = XtVaCreateManagedWidget("Chrome",
									 xfeChromeWidgetClass,
									 frame,
									 NULL);

  	XfeAddEditresSupport(frame);

	return frame;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeAddEditresSupport(Widget shell)
{
	assert( XfeIsAlive(shell) );
	assert( XtIsShell(shell) );

    XtAddEventHandler(shell,
					  (EventMask) 0,
					  True,
					  (XtEventHandler) _XEditResCheckMessages,
					  NULL);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeRegisterStringToWidgetCvt(void)
{
	XtSetTypeConverter(XtRString,
					   XtRWidget,
					   XmuNewCvtStringToWidget,
					   parentCvtArg,
					   XtNumber(parentCvtArg),
					   XtCacheNone,
					   NULL);
}
/*----------------------------------------------------------------------*/
/* extern */ XtAppContext
XfeAppContext(void)
{
	assert( _xfe_app_context != NULL );

	return _xfe_app_context;
}
/*----------------------------------------------------------------------*/
/* extern */ Widget
XfeAppShell(void)
{
	assert( _xfe_app_shell != NULL );

	return _xfe_app_shell;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeAppMainLoop(void)
{
	assert( _xfe_app_context != NULL );

	XtRealizeWidget(_xfe_app_shell);

    XtAppMainLoop(_xfe_app_context);
}
/*----------------------------------------------------------------------*/
/* extern */ XtIntervalId
XfeAppAddTimeOut(unsigned long			interval,
				 XtTimerCallbackProc	proc,
				 XtPointer				client_data)
{
	return XtAppAddTimeOut(XfeAppContext(),interval,proc,client_data);
}
/*----------------------------------------------------------------------*/
