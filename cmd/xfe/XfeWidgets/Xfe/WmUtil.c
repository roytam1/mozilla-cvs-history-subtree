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
/* Name:		<Xfe/WmUtil.c>											*/
/* Description:	Window manager utilities source.						*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* OpenLook																*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeIsOpenLookRunning(Widget w)
{
	Widget			app_shell = XfeAncestorFindApplicationShell(w);
	Display *		dpy = XtDisplay(app_shell);
	int				result;
	Atom			actual_type = 0;
	int				actual_format = 0;
	unsigned long	nitems = 0;
	unsigned long	bytes_after = 0;
	unsigned char *	data = 0;
	
	Atom proto_atom = XInternAtom(dpy, "_SUN_WM_PROTOCOLS", True);

    /* If the atom isn't interned, OLWM can't be running. */
	if (proto_atom == None)
	{
		return False;
	}

	result=XGetWindowProperty(dpy,
							  RootWindowOfScreen(DefaultScreenOfDisplay(dpy)),
							  proto_atom,
							  0, 0,            /* read 0 bytes */
							  False,           /* don't delete */
							  AnyPropertyType, /* type expected */
							  &actual_type,    /* returned values */
							  &actual_format,
							  &nitems,
							  &bytes_after,
							  &data);
	
	/* At most, this will be 1 allocated byte. */
	if (data)
	{
		XtFree(data);
	}
		
	/* If any of these are true, then OLWM isn't running. */
	if (result != Success ||            /* no such property */
		actual_type != XA_ATOM ||       /* didn't contain type Atom */
		bytes_after <= 0)               /* zero length */
	{
		return False;
	}
	
	/*
	 * Otherwise, it sure looks like OLWM is running.  If it's not, 
	 * then it's some other program that is imitating it well. 
	 */
	
	return True;
}
/*----------------------------------------------------------------------*/
