/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#ifndef nsAReadableString_h___
#define nsAReadableString_h___

#ifndef nscore_h___
#include "nscore.h"
  // for |PRUnichar|
#endif

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
#endif

#ifndef nsPrivateSharableString_h___
#include "nsPrivateSharableString.h"
#endif

#ifndef nsStringIterator_h___
#include "nsStringIterator.h"
#endif

#include "nsMemory.h"

/*
  This file defines the abstract interfaces |nsAReadableString| and
  |nsAReadableCString| (the 'A' is for 'abstract', as opposed to the 'I' in
  [XP]COM interface names).

  These types are intended to be as source compatible as possible with the original
  definitions of |const nsString&| and |const nsCString&|, respectively.  In otherwords,
  these interfaces provide only non-mutating access to the underlying strings.  We
  split the these interfaces out from the mutating parts (see
  "nsAWritableString.h") because tests showed that we could exploit specialized
  implementations in some areas; we need an abstract interface to bring the whole
  family of strings together.

  |nsAReadableString| is a string of |PRUnichar|s.  |nsAReadableCString| (note the
  'C') is a string of |char|s.
*/

template <class CharT> class basic_nsAReadableString;

template <class CharT> class basic_nsAWritableString;
  // ...because we sometimes use them as `out' params

#ifdef _MSC_VER
    // Under VC++, at the highest warning level, we are overwhelmed  with warnings
    //  about a possible error when |operator->()| is used against something that
    //  doesn't have members, e.g., a |PRUnichar|.  This is to be expected with
    //  templates, so we disable the warning.
  #pragma warning( disable: 4284 )
#endif


  //
  // nsAReadable[C]String
  //

template <class CharT>
class basic_nsAReadableString
    : public nsPrivateSharableString<CharT>
  {
    public:
//      typedef CharT                     char_type;
//      typedef PRUint32                  size_type;
//      typedef PRUint32                  index_type;

      typedef nsReadingIterator<CharT>  const_iterator;


      // basic_nsAReadableString();                                         // auto-generated default constructor OK (we're abstract anyway)
      // basic_nsAReadableString( const basic_nsAReadableString<CharT>& );  // auto-generated copy-constructor OK (again, only because we're abstract)
    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const basic_nsAReadableString<CharT>& );              // but assignment is _not_ OK (we're immutable) so make it impossible

    public:
      virtual ~basic_nsAReadableString() { }
        // ...yes, I expect to be sub-classed.

      nsReadingIterator<CharT>& BeginReading( nsReadingIterator<CharT>& ) const;
      nsReadingIterator<CharT>& EndReading( nsReadingIterator<CharT>& ) const;

      virtual PRUint32  Length() const = 0;
      PRBool  IsEmpty() const;

        /**
         * |CharAt|, |operator[]|, |First()|, and |Last()| are not guaranteed to be constant-time operations.
         * These signatures should be pushed down into interfaces that guarantee flat allocation.
         * Clients at _this_ level should always use iterators.
         */
      CharT  CharAt( PRUint32 ) const;
      CharT  operator[]( PRUint32 ) const;
      CharT  First() const;
      CharT  Last() const;

      PRUint32  CountChar( CharT ) const;


        /*
          |Left|, |Mid|, and |Right| are annoying signatures that seem better almost
          any _other_ way than they are now.  Consider these alternatives

            aWritable = aReadable.Left(17);   // ...a member function that returns a |Substring|
            aWritable = Left(aReadable, 17);  // ...a global function that returns a |Substring|
            Left(aReadable, 17, aWritable);   // ...a global function that does the assignment

          as opposed to the current signature

            aReadable.Left(aWritable, 17);    // ...a member function that does the assignment

          or maybe just stamping them out in favor of |Substring|, they are just duplicate functionality

            aWritable = Substring(aReadable, 0, 17);
        */
            
      PRUint32  Left( basic_nsAWritableString<CharT>&, PRUint32 ) const;
      PRUint32  Mid( basic_nsAWritableString<CharT>&, PRUint32, PRUint32 ) const;
      PRUint32  Right( basic_nsAWritableString<CharT>&, PRUint32 ) const;

      // Find( ... ) const;
      PRInt32 FindChar( CharT, PRUint32 aOffset = 0 ) const;
      // FindCharInSet( ... ) const;
      // RFind( ... ) const;
      // RFindChar( ... ) const;
      // RFindCharInSet( ... ) const;


      int  Compare( const basic_nsAReadableString<CharT>& rhs ) const;
      int  Compare( const CharT* ) const;
//    int  Compare( const CharT*, PRUint32 ) const;
//    int  Compare( CharT ) const;

        // |Equals()| is a synonym for |Compare()|
      PRBool  Equals( const basic_nsAReadableString<CharT>& rhs ) const;
      PRBool  Equals( const CharT* ) const;
//    PRBool  Equals( const CharT*, PRUint32 ) const;
//    PRBool  Equals( CharT ) const;

        // Comparison operators are all synonyms for |Compare()|
      PRBool  operator!=( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)!=0; }
      PRBool  operator< ( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)< 0; }
      PRBool  operator<=( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)<=0; }
      PRBool  operator==( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)==0; }
      PRBool  operator>=( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)>=0; }
      PRBool  operator> ( const basic_nsAReadableString<CharT>& rhs ) const { return Compare(rhs)> 0; }


        /*
          Shouldn't be implemented because they're i18n sensitive.
          Let's leave them in |nsString| for now.
        */

      // ToLowerCase
      // ToUpperCase
      // EqualsIgnoreCase
      // IsASCII
      // IsSpace
      // IsAlpha
      // IsDigit
      // ToFloat
      // ToInteger

      // char* ToNewCString() const;
      // char* ToNewUTF8String() const;
      // PRUnichar* ToNewUnicode() const;
      // char* ToCString( char*, PRUint32, PRUint32 ) const;


        /*
          Shouldn't be implemented because it's wrong duplication.
          Let's leave it in |nsString| for now.
        */

      // nsString* ToNewString() const;
        // NO!  The right way to say this is |new nsString( fromAReadableString )|


        /*
          Shouldn't be implemented because they're not generally applicable.
          Let's leave them in |nsString| for now.
        */

      // IsOrdered
      // BinarySearch

    // protected:
      virtual const void* Implementation() const;
      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 = 0 ) const = 0;
      virtual PRBool Promises( const basic_nsAReadableString<CharT>& aString ) const { return &aString == this; }
//    virtual PRBool PromisesExactly( const basic_nsAReadableString<CharT>& aString ) const { return false; }

    private:
        // NOT TO BE IMPLEMENTED
      typedef typename nsCharTraits<CharT>::incompatible_char_type incompatible_char_type;
      PRUint32  CountChar( incompatible_char_type ) const;
//    in        Compare( incompatible_char_type ) const;
//    PRBool    Equals( incompatible_char_type ) const;
  };

  /*
    The following macro defines a cast that helps us solve type-unification error problems on compilers
    with poor template support.  String clients probably _never_ need to use it.  String implementors
    sometimes will.
  */

#ifdef NEED_CPP_TEMPLATE_CAST_TO_BASE
#define NS_READABLE_CAST(CharT, expr)  (NS_STATIC_CAST(const basic_nsAReadableString<CharT>&, (expr)))
#else
#define NS_READABLE_CAST(CharT, expr)  (expr)
#endif

  /**
   * Note: measure -- should the |BeginReading| and |EndReading| be |inline|?
   */
template <class CharT>
inline
nsReadingIterator<CharT>&
basic_nsAReadableString<CharT>::BeginReading( nsReadingIterator<CharT>& aResult ) const
  {
    aResult.mOwningString = this;
    GetReadableFragment(aResult.mFragment, kFirstFragment);
    aResult.mPosition = aResult.mFragment.mStart;
    aResult.normalize_forward();
    return aResult;
  }

template <class CharT>
inline
nsReadingIterator<CharT>&
basic_nsAReadableString<CharT>::EndReading( nsReadingIterator<CharT>& aResult ) const
  {
    aResult.mOwningString = this;
    GetReadableFragment(aResult.mFragment, kLastFragment);
    aResult.mPosition = aResult.mFragment.mEnd;
    // must not |normalize_backward| as that would likely invalidate tests like |while ( first != last )|
    return aResult;
  }

template <class CharT>
inline
PRBool
basic_nsAReadableString<CharT>::IsEmpty() const
  {
    return Length() == 0;
  }

template <class CharT>
inline
PRBool
basic_nsAReadableString<CharT>::Equals( const basic_nsAReadableString<CharT>& rhs ) const
  {
    return Length() == rhs.Length() && Compare(rhs) == 0;
  }

template <class CharT>
inline
PRBool
operator==( const nsReadingIterator<CharT>& lhs, const nsReadingIterator<CharT>& rhs )
  {
    return lhs.get() == rhs.get();
  }

template <class CharT>
inline
PRBool
operator!=( const nsReadingIterator<CharT>& lhs, const nsReadingIterator<CharT>& rhs )
  {
    return lhs.get() != rhs.get();
  }


#define NS_DEF_1_STRING_PTR_COMPARISON_OPERATOR(comp, _StringT, _CharT)   \
  inline                                                                  \
  PRBool                                                                  \
  operator comp( const _StringT& lhs, const _CharT* rhs )                 \
    {                                                                     \
      return PRBool(Compare(NS_READABLE_CAST(_CharT, lhs), rhs) comp 0);  \
    }

#define NS_DEF_1_PTR_STRING_COMPARISON_OPERATOR(comp, _StringT, _CharT)   \
  inline                                                                  \
  PRBool                                                                  \
  operator comp( const _CharT* lhs, const _StringT& rhs )                 \
    {                                                                     \
      return PRBool(Compare(lhs, NS_READABLE_CAST(_CharT, rhs)) comp 0);  \
    }

#define NS_DEF_1_STRING_STRING_COMPARISON_OPERATOR(comp, _StringT, _CharT)  \
  inline                                                                    \
  PRBool                                                                    \
  operator comp( const _StringT& lhs, const _StringT& rhs )                 \
    {                                                                       \
      return PRBool(Compare(NS_READABLE_CAST(_CharT, lhs), NS_READABLE_CAST(_CharT, rhs)) comp 0); \
    }

#define NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(comp, _StringT, _CharT) \
  template <class _CharT> NS_DEF_1_STRING_PTR_COMPARISON_OPERATOR(comp, _StringT, _CharT) \
  template <class _CharT> NS_DEF_1_PTR_STRING_COMPARISON_OPERATOR(comp, _StringT, _CharT)

#define NS_DEF_3_STRING_COMPARISON_OPERATORS(comp, _StringT, _CharT)  \
  NS_DEF_1_STRING_STRING_COMPARISON_OPERATOR(comp, _StringT, _CharT)  \
  NS_DEF_1_STRING_PTR_COMPARISON_OPERATOR(comp, _StringT, _CharT)     \
  NS_DEF_1_PTR_STRING_COMPARISON_OPERATOR(comp, _StringT, _CharT)

#define NS_DEF_TEMPLATE_STRING_COMPARISON_OPERATORS(_StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(!=, _StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(< , _StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(<=, _StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(==, _StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(>=, _StringT, _CharT) \
  NS_DEF_2_TEMPLATE_STRING_COMPARISON_OPERATORS(> , _StringT, _CharT)

#define NS_DEF_STRING_COMPARISON_OPERATORS(_StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(!=, _StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(< , _StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(<=, _StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(==, _StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(>=, _StringT, _CharT) \
  NS_DEF_3_STRING_COMPARISON_OPERATORS(> , _StringT, _CharT)


NS_DEF_TEMPLATE_STRING_COMPARISON_OPERATORS(basic_nsAReadableString<CharT>, CharT)



template <class CharT>
const void*
basic_nsAReadableString<CharT>::Implementation() const
  {
    return 0;
  }



template <class CharT>
CharT
basic_nsAReadableString<CharT>::CharAt( PRUint32 aIndex ) const
  {
    NS_ASSERTION(aIndex<Length(), "|CharAt| out-of-range");

    nsReadingIterator<CharT> iter;
    return *(BeginReading(iter).advance(PRInt32(aIndex)));
  }

template <class CharT>
inline
CharT
basic_nsAReadableString<CharT>::operator[]( PRUint32 aIndex ) const
  {
    return CharAt(aIndex);
  }

template <class CharT>
CharT
basic_nsAReadableString<CharT>::First() const
  {
    NS_ASSERTION(Length()>0, "|First()| on an empty string");

    nsReadingIterator<CharT> iter;
    return *BeginReading(iter);
  }

template <class CharT>
CharT
basic_nsAReadableString<CharT>::Last() const
  {
    NS_ASSERTION(Length()>0, "|Last()| on an empty string");

    nsReadingIterator<CharT> iter;
    EndReading(iter);

    if ( !IsEmpty() )
      iter.advance(-1);

    return *iter; // Note: this has undefined results if |IsEmpty()|
  }

template <class CharT>
PRUint32
basic_nsAReadableString<CharT>::CountChar( CharT c ) const
  {
#if 0
    nsReadingIterator<CharT> countBegin, countEnd;
    return PRUint32(NS_COUNT(BeginReading(countBegin), EndReading(countEnd), c));
#else
    PRUint32 result = 0;
    PRUint32 lengthToExamine = Length();

    nsReadingIterator<CharT> iter;
    for ( BeginReading(iter); ; )
      {
        PRInt32 lengthToExamineInThisFragment = iter.size_forward();
        const CharT* fromBegin = iter.get();
        result += PRUint32(NS_COUNT(fromBegin, fromBegin+lengthToExamineInThisFragment, c));
        if ( !(lengthToExamine -= lengthToExamineInThisFragment) )
          return result;
        iter.advance(lengthToExamineInThisFragment);
      }
      // never reached; quiets warnings
    return 0;
#endif
  }

#if 0
  // had to move these definitions into "nsAWritableString.h"
template <class CharT>
PRUint32
basic_nsAReadableString<CharT>::Mid( basic_nsAWritableString<CharT>& aResult, PRUint32 aStartPos, PRUint32 aLengthToCopy ) const
  {
      // If we're just assigning our entire self, give |aResult| the opportunity to share
    if ( aStartPos == 0 && aLengthToCopy >= Length() )
      aResult = *this;
    else
      aResult = Substring(*this, aStartPos, aLengthToCopy);

    return aResult.Length();
  }

template <class CharT>
inline
PRUint32
basic_nsAReadableString<CharT>::Left( basic_nsAWritableString<CharT>& aResult, PRUint32 aLengthToCopy ) const
  {
    return Mid(aResult, 0, aLengthToCopy);
  }

template <class CharT>
PRUint32
basic_nsAReadableString<CharT>::Right( basic_nsAWritableString<CharT>& aResult, PRUint32 aLengthToCopy ) const
  {
    PRUint32 myLength = Length();
    aLengthToCopy = NS_MIN(myLength, aLengthToCopy);
    return Mid(aResult, myLength-aLengthToCopy, aLengthToCopy);
  }
#endif

template <class CharT>
PRInt32
basic_nsAReadableString<CharT>::FindChar( CharT aChar, PRUint32 aOffset ) const
  {
    nsReadingIterator<CharT> iter, done_searching;
    BeginReading(iter).advance( PRInt32(aOffset) );
    EndReading(done_searching);

    PRUint32 lengthSearched = 0;
    while ( iter != done_searching )
      {
        PRInt32 fragmentLength = iter.size_forward();
        const CharT* charFoundAt = nsCharTraits<CharT>::find(iter.get(), fragmentLength, aChar);
        if ( charFoundAt )
          return lengthSearched + (charFoundAt-iter.get()) + aOffset;

        lengthSearched += fragmentLength;
        iter.advance(fragmentLength);
      }

    return -1;
  }








#if 0
template <class CharT>
inline
PRBool
basic_nsAReadableString<CharT>::Equals( const CharT* rhs, PRUint32 rhs_length ) const
  {
    return Compare(literal_string(rhs, rhs_length)) == 0;
  }
#endif

#if 0
template <class CharT>
inline
int
basic_nsAReadableString<CharT>::Compare( const CharT* rhs, PRUint32 rhs_length ) const
  {
    return ::Compare(*this, NS_READABLE_CAST(CharT, literal_string(rhs, rhs_length)));
  }
#endif



  //
  // nsPromiseConcatenation
  //

template <class CharT> class nsPromiseReadable : public basic_nsAReadableString<CharT> { };

template <class CharT>
class nsPromiseConcatenation
      : public nsPromiseReadable<CharT>
    /*
      NOT FOR USE BY HUMANS

      Instances of this class only exist as anonymous temporary results from |operator+()|.
      This is the machinery that makes string concatenation efficient.  No allocations or
      character copies are required unless and until a final assignment is made.  It works
      its magic by overriding and forwarding calls to |GetReadableFragment()|.

      Note: |nsPromiseConcatenation| imposes some limits on string concatenation with |operator+()|.
        - no more than 33 strings, e.g., |s1 + s2 + s3 + ... s32 + s33|
        - left to right evaluation is required ... do not use parentheses to override this

      In practice, neither of these is onerous.  Parentheses do not change the semantics of the
      concatenation, only the order in which the result is assembled ... so there's no reason
      for a user to need to control it.  Too many strings summed together can easily be worked
      around with an intermediate assignment.  I wouldn't have the parentheses limitation if I
      assigned the identifier mask starting at the top, the first time anybody called
      |GetReadableFragment()|.
    */
  {
    protected:
      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 ) const;

      enum { kLeftString, kRightString };

      int
      GetCurrentStringFromFragment( const nsReadableFragment<CharT>& aFragment ) const
        {
          return (NS_REINTERPRET_CAST(PRUint32, aFragment.mFragmentIdentifier) & mFragmentIdentifierMask) ? kRightString : kLeftString;
        }

      int
      SetLeftStringInFragment( nsReadableFragment<CharT>& aFragment ) const
        {
          aFragment.mFragmentIdentifier = NS_REINTERPRET_CAST(void*, NS_REINTERPRET_CAST(PRUint32, aFragment.mFragmentIdentifier) & ~mFragmentIdentifierMask);
          return kLeftString;
        }

      int
      SetRightStringInFragment( nsReadableFragment<CharT>& aFragment ) const
        {
          aFragment.mFragmentIdentifier = NS_REINTERPRET_CAST(void*, NS_REINTERPRET_CAST(PRUint32, aFragment.mFragmentIdentifier) | mFragmentIdentifierMask);
          return kRightString;
        }

    public:
      nsPromiseConcatenation( const basic_nsAReadableString<CharT>& aLeftString, const basic_nsAReadableString<CharT>& aRightString, PRUint32 aMask = 1 )
          : mFragmentIdentifierMask(aMask)
        {
          mStrings[kLeftString] = &aLeftString;
          mStrings[kRightString] = &aRightString;
        }

      nsPromiseConcatenation( const nsPromiseConcatenation<CharT>& aLeftString, const basic_nsAReadableString<CharT>& aRightString )
          : mFragmentIdentifierMask(aLeftString.mFragmentIdentifierMask<<1)
        {
          mStrings[kLeftString] = &aLeftString;
          mStrings[kRightString] = &aRightString;
        }

      // nsPromiseConcatenation( const nsPromiseConcatenation<CharT>& ); // auto-generated copy-constructor should be OK
      // ~nsPromiseConcatenation();                                      // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsPromiseConcatenation<CharT>& );            // we're immutable, you can't assign into a concatenation

    public:

      virtual PRUint32 Length() const;
      virtual PRBool Promises( const basic_nsAReadableString<CharT>& ) const;
//    virtual PRBool PromisesExactly( const basic_nsAReadableString<CharT>& ) const;

//    nsPromiseConcatenation<CharT> operator+( const basic_nsAReadableString<CharT>& rhs ) const;

      PRUint32 GetFragmentIdentifierMask() const { return mFragmentIdentifierMask; }

    private:
      void operator+( const nsPromiseConcatenation<CharT>& ); // NOT TO BE IMPLEMENTED
        // making this |private| stops you from over parenthesizing concatenation expressions, e.g., |(A+B) + (C+D)|
        //  which would break the algorithm for distributing bits in the fragment identifier

    private:
      const basic_nsAReadableString<CharT>* mStrings[2];
      PRUint32 mFragmentIdentifierMask;
  };

// NS_DEF_TEMPLATE_STRING_COMPARISON_OPERATORS(nsPromiseConcatenation<CharT>, CharT)

template <class CharT>
PRUint32
nsPromiseConcatenation<CharT>::Length() const
  {
    return mStrings[kLeftString]->Length() + mStrings[kRightString]->Length();
  }

template <class CharT>
PRBool
nsPromiseConcatenation<CharT>::Promises( const basic_nsAReadableString<CharT>& aString ) const
  {
    return mStrings[0]->Promises(aString) || mStrings[1]->Promises(aString);
  }

#if 0
PRBool
nsPromiseConcatenation<CharT>::PromisesExactly( const basic_nsAReadableString<CharT>& aString ) const
  {
      // Not really like this, test for the empty string, etc
    return mStrings[0] == &aString && !mStrings[1] || !mStrings[0] && mStrings[1] == &aString;
  }
#endif

template <class CharT>
const CharT*
nsPromiseConcatenation<CharT>::GetReadableFragment( nsReadableFragment<CharT>& aFragment, nsFragmentRequest aRequest, PRUint32 aPosition ) const
  {
    int whichString;

      // based on the request, pick which string we will forward the |GetReadableFragment()| call into

    switch ( aRequest )
      {
        case kPrevFragment:
        case kNextFragment:
          whichString = GetCurrentStringFromFragment(aFragment);
          break;

        case kFirstFragment:
          whichString = SetLeftStringInFragment(aFragment);
          break;

        case kLastFragment:
          whichString = SetRightStringInFragment(aFragment);
          break;

        case kFragmentAt:
          PRUint32 leftLength = mStrings[kLeftString]->Length();
          if ( aPosition < leftLength )
            whichString = SetLeftStringInFragment(aFragment);
          else
            {
              whichString = SetRightStringInFragment(aFragment);
              aPosition -= leftLength;
            }
          break;
            
      }

    const CharT* result;
    PRBool done;
    do
      {
        done = PR_TRUE;
        result = mStrings[whichString]->GetReadableFragment(aFragment, aRequest, aPosition);

        if ( !result )
          {
            done = PR_FALSE;
            if ( aRequest == kNextFragment && whichString == kLeftString )
              {
                aRequest = kFirstFragment;
                whichString = SetRightStringInFragment(aFragment);
              }
            else if ( aRequest == kPrevFragment && whichString == kRightString )
              {
                aRequest = kLastFragment;
                whichString = SetLeftStringInFragment(aFragment);
              }
            else
              done = PR_TRUE;
          }
      }
    while ( !done );
    return result;
  }

#if 0
template <class CharT>
inline
nsPromiseConcatenation<CharT>
nsPromiseConcatenation<CharT>::operator+( const basic_nsAReadableString<CharT>& rhs ) const
  {
    return nsPromiseConcatenation<CharT>(*this, rhs, mFragmentIdentifierMask<<1);
  }
#endif





  //
  // nsPromiseSubstring
  //

template <class CharT>
class nsPromiseSubstring
      : public nsPromiseReadable<CharT>
    /*
      NOT FOR USE BY HUMANS (mostly)

      ...not unlike |nsPromiseConcatenation|.  Instances of this class exist only as anonymous
      temporary results from |Substring()|.  Like |nsPromiseConcatenation|, this class only
      holds a pointer, no string data of its own.  It does its magic by overriding and forwarding
      calls to |GetReadableFragment()|.
    */
  {
    protected:
      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 ) const;

    public:
      nsPromiseSubstring( const basic_nsAReadableString<CharT>& aString, PRUint32 aStartPos, PRUint32 aLength )
          : mString(aString),
            mStartPos( NS_MIN(aStartPos, aString.Length()) ),
            mLength( NS_MIN(aLength, aString.Length()-mStartPos) )
        {
          // nothing else to do here
        }

      nsPromiseSubstring( const nsReadingIterator<CharT>& aStart, const nsReadingIterator<CharT>& aEnd )
          : mString(aStart.string())
        {
          nsReadingIterator<CharT> zeroPoint;
          mString.BeginReading(zeroPoint);
          mStartPos = Distance(zeroPoint, aStart);
          mLength = Distance(aStart, aEnd);
        }

      // nsPromiseSubstring( const nsPromiseSubstring<CharT>& ); // auto-generated copy-constructor should be OK
      // ~nsPromiseSubstring();                                  // auto-generated destructor OK

    private:
        // NOT TO BE IMPLEMENTED
      void operator=( const nsPromiseSubstring<CharT>& );        // we're immutable, you can't assign into a substring

    public:
      virtual PRUint32 Length() const;
      virtual PRBool Promises( const basic_nsAReadableString<CharT>& aString ) const { return mString.Promises(aString); }

    private:
      const basic_nsAReadableString<CharT>& mString;
      PRUint32 mStartPos;
      PRUint32 mLength;
  };

// NS_DEF_TEMPLATE_STRING_COMPARISON_OPERATORS(nsPromiseSubstring<CharT>, CharT)

template <class CharT>
PRUint32
nsPromiseSubstring<CharT>::Length() const
  {
    return mLength;
  }

template <class CharT>
const CharT*
nsPromiseSubstring<CharT>::GetReadableFragment( nsReadableFragment<CharT>& aFragment, nsFragmentRequest aRequest, PRUint32 aPosition ) const
  {
      // Offset any request for a specific position (First, Last, At) by our
      //  substrings startpos within the owning string

    if ( aRequest == kFirstFragment )
      {
        aPosition = mStartPos;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kLastFragment )
      {
        aPosition = mStartPos + mLength;
        aRequest = kFragmentAt;
      }
    else if ( aRequest == kFragmentAt )
      aPosition += mStartPos;

    // requests for |kNextFragment| or |kPrevFragment| are just relayed down into the string we're slicing

    const CharT* position_ptr = mString.GetReadableFragment(aFragment, aRequest, aPosition);

    // If |GetReadableFragment| returns |0|, then we are off the string, the contents of the
    //  fragment are garbage.

      // Therefore, only need to fix up the fragment boundaries when |position_ptr| is not null
    if ( position_ptr )
      {
          // if there's more physical data in the returned fragment than I logically have left...
        size_t logical_size_backward = aPosition - mStartPos;
        if ( size_t(position_ptr - aFragment.mStart) > logical_size_backward )
          aFragment.mStart = position_ptr - logical_size_backward;

        size_t logical_size_forward = mLength - logical_size_backward;
        if ( size_t(aFragment.mEnd - position_ptr) > logical_size_forward )
          aFragment.mEnd = position_ptr + logical_size_forward;
      }

    return position_ptr;
  }




#ifdef NEED_CPP_DERIVED_TEMPLATE_OPERATORS

  #define NS_DEF_TEMPLATE_DERIVED_STRING_STRING_OPERATOR_PLUS(_String1T, _String2T) \
  template <class CharT>                                                 \
  inline                                                                 \
  nsPromiseConcatenation<CharT>                                          \
  operator+( const _String1T<CharT>& lhs, const _String2T<CharT>& rhs )  \
    {                                                                    \
      return nsPromiseConcatenation<CharT>(lhs, rhs);                    \
    }

  NS_DEF_TEMPLATE_DERIVED_STRING_STRING_OPERATOR_PLUS(nsPromiseSubstring, nsPromiseSubstring)
  NS_DEF_TEMPLATE_DERIVED_STRING_STRING_OPERATOR_PLUS(nsPromiseConcatenation, nsPromiseSubstring)

#endif // NEED_CPP_DERIVED_TEMPLATE_OPERATORS


  //
  // Global functions
  //

template <class CharT>
inline
PRBool
SameImplementation( const basic_nsAReadableString<CharT>& lhs, const basic_nsAReadableString<CharT>& rhs )
  {
    const void* imp_tag = lhs.Implementation();
    return imp_tag && (imp_tag==rhs.Implementation());
  }

inline
nsPromiseSubstring<char>
Substring( const basic_nsAReadableString<char>& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsPromiseSubstring<char>(aString, aStartPos, aSubstringLength);
  }

inline
nsPromiseSubstring<PRUnichar>
Substring( const basic_nsAReadableString<PRUnichar>& aString, PRUint32 aStartPos, PRUint32 aSubstringLength )
  {
    return nsPromiseSubstring<PRUnichar>(aString, aStartPos, aSubstringLength);
  }

inline
nsPromiseSubstring<char>
Substring( const nsReadingIterator<char>& aStart, const nsReadingIterator<char>& aEnd )
  {
    return nsPromiseSubstring<char>(aStart, aEnd);
  }

inline
nsPromiseSubstring<PRUnichar>
Substring( const nsReadingIterator<PRUnichar>& aStart, const nsReadingIterator<PRUnichar>& aEnd )
  {
    return nsPromiseSubstring<PRUnichar>(aStart, aEnd);
  }

template <class CharT>
int
Compare( const basic_nsAReadableString<CharT>& lhs, const basic_nsAReadableString<CharT>& rhs )
  {
    if ( &lhs == &rhs )
      return 0;

    PRUint32 lLength = lhs.Length();
    PRUint32 rLength = rhs.Length();
    PRUint32 lengthToCompare = NS_MIN(lLength, rLength);

    nsReadingIterator<CharT> leftIter, rightIter;
    lhs.BeginReading(leftIter);
    rhs.BeginReading(rightIter);

    int result;

    for (;;)
      {
        PRUint32 lengthAvailable = PRUint32( NS_MIN(leftIter.size_forward(), rightIter.size_forward()) );

        if ( lengthAvailable > lengthToCompare )
          lengthAvailable = lengthToCompare;

          // Note: |result| should be declared in this |if| expression, but some compilers don't like that
        if ( (result = nsCharTraits<CharT>::compare(leftIter.get(), rightIter.get(), lengthAvailable)) != 0 )
          return result;

        if ( !(lengthToCompare -= lengthAvailable) )
          break;

        leftIter.advance( PRInt32(lengthAvailable) );
        rightIter.advance( PRInt32(lengthAvailable) );
      }

    if ( lLength < rLength )
      return -1;
    else if ( rLength < lLength )
      return 1;
    else
      return 0;
  }

template <class CharT>
inline
int
Compare( const basic_nsAReadableString<CharT>& lhs, const CharT* rhs )
  {
    return Compare(lhs, NS_READABLE_CAST(CharT, literal_string(rhs)));
  }

template <class CharT>
inline
int
Compare( const CharT* lhs, const basic_nsAReadableString<CharT>& rhs )
  {
    return Compare(NS_READABLE_CAST(CharT, literal_string(lhs)), rhs);
  }

// XXX Note that these are located here because some compilers are
// sensitive to the ordering of declarations with regard to templates.
template <class CharT>
inline
int
basic_nsAReadableString<CharT>::Compare( const basic_nsAReadableString<CharT>& rhs ) const
  {
    return ::Compare(*this, rhs);
  }

template <class CharT>
inline
PRBool
basic_nsAReadableString<CharT>::Equals( const CharT* rhs ) const
  {
    return Compare(literal_string(rhs)) == 0;
  }

template <class CharT>
inline
int
basic_nsAReadableString<CharT>::Compare( const CharT* rhs ) const
  {
    return ::Compare(*this, NS_READABLE_CAST(CharT, literal_string(rhs)));
  }



  /*
    How shall we provide |operator+()|?

    What would it return?  It has to return a stack based object, because the client will
    not be given an opportunity to handle memory management in an expression like

      myWritableString = stringA + stringB + stringC;

    ...so the `obvious' answer of returning a new |nsSharedString| is no good.  We could
    return an |nsString|, if that name were in scope here, though there's no telling what the client
    will really want to do with the result.  What might be better, though,
    is to return a `promise' to concatenate some strings...

    By making |nsPromiseConcatenation| inherit from readable strings, we automatically handle
    assignment and other interesting uses within writable strings, plus we drastically reduce
    the number of cases we have to write |operator+()| for.  The cost is extra temporary concat strings
    in the evaluation of strings of '+'s, e.g., |A + B + C + D|, and that we have to do some work
    to implement the virtual functions of readables.
  */

template <class CharT>
inline
nsPromiseConcatenation<CharT>
operator+( const nsPromiseConcatenation<CharT>& lhs, const basic_nsAReadableString<CharT>& rhs )
  {
    return nsPromiseConcatenation<CharT>(lhs, rhs, lhs.GetFragmentIdentifierMask()<<1);
  }

template <class CharT>
inline
nsPromiseConcatenation<CharT>
operator+( const basic_nsAReadableString<CharT>& lhs, const basic_nsAReadableString<CharT>& rhs )
  {
    return nsPromiseConcatenation<CharT>(lhs, rhs);
  }



#ifdef NEED_CPP_DERIVED_TEMPLATE_OPERATORS
  #define NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(_String1T, _String2T, _CharT) \
    inline                                                  \
    nsPromiseConcatenation<_CharT>                          \
    operator+( const _String1T& lhs, const _String2T& rhs ) \
      {                                                     \
        return nsPromiseConcatenation<_CharT>(lhs, rhs);    \
      }

  #define NS_DEF_DERIVED_STRING_OPERATOR_PLUS(_StringT, _CharT) \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(_StringT, _StringT, _CharT) \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(nsPromiseSubstring<_CharT>, _StringT, _CharT) \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(_StringT, nsPromiseSubstring<_CharT>, _CharT) \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(nsPromiseConcatenation<_CharT>, _StringT, _CharT)

  #define NS_DEF_2_STRING_STRING_OPERATOR_PLUS(_String1T, _String2T, _CharT)  \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(_String1T, _String2T, _CharT)  \
    NS_DEF_DERIVED_STRING_STRING_OPERATOR_PLUS(_String2T, _String1T, _CharT)

#else
  #define NS_DEF_DERIVED_STRING_OPERATOR_PLUS(_StringT, _CharT)
  #define NS_DEF_2_STRING_STRING_OPERATOR_PLUS(_String1T, _String2T, _CharT)
#endif


#define kDefaultFlatStringSize 64

template <class CharT>
class basic_nsPromiseFlatString
    : public basic_nsAReadableString<CharT>
  {
    public:
      explicit basic_nsPromiseFlatString( const basic_nsAReadableString<CharT>& );

      virtual
     ~basic_nsPromiseFlatString( )
        {
          if (mOwnsBuffer)
            nsMemory::Free((void*)mBuffer);
        }

      virtual PRUint32  Length() const { return mLength; }
      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 = 0 ) const;

      const CharT* get() const        { return mBuffer; }
      operator const CharT*() const   { return get(); } // to be deprecated, prefer |get()|

    protected:
      PRUint32      mLength;
      const CharT*  mBuffer;
      PRBool        mOwnsBuffer;
      CharT         mInlineBuffer[kDefaultFlatStringSize];
  };

template <class CharT>
basic_nsPromiseFlatString<CharT>::basic_nsPromiseFlatString( const basic_nsAReadableString<CharT>& aString )
    : mLength(aString.Length()),
      mOwnsBuffer(PR_FALSE)
  {
    typedef nsReadingIterator<CharT> iterator;

    iterator start;
    iterator end;
    
    aString.BeginReading(start);
    aString.EndReading(end);

    // First count the number of buffers
    PRInt32 buffer_count = 0;
    while ( start != end )
      {
        buffer_count++;
        start.advance(start.size_forward());
      }

    // Now figure out what we want to do with the string
    aString.BeginReading(start);
    // XXX Not guaranteed null-termination in the first case
    // If it's a single buffer, we just use the implementation's buffer
    if ( buffer_count == 1 ) 
      mBuffer = start.get();
    // If it's too big for our inline buffer, we allocate a new one
    else if ( mLength > kDefaultFlatStringSize-1 )
      {
        CharT* result = NS_STATIC_CAST(CharT*, nsMemory::Alloc((mLength+1) * sizeof(CharT)));
    		CharT* toBegin = result;
        *copy_string(start, end, toBegin) = CharT(0);

        mBuffer = result;
        mOwnsBuffer = PR_TRUE;
      }
    // Otherwise copy into our internal buffer
    else
      {
        mBuffer = mInlineBuffer;
        CharT* toBegin = &mInlineBuffer[0];
        copy_string( start, end, toBegin);
        mInlineBuffer[mLength] = 0;
      }
  }


template <class CharT>
const CharT* 
basic_nsPromiseFlatString<CharT>::GetReadableFragment( nsReadableFragment<CharT>& aFragment, 
						 nsFragmentRequest aRequest, 
						 PRUint32 aOffset ) const 
  {
    switch ( aRequest )
      {
        case kFirstFragment:
        case kLastFragment:
        case kFragmentAt:
          aFragment.mEnd = (aFragment.mStart = mBuffer) + mLength;
          return aFragment.mStart + aOffset;
        
        case kPrevFragment:
        case kNextFragment:
        default:
          return 0;
      }
  }


typedef basic_nsAReadableString<PRUnichar>    nsAReadableString;
typedef basic_nsAReadableString<char>         nsAReadableCString;

typedef basic_nsPromiseFlatString<PRUnichar>  nsPromiseFlatString;
typedef basic_nsPromiseFlatString<char>       nsPromiseFlatCString;


#endif // !defined(nsAReadableString_h___)
