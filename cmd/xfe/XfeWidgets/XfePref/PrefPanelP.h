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
/* Name:		<Xfe/PrefPanelP.h>										*/
/* Description:	XfePrefPanel widget private header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/


#ifndef _XfePrefPanelP_h_						/* start PrefPanelP.h	*/
#define _XfePrefPanelP_h_

#include <Xfe/PrefPanel.h>
#include <Xfe/ManagerP.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION
	
/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrefPanelClassPart												*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	XtPointer			extension;					/* extension		*/ 
} XfePrefPanelClassPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrefPanelClassRec													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePrefPanelClassRec
{
    CoreClassPart				core_class;
    CompositeClassPart			composite_class;
    ConstraintClassPart			constraint_class;
    XmManagerClassPart			manager_class;
    XfeManagerClassPart			xfe_manager_class;
    XfePrefPanelClassPart		xfe_pref_panel_class;
} XfePrefPanelClassRec;

externalref XfePrefPanelClassRec xfePrefPanelClassRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrefPanelPart														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePrefPanelPart
{
	/* Color resources */
	Pixel				title_background;		/* Title background		*/
	Pixel				title_foreground;		/* Title foreground		*/

	/* i18n resources */
    unsigned char		title_direction;		/* Title direction		*/
	Dimension			title_spacing;			/* Title spacing		*/

	/* Title resources */
	Widget				title;					/* Title				*/
	unsigned char		title_alignment;		/* Title alignment		*/
	XmFontList			title_font_list;		/* Title font list		*/
	XmString			title_string;			/* Title string			*/
	Widget				title_icon;				/* Title string			*/

	/* Sub title resources */
	Widget				sub_title;				/* Sub title			*/
	unsigned char		sub_title_alignment;	/* Sub title alignment	*/
	XmString			sub_title_string;		/* Sub title string		*/
	XmFontList			sub_title_font_list;	/* Sub title font list	*/

	/* Widget names */
	String				title_widget_name;		/* Title widget name	*/
	String				sub_title_widget_name;	/* Sub title widget name*/

    /* Private data -- Dont even look past this comment -- */

} XfePrefPanelPart;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrefPanelRec														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct _XfePrefPanelRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    XfeManagerPart		xfe_manager;
    XfePrefPanelPart	xfe_pref_panel;
} XfePrefPanelRec;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePrefPanelPart Access Macro										*/
/*																		*/
/*----------------------------------------------------------------------*/
#define _XfePrefPanelPart(w) &(((XfePrefPanelWidget) w) -> xfe_pref_panel)

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end PrefPanelP.h		*/

