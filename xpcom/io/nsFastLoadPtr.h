/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla FastLoad code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Brendan Eich <brendan@mozilla.org> (original author)
 */

#ifndef nsFastLoadPtr_h___
#define nsFastLoadPtr_h___

/**
 * Mozilla FastLoad file object pointer template type.
 *
 * Use nsFastLoadPtr<T> rather than nsCOMPtr<T> when declaring a strong XPCOM
 * ref member of a data structure that's conditionally loaded at application
 * startup.  You must be willing to tolerate the null mRawPtr test on every
 * dereference of this member pointer, or else copy it to a local to optimize
 * away the cost.
 *
 * In an object graph that's serialized and deserialized via nsISerializable,
 * nsIObjectOutputStream, and nsIObjectInputStream, if any object reference is
 * via an nsFastLoadPtr, then *all references* must be via nsFastLoadPtrs (no
 * weak refs allowed).  Otherwise a slow ref will cause a duplicate object to
 * be deserialized.
 *
 * XXXbe fix this, or at least add duplicate detection
 */

#ifndef nsCOMPtr_h___
#include "nsCOMPtr.h"
#endif

#ifndef nsIFastLoadService_h___
#include "nsIFastLoadService.h"
#endif

template <class T>
class nsFastLoadPtr : public nsCOMPtr<T> {
  public:
    nsDerivedSafe<T>* get() const {
        if (!mRawPtr) {
            gFastLoadService_->GetFastLoadReferent(NS_STATIC_CAST(nsISupports**,
                                                                  &mRawPtr));
        }
        return NS_REINTERPRET_CAST(nsDerivedSafe<T>*, mRawPtr);
    }

    nsresult read(nsIObjectInputStream* aInputStream) {
        return gFastLoadService_->ReadFastLoadPtr(aInputStream,
                                                  NS_STATIC_CAST(nsISupports**,
                                                                 &mRawPtr));
    }

    nsresult write(nsIObjectOutputStream* aOutputStream, const nsCID& aCID) {
        return gFastLoadService_->WriteFastLoadPtr(aOutputStream,
                                                   NS_STATIC_CAST(nsISupports*,
                                                                  mRawPtr),
                                                   aCID);
    }
};

/**
 * Template class, so we don't want a static service pointer -- use a global
 * weak ref set by the service singleton's ctor and cleared by its dtor.
 */
PR_EXPORT_DATA(nsIFastLoadService*) gFastLoadService_;

#endif // nsFastLoadPtr_h___
