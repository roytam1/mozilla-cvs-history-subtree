/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */


#ifndef __NS_ISVGLIBART_BITMAP_H__
#define __NS_ISVGLIBART_BITMAP_H__

#include "nsISupports.h"

class nsIRenderingContext;
class nsIPresContext;
struct nsRect;
typedef PRUint32 nscolor;

////////////////////////////////////////////////////////////////////////
// nsISVGLibartBitmap
// Abstraction of a 24 bpp libart-compatible bitmap hiding
// platform-specific implementation details

// {18E4F62F-60A4-42D1-BCE2-43445656096E}
#define NS_ISVGLIBARTBITMAP_IID \
{ 0x18e4f62f, 0x60a4, 0x42d1, { 0xbc, 0xe2, 0x43, 0x44, 0x56, 0x56, 0x09, 0x6e } }

class nsISVGLibartBitmap : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISVGLIBARTBITMAP_IID; return iid; }

  NS_IMETHOD_(PRUint8 *) GetBits()=0;
  NS_IMETHOD_(int) GetIndexR()=0;
  NS_IMETHOD_(int) GetIndexG()=0;
  NS_IMETHOD_(int) GetIndexB()=0;
  NS_IMETHOD_(int) GetLineStride()=0;
  NS_IMETHOD_(int) GetWidth()=0;
  NS_IMETHOD_(int) GetHeight()=0;

  // Obtain a rendering context for part of the bitmap. In general
  // this will be different to the RC passed at initialization time
  NS_IMETHOD_(void) LockRenderingContext(const nsRect& rect, nsIRenderingContext**ctx)=0;
  NS_IMETHOD_(void) UnlockRenderingContext()=0;

  // flush changes to the rendering context passed at initialization time
  NS_IMETHOD_(void) Flush()=0;
};


////////////////////////////////////////////////////////////////////////
// classes implementing this interface:

// nsSVGLibartBitmapWindows
// XXX

// nsSVGLibartBitmapXLib
// XXX

// nsSVGLibartBitmapMac
// XXX

#if defined(MOZ_ENABLE_GTK2)
nsresult
NS_NewSVGLibartBitmapGdk(nsISVGLibartBitmap **result,
                         nsIRenderingContext *ctx,
                         nsIPresContext *presContext,
                         const nsRect & rect);
#else
// nsSVGLibartBitmapDefault: an implementation based on nsIImage that
// should work on all platforms but doesn't support obtaining
// RenderingContexts with Lock/UnlockRenderingContext
nsresult
NS_NewSVGLibartBitmapDefault(nsISVGLibartBitmap **result,
                             nsIRenderingContext *ctx,
                             nsIPresContext* presContext,
                             const nsRect & rect);
#endif


#endif // __NS_ISVGLIBART_BITMAP_H__
