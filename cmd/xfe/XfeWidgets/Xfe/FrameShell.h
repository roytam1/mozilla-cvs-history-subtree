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
/* Name:		<Xfe/TempShell.h>										*/
/* Description:	XfeTempShell widget public header file.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeFrameShell_h_						/* start FrameShell.h	*/
#define _XfeFrameShell_h_

#include <Xfe/Manager.h>
#include <Xfe/BypassShell.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShell resource names											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNbeforeResizeCallback				"beforeResizeCallback"
#define XmNdeleteWindowCallback				"deleteWindowCallback"
#define XmNmoveCallback						"moveCallback"
#define XmNsaveYourselfCallback				"saveYourselfCallback"
#define XmNtitleChangedCallback				"titleChangedCallback"

#define XmNstartIconic						"startIconic"
#define XmNtrackPosition					"trackPosition"
#define XmNtrackSaveYourself				"trackSaveYourself"
#define XmNhasBeenMapped				"hasBeenMapped"
#define XmNbypassShell					"bypassShell"
#define XmNtrackSize						"trackSize"
#define XmNtrackDeleteWindow			"trackDeleteWindow"
#define XmNtrackEditres					"trackEditres"
#define XmNtrackMapping					"trackMapping"

#define XmCBypassShell						"BypassShell"
#define XmCStartIconic						"StartIconic"
#define XmCTrackEditres						"TrackEditres"
#define XmCTrackMapping						"TrackMapping"
#define XmCTrackPosition					"TrackPosition"
#define XmCTrackSaveYourself				"TrackSaveYourself"
#define XmCTrackSize						"TrackSize"


/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox class names													*/
/*																		*/
/*----------------------------------------------------------------------*/
externalref WidgetClass xfeFrameShellWidgetClass;

typedef struct _XfeFrameShellClassRec *			XfeFrameShellWidgetClass;
typedef struct _XfeFrameShellRec *				XfeFrameShellWidget;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeBox subclass test macro											*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeIsFrameShell(w)	XtIsSubclass(w,xfeFrameShellWidgetClass)

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeFrameShell public methods											*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeCreateFrameShell					(Widget		pw,
									 String		name,
									 ArgList	av,
									 Cardinal	ac);
/*----------------------------------------------------------------------*/
extern Widget
XfeFrameShellGetBypassShell			(Widget		w);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end FrameShell.h		*/
