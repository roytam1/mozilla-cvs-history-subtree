/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef nsCOMCString_h__
#define nsCOMCString_h__

#include "prtypes.h"
class nsIAllocator;

class nsCOMCString
{
protected:
    static PRInt32       gRefCnt;
    static nsIAllocator* gAllocator;
    static void EnsureAllocator();
    static void ReleaseAllocator();

    char* mBuf;
    void CopyIntoSelf(const char* aCString);

public:
    nsCOMCString();
    nsCOMCString(const char* aCString);
    nsCOMCString(const nsCOMCString& aCOMCString);
    ~nsCOMCString();

    nsCOMCString& operator =(const char* aCString);
    nsCOMCString& operator =(const nsCOMCString& aCOMCString);

    operator const char*();

    char* ToNewCString();
    const char* get();

    // XXX Should be private; but can't be due to compiler problems. See
    // nsCOMPtr for details.
    char** StartAssignment();
};


class nsGetterCopies {
private:
    nsCOMCString& mTargetCOMCString;

public:
    nsGetterCopies(nsCOMCString& aCOMCString) : mTargetCOMCString(aCOMCString) {};

    operator char**() {
        return mTargetCOMCString.StartAssignment();
    }
};

inline nsGetterCopies
getter_Copies(nsCOMCString& aCOMCString)
{
    return nsGetterCopies(aCOMCString);
}

#endif // nsCOMCString_h__
