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
 * The Initial Developer of the Original Code is
 * IBM Corporation.  Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#ifdef IBMBIDI
#ifndef nsBidiUtilsImp_h__
#define nsBidiUtilsImp_h__

#include "nsCom.h"
#include "nsISupports.h"
#include "nsIUBidiUtils.h"

// IBMBIDI - Egypt - Start
#include "nsITextContent.h"
//#include "nsTextRun.h"
#include "nsTextFragment.h"
#include "nsTextTransformer.h"
//#include "nsBidiOptions.h"
#include "nsIUBidiUtils.h"
//class nsAutoTextBuffer;
// IBMBIDI - Egypt - End

class nsBidiUtilsImp : public nsIUBidiUtils {
   NS_DECL_ISUPPORTS
   
public: 
   nsBidiUtilsImp();
   virtual ~nsBidiUtilsImp();


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

// IBMBIDI - EGYPT - Start
   void HebrewReordering(const PRUnichar *aString, PRUint32 aLen,
   PRUnichar* aBuf, PRUint32 &aBufLen);
   void ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
   PRUnichar* aBuf, PRUint32 &aBufLen, PRUint32* map);
   PRBool NeedComplexScriptHandling(const PRUnichar *aString, PRUint32 aLen,
   PRBool bFontSupportHebrew, PRBool* oHebrew,
   PRBool bFontSupportArabic, PRBool* oArabic);

   void numbers_to_arabic (PRUnichar* uch);
   void numbers_to_hindi (PRUnichar* uch);
   void HandleNumbers (PRUnichar* Buffer, PRUint32 size, PRUint32  Num_Flag);
   void Conv_FE_06 (const nsString aSrc, nsString & aDst );
//ahmed
   void Conv_FE_06_WithReverse(const nsString aSrc, nsString & aDst );
   void Conv_06_FE_WithReverse(const nsString aSrc, nsString & aDst,PRUint32* map );
   PRBool NeedReverseHandling(const PRUnichar *aString, PRUint32 aLen);
   void GetSystem();
   PRBool IsWin95();
   PRBool IsWin98();
   PRBool IsWinNT();
   PRBool m_bWin32s;
   PRBool m_bWinNT;
   PRBool m_bWin4;
   PRBool m_bWin98;
   PRBool m_bWin95;
   PRBool m_bWin5;
 //	Init os version major, minor, build.
   PRInt32 m_dwMajor;
   PRInt32 m_dwMinor;
   PRInt32 m_dwBuild;
// IBMBIDI - EGYPT - End
};

#endif  /* nsBidiUtilsImp_h__ */

#endif // IBMBIDI
