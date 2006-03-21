/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 *
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by the Initial Developer are
 * Copyright (C) 2003 the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Joe Hewitt <hewitt@netscape.com>
 *    Peter Amstutz <tetron@interreality.org>
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
 */

#include "membufDeviceContext.h"

#include "membufRenderingContext.h"

#include "nsCOMPtr.h"
#include "nsIView.h"

NS_IMPL_ISUPPORTS_INHERITED0(membufDeviceContext, DeviceContextImpl)

membufDeviceContext::membufDeviceContext()
{
    printf("membufDeviceContext::membufDeviceContext()[%x]\n", this);
    NS_INIT_ISUPPORTS();

    mDevUnitsToAppUnits = 1.0f;
    mAppUnitsToDevUnits = 1.0f;
    mCPixelScale = 1.0f;
    mZoom = 1.0f;
    //mTextZoom = 1.0f;
}

membufDeviceContext::~membufDeviceContext()
{
}

NS_IMETHODIMP
membufDeviceContext::Init( nsNativeWidget aWidget )
{
    printf("membufDeviceContext::Init()[%x]\n", this);
    mTwipsToPixels = 96 / (float)NSIntPointsToTwips(72);
    mPixelsToTwips = 1.0f / mTwipsToPixels;

    //DeviceContextImpl::CommonInit();

    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::CreateRenderingContext( nsIView *aView,
                                             nsIRenderingContext *&aContext )
{
    printf("membufDeviceContext::CreateRenderingContext(1)\n");
    nsCOMPtr<nsIWidget> widget = aView->GetWidget();

    return CreateRenderingContext(widget, aContext);
}

NS_IMETHODIMP
membufDeviceContext::CreateRenderingContext( nsIDrawingSurface* aSurface,
                                             nsIRenderingContext *&aContext )
{
    printf("membufDeviceContext::CreateRenderingContext(2)\n");
    nsresult rv;

    aContext = nsnull;
    nsCOMPtr<nsIRenderingContext> pContext;
    rv = CreateRenderingContextInstance(*getter_AddRefs(pContext));
    if (NS_SUCCEEDED(rv)) {
        rv = pContext->Init(this, aSurface);
        if (NS_SUCCEEDED(rv)) {
            aContext = pContext;
            NS_ADDREF(aContext);
        }
    }

    return rv;
}

NS_IMETHODIMP
membufDeviceContext::CreateRenderingContext( nsIWidget *aWidget,
                                             nsIRenderingContext *&aContext )
{
    printf("membufDeviceContext::CreateRenderingContext(3)\n");
    nsresult rv;

    aContext = nsnull;
    nsCOMPtr<nsIRenderingContext> pContext;
    rv = CreateRenderingContextInstance(*getter_AddRefs(pContext));
    if (NS_SUCCEEDED(rv)) {
        rv = pContext->Init(this, aWidget);
        if (NS_SUCCEEDED(rv)) {
            aContext = pContext;
            NS_ADDREF(aContext);
        }
    }

    return rv;
}

NS_IMETHODIMP membufDeviceContext::CreateRenderingContext(nsIRenderingContext *&aContext)
{
    printf("membufDeviceContext::CreateRenderingContext(4) -- danger!!!! \n");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP membufDeviceContext::CreateRenderingContextInstance(nsIRenderingContext *&aContext)
{
    printf("membufDeviceContext::CreateRenderingContextInstance()\n");
    nsCOMPtr<nsIRenderingContext> renderingContext = new membufRenderingContext();
    if (!renderingContext)
        return NS_ERROR_OUT_OF_MEMORY;

    aContext = renderingContext;
    NS_ADDREF(aContext);

    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
    aSupportsWidgets = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::GetScrollBarDimensions(float &aWidth, float &aHeight) const
{
    aWidth = 10.0f * mPixelsToTwips;
    aHeight = 10.0f * mPixelsToTwips;
    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::GetDrawingSurface(nsIRenderingContext &aContext,
                                       nsIDrawingSurface *&aSurface)
{
    //aContext.CreateDrawingSurface(nsnull, 0, aSurface);
    printf("XXXjgaunt - DANGER - using rendering context without a check\n");
    nsIDrawingSurface *tempSurface;
    aContext.GetDrawingSurface(&tempSurface);
    if (tempSurface)
      aSurface = tempSurface;
    return nsnull == aSurface ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}


NS_IMETHODIMP
membufDeviceContext::CheckFontExistence(const nsString &aFaceName)
{
    return NS_OK;
}

NS_IMETHODIMP
membufDeviceContext::GetDepth(PRUint32 &aDepth)
{
    aDepth = 32;
    return NS_OK;
}


NS_IMETHODIMP
membufDeviceContext::GetPaletteInfo(nsPaletteInfo &aPaletteInfo)
{
    aPaletteInfo.isPaletteDevice = PR_FALSE;
    aPaletteInfo.sizePalette = 0;
    aPaletteInfo.numReserved = 0;
    aPaletteInfo.palette = nsnull;
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::ConvertPixel(nscolor aColor, PRUint32 & aPixel)
{
    aPixel = aColor;
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::GetDeviceSurfaceDimensions(PRInt32 &aWidth, PRInt32 &aHeight)
{
    aWidth = NSToIntRound(float(1000) * mDevUnitsToAppUnits);
    aHeight = NSToIntRound(float(1000) * mDevUnitsToAppUnits);
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::GetRect(nsRect &aRect)
{
    aRect.x = 0;
    aRect.y = 0;
    this->GetDeviceSurfaceDimensions(aRect.width, aRect.height);

    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::GetClientRect(nsRect &aRect)
{
    return this->GetRect(aRect);
}



/*
 * below methods are for printing and are not implemented
 */
NS_IMETHODIMP membufDeviceContext::GetDeviceContextFor(nsIDeviceContextSpec *aDevice,
                                                     nsIDeviceContext *&aContext)
{
    /* we don't do printing */
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP membufDeviceContext::PrepareDocument(PRUnichar * aTitle,
                                                 PRUnichar*  aPrintToFileName)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::BeginDocument(PRUnichar*  aTitle,
                                               PRUnichar*  aPrintToFileName,
                                               PRInt32     aStartPage,
                                               PRInt32     aEndPage)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::EndDocument(void)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::AbortDocument(void)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::BeginPage(void)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::EndPage(void)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::SetAltDevice(nsIDeviceContext* aAltDC)
{
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::GetAltDevice(nsIDeviceContext** aAltDC)
{
    *aAltDC = nsnull;
    return NS_OK;
}


NS_IMETHODIMP membufDeviceContext::SetUseAltDC(PRUint8 aValue, PRBool aOn)
{
    return NS_OK;
}

