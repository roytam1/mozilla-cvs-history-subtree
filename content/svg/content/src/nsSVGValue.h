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

#ifndef __NS_SVGVALUE_H__
#define __NS_SVGVALUE_H__

#include "nsISVGValue.h"
#include "nsGenericElement.h" // for CheapVoidArray
#include "nsISVGValueObserver.h"

// XXX this should be somewhere else
#if defined(XP_PC) && !defined(XP_OS2)
#define STDCALL __stdcall
#else
#define STDCALL
#endif

typedef nsresult
(STDCALL nsISVGValueObserver::*SVGObserverNotifyFunction)(nsISVGValue*);


class nsSVGValue : public nsISVGValue
{
protected:
  nsSVGValue();
  virtual ~nsSVGValue();

  // to be called by subclass whenever value is being modified.
  // nested calls will be ignored, so calls need to be balanced
  void WillModify();
  void DidModify();
  
public:
  // Partial Implementation of nsISVGValue interface:
  NS_IMETHOD AddObserver(nsISVGValueObserver* observer);
  NS_IMETHOD RemoveObserver(nsISVGValueObserver* observer);
  
protected:
  // implementation helpers
  void ReleaseObservers();
  
  void NotifyObservers(SVGObserverNotifyFunction f);
  
private:
  nsCheapVoidArray mObservers;
  PRInt32 mModifyNestCount;
};


#endif //__NS_SVGVALUE_H__
