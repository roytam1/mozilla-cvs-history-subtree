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

String::~String()
{
  delete mBuffer;
}

void String::append(const UNICODE_CHAR aSource)
{
  ensureCapacity(1);
  mBuffer[mLength + 1] = aSource;
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

void String::ensureCapacity(const PRUint32 aCapacity)
{
  PRUint32 freeSpace = mBufferLength - mLength;

  if (freeSpace >= aCapacity) {
    return;
  }

  mBufferLength += aCapacity - freeSpace;
  UNICODE_CHAR* tempBuffer = new UNICODE_CHAR[mBufferLength];
  memcpy(tempBuffer, mBuffer, mLength);
  delete mBuffer;
  mBuffer = tempBuffer;
}

PRInt32 String::indexOf(UNICODE_CHAR aData, const PRUint32 aOffset) const
{
  PRUint32 searchIndex = aOffset;

  while (searchIndex < mLength) {
    if (mBuffer[searchIndex] == aData)
      return searchIndex;
    ++searchIndex;
  }
  return NOT_FOUND;
}

PRInt32 String::indexOf(const String& aData, const PRUint32 aOffset) const
{
  PRUint32 searchIndex = aOffset;
  PRUint32 searchLimit = mLength - aData.mLength;

  while (searchIndex <= searchLimit) {
    if (isEqual(&mBuffer[searchIndex], aData.mBuffer, aData.mLength))
      return searchIndex;
    ++searchIndex;
  }
  return NOT_FOUND;
}

PRInt32 String::lastIndexOf(UNICODE_CHAR aData, const PRUint32 aOffset) const
{
    if (aOffset >= mLength)
       return NOT_FOUND;

    PRUint32 searchIndex = aOffset + 1;
    while (--searchIndex > 0) {
        if (mBuffer[searchIndex] == data)
          return searchIndex;
    }
    return NOT_FOUND;
}

MBool String::isEqual(const String& aData) const
{
  if (mLength != aData.mLength)
    return MB_FALSE;
  return isEqual(mBuffer, aData.mBuffer, aData.mLength);
}

MBool String::isEqualIgnoreCase(const String& aData) const
{
  if (mLength != aData.mLength)
    return MB_FALSE;

  const UNICODE_CHAR* otherBuffer = aData.mBuffer;
  UNICODE_CHAR thisChar, otherChar;
  PRUint32 compLoop = 0;
  while (compLoop < mLength) {
    thisChar = mBuffer[compLoop];
    if ((thisChar >= 'A') && (thisChar <= 'Z'))
      thisChar += 32;
    otherChar = otherBuffer[compLoop];
    if ((otherChar >= 'A') && (otherChar <= 'Z'))
      otherChar += 32;
    if (thisChar != otherChar)
      return MB_FALSE;
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

void String::setLength(const PRUint32 length)
{
    setLength(length, '\0');
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
    memcpy(aDest.mBuffer, &mBuffer[aStart], substrLength);
    aDest.mLength = substrLength;
  }

  return aDest;
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

const UNICODE_CHAR* String::toUnicode() const
{
  UNICODE_CHAR* tmpBuffer = new UNICODE_CHAR[mLength + 1];
  NS_ASSERTION(tmpBuffer, "out of memory");
  if (tmpBuffer) {
    memcpy(tmpBuffer, mBuffer, mLength);
    tmpBuffer[mLength] = 0;
  }
  return tmpBuffer;
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

void String::trim()
{
  // As long as we are not working on an emtpy string, trim from the right
  // first, so we don't have to move useless spaces when we trim from the left.
  MBool done = MB_FALSE;
  PRUint32 trimLoop = mLength - 1;

  if (mLength > 0)
  {
    while (!done)
    {
      switch (mBuffer[trimLoop])
      {
        case ' ' :
        case '\t' :
        case '\n' :
        case '\r' :
          --mLength;
          --trimLoop;
          break;

        default :
          done = MB_TRUE;
          break;
      }
    }
  }

  // Now, if there are any characters left to the string, Trim to the left.
  // First count the number of "left" spaces.  Then move all characters to the
  // left by that ammount.
  if (mLength > 0)
  {
    done = MB_FALSE;
    trimLoop = 0;
    while (!done)
    {
      switch (mBuffer[trimLoop])
      {
        case ' ' :
        case '\t' :
        case '\n' :
        case '\r' :
          ++trimLoop;
          break;

        default :
          done = MB_TRUE;
          break;
      }
    }

    if (trimLoop < mLength) {
      memmove(mBuffer, &mBuffer[trimLoop], (mLength - trimLoop) * sizeof(UNICODE_CHAR));
    }
    mLength -= trimLoop;
  }
}
