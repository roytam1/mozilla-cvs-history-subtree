/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

#include "TxString.h"
#include <stdlib.h>
#include <string.h>

String::String() : mBuffer(0),
                   mBufferLength(0),
                   mLength(0)
{
}

String::String(const String& aSource) : mBuffer(aSource.toUnicode()),
                                        mBufferLength(aSource.mLength),
                                        mLength(aSource.mLength)
{
}

String::String(const UNICODE_CHAR* aSource,
               const PRUint32 aLength) : mBuffer(0),
                                         mBufferLength(0),
                                         mLength(0)
{
  if (!aSource) {
    return;
  }

  PRUint32 length = aLength;
  if (length == 0) {
    length = unicodeLength(aSource);
  }
  ensureCapacity(length);
  memcpy(mBuffer, aSource, length * sizeof(UNICODE_CHAR));
  mLength = length;
}

String::~String()
{
  delete [] mBuffer;
}

void String::append(const UNICODE_CHAR aSource)
{
  ensureCapacity(1);
  mBuffer[mLength] = aSource;
  ++mLength;
}

void String::append(const String& aSource)
{
  ensureCapacity(aSource.mLength);
  memcpy(&mBuffer[mLength], aSource.mBuffer,
         aSource.mLength * sizeof(UNICODE_CHAR));
  mLength += aSource.mLength;
}

void String::insert(const PRUint32 aOffset, const UNICODE_CHAR aSource)
{
  if (aOffset < mLength) {
    ensureCapacity(1);
    memmove(&mBuffer[aOffset + 1], &mBuffer[aOffset],
            (mLength - aOffset) * sizeof(UNICODE_CHAR));
    mBuffer[aOffset] = aSource;
    mLength += 1;
  }
  else {
    append(aSource);
  }
}

void String::insert(const PRUint32 aOffset, const String& aSource)
{
  if (aOffset < mLength) {
    ensureCapacity(aSource.mLength);
    memmove(&mBuffer[aOffset + aSource.mLength], &mBuffer[aOffset],
            (mLength - aOffset) * sizeof(UNICODE_CHAR));
    memcpy(&mBuffer[aOffset], aSource.mBuffer,
           aSource.mLength * sizeof(UNICODE_CHAR));
    mLength += aSource.mLength;
  }
  else {
    append(aSource);
  }
}

void String::replace(const PRUint32 aOffset, const UNICODE_CHAR aSource)
{
  if (aOffset < mLength) {
    mBuffer[aOffset] = aSource;
  }
  else {
    append(aSource);
  }
}

void String::replace(const PRUint32 aOffset, const String& aSource)
{
  if (aOffset < mLength) {
    PRUint32 finalLength = aOffset + aSource.mLength;

    if (finalLength > mLength) {
      ensureCapacity(finalLength - mBufferLength);
      mLength = finalLength;
    }
    memcpy(&mBuffer[aOffset], aSource.mBuffer,
           aSource.mLength * sizeof(UNICODE_CHAR));
  }
  else {
    append(aSource);
  }
}

void String::deleteChars(const PRUint32 aOffset, const PRUint32 aCount)
{
  PRUint32 cutEnd = aOffset + aCount;

  if (cutEnd < mLength) {
    memmove(&mBuffer[aOffset], &mBuffer[cutEnd],
            (mLength - cutEnd) * sizeof(UNICODE_CHAR));
    mLength -= aCount;
  }
  else {
    mLength = aOffset;
  }
}

UNICODE_CHAR String::charAt(const PRUint32 aIndex) const
{
  if (aIndex < mLength) {
    return mBuffer[aIndex];
  }
  return (UNICODE_CHAR)-1;
}

void String::clear()
{
  mLength = 0;
}

PRInt32 String::indexOf(const UNICODE_CHAR aData, const PRInt32 aOffset) const
{
  PRInt32 searchIndex = aOffset;

  while (searchIndex < mLength) {
    if (mBuffer[searchIndex] == aData) {
      return searchIndex;
    }
    ++searchIndex;
  }
  return kNotFound;
}

PRInt32 String::indexOf(const String& aData, const PRInt32 aOffset) const
{
  PRInt32 searchIndex = aOffset;
  PRInt32 searchLimit = mLength - aData.mLength;

  while (searchIndex <= searchLimit) {
    if (memcmp(&mBuffer[searchIndex], aData.mBuffer,
        aData.mLength * sizeof(UNICODE_CHAR)) == 0) {
      return searchIndex;
    }
    ++searchIndex;
  }
  return kNotFound;
}

PRInt32 String::lastIndexOf(const UNICODE_CHAR aData, const PRInt32 aOffset) const
{
  if (aOffset < 0) {
     return kNotFound;
  }

  PRUint32 searchIndex = mLength - aOffset;
  while (--searchIndex >= 0) {
    if (mBuffer[searchIndex] == aData) {
      return searchIndex;
    }
  }
  return kNotFound;
}

MBool String::isEqual(const String& aData) const
{
  if (mLength != aData.mLength) {
    return MB_FALSE;
  }
  return (memcmp(mBuffer, aData.mBuffer, mLength * sizeof(UNICODE_CHAR)) == 0);
}

MBool String::isEqualIgnoreCase(const String& aData) const
{
  if (mLength != aData.mLength) {
    return MB_FALSE;
  }

  UNICODE_CHAR thisChar, otherChar;
  PRUint32 compLoop = 0;
  while (compLoop < mLength) {
    thisChar = mBuffer[compLoop];
    if ((thisChar >= 'A') && (thisChar <= 'Z')) {
      thisChar += 32;
    }
    otherChar = aData.mBuffer[compLoop];
    if ((otherChar >= 'A') && (otherChar <= 'Z')) {
      otherChar += 32;
    }
    if (thisChar != otherChar) {
      return MB_FALSE;
    }
    ++compLoop;
  }
  return MB_TRUE;
}

MBool String::isEmpty() const
{
  return (mLength == 0);
}

PRUint32 String::length() const
{
  return mLength;
}

void String::setLength(const PRUint32 aLength)
{
  mLength = aLength;
}

String& String::subString(const PRUint32 aStart, String& aDest) const
{
  return subString(aStart, mLength, aDest);
}

String& String::subString(const PRUint32 aStart, const PRUint32 aEnd, String& aDest) const
{
  PRUint32 end = (aEnd > mLength) ? mLength : aEnd;

  aDest.clear();
  if (aStart < end) {
    PRUint32 substrLength = end - aStart;

    aDest.ensureCapacity(substrLength);
    memcpy(aDest.mBuffer, &mBuffer[aStart],
           substrLength * sizeof(UNICODE_CHAR));
    aDest.mLength = substrLength;
  }
  return aDest;
}

void String::toLowerCase()
{
  PRUint32 conversionLoop;

  for (conversionLoop = 0; conversionLoop < mLength; ++conversionLoop) {
    if ((mBuffer[conversionLoop] >= 'A') &&
        (mBuffer[conversionLoop] <= 'Z'))
      mBuffer[conversionLoop] += 32;
  }
}

void String::toUpperCase()
{
  PRUint32 conversionLoop;

  for (conversionLoop = 0; conversionLoop < mLength; ++conversionLoop) {
    if ((mBuffer[conversionLoop] >= 'a') &&
        (mBuffer[conversionLoop] <= 'z'))
      mBuffer[conversionLoop] -= 32;
  }
}

String& String::operator = (const String& aSource)
{
  mBuffer = aSource.toUnicode();
  mBufferLength = aSource.mLength;
  mLength = aSource.mLength;
  return *this;
}

void String::ensureCapacity(const PRUint32 aCapacity)
{
  PRUint32 freeSpace = mBufferLength - mLength;

  if (freeSpace >= aCapacity) {
    return;
  }

  mBufferLength += aCapacity - freeSpace;
  UNICODE_CHAR* tempBuffer = new UNICODE_CHAR[mBufferLength];
  if (mLength > 0) {
    memcpy(tempBuffer, mBuffer, mLength * sizeof(UNICODE_CHAR));
  }
  delete [] mBuffer;
  mBuffer = tempBuffer;
}

UNICODE_CHAR* String::toUnicode() const
{
  if (mLength == 0) {
    return 0;
  }
  UNICODE_CHAR* tmpBuffer = new UNICODE_CHAR[mLength];
  NS_ASSERTION(tmpBuffer, "out of memory");
  if (tmpBuffer) {
    memcpy(tmpBuffer, mBuffer, mLength * sizeof(UNICODE_CHAR));
  }
  return tmpBuffer;
}

PRUint32 String::unicodeLength(const UNICODE_CHAR* aData)
{
  PRUint32 index = 0;

  // Count UNICODE_CHARs Until a Unicode "NULL" is found.
  while (aData[index] != 0x0000) {
    ++index;
  }
  return index;
}

ostream& operator<<(ostream& aOutput, const String& aSource)
{
  PRUint32 outputLoop;

  for (outputLoop = 0; outputLoop < aSource.mLength; ++outputLoop) {
    aOutput << (char)aSource.charAt(outputLoop);
  }
  return aOutput;
}

// XXX DEPRECATED
String::String(const PRUint32 aSize) : mBuffer(0),
                                       mBufferLength(0),
                                       mLength(0)
{
  ensureCapacity(aSize);
}

String::String(const char* aSource) : mBuffer(0),
                                      mBufferLength(0),
                                      mLength(0)
{
  if (!aSource) {
    return;
  }

  PRUint32 length = strlen(aSource);
  ensureCapacity(length);
  PRUint32 counter;
  for (counter = 0; counter < length; ++counter) {
    mBuffer[counter] = (UNICODE_CHAR)aSource[counter];
  }
  mLength = length;
}

void String::append(const char* aSource)
{
  if (!aSource) {
    return;
  }

  PRUint32 length = strlen(aSource);
  ensureCapacity(length);
  PRUint32 counter;
  for (counter = 0; counter < length; ++counter) {
    mBuffer[mLength + counter] = (UNICODE_CHAR)aSource[counter];
  }
  mLength += length;
}

MBool String::isEqual(const char* aData) const
{
  if (!aData) {
    return MB_FALSE;
  }

  PRUint32 length = strlen(aData);
  if (length != mLength) {
    return MB_FALSE;
  }

  PRUint32 counter;
  for (counter = 0; counter < length; ++counter) {
    if (mBuffer[counter] != (UNICODE_CHAR)aData[counter]) {
      return MB_FALSE;
    }
  }
  return MB_TRUE;
}

char* String::toCharArray() const
{
  char* tmpBuffer = new char[mLength + 1];
  NS_ASSERTION(tmpBuffer, "out of memory");
  if (tmpBuffer) {
    PRUint32 conversionLoop;

    for (conversionLoop = 0; conversionLoop < mLength; ++conversionLoop) {
      tmpBuffer[conversionLoop] = (char)mBuffer[conversionLoop];
    }
    tmpBuffer[mLength] = 0;
  }
  return tmpBuffer;
}
