/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is Mozilla.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications.  Portions created by Netscape Communications are
 * Copyright (C) 2001 by Netscape Communications.  All
 * Rights Reserved.
 * 
 * Contributor(s): 
 *   Scott Collins <scc@mozilla.org> (original author)
 */

#ifndef nsPromiseConcatenation_h___
#define nsPromiseConcatenation_h___

#ifndef nsStringTraits_h___
#include "nsStringTraits.h"
#endif

#ifndef nsAPromiseString_h___
#include "nsAPromiseString.h"
#endif

//-------1---------2---------3---------4---------5---------6---------7---------8

  /**
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

template <class CharT>
class nsPromiseConcatenation
      : public nsStringTraits<CharT>::abstract_promise_type
  {
    typedef typename nsStringTraits<CharT>::abstract_string_type  string_type;
    typedef string_type::const_iterator                           const_iterator;

    protected:
      virtual const CharT* GetReadableFragment( nsReadableFragment<CharT>&, nsFragmentRequest, PRUint32 ) const;
      virtual       CharT* GetWritableFragment( nsWritableFragment<CharT>&, nsFragmentRequest, PRUint32 ) { }

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
      nsPromiseConcatenation( const string_type& aLeftString, const string_type& aRightString, PRUint32 aMask = 1 )
          : mFragmentIdentifierMask(aMask)
        {
          mStrings[kLeftString] = &aLeftString;
          mStrings[kRightString] = &aRightString;
        }

      nsPromiseConcatenation( const nsPromiseConcatenation<CharT>& aLeftString, const string_type& aRightString )
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
      virtual PRBool Promises( const string_type& ) const;
//    virtual PRBool PromisesExactly( const string_type& ) const;

//    nsPromiseConcatenation<CharT> operator+( const string_type& rhs ) const;

      PRUint32 GetFragmentIdentifierMask() const { return mFragmentIdentifierMask; }

    private:
      void operator+( const nsPromiseConcatenation<CharT>& ); // NOT TO BE IMPLEMENTED
        // making this |private| stops you from over parenthesizing concatenation expressions, e.g., |(A+B) + (C+D)|
        //  which would break the algorithm for distributing bits in the fragment identifier

    private:
      const string_type*  mStrings[2];
      PRUint32            mFragmentIdentifierMask;
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
nsPromiseConcatenation<CharT>::Promises( const string_type& aString ) const
  {
    return mStrings[0]->Promises(aString) || mStrings[1]->Promises(aString);
  }

#if 0
PRBool
nsPromiseConcatenation<CharT>::PromisesExactly( const string_type& aString ) const
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
nsPromiseConcatenation<CharT>::operator+( const string_type& rhs ) const
  {
    return nsPromiseConcatenation<CharT>(*this, rhs, mFragmentIdentifierMask<<1);
  }
#endif



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

inline
nsPromiseConcatenation<PRUnichar>
operator+( const nsPromiseConcatenation<PRUnichar>& lhs, const nsAString& rhs )
  {
    return nsPromiseConcatenation<PRUnichar>(lhs, rhs, lhs.GetFragmentIdentifierMask()<<1);
  }

inline
nsPromiseConcatenation<char>
operator+( const nsPromiseConcatenation<char>& lhs, const nsACString& rhs )
  {
    return nsPromiseConcatenation<char>(lhs, rhs, lhs.GetFragmentIdentifierMask()<<1);
  }

inline
nsPromiseConcatenation<PRUnichar>
operator+( const nsAString& lhs, const nsAString& rhs )
  {
    return nsPromiseConcatenation<PRUnichar>(lhs, rhs);
  }

inline
nsPromiseConcatenation<char>
operator+( const nsACString& lhs, const nsACString& rhs )
  {
    return nsPromiseConcatenation<char>(lhs, rhs);
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


#endif /* !defined(nsPromiseConcatenation_h___) */
