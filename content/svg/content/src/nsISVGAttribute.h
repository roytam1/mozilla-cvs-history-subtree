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


#ifndef __NS_ISVGATTRIBUTE_H__
#define __NS_ISVGATTRIBUTE_H__

#include "nsIDOMAttr.h"

class nsISVGValue;

////////////////////////////////////////////////////////////////////////
// nsISVGAttribute: private interface for svg attributes

// {6557CCDF-7252-481d-8AB0-7E083E7E7AB0}
#define NS_ISVGATTRIBUTE_IID \
{ 0x6557ccdf, 0x7252, 0x481d, { 0x8a, 0xb0, 0x7e, 0x8, 0x3e, 0x7e, 0x7a, 0xb0 } }

class nsISVGAttribute : public nsIDOMAttr
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISVGATTRIBUTE_IID; return iid; }

  NS_IMETHOD GetSVGValue(nsISVGValue** value) = 0;
};

#endif // __NS_ISVGATTRIBUTE_H__

