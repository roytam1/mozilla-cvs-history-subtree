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
 *  Patrick C. Beard <beard@netscape.com>
 *  Simon Fraser     <sfraser@netscape.com>
 */

#include "nsDeviceContextSpecX.h"

#include "prmem.h"
#include "plstr.h"

#include "nsIServiceManager.h"
#include "nsIPrintOptions.h"

/** -------------------------------------------------------
 *  Construct the nsDeviceContextSpecX
 *  @update   dc 12/02/98
 */
nsDeviceContextSpecX::nsDeviceContextSpecX()
: mPrintingContext(0)
, mPageFormat(kPMNoPageFormat)
, mPrintSettings(kPMNoPrintSettings)
, mSavedPort(0)
, mBeganPrinting(PR_FALSE)
{
    NS_INIT_REFCNT();
}

/** -------------------------------------------------------
 *  Destroy the nsDeviceContextSpecX
 *  @update   dc 12/02/98
 */
nsDeviceContextSpecX::~nsDeviceContextSpecX()
{
  ClosePrintManager();
}

NS_IMPL_ISUPPORTS2(nsDeviceContextSpecX, nsIDeviceContextSpec, nsIPrintingContext)

/** -------------------------------------------------------
 *  Initialize the nsDeviceContextSpecMac
 *  @update   dc 12/02/98
 */
NS_IMETHODIMP nsDeviceContextSpecX::Init(PRBool	aQuiet)
{
  nsresult rv;
  nsCOMPtr<nsIPrintOptions> printOptionsService = do_GetService("@mozilla.org/gfx/printoptions;1", &rv);
  if (NS_FAILED(rv)) return rv;

  // Because page setup can get called at any time, we can't use the session APIs here.
  OSStatus status = ::PMBegin();
  if (status != noErr) return NS_ERROR_FAILURE;

  mBeganPrinting = PR_TRUE;
  
  PMPageFormat    optionsPageFormat = kPMNoPageFormat;
  rv = printOptionsService->GetNativeData(nsIPrintOptions::kNativeDataPrintRecord, (void **)&optionsPageFormat);
  if (NS_FAILED(rv)) return rv;
  
  status = ::PMNewPageFormat(&mPageFormat);
  if (status != noErr) return NS_ERROR_FAILURE;
  
  if (optionsPageFormat != kPMNoPageFormat)
  {
    status = ::PMCopyPageFormat(optionsPageFormat, mPageFormat);
    ::PMDisposePageFormat(optionsPageFormat);
  }
  else
    status = ::PMDefaultPageFormat(mPageFormat);

  if (status != noErr) return NS_ERROR_FAILURE;
  
  Boolean validated;
  ::PMValidatePageFormat(mPageFormat, &validated);
  
  status = ::PMNewPrintSettings(&mPrintSettings);
  if (status != noErr) return NS_ERROR_FAILURE;
  
  status = ::PMDefaultPrintSettings(mPrintSettings);
  if (status != noErr) return NS_ERROR_FAILURE;

  if (! aQuiet)
  {
    ::InitCursor();

    Boolean accepted = false;
    status = ::PMPrintDialog(mPrintSettings, mPageFormat, &accepted);
    if (! accepted)
        return NS_ERROR_ABORT;

    if (status != noErr)
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::PrintManagerOpen(PRBool* aIsOpen)
{
    *aIsOpen = mBeganPrinting;
    return NS_OK;
}

/** -------------------------------------------------------
 * Closes the printmanager if it is open.
 * @update   dc 12/03/98
 */
NS_IMETHODIMP nsDeviceContextSpecX::ClosePrintManager()
{
  if (mPrintSettings != kPMNoPrintSettings)
    ::PMDisposePrintSettings(mPrintSettings);

  if (mPageFormat != kPMNoPageFormat)
    ::PMDisposePageFormat(mPageFormat);

  if (mBeganPrinting)
    ::PMEnd();
    
	return NS_OK;
}  

NS_IMETHODIMP nsDeviceContextSpecX::BeginDocument()
{
    OSStatus status = ::PMBeginDocument(mPrintSettings, mPageFormat, &mPrintingContext);
    if (status != noErr) return NS_ERROR_FAILURE;
    
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndDocument()
{
    ::PMEndDocument(mPrintingContext);
    mPrintingContext = 0;
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::BeginPage()
{
	// see http://devworld.apple.com/techpubs/carbon/graphics/CarbonPrintingManager/Carbon_Printing_Manager/Functions/PMSessionBeginPage.html
    OSStatus status = ::PMBeginPage(mPrintingContext, NULL);
    if (status != noErr) return NS_ERROR_FAILURE;
    
    ::GetPort(&mSavedPort);
    GrafPtr printingPort;
    status = ::PMGetGrafPtr(mPrintingContext, &printingPort);
    if (status != noErr) return NS_ERROR_FAILURE;
    ::SetPort(printingPort);
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndPage()
{
    OSStatus status = ::PMEndPage(mPrintingContext);
    if (mSavedPort)
    {
        ::SetPort(mSavedPort);
        mSavedPort = 0;
    }
    if (status != noErr) return NS_ERROR_FAILURE;
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::GetPrinterResolution(double* aResolution)
{
    PMResolution defaultResolution;
    OSStatus status = ::PMGetPrinterResolution(kPMDefaultResolution, &defaultResolution);
    if (status == noErr)
        *aResolution = defaultResolution.hRes;
    
    return (status == noErr ? NS_OK : NS_ERROR_FAILURE);
}

NS_IMETHODIMP nsDeviceContextSpecX::GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight)
{
    PMRect pageRect;
    ::PMGetAdjustedPageRect(mPageFormat, &pageRect);
    *aTop = pageRect.top, *aLeft = pageRect.left;
    *aBottom = pageRect.bottom, *aRight = pageRect.right;
    return NS_OK;
}
