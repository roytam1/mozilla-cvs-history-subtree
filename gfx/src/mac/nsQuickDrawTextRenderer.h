

#ifndef nsQuickDrawTextRenderer_h__
#define nsQuickDrawTextRenderer_h__

#include "nsUnicodeRenderingToolkit.h"

#include "nsMacTextRenderer.h"

//-------------------------------------------------------------------------
//  nsQuickDrawTextRenderer
//  
//  Implementation class for QuickDraw text rendering
//  
//  
//-------------------------------------------------------------------------

class nsQuickDrawTextRenderer : public nsMacTextRenderer
{
public:

                        nsQuickDrawTextRenderer(PRBool inHighQualityRendering);
  virtual               ~nsQuickDrawTextRenderer();
    
  virtual nsresult      GetTextWidth(const char* aString,
                                PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth);
  virtual nsresult      GetTextWidth(const PRUnichar *aString,
                                PRUint32 aStartOffset, PRUint32 aLength, nscoord &aWidth);
  
  virtual nsresult      GetTextDimensions(const char* aString, PRUint32 aStartOffset,
                                PRUint32 aLength, nsTextDimensions& aDimensions);
  virtual nsresult      GetTextDimensions(const PRUnichar* aString, PRUint32 aStartOffset,
                                PRUint32 aLength, nsTextDimensions& aDimensions);
  
  virtual nsresult      GetTextDimensions(const char*        inString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions);

  virtual nsresult      GetTextDimensions(const PRUnichar*   inString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions);

  virtual nsresult      DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength,
                                  nscoord aX, nscoord aY, const nscoord* aSpacing);
  virtual nsresult      DrawText(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength,
                                   nscoord aX, nscoord aY, const nscoord* aSpacing);

protected:

  nsresult              SetPortTextState();

protected:

  nsUnicodeRenderingToolkit mUnicodeRenderingToolkit;

};


#endif // nsQuickDrawTextRenderer_h__
