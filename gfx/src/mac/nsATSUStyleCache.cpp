
#include "nsATSUStyleCache.h"


const Float32 nsCharWidthCache::kUninitializedCharWidthValue = 1.0f;


//-------------------------------------------------------------------------
//  IsTextASCII
// 
//  return true if the text is 7-bit ascii (no high bits set)
//-------------------------------------------------------------------------
PRBool nsCharWidthCache::IsTextASCII(const PRUnichar* inText, PRInt32 inTextLength)
{
  const PRUnichar*  c     =  inText;
  const PRUnichar*  cEnd  =  inText + inTextLength;
  
  while (c < cEnd)
  {
    if (*c & 0xFF80)
      return PR_FALSE;
    c ++;
  }

  return PR_TRUE;
}

//-------------------------------------------------------------------------
//  GetTotalCharacterWidth
// 
//  try to get the width of all the characters from the cache. Returns PR_FALSE if
//  we don't have cached widths, or the string is not 7-bit ASCII. In this case,
//  the value of *outTotalWidth is undefined.
//-------------------------------------------------------------------------
PRBool nsCharWidthCache::GetTotalCharacterWidth(const PRUnichar* inText, PRInt32 inTextLength, Float32 *outTotalWidth)
{
  Float32             totalWidth  = 0.0f;
  const PRUnichar*    c           = inText;
  const PRUnichar*    cEnd        = c + inTextLength;
  
  while (c < cEnd)
  {
    Float32   charWidth = mCharWidthTable[*c];
    
    if ((*c & 0xFF80) || (charWidth == kUninitializedCharWidthValue))
      return PR_FALSE;
    
    totalWidth += charWidth;
    c ++;
  }

  *outTotalWidth = totalWidth;
  return PR_TRUE;
}

//-------------------------------------------------------------------------
//  CacheCharacterWidths
// 
//  put character widths in the cache
//-------------------------------------------------------------------------
void nsCharWidthCache::CacheCharacterWidths(const PRUnichar* inText, PRInt32 inTextLength, ATSUGlyphInfoArray* inGlyphBuffer, Float32 lastCharWidth)
{
  PRInt32    lastCharOffset = inTextLength - 1;
  
  for (PRInt32 i = 0; i < lastCharOffset; i ++)
  {
    const PRUnichar   uniChar = inText[i];
    
    NS_ASSERTION(inGlyphBuffer->glyphs[i].charIndex == i, "Text does not have a 1-to-1 glyph/character mapping");
  
    mCharWidthTable[uniChar] = inGlyphBuffer->glyphs[i + 1].idealX - inGlyphBuffer->glyphs[i].idealX;
  }

  // ick. We have to save the width of the last character separately
  if (lastCharWidth > 0.0)
    mCharWidthTable[inText[lastCharOffset]] = lastCharWidth;
}


#pragma mark -

//-------------------------------------------------------------------------
//  nsATSUStyleCache
// 
//-------------------------------------------------------------------------
nsATSUStyleCache::nsATSUStyleCache()
: mTable(nsnull)
, mCount(0)
{
  mTable = PL_NewHashTable(8, (PLHashFunction)HashKey, 
                      (PLHashComparator)CompareKeys, 
                      (PLHashComparator)CompareValues,
                      nsnull, nsnull);
  mCount = 0;
}


//-------------------------------------------------------------------------
//  ~nsATSUStyleCache
// 
//-------------------------------------------------------------------------
nsATSUStyleCache::~nsATSUStyleCache()
{
  if (mTable)
  {
    PL_HashTableEnumerateEntries(mTable, FreeHashEntries, 0);
    PL_HashTableDestroy(mTable);
  }
}

//-------------------------------------------------------------------------
//  Get
// 
//-------------------------------------------------------------------------
PRBool nsATSUStyleCache::GetStyle(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor,
                  ATSUStyle* outStyle, nsCharWidthCache** outWidthCache)
{
	nsAtsuStyleCacheKey k = {aFont, aSize, aColor, (aBold ? 1 : 0) + (aItalic ? 2 : 0) };
	return Get(&k, outStyle, outWidthCache);
}


//-------------------------------------------------------------------------
//  Set
// 
//-------------------------------------------------------------------------
void nsATSUStyleCache::SetStyle(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor,
                  ATSUStyle inStyle, nsCharWidthCache** outWidthCache)
{
	nsAtsuStyleCacheKey k = {aFont, aSize, aColor, (aBold ? 1 : 0) + (aItalic ? 2 : 0) };
	return Set(&k, inStyle, outWidthCache);
}


#pragma mark -

//-------------------------------------------------------------------------
//  Get
// 
//-------------------------------------------------------------------------
PRBool nsATSUStyleCache::Get(nsAtsuStyleCacheKey *key, ATSUStyle* outStyle, nsCharWidthCache** outWidthCache)
{
	PLHashEntry **hep = PL_HashTableRawLookup(mTable, HashKey(key), key);
	PLHashEntry *he = *hep;
	if (he)
	{
	  nsATSUStyleData*  cacheEntry = (nsATSUStyleData *)he->value;
		*outStyle       = cacheEntry->mATSUStyle;
		*outWidthCache   = &cacheEntry->mCharWidthCache;
		return PR_TRUE;
	}
	
	*outWidthCache = nsnull;
	*outStyle = nsnull;
	return PR_FALSE;
}


//-------------------------------------------------------------------------
//  Set
// 
//-------------------------------------------------------------------------
void nsATSUStyleCache::Set(nsAtsuStyleCacheKey *key, ATSUStyle inStyle, nsCharWidthCache** outWidthCache)
{
	nsAtsuStyleCacheKey *newKey   = new nsAtsuStyleCacheKey;
	nsATSUStyleData     *newEntry = new nsATSUStyleData(inStyle);

	if (newKey && newEntry)
	{
		*newKey = *key;
		PL_HashTableAdd(mTable, newKey, (void *)newEntry);
		mCount ++;
		
		*outWidthCache = &newEntry->mCharWidthCache;
	}
}


//-------------------------------------------------------------------------
//  HashKey
// 
//-------------------------------------------------------------------------
PR_CALLBACK PLHashNumber nsATSUStyleCache::HashKey(const void *aKey)
{
	nsAtsuStyleCacheKey* key = (nsAtsuStyleCacheKey*)aKey;	
	return 	key->font + (key->size << 7) + (key->style << 12) + key->color;
}


//-------------------------------------------------------------------------
//  CompareKeys
// 
//-------------------------------------------------------------------------
PR_CALLBACK PRIntn nsATSUStyleCache::CompareKeys(const void *v1, const void *v2)
{
	nsAtsuStyleCacheKey *k1 = (nsAtsuStyleCacheKey *)v1;
	nsAtsuStyleCacheKey *k2 = (nsAtsuStyleCacheKey *)v2;
	return (k1->font == k2->font) && (k1->color == k2->color) && (k1->size == k2->size) && (k1->style == k2->style);
}


//-------------------------------------------------------------------------
//  CompareValues
// 
//-------------------------------------------------------------------------
PR_CALLBACK PRIntn nsATSUStyleCache::CompareValues(const void *v1, const void *v2)
{
	nsATSUStyleData *t1 = (nsATSUStyleData*)v1;
	nsATSUStyleData *t2 = (nsATSUStyleData*)v2;
	return (t1 == t2);
}


//-------------------------------------------------------------------------
//  FreeHashEntries
// 
//-------------------------------------------------------------------------
PR_CALLBACK PRIntn nsATSUStyleCache::FreeHashEntries(PLHashEntry *he, PRIntn i, void *arg)
{
  nsATSUStyleData*  cacheEntry = (nsATSUStyleData *)he->value;
  if (cacheEntry)
  	::ATSUDisposeStyle(cacheEntry->mATSUStyle);
  delete cacheEntry;
	delete (nsAtsuStyleCacheKey*)he->key;
	return HT_ENUMERATE_REMOVE;
}

