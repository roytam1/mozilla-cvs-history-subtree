/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef ilIImageRenderer_h___
#define ilIImageRenderer_h___

#include <stdio.h>
#include "libimg.h"
#include "nsISupports.h"

// IID for the nsIImageRenderer interface
#define IL_IIMAGERENDERER_IID    \
{ 0xec4e9fc0, 0xb1f3, 0x11d1,   \
{ 0x9b, 0xc3, 0x00, 0x60, 0x08, 0x8c, 0xa6, 0xb3 } };

class ilIImageRenderer : public nsISupports {
public:
 virtual void NewPixmap(void* aDisplayContext, 
			PRInt32 aWidth, PRInt32 aHeight, 
			IL_Pixmap* aImage, IL_Pixmap* aMask)=0;

  virtual void UpdatePixmap(void* aDisplayContext, 
			    IL_Pixmap* aImage, 
			    PRInt32 aXOffset, PRInt32 aYOffset, 
			    PRInt32 aWidth, PRInt32 aHeight)=0;

  virtual void ControlPixmapBits(void* aDisplayContext, 
				 IL_Pixmap* aImage, PRUint32 aControlMsg)=0;

  virtual void DestroyPixmap(void* aDisplayContext, IL_Pixmap* aImage)=0;

  virtual void DisplayPixmap(void* aDisplayContext, 
			     IL_Pixmap* aImage, IL_Pixmap* aMask, 
			     PRInt32 aX, PRInt32 aY, 
			     PRInt32 aXOffset, PRInt32 aYOffset, 
			     PRInt32 aWidth, PRInt32 aHeight)=0;

  virtual void DisplayIcon(void* aDisplayContext, 
			   PRInt32 aX, PRInt32 aY, PRUint32 aIconNumber)=0;

  virtual void GetIconDimensions(void* aDisplayContext, 
				 PRInt32 *aWidthPtr, PRInt32 *aHeightPtr, 
				 PRUint32 aIconNumber)=0;
};

#endif
