/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by IBM Corporation are Copyright (C) 2003
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Darin Fisher <darin@meer.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsTStringBase.h"

  /**
   * nsTStringBase obsolete API support
   */

#if MOZ_STRING_WITH_OBSOLETE_API

#include "nsTString.h"
#include "nsTDependentString.h"
#include "nsTDependentSubstring.h"
#include "nsReadableUtils.h"
#include "bufferRoutines.h"
#include "nsCRT.h"


static const char* kWhitespace="\b\t\r\n ";


//-----------------------------------------------------------------------------

template <class CharT>
static CharT
GetFindInSetFilter( const CharT* set)
  {
    // Calculate filter
    CharT filter = ~CharT(0); // All bits set
    while (*set) {
      filter &= ~(*set);
      ++set;
    }
    return filter;
  }

//-----------------------------------------------------------------------------

template <class CharT> struct nsObsoleteBufferRoutines {};

NS_SPECIALIZE_TEMPLATE
struct nsObsoleteBufferRoutines<char>
  {
    static
    PRInt32 compare( const char* a, const char* b, PRUint32 max, PRBool ic )
      {
        return Compare1To1(a, b, max, ic);
      }

    static
    PRInt32 compare( const char* a, const PRUnichar* b, PRUint32 max, PRBool ic )
      {
        return Compare1To2(a, b, max, ic);
      }

    static
    PRInt32 find_char( const char* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return FindChar1(s, max, offset, c, count);
      }

    static
    PRInt32 rfind_char( const char* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return RFindChar1(s, max, offset, c, count);
      }

    static
    char get_find_in_set_filter( const char* set )
      {
        return GetFindInSetFilter(set);
      }

    static
    PRInt32 strip_chars( char* s, PRUint32 len, const char* set )
      {
        return StripChars1(s, len, set);
      }

    static
    PRInt32 compress_chars( char* s, PRUint32 len, const char* set ) 
      {
        return CompressChars1(s, len, set);
      }
  };

NS_SPECIALIZE_TEMPLATE
struct nsObsoleteBufferRoutines<PRUnichar>
  {
    static
    PRInt32 compare( const PRUnichar* a, const PRUnichar* b, PRUint32 max, PRBool ic )
      {
        NS_ASSERTION(!ic, "no case-insensitive compare here");
        return Compare2To2(a, b, max);
      }

    static
    PRInt32 compare( const PRUnichar* a, const char* b, PRUint32 max, PRBool ic )
      {
        return Compare2To1(a, b, max, ic);
      }

    static
    PRInt32 find_char( const PRUnichar* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return FindChar2(s, max, offset, c, count);
      }

    static
    PRInt32 rfind_char( const PRUnichar* s, PRUint32 max, PRInt32 offset, const PRUnichar c, PRInt32 count )
      {
        return RFindChar2(s, max, offset, c, count);
      }

    static
    PRUnichar get_find_in_set_filter( const PRUnichar* set )
      {
        return GetFindInSetFilter(set);
      }

    static
    PRUnichar get_find_in_set_filter( const char* set )
      {
        return (~PRUnichar(0)^~char(0)) | GetFindInSetFilter(set);
      }

    static
    PRInt32 strip_chars( PRUnichar* s, PRUint32 max, const char* set )
      {
        return StripChars2(s, max, set);
      }

    static
    PRInt32 compress_chars( PRUnichar* s, PRUint32 len, const char* set ) 
      {
        return CompressChars2(s, len, set);
      }
  };

//-----------------------------------------------------------------------------

template <class L, class R>
static PRInt32
FindSubstring( const L* big, PRUint32 bigLen,
               const R* little, PRUint32 littleLen,
               PRBool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    PRInt32 i, max = PRInt32(bigLen - littleLen);
    for (i=0; i<=max; ++i, ++big)
      {
        if (nsObsoleteBufferRoutines<L>::compare(big, little, littleLen, ignoreCase) == 0)
          return i;
      }

    return kNotFound;
  }

template <class L, class R>
static PRInt32
RFindSubstring( const L* big, PRUint32 bigLen,
                const R* little, PRUint32 littleLen,
                PRBool ignoreCase )
  {
    if (littleLen > bigLen)
      return kNotFound;

    PRInt32 i, max = PRInt32(bigLen - littleLen);

    const L* iter = big + max;
    for (i=max; iter >= big; --i, --iter)
      {
        if (nsObsoleteBufferRoutines<L>::compare(iter, little, littleLen, ignoreCase) == 0)
          return i;
      }

    return kNotFound;
  }

template <class CharT, class SetCharT>
static PRInt32
FindCharInSet( const CharT* data, PRUint32 dataLen, const SetCharT* set )
  {
    CharT filter = nsObsoleteBufferRoutines<CharT>::get_find_in_set_filter(set);

    const CharT* end = data + dataLen; 
    for (const CharT* iter = data; iter < end; ++iter)
      {
        CharT currentChar = *iter;
        if (currentChar & filter)
          continue; // char is not in filter set; go on with next char.

        // test all chars
        const SetCharT* charInSet = set;
        CharT setChar = CharT(*charInSet);
        while (setChar)
          {
            if (setChar == currentChar)
              return iter - data; // found it!  return index of the found char.

            setChar = CharT(*(++charInSet));
          }
      }
    return kNotFound;
  }

template <class CharT, class SetCharT>
static PRInt32
RFindCharInSet( const CharT* data, PRUint32 dataLen, const SetCharT* set )
  {
    CharT filter = nsObsoleteBufferRoutines<CharT>::get_find_in_set_filter(set);

    for (const CharT* iter = data + dataLen; iter >= data; --iter)
      {
        CharT currentChar = *iter;
        if (currentChar & filter)
          continue; // char is not in filter set; go on with next char.

        // test all chars
        const CharT* charInSet = set;
        CharT setChar = *charInSet;
        while (setChar)
          {
            if (setChar == currentChar)
              return iter - data; // found it!  return index of the found char.

            setChar = *(++charInSet);
          }
      }
    return kNotFound;
  }

  // this is rickg's original code from AppendInt
static void
IntegerToCString( PRInt32 aInteger, PRInt32 aRadix, char *aBuf )
  {
    PRUint32 theInt=(PRUint32)aInteger;

    PRInt32 radices[] = {1000000000,268435456};
    PRInt32 mask1=radices[16==aRadix];

    PRInt32 charpos=0;
    if(aInteger<0) {
      theInt*=-1;
      if(10==aRadix) {
        aBuf[charpos++]='-';
      }
      else theInt=(int)~(theInt-1);
    }

    PRBool isfirst=PR_TRUE;
    while(mask1>=1) {
      PRInt32 theDiv=theInt/mask1;
      if((theDiv) || (!isfirst)) {
        aBuf[charpos++]="0123456789abcdef"[theDiv];
        isfirst=PR_FALSE;
      }
      theInt-=theDiv*mask1;
      mask1/=aRadix;
    }
  }

/**
 * This is a copy of |PR_cnvtf| with a bug fixed.  (The second argument
 * of PR_dtoa is 2 rather than 1.)
 *
 * XXXdarin if this is the right thing, then why wasn't it fixed in NSPR?!?
 */
void 
Modified_cnvtf(char *buf, int bufsz, int prcsn, double fval)
{
  PRIntn decpt, sign, numdigits;
  char *num, *nump;
  char *bufp = buf;
  char *endnum;

  /* If anything fails, we store an empty string in 'buf' */
  num = (char*)PR_MALLOC(bufsz);
  if (num == NULL) {
    buf[0] = '\0';
    return;
  }
  if (PR_dtoa(fval, 2, prcsn, &decpt, &sign, &endnum, num, bufsz)
      == PR_FAILURE) {
    buf[0] = '\0';
    goto done;
  }
  numdigits = endnum - num;
  nump = num;

  /*
   * The NSPR code had a fancy way of checking that we weren't dealing
   * with -0.0 or -NaN, but I'll just use < instead.
   * XXX Should we check !isnan(fval) as well?  Is it portable?  We
   * probably don't need to bother since NAN isn't portable.
   */
  if (sign && fval < 0.0f) {
    *bufp++ = '-';
  }

  if (decpt == 9999) {
    while ((*bufp++ = *nump++) != 0) {} /* nothing to execute */
    goto done;
  }

  if (decpt > (prcsn+1) || decpt < -(prcsn-1) || decpt < -5) {
    *bufp++ = *nump++;
    if (numdigits != 1) {
      *bufp++ = '.';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = 'e';
    PR_snprintf(bufp, bufsz - (bufp - buf), "%+d", decpt-1);
  }
  else if (decpt >= 0) {
    if (decpt == 0) {
      *bufp++ = '0';
    }
    else {
      while (decpt--) {
        if (*nump != '\0') {
          *bufp++ = *nump++;
        }
        else {
          *bufp++ = '0';
        }
      }
    }
    if (*nump != '\0') {
      *bufp++ = '.';
      while (*nump != '\0') {
        *bufp++ = *nump++;
      }
    }
    *bufp++ = '\0';
  }
  else if (decpt < 0) {
    *bufp++ = '0';
    *bufp++ = '.';
    while (decpt++) {
      *bufp++ = '0';
    }

    while (*nump != '\0') {
      *bufp++ = *nump++;
    }
    *bufp++ = '\0';
  }
done:
  PR_DELETE(num);
}

  /**
   * this method changes the meaning of |offset| and |count|:
   * 
   * upon return,
   *   |offset| specifies start of search range
   *   |count| specifies length of search range
   */ 
static void
Find_ComputeSearchRange( PRUint32 bigLen, PRUint32 littleLen, PRInt32& offset, PRInt32& count )
  {
    // |count| specifies how many iterations to make from |offset|

    if (offset < 0 || PRUint32(offset) > bigLen)
      {
        count = 0;
        return;
      }

    PRInt32 maxCount = bigLen - offset;
    if (count < 0 || count > maxCount)
      {
        count = maxCount;
      } 
    else
      {
        count += littleLen;
        if (count > maxCount)
          count = maxCount;
      }
  }

  /**
   * this method changes the meaning of |offset| and |count|:
   * 
   * upon return,
   *   |offset| specifies start of search range
   *   |count| specifies length of search range
   */ 
static void
RFind_ComputeSearchRange( PRUint32 bigLen, PRUint32 littleLen, PRInt32& offset, PRInt32& count )
  {
    // |count| specifies how many iterations to make from |offset|

    if (littleLen > bigLen)
      {
        count = 0;
        return;
      }

    PRInt32 maxOffset = PRInt32(bigLen - littleLen);
    if (offset < 0)
      {
        offset = maxOffset;
      }
    else if (offset > maxOffset)
      {
        count = 0;
        return;
      }

    // always do at least one iteration
    PRInt32 maxCount = offset + 1;
    if (count < 0 || count > maxCount)
      {
        count = maxCount;
      }
    else
      {
        count += littleLen;
        if (count > maxCount)
          count = maxCount;
      }

    offset -= (count - littleLen);
  }

//-----------------------------------------------------------------------------


  /**
   * nsTStringBase::Find
   *
   * aOffset specifies starting index
   * aCount specifies number of string compares (iterations)
   */

template <class CharT>
PRInt32
nsTStringBase<CharT>::Find( const nsCString& aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    // this method changes the meaning of aOffset and aCount:
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

template <class CharT>
PRInt32
nsTStringBase<CharT>::Find( const char* aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    return Find(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::Find( const nsAFlatString& aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    // this method changes the meaning of aOffset and aCount:
    Find_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = FindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), PR_FALSE);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::Find( const PRUnichar* aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    return Find(nsDependentString(aString), aOffset, aCount);
  }


  /**
   * nsTStringBase::RFind
   *
   * aOffset specifies starting index
   * aCount specifies number of string compares (iterations)
   */

template <class CharT>
PRInt32
nsTStringBase<CharT>::RFind( const nsCString& aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    // this method changes the meaning of aOffset and aCount:
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), aIgnoreCase);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

template <class CharT>
PRInt32
nsTStringBase<CharT>::RFind( const char* aString, PRBool aIgnoreCase, PRInt32 aOffset, PRInt32 aCount) const
  {
    return RFind(nsDependentCString(aString), aIgnoreCase, aOffset, aCount);
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::RFind( const nsAFlatString& aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    // this method changes the meaning of aOffset and aCount:
    RFind_ComputeSearchRange(mLength, aString.Length(), aOffset, aCount);

    PRInt32 result = RFindSubstring(mData + aOffset, aCount, aString.get(), aString.Length(), PR_FALSE);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::RFind( const PRUnichar* aString, PRInt32 aOffset, PRInt32 aCount ) const
  {
    return RFind(nsDependentString(aString), aOffset, aCount);
  }


  /**
   * nsTStringBase::RFindChar
   */

template <class CharT>
PRInt32
nsTStringBase<CharT>::RFindChar( PRUnichar aChar, PRInt32 aOffset, PRInt32 aCount) const
  {
    return nsObsoleteBufferRoutines<CharT>::rfind_char(mData, mLength, aOffset, aChar, aCount);
  }


  /**
   * nsTStringBase::FindCharInSet
   */

template <class CharT>
PRInt32
nsTStringBase<CharT>::FindCharInSet( const char* aSet, PRInt32 aOffset ) const
  {
    if (aOffset < 0)
      aOffset = 0;
    else if (aOffset >= PRInt32(mLength))
      return kNotFound;
    
    PRInt32 result = ::FindCharInSet(mData + aOffset, mLength - aOffset, aSet);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::FindCharInSet( const PRUnichar* aSet, PRInt32 aOffset ) const
  {
    if (aOffset < 0)
      aOffset = 0;
    else if (aOffset >= PRInt32(mLength))
      return kNotFound;
    
    PRInt32 result = ::FindCharInSet(mData + aOffset, mLength - aOffset, aSet);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }


  /**
   * nsTStringBase::RFindCharInSet
   */

template <class CharT>
PRInt32
nsTStringBase<CharT>::RFindCharInSet( const CharT* aSet, PRInt32 aOffset ) const
  {
    if (aOffset < 0)
      aOffset = 0;
    else if (aOffset >= PRInt32(mLength))
      return kNotFound;
    
    PRInt32 result = ::RFindCharInSet(mData + aOffset, mLength - aOffset, aSet);
    if (result != kNotFound)
      result += aOffset;
    return result;
  }


  /**
   * nsTStringBase::Compare,CompareWithConversion,etc.
   */

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<char>::Compare( const char* aString, PRBool aIgnoreCase, PRInt32 aCount ) const
  {
    PRUint32 strLen = char_traits::length(aString);

    PRInt32 maxCount = PRInt32(NS_MIN(mLength, strLen));

    PRInt32 compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    PRInt32 result =
        nsObsoleteBufferRoutines<char>::compare(mData, aString, compareCount, aIgnoreCase);

    if (result == 0 &&
          (aCount < 0 || strLen < PRUint32(aCount) || mLength < PRUint32(aCount)))
      {
        // Since the caller didn't give us a length to test, or strings shorter
        // than aCount, and compareCount characters matched, we have to assume
        // that the longer string is greater.

        if (mLength != strLen)
          result = (mLength < strLen) ? -1 : 1;
      }
    return result;
  }

NS_SPECIALIZE_TEMPLATE
PRInt32
nsTStringBase<PRUnichar>::CompareWithConversion( const char* aString, PRBool aIgnoreCase, PRInt32 aCount ) const
  {
    PRUint32 strLen = nsCharTraits<char>::length(aString);

    PRInt32 maxCount = PRInt32(NS_MIN(mLength, strLen));

    PRInt32 compareCount;
    if (aCount < 0 || aCount > maxCount)
      compareCount = maxCount;
    else
      compareCount = aCount;

    PRInt32 result =
        nsObsoleteBufferRoutines<PRUnichar>::compare(mData, aString, compareCount, aIgnoreCase);

    if (result == 0 &&
          (aCount < 0 || strLen < PRUint32(aCount) || mLength < PRUint32(aCount)))
      {
        // Since the caller didn't give us a length to test, or strings shorter
        // than aCount, and compareCount characters matched, we have to assume
        // that the longer string is greater.

        if (mLength != strLen)
          result = (mLength < strLen) ? -1 : 1;
      }
    return result;
  }

NS_SPECIALIZE_TEMPLATE
PRBool
nsTStringBase<char>::EqualsWithConversion( const char* aString, PRBool aIgnoreCase, PRInt32 aCount ) const
  {
    return Compare(aString, aIgnoreCase, aCount) == 0;
  }

NS_SPECIALIZE_TEMPLATE
PRBool
nsTStringBase<PRUnichar>::EqualsWithConversion( const char* aString, PRBool aIgnoreCase, PRInt32 aCount ) const
  {
    return CompareWithConversion(aString, aIgnoreCase, aCount) == 0;
  }

NS_SPECIALIZE_TEMPLATE
PRBool
nsTStringBase<PRUnichar>::IsASCII( const PRUnichar* aBuffer )
  {
    // XXX nsDependentString will compute the length of aBuffer and that is
    // something we could potentially optimize away, but there probably isn't
    // much point to doing so since this function is obsolete.

    return ::IsASCII(aBuffer ?
        NS_STATIC_CAST(const self_type&, nsDependentString(aBuffer)) : *this);
  }

NS_SPECIALIZE_TEMPLATE
PRBool
nsTStringBase<PRUnichar>::IsSpace( PRUnichar c )
  {
    // XXX i18n
    return (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
  }

  
  /**
   * ToCString, ToFloat, ToInteger
   */

NS_SPECIALIZE_TEMPLATE
char*
nsTStringBase<PRUnichar>::ToCString( char* aBuf, PRUint32 aBufLength, PRUint32 aOffset ) const
  {
      // because the old implementation checked aBuf
    if (!(aBuf && aBufLength > 0 && aOffset <= mLength))
      return nsnull;

    PRUint32 maxCount = NS_MIN(aBufLength-1, mLength - aOffset);

    CopyChars2To1(aBuf, 0, (const char*) mData, aOffset, maxCount);
    aBuf[maxCount] = 0;
    return aBuf;
  }

NS_SPECIALIZE_TEMPLATE
float
nsTStringBase<char>::ToFloat(PRInt32* aErrorCode) const
  {
    float res = 0.0f;
    if (mLength > 0)
      {
        char *conv_stopped;
        const char *str = mData;
        // Use PR_strtod, not strtod, since we don't want locale involved.
        res = (float)PR_strtod(str, &conv_stopped);
        if (conv_stopped == str+mLength)
          *aErrorCode = (PRInt32) NS_OK;
        else // Not all the string was scanned
          *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    else
      {
        // The string was too short (0 characters)
        *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    return res;
  }

NS_SPECIALIZE_TEMPLATE
float
nsTStringBase<PRUnichar>::ToFloat(PRInt32* aErrorCode) const
  {
    float res = 0.0f;
    char buf[100];
    if (mLength > 0 && mLength < sizeof(buf))
      {
        char *conv_stopped;
        const char *str = ToCString(buf, sizeof(buf));
        // Use PR_strtod, not strtod, since we don't want locale involved.
        res = (float)PR_strtod(str, &conv_stopped);
        if (conv_stopped == str+mLength)
          *aErrorCode = (PRInt32) NS_OK;
        else // Not all the string was scanned
          *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    else
      {
        // The string was too short (0 characters) or too long (sizeof(buf))
        *aErrorCode = (PRInt32) NS_ERROR_ILLEGAL_VALUE;
      }
    return res;
  }

  // it's a shame to replicate this code.  it was done this way in the past
  // to help performance.  this function also gets to keep the rickg style
  // indentation :-/
template <class CharT>
PRInt32
nsTStringBase<CharT>::ToInteger( PRInt32* aErrorCode, PRUint32 aRadix ) const
  {
  CharT*  cp=mData;
  PRInt32 theRadix=10; // base 10 unless base 16 detected, or overriden (aRadix != kAutoDetect)
  PRInt32 result=0;
  PRBool  negate=PR_FALSE;
  CharT   theChar=0;

    //initial value, override if we find an integer
  *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
  
  if(cp) {
  
    //begin by skipping over leading chars that shouldn't be part of the number...
    
    CharT*  endcp=cp+mLength;
    PRBool  done=PR_FALSE;
    
    while((cp<endcp) && (!done)){
      switch(*cp++) {
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
          theRadix=16;
          done=PR_TRUE;
          break;
        case '0': case '1': case '2': case '3': case '4': 
        case '5': case '6': case '7': case '8': case '9':
          done=PR_TRUE;
          break;
        case '-': 
          negate=PR_TRUE; //fall through...
          break;
        case 'X': case 'x': 
          theRadix=16;
          break; 
        default:
          break;
      } //switch
    }

    if (done) {

        //integer found
      *aErrorCode = NS_OK;

      if (aRadix!=kAutoDetect) theRadix = aRadix; // override

        //now iterate the numeric chars and build our result
      CharT* first=--cp;  //in case we have to back up.
      PRBool haveValue = PR_FALSE;

      while(cp<endcp){
        theChar=*cp++;
        if(('0'<=theChar) && (theChar<='9')){
          result = (theRadix * result) + (theChar-'0');
          haveValue = PR_TRUE;
        }
        else if((theChar>='A') && (theChar<='F')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; //backup
              result=0;
              haveValue = PR_FALSE;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'A')+10);
            haveValue = PR_TRUE;
          }
        }
        else if((theChar>='a') && (theChar<='f')) {
          if(10==theRadix) {
            if(kAutoDetect==aRadix){
              theRadix=16;
              cp=first; //backup
              result=0;
              haveValue = PR_FALSE;
            }
            else {
              *aErrorCode=NS_ERROR_ILLEGAL_VALUE;
              result=0;
              break;
            }
          }
          else {
            result = (theRadix * result) + ((theChar-'a')+10);
            haveValue = PR_TRUE;
          }
        }
        else if((('X'==theChar) || ('x'==theChar)) && (!haveValue || result == 0)) {
          continue;
        }
        else if((('#'==theChar) || ('+'==theChar)) && !haveValue) {
          continue;
        }
        else {
          //we've encountered a char that's not a legal number or sign
          break;
        }
      } //while
      if(negate)
        result=-result;
    } //if
  }
  return result;
}


  /**
   * nsTStringBase::Mid
   */

template <class CharT>
PRUint32
nsTStringBase<CharT>::Mid( self_type& aResult, index_type aStartPos, size_type aLengthToCopy ) const
  {
    if (aStartPos == 0 && aLengthToCopy >= mLength)
      aResult = *this;
    else
      aResult = Substring(*this, aStartPos, aLengthToCopy);

    return aResult.mLength;
  }


  /**
   * nsTStringBase::SetCharAt
   */

template <class CharT>
PRBool
nsTStringBase<CharT>::SetCharAt( PRUnichar aChar, PRUint32 aIndex )
  {
    if (aIndex >= mLength)
      return PR_FALSE;

    EnsureMutable();

    mData[aIndex] = CharT(aChar);
    return PR_TRUE;
  }

 
  /**
   * nsTStringBase::StripChars,StripChar,StripWhitespace
   */

template <class CharT>
void
nsTStringBase<CharT>::StripChars( const char* aSet )
  {
    EnsureMutable();
    mLength = nsObsoleteBufferRoutines<CharT>::strip_chars(mData, mLength, aSet);
  }

template <class CharT>
void
nsTStringBase<CharT>::StripChar( char_type aChar, PRInt32 aOffset )
  {
    if (mLength == 0 || aOffset >= PRInt32(mLength))
      return;

    EnsureMutable(); // XXX do this lazily?

    // XXXdarin this code should defer writing until necessary.

    char_type* to   = mData + aOffset;
    char_type* from = mData + aOffset;
    char_type* end  = mData + mLength;

    while (from < end)
      {
        char_type theChar = *from++;
        if (aChar != theChar)
          *to++ = theChar;
      }
    *to = char_type(0); // add the null
    mLength = to - mData;
  }

template <class CharT>
void
nsTStringBase<CharT>::StripWhitespace()
  {
    StripChars(kWhitespace);
  }


  /**
   * nsTStringBase::ReplaceChar,ReplaceSubstring
   */

template <class CharT>
void
nsTStringBase<CharT>::ReplaceChar( char_type aOldChar, char_type aNewChar )
  {
    EnsureMutable(); // XXX do this lazily?

    for (PRUint32 i=0; i<mLength; ++i)
      {
        if (mData[i] == aOldChar)
          mData[i] = aNewChar;
      }
  }

template <class CharT>
void
nsTStringBase<CharT>::ReplaceChar( const char* aSet, char_type aNewChar )
  {
    EnsureMutable(); // XXX do this lazily?

    char_type* data = mData;
    PRUint32 lenRemaining = mLength;

    while (lenRemaining)
      {
        PRInt32 i = ::FindCharInSet(data, lenRemaining, aSet);
        if (i == kNotFound)
          break;

        data[i++] = aNewChar;
        data += i;
        lenRemaining -= i;
      }
  }

template <class CharT>
void
nsTStringBase<CharT>::ReplaceSubstring( const char_type* aTarget, const char_type* aNewValue )
  {
    ReplaceSubstring(nsTDependentString<char_type>(aTarget),
                     nsTDependentString<char_type>(aNewValue));
  }

template <class CharT>
void
nsTStringBase<CharT>::ReplaceSubstring( const self_type& aTarget, const self_type& aNewValue )
  {
    PRInt32 i = 0;

    // XXX is this right?

    while (PRUint32(i) < mLength)
      {
        PRInt32 r = FindSubstring(mData + i, mLength - i, aTarget.Data(), aTarget.Length(), PR_FALSE);
        if (r == kNotFound)
          break;

        Replace(i + r, aTarget.Length(), aNewValue);
        i += aNewValue.Length();
      }
  }


  /**
   * nsTStringBase::Trim
   */

template <class CharT>
void
nsTStringBase<CharT>::Trim( const char* aSet, PRBool aTrimLeading, PRBool aTrimTrailing, PRBool aIgnoreQuotes )
  {
      // the old implementation worried about aSet being null :-/
    if (!aSet)
      return;

    char_type* start = mData;
    char_type* end   = mData + mLength;

      // skip over quotes if requested
    if (aIgnoreQuotes && mLength > 2 && mData[0] == mData[mLength - 1] &&
          (mData[0] == '\'' || mData[0] == '"'))
      {
        ++start;
        --end;
      }

    PRUint32 setLen = nsCharTraits<char>::length(aSet);

    if (aTrimLeading)
      {
        PRUint32 cutStart = start - mData;
        PRUint32 cutLength = 0;

          // walk forward from start to end
        for (; start != end; ++start, ++cutLength)
          {
            PRInt32 pos = FindChar1(aSet, setLen, 0, *start, setLen);
            if (pos == kNotFound)
              break;
          }

        if (cutLength)
          {
            Cut(cutStart, cutLength);

              // reset iterators
            start = mData + cutStart;
            end   = mData + mLength - cutStart;
          }
      }

    if (aTrimTrailing)
      {
        PRUint32 cutEnd = end - mData;
        PRUint32 cutLength = 0;

          // walk backward from end to start
        --end;
        for (; end >= start; --end, ++cutLength)
          {
            PRInt32 pos = FindChar1(aSet, setLen, 0, *end, setLen);
            if (pos == kNotFound)
              break;
          }

        if (cutLength)
          Cut(cutEnd - cutLength, cutLength);
      }
  }


  /**
   * nsTStringBase::CompressWhitespace
   */

template <class CharT>
void
nsTStringBase<CharT>::CompressWhitespace( PRBool aTrimLeading, PRBool aTrimTrailing )
  {
    const char* set = kWhitespace;

    ReplaceChar(set, ' ');
    Trim(set, aTrimLeading, aTrimTrailing);

      // this one does some questionable fu... just copying the old code!
    mLength = nsObsoleteBufferRoutines<char_type>::compress_chars(mData, mLength, set);
  }


  /**
   * nsTStringBase::AssignWithConversion
   */

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<char>::AssignWithConversion( const nsAString& aData )
  {
    LossyCopyUTF16toASCII(aData, *this);
  }

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<PRUnichar>::AssignWithConversion( const nsACString& aData )
  {
    CopyASCIItoUTF16(aData, *this);
  }

template <class CharT>
void
nsTStringBase<CharT>::AssignWithConversion( const incompatible_char_type* aData, PRInt32 aLength )
  {
      // for compatibility with the old string implementation, we need to allow
      // for a NULL input buffer :-(
    if (!aData)
      {
        Truncate();
      }
    else
      {
        if (aLength < 0)
          aLength = nsCharTraits<incompatible_char_type>::length(aData);

        AssignWithConversion(Substring(aData, aData + aLength));
      }
  }


  /**
   * nsTStringBase::AppendWithConversion
   */

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<char>::AppendWithConversion( const nsAString& aData )
  {
    LossyAppendUTF16toASCII(aData, *this);
  }

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<PRUnichar>::AppendWithConversion( const nsACString& aData )
  {
    AppendASCIItoUTF16(aData, *this);
  }

template <class CharT>
void
nsTStringBase<CharT>::AppendWithConversion( const incompatible_char_type* aData, PRInt32 aLength )
  {
      // for compatibility with the old string implementation, we need to allow
      // for a NULL input buffer :-(
    if (aData)
      {
        if (aLength < 0)
          aLength = nsCharTraits<incompatible_char_type>::length(aData);

        AppendWithConversion(Substring(aData, aData + aLength));
      }
  }


  /**
   * nsTStringBase::InsertWithConversion
   */

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<PRUnichar>::InsertWithConversion( const char* aData, PRUint32 aOffset, PRInt32 aCount )
  {
    if (aCount < 0)
      aCount = nsCharTraits<char>::length(aData);

    Insert(NS_ConvertASCIItoUTF16(aData), aOffset);
  }


  /**
   * nsTStringBase::AppendInt
   */

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<char>::AppendInt( PRInt32 aInteger, PRInt32 aRadix )
  {
    char buf[] = {'0',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    IntegerToCString(aInteger, aRadix, buf);
    Append(buf);
  }

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<PRUnichar>::AppendInt( PRInt32 aInteger, PRInt32 aRadix )
  {
    char buf[] = {'0',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    IntegerToCString(aInteger, aRadix, buf);
    AppendWithConversion(buf);
  }


  /**
   * nsTStringBase::AppendFloat
   */

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<char>::AppendFloat( double aFloat )
  {
    char buf[40];
    // Use Modified_cnvtf, which is locale-insensitive, instead of the
    // locale-sensitive PR_snprintf or sprintf(3)
    Modified_cnvtf(buf, sizeof(buf), 6, aFloat);
    Append(buf);
  }

NS_SPECIALIZE_TEMPLATE
void
nsTStringBase<PRUnichar>::AppendFloat( double aFloat )
  {
    char buf[40];
    // Use Modified_cnvtf, which is locale-insensitive, instead of the
    // locale-sensitive PR_snprintf or sprintf(3)
    Modified_cnvtf(buf, sizeof(buf), 6, aFloat);
    AppendWithConversion(buf);
  }


  /**
   * explicit template instantiation
   */

template PRInt32  nsTStringBase<char>::Find( const nsCString&, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::Find( const char*, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::RFind( const nsCString&, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::RFind( const char*, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::RFindChar( PRUnichar, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::FindCharInSet( const char*, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::RFindCharInSet( const char*, PRInt32 ) const;
template PRInt32  nsTStringBase<char>::ToInteger( PRInt32*, PRUint32 ) const;
template PRUint32 nsTStringBase<char>::Mid( self_type&, index_type, size_type ) const;
template PRBool   nsTStringBase<char>::SetCharAt( PRUnichar, PRUint32 );
template void     nsTStringBase<char>::StripChars( const char* );
template void     nsTStringBase<char>::StripChar( char, PRInt32 );
template void     nsTStringBase<char>::StripWhitespace();
template void     nsTStringBase<char>::ReplaceChar( char, char );
template void     nsTStringBase<char>::ReplaceChar( const char*, char );
template void     nsTStringBase<char>::ReplaceSubstring( const char*, const char* );
template void     nsTStringBase<char>::ReplaceSubstring( const self_type&, const self_type& );
template void     nsTStringBase<char>::Trim( const char*, PRBool, PRBool, PRBool );
template void     nsTStringBase<char>::CompressWhitespace( PRBool, PRBool );
template void     nsTStringBase<char>::AssignWithConversion( const PRUnichar*, PRInt32 );
template void     nsTStringBase<char>::AppendWithConversion( const PRUnichar*, PRInt32 );

template PRInt32  nsTStringBase<PRUnichar>::Find( const nsCString&, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::Find( const char*, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::RFind( const nsCString&, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::RFind( const char*, PRBool, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::RFindChar( PRUnichar, PRInt32, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::FindCharInSet( const char*, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::RFindCharInSet( const PRUnichar*, PRInt32 ) const;
template PRInt32  nsTStringBase<PRUnichar>::ToInteger( PRInt32*, PRUint32 ) const;
template PRUint32 nsTStringBase<PRUnichar>::Mid( self_type&, index_type, size_type ) const;
template PRBool   nsTStringBase<PRUnichar>::SetCharAt( PRUnichar, PRUint32 );
template void     nsTStringBase<PRUnichar>::StripChars( const char* );
template void     nsTStringBase<PRUnichar>::StripChar( PRUnichar, PRInt32 );
template void     nsTStringBase<PRUnichar>::StripWhitespace();
template void     nsTStringBase<PRUnichar>::ReplaceChar( PRUnichar, PRUnichar );
template void     nsTStringBase<PRUnichar>::ReplaceChar( const char*, PRUnichar );
template void     nsTStringBase<PRUnichar>::ReplaceSubstring( const PRUnichar*, const PRUnichar* );
template void     nsTStringBase<PRUnichar>::ReplaceSubstring( const self_type&, const self_type& );
template void     nsTStringBase<PRUnichar>::Trim( const char*, PRBool, PRBool, PRBool );
template void     nsTStringBase<PRUnichar>::CompressWhitespace( PRBool, PRBool );
template void     nsTStringBase<PRUnichar>::AssignWithConversion( const char*, PRInt32 );
template void     nsTStringBase<PRUnichar>::AppendWithConversion( const char*, PRInt32 );

#endif // !MOZ_STRING_WITH_OBSOLETE_API
