/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Chris Saari <saari@netscape.com>
 */

#ifndef _nsGIFDecoder2_h
#define _nsGIFDecoder2_h

#include "nsCOMPtr.h"
#include "nsIImageDecoder.h"
#include "nsIImageContainer.h"
#include "nsIImageDecoderObserver.h"
#include "nsIImageFrame.h"
#include "nsIImageRequest2.h"

#include "GIF2.h"

//1808b950-1dd2-11b2-97fd-9ec6cc3f42cd
#define NS_GIFDECODER2_CID \
{ 0x1808b950, 0x1dd2, 0x11b2, \
{ 0x97, 0xfd, 0x9e, 0xc6, 0xcc, 0x3f, 0x42, 0xcd } }

//////////////////////////////////////////////////////////////////////
// nsGIFDecoder2 Definition

class nsGIFDecoder2 : public nsIImageDecoder   
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIMAGEDECODER
  NS_DECL_NSIOUTPUTSTREAM

  nsGIFDecoder2();
  virtual ~nsGIFDecoder2();

  static NS_METHOD Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);
  
  NS_METHOD ProcessData(unsigned char *data, PRUint32 count);
  
  nsCOMPtr<nsIImageContainer> mImageContainer;
  nsCOMPtr<nsIImageFrame> mImageFrame;
  nsCOMPtr<nsIImageRequest> mImageRequest;
  nsCOMPtr<nsIImageDecoderObserver> mObserver; // this is just qi'd from mRequest for speed
  
  gif_struct mGIFStruct;
  
  PRUint8 *colorLine, *alphaLine;
  PRUint8 *interlacebuf;
  PRUint32 ibpr;
};

// static callbacks for the GIF decoder
static int PR_CALLBACK BeginGIF(
  void*    aClientData,
  PRUint32 aLogicalScreenWidth, 
  PRUint32 aLogicalScreenHeight,
  GIF_RGB* aBackgroundRGB,
  GIF_RGB* aTransparencyChromaKey);
  
static int PR_CALLBACK HaveDecodedRow(
  void* aClientData,
  PRUint8* aRowBufPtr,   // Pointer to single scanline temporary buffer
  PRUint8* aRGBrowBufPtr,// Pointer to temporary storage for dithering/mapping
  int aXOffset,          // With respect to GIF logical screen origin
  int aLength,           // Length of the row?
  int aRow,              // Row number?
  int aDuplicateCount,   // Number of times to duplicate the row?
  PRUint8 aDrawMode,     // il_draw_mode
  int aInterlacePass);
    
static int PR_CALLBACK NewPixmap();

static int PR_CALLBACK EndGIF();
static int PR_CALLBACK BeginImageFrame(
  void*    aClientData,
  PRUint32 aFrameNumber,   /* Frame number, 1-n */
  PRUint32 aFrameXOffset,  /* X offset in logical screen */
  PRUint32 aFrameYOffset,  /* Y offset in logical screen */
  PRUint32 aFrameWidth,    
  PRUint32 aFrameHeight,   
  GIF_RGB* aTransparencyChromaKey);
static int PR_CALLBACK EndImageFrame();
static int PR_CALLBACK SetupColorspaceConverter();
static int PR_CALLBACK ResetPalette();
static int PR_CALLBACK InitTransparentPixel();
static int PR_CALLBACK DestroyTransparentPixel();

static int PR_CALLBACK HaveImageAll(
  void* aClientData);
#endif
