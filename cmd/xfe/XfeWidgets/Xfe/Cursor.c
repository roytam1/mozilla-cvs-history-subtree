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
/* Name:		<Xfe/Cursor.c>											*/
/* Description:	Xfe widgets cursor utilities.							*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeP.h>

#define hand_width 16
#define hand_height 16

#define hand_x_hot 3
#define hand_y_hot 2

static char hand_bits[] = 
{
	0x80, 0x01, 0x58, 0x0e, 0x64, 0x12, 0x64, 0x52, 0x48, 0xb2, 0x48, 0x92,
	0x16, 0x90, 0x19, 0x80, 0x11, 0x40, 0x02, 0x40, 0x04, 0x40, 0x04, 0x20,
	0x08, 0x20, 0x10, 0x10, 0x20, 0x10, 0x20, 0x10
};

#define hand_mask_width 16
#define hand_mask_height 16

static char hand_mask_bits[] = 
{
	0xff, 0xff, 0x7f, 0xfe, 0x67, 0xf2, 0x67, 0xf2, 0x4f, 0xb2, 0x4f, 0x92,
	0x17, 0x90, 0x19, 0x80, 0x11, 0xc0, 0x03, 0xc0, 0x07, 0xc0, 0x07, 0xe0,
	0x0f, 0xe0, 0x1f, 0xf0, 0x3f, 0xf0, 0x3f, 0xf0

#if 0
	0x80, 0x01, 0xd8, 0x0f, 0xfc, 0x1f, 0xfc, 0x5f, 0xf8, 0xff, 0xf8, 0xff,
	0xfe, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xfc, 0x3f,
	0xf8, 0x3f, 0xf0, 0x1f, 0xe0, 0x1f, 0xe0, 0x1f
#endif
};

/*----------------------------------------------------------------------*/
/*																		*/
/* Cursor																*/
/*																		*/
/*----------------------------------------------------------------------*/
/* extern */ void
XfeCursorDefine(Widget w,Cursor cursor)
{
    if (_XfeIsRealized(w) && _XfeCursorGood(cursor))
    {
		XDefineCursor(XtDisplay(w),_XfeWindow(w),cursor);
    }
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeCursorUndefine(Widget w)
{
    if (_XfeIsRealized(w))
    {
		XUndefineCursor(XtDisplay(w),_XfeWindow(w));
    }
}
/*----------------------------------------------------------------------*/
/* extern */ Boolean
XfeCursorGood(Cursor cursor)
{
	return _XfeCursorGood(cursor);
}
/*----------------------------------------------------------------------*/
/* extern */ Cursor
XfeCursorGetDragHand(Widget w)
{
	static Cursor	hand_cursor = None;

	if (hand_cursor == None)
	{
		Pixmap		cursor_pixmap;
		Pixmap		cursor_mask;
		XColor		white_color;
		XColor		black_color;

		assert( _XfeIsAlive(w) );
		
		cursor_pixmap = 
			XCreatePixmapFromBitmapData(XtDisplay(w),
										DefaultRootWindow(XtDisplay(w)),
										hand_bits,
										hand_width,
										hand_height,
										BlackPixelOfScreen(_XfeScreen(w)),
										WhitePixelOfScreen(_XfeScreen(w)),
										1);
		
		
		assert( _XfePixmapGood(cursor_pixmap) );
		
		cursor_mask = 
			XCreatePixmapFromBitmapData(XtDisplay(w),
										DefaultRootWindow(XtDisplay(w)),
										hand_mask_bits,
										hand_mask_width,
										hand_mask_height,
										BlackPixelOfScreen(_XfeScreen(w)),
										WhitePixelOfScreen(_XfeScreen(w)),
										1);
		
		assert( _XfePixmapGood(cursor_mask) );
		
		white_color.red = white_color.green = white_color.blue = 0xffff;
		black_color.red = black_color.green = black_color.blue = 0x0;
		
		hand_cursor = XCreatePixmapCursor(XtDisplay(w),
										  cursor_pixmap,
										  cursor_mask,
										  &white_color,
										  &black_color,
										  hand_x_hot,
										  hand_y_hot);

		assert( hand_cursor != None );
	}


	return hand_cursor;
}
/*----------------------------------------------------------------------*/

