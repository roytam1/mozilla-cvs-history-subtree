/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#ifndef nsIEnumerator_h___
#define nsIEnumerator_h___

#include "nsISupports.h"

#define NS_IENUMERATOR_IID                           \
{ /* ad385286-cbc4-11d2-8cca-0060b0fc14a3 */         \
    0xad385286,                                      \
    0xcbc4,                                          \
    0x11d2,                                          \
    {0x8c, 0xca, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}


class nsIEnumerator : public nsISupports {
public:
  static const nsIID& GetIID(void) { static nsIID iid = NS_IENUMERATOR_IID; return iid; }
  NS_IMETHOD HasMoreElements(PRBool* aResult) = 0;
  NS_IMETHOD GetNext(nsISupports** aResult) = 0;
};

#define NS_IBIDIRECTIONALENUMERATOR_IID              \
{ /* 75f158a0-cadd-11d2-8cca-0060b0fc14a3 */         \
    0x75f158a0,                                      \
    0xcadd,                                          \
    0x11d2,                                          \
    {0x8c, 0xca, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsIBidirectionalEnumerator : public nsIEnumerator {
public:

  static const nsIID& GetIID(void) { static nsIID iid = NS_IBIDIRECTIONALENUMERATOR_IID; return iid; }

  NS_IMETHOD HasPreviousElements(PRBool* aResult) = 0;
  NS_IMETHOD GetPrev(nsISupports** aResult) = 0;
};

// Construct and return an implementation of a "conjoining enumerator." This
// enumerator lets you string together two other enumerators into one sequence.
// The result is an nsIBidirectionalEnumerator, but if either input is not
// also bidirectional, the Last and Prev operations will fail.
extern "C" NS_COM nsresult
NS_NewConjoiningEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                           nsIBidirectionalEnumerator* *aInstancePtrResult);

// Construct and return an implementation of a "union enumerator." This
// enumerator will only return elements that are found in both constituent
// enumerators.
extern "C" NS_COM nsresult
NS_NewUnionEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                      nsIEnumerator* *aInstancePtrResult);

// Construct and return an implementation of an "intersection enumerator." This
// enumerator will return elements that are found in either constituent
// enumerators, eliminating duplicates.
extern "C" NS_COM nsresult
NS_NewIntersectionEnumerator(nsIEnumerator* first, nsIEnumerator* second,
                             nsIEnumerator* *aInstancePtrResult);

// Construct and return an implementation of an empty enuemrator. This enumerator
// will have no elements.
extern "C" NS_COM nsresult
NS_NewEmptyEnumerator(nsIEnumerator** aResult);

#endif // __nsIEnumerator_h

