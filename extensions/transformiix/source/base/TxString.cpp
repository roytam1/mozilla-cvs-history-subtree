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

String::String(const String& aSource) : mLength(aSource.length()),
                                        mBufferLength(mLength),
                                        mBuffer(aSource.toUnicode())
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
  memcpy(mBuffer[mLength], aSource.mBuffer,
         aSource.mLength * sizeof(UNICODE_CHAR));
  mLength += aSource.mLength;
}

void String::insert(const PRInt32 aOffset, const UNICODE_CHAR aSource)
{
  PRInt32 offset = (aOffset < 0) ? 0 : aOffset;

  if (offset < mLength) {
    ensureCapacity(1);
    memmove(&mBuffer[offset + 1], &mBuffer[offset],
            (mLength - offset) * sizeof(UNICODE_CHAR));
    mBuffer[offset] = aSource;
    mLength += 1;
  }
  else {
    append(aSource);
  }
}

void String::insert(const PRInt32 aOffset, const String& aSource)
{
  PRInt32 offset = (aOffset < 0) ? 0 : aOffset;

  if (offset < mLength) {
    ensureCapacity(aSource.mLength);
    memmove(&mBuffer[offset + aSource.mLength], &mBuffer[offset],
            (mLength - offset) * sizeof(UNICODE_CHAR));
    memcpy(&mBuffer[offset], aSource.mBuffer,
           aSource.mLength * sizeof(UNICODE_CHAR));
    mLength += aSource.mLength;
  }
  else {
    append(aSource);
  }
}

void String::replace(const PRInt32 aOffset, const UNICODE_CHAR aSource)
{
  PRInt32 offset = (aOffset < 0) ? 0 : aOffset;

  if (offset < mLength) {
    mBuffer[offset] = aSource;
  }
  else {
    append(aSource);
  }
}

void String::replace(PRInt32 aOffset, const String& aSource)
{
  PRInt32 offset = (aOffset < 0) ? 0 : aOffset;

  if (offset < mLength) {
    PRInt32 finalLength = offset + aSource.mLength;

    if (finalLength > mLength) {
      ensureCapacity(totalOffset - mLength);
      mLength = finalLength;
    }
    memcpy(&mBuffer[offset], aSource.mBuffer,
           aSource.mLength * sizeof(UNICODE_CHAR));
  }
  else {
    append(aSource);
  }
}

void String::deleteChars(PRInt32 aOffset, PRInt32 aCount)
{
  PRInt32 offset = (aOffset < 0) ? 0 : aOffset;
  PRInt32 cutEnd = offset + count;

  if (cutEnd < mLength) {
    memmove(&mBuffer[offset], &mBuffer[cutEnd],
            (mLength - cutEnd) * sizeof(UNICODE_CHAR));
    mLength -= count;
  }
  else {
    mLength = offset;
  }
}

UNICODE_CHAR String::charAt(PRInt32 index) const
{
  if ((index < mLength) && (index >= 0)) {
    return mBuffer[index];
  }
  return (UNICODE_CHAR)-1;
}

void String::clear()
{
  mLength = 0;
}

void String::ensureCapacity(PRInt32 aCapacity)
{
  PRInt32 freeSpace = mBufferLength - mLength;

  if (freeSpace >= aCapacity) {
    return;
  }

  mBufferLength += aCapacity - freeSpace;
  UNICODE_CHAR* tempBuffer = new UNICODE_CHAR[mBufferLength];
  memcpy(tempBuffer, mBuffer, mLength);
  if (mBuffer)
     delete mBuffer;
  mBuffer = tempBuffer;
}

PRInt32 String::indexOf(UNICODE_CHAR aData, PRInt32 aOffset) const
{
  PRInt32 searchIndex = (aOffset < 0) ? 0 : aOffset;

  while (searchIndex < mLength) {
    if (mBuffer[searchIndex] == data)
      return searchIndex;
    ++searchIndex;
  }
  return NOT_FOUND;
}

PRInt32 String::indexOf(const String& aData, PRInt32 aOffset) const
{
  PRInt32 searchIndex = (aOffset < 0) ? 0 : aOffset;
  PRInt32 searchLimit = mLength - aData.mLength;

  while (searchIndex <= searchLimit) {
    if (isEqual(&mBuffer[searchIndex], aData.mBuffer, aData.mLength))
      return searchIndex;
    ++searchIndex;
  }
  return NOT_FOUND;
}

PRInt32 String::lastIndexOf(UNICODE_CHAR aData, PRInt32 aOffset) const
{
    if ((aOffset < 0) || (aOffset >= mLength))
       return NOT_FOUND;

    PRInt32 searchIndex = aOffset;
    while (searchIndex >= 0) {
        if (mBuffer[searchIndex] == data)
          return searchIndex;
        --searchIndex;
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
  PRInt32 compLoop = 0;
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

PRInt32 String::length() const
{
  return mLength;
}

void String::setLength(PRInt32 length) {
    setLength(length, '\0');
}

String& String::subString(PRInt32 aStart, String& aDest) const
{
  return subString(aStart, mLength, aDest);
}

String& String::subString(PRInt32 aStart, PRInt32 aEnd, String& aDest) const
{
  PRInt32 start = (aStart < 0) ? 0 : aStart;
  PRInt32 end = (aEnd > mLength) ? mLength : aEnd;

  aDest.clear();
  if (start < end) {
    aDest.ensureCapacity(end - start);
    memcpy(aDest.mBuffer, &mBuffer[aStart], start - end);
    aDest.mLength = start - end;
  }

  return aDest;
}

char* String::toCharArray() const
{
  char* tmpBuffer = new char[mLength + 1];
  NS_ASSERTION(tmpBuffer, "out of memory");
  return toCharArray(tmpBuffer);
}

const UNICODE_CHAR* String::toUnicode() const
{
  return mBuffer;
}

void String::toLowerCase()
{
  PRInt32 conversionLoop;

  for (conversionLoop = 0; conversionLoop < mLength; ++conversionLoop) {
    if ((mBuffer[conversionLoop] >= 'A') &&
        (mBuffer[conversionLoop] <= 'Z'))
      mBuffer[conversionLoop] += 32;
  }
}

void String::toUpperCase()
{
  PRInt32 conversionLoop;

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
  PRInt32 trimLoop = mLength - 1;

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
