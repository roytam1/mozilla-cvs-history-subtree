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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */

#ifndef __NS_SVGGRAPHICFRAME_H__
#define __NS_SVGGRAPHICFRAME_H__

#include "nsFrame.h"
#include "nsISVGFrame.h"
#include "nsWeakReference.h"
#include "nsISVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsASVGGraphicSource.h"
#include "nsSVGGraphic.h"
#include "libart-incs.h"

class nsIPresContext;
class nsSVGRenderingContext;
class nsIDOMSVGMatrix;

typedef nsFrame nsSVGGraphicFrameBase;

class nsSVGGraphicFrame : public nsSVGGraphicFrameBase,
                          public nsISVGFrame,
                          public nsISVGValueObserver,
                          public nsSupportsWeakReference,
                          public nsASVGGraphicSource
{
protected:
  nsSVGGraphicFrame();
  virtual ~nsSVGGraphicFrame();
  
public:
   // nsISupports interface:
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return NS_OK; }
  NS_IMETHOD_(nsrefcnt) Release() { return NS_OK; }
public:

  // nsIFrame interface:
  NS_IMETHOD
  Init(nsIPresContext*  aPresContext,
       nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIStyleContext* aContext,
       nsIFrame*        aPrevInFlow);

  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType,
                               PRInt32         aHint);

  NS_IMETHOD DidSetStyleContext(nsIPresContext* aPresContext);
  
  
  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);

  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference

  // nsISVGFrame interface:
  NS_IMETHOD Paint(nsSVGRenderingContext* renderingContext);
  NS_IMETHOD GetFrameForPoint(float x, float y, nsIFrame** hit);
  NS_IMETHOD NotifyCTMChanged();
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD IsRedrawSuspended(PRBool* isSuspended);
  NS_IMETHOD InvalidateRegion(nsArtUtaRef uta, PRBool bRedraw);

  // nsASVGGraphicSource methods:
  virtual void GetCTM(nsIDOMSVGMatrix** ctm);
  virtual const nsStyleSVG* GetStyle();
  // to be implemented by subclass:
  //virtual void ConstructPath(nsASVGPathBuilder* pathBuilder)=0;
  
protected:
  virtual nsresult Init();
  void UpdateGraphic(nsSVGGraphicUpdateFlags flags);

private:
  nsSVGGraphicUpdateFlags mUpdateFlags;
  nsSVGGraphic mGraphic;
};

#endif // __NS_SVGGRAPHICFRAME_H__
