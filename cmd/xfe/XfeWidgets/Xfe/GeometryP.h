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
/* Name:		<Xfe/GeometryP.h>										*/
/* Description:	Xfe geometry functions private header.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeGeometryP_h_						/* start GeometryP.h	*/
#define _XfeGeometryP_h_

XFE_BEGIN_CPLUSPLUS_PROTECTION

#include <Xm/XmP.h>
#include <Xfe/Geometry.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeGeometry															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	int x;
	int y;
	int width;
	int height;
} XfeGeometryRec,*XfeGeometry;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfeDimensions														*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	int width;
	int height;
} XfeDimensionsRec,*XfeDimensions;

/*----------------------------------------------------------------------*/
/*																		*/
/* XfePosition															*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef struct
{
	int x;
	int y;
} XfePositionRec,*XfePosition;

/*----------------------------------------------------------------------*/
/*																		*/
/* Geometry defines														*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XfeGEOMETRY_INVALID_DIMENSION -1

/*----------------------------------------------------------------------*/
/*																		*/
/* Misc																	*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Dimension
_XfeHeightCenter				(Widget			one,
								 Widget			two);
/*----------------------------------------------------------------------*/
extern Dimension
_XfeWidthCenter					(Widget			one,
								 Widget			two);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Geometry																*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void						
_XfeConfigureWidget				(Widget			w,
								 int			x,
								 int			y,
								 int			width,
								 int			height);
/*----------------------------------------------------------------------*/
extern void						
_XfeConfigureOrHideWidget		(Widget			w,
								 int			x,
								 int			y,
								 int			width,
								 int			height);
/*----------------------------------------------------------------------*/
extern void
_XfeResizeWidget				(Widget			w,
								 int			width,
								 int			height);
/*----------------------------------------------------------------------*/
extern void
_XfeMoveWidget					(Widget			w,
								 int			x,
								 int			y);
/*----------------------------------------------------------------------*/
extern void
_XfePreferredGeometry			(Widget			w,
								 Dimension *	width_out,
								 Dimension *	height_out);
/*----------------------------------------------------------------------*/
extern Boolean
_XfeMakeGeometryRequest			(Widget			w,
								 Dimension		width,
								 Dimension		height);
/*----------------------------------------------------------------------*/
extern XtGeometryResult
_XfeLiberalGeometryManager		(Widget				child,
								 XtWidgetGeometry *	request,
								 XtWidgetGeometry *	reply);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end GeometryP.h		*/
