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
/* Name:		<XfeTest/TestPixmap.c>									*/
/* Description:	Xfe widget pixmap test funcs.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeTest.h>

#ifdef XFE_USE_NATIVE_XPM
#include <X11/xpm.h>
#endif

#define DUMB_ASS_DEFAULT "75_foreground"

/*----------------------------------------------------------------------*/
Boolean
XfeAllocatePixmapFromFile(char *			filename,
						  Display *			dpy,
						  Drawable			d,
						  Colormap			colormap,
						  Cardinal			closeness,
						  Cardinal			depth,
						  Pixel				bg,
						  Pixmap *			pixmap,
						  Pixmap *			mask)
{
	Boolean				result = True;

#ifdef XFE_USE_NATIVE_XPM
    XpmAttributes		attrib;
	XpmColorSymbol		symbols[1];

    assert( dpy != NULL );
    assert( d != None );
    assert( colormap != None );
    assert( pixmap != NULL );
    assert( mask != NULL );
    assert( depth > 0 );
	assert( filename != NULL );
	assert( access(filename,F_OK | R_OK) == 0 );

	/*  Set up the transparent symbol */
	symbols[0].name		= NULL;
	symbols[0].value	= "none";
	symbols[0].pixel	= bg;

    /* Set up the Xfem Attributes mask and strucutre */
    attrib.valuemask = 
		XpmCloseness | XpmDepth | XpmColormap | XpmColorSymbols;

    attrib.colorsymbols		= symbols;
    attrib.numsymbols		= 1;
    attrib.closeness		= closeness;
    attrib.colormap			= colormap;
    attrib.depth			= depth;

    /* Try to read the xpm file */
    if (XpmReadFileToPixmap(dpy,d,filename,pixmap,mask,&attrib) != XpmSuccess)
    {
		*pixmap = XmUNSPECIFIED_PIXMAP;
		*mask = XmUNSPECIFIED_PIXMAP;
		
		result = False;
    }

#else

	/* Pick a dumb ass default so that loser platform will at least run */
	*pixmap = XmGetPixmap(DefaultScreenOfDisplay(dpy),
						  DUMB_ASS_DEFAULT,
						  BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)),
						  bg);

	assert( *pixmap != XmUNSPECIFIED_PIXMAP );
	
	*mask = XmUNSPECIFIED_PIXMAP;

	result = True;

#endif

	return result;
}
/*----------------------------------------------------------------------*/
Boolean
XfeAllocatePixmapFromData(char **			data,
						  Display *			dpy,
						  Drawable			d,
						  Colormap			colormap,
						  Cardinal			closeness,
						  Cardinal			depth,
						  Pixel				bg,
						  Pixmap *			pixmap,
						  Pixmap *			mask)
{
	Boolean				result = True;

#ifdef XFE_USE_NATIVE_XPM
    XpmAttributes		attrib;
	XpmColorSymbol		symbols[1];

    assert( dpy != NULL );
    assert( d != None );
    assert( colormap != None );
    assert( pixmap != NULL );
    assert( mask != NULL );
    assert( depth > 0 );
	assert( data != NULL );

	/*  Set up the transparent symbol */
	symbols[0].name		= NULL;
	symbols[0].value	= "none";
	symbols[0].pixel	= bg;

    /* Set up the Xfem Attributes mask and strucutre */
    attrib.valuemask = 
		XpmCloseness | XpmDepth | XpmColormap | XpmColorSymbols;
	
    attrib.colorsymbols		= symbols;
    attrib.numsymbols		= 1;
    attrib.closeness		= closeness;
    attrib.colormap			= colormap;
    attrib.depth			= depth;

    if (XpmCreatePixmapFromData(dpy,d,data,pixmap,mask,&attrib) != XpmSuccess)
    {
		*pixmap = XmUNSPECIFIED_PIXMAP;
		*mask = XmUNSPECIFIED_PIXMAP;
		
		result = False;
    }

#else

	/* Pick a dumb ass default so that loser platform will at least run */
	*pixmap = XmGetPixmap(DefaultScreenOfDisplay(dpy),
						  DUMB_ASS_DEFAULT,
						  BlackPixelOfScreen(DefaultScreenOfDisplay(dpy)),
						  bg);

	assert( *pixmap != XmUNSPECIFIED_PIXMAP );
	
	*mask = XmUNSPECIFIED_PIXMAP;

	result = True;

#endif

	return result;
}
/*----------------------------------------------------------------------*/
Pixmap
XfeGetPixmapFromFile(Widget w,char * filename)
{
    Pixmap pixmap = XmUNSPECIFIED_PIXMAP;
    Pixmap mask = XmUNSPECIFIED_PIXMAP;

	assert( filename != NULL );

#ifdef XFE_USE_NATIVE_XPM
    XfeAllocatePixmapFromFile(filename,
							  XtDisplay(w),
							  DefaultRootWindow(XtDisplay(w)),
							  XfeColormap(w),
							  40000,
							  XfeDepth(w),
							  XfeBackground(w),
							  &pixmap,
							  &mask);
#else

	pixmap = XmGetPixmap(XtScreen(w),
						 DUMB_ASS_DEFAULT,
						 XfeForeground(w),
						 XfeBackground(w));

	assert( pixmap != XmUNSPECIFIED_PIXMAP );

#endif
	
	if (XfePixmapGood(mask))
	{
		XFreePixmap(XtDisplay(w),mask);
	}
	
    return pixmap;
}
/*----------------------------------------------------------------------*/
Pixmap
XfeGetPixmapFromData(Widget w,char ** data)
{
    Pixmap pixmap = XmUNSPECIFIED_PIXMAP;
    Pixmap mask = XmUNSPECIFIED_PIXMAP;

	assert( data != NULL );

#ifdef XFE_USE_NATIVE_XPM
    XfeAllocatePixmapFromData(data,
							  XtDisplay(w),
							  DefaultRootWindow(XtDisplay(w)),
							  XfeColormap(w),
							  40000,
							  XfeDepth(w),
							  XfeBackground(w),
							  &pixmap,
							  &mask);
#else

	pixmap = XmGetPixmap(XtScreen(w),
						 DUMB_ASS_DEFAULT,
						 XfeForeground(w),
						 XfeBackground(w));

	assert( pixmap != XmUNSPECIFIED_PIXMAP );

#endif
	
	if (XfePixmapGood(mask))
	{
		XFreePixmap(XtDisplay(w),mask);
	}
	
    return pixmap;
}
/*----------------------------------------------------------------------*/
XfePixmapTable
XfeAllocatePixmapTable(Widget w,String * files,Cardinal num_files)
{
	XfePixmapTable	table = NULL;
	Cardinal		i;

	assert( files != NULL );
	assert( num_files > 0 );

	table = (XfePixmapTable) XtMalloc(sizeof(Pixmap) * num_files);

	assert( table != NULL );

	for(i = 0; i < num_files; i++)
	{
		table[i] = XfeGetPixmapFromFile(w,files[i]);
	}

    return table;
}
/*----------------------------------------------------------------------*/
