/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Transaction Manager.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corp.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   John Gaunt <jgaunt@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "plstr.h"
#include "tmVector.h"

////////////////////////////////////////////////////////////////////////////
// Constructor(s) & Destructor

// can not be responsible for reclaiming memory pointed to by the void*s in
//   the collection - how would we reclaim, don't know how they were allocated
tmVector::~tmVector() {
  Clear();
  delete [] mElements;
}

///////////////////////////////////////////////////////////////////////////////
// Public Member Functions

nsresult
tmVector::Init() {

  mElements = new void*[mCapacity];
  if (!mElements)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

///////////////////////////////////////////////////////////////////////////////
// mutators

nsresult
tmVector::Add(void *aElement){

  if (!aElement)
    return NS_ERROR_INVALID_ARG;

  // make sure there is room
  if (mNext == mCapacity)
    if (NS_FAILED(Grow()))
      return NS_ERROR_OUT_OF_MEMORY;

  // put the element in the array
  mElements[mNext] = aElement;
  mSize++;

  // encapsulates the index into a success value
  return NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_TM, mNext++);
}

nsresult
tmVector::Remove(void *aElement) {

  if (!aElement)
    return NS_ERROR_INVALID_ARG;

  for (mIndex = 0; mIndex < mNext; mIndex++) {
    if (mElements[mIndex] == aElement) {
      mElements[mIndex] = nsnull;
      mSize--;
      if (mIndex == mNext-1) {   // if we removed the last element
        mNext--;
        // don't test for success of the shrink
        Shrink();
      }
    }
  }
  return NS_OK;
}

nsresult
tmVector::RemoveAt(PRUint32 aIndex) {

  // bounds check
  if (aIndex >= mNext || aIndex < 0)
    return NS_ERROR_INVALID_ARG;

  // remove the element if it isn't already nsnull
  if (mElements[aIndex] != nsnull) {
    mElements[aIndex] = nsnull;
    mSize--;
    if (aIndex == mNext-1) {   // if we removed the last element
      mNext--;
      // don't test for success of the shrink
      Shrink();
    }
  }
  return NS_OK;
}

// Does not delete any of the data, merely removes references to them
void
tmVector::Clear(){

  for (mIndex = 0; mIndex < mCapacity; mIndex++) {
    mElements[mIndex] = nsnull;
  }
  mSize = 0;
  mNext = 0;
}

///////////////////////////////////////////////////////////////////////////////
// accessors

void*
tmVector::ElementAt(PRUint32 aIndex) {

  // bounds check
  if (aIndex >= mNext || aIndex < 0)
    return nsnull;

  return mElements[aIndex];
}

void*
tmVector::FirstElement() {

  // find the first non-null element
  for (mIndex = 0; mIndex < mNext; mIndex++) {
    if (mElements[mIndex] != nsnull)
      return (void*)mElements[mIndex];
  }
  return nsnull;
}

void*
tmVector::LastElement() {
  return (void*)mElements[mNext-1];
}

///////////////////////////////////////////////////////////////////////////////
// utility

PRBool
tmVector::Contains(void *aElement) {

  for (mIndex = 0; mIndex < mNext; mIndex++) {
    if (mElements[mIndex] == aElement)
      return PR_TRUE;
  }
  return PR_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// iteration

// Sets the internal indices used for iteration
void
tmVector::Iterate() {
  mIterator = mLastIterator = 0;
}

// recursive
void*
tmVector::Next() {

  // bounds check -- have we reached the end?
  if (mIterator >= mNext)
    return nsnull;

  // recurse if current element is null -- there can be holes.
  if (mElements[mIterator] == nsnull) {
    mIterator++;
    return Next();
  }

  // we've found an element
  mLastIterator = mIterator;
  return (void*)mElements[mIterator++];
}

//////////////////////////////////////////////////////////////////////////////
// Protected Member Functions

// increases the capacity by the growth increment
nsresult
tmVector::Grow() {

  // 1) compute the new capacity and allocate a new array
  PRUint32 newcap = mCapacity + mGrowthIncrement;
  void **elements = new void*[newcap];
  if (!elements)
    return NS_ERROR_OUT_OF_MEMORY;

  // 2) assign all elements into the new array, zeroing the extra slots
  for (mIndex = 0; mIndex < newcap; mIndex++) {
    if (mIndex < mNext) {           // only assign data that exists
      elements[mIndex] = mElements[mIndex];
      mElements[mIndex] = nsnull;
    }
    else
      elements[mIndex] = nsnull;
  }

  // 3) clean up the old array
  delete [] mElements;

  // 4) set the variables to their new values
  mElements = elements;
  mCapacity = newcap;

  return NS_OK;
}

// reduces the capacity by the growth increment. leaves room
//   for one more add before needing to Grow().
nsresult
tmVector::Shrink() {

  // 1) compute the new capacity
  PRUint32 newcap = mCapacity - mGrowthIncrement;

  // 2) if the next element could be assigned still
  if (mNext < newcap) {

    // 3) allocate the new array
    void **elements = new void*[newcap];
    if (!elements)
      return NS_ERROR_OUT_OF_MEMORY;

    // 4) assign the data  into the new array
    for(mIndex = 0; mIndex < newcap; mIndex++) {
      elements[mIndex] = mElements[mIndex];
      mElements[mIndex] = nsnull;
    }

    // 5) clean up the old array
    delete [] mElements;

    // 6) set the variables to their new values
    mElements = elements;
    mCapacity = newcap;
  }
  return NS_OK;
}
