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
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributor(s):
 */

#ifndef nsRegion_h___
#define nsRegion_h___

#include "nsIRegion.h"

#include "windows.h"

#define NS_REGION_CID \

class nsRegion : public nsIRegion
{
public:
  nsRegion();
  virtual ~nsRegion();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREGION

protected:
  LPRGNDATA mData;
  PRUint32  mDataSize;

  HRGN      mRegion;
  int       mRegionType;
};

#endif  // nsRegion_h___ 
