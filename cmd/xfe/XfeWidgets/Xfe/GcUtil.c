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
/* Name:		<Xfe/GcUtil.c>											*/
/* Description:	Xfe widgets graphic context utilities.					*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* GC functions															*/
/*																		*/
/*----------------------------------------------------------------------*/
GC
XfeAllocateStringGc(Widget		w,
					XmFontList	font_list,
					Pixel		fg,
					Pixel		bg,
					Boolean	 	sensitive)
/*----------------------------------------------------------------------*/
{
	XGCValues		values;
	XFontStruct *	fs = (XFontStruct *) NULL;
	XtGCMask		value_mask;

	value_mask = GCGraphicsExposures | GCForeground;
   
	values.graphics_exposures	= False;
	values.foreground			= fg;

	/* Add insensitive tile if needed */
	if (!sensitive)
	{
		value_mask |= (GCFillStyle | GCTile);
      
		values.fill_style	= FillTiled;

		values.tile = XfeInsensitiveTile(_XfeScreen(w),_XfeDepth(w),fg,bg);
	}

	/* Obtain the XFontStruct */
	_XmFontListGetDefaultFont(font_list,&fs);

	/* Add font if it is good */
	if (fs != NULL)
	{
		value_mask |= GCFont;
		values.font = fs->fid;
	}

	/* Allocate the string GC */
	return XtAllocateGC(w,_XfeDepth(w),value_mask,&values,0,0);
}
/*----------------------------------------------------------------------*/
GC
XfeAllocateColorGc(Widget w,Pixel fg,Pixel bg,Boolean sensitive)
{
	XGCValues	values;
	XtGCMask	value_mask;
	
	value_mask = GCGraphicsExposures | GCForeground;
	
	values.graphics_exposures	= False;
	values.foreground			= fg;
	
	/* Add insensitive tile if needed */
	if (!sensitive)
	{
		value_mask |= (GCFillStyle | GCTile);
		
		values.fill_style	= FillTiled;
		
		values.tile = XfeInsensitiveTile(_XfeScreen(w),_XfeDepth(w),fg,bg);
	}
	
	return XtGetGC(w,value_mask,&values);
}
/*----------------------------------------------------------------------*/
GC
XfeAllocateTileGc(Widget w,Pixmap tile_pixmap)
{
	XGCValues	values;
	XtGCMask	value_mask;
	
	value_mask = GCGraphicsExposures;
	
	values.graphics_exposures	= False;

	/* Use the tile pixmap only if it is good */
	if (_XfePixmapGood(tile_pixmap))
	{
		value_mask |= GCFillStyle | GCTile;

		values.fill_style 	= FillTiled;
		values.tile			= tile_pixmap;
	}

	return XtGetGC(w,value_mask,&values);
}
/*----------------------------------------------------------------------*/
GC
XfeAllocateTransparentGc(Widget w)
{
	XGCValues	values;
	XtGCMask	value_mask = GCGraphicsExposures;
	XtGCMask	dynamic_mask = GCClipMask | GCClipXOrigin | GCClipYOrigin;

	values.graphics_exposures = False;

	return XtAllocateGC(w,_XfeDepth(w),value_mask,&values,dynamic_mask,0);
}
/*----------------------------------------------------------------------*/
GC
XfeAllocateSelectionGc(Widget		w,
					   Dimension	thickness,
					   Pixel		fg,
					   Pixel		bg)
{
	XGCValues	values;
	XtGCMask	value_mask;

	value_mask = 
		GCGraphicsExposures | GCForeground | GCLineWidth | 
		GCFunction | GCSubwindowMode;

	values.graphics_exposures	= False;
	values.line_width			= thickness;
	values.function				= GXxor;
	values.subwindow_mode		= IncludeInferiors;
	values.foreground			= fg ^ bg;

	return XtGetGC(w,value_mask,&values);
}
/*----------------------------------------------------------------------*/
GC
XfeAllocateCopyGc(Widget w)
{
	XGCValues	values;
	XtGCMask	value_mask;

	value_mask = GCGraphicsExposures;

	values.graphics_exposures	= False;

	return XtGetGC(w,value_mask,&values);
}
/*----------------------------------------------------------------------*/
