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

/*

  A set of string wrapper classes that ease transition to use of XPIDL
  interfaces.

 */

#ifndef nsXPIDLString_h__
#define nsXPIDLString_h__

#include "nscore.h" // for PRUnichar

////////////////////////////////////////////////////////////////////////
// nsXPIDLString
//
//   A wrapper for Unicode strings. With the |getter_Copies()| and
//   |getter_Shares()| helper functions, this can be used instead of
//   the "naked" |PRUnichar*| interface for |wstring| parameters in
//   XPIDL interfaces.
//

class nsXPIDLString {
private:
    PRUnichar* mBuf;
    PRBool     mBufOwner;

    PRUnichar** StartAssignmentByValue();
    const PRUnichar** StartAssignmentByReference();

public:
    /**
     * Construct a new, uninitialized wrapper for a Unicode string.
     */
    nsXPIDLString();

    virtual ~nsXPIDLString();

    /**
     * Return an immutable reference to the Unicode string.
     */
    operator const PRUnichar*();

    /**
     * Make a copy of the Unicode string. Use this function in the
     * callee to ensure that the correct memory allocator is used.
     */
    static PRUnichar* Copy(const PRUnichar* aString);

    // A helper class for assignment-by-value. This class is an
    // implementation detail and should not be considered part of the
    // public interface.
    class GetterCopies {
    private:
        nsXPIDLString& mXPIDLString;

    public:
        GetterCopies(nsXPIDLString& aXPIDLString)
            : mXPIDLString(aXPIDLString) {}

        operator PRUnichar**() {
            return mXPIDLString.StartAssignmentByValue();
        }

        friend GetterCopies& getter_Copies(nsXPIDLString& aXPIDLString);
    };

    friend class GetterCopies;

    // A helper class for assignment-by-reference. This class is an
    // implementation detail and should not be considered part of the
    // public interface.
    class GetterShares {
    private:
        nsXPIDLString& mXPIDLString;

    public:
        GetterShares(nsXPIDLString& aXPIDLString)
            : mXPIDLString(aXPIDLString) {}

        operator const PRUnichar**() {
            return mXPIDLString.StartAssignmentByReference();
        }

        friend GetterShares& getter_Shares(nsXPIDLString& aXPIDLString);
    };

    friend class GetterShares;

private:
    // not to be implemented
    nsXPIDLString(nsXPIDLString& aXPIDLString) {}
    nsXPIDLString& operator =(nsXPIDLString& aXPIDLString) { return *this; }
};


/**
 * Use this function to "wrap" the nsXPIDLString object that is to
 * receive an |out| value.
 */
inline nsXPIDLString::GetterCopies&
getter_Copies(nsXPIDLString& aXPIDLString)
{
    return nsXPIDLString::GetterCopies(aXPIDLString);
}

/**
 * Use this function to "wrap" the nsXPIDLString object that is to
 * receive a |[shared] out| value.
 */
inline nsXPIDLString::GetterShares&
getter_Shares(nsXPIDLString& aXPIDLString)
{
    return nsXPIDLString::GetterShares(aXPIDLString);
}



////////////////////////////////////////////////////////////////////////
// nsXPIDLCString
//
//   A wrapper for Unicode strings. With the |getter_Copies()| and
//   |getter_Shares()| helper functions, this can be used instead of
//   the "naked" |char*| interface for |wstring| parameters in XPIDL
//   interfaces.
//

class nsXPIDLCString {
private:
    char*  mBuf;
    PRBool mBufOwner;

    char** StartAssignmentByValue();
    const char** StartAssignmentByReference();

public:
    /**
     * Construct a new, uninitialized wrapper for a single-byte string.
     */
    nsXPIDLCString();

    virtual ~nsXPIDLCString();

    /**
     * Return an immutable reference to the single-byte string.
     */
    operator const char*();

    /**
     * Make a copy of the single-byte string. Use this function in the
     * callee to ensure that the correct memory allocator is used.
     */
    static char* Copy(const char* aString);

    // A helper class for assignment-by-value. This class is an
    // implementation detail and should not be considered part of the
    // public interface.
    class GetterCopies {
    private:
        nsXPIDLCString& mXPIDLString;

    public:
        GetterCopies(nsXPIDLCString& aXPIDLString)
            : mXPIDLString(aXPIDLString) {}

        operator char**() {
            return mXPIDLString.StartAssignmentByValue();
        }

        friend GetterCopies& getter_Copies(nsXPIDLCString& aXPIDLString);
    };

    friend class GetterCopies;

    // A helper class for assignment-by-reference. This class is an
    // implementation detail and should not be considered part of the
    // public interface.
    class GetterShares {
    private:
        nsXPIDLCString& mXPIDLString;

    public:
        GetterShares(nsXPIDLCString& aXPIDLString)
            : mXPIDLString(aXPIDLString) {}

        operator const char**() {
            return mXPIDLString.StartAssignmentByReference();
        }

        friend GetterShares& getter_Shares(nsXPIDLCString& aXPIDLString);
    };

    friend class GetterShares;

private:
    // not to be implemented
    nsXPIDLCString(nsXPIDLCString& aXPIDLString) {}
    nsXPIDLCString& operator =(nsXPIDLCString& aXPIDLString) { return *this; }
};

/**
 * Use this function to "wrap" the nsXPIDLCString object that is to
 * receive an |out| value.
 */
inline nsXPIDLCString::GetterCopies&
getter_Copies(nsXPIDLCString& aXPIDLString)
{
    return nsXPIDLCString::GetterCopies(aXPIDLString);
}


/**
 * Use this function to "wrap" the nsXPIDLCString object that is to
 * receive a |[shared] out| value.
 */
inline nsXPIDLCString::GetterShares&
getter_Shares(nsXPIDLCString& aXPIDLString)
{
    return nsXPIDLCString::GetterShares(aXPIDLString);
}



#endif // nsXPIDLString_h__
