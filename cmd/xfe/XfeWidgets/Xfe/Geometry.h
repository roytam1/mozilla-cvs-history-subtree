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
/* Name:		<Xfe/Geometry.h>										*/
/* Description:	Xfe geometry public functions header.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeGeometry_h_							/* start Geometry.h		*/
#define _XfeGeometry_h_

#include <Xfe/BasicDefines.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* Access functions														*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Dimension	XfeBorderWidth		(Widget w);
extern Dimension	XfeWidth			(Widget w);
extern Dimension	XfeHeight			(Widget w);
extern Dimension	XfeScreenWidth		(Widget w);
extern Dimension	XfeScreenHeight		(Widget w);
extern Position		XfeX				(Widget w);
extern Position		XfeY				(Widget w);
extern Position		XfeRootX			(Widget w);
extern Position		XfeRootY			(Widget w);

/*----------------------------------------------------------------------*/
/*																		*/
/* Biggest, widest and tallest functions								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Widget
XfeBiggestWidget				(Boolean		horizontal,
								 WidgetList		widgets,
								 Cardinal		n);
/*----------------------------------------------------------------------*/
extern Dimension
XfeVaGetWidestWidget			(Widget			w,
								 ...);
/*----------------------------------------------------------------------*/
extern Dimension
XfeVaGetTallestWidget			(Widget			w,
								 ...);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeMoveChildrenByOffset	Move all children of a manager by an 		*/
/*							(x,y) offset.								*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeMoveChildrenByOffset			(Widget			w,
								 int			x_offset,
								 int			y_offset);
/*----------------------------------------------------------------------*/


XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Geometry.h		*/
