

#include "nsIDeviceContext.h"

#include "nsString.h"
#include "nsGraphicState.h"


#include "nsMacTextRenderer.h"



//-------------------------------------------------------------------------
//  nsMacTextRenderer
// 
//-------------------------------------------------------------------------
nsMacTextRenderer::nsMacTextRenderer(PRBool inHighQualityRendering)
: mPort(nsnull)
, mDeviceContext(nsnull)
, mGS(nsnull)
, mPixels2Twips(1.0f)
, mRightToLeftText(PR_FALSE)
, mHighQualityRendering(inHighQualityRendering)
{
}


//-------------------------------------------------------------------------
//  ~nsMacTextRenderer
// 
//-------------------------------------------------------------------------
nsMacTextRenderer::~nsMacTextRenderer()
{
}


//-------------------------------------------------------------------------
//  SetupDrawingState
// 
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::SetupDrawingState(CGrafPtr inPort, nsIDeviceContext* inDeviceContext, nsGraphicState* inGS)
{
  mPort = inPort;
  mDeviceContext = inDeviceContext;
  mGS = inGS;

  NS_ASSERTION(mDeviceContext, "No device context");
  mDeviceContext->GetDevUnitsToAppUnits(mPixels2Twips);
  return NS_OK;
}

//-------------------------------------------------------------------------
//  ClearDrawingState
// 
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::ClearDrawingState()
{
  mPort = nsnull;
  mDeviceContext = nsnull;
  mGS = nsnull;
  return NS_OK;
}

//-------------------------------------------------------------------------
//  GetTextWidth
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::GetTextWidth(const char* aString,
                PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth)
{
  return GetTextWidth(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aWidth);
}


//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::GetTextDimensions(const char* aString, PRUint32 aStartOffset,
                PRUint32 aLength, nsTextDimensions& aDimensions)
{
  return GetTextDimensions(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aDimensions);
}


//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::GetTextDimensions(const char*  inString,
                           PRInt32           inLength,
                           nscoord           inAvailWidth,
                           const PRInt32*    inWordBreaks,
                           PRInt32           inNumBreaks,
                           nsTextDimensions& outDimensions,
                           PRInt32&          outNumCharsFit,
                           nsTextDimensions& outLastWordDimensions)
{
  return GetTextDimensions(NS_ConvertASCIItoUCS2(inString).get(), inLength, inAvailWidth, inWordBreaks, inNumBreaks,
          outDimensions, outNumCharsFit, outLastWordDimensions);
}


//-------------------------------------------------------------------------
//  DrawText
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsMacTextRenderer::DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength,
                  nscoord aX, nscoord aY, const nscoord* aSpacing)
{
  return DrawText(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aX, aY, aSpacing);
}

