
#include <FixMath.h>
#include <TextEdit.h>
#include <ATSUnicode.h>

#include "nsString.h"

#include "nsFontUtils.h"
#include "nsGraphicState.h"
#include "nsRegionPool.h"
#include "nsGfxUtils.h"

#include "nsUnicodeMappingUtil.h"
#include "nsFontEnumerator.h"

#include "nsATSUStyleCache.h"
#include "nsUnicodeTextRenderer.h"


#define FixedToFloat(a)  ((float)(a) / fixed1)
#define FloatToFixed(a)  ((Fixed)((float)(a) * fixed1))

#define ENABLE_CACHE
//#define SIMPLE_ATSUI_DRAWING


#pragma mark
class nsATSUGlyphBuffer
{
public:
                        nsATSUGlyphBuffer()
                        : mAllocatedGlyphBuffer(nsnull)
                        , mGlyphArray((ATSUGlyphInfoArray *)mGlyphBuffer)
                        , mAllocatedBufferSize(0)
                        , mGlyphBufferSize(kInternalGlyphBufferSize)
                        {
                        
                        }
                        
                        
                        ~nsATSUGlyphBuffer()
                        {
                          if (mAllocatedGlyphBuffer)
                            free(mAllocatedGlyphBuffer);
                        }

  OSStatus              EnsureSpace(PRInt32 inNumGlyphs)
                        {
                          PRInt32   bufferSize = sizeof(ATSUGlyphInfoArray) + (inNumGlyphs - 1) * sizeof(ATSUGlyphInfo);
                          if (bufferSize > kInternalGlyphBufferSize)
                          {
                            if (bufferSize > mAllocatedBufferSize)
                            {
                              PRUint8  *newBuffer = (PRUint8 *)realloc(mAllocatedGlyphBuffer, bufferSize);
                              if (!newBuffer) return memFullErr;
                              
                              mAllocatedGlyphBuffer = newBuffer;
                              mAllocatedBufferSize = bufferSize;
                            }
                          
                            mGlyphArray = (ATSUGlyphInfoArray *)mAllocatedGlyphBuffer;
                            mGlyphBufferSize = mAllocatedBufferSize;
                          }
                          else
                          {
                            mGlyphArray = (ATSUGlyphInfoArray *)mGlyphBuffer;
                            // we could dispose of the allocated buffer here, but no need
                            mGlyphBufferSize = kInternalGlyphBufferSize;
                          }
                          
                          return noErr;
                        }

  ATSUGlyphInfoArray*   GetGlyphBuffer()
                        {
                          return mGlyphArray;
                        }

  PRInt32               GetGlyphBufferSize()
                        {
                          return mGlyphBufferSize;
                        }
  
protected:

  enum {
    kInternalGlyphBufferSize    = 1024  
  };
  
  PRUint8               mGlyphBuffer[kInternalGlyphBufferSize];

  PRUint8               *mAllocatedGlyphBuffer;
  PRInt32               mAllocatedBufferSize;

  PRInt32               mGlyphBufferSize;
  ATSUGlyphInfoArray    *mGlyphArray;     // may point to mGlyphBuffer or an allocated buffer
};


#pragma mark
class nsATSUIDrawingUtils
{
public:

                        nsATSUIDrawingUtils(const PRUnichar* inText, PRInt32 inTextLength, CGrafPtr inPort, Boolean inAllowTypographicalFeatures = false);
                        ~nsATSUIDrawingUtils();

  OSStatus              CreateTextLayout(ATSUStyle inStyle, nsCharWidthCache* inCharWidthCache);

  OSStatus              GetGlyphInfo(PRInt32 inSegmentOffset, PRInt32 inSegmentLength, ByteCount& ioBufferSize, ATSUGlyphInfoArray* outGlyphInfoPtr);

  OSStatus              GetUnicodeStringWidth(PRInt32 inSegmentOffset, PRInt32 inSegmentLength, ATSUTextMeasurement& outWidth);

  OSStatus              RenderUnicodeString(PRInt32 inSegmentOffset, PRInt32 inSegmentLength, short h, short v, Float32* inSpacing = nsnull);

  OSStatus              FindLineBreak(PRInt32 inSegmentOffset, PRInt32 inSegmentLength,
                                ItemCount inNumBreakPositions, const UniCharCount* inBreaks,
                                ATSUTextMeasurement inLineWidth, UniCharArrayOffset& outOffset,
	                              ATSUTextMeasurement& outWidth, Boolean& outBrokeWord);
	
  static OSStatus       MakeSimpleATSUIStyle(StringPtr fontName, short fontID, short fontSize, short fontStyle, RGBColor *fontColor, Boolean inAllowTypogFeatures, ATSUStyle *theStyle);
  static OSStatus       SetupATSUIFontFallbacks();
  
protected:

  OSStatus              SetTextObjectCGContext();
  OSStatus              SetLayoutControls();
  
protected:

  const PRUnichar*      mText;            // not owned
  PRInt32               mTextLength;
  
  ATSUTextLayout        mTextLayout;      // owned
  ATSUStyle             mTextStyle;       // text layout is assumed to be monostyled
  CGrafPtr              mPort;            // not owned
  CGContextRef          mCGContextRef;    // owned
  
  nsCharWidthCache*     mCharWidthCache;  // not owned
  
  Boolean               mAllowTypographicalFeatures;      // allow kerning and ligatures, disable width caching. Slow!
};


#pragma mark -

//-----------------------------------------------------------------------------
nsATSUIDrawingUtils::nsATSUIDrawingUtils(const PRUnichar* inText, PRInt32 inTextLength, CGrafPtr inPort, Boolean inAllowTypographicalFeatures)
//-----------------------------------------------------------------------------
: mText(inText)
, mTextLength(inTextLength)
, mTextLayout(nsnull)
, mTextStyle(nsnull)
, mPort(inPort)
, mCGContextRef(nsnull)
, mCharWidthCache(nsnull)
, mAllowTypographicalFeatures(inAllowTypographicalFeatures)
{
  if (!mPort) {
    nsGraphicsUtils::SetPortToKnownGoodPort();
    ::GetPort(&mPort);
  }

  ::QDBeginCGContext(mPort, &mCGContextRef);
}

//-----------------------------------------------------------------------------
nsATSUIDrawingUtils::~nsATSUIDrawingUtils()
//-----------------------------------------------------------------------------
{
  if (mTextLayout)
    ::ATSUDisposeTextLayout(mTextLayout);

  ::QDEndCGContext(mPort, &mCGContextRef);
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::CreateTextLayout(ATSUStyle inStyle, nsCharWidthCache* inCharWidthCache)
//-----------------------------------------------------------------------------
{
  ATSUTextLayout  textLayout;
  OSStatus  err = ::ATSUCreateTextLayoutWithTextPtr((ConstUniCharArrayPtr)mText, 0, (UniCharCount)mTextLength,
                          (UniCharCount)mTextLength, 1, (UniCharCount *)&mTextLength, &inStyle, &textLayout);
  if (err != noErr)
    return err;
  
  mTextStyle      = inStyle;
  mTextLayout     = textLayout;
  mCharWidthCache = inCharWidthCache;
  
  err = SetLayoutControls();
  if (err != noErr)
    return err;

  err = SetTextObjectCGContext();
  if (err != noErr)
    return err;
  
  return noErr;
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::GetGlyphInfo(PRInt32 inSegmentOffset, PRInt32 inSegmentLength,
              ByteCount& ioBufferSize, ATSUGlyphInfoArray* outGlyphInfoPtr)
//-----------------------------------------------------------------------------
{
  OSStatus err = ::ATSUGetGlyphInfo(mTextLayout, (UniCharCount)inSegmentOffset, (UniCharCount)inSegmentLength, &ioBufferSize, outGlyphInfoPtr);
  return err;
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::GetUnicodeStringWidth(PRInt32 inSegmentOffset, PRInt32 inSegmentLength, ATSUTextMeasurement& outWidth)
//-----------------------------------------------------------------------------
{
  NS_ASSERTION(mTextLayout && mCGContextRef, "Not initialized");
  
  OSStatus  err = noErr;
  
  if (mAllowTypographicalFeatures)
  {
  	// measure the text. this is slow
  	ATSUTextMeasurement oTextBefore, ascent, descent;
  	err = ::ATSUMeasureText(mTextLayout, inSegmentOffset, inSegmentLength, &oTextBefore, &outWidth, &ascent, &descent);
  	return err;
	}	

  PRBool    canCacheASCIIText = mCharWidthCache && mCharWidthCache->IsTextASCII(mText + inSegmentOffset, inSegmentLength);
  Float32   measuredWidth;
  if (canCacheASCIIText && mCharWidthCache->GetTotalCharacterWidth(mText + inSegmentOffset, inSegmentLength, &measuredWidth))
  {
    outWidth = FloatToFixed(measuredWidth);
    return noErr;
  }

#if NOISY_TEXT_CALLS
  {
    nsCAutoString theString; theString.AssignWithConversion((PRUnichar*)mText + inSegmentOffset, inSegmentLength);
    printf("Measuring width of '%s'\n", theString.get());
  }
#endif
  
	nsATSUGlyphBuffer   glyphBuffer;
  err = glyphBuffer.EnsureSpace(inSegmentOffset + inSegmentLength + 1);
  if (err != noErr) return err;
  
  ATSUGlyphInfoArray  *glyphBufferPtr = glyphBuffer.GetGlyphBuffer();
  ByteCount           bufferSize      = glyphBuffer.GetGlyphBufferSize();

  // ATSUGetGlyphInfo seems to return info on all glyphs in the text layout, and ignore
  // its text offset and length params.
  err = GetGlyphInfo(0, inSegmentOffset + inSegmentLength, bufferSize, glyphBufferPtr);
  if (err != noErr) return err;

  // need to take into account the fact that there may not be a 1-1 mapping between chars and glyphs
  Float32   startOffset = 0.0f;
  Float32   endOffset   = 0.0f;
  UniCharCount  endCharIndex = inSegmentOffset + inSegmentLength;
  
  ItemCount curGlyph = 0;
  while (curGlyph < glyphBufferPtr->numGlyphs)
  {
    if (glyphBufferPtr->glyphs[curGlyph].charIndex == inSegmentOffset && startOffset == 0.0)
      startOffset = glyphBufferPtr->glyphs[curGlyph].idealX;
    
    if (glyphBufferPtr->glyphs[curGlyph].charIndex == endCharIndex)
    {
      endOffset = glyphBufferPtr->glyphs[curGlyph].idealX;
      break;
    }
  
    curGlyph ++;
  }
  
  Float32   lastGlyphWidth = 0.0f;
  
  if (curGlyph == glyphBufferPtr->numGlyphs)
  {
    ATSGlyphIdealMetrics   glyphMetrics;
    err = ::ATSUGlyphGetIdealMetrics(mTextStyle, 1, &glyphBufferPtr->glyphs[glyphBufferPtr->numGlyphs - 1].glyphID, 0, &glyphMetrics);
    if (err != noErr) return err;

    lastGlyphWidth = glyphMetrics.advance.x;
    endOffset = glyphBufferPtr->glyphs[glyphBufferPtr->numGlyphs - 1].idealX + lastGlyphWidth;
  }

  Float32   totalWidth = endOffset - startOffset;
   
#ifdef ENABLE_CACHE
  if (canCacheASCIIText)
    mCharWidthCache->CacheCharacterWidths(mText + inSegmentOffset, inSegmentLength, glyphBufferPtr, lastGlyphWidth);  
#endif

  outWidth = FloatToFixed(totalWidth);
  
	return noErr;
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::RenderUnicodeString(PRInt32 inSegmentOffset, PRInt32 inSegmentLength, short h, short v, Float32* inSpacing)
//-----------------------------------------------------------------------------
{
  NS_ASSERTION(mTextLayout && mCGContextRef, "Not initialized");

  GrafPtr curPort;
  ::GetPort(&curPort);

	StRegionFromPool    clipRegion;
	::GetPortClipRegion(curPort, clipRegion);

  Rect    bounds;
  ::GetPortBounds(curPort, &bounds);

  ::SyncCGContextOriginWithPort(mCGContextRef, curPort);
	::ClipCGContextToRegion(mCGContextRef, &bounds, clipRegion);

	short transV = (bounds.bottom - bounds.top - v);
	
	OSStatus err = noErr;

  if (inSpacing == 0)
  {
  	// draw the text
  	err = ::ATSUDrawText(mTextLayout, inSegmentOffset, inSegmentLength, FixRatio(h, 1), FixRatio(transV, 1));
  	if (err != noErr)
  	  return err;
  }
  else
  {
    // we can only use ATSUGetGlyphInfo/ATSUDrawGlyphInfo when we're drawing
    // the entire string, because both act on the entire text layout.
    if (inSegmentOffset == 0 && inSegmentLength == mTextLength)
    {
    	nsATSUGlyphBuffer   glyphBuffer;
      err = glyphBuffer.EnsureSpace(inSegmentOffset + inSegmentLength + 1);
      if (err != noErr) 
        return err;

      ATSUGlyphInfoArray  *glyphBufferPtr = glyphBuffer.GetGlyphBuffer();
      ByteCount           bufferSize      = glyphBuffer.GetGlyphBufferSize();

      err = GetGlyphInfo(0, inSegmentOffset + inSegmentLength, bufferSize, glyphBufferPtr);
      if (err != noErr)
        return err;

      Float32Point    location;
      location.x = (Float32)h;
      location.y = (Float32)transV;
      
      // fix up the spacing
      Float32   cumulativeSpacing = 0;
      for (PRInt32 i = 0; i < inSegmentLength; i ++)
      {
        glyphBufferPtr->glyphs[i].idealX = cumulativeSpacing;
        cumulativeSpacing += inSpacing[i];
      }
        
      err = ::ATSUDrawGlyphInfo(glyphBufferPtr, location);
      if (err != noErr)
        return err;
    }
    else
    {
      // have to draw 1 char at a time. Ick.
      short   curH = h;
      
      for (PRInt32 i = inSegmentOffset; i < inSegmentOffset + inSegmentLength; i ++)
      {
        err = RenderUnicodeString(i, 1, curH, v);    // re-enters, but without spacing array        
        if (err != noErr)
          return err;
          
        curH += (short)inSpacing[i];
      }
    }
    
  }
  
	return noErr;
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::FindLineBreak(PRInt32 inSegmentOffset, PRInt32 inSegmentLength,
                              ItemCount inNumBreakPositions, const UniCharCount* inBreaks,
                              ATSUTextMeasurement inLineWidth, UniCharArrayOffset& outOffset,
                              ATSUTextMeasurement& outWidth, Boolean& outBrokeWord)
//-----------------------------------------------------------------------------
{
	
  NS_ASSERTION(mTextLayout && mCGContextRef, "Not initialized");
  NS_ASSERTION(inLineWidth > 0, "Cannot break zero-width line");
  
  outBrokeWord = false;

	OSStatus        err = noErr;

#if 0
  if (mAllowTypographicalFeatures)
  {
    UniCharArrayOffset  breakPos;

    // find the break in the text
  	err = ::ATSUBreakLine(mTextLayout, kATSUFromTextBeginning, inLineWidth, false, &breakPos);
  	if (err == kATSULineBreakInWord)
  	{
  	  outBrokeWord = true;
  	  err = noErr;
  	}

  	if (err != noErr)
  	  return err;

    ATSUTextMeasurement oTextBefore, ascent, descent;
    
    if (outBrokeWord)   // measure the whole thing
    	err = ::ATSUMeasureText(mTextLayout, inSegmentOffset, inSegmentLength, &oTextBefore, &outWidth, &ascent, &descent);
    else
    	err = ::ATSUMeasureText(mTextLayout, inSegmentOffset, breakPos, &oTextBefore, &outWidth, &ascent, &descent);

    if (err != noErr)
      return err;

    outOffset = breakPos;
    return noErr;
  }
#endif

  nsATSUGlyphBuffer   glyphBuffer;
  err = glyphBuffer.EnsureSpace(inSegmentOffset + inSegmentLength + 1);
  if (err != noErr)
    return err;
  
  ATSUGlyphInfoArray  *glyphBufferPtr = glyphBuffer.GetGlyphBuffer();
  ByteCount           bufferSize      = glyphBuffer.GetGlyphBufferSize();

  err = GetGlyphInfo(0, inSegmentOffset + inSegmentLength, bufferSize, glyphBufferPtr);
  if (err != noErr)
    return err;
  
  Float32             lineWidthFloat = FixedToFloat(inLineWidth);
  UniCharArrayOffset  lastBreakOffset = 0;
  Float32             brokenLineWidth;
  UniCharCount        curGlyph = 0;
  
  for (ItemCount i = 0; i < inNumBreakPositions; i ++)
  {
    UniCharCount    curBreakOffset = inBreaks[i];

    // find the first glyph which starts after this break
    while ((curGlyph < glyphBufferPtr->numGlyphs) && (glyphBufferPtr->glyphs[curGlyph].charIndex < curBreakOffset))
      curGlyph++;

    if (curGlyph == glyphBufferPtr->numGlyphs)
    {
      // figure out if the entire line fits
      UniCharCount  lastGlyphIndex = glyphBufferPtr->numGlyphs - 1;
      Float32       entireLineWidth;
      
      // add the width of the last character
      ATSGlyphIdealMetrics   glyphMetrics;
      err = ::ATSUGlyphGetIdealMetrics(mTextStyle, 1, &glyphBufferPtr->glyphs[lastGlyphIndex].glyphID, 0, &glyphMetrics);
      if (err != noErr)
        return err;

      entireLineWidth = glyphBufferPtr->glyphs[lastGlyphIndex].idealX + glyphMetrics.advance.x;
      if (lastBreakOffset == 0 || entireLineWidth <= lineWidthFloat)
      {
        brokenLineWidth = entireLineWidth;
        lastBreakOffset = inBreaks[inNumBreakPositions - 1];
      }
      break;
    }
  
    Float32   glyphOffset = glyphBufferPtr->glyphs[curGlyph].idealX;
    if (glyphOffset > lineWidthFloat)      // this break is too long
    {
      // if we didn't find a break yet, return the length up to here
      if (lastBreakOffset == 0)
      {
        brokenLineWidth = glyphBufferPtr->glyphs[curGlyph].idealX;
        lastBreakOffset = curBreakOffset;      
      }
      break;
    }
    
    lastBreakOffset = curBreakOffset;
    brokenLineWidth = glyphOffset;
  }

  outWidth = FloatToFixed(brokenLineWidth);
  outOffset = lastBreakOffset;
  
	return noErr;
}


#pragma mark -

//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::MakeSimpleATSUIStyle(StringPtr fontName, short fontID,
                short fontSize, short fontStyle, RGBColor *fontColor, Boolean inAllowTypogFeatures, ATSUStyle* outStyle)
//-----------------------------------------------------------------------------
{
	OSStatus    err;
	ATSUStyle   localStyle;
	ATSUFontID  atsuFont;
	Fixed       atsuSize;
	short       atsuOrientation;
	RGBColor    defaultColor = { 0, 0, 0};
	Boolean     trueVar = true, falseVar = false;
	
		/* Three parrallel arrays for setting up attributes. */
	const ATSUAttributeTag theTags[] = {
			kATSUFontTag,
			kATSUSizeTag,
			kATSUVerticalCharacterTag,
			kATSUColorTag,
			kATSUQDBoldfaceTag,
			kATSUQDItalicTag, 
			kATSUQDUnderlineTag,
			kATSUQDCondensedTag,
			kATSUQDExtendedTag,
			kATSUKerningInhibitFactorTag,      // disable kerning (mostly)
			kATSUNoOpticalAlignmentTag
		};
		
	const ByteCount theSizes[] = {
			sizeof(ATSUFontID), sizeof(Fixed), sizeof(UInt16),
			sizeof(RGBColor), sizeof(Boolean), sizeof(Boolean),
			sizeof(Boolean), sizeof(Boolean), sizeof(Boolean),
			sizeof(Fract),
			sizeof(Boolean),
			sizeof(Fixed),
			sizeof(Fixed),
			sizeof(Fixed)	
		};

	ATSUAttributeValuePtr theValues[] = {
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
		};

  ItemCount   numFeatures = sizeof(theTags) / sizeof(theTags[0]);
  if (inAllowTypogFeatures)
    numFeatures -= 2;   // allow kerning and alignment
  
	// set up locals
	localStyle = NULL;
	atsuFont = 0;
	atsuSize = FixRatio(fontSize, 1);
	atsuOrientation = kATSUStronglyHorizontal;
	Fract   noKerning = fract1;                       // disable kerning
	Fixed   ligatureDecomposition = FixRatio(-1, 1);  // discourage ligatures
	Fixed   noShifting = FixRatio(-1, 1);
	Fixed   noTracking = FixRatio(1, 1);
	
	// set the values array to point to our locals
	theValues[0] = &atsuFont;
	theValues[1] = &atsuSize;
	theValues[2] = &atsuOrientation;
	theValues[3] = ((fontColor != NULL) ? fontColor : &defaultColor);
	theValues[4] = ((fontStyle & bold) != 0 ? &trueVar : &falseVar);
	theValues[5] = ((fontStyle & italic) != 0 ? &trueVar : &falseVar);
	theValues[6] = ((fontStyle & underline) != 0 ? &trueVar : &falseVar);
	theValues[7] = ((fontStyle & condense) != 0 ? &trueVar : &falseVar);
	theValues[8] = ((fontStyle & extend) != 0 ? &trueVar : &falseVar);
	theValues[9] = &noKerning;
	theValues[10] = &trueVar;
	theValues[11] = &noShifting;
	theValues[12] = &noShifting;
	theValues[13] = &noTracking;

  // find the font ID
  if (fontName)
  {
  	err = ::ATSUFindFontFromName((Ptr)fontName+1, (long)fontName[0],
  		kFontFullName, kFontMacintoshPlatform, kFontRomanScript, 
  		kFontNoLanguage, &atsuFont);
  	if (err != noErr)
  	  return err;
	}
	else
	{
	  fontStyle = normal;   // get the base font, not a font variant.
  	err = ::ATSUFONDtoFontID(fontID, fontStyle, &atsuFont);
  	if (err != noErr)
  	  return err;
	}
	
#if DEBUG
  if (!fontName)
  {
    char      fontNameString[256];
    ByteCount nameLen;
    ItemCount nameIndex;
    ::ATSUFindFontName(atsuFont, kFontFullName, kFontMacintoshPlatform, kFontRomanScript, kFontNoLanguage, 255, fontNameString, &nameLen, &nameIndex);  
  }
#endif

  // create a style
	err = ::ATSUCreateStyle(&localStyle);
	if (err != noErr)
	  return err;

  // set the style attributes
	err = ::ATSUSetAttributes(localStyle, numFeatures, theTags, theSizes, theValues);
	if (err != noErr) {
    ::ATSUDisposeStyle(localStyle);
    return err;
  }
	
  if (!inAllowTypogFeatures)
  {
  	// disable ligatures
  	ATSUFontFeatureType     featuresToClear[]  = { kLigaturesType, kLigaturesType };
  	ATSUFontFeatureSelector featuresSelector[] = { kCommonLigaturesOffSelector, kRareLigaturesOffSelector };
  	
    err = ::ATSUSetFontFeatures(localStyle, sizeof(featuresSelector) / sizeof(featuresSelector[0]), featuresToClear, featuresSelector);
    NS_ASSERTION(err == noErr, "Error setting font features");
  	if (err != noErr) {
      ::ATSUDisposeStyle(localStyle);
      return err;
    }
  }
    
	*outStyle = localStyle;
	return noErr;
}

//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::SetupATSUIFontFallbacks()
//-----------------------------------------------------------------------------
{
  nsUnicodeMappingUtil*   mappingUtil = nsUnicodeMappingUtil::GetSingleton();
  
  const PRInt32 kMaxFallbackFontIDs = 12;     // should match size of script code array
  
  // scripts that we care about
  static const ScriptCode   kFallbackScripts[] = {
    smRoman,
    smJapanese,
    smSimpChinese,
    smTradChinese,
    smKorean,
    smHebrew,
    smGreek,
    smCentralEuroRoman,
    smCyrillic,
    smThai,
    smArabic,
    smGeorgian    
  };
  
  PRInt32     curFontIDIndex = 0;
  ATSUFontID  fontIDs[kMaxFallbackFontIDs];
  
  for (PRInt32 i = 0; i < sizeof(kFallbackScripts) / sizeof(ScriptCode); i ++)
  {
    nsString*   fontForScript =  mappingUtil->GenericFontNameForScript(kFallbackScripts[i], kSerif);
    if (!fontForScript) continue;
    
    short   fontNum;
    nsFontEnumeratorMac::GetMacFontID(*fontForScript, fontNum);
    ATSUFontID    atsuFontID;
    if ((fontNum != -1) && (::ATSUFONDtoFontID(fontNum, normal, &atsuFontID) == noErr))
    {
      fontIDs[curFontIDIndex] = atsuFontID;
      curFontIDIndex ++;
    }
  }
  
  return ::ATSUSetFontFallbacks(curFontIDIndex, fontIDs, kATSUSequentialFallbacksPreferred);
}


#pragma mark -

//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::SetLayoutControls()
//-----------------------------------------------------------------------------
{
  ATSUAttributeTag        attributeTag  = kATSULineLayoutOptionsTag;
  ByteCount               attribSize    = sizeof(ATSLineLayoutOptions);
  ATSLineLayoutOptions    layoutOptions = kATSLineIsDisplayOnly;

  if (!mAllowTypographicalFeatures)
  {
    layoutOptions |= (  kATSLineHasNoHangers            // no hanging punctuation
                      | kATSLineHasNoOpticalAlignment
                      | kATSLineNoSpecialJustification);
  }
  
  ATSUAttributeValuePtr   valuePtr = &layoutOptions;
  OSStatus err = ::ATSUSetLayoutControls(mTextLayout, 1, &attributeTag, &attribSize, &valuePtr);
  if (err != noErr)
    return err;

  // turn on automatic font fallbacks
  //err = ::ATSUSetTransientFontMatching(textLayout, true);
  //if (err != noErr) return err;

  return noErr;
}


//-----------------------------------------------------------------------------
OSStatus nsATSUIDrawingUtils::SetTextObjectCGContext()
//-----------------------------------------------------------------------------
{
  // associate it with a CGContext
  ByteCount             iSize = sizeof(CGContextRef);
  ATSUAttributeTag      iTag = kATSUCGContextTag;
  ATSUAttributeValuePtr iValuePtr = &mCGContextRef;
  return ::ATSUSetLayoutControls(mTextLayout, 1, &iTag, &iSize, &iValuePtr);
}

#pragma mark -

//-------------------------------------------------------------------------
//  nsUnicodeTextRenderer
// 
//-------------------------------------------------------------------------
nsUnicodeTextRenderer::nsUnicodeTextRenderer(PRBool inHighQualityRendering)
: nsMacTextRenderer(inHighQualityRendering)
{
  nsATSUIDrawingUtils::SetupATSUIFontFallbacks();
}


//-------------------------------------------------------------------------
//  ~nsUnicodeTextRenderer
// 
//-------------------------------------------------------------------------
nsUnicodeTextRenderer::~nsUnicodeTextRenderer()
{
}


#pragma mark -


//#define NOISY_TEXT_CALLS  DEBUG

//-------------------------------------------------------------------------
//  GetTextWidth
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::GetTextWidth(const char* aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord& aWidth)
{
#if NOISY_TEXT_CALLS
  char      tempBuf[256];
  PRInt32   len = aLength > 255 ? 255 : aLength;
  strncpy(tempBuf, aString, len);
  tempBuf[len] = '\0';
  printf("getting width of '%s'\n", tempBuf);
#endif
  return GetTextWidth(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aWidth);
}

//-------------------------------------------------------------------------
//  GetTextWidth
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::GetTextWidth(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength, nscoord &aWidth)
{
  nsTextDimensions    textDimensions;
  nsresult rv = GetTextDimensions(aString, aStartOffset, aLength, textDimensions);
  aWidth = textDimensions.width;
  return rv;
}

//-------------------------------------------------------------------------
//  GetTextDimensions
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::GetTextDimensions(const char* aString, PRUint32 aStartOffset, PRUint32 aLength, nsTextDimensions& aDimensions)
{
#if NOISY_TEXT_CALLS
  char      tempBuf[256];
  PRInt32   len = aLength > 255 ? 255 : aLength;
  strncpy(tempBuf, aString, len);
  tempBuf[len] = '\0';
  printf("getting dimensions of '%s'\n", tempBuf);
#endif
  return GetTextDimensions(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aDimensions);
}

//-------------------------------------------------------------------------
//  GetTextDimensions
// 
//-------------------------------------------------------------------------

nsresult nsUnicodeTextRenderer::GetTextDimensions(const PRUnichar* aString, PRUint32 aStartOffset, PRUint32 aLength, nsTextDimensions& outDimensions)
{
  mGS->mFontMetrics->GetMaxAscent(outDimensions.ascent);
  mGS->mFontMetrics->GetMaxDescent(outDimensions.descent);

  if (aLength == 0)
  {
    outDimensions.width = 0;
    return NS_OK;
  }

  ATSUStyle         atsuStyle;
  nsCharWidthCache* charWidthCache;
  nsresult rv = GetCurrentATSUStyle(&atsuStyle, &charWidthCache);
  if (NS_FAILED(rv))
    return rv;
  
  nsATSUIDrawingUtils   atsuiDrawing(aString, aStartOffset + aLength, mPort, mHighQualityRendering);
  OSStatus err = atsuiDrawing.CreateTextLayout(atsuStyle, charWidthCache);
  if (err != noErr)
    return NS_ERROR_FAILURE;
  
  ATSUTextMeasurement   width;
  err = atsuiDrawing.GetUnicodeStringWidth(aStartOffset, aLength, width);
  if (err != noErr)
    return NS_ERROR_FAILURE;
  
#if NOISY_TEXT_CALLS
  {
    nsCAutoString theString; theString.AssignWithConversion(aString, aLength);
    printf("Width of '%s' (%d long) measured as %f\n", theString.get(), aLength, FixedToFloat(width));
  }
#endif
    
  float   pixToTwips;
  mDeviceContext->GetDevUnitsToAppUnits(pixToTwips);
  outDimensions.width = pixToTwips * FixedToFloat(width);
  
  RecycleStyle(atsuStyle);
  return NS_OK;
}


//-------------------------------------------------------------------------
//  GetTextDimensions
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::GetTextDimensions(const char*  inString,
                                           PRInt32           inLength,
                                           nscoord           inAvailWidth,
                                           const PRInt32*    inWordBreaks,
                                           PRInt32           inNumBreaks,
                                           nsTextDimensions& outDimensions,
                                           PRInt32&          outNumCharsFit,
                                           nsTextDimensions& outLastWordDimensions)
{
#if NOISY_TEXT_CALLS
  char      tempBuf[256];
  PRInt32   len = inLength > 255 ? 255 : inLength;
  strncpy(tempBuf, inString, len);
  tempBuf[len] = '\0';
  printf("getting breaks for '%s'\n", tempBuf);
#endif
  return GetTextDimensions(NS_ConvertASCIItoUCS2(inString).get(), inLength, inAvailWidth, inWordBreaks, inNumBreaks,
          outDimensions, outNumCharsFit, outLastWordDimensions);
}

//-------------------------------------------------------------------------
//  GetTextDimensions
// 
// Note: aBreaks[] is supplied to us so that the first word is located
// at aString[0 .. aBreaks[0]-1] and more generally, the k-th word is
// located at aString[aBreaks[k-1] .. aBreaks[k]-1]. Whitespace can
// be included and each of them counts as a word in its own right.
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::GetTextDimensions(const PRUnichar*  inString,     // in string
                                           PRInt32           inLength,            // in string length
                                           nscoord           inAvailWidth,        // available width in twips (should be an nscoord)
                                           const PRInt32*    inWordBreaks,        // array of possible word breaks
                                           PRInt32           inNumBreaks,         // number of breaks
                                           nsTextDimensions& outDimensions,       // returned dimensions of the final line of text
                                           PRInt32&          outNumCharsFit,      // returned number of chars on the line
                                           nsTextDimensions& outLastWordDimensions) // returned last word dimensions, if known
{
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

  ATSUStyle         atsuStyle;
  nsCharWidthCache* charWidthCache;
  nsresult rv = GetCurrentATSUStyle(&atsuStyle, &charWidthCache);
  if (NS_FAILED(rv))
    return rv;
  
  nsATSUIDrawingUtils   atsuiDrawing(inString, inLength, mPort, mHighQualityRendering);
  OSStatus err = atsuiDrawing.CreateTextLayout(atsuStyle, charWidthCache);
  if (err != noErr)
    return NS_ERROR_FAILURE;

  float   twipsToPix;
  mDeviceContext->GetAppUnitsToDevUnits(twipsToPix);

  ATSUTextMeasurement   lineWidth = FloatToFixed((float)inAvailWidth * twipsToPix);
  ATSUTextMeasurement   linebreakWidth;
  UniCharArrayOffset    breakOffset;
  Boolean               brokeWord;
  
  err = atsuiDrawing.FindLineBreak(0, inLength, inNumBreaks, (const UniCharArrayOffset *)inWordBreaks,
          lineWidth, breakOffset, linebreakWidth, brokeWord);
  if (err != noErr)
    return NS_ERROR_FAILURE;

  // make sure we agree with gecko about the line breaks
#if DEBUG
  {
    PRBool    foundBreak = PR_FALSE;
    
    for (PRInt32 i = 0; i < inNumBreaks; i ++)
    {
      if (inWordBreaks[i] == breakOffset)
      {
        foundBreak = PR_TRUE;
        break;
      }
    }
  
    NS_ASSERTION(foundBreak || brokeWord || breakOffset == 0, "Didn't match breaks");
  }
#endif  
  
  if (brokeWord || breakOffset == 0)
  {
    // if nothing fit, return the entire length
    //linebreakWidth = 0;
    breakOffset = inLength;
  }
  
  float   pixToTwips;
  mDeviceContext->GetDevUnitsToAppUnits(pixToTwips);
  outDimensions.width = pixToTwips * FixedToFloat(linebreakWidth);

#if NOISY_TEXT_CALLS
  {
    nsCAutoString theString; theString.AssignWithConversion(inString, inLength);
    printf("Linebreak of '%s' (%d long) measured as %f\n", theString.get(), inLength, FixedToFloat(linebreakWidth));
  }
#endif
  
//  NS_ASSERTION(outDimensions.width <= inAvailWidth, "Hey, we said we could fit");

  outLastWordDimensions.ascent = outDimensions.ascent;
  outLastWordDimensions.descent = outDimensions.descent;
  
  outNumCharsFit = breakOffset;

  RecycleStyle(atsuStyle);
  return NS_OK;
}

//-------------------------------------------------------------------------
//  DrawText
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::DrawText(const char *aString, PRUint32 aStartOffset, PRUint32 aLength,
                      nscoord aX, nscoord aY, const nscoord* aSpacing)
{
  return DrawText(NS_ConvertASCIItoUCS2(aString).get(), aStartOffset, aLength, aX, aY, aSpacing);
}

//-------------------------------------------------------------------------
//  DrawText
// 
//-------------------------------------------------------------------------
nsresult nsUnicodeTextRenderer::DrawText(const PRUnichar *aString, PRUint32 aStartOffset, PRUint32 aLength,
                      nscoord aX, nscoord aY, const nscoord* aSpacing)
{

  ATSUStyle         atsuStyle;
  nsCharWidthCache* charWidthCache;
  nsresult rv = GetCurrentATSUStyle(&atsuStyle, &charWidthCache);
  if (NS_FAILED(rv))
    return rv;

  PRInt32   h = aX;
  PRInt32   v = aY;

	mGS->mTMatrix.TransformCoord(&h, &v);

  nsATSUIDrawingUtils   atsuiDrawing(aString, aStartOffset + aLength, mPort, mHighQualityRendering);
  OSStatus err = atsuiDrawing.CreateTextLayout(atsuStyle, charWidthCache);
  if (err != noErr)
    return NS_ERROR_FAILURE;
  
  Float32* spacingArray = nsnull;
  if (aSpacing)
  {
    nsSpillableBuffer<Float32>   spacingBuffer(aLength);
    if (spacingBuffer.GetCapacity() < aLength)
      return NS_ERROR_OUT_OF_MEMORY;
    
    spacingArray = spacingBuffer.GetBuffer();
    
    float   testX = 1.0f, testY = 0.0f;
    mGS->mTMatrix.TransformNoXLate(&testX, &testY);

    for (PRInt32 i = 0; i < aLength; i ++)
      spacingArray[i] = (float)aSpacing[i] * testX;
  }
  
  err = atsuiDrawing.RenderUnicodeString(aStartOffset, aLength, h, v, spacingArray);
  if (err != noErr)
    return NS_ERROR_FAILURE;
  
  RecycleStyle(atsuStyle);
  return NS_OK;
}


#pragma mark -

//-------------------------------------------------------------------------
//  GetCurrentATSUStyle
// 
//-------------------------------------------------------------------------
#define COLOR8TOCOLOR16(color8)	 ((color8 << 8) | color8)

nsresult nsUnicodeTextRenderer::GetCurrentATSUStyle(ATSUStyle* outStyle, nsCharWidthCache** outCharWidthCache)
{
  *outStyle = nsnull;
  *outCharWidthCache = nsnull;
  
	TextStyle		theStyle;
	nsFontUtils::GetNativeTextStyle(*mGS->mFontMetrics, *mDeviceContext, theStyle);

	RGBColor color;
	color.red = COLOR8TOCOLOR16(NS_GET_R(mGS->mColor));
	color.green = COLOR8TOCOLOR16(NS_GET_G(mGS->mColor));
	color.blue = COLOR8TOCOLOR16(NS_GET_B(mGS->mColor));

  nsATSUStyleCache*   styleCache = GetGlobalStyleCache();
  if (styleCache == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  
  if (!styleCache->GetStyle(theStyle.tsFont,
                              theStyle.tsSize,
                              (theStyle.tsFace & bold) != 0,
                              (theStyle.tsFace & italic) != 0,
                              mGS->mColor, outStyle, outCharWidthCache))
  {
    OSStatus    err;
    err = nsATSUIDrawingUtils::MakeSimpleATSUIStyle(nsnull, theStyle.tsFont, theStyle.tsSize, theStyle.tsFace, &color, mHighQualityRendering, outStyle);
    if (err != noErr)
      return NS_ERROR_FAILURE;
      
    styleCache->SetStyle(   theStyle.tsFont,
                              theStyle.tsSize,
                              (theStyle.tsFace & bold) != 0,
                              (theStyle.tsFace & italic) != 0,
                              mGS->mColor, *outStyle, outCharWidthCache);
  }

  return NS_OK;    
}

//-------------------------------------------------------------------------
//  RecycleStyle
// 
//-------------------------------------------------------------------------
void nsUnicodeTextRenderer::RecycleStyle(ATSUStyle inStyle)
{
  // do nothing since the style is in the cache
}


#pragma mark -

//-------------------------------------------------------------------------
//  GetGlobalStyleCache
// 
//-------------------------------------------------------------------------
nsATSUStyleCache* nsUnicodeTextRenderer::GetGlobalStyleCache()
{
  static nsATSUStyleCache*  sStyleCache;
  
  if (!sStyleCache)
    sStyleCache = new nsATSUStyleCache;
    
  return sStyleCache;
}
