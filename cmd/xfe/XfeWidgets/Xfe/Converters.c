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
/* Name:		<Xfe/Converters.c>										*/
/* Description:	Xfe widgets resource converters source.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

#include <Xfe/Label.h>

#include <string.h> /* for strncasecmp() */

/*----------------------------------------------------------------------*/
/*																		*/
/* Converter Args for StringTo{misc}									*/
/*																		*/
/*----------------------------------------------------------------------*/
static XtConvertArgRec _XfeWidgetConvertArg[] = 
{
	{
		XtWidgetBaseOffset,
		(XtPointer) XtOffsetOf(WidgetRec , core . self),
		sizeof(Widget)
	}
};
/*----------------------------------------------------------------------*/

#define COMPARE(s,v) (strncasecmp( (s),(v),strlen(v) ) == 0)

/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeCvtStringToModifiers(Display *	dpy,
					   XrmValue *	args,
					   Cardinal *	nargs,
					   XrmValue *	from,
					   XrmValue *	to,
					   XtPointer *	data)
{
	String		str = (String) from->addr;
	Modifiers	mod = 0;
   
	if      (COMPARE(str,"no"))			mod = 0;
	else if (COMPARE(str,"any"))		mod = AnyModifier;
	else if (COMPARE(str,"alt"))		mod = Mod1Mask;
	else if (COMPARE(str,"control"))	mod = ControlMask;
	else if (COMPARE(str,"lock"))		mod = LockMask;
	else if (COMPARE(str,"mod1"))		mod = Mod1Mask;
	else if (COMPARE(str,"mod2"))		mod = Mod2Mask;
	else if (COMPARE(str,"mod3"))		mod = Mod3Mask;
	else if (COMPARE(str,"mod4"))		mod = Mod4Mask;
	else if (COMPARE(str,"mod5"))		mod = Mod4Mask;
	else if (COMPARE(str,"shift"))		mod = ShiftMask;
    else
	{
		XtDisplayStringConversionWarning(dpy,str,XmRModifiers);

		return False;
	}

	_XfeConverterDone(mod,Modifiers);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeRegisterStringToModifiersCvt()
{
	XtSetTypeConverter(XmRString,
					   XmRModifiers,
					   XfeCvtStringToModifiers,
					   _XfeWidgetConvertArg,
					   XtNumber(_XfeWidgetConvertArg),
					   XtCacheNone,
					   NULL);
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeRegisterConverters()
{
	XfeRegisterStringToModifiersCvt();
}
/*----------------------------------------------------------------------*/
