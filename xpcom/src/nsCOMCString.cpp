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

#include "nsCOMCString.h"
#include "nsIAllocator.h"
#include "nsIServiceManager.h"
#include "plstr.h"

static NS_DEFINE_CID(kAllocatorCID, NS_ALLOCATOR_CID);

PRInt32 nsCOMCString::gRefCnt;
nsIAllocator* nsCOMCString::gAllocator;

void
nsCOMCString::EnsureAllocator()
{
    if (gRefCnt++ == 0) {
        nsServiceManager::GetService(kAllocatorCID,
                                     nsIAllocator::GetIID(),
                                     (nsISupports**) &gAllocator);
    }
}


void
nsCOMCString::ReleaseAllocator()
{
    if (--gRefCnt == 0) {
        nsServiceManager::ReleaseService(kAllocatorCID, gAllocator);
        gAllocator = 0;
    }
}

void
nsCOMCString::CopyIntoSelf(const char* aCString)
{
    if (mBuf)
        gAllocator->Free(mBuf);

    if (aCString) {
        mBuf = (char*) gAllocator->Alloc(PL_strlen(aCString) + 1);
        if (mBuf)
            PL_strcpy(mBuf, aCString);
    }
    else {
        mBuf = 0;
    }
}

nsCOMCString::nsCOMCString()
    : mBuf(0)
{
    EnsureAllocator();
}


nsCOMCString::nsCOMCString(const char* aCString)
    : mBuf(0)
{
    EnsureAllocator();
    CopyIntoSelf(aCString);
}

nsCOMCString::nsCOMCString(const nsCOMCString& aCOMCString)
    : mBuf(0)
{
    EnsureAllocator();
    CopyIntoSelf(aCOMCString.mBuf);
}


nsCOMCString::~nsCOMCString()
{
    if (mBuf)
        gAllocator->Free(mBuf);

    ReleaseAllocator();
}

nsCOMCString& 
nsCOMCString::operator =(const char* aCString)
{
    CopyIntoSelf(aCString);
    return *this;
}

nsCOMCString&
nsCOMCString::operator =(const nsCOMCString& aCOMCString)
{
    CopyIntoSelf(aCOMCString.mBuf);
    return *this;
}

nsCOMCString::operator const char*()
{
    return mBuf;
}

char**
nsCOMCString::StartAssignment()
{
    if (mBuf)
        gAllocator->Free(mBuf);

    return &mBuf;
}


char*
nsCOMCString::ToNewCString()
{
    char* result = 0;
    if (mBuf) {
        result = (char*) gAllocator->Alloc(PL_strlen(mBuf) + 1);
        if (result)
            PL_strcpy(result, mBuf);
    }
    return result;
}

const char*
nsCOMCString::get()
{
    return mBuf;
}
