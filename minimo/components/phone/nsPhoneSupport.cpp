/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Phone Support for Minimo
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Doug Turner <dougt@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WINCE
#include "windows.h"
#include "phone.h"
#include "sms.h"
#endif

#include "nsString.h"
#include "nsIPhoneSupport.h"
#include "nsIGenericFactory.h"

class nsPhoneSupport : public nsIPhoneSupport
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPHONESUPPORT
  
  nsPhoneSupport() {};
  ~nsPhoneSupport(){};
  
};

NS_IMPL_ISUPPORTS1(nsPhoneSupport, nsIPhoneSupport)

NS_IMETHODIMP
nsPhoneSupport::MakeCall(const PRUnichar *telephoneNumber, const PRUnichar *telephoneDescription)
{
#ifdef WINCE
  long result = -1;
  
  typedef LONG (*__PhoneMakeCall)(PHONEMAKECALLINFO *ppmci);
  
  HMODULE hPhoneDLL = LoadLibrary("phone.dll"); 
  if(hPhoneDLL)
  {
    __PhoneMakeCall MakeCall = (__PhoneMakeCall) GetProcAddress( hPhoneDLL,
                                                                 "PhoneMakeCall");
    if(MakeCall)
    {
      PHONEMAKECALLINFO callInfo;
      callInfo.cbSize          = sizeof(PHONEMAKECALLINFO);
      callInfo.dwFlags         = PMCF_PROMPTBEFORECALLING;
      callInfo.pszDestAddress  = telephoneNumber;
      callInfo.pszAppName      = nsnull;
      callInfo.pszCalledParty  = telephoneDescription;
      callInfo.pszComment      = nsnull; 
      
      result = MakeCall(&callInfo);
    }
    FreeLibrary(hPhoneDLL);
  } 
  return (result == 0) ? NS_OK : NS_ERROR_FAILURE;
  
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

NS_IMETHODIMP 
nsPhoneSupport::SendSMS(const PRUnichar *smsDest, const PRUnichar *smsMessage)
{
#ifdef WINCE
  // wince -- this doesn't work yet.
  typedef HRESULT (*__SmsOpen)(const LPCWSTR ptsMessageProtocol,
                               const DWORD dwMessageModes,
                               SMS_HANDLE* const psmshHandle,
                               HANDLE* const phMessageAvailableEvent);
  
  typedef HRESULT (*__SmsSendMessage)(const SMS_HANDLE smshHandle,
                                      const SMS_ADDRESS * const psmsaSMSCAddress,
                                      const SMS_ADDRESS * const psmsaDestinationAddress,
                                      const SYSTEMTIME * const pstValidityPeriod,
                                      const BYTE * const pbData,
                                      const DWORD dwDataSize,
                                      const BYTE * const pbProviderSpecificData,
                                      const DWORD dwProviderSpecificDataSize,
                                      const SMS_DATA_ENCODING smsdeDataEncoding,
                                      const DWORD dwOptions,
                                      SMS_MESSAGE_ID * psmsmidMessageID);
  
  typedef HRESULT (*__SmsClose)(const SMS_HANDLE smshHandle);
  
  HMODULE hSmsDLL = LoadLibrary("sms.dll"); 
  if(hSmsDLL)
  {
    __SmsOpen        smsOpen  = (__SmsOpen) GetProcAddress(hSmsDLL, "SmsOpen");
    __SmsSendMessage smsSend  = (__SmsSendMessage) GetProcAddress(hSmsDLL, "SmsSendMessage");
    __SmsClose       smsClose = (__SmsClose) GetProcAddress(hSmsDLL, "SmsClose");
    
    if (!smsOpen || !smsSend || !smsClose)
      return NS_ERROR_NOT_AVAILABLE;
    
    SMS_HANDLE smshHandle;
    SMS_ADDRESS smsaDestination;
    TEXT_PROVIDER_SPECIFIC_DATA tpsd;
    SMS_MESSAGE_ID smsmidMessageID = 0;
    
    // try to open an SMS Handle
    HRESULT hr = smsOpen(L"Microsoft Text SMS Protocol", SMS_MODE_SEND, &smshHandle, NULL);
    
    if (hr == SMS_E_INVALIDPROTOCOL)
    {
      return NS_ERROR_NOT_AVAILABLE;
    }
    
    if (hr != ERROR_SUCCESS)
    {
      return NS_ERROR_FAILURE;
    }
    
    // Create the destination address
    memset (&smsaDestination, 0, sizeof (smsaDestination));
    smsaDestination.smsatAddressType = SMSAT_INTERNATIONAL;
    memcpy((void*) smsaDestination.ptsAddress, (void*) smsDest, 100);
    
    // Set up provider specific data
    tpsd.dwMessageOptions = PS_MESSAGE_OPTION_NONE;
    tpsd.psMessageClass = PS_MESSAGE_CLASS1;
    tpsd.psReplaceOption = PSRO_NONE;
    tpsd.dwHeaderDataSize = 0;
    
    // Send the message, indicating success or failure
    hr = smsSend(smshHandle,
                 NULL, 
                 &smsaDestination,
                 NULL,
                 (PBYTE) smsMessage, 
                 wcslen(smsMessage) * sizeof (WCHAR), 
                 (PBYTE) &tpsd, 
                 sizeof(TEXT_PROVIDER_SPECIFIC_DATA), /*12*/ 
                 SMSDE_OPTIMAL, 
                 SMS_OPTION_DELIVERY_NONE, 
                 &smsmidMessageID);
    
    smsClose (smshHandle);
    
    if (hr == ERROR_SUCCESS)
      return NS_OK;
    
    FreeLibrary(hSmsDLL);
  }
  return NS_ERROR_FAILURE;
  
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


//------------------------------------------------------------------------------
//  XPCOM REGISTRATION BELOW
//------------------------------------------------------------------------------

#define nsPhoneSupport_CID                          \
{ 0x2a08c9e4, 0xf853, 0x4f02,                       \
{0x88, 0xd8, 0xd6, 0x2f, 0x27, 0xca, 0x06, 0x85} }

#define nsPhoneSupport_ContractID "@mozilla.org/phone/support;1"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPhoneSupport)

static const nsModuleComponentInfo components[] =
{
  { "Phone Support", 
    nsPhoneSupport_CID, 
    nsPhoneSupport_ContractID,
    nsPhoneSupportConstructor,
    nsnull,
    nsnull
  }  
};

NS_IMPL_NSGETMODULE(nsPhoneSupportModule, components)
