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
/* Name:		<Xfe/XfeBm.h>											*/
/*																		*/
/* Description:	Header for BmButton and BmCascade.  Widgets subclassed	*/
/*              from XmPushButton and XmCascadeButton.  Used in the		*/
/*              Mozilla bookmark filing mechanism.						*/
/*																		*/
/* Author:		Ramiro Estrugo <ramiro@netscape.com>					*/
/*																		*/
/*----------------------------------------------------------------------*/

#ifndef _XfeXfeBm_h_							/* start XfeBm.h		*/
#define _XfeXfeBm_h_

#include <Xfe/Xfe.h>

XFE_BEGIN_CPLUSPLUS_PROTECTION

/*----------------------------------------------------------------------*/
/*																		*/
/* Bm Resource Names													*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmNaccentType					"accentType"
#define XmNarmPixmapMask				"armPixmapMask"
#define XmNlabelPixmapMask				"labelPixmapMask"

/*----------------------------------------------------------------------*/
/*																		*/
/* Bm Class Names														*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmCAccentType					"AccentType"
#define XmCArmPixmapMask				"ArmPixmapMask"
#define XmCLabelPixmapMask				"LabelPixmapMask"

/*----------------------------------------------------------------------*/
/*																		*/
/* Bm Representation Types												*/
/*																		*/
/*----------------------------------------------------------------------*/
#define XmRAccentType					"AccentType"

/*----------------------------------------------------------------------*/
/*																		*/
/* XmRAccentType														*/
/*																		*/
/*----------------------------------------------------------------------*/
enum
{
    XmACCENT_NONE,
    XmACCENT_BOTTOM,
	XmACCENT_TOP,
    XmACCENT_ALL
};
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Accent filing mode													*/
/*																		*/
/*----------------------------------------------------------------------*/
typedef enum
{
    XmACCENT_FILE_ANYWHERE,
    XmACCENT_FILE_SELF
} XfeAccentFileMode;
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Bm Representation types												*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void			
XfeBmRegisterRepresentationTypes	(void);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Accent enabled/disabled functions									*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentDisable			(void);
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentEnable			(void);
/*----------------------------------------------------------------------*/
extern Boolean
XfeBmAccentIsEnabled		(void);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade accent offset values set/get functions		*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentSetOffsetLeft		(Dimension		offset_left);
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentSetOffsetRight		(Dimension		offset_right);
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentSetShadowThickness	(Dimension		shadow_thickness);
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentSetThickness			(Dimension		thickness);
/*----------------------------------------------------------------------*/
extern Dimension
XfeBmAccentGetOffsetLeft		(void);
/*----------------------------------------------------------------------*/
extern Dimension
XfeBmAccentGetOffsetRight		(void);
/*----------------------------------------------------------------------*/
extern Dimension
XfeBmAccentGetShadowThickness	(void);
/*----------------------------------------------------------------------*/
extern Dimension
XfeBmAccentGetThickness			(void);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade accent filing mode.						*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeBmAccentSetFileMode			(XfeAccentFileMode	file_mode);
/*----------------------------------------------------------------------*/
extern XfeAccentFileMode
XfeBmAccentGetFileMode			(void);
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/*																		*/
/* Global BmButton/BmCascade pixmap offset values set/get functions		*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeBmPixmapSetOffset			(Dimension		offset_lefft);
/*----------------------------------------------------------------------*/
extern Dimension
XfeBmPixmapGetOffset			(void);
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*																		*/
/* Public accent drawing functions.										*/
/*																		*/
/*----------------------------------------------------------------------*/
extern void
XfeMenuItemDrawAccent			(Widget			item,
								 unsigned char	accent_type,
								 Dimension		offset_left,
								 Dimension		offset_right,
								 Dimension		shadow_thickness,
								 Dimension		accent_thickness);
/*----------------------------------------------------------------------*/
extern void
XfeMenuItemEraseAccent			(Widget			item,
								 unsigned char	accent_type,
								 Dimension		offset_left,
								 Dimension		offset_right,
								 Dimension		shadow_thickness,
								 Dimension		accent_thickness);
/*----------------------------------------------------------------------*/

XFE_END_CPLUSPLUS_PROTECTION

#endif											/* end XfeBm.h			*/
