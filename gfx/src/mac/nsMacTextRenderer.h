
#ifndef nsMacTextRenderer_h__
#define nsMacTextRenderer_h__


#include <Quickdraw.h>
#include <CGContext.h>

#include "nscore.h"
#include "nsCoord.h"
#include "nsSpillableBuffer.h"


//-------------------------------------------------------------------------
//  nsMacTextRenderer
//  
//  Pure virtual base class for text rendering.
//  
//  Has implementations of methods for char* entry points that simply
//  expand to unicode, and call the unicode methods.
//  
//-------------------------------------------------------------------------

class nsIDeviceContext;
class nsGraphicState;
class nsTextDimensions;


class nsMacTextRenderer
{
public:

                        nsMacTextRenderer(PRBool inHighQualityRendering = PR_TRUE);
  virtual               ~nsMacTextRenderer();
  
  virtual nsresult      SetupDrawingState(CGrafPtr inPort,
                                nsIDeviceContext* inDeviceContext, nsGraphicState* inGS);
  
  virtual nsresult      ClearDrawingState();

  virtual nsresult      GetTextWidth(const char* aString,
                                PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth);
  virtual nsresult      GetTextWidth(const PRUnichar *aString,
                                PRUint32 aStartOffset, PRUint32 aLength, nscoord &aWidth) = 0;
  
  virtual nsresult      GetTextDimensions(const char* aString, PRUint32 aStartOffset,
                                PRUint32 aLength, nsTextDimensions& aDimensions);
  virtual nsresult      GetTextDimensions(const PRUnichar* aString, PRUint32 aStartOffset,
                                PRUint32 aLength, nsTextDimensions& aDimensions) = 0;
  
  virtual nsresult      GetTextDimensions(const char*  inString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions);

  virtual nsresult      GetTextDimensions(const PRUnichar*   aString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions) = 0;

  virtual nsresult      DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength,
                                  nscoord aX, nscoord aY, const nscoord* aSpacing);
  virtual nsresult      DrawText(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength,
                                   nscoord aX, nscoord aY, const nscoord* aSpacing) = 0;
  
  virtual nsresult      SetRightToLeftText(PRBool isRightToLeft) { mRightToLeftText = isRightToLeft; return NS_OK; }

  PRBool                GetHighQualityRendering()                               { return mHighQualityRendering; }
  void                  SetHighQualityRendering(PRBool inHighQualityRendering)  { mHighQualityRendering = inHighQualityRendering; }

protected:

  CGrafPtr              mPort;

  nsIDeviceContext      *mDeviceContext;
  nsGraphicState        *mGS;

  float                 mPixels2Twips;
  
  PRPackedBool          mRightToLeftText;
  PRPackedBool          mHighQualityRendering;
};




//-------------------------------------------------------------------------
//  nsBreakLengthCache
//  
//  This class can be used to maintain a cache of measured text widths,
//  which is useful for implementations of nsIRenderingContext::GetTextDimensions.
//  
//-------------------------------------------------------------------------
template <class T>
class nsBreakLengthCache
{
public:
  typedef T char_type;

private:

                      enum
                      {
                        kUninitializedBreakLength  = -1
                      };
                      
public:
                      nsBreakLengthCache( nsMacTextRenderer& inTextRenderer,
                                          const char_type* inText,
                                          PRInt32         inTextLength,
                                          PRInt32         inNumBreaks,
                                          const PRInt32*  inBreakOffsetsArray,
                                          PRBool          *outGotBuffer)
                      : mTextRenderer(inTextRenderer)
                      , mText(inText)
                      , mTextLength(inTextLength)
                      , mNumBreaks(inNumBreaks)
                      , mBreakOffsetsArray(inBreakOffsetsArray)
                      , mBreakLengthsBuffer(inNumBreaks)
                      , mBreakLengthsArray(nsnull)
                      {
                        mBreakLengthsArray = mBreakLengthsBuffer.GetBuffer();
                        
                        PRBool    gotBuffer = (mBreakLengthsBuffer.GetCapacity() >= mNumBreaks);
                        if (gotBuffer)
                        {
                          for (PRInt32 i = 0; i < inNumBreaks; i ++)
                            mBreakLengthsArray[i] = kUninitializedBreakLength;
                        }
                        *outGotBuffer = gotBuffer;
                      }
                      
                      ~nsBreakLengthCache()
                      {
                      }
                      
  nscoord             GetWidthForBreakIndex(PRInt32 inBreakIndex)
                      {
                        NS_ASSERTION(inBreakIndex >= 0 && inBreakIndex < mNumBreaks, "Bad break index");
                        
                        nscoord   breakWidth = mBreakLengthsArray[inBreakIndex];
                        
                        if (breakWidth == kUninitializedBreakLength)
                        {
                          PRInt32   startMeasureOffset  = 0;
                          nscoord   startLength         = 0;
                          
                          // look for an earlier break that we've already measured
                          for (PRInt32 i = inBreakIndex - 1; i >= 0; i--)
                          {
                            if (mBreakLengthsArray[i] != kUninitializedBreakLength)
                            {
                              startMeasureOffset  = mBreakOffsetsArray[i];
                              startLength         = mBreakLengthsArray[i];
                              break;
                            }
                          }
                          
                          // now measure from the last known point
                          nscoord   segmentWidth;
                          mTextRenderer.GetTextWidth(mText, startMeasureOffset, mBreakOffsetsArray[inBreakIndex] - startMeasureOffset, segmentWidth);
                          breakWidth = startLength + segmentWidth;
                          
                          // remember it
                          mBreakLengthsArray[inBreakIndex] = breakWidth;
                        }
                      
                        return breakWidth;
                      }


protected:

  nsMacTextRenderer&            mTextRenderer;
  const char_type*              mText;
  const PRInt32                 mTextLength;
  
  const PRInt32                 mNumBreaks;
  const PRInt32*                mBreakOffsetsArray;
  
  nsSpillableBuffer<nscoord>    mBreakLengthsBuffer;
  nscoord*                      mBreakLengthsArray;

};

#endif // nsMacTextRenderer_h__
