/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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


#ifndef nsString_h__
#define nsString_h__

#include "nsCom.h"
#include "prtypes.h"

#define NS_STRING_NO_INDEX ((PRUint32) -1)

class NS_COM nsString
{
public:
    nsString(const char* cstr = "");

    nsString(const nsString& s);

    virtual ~nsString(void);

    virtual const nsString&
    operator =(const nsString& s);

    virtual const nsString&
    operator =(const char* cstr);

    virtual char&
    operator [](PRUint32 index) const;

    virtual char&
    CharAt(PRUint32 index) const;

    virtual const nsString&
    operator +=(nsString& s);

    virtual const nsString&
    operator +=(const char* cstr);

    virtual const nsString&
    operator +=(const char c);

    // XXX Not allowed with Unicode...but, until we get unicode, I
    // need to ease my pain...
    virtual operator char*(void) const;

    virtual PRUint32
    Length(void) const;

    virtual PRBool
    Equals(nsString& s) const;

    virtual PRBool
    Equals(const char* cstr) const;

    virtual PRBool
    EqualsIgnoreCase(nsString& s) const;

    virtual PRBool
    EqualsIgnoreCase(const char* cstr) const;

    virtual PRUint32
    Find(char c, PRUint32 offset = 0) const;

    virtual PRUint32
    RFind(char c) const;

    virtual const nsString&
    Cut(PRUint32 start, PRUint32 count);

    virtual char*
    ToCString(char* buf, PRUint32 len) const;

    virtual char*
    ToNewCString() const;

    virtual PRUint32
    Split(nsString* result[], char splitter) const;

protected:
    char* fBuffer;
    int   fSize;
};

#endif // nsString_h__
