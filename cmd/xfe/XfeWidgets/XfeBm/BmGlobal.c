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
/* Name:		<XfeBm/BmGlobal.c>										*/
/*																		*/
/* Description:	Global functions to set/get accent drawing and filing	*/
/*				properties.												*/
/*																		*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#include <Xfe/XfeBm.h>

/*----------------------------------------------------------------------*/
/*																		*/
/* Accent enabled/disabled functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
static Boolean _accent_is_enabled = False;

/* extern */ Boolean
XfeBmAccentIsEnabled(void)
{
	return _accent_is_enabled;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeBmAccentDisable(void)
{
	_accent_is_enabled = False;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeBmAccentEnable(void)
{
	_accent_is_enabled = True;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade accent offset values set/get functions		*/
/*																		*/
/*----------------------------------------------------------------------*/
static Dimension	_accent_offset_left			= 4;
static Dimension	_accent_offset_right		= 8;
static Dimension	_accent_shadow_thickness	= 1;
static Dimension	_accent_thickness			= 1;

/* extern */ void
XfeBmAccentSetOffsetLeft(Dimension offset_left)
{
	_accent_offset_left = offset_left;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeBmAccentSetOffsetRight(Dimension offset_right)
{
	_accent_offset_right = offset_right;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeBmAccentSetShadowThickness(Dimension shadow_thickness)
{
	_accent_shadow_thickness = shadow_thickness;
}
/*----------------------------------------------------------------------*/
/* extern */ void
XfeBmAccentSetThickness(Dimension thickness)
{
	_accent_thickness = thickness;
}
/*----------------------------------------------------------------------*/
/* extern */ Dimension
XfeBmAccentGetOffsetLeft(void)
{
	return _accent_offset_left;
}
/*----------------------------------------------------------------------*/
/* extern */ Dimension
XfeBmAccentGetOffsetRight(void)
{
	return _accent_offset_right;
}
/*----------------------------------------------------------------------*/
/* extern */ Dimension
XfeBmAccentGetShadowThickness(void)
{
	return _accent_shadow_thickness;
}
/*----------------------------------------------------------------------*/
/* extern */ Dimension
XfeBmAccentGetThickness(void)
{
	return _accent_thickness;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade accent filing mode.						*/
/*																		*/
/*----------------------------------------------------------------------*/
static XfeAccentFileMode	_accent_file_mode = XmACCENT_FILE_ANYWHERE;

/* extern */ void
XfeBmAccentSetFileMode(XfeAccentFileMode file_mode)
{
	_accent_file_mode = file_mode;
}
/*----------------------------------------------------------------------*/
/* extern */ XfeAccentFileMode
XfeBmAccentGetFileMode(void)
{
	return _accent_file_mode;
}
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade pixmap offset values set/get functions		*/
/*																		*/
/*----------------------------------------------------------------------*/
static Dimension	_pixmap_offset = 4;

/* extern */ void
XfeBmPixmapSetOffset(Dimension offset)
{
	_pixmap_offset = offset;
}
/*----------------------------------------------------------------------*/
/* extern */ Dimension
XfeBmPixmapGetOffset(void)
{
	return _pixmap_offset;
}
/*----------------------------------------------------------------------*/
