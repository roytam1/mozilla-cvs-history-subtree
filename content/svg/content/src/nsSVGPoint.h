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

#ifndef __NS_SVGPOINT_H__
#define __NS_SVGPOINT_H__

#include "nsIDOMSVGPoint.h"
#include "nsSVGValue.h"

class nsSVGPoint : public nsIDOMSVGPoint,
                   public nsSVGValue
{
public:
  static nsresult Create(float x, float y, nsIDOMSVGPoint** aResult);
  
protected:
  nsSVGPoint(float x, float y);
  
public:
  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsIDOMSVGPoint interface:
  NS_DECL_NSIDOMSVGPOINT

  // nsISVGValue interface:
  NS_IMETHOD SetValueString(const nsAReadableString& aValue);
  NS_IMETHOD GetValueString(nsAWritableString& aValue);
  
  
protected:
  float mX;
  float mY;
};


#endif //__NS_SVGPOINT_H__
