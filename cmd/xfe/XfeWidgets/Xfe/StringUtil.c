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
/* Name:		<Xfe/StringUtil.c>										*/
/* Description:	Xfe widgets XmString utilities.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

#include <Xm/LabelP.h>
#include <Xm/LabelGP.h>
#include <Xfe/LabelP.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* This function is useful when effecient access to a widget's 			*/
/* XmNlabelString is needed.  It avoids the GetValues() and 			*/
/* XmStringCopy() overhead.  Of course, the result should be 			*/
/* considered read only.												*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XmString
XfeFastAccessLabelString(Widget w)
{
	XmString	label_string = NULL;

	assert( _XfeIsAlive(w) );

	if (!_XfeIsAlive(w))
	{
		return NULL;
	}

	if (XfeIsLabel(w))
	{
		label_string = ((XfeLabelWidget) w) -> xfe_label . label_string;
	}
#ifdef DEBUG_ramiro
	else
	{
		assert( 0 );
	}
#endif

	return label_string;
}
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* XmString utils														*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XmString
XfeXmStringCopy(Widget w,XmString xmstr,String fallback)
{
    XmString new_xmstr;
    
    /* Make sure the string is setup properly */
    if (!xmstr)
    {
		/* If no xmstring is given, create using the fallback cstring */
		new_xmstr = XmStringCreateLocalized(fallback);
    }
    else
    {
		/* Otherwise make a carbon copy - no check done to verify xmstring */
		new_xmstr = XmStringCopy(xmstr);
    }
    
    return new_xmstr;
}
/*----------------------------------------------------------------------*/
/* extern */ String
XfeXmStringGetPSZ(XmString xmstr,char * tag)
{
	String		psz_string = NULL;

	if (xmstr)
	{
		XmStringGetLtoR(xmstr,tag,&psz_string);
	}	
	
	return psz_string;
}
/*----------------------------------------------------------------------*/
/*extern*/ void
XfeSetXmStringPSZ(Widget w,String name,char * tag,char * value)
{
	XmString xmstr;

	assert( w != NULL );
	assert( name != NULL );

	xmstr = XmStringCreateLtoR(value,tag);

    XfeSetValue(w,name,(XtArgVal) xmstr);

	if (xmstr)
	{
		XmStringFree(xmstr);
	}
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XmStringTable utils													*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ XmString *
XfeXmStringTableCopy(XmString * items,Cardinal num_items)
{
	XmString * copy = NULL;

	return copy;
}
/*----------------------------------------------------------------------*/

