

#ifndef nsUnicodeTextRenderer_h__
#define nsUnicodeTextRenderer_h__

#include <ATSUnicode.h>
#include "nsMacTextRenderer.h"


//-------------------------------------------------------------------------
//  nsUnicodeTextRenderer
//  
//  API for text rendering using ATSUI on Mac OS X
//  
//  All nscoord values in this API are in app units (twips)
//  
//  
//-------------------------------------------------------------------------
class nsCharWidthCache;
class nsATSUStyleCache;

class nsUnicodeTextRenderer : public nsMacTextRenderer
{
public:

                        nsUnicodeTextRenderer(PRBool inHighQualityRendering);
  virtual               ~nsUnicodeTextRenderer();
    
  virtual nsresult      GetTextWidth(const char* aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth);
  virtual nsresult      GetTextWidth(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord &aWidth);
  
  virtual nsresult      GetTextDimensions(const char* aString, PRUint32 aStartOffset, PRUint32 aLength, nsTextDimensions& aDimensions);
  virtual nsresult      GetTextDimensions(const PRUnichar* aString, PRUint32 aStartOffset, PRUint32 aLength, nsTextDimensions& aDimensions);
  
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
                                           nsTextDimensions& outLastWordDimensions);

  virtual nsresult      DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord aX, nscoord aY, const nscoord* aSpacing);
  virtual nsresult      DrawText(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord aX, nscoord aY, const nscoord* aSpacing);


protected:

  nsresult              GetCurrentATSUStyle(ATSUStyle* outStyle, nsCharWidthCache** outCharWidthCache);
  void                  RecycleStyle(ATSUStyle inStyle);
  
  nsATSUStyleCache*     GetGlobalStyleCache();

};

#endif // nsUnicodeTextRenderer_h__
