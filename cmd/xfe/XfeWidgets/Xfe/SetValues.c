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
/* Name:		<Xfe/SetValues.c>										*/
/* Description:	XfeManager SetValues() procedures.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/ManagerP.h>
#include <Xfe/PrimitiveP.h>

#define MESSAGE0 "XfeManager is an abstract class and cannot be instanciated."

/*----------------------------------------------------------------------*/
/*																		*/
/* Apply function client data struct									*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	String		resource_name;
	XtArgVal	resource_value;
} _ApplyDataRec;

#define SET_APPLY_DATA(_d,_n,_v) \
  (_d) . resource_name  = (_n); \
  (_d) . resource_value = (_v);

/*----------------------------------------------------------------------*/
/*																		*/
/* Children apply client procedures										*/
/*																		*/
/*----------------------------------------------------------------------*/
static void		ApplyChildSetValue		(Widget,Widget,XtPointer);
static void		ApplyChildSetSensitive	(Widget,Widget,XtPointer);

/*----------------------------------------------------------------------*/
/*																		*/
/* _XfeManagerPropagateSetValues()										*/
/*																		*/
/* Used to propagate common SetValues() changes to children.			*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
_XfeManagerPropagateSetValues(Widget		ow,
							  Widget		nw,
							  Boolean		propagate_sensitive)
{
	_ApplyDataRec	data;

	/* XmNbackground */
	if (_XfeBackgroundPixel(nw) != _XfeBackgroundPixel(ow))
	{
		SET_APPLY_DATA(data,XmNbackground,_XfeBackgroundPixel(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNbackgroundPixmap */
	if (_XfeBackgroundPixmap(nw) != _XfeBackgroundPixmap(ow))
	{
		SET_APPLY_DATA(data,XmNbackground,_XfeBackgroundPixmap(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNborderColor */
	if (_XfeBorderPixmap(nw) != _XfeBorderPixmap(ow))
	{
		SET_APPLY_DATA(data,XmNborderColor,_XfeBorderPixmap(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

 	/* XmNborderPixmap */
	if (_XfeBorderPixmap(nw) != _XfeBorderPixmap(ow))
	{
		SET_APPLY_DATA(data,XmNborderPixmap,_XfeBorderPixmap(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNtopShadowPixmap */
	if (_XfemTopShadowPixmap(nw) != _XfemTopShadowPixmap(ow))
	{
		SET_APPLY_DATA(data,XmNtopShadowPixmap,_XfemTopShadowPixmap(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNbottomShadowPixmap */
	if (_XfemBottomShadowPixmap(nw) != _XfemBottomShadowPixmap(ow))
	{
		SET_APPLY_DATA(data,XmNbottomShadowPixmap,_XfemBottomShadowPixmap(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNtopShadowColor */
	if (_XfemTopShadowColor(nw) != _XfemTopShadowColor(ow))
	{
		SET_APPLY_DATA(data,XmNtopShadowColor,_XfemTopShadowColor(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNbottomShadowColor */
	if (_XfemBottomShadowColor(nw) != _XfemBottomShadowColor(ow))
	{
		SET_APPLY_DATA(data,XmNbottomShadowColor,_XfemBottomShadowColor(nw));

		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNforeground */
	if (_XfemForeground(nw) != _XfemForeground(ow))
	{
		SET_APPLY_DATA(data,XmNforeground,_XfemForeground(nw));
		
		XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetValue,&data,True,False);

		_XfemConfigFlags(nw) |= XfeConfigExpose;
	}

	/* XmNsensitive */
	if (propagate_sensitive)
	{
		if (_XfeSensitive(nw) != _XfeSensitive(ow))
		{
			XfeManagerApply(nw,XfeCHILDREN_INFO_ANY,ApplyChildSetSensitive,NULL,True,False);
			
			_XfemConfigFlags(nw) |= XfeConfigExpose;
		}
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Children apply client procedures										*/
/*																		*/
/*----------------------------------------------------------------------*/
static void
ApplyChildSetValue(Widget w,Widget child,XtPointer client_data)
{
	_ApplyDataRec *	data = (_ApplyDataRec *) client_data;

    XfeSetValue(child,data->resource_name,data->resource_value);

#ifdef DEBUG_ramiro
	printf("XfeSetValue(%s,%s,%d)\n",
		   XtName(child),
		   data->resource_name,
		   (int) data->resource_value);
#endif
}
/*----------------------------------------------------------------------*/
static void
ApplyChildSetSensitive(Widget w,Widget child,XtPointer client_data)
{
	XtSetSensitive(child,_XfeSensitive(w));
}
/*----------------------------------------------------------------------*/
