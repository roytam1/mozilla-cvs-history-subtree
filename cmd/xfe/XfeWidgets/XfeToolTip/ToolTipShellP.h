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
/* Name:		<Xfe/ToolTipShellP.h>									*/
/* Description:	XfeToolTipShell widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeToolTipShellP_h_					/* start ToolTipShellP.h*/
#define _XfeToolTipShellP_h_

#include <Xfe/ToolTipShell.h>
#include <Xfe/BypassShellP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShellClassPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer				extension;			/* extension			*/
} XfeToolTipShellClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShellClassRec												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolTipShellClassRec
{
	CoreClassPart				core_class;
	CompositeClassPart			composite_class;
	ShellClassPart				shell_class;
	WMShellClassPart			wm_shell_class;
	VendorShellClassPart		vendor_shell_class;
    XfeBypassShellClassPart		xfe_bypass_shell_class;
    XfeToolTipShellClassPart	xfe_tool_tip_shell_class;
} XfeToolTipShellClassRec;

externalref XfeToolTipShellClassRec xfeToolTipShellClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShellPart													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolTipShellPart
{
    /* Tool tip resources */
	XmFontList			font_list;				/* Title font list		*/
	Widget				tool_tip_label;			/* Tool tip label		*/
	int					tool_tip_timeout;		/* Tool tip timeout		*/

	/* Enumeration resources */
    unsigned char		tool_tip_type;			/* Tool tip type		*/
    unsigned char		tool_tip_placement;		/* Tool tip placement	*/

	/* Offset resources */
	int					horizontal_offset;		/* Horizontal offset	*/
	int					vertical_offset;		/* Vertical offset		*/

    /* Private data -- Dont even look past this comment -- */
	XtIntervalId		tool_tip_timer_id;		/* Tool tip timer id	*/

} XfeToolTipShellPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShellRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfeToolTipShellRec
{
	CorePart				core;
	CompositePart			composite;
	ShellPart				shell;
	WMShellPart				wm;
	VendorShellPart			vendor;
    XfeBypassShellPart		xfe_bypass_shell;
    XfeToolTipShellPart		xfe_tool_tip_shell;
} XfeToolTipShellRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShellPart access macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfeToolTipShellPart(w) \
&(((XfeToolTipShellWidget) w) -> xfe_tool_tip_shell)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShell method invocation functions							*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
_XfeToolTipShellLayoutTitle			(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeToolTipShellLayoutArrow			(Widget			w);
/*----------------------------------------------------------------------*/
extern void
_XfeToolTipShellDrawHighlight		(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/
extern void
_XfeToolTipShellDrawTitleShadow		(Widget			w,
								 XEvent *		event,
								 Region			region,
								 XRectangle *	clip_rect);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeToolTipShell - superclass = XfeManager							*/
/*																		*/
/* Component preparation macros.										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XFE_PREPARE_ARROW							XfePrepare1

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end ToolTipShellP.h	*/

