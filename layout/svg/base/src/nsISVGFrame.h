/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsISVGFrame_h__
#define nsISVGFrame_h__

#include "nsISupports.h"

#include "nsArtUtaRef.h"

class nsSVGRenderingContext;
class nsIFrame;

#define NS_ISVGFRAME_IID \
  {0x84957983, 0x712, 0x11d4, \
    { 0x97, 0x7, 0x0, 0x60, 0xb0, 0xfb, 0x99, 0x56 }}

class nsISVGFrame : public nsISupports {
public:

  NS_DEFINE_STATIC_IID_ACCESSOR(NS_ISVGFRAME_IID)

  // down the tree
    
  NS_IMETHOD Paint(nsSVGRenderingContext* renderingContext)=0;
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit)=0;
  NS_IMETHOD NotifyCTMChanged()=0;
  NS_IMETHOD NotifyRedrawSuspended()=0;
  NS_IMETHOD NotifyRedrawUnsuspended()=0;

  // up the tree. these should really go into a separate nsISVGContainerFrame interface  
  NS_IMETHOD InvalidateRegion(nsArtUtaRef uta, PRBool bRedraw)=0;
  NS_IMETHOD IsRedrawSuspended(PRBool* isSuspended)=0;
};

#endif
