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
 * The Original Code is IBM code.
 * 
 * The Initial Developer of the Original Code is IBM.
 * Portions created by IBM are
 * Copyright (C) International Business Machines
 * Corporation, 2000.  All Rights Reserved.
 * 
 * Contributor(s): Simon Montagu
 */

#include "nsBidiKeyboard.h"
#include <windows.h>

static NS_DEFINE_IID(kIBidiKeyboardIID, NS_IBIDIKEYBOARD_IID);

NS_IMPL_ISUPPORTS(nsBidiKeyboard, NS_IBIDIKEYBOARD_IID)

#define RTL_LANGS 17

char langID[RTL_LANGS][KL_NAMELENGTH]=
{
  "00000401", //Arabic (Saudi Arabia)
  "00000801", //Arabic (Iraq)
  "00000c01", //Arabic (Egypt)
  "00001001", //Arabic (Libya)
  "00001401", //Arabic (Algeria)
  "00001801", //Arabic (Morocco)
  "00001c01", //Arabic (Tunisia)
  "00002001", //Arabic (Oman)
  "00002401", //Arabic (Yemen)
  "00002801", //Arabic (Syria)
  "00002c01", //Arabic (Jordan)
  "00003001", //Arabic (Lebanon)
  "00003401", //Arabic (Kuwait)
  "00003801", //Arabic (U.A.E.)
  "00003c01", //Arabic (Bahrain)
  "00004001", //Arabic (Qatar)
  "0000040d"  //Hebrew
};

// XXX - need a cross-language solution for this
#define kRTLLanguage 	"0000040d"  //Hebrew
//#define kRTLLanguage 	"00000c01", //Arabic (Egypt)
#define kLTRLanguage 	"00000409"  //US English

nsBidiKeyboard::nsBidiKeyboard() : nsIBidiKeyboard()
{
  NS_INIT_REFCNT();
}

nsBidiKeyboard::~nsBidiKeyboard()
{
}

NS_IMETHODIMP nsBidiKeyboard::IsLangRTL(PRBool *aIsRTL)
{
#ifdef IBMBIDI
  char currentLang[KL_NAMELENGTH]="00000000";    // to get keyboar layout name in this array

  *aIsRTL = PR_FALSE;
  GetKeyboardLayoutName(currentLang);
  for(int count=0;count<RTL_LANGS;count++) {
    if(strcmp(currentLang,langID[count])==0) {
      *aIsRTL = PR_TRUE;
      break;
    }
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP nsBidiKeyboard::SetLangFromBidiLevel(PRUint8 aLevel)
{
#ifdef IBMBIDI
  LoadKeyboardLayout((aLevel & 1) ? kRTLLanguage : kLTRLanguage, KLF_ACTIVATE);
#endif
  return NS_OK;
}
