

#ifndef nsFontEnumerator_h__
#define nsFontEnumerator_h__

#include "nsIFontEnumerator.h"


class nsHashtable;
class nsHashKey;
class nsAString;

class nsFontEnumeratorMac : public nsIFontEnumerator
{
public:
                    nsFontEnumeratorMac();
  virtual           ~nsFontEnumeratorMac();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFONTENUMERATOR

public:

  static nsHashtable* GetFontList();
  static PRBool       GetMacFontID(const nsAString& inFontName, short &outFontID);

protected:

  // sort callback
  static int          CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure);
  // enumerator callbacks
  static PRBool       EnumerateFamily(nsHashKey *aKey, void *aData, void* closure);
  static PRBool       EnumerateFont(nsHashKey *aKey, void *aData, void* closure);
  
  
  static void         InitFontInfoList(nsHashtable** outHashTable);

protected:

  static void         FillFontTable(nsHashtable* inFontTable);
    
};


#endif // nsFontEnumerator_h__
