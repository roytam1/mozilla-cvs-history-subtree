

#include <MacTypes.h>
#include <Fonts.h>
#include <TextEncodingConverter.h>
#include <ATSUnicode.h>

#include "nsQuickSort.h"
#include "nsCRT.h"
#include "nsHashtable.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"


#include "nsUnicodeMappingUtil.h"   // only for nsGenericFontNameType
#include "nsFontEnumerator.h"


//------------------------------------------------------------------------
class FontNameKey : public nsHashKey
{
public:
  FontNameKey(const nsAString& aString)
  {
	  mString.Assign(aString);
  }

  virtual PRUint32 HashCode(void) const;
  virtual PRBool Equals(const nsHashKey *aKey) const;
  virtual nsHashKey *Clone(void) const;

  nsAutoString  mString;      // would an nsString be better?
};


PRUint32 FontNameKey::HashCode(void) const
{
  nsString str;
  ToLowerCase(mString, str);
  return nsCRT::HashCode(str.get());
}

PRBool FontNameKey::Equals(const nsHashKey *aKey) const
{
  return mString.Equals(((FontNameKey*)aKey)->mString,
                          nsCaseInsensitiveStringComparator());
}

nsHashKey* FontNameKey::Clone(void) const
{
  return new FontNameKey(mString);
}


#pragma mark -

//------------------------------------------------------------------------
nsFontEnumeratorMac::nsFontEnumeratorMac()
{
  NS_INIT_REFCNT();
}


//------------------------------------------------------------------------
nsFontEnumeratorMac::~nsFontEnumeratorMac()
{
}


NS_IMPL_ISUPPORTS1(nsFontEnumeratorMac, nsIFontEnumerator)

typedef struct EnumerateFamilyInfo
{
  PRUnichar** mArray;
  PRInt32     mIndex;
} EnumerateFamilyInfo;

typedef struct EnumerateFontInfo
{
  PRUnichar** mArray;
  PRInt32     mIndex;
  PRInt32     mCount;
  ScriptCode	mScript;
  nsGenericFontNameType mType;
} EnumerateFontInfo;


int
nsFontEnumeratorMac::CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure)
{
  const PRUnichar* str1 = *((const PRUnichar**) aArg1);
  const PRUnichar* str2 = *((const PRUnichar**) aArg2);

  // XXX add nsICollation stuff

  return nsCRT::strcmp(str1, str2);
}


PRBool
nsFontEnumeratorMac::EnumerateFamily(nsHashKey *aKey, void *aData, void* closure)
{
  EnumerateFamilyInfo* info = (EnumerateFamilyInfo*) closure;
  PRUnichar** array = info->mArray;
  PRInt32 j = info->mIndex;
  
  PRUnichar* str = ToNewUnicode(((FontNameKey*)aKey)->mString);
  if (!str) {
    for (j = j - 1; j >= 0; j--) {
      nsMemory::Free(array[j]);
    }
    info->mIndex = 0;
    return PR_FALSE;
  }
  
  array[j] = str;
  info->mIndex++;

  return PR_TRUE;
}


NS_IMETHODIMP
nsFontEnumeratorMac::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  *aCount = 0;
  *aResult = nsnull;

	nsHashtable* list = GetFontList();
	if (!list)
		return NS_ERROR_FAILURE;

	PRInt32 items = list->Count();
  PRUnichar** array = (PRUnichar**)nsMemory::Alloc(items * sizeof(PRUnichar*));
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  EnumerateFamilyInfo info = { array, 0 };
  list->Enumerate(nsFontEnumeratorMac::EnumerateFamily, &info);
  NS_ASSERTION(items == info.mIndex, "didn't get all the fonts");
  if (!info.mIndex) {
    nsMemory::Free(array);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_QuickSort(array, info.mIndex, sizeof(PRUnichar*), nsFontEnumeratorMac::CompareFontNames, nsnull);

  *aCount = info.mIndex;
  *aResult = array;
  return NS_OK;
}

PRBool
nsFontEnumeratorMac::EnumerateFont(nsHashKey *aKey, void *aData, void* closure)
{
  EnumerateFontInfo* info = (EnumerateFontInfo*) closure;
  PRUnichar** array = info->mArray;
  PRInt32 j = info->mCount;
  
  PRBool match = PR_FALSE;
#if TARGET_CARBON
  // we need to match the cast of FMFontFamily in nsDeviceContextMac :: InitFontInfoList()
  FMFontFamily fontFamily = (FMFontFamily) aData;
  TextEncoding fontEncoding;
  OSStatus status = ::FMGetFontFamilyTextEncoding(fontFamily, &fontEncoding);
  if (noErr == status) {
    ScriptCode script;
    status = ::RevertTextEncodingToScriptInfo(fontEncoding, &script, nsnull, nsnull);
    match = ((noErr == status) && (script == info->mScript));
  }
#else
  short	fondID = (short) aData;
  ScriptCode script = ::FontToScript(fondID);
	match = (script == info->mScript);
#endif

	if (match)
	{
	  PRUnichar* str = ToNewUnicode(((FontNameKey*)aKey)->mString);
	  if (!str) {
	    for (j = j - 1; j >= 0; j--) {
	      nsMemory::Free(array[j]);
	    }
	    info->mIndex = 0;
	    return PR_FALSE;
	  }
	  array[j] = str;
	  info->mCount++;
	}
	info->mIndex++;
  return PR_TRUE;
}

NS_IMETHODIMP
nsFontEnumeratorMac::EnumerateFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG(aLangGroup);
  NS_ENSURE_ARG(aGeneric);

  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  *aCount = 0;
  *aResult = nsnull;

  if ((!strcmp(aLangGroup, "x-unicode")) ||
      (!strcmp(aLangGroup, "x-user-def"))) {
    return EnumerateAllFonts(aCount, aResult);
  }

	nsHashtable* list = GetFontList();
	if (!list)
		return NS_ERROR_FAILURE;

	PRInt32 items = list->Count();
  PRUnichar** array = (PRUnichar**)nsMemory::Alloc(items * sizeof(PRUnichar*));
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  nsUnicodeMappingUtil* gUtil = nsUnicodeMappingUtil::GetSingleton();
	if (!gUtil) {
		return NS_ERROR_FAILURE;
	}
  
  nsAutoString genName; genName.AssignWithConversion(aGeneric);
  EnumerateFontInfo info = { array, 0 , 0, gUtil->MapLangGroupToScriptCode(aLangGroup), gUtil->MapGenericFontNameType(genName) };
  list->Enumerate(nsFontEnumeratorMac::EnumerateFont, &info);
  if (!info.mIndex) {
    nsMemory::Free(array);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_QuickSort(array, info.mCount, sizeof(PRUnichar*), nsFontEnumeratorMac::CompareFontNames, nsnull);

  *aCount = info.mCount;
  *aResult = array;
  return NS_OK;
}


NS_IMETHODIMP
nsFontEnumeratorMac::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG(aLangGroup);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;
  PRUint32 count;
  PRUnichar **ptr;
  
  // optimize this!
  nsresult res = nsFontEnumeratorMac::EnumerateFonts(aLangGroup, "", &count, &ptr);
  NS_ENSURE_SUCCESS(res, res);
  *aResult = (count > 0);
  PRUint32 i;
  for(i = 0 ; i < count; i++)
  	nsMemory::Free(ptr[i]);
  nsMemory::Free(ptr);
  return NS_OK;
}


NS_IMETHODIMP
nsFontEnumeratorMac::UpdateFontList(PRBool *updateFontList)
{
  *updateFontList = PR_FALSE; // always return false for now
  return NS_OK;
}

void nsFontEnumeratorMac::InitFontInfoList(nsHashtable** outHashTable)
{
	nsHashtable*    fontList = nsnull;
	
	fontList = new nsHashtable();
	if (!fontList)
	  return;
	
  nsFontEnumeratorMac::FillFontTable(fontList);
	*outHashTable = fontList;
}


nsHashtable* nsFontEnumeratorMac::GetFontList()
{
  static nsHashtable*   gFontList;
  
  if (!gFontList)
  {
    InitFontInfoList(&gFontList);
  }
  
  return gFontList;
}


PRBool nsFontEnumeratorMac::GetMacFontID(const nsAString& inFontName, short &outFontID)
{
  nsHashtable*  fontList = GetFontList();
  
  FontNameKey key(inFontName);
	outFontID = (short)fontList->Get(&key);      // really an FMFontFamily for carbon

	return (outFontID != 0) && (kFontIDSymbol != outFontID);
}


#pragma mark -

// used as a size-1 cache

typedef struct TTextToUnicodeConverter
{
  TECObjectRef    mTextConverter;
  TextEncoding    mSourceEncoding;
} TTextToUnicodeConverter, *TTextToUnicodeConverterPtr;


// make a new converter, if necessary, to convert from the given encoding to unicode.
static OSStatus EnsureTextConverter(TextEncoding inSourceEncoding, TTextToUnicodeConverterPtr ioConverter)
{
  OSStatus  err = noErr;

  if ((ioConverter->mTextConverter == nsnull) || (ioConverter->mSourceEncoding != inSourceEncoding))
  {
    if (ioConverter->mTextConverter)
    {
      (void)::TECDisposeConverter(ioConverter->mTextConverter);
      ioConverter->mTextConverter = nsnull;
    }

    TextEncoding unicodeEncoding = ::CreateTextEncoding(kTextEncodingUnicodeDefault, 
                                          kTextEncodingDefaultVariant,
                                          kTextEncodingDefaultFormat);

    ioConverter->mSourceEncoding = inSourceEncoding;
    err = ::TECCreateConverter(&ioConverter->mTextConverter, inSourceEncoding, unicodeEncoding);
    if (err != noErr)
    {
      ioConverter->mTextConverter = nsnull;
      NS_ASSERTION(0, "Error making converter");
    }
  }

  return err;
}


static void DisposeTextConverter(TTextToUnicodeConverterPtr inConverter)
{
    if (inConverter->mTextConverter)
    {
      (void)::TECDisposeConverter(inConverter->mTextConverter);
      inConverter->mTextConverter = nsnull;
    }
}

// this null-terminates outBuffer
static OSStatus ConvertFontNameToUnicode(TTextToUnicodeConverterPtr ioConverter, ConstStr255Param inFontName, TextEncoding inTextEncoding,
              PRUnichar* outBuffer, PRInt32 inBufferCharacterSize)
{
  OSStatus  err;
  
  err = EnsureTextConverter(inTextEncoding, ioConverter);
  if (err != noErr) 
    return err;

  ByteCount actualInputLength, actualOutputLength;

  err = ::TECConvertText(ioConverter->mTextConverter, &inFontName[1], inFontName[0], &actualInputLength, 
        (TextPtr)outBuffer, sizeof(PRUnichar) * (inBufferCharacterSize - 1), &actualOutputLength);	
  if (err != noErr)
  {
    NS_WARNING("Error converting font name to unicode. Buffer too small?");
    return err;
  }

  outBuffer[actualOutputLength / sizeof(PRUnichar)] = '\0';
  return noErr;
}


#pragma mark ¥ÊClassic Font enumeration

#if !TARGET_API_MAC_CARBON

void nsFontEnumeratorMac::FillFontTable(nsHashtable* inFontTable)
{
	OSStatus        err;

  short numFONDs = ::CountResources('FOND');

  ScriptCode    lastscript = smUninterp;
  TextEncoding  lastSourceEncoding = kTextEncodingMacUninterp;

  TTextToUnicodeConverter   textConverter = { nsnull, kTextEncodingMacUninterp };

  for (short i = 1; i <= numFONDs ; i++)
  {
    Handle fond = ::GetIndResource('FOND', i);
    if (fond)
    {
      short	  fondID;
      OSType	resType;
      Str255	fontName;
      ::GetResInfo(fond, &fondID, &resType, fontName); 

      if ((0 != fontName[0]) && ('.' != fontName[1]) && ('%' != fontName[1]))
      {
        ScriptCode script = ::FontToScript(fondID);
        if (script != lastscript)
        {
          lastscript = script;
          (void)::UpgradeScriptInfoToTextEncoding(script, kTextLanguageDontCare, 
                      kTextRegionDontCare, NULL, &lastSourceEncoding);
        }

        const PRInt32 kUnicodeBufferSize = 256;
        PRUnichar unicodeFontName[kUnicodeBufferSize];

        if (ConvertFontNameToUnicode(&textConverter, fontName, lastSourceEncoding, unicodeFontName, kUnicodeBufferSize) == noErr)
        {
          nsDependentString depString(unicodeFontName);
          FontNameKey       key(depString);
          inFontTable->Put(&key, (void*)fondID);
        }
        else
        {
          NS_WARNING("Failed to convert font name");
        }
      }

      ::ReleaseResource(fond);
    }
  }

  DisposeTextConverter(&textConverter);
}

#endif

#pragma mark ¥ÊCarbon Font enumeration

#if TARGET_API_MAC_CARBON // && !defined(UNICODE_FONT_RENDERING)

void nsFontEnumeratorMac::FillFontTable(nsHashtable* inFontTable)
{
	OSStatus        err;

  // use the new Font Manager enumeration API.
  FMFontFamilyIterator iter;
  err = ::FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, &iter);
  if (err != noErr)
    return;

  TTextToUnicodeConverter   textConverter = { nsnull, kTextEncodingMacUninterp };

  // enumerate all fonts.
  FMFontFamily fontFamily;
  
  while (::FMGetNextFontFamily(&iter, &fontFamily) == noErr)
  {
    Str255 fontName;
    err = ::FMGetFontFamilyName(fontFamily, fontName);
    if (err != noErr || fontName[0] == 0 || fontName[1] == '.' || fontName[1] == '%')
      continue;

    TextEncoding fontEncoding;
    (void)::FMGetFontFamilyTextEncoding(fontFamily, &fontEncoding);

    const PRInt32 kUnicodeBufferSize = 256;
    PRUnichar unicodeFontName[kUnicodeBufferSize];

    if (ConvertFontNameToUnicode(&textConverter, fontName, fontEncoding, unicodeFontName, kUnicodeBufferSize) == noErr)
    {
      nsDependentString depString(unicodeFontName);
      FontNameKey       key(depString);
      inFontTable->Put(&key, (void*)fontFamily);
    }
    else
    {
      NS_WARNING("Failed to convert font name");
    }
  }

  DisposeTextConverter(&textConverter);

  (void)::FMDisposeFontFamilyIterator(&iter);
  
}

#endif // TARGET_API_MAC_CARBON && !defined(UNICODE_FONT_RENDERING)


#pragma mark ¥ÊCarbon Font enumeration

#if 0 // TARGET_API_MAC_CARBON && defined(UNICODE_FONT_RENDERING)
// Unused for now.

void nsFontEnumeratorMac::FillFontTable(nsHashtable* inFontTable)
{
  OSStatus err;
  
  ItemCount   numFonts;
  err = ::ATSUFontCount(&numFonts);
  if (err != noErr) {
    NS_ASSERTION(0, "Error counting ATSU fonts");
    return;
  }
  
  ATSUFontID*    fontIDList = (ATSUFontID*)malloc(numFonts * sizeof(ATSUFontID));
  if (!fontIDList) return;
  
  ItemCount   numFontIDs;
  err = ::ATSUGetFontIDs(fontIDList, numFonts, &numFontIDs);
  NS_ASSERTION((err == noErr) && (numFonts == numFontIDs), "ATSUGetFontIDs error");
  
  for (ItemCount i = 0; i < numFontIDs; i ++)
  {
    ATSUFontID    thisFontID = fontIDList[i];
  
    const PRInt32 kUnicodeBufferSize = 256;
    PRUnichar     unicodeFontName[kUnicodeBufferSize];
    ByteCount     actualNameBytes;
    ItemCount     nameIndex;
    
    // this will currently include all fonts, including multiple fonts in a single family
    // *** does not work (can't return unicode font names??)
    err = ::ATSUFindFontName(thisFontID, kFontFamilyName, kFontUnicodePlatform, kFontUnicodeDefaultSemantics,
        kFontEnglishLanguage, sizeof(PRUnichar) * kUnicodeBufferSize, (Ptr)unicodeFontName, &actualNameBytes, &nameIndex);
    NS_ASSERTION(err == noErr, "Error finding font name -- buffer too small?");
    if (err == noErr)
    {
      Style   fontStyle;
      SInt16  fondID;
      
      ::ATSUFontIDtoFOND(thisFontID, &fondID, &fontStyle);
      
      if (fontStyle == normal)      // ?? Maybe look up the name to ensure that it hasn't already been added
      {
        nsDependentString depString(unicodeFontName);
        FontNameKey       key(depString);
        inFontTable->Put(&key, (void*)fondID);
      }
    }
  }

  free((void*)fontIDList);
}

#endif //  TARGET_API_MAC_CARBON && defined()
