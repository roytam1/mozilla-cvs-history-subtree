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

#ifndef __NS_SVGTRANSFORMLIST_H__
#define __NS_SVGTRANSFORMLIST_H__

#include "nsSVGValue.h"
#include "nsISVGValueObserver.h"
#include "nsWeakReference.h"
#include "nsIDOMSVGTransformList.h"
#include "nsVoidArray.h"

class nsSVGTransformList : public nsSVGValue,
                           public nsIDOMSVGTransformList,
                           public nsISVGValueObserver,
                           public nsSupportsWeakReference
{
public:
  static nsresult Create(const nsAReadableString& aValue, nsISVGValue** aResult);
  static nsresult Create(nsIDOMSVGTransformList** aResult);
  
protected:
  nsSVGTransformList();
  virtual ~nsSVGTransformList();
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGTransformList interface:
  NS_DECL_NSIDOMSVGTRANSFORMLIST
  
  // remainder of nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);

  // nsISVGValueObserver
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable);
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable);
  
  // nsISupportsWeakReference
  // implementation inherited from nsSupportsWeakReference

  
  // other methods:
  nsIDOMSVGTransform* ElementAt(PRInt32 index);
  void AppendElement(nsIDOMSVGTransform* aElement);
  void RemoveElementAt(PRInt32 index);
  void InsertElementAt(nsIDOMSVGTransform* aElement, PRInt32 index);
  
protected:
  void ReleaseTransforms();
  
  nsAutoVoidArray mTransforms;
};


#endif //__NS_SVGTRANSFORMLIST_H__
