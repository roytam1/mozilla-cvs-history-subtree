/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Initial Developer of the Original Code is
 * IBM Corporation.   Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifdef IBMBIDI
#include "nsCom.h"
#include "pratom.h"
#include "nsUUDll.h"
#include "nsISupports.h"
#include "nsBidiCategoryImp.h"
#include "bidicattable.h"
#include "symmtable.h"

NS_DEFINE_IID(kIUBidiCategoryIID, NS_IUBIDICATEGORY_IID);

NS_IMPL_ISUPPORTS(nsBidiCategoryImp, kIUBidiCategoryIID);


nsBidiCategoryImp::nsBidiCategoryImp()
{
	NS_INIT_REFCNT();
	PR_AtomicIncrement(&g_InstanceCount);
}

nsBidiCategoryImp::~nsBidiCategoryImp()
{
	PR_AtomicDecrement(&g_InstanceCount);
}

NS_IMETHODIMP nsBidiCategoryImp::Get(PRUnichar aChar, eBidiCategory* oResult)
{
	*oResult = GetBidiCat(aChar);
	if( eBidiCat_CC == *oResult)
		*oResult = (eBidiCategory)(aChar & 0xFF); /* Control codes have special treatment to keep the tables smaller */
	return NS_OK;
}

NS_IMETHODIMP nsBidiCategoryImp::Is( PRUnichar aChar, eBidiCategory aBidiCategory, PRBool* oResult)
{
	eBidiCategory bCat = GetBidiCat(aChar);
	if (eBidiCat_CC == bCat)
		bCat = (eBidiCategory)(aChar & 0xFF);
	*oResult = (bCat == aBidiCategory);
  return NS_OK;
}

NS_IMETHODIMP nsBidiCategoryImp::IsControl( PRUnichar aChar, PRBool* oResult)
{
#define LRM_CHAR 0x200e
    *oResult = (eBidiCat_CC == GetBidiCat(aChar) || ((aChar)&0xfffe)==LRM_CHAR);
	return NS_OK;
}

NS_IMETHODIMP nsBidiCategoryImp::GetICU( PRUnichar aChar, UCharDirection* oResult)
{
	eBidiCategory bCat = GetBidiCat(aChar);
	if (eBidiCat_CC != bCat) {
		*oResult = ebc2ucd[bCat];
	} else {
		*oResult = cc2ucd[aChar - 0x202a];
	}
	return NS_OK;
}

NS_IMETHODIMP nsBidiCategoryImp::SymmSwap(PRUnichar* aChar)
{
  switch (*aChar & 0xFF00) {

    case 0x0000:
      *aChar ^= symmtable_00[*aChar & 0xff];
      break;

    case 0x2000:
      *aChar ^= symmtable_20[*aChar & 0xff];
      break;

#ifdef HANDLE_GLYPHS_WITHOUT_MATES // placeholder for code to do something in these cases
    PRUint8 mask;

    case 0x2200:
      mask = symmtable_22[*aChar & 0xff];
      if (GWM == mask)
        ; // Do something
      else
        *aChar ^= mask;
      break;

    case 0x2300:
      mask = symmtable_23[*aChar & 0xff];
      if (GWM == mask)
        ; // Do something
      else
        *aChar ^= mask;
      break;
#else
    case 0x2200:
      *aChar ^= symmtable_22[*aChar & 0xff];
      break;

    case 0x2300:
      *aChar ^= symmtable_23[*aChar & 0xff];
      break;
#endif
    case 0x3000:
      *aChar ^= symmtable_30[*aChar & 0xff];
      break;
  }
  return NS_OK;
}

#endif // IBMBIDI
