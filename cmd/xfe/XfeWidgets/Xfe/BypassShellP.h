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
/* Name:		<Xfe/BypassShellP.h>									*/
/* Description:	XfeBypassShell widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeBypassShellP_h_						/* start BypassShellP.h	*/
#define _XfeBypassShellP_h_

#include <Xfe/BypassShell.h>
#include <Xfe/ManagerP.h>
#include <X11/ShellP.h>
#include <Xm/VendorSEP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShellClassPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfeBypassShellClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShellClassRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBypassShellClassRec
{
  	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ShellClassPart				shell_class;
	WMShellClassPart			wm_shell_class;
	VendorShellClassPart		vendor_shell_class;
    XfeBypassShellClassPart		xfe_bypass_shell_class;
} XfeBypassShellClassRec;

externalref XfeBypassShellClassRec xfeBypassShellClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShellPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBypassShellPart
{
	/* Realization callback resources */
    XtCallbackList		realize_callback;		/* Realize cb			*/
    XtCallbackList		before_realize_callback;/* Before realize cb	*/

	/* Mapping callbacks */
    XtCallbackList		map_callback;			/* Map cb				*/
    XtCallbackList		unmap_callback;			/* Unmap cb				*/

	XtCallbackList		change_managed_callback;/* Change managed cb	*/

	/* Shadow resources */
	Pixel				bottom_shadow_color;	/* Bottom shadow color	*/
	Pixel				top_shadow_color;		/* Top shadow color		*/
    Dimension			shadow_thickness;		/* Shadow Thickness		*/
    unsigned char		shadow_type;			/* Shadow Type			*/

	/* Cursor resources */
	Cursor				cursor;					/* Cursor				*/

	/* Other resources */
	Boolean				ignore_exposures;		/* Ignore_exposures 	*/

    /* Private data -- Dont even look past this comment -- */
	GC					bottom_shadow_GC;		/* Bottom shadow GC		*/
	GC					top_shadow_GC;			/* Top shadow GC		*/
	Widget				managed_child;			/* Managed child		*/

} XfeBypassShellPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShellRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeBypassShellRec
{
	CorePart				core;
	CompositePart			composite;
	ShellPart				shell;
	WMShellPart				wm;
	VendorShellPart			vendor;
    XfeBypassShellPart		xfe_bypass_shell;
} XfeBypassShellRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShellPart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeBypassShellPart(w) \
&(((XfeBypassShellWidget) w) -> xfe_bypass_shell)

/*----------------------------------------------------------------------*/
/*																		*/
/* XmVendorShell Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeVendorShellPart(w) &(((XmVendorShellExtObject) w) -> vendor)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBypassShell private methods										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
_XfeBypassShellGlobalIsAlive				(void);
/*----------------------------------------------------------------------*/
extern Widget
_XfeBypassShellGlobalAccess				(void);
/*----------------------------------------------------------------------*/
extern Widget
_XfeBypassShellGlobalInitialize			(Widget		pw,
										 String		name,
										 Arg *		av,
										 Cardinal	ac);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end BypassShellP.h	*/

