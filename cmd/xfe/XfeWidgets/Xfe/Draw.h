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
/* Name:		<Xfe/Draw.h>											*/
/* Description:	Xfe widgets drawing functions header file.				*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeDraw_h_								/* start Draw.h			*/
#define _XfeDraw_h_

#include <Xm/Xm.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* Drawing functions													*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeDrawRectangle			(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 Dimension		thickness);
/*----------------------------------------------------------------------*/
extern void
XfeClearRectangle			(Display *		dpy,
							 Drawable		d,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 Dimension		thickness,
							 Boolean		exposures);
/*----------------------------------------------------------------------*/
extern void
XfeDrawCross				(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 Dimension		thickness);
/*----------------------------------------------------------------------*/
extern void
XfeDrawArrow				(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 unsigned char	direction);
/*----------------------------------------------------------------------*/
extern void
XfeDrawMotifArrow			(Display *		dpy,
							 Drawable		d,
							 GC				top_gc,
							 GC				bottom_gc,
							 GC				center_gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 unsigned char	direction,
							 Dimension		shadow_thickness,
							 Boolean		swap);
/*----------------------------------------------------------------------*/
extern void
XfeDrawVerticalLine			(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		height,
							 Dimension		thickness);
/*----------------------------------------------------------------------*/
extern void
XfeDrawHorizontalLine		(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		thickness);
/*----------------------------------------------------------------------*/
extern void
XfeStippleRectangle			(Display *		dpy,
							 Drawable		d,
							 GC				gc,
							 Position		x,
							 Position		y,
							 Dimension		width,
							 Dimension		height,
							 Dimension		step);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Drawable functions													*/
/*																		*/
/*----------------------------------------------------------------------*/
extern Boolean
XfeDrawableGetGeometry		(Display *		dpy,
							 Drawable		d,
							 Position *		x_out,
							 Position *		y_out,
							 Dimension *	width_out,
							 Dimension *	height_out);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Shadow functions														*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeDrawShadowsAroundWidget	(Widget			parent,
							 Widget			child,
							 GC				top_gc,
							 GC				bottom_gc,
							 Dimension		offset,
							 Dimension		shadow_thickness,
							 unsigned char	shadow_type);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end Draw.h			*/
