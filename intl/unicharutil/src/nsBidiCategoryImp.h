/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 */
#ifdef IBMBIDI
#ifndef nsBidiCategoryImp_h__
#define nsBidiCategoryImp_h__

#include "nsCom.h"
#include "nsISupports.h"
#include "nsIUBidiCategory.h"

class nsBidiCategoryImp : public nsIUBidiCategory {
   NS_DECL_ISUPPORTS
   
public: 
   nsBidiCategoryImp();
   virtual ~nsBidiCategoryImp();


   /**
    * Give a Unichar, return an eBidiCategory
    */
   NS_IMETHOD Get( PRUnichar aChar, eBidiCategory* oResult);
    
   /**
    * Give a Unichar, and an eBidiCategory, 
    * return PR_TRUE if the Unichar is in that category, 
    * return PR_FALSE, otherwise
    */
   NS_IMETHOD Is( PRUnichar aChar, eBidiCategory aBidiCategory, PRBool* oResult);
   
   /**
    * Give a Unichar
    * return PR_TRUE if the Unichar is a Bidi control character (LRE, RLE, PDF, LRO, RLO, LRM, RLM)
    * return PR_FALSE, otherwise
    */
   NS_IMETHOD IsControl( PRUnichar aChar, PRBool* oResult);

   /**
    * Give a Unichar, return a UCharDirection (compatible with ICU)
    */
   NS_IMETHOD GetICU( PRUnichar aChar, UCharDirection* oResult);

   /**
    * Give a Unichar, return the symmetric equivalent
    */
   NS_IMETHOD SymmSwap( PRUnichar* aChar);

};

#endif  /* nsBidiCategoryImp_h__ */

#endif // IBMBIDI
