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
 * Please see release.txt distributed with this file for more information.
 *
 */

#include "TxString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"

String::String()
{
}

String::String(const String& aSource) : mString(aSource.mString)
{
}

String::String(const nsAString& aSource) : mString(aSource)
{
}

String::~String()
{
}

void String::append(const UNICODE_CHAR aSource)
{
  mString.Append(aSource);
}

void String::append(const String& aSource)
{
  mString.Append(aSource.mString);
}

void String::insert(const PRUint32 aOffset, const UNICODE_CHAR aSource)
{
  mString.Insert(aSource, aOffset);
}

void String::insert(const PRUint32 aOffset, const String& aSource)
{
  mString.Insert(aSource.mString, aOffset);
}

void String::replace(const PRUint32 aOffset, const UNICODE_CHAR aSource)
{
  mString.SetCharAt(aSource, aOffset);
}

void String::replace(const PRUint32 aOffset, const String& aSource)
{
  mString.Replace(aOffset, mString.Length() - aOffset, aSource.mString);
}

void String::deleteChars(const PRUint32 aOffset, const PRUint32 aCount)
{
  mString.Cut(aOffset, aCount);
}

UNICODE_CHAR String::charAt(const PRUint32 aIndex) const
{
  if (aIndex < mString.Length())
    return mString.CharAt(aIndex);
  return (UNICODE_CHAR)-1;
}

void String::clear()
{
  mString.Truncate();
}

void String::ensureCapacity(const PRUint32 aCapacity)
{
  mString.SetCapacity(aCapacity);
}

PRInt32 String::indexOf(const UNICODE_CHAR aData, const PRInt32 aOffset) const
{
  return mString.FindChar(aData, aOffset);
}

PRInt32 String::indexOf(const String& aData, const PRInt32 aOffset) const
{
  return mString.Find(aData.mString, aOffset);
}

MBool String::isEqual(const String& aData) const
{
  if (this == &aData)
    return MB_TRUE;
  return mString.Equals(aData.mString);
}

MBool String::isEqualIgnoreCase(const String& aData) const
{
  if (this == &aData)
    return MB_TRUE;
  return mString.Equals(aData.mString, nsCaseInsensitiveStringComparator());
}

MBool String::isEmpty() const
{
  return mString.IsEmpty();
}

PRUint32 String::length() const
{
  return mString.Length();
}

void String::setLength(const PRUint32 aLength)
{
  mString.SetLength(aLength);
}

String& String::subString(const PRUint32 aStart, String& aDest) const
{
  PRUint32 length = mString.Length() - aStart;
  if (length < 0) {
    aDest.clear();
  }
  else {
    aDest.mString.Assign(Substring(mString, aStart, length));
  }
  return aDest;
}

String& String::subString(const PRUint32 aStart, const PRUint32 aEnd, String& aDest) const
{
  PRUint32 length = aEnd - aStart;
  if (length < 0) {
    aDest.clear();
  }
  else {
    aDest.mString.Assign(Substring(mString, aStart, length));
  }
  return aDest;
}

void String::toLowerCase()
{
  ToLowerCase(mString);
}

void String::toUpperCase()
{
  ToUpperCase(mString);
}

nsString& String::getNSString()
{
  return mString;
}

const nsString& String::getConstNSString() const
{
  return mString;
}

// XXX DEPRECATED
String::String(const PRUint32 aSize)
{
  mString.SetCapacity(aSize);
}

String::String(const char* aSource)
{
  mString.AssignWithConversion(aSource);
}

String& String::operator = (const char* aSource)
{
  mString.AssignWithConversion(aSource);
  return *this;
}

void String::append(const char* aSource)
{
  mString.AppendWithConversion(aSource);
}

PRInt32 String::indexOf(const char aData, const PRInt32 aOffset) const
{
  return mString.FindChar((const PRUnichar)aData, aOffset);
}

PRInt32 String::lastIndexOf(const char aData, const PRInt32 aOffset) const
{
  return mString.RFindChar((const PRUnichar)aData, aOffset);
}

MBool String::isEqual(const char* aData) const
{
  return mString.EqualsWithConversion(aData);
}

char* String::toCharArray() const
{
  return ToNewCString(mString);
}
