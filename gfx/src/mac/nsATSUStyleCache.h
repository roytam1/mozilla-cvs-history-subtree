
#ifndef nsATSUStyleCache_h__
#define nsATSUStyleCache_h__

#include <math.h>
#include <ATSUnicode.h>

#include "plhash.h"
#include "nsColor.h"


struct PLHashTable;

class nsCharWidthCache
{
public:
  enum {
    kTableSize = 128
  };

  static const Float32 kUninitializedCharWidthValue;

                        nsCharWidthCache()
                        {
                          for (PRInt32 i = 0; i < kTableSize; i ++)
                            mCharWidthTable[i] = kUninitializedCharWidthValue;
                        }
                        
  Float32*              GetCharWidthTable() { return mCharWidthTable; }

                        // return true if the text is 7-bit ascii (no high bits set)
  PRBool                IsTextASCII(const PRUnichar* inText, PRInt32 inTextLength);
  
                        // try to get the width of all the characters from the cache. Returns PR_FALSE if
                        // we don't have cached widths, or the string is not 7-bit ASCII
  PRBool                GetTotalCharacterWidth(const PRUnichar* inText, PRInt32 inTextLength, Float32 *outTotalWidth);

                        // put character widths in the cache
  void                  CacheCharacterWidths(const PRUnichar* inText, PRInt32 inTextLength, ATSUGlyphInfoArray* inGlyphBuffer, Float32 lastCharWidth);

protected:

  Float32               mCharWidthTable[kTableSize];
};


class nsATSUStyleData
{
friend class nsATSUStyleCache;

public:
  
                      nsATSUStyleData(ATSUStyle inStyle)
                      : mATSUStyle(inStyle)
                      {
                      }

  ATSUStyle           GetATSUStyle()      { return mATSUStyle; }
  nsCharWidthCache&   GetCharWidthCache() { return mCharWidthCache; }

protected:

  ATSUStyle           mATSUStyle;
  nsCharWidthCache    mCharWidthCache;
  
};


class nsATSUStyleCache
{
public:

                      nsATSUStyleCache();
                      ~nsATSUStyleCache();

  PRBool              GetStyle(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor,
                                        ATSUStyle* outStyle, nsCharWidthCache** outWidthCache);
  void                SetStyle(short aFont, short aSize, PRBool aBold, PRBool aItalic, nscolor aColor,
                                        ATSUStyle inStyle, nsCharWidthCache** outWidthCache);
  
protected:

	struct nsAtsuStyleCacheKey
	{
		short   font;
		short	  size;
		nscolor color;
		short   style;      // bold or italic
	};
  
	static PR_CALLBACK PLHashNumber HashKey(const void *aKey);
	static PR_CALLBACK PRIntn		CompareKeys(const void *v1, const void *v2);
	static PR_CALLBACK PRIntn		CompareValues(const void *v1, const void *v2);
	static PR_CALLBACK PRIntn		FreeHashEntries(PLHashEntry *he, PRIntn i, void *arg);


	PRBool	Get(nsAtsuStyleCacheKey *key, ATSUStyle* outStyle, nsCharWidthCache** outWidthCache);
	void	  Set(nsAtsuStyleCacheKey *key, ATSUStyle inStyle, nsCharWidthCache** outWidthCache);

protected:


  PLHashTable     *mTable;
  PRUint32				mCount;

};



#endif // nsATSUStyleCache_h__
