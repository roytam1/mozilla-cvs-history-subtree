/*
 * (C) Copyright The MITRE Corporation 1999  All rights reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * The program provided "as is" without any warranty express or
 * implied, including the warranty of non-infringement and the implied
 * warranties of merchantibility and fitness for a particular purpose.
 * The Copyright owner will not be liable for any damages suffered by
 * you as a result of using the Program. In no event will the Copyright
 * owner be liable for any special, indirect or consequential damages or
 * lost profits even if the Copyright owner has been advised of the
 * possibility of their occurrence.
 *
 * Contributor(s):
 *
 * Tom Kneeland
 *    -- original author.
 *
 * Keith Visco <kvisco@ziplink.net>
 * Larry Fitzpatrick
 *
 */

#ifndef txString_h__
#define txString_h__

#include "TxObject.h"
#include "baseutils.h"

#ifdef TX_EXE
#include <iostream.h>
typedef unsigned short UNICODE_CHAR;
#define NOT_FOUND -1
#else
#include "nsString.h"
typedef PRUnichar UNICODE_CHAR;
#define NOT_FOUND kNotFound
#endif

class String : public TxObject
{
public:
    String();
    String(const String& aSource);
#ifdef TX_EXE
    String(const UNICODE_CHAR* source);
    String(const UNICODE_CHAR* source, const PRUint32 length = 0);
#else
    explicit String(const nsAString& aSource);
#endif
    ~String();

    // Append aSource to this string
    void append(const UNICODE_CHAR aSource);
    void append(const String& aSource);

    // Insert aSource at aOffset in this string
    void insert(const PRUint32 aOffset, const UNICODE_CHAR aSource);
    void insert(const PRUint32 aOffset, const String& aSource);

    // Replace characters starting at aOffset with aSource
    void replace(const PRUint32 aOffset, const UNICODE_CHAR aSource);
    void replace(const PRUint32 aOffset, const String& aSource);

    // Delete aCount characters starting at aOffset
    void deleteChars(const PRUint32 aOffset, const PRUint32 aCount);

    /*
     * Returns the character at aIndex. If the index is out of
     * bounds, -1 will be returned.
     */
    UNICODE_CHAR charAt(const PRUint32 aIndex) const;

    // Clear the string
    void clear();

    // Make sure the string buffer can hold aCapacity characters
    void ensureCapacity(const PRUint32 aCapacity);

    // Returns index of first occurrence of aData
    PRInt32 indexOf(const UNICODE_CHAR aData, const PRUint32 aOffset = 0) const;
    PRInt32 indexOf(const String& data, const PRUint32 aOffset = 0) const;

    // Returns index of last occurrence of aData
    PRInt32 lastIndexOf(const UNICODE_CHAR aData, const PRUint32 aOffset = 0) const;

    // Check equality between strings
    MBool isEqual(const String& aData) const;
    MBool isEqualIgnoreCase(const String& aData) const;

    // Check whether the string is empty
    MBool isEmpty() const;

    // Return the length of the string
    PRUint32 length() const;

    /**
     * Sets the Length of this String, if length is less than 0, it will
     * be set to 0; if length > current length, the string will be extended
     * and padded with '\0' null characters. Otherwise the String
     * will be truncated
    **/
    void setLength(const PRUint32 aLength);

    /**
     * Returns a substring starting at start
     * Note: the dest String is cleared before use
    **/
    String& subString(const PRUint32 aStart, String& aDest) const;

    /**
     * Returns the subString starting at start and ending at end
     * Note: the dest String is cleared before use
    **/
    String& subString(const PRUint32 aStart, const PRUint32 aEnd, String& aDest) const;

    // Convert string to lowercase
    void toLowerCase();

    // Convert string to uppercase
    void toUpperCase();

    // Trim whitespace from both ends
    void trim();

#ifndef TX_EXE
    nsString& getNSString();
    const nsString& getConstNSString() const;
#endif

private:
#ifndef TX_EXE
    nsString mString;
#else
    UNICODE_CHAR* mBuffer;
    PRUint32 mBufferLength;
    PRUint32 mLength;
#endif

// XXX DEPRECATED
public:
    explicit String(const PRUint32 aSize);
    /* explicit */ String(const char* aSource); // XXX Used for literal strings
    String& operator = (const char* aSource); // XXX Used for literal strings
    void append(const char* aSource);
    PRInt32 indexOf(char aData, const PRUint32 aOffset = 0) const;
    PRInt32 lastIndexOf(const char aData, const PRUint32 aOffset = 0) const;
    MBool isEqual(const char* aData) const;
    char* toCharArray() const;
private:
#ifdef TX_EXE
    UNICODE_CHAR* toUnicode() const;
    //Translate UNICODE_CHARs to Chars and output to the provided stream
    friend ostream& operator<<(ostream& output, const String& source);
#endif
// XXX DEPRECATED
};

#ifdef TX_EXE
ostream& operator<<(ostream& output, const String& source);
#endif

#endif // txString_h__
