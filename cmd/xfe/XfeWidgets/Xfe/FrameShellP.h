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
/* Name:		<Xfe/FrameShellP.h>										*/
/* Description:	XfeFrameShell widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeFrameShellP_h_						/* start FrameShellP.h	*/
#define _XfeFrameShellP_h_

#include <Xfe/FrameShell.h>
#include <Xfe/ManagerP.h>
#include <X11/ShellP.h>
#include <Xm/VendorSEP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShellClassPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfeFrameShellClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShellClassRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFrameShellClassRec
{
  	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ShellClassPart				shell_class;
	WMShellClassPart			wm_shell_class;
	VendorShellClassPart		vendor_shell_class;
	TopLevelShellClassPart		top_level_shell_class;
    XfeFrameShellClassPart		xfe_frame_shell_class;
} XfeFrameShellClassRec;

externalref XfeFrameShellClassRec xfeFrameShellClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShellPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFrameShellPart
{
	/* Realization callback resources */
    XtCallbackList		realize_callback;		/* Realize cb			*/
    XtCallbackList		before_realize_callback;/* Before realize cb	*/

	/* Configure callback resources */
    XtCallbackList		resize_callback;		/* Resize cb			*/
    XtCallbackList		before_resize_callback;	/* Before resize cb		*/
    XtCallbackList		move_callback;			/* Move cb				*/

	/* WM Protocol callback resources */
    XtCallbackList		delete_window_callback;	/* Delete window cb		*/
    XtCallbackList		save_yourself_callback;	/* Save yourself cb		*/

	/* Mapping callbacks */
    XtCallbackList		first_map_callback;		/* First map cb			*/
    XtCallbackList		map_callback;			/* Map cb				*/
    XtCallbackList		unmap_callback;			/* Unmap cb				*/

	/* Title changed callback */
    XtCallbackList		title_changed_callback;	/* Title changed cb		*/

	/* Tracking resources */
	Boolean				track_delete_window;	/* Track selete window ?*/
	Boolean				track_editres;			/* Track editres ?		*/
	Boolean				track_mapping;			/* Track mapping ?		*/
	Boolean				track_position;			/* Track position ?		*/
	Boolean				track_save_yourself;	/* Track session man ?	*/
	Boolean				track_size;				/* Track size ?			*/

	/* Other resources */
	Boolean				has_been_mapped;		/* Has been mapped ?	*/
	Boolean				start_iconic;			/* Start iconic ?		*/

	Widget				bypass_shell;			/* Bypass shell			*/


    /* Private data -- Dont even look past this comment -- */

} XfeFrameShellPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShellRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeFrameShellRec
{
	CorePart				core;
	CompositePart			composite;
	ShellPart				shell;
	WMShellPart				wm;
	VendorShellPart			vendor;
	TopLevelShellPart		top_level_shell;
    XfeFrameShellPart		xfe_frame_shell;
} XfeFrameShellRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShellPart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeFrameShellPart(w) &(((XfeFrameShellWidget) w) -> xfe_frame_shell)

/*----------------------------------------------------------------------*/
/*																		*/
/* XmVendorShell Access Macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeVendorShellPart(w) &(((XmVendorShellExtObject) w) -> vendor)

/*----------------------------------------------------------------------*/
/*																		*/
/* WMShellPart Access Macro												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _WMShellPart(w) &(((WMShellWidget) w) -> wm)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end FrameShellP.h	*/

