

#include <FixMath.h>

#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"

#include "nsGraphicState.h"
#include "nsFontUtils.h"
#include "nsSpillableBuffer.h"


#include "nsQuickDrawTextRenderer.h"

#define FloatToFixed(a)  ((Fixed)((float)(a) * fixed1))

//-------------------------------------------------------------------------
//  nsQuickDrawTextRenderer
//  
//  inHighQualityRendering param is ignored for QuickDraw.
//-------------------------------------------------------------------------
nsQuickDrawTextRenderer::nsQuickDrawTextRenderer(PRBool inHighQualityRendering)
: nsMacTextRenderer(inHighQualityRendering)
{
}

//-------------------------------------------------------------------------
//  ~nsQuickDrawTextRenderer
//  
//  
//-------------------------------------------------------------------------
nsQuickDrawTextRenderer::~nsQuickDrawTextRenderer()
{

}

//-------------------------------------------------------------------------
//  GetTextWidth
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextWidth(const char* aString,
                PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth)
{
	// set native font and attributes
	SetPortTextState();
  
	// measure text
	short textWidth = ::TextWidth(aString, aStartOffset, aLength);
	aWidth = NSToCoordRound(float(textWidth) * mPixels2Twips);

  return NS_OK;
}

//-------------------------------------------------------------------------
//  GetTextWidth
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextWidth(const PRUnichar* aString,
                PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth)
{
 	nsresult rv = SetPortTextState();
 	if (NS_FAILED(rv))
 		return rv;

	rv = mUnicodeRenderingToolkit.PrepareToDraw(mPixels2Twips, mDeviceContext, mGS, mPort, mRightToLeftText);
	if (NS_SUCCEEDED(rv))
    rv = mUnicodeRenderingToolkit.GetWidth(aString + aStartOffset, aLength, aWidth, 0);

  return rv;
}

//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextDimensions(const char* aString, PRUint32 aStartOffset,
                PRUint32 aLength, nsTextDimensions& aDimensions)
{
  nsresult rv = GetTextWidth(aString, aStartOffset, aLength, aDimensions.width);
  if (NS_SUCCEEDED(rv) && (mGS->mFontMetrics))
  {
    mGS->mFontMetrics->GetMaxAscent(aDimensions.ascent);
    mGS->mFontMetrics->GetMaxDescent(aDimensions.descent);
  }
  return rv;
}

//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextDimensions(const PRUnichar* aString, PRUint32 aStartOffset,
                PRUint32 aLength, nsTextDimensions& aDimensions)
{
  nsresult rv = SetPortTextState();
  if (NS_FAILED(rv))
    return rv;

  rv = mUnicodeRenderingToolkit.PrepareToDraw(mPixels2Twips, mDeviceContext, mGS, mPort, mRightToLeftText);
	if (NS_SUCCEEDED(rv))
    rv = mUnicodeRenderingToolkit.GetTextDimensions(aString + aStartOffset, aLength, aDimensions, 0);
    
  return rv;
}


//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextDimensions(const char*  inString,
                           PRInt32           inLength,
                           nscoord           inAvailWidth,
                           const PRInt32*    inWordBreaks,
                           PRInt32           inNumBreaks,
                           nsTextDimensions& outDimensions,
                           PRInt32&          outNumCharsFit,
                           nsTextDimensions& outLastWordDimensions)
{

	// set native font and attributes
	SetPortTextState();

  // aLastWordDimensions.width should be set to -1 to reply that we don't
  // know the width of the last word since we measure multiple words
  outLastWordDimensions.Clear();
  outLastWordDimensions.width = -1;

  mGS->mFontMetrics->GetMaxAscent(outDimensions.ascent);
  mGS->mFontMetrics->GetMaxDescent(outDimensions.descent);

  if (inLength == 0)
  {
    outDimensions.width = 0;
    outNumCharsFit = 0;
    return NS_OK;
  }
  
  PRBool  cacheGood;
  nsBreakLengthCache<char>    breaksCache(*this, inString, inLength, inNumBreaks, inWordBreaks, &cacheGood);
  if (!cacheGood) return NS_ERROR_OUT_OF_MEMORY;
  
  // do binary search to find the break offset
  PRInt32     startBreakIndex     = 0;
  PRInt32     endBreakIndex       = inNumBreaks - 1;
  PRInt32     foundBreakIndex     = -1;
  
  while (endBreakIndex >= startBreakIndex)
  {
    PRInt32   midIndex  = (startBreakIndex + endBreakIndex) / 2;
    
    nscoord midLength = breaksCache.GetWidthForBreakIndex(midIndex);
    
    if (midLength > inAvailWidth)
    {
      endBreakIndex = midIndex - 1;
      if (endBreakIndex < startBreakIndex)
      {
        if (midIndex > 0)
          foundBreakIndex = midIndex - 1;
        break;
      }
    }
    else
    {
      startBreakIndex = midIndex + 1;
      if (startBreakIndex > endBreakIndex)
      {
        if (midIndex >= 0 && midIndex < inNumBreaks)
          foundBreakIndex = midIndex;          
        break;
      }
    }
  }
  
  // this will only happen when the text up to the first break doesn't fit,
  // in which case layout wants us to return the  width up to the first break
  if (foundBreakIndex == -1)
    foundBreakIndex = (inNumBreaks > 1) ? 1 : 0;
    
  outNumCharsFit = inWordBreaks[foundBreakIndex];
  outDimensions.width = breaksCache.GetWidthForBreakIndex(foundBreakIndex);

  outLastWordDimensions.ascent = outDimensions.ascent;
  outLastWordDimensions.descent = outDimensions.descent;
    
  return NS_OK;
}


//-------------------------------------------------------------------------
//  GetTextDimensions
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::GetTextDimensions(const PRUnichar*  inString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions)
{

	// set native font and attributes
	SetPortTextState();

  // aLastWordDimensions.width should be set to -1 to reply that we don't
  // know the width of the last word since we measure multiple words
  outLastWordDimensions.Clear();
  outLastWordDimensions.width = -1;

  mGS->mFontMetrics->GetMaxAscent(outDimensions.ascent);
  mGS->mFontMetrics->GetMaxDescent(outDimensions.descent);

  if (inLength == 0)
  {
    outDimensions.width = 0;
    outNumCharsFit = 0;
    return NS_OK;
  }
  
  PRBool  cacheGood;
  nsBreakLengthCache<PRUnichar>    breaksCache(*this, inString, inLength, inNumBreaks, inWordBreaks, &cacheGood);
  if (!cacheGood) return NS_ERROR_OUT_OF_MEMORY;
  
  // do binary search to find the break offset
  PRInt32     startBreakIndex     = 0;
  PRInt32     endBreakIndex       = inNumBreaks - 1;
  PRInt32     foundBreakIndex     = -1;
  
  while (endBreakIndex >= startBreakIndex)
  {
    PRInt32   midIndex  = (startBreakIndex + endBreakIndex) / 2;
    
    nscoord midLength = breaksCache.GetWidthForBreakIndex(midIndex);
    
    if (midLength > inAvailWidth)
    {
      endBreakIndex = midIndex - 1;
      if (endBreakIndex < startBreakIndex)
      {
        if (midIndex > 0)
          foundBreakIndex = midIndex - 1;
        break;
      }
    }
    else
    {
      startBreakIndex = midIndex + 1;
      if (startBreakIndex > endBreakIndex)
      {
        if (midIndex >= 0 && midIndex < inNumBreaks)
          foundBreakIndex = midIndex;          
        break;
      }
    }
  }
  
  // this will only happen when the text up to the first break doesn't fit,
  // in which case layout wants us to return the  width up to the first break
  if (foundBreakIndex == -1)
    foundBreakIndex = (inNumBreaks > 1) ? 1 : 0;
    
  outNumCharsFit = inWordBreaks[foundBreakIndex];
  outDimensions.width = breaksCache.GetWidthForBreakIndex(foundBreakIndex);

  outLastWordDimensions.ascent = outDimensions.ascent;
  outLastWordDimensions.descent = outDimensions.descent;
    
  return NS_OK;
}

//-------------------------------------------------------------------------
//  DrawText
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength,
                  nscoord aX, nscoord aY, const nscoord* aSpacing)
{
	PRInt32 x = aX;
	PRInt32 y = aY;
	
	if (mGS->mFontMetrics) {
		// set native font and attributes
		SetPortTextState();
	}

	mGS->mTMatrix.TransformCoord(&x,&y);

	::MoveTo(x,y);
	if ( aSpacing == NULL )
		::DrawText(aString, aStartOffset, aLength);
	else
	{
	  nsSpillableBuffer<PRIntn>   spacingBuffer(aLength);
	  if (spacingBuffer.GetCapacity() < aLength)
	    return NS_ERROR_OUT_OF_MEMORY;
	  
	  PRIntn* spacingArray = spacingBuffer.GetBuffer();
		mGS->mTMatrix.ScaleXCoords(aSpacing, aLength, spacingArray);

		PRInt32 currentX = x;
		const char* stringStart = aString + aStartOffset;

		for (PRInt32 i = 0; i < aLength; i++)
		{
			::DrawChar(stringStart[i]);
			currentX += spacingArray[i];
			::MoveTo(currentX, y);
		}
	}

  return NS_OK;
}

//-------------------------------------------------------------------------
//  DrawText
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::DrawText(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength,
                  nscoord aX, nscoord aY, const nscoord* aSpacing)
{
 	nsresult rv = SetPortTextState();
 	if (NS_FAILED(rv))
 		return rv;

	NS_PRECONDITION(mGS->mFontMetrics != nsnull, "No font metrics in SetPortTextState");
	
	if (nsnull == mGS->mFontMetrics)
		return NS_ERROR_NULL_POINTER;

	rv = mUnicodeRenderingToolkit.PrepareToDraw(mPixels2Twips, mDeviceContext, mGS, mPort,mRightToLeftText);
	if (NS_SUCCEEDED(rv))
		rv = mUnicodeRenderingToolkit.DrawString(aString + aStartOffset, aLength, aX, aY, 0, aSpacing);

  return rv;
}

#pragma mark -


//-------------------------------------------------------------------------
//  DrawText
//  
//  Default implementation expands to Unicode, then calls the unicode method
//-------------------------------------------------------------------------
nsresult
nsQuickDrawTextRenderer::SetPortTextState()
{
	NS_PRECONDITION(mGS->mFontMetrics != nsnull, "No font metrics in SetPortTextState");
	
	if (nsnull == mGS->mFontMetrics)
		return NS_ERROR_NULL_POINTER;

	NS_PRECONDITION(mDeviceContext != nsnull, "No device context in SetPortTextState");
	
	if (nsnull == mDeviceContext)
		return NS_ERROR_NULL_POINTER;

	TextStyle		theStyle;
	nsFontUtils::GetNativeTextStyle(*mGS->mFontMetrics, *mDeviceContext, theStyle);

	::TextFont(theStyle.tsFont);
	::TextSize(theStyle.tsSize);
	::TextFace(theStyle.tsFace);

	return NS_OK;
}
