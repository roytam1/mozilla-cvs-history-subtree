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

#ifndef _tmVector_H_
#define _tmVector_H_

#include "tmUtils.h"

/**
  * A simple, clear, self-growing, collection of objects. typed independant
  *   basically a growing array. Useful in situations where you need an
  *   indexed collection but do not know the size in advance and need the
  *   ability for increase and decrease in size. Not optimized for anything
  *   in particular, or any size in particular.
  *
  * Is able to guarantee the index of an item will
  *   not change due to removals of a lower indexed item. The growing,
  *   and shrinking all happens to the end of the collection
  *
  * Does not backfill, adds to the end. At some point this should be 
  *   changed to make best use of space.
  */
class tmVector
{
public:

  ////////////////////////////////////////////////////////////////////////////
  // Constructor(s) & Destructor

  /**
    * Set some sane default values to set up the internal storage. Init()
    *   must be called after construction of the object to allcate the
    *   backing store.
    */
  tmVector() : mIterator(0), mIndex(0), mNext(0) , mSize(0), mCapacity(10),
               mGrowthIncrement(3) {;}

  /**
    * Reclaim the memory allocated in the Init() method. 
    */
  virtual ~tmVector();

  ////////////////////////////////////////////////////////////////////////////
  // Public Member Functions

  /**
    * Allocates the storage back-end
    *
    * @returns NS_OK if allocation succeeded
    * @returns NS_ERROR_OUT_OF_MEMORY if the allocation failed
    */
  nsresult Init();

  // mutators

  /**
    * @returns NS_ERROR_INVALID_ARG if the element passed in is null
    * @returns NS_ERROR_OUT_OF_MEMORY if the collection needed to grow and
    *          the allocation of a new backing store failed.
    * @returns NS_ERROR_GENERATE_SUCCESS(index) if the add was successful.
    *          The calling method can get the index of the element added by
    *          using the macro NS_ERROR_GET_CODE() on the return value. But 
    *          for simple success test use NS_SUCCEEDED() on the return value.
    */
  nsresult Add(void *aElement);

  /**
    * This does not collapse the collection, it leaves holes. Note, it also
    *   doesn't delete the element, it merely removes it from the collection
    *
    * @returns NS_OK if the item was removed successfully
    * @returns NS_ERROR_INVALID_ARG if the index is out of bounds of the array
    */
  nsresult Remove(void *aElement);

  /**
    * This does not collapse the collection, it leaves holes.Note, it also
    *   doesn't delete the element, it merely removes it from the collection
    *
    * @returns NS_OK if the item was removed successfully
    * @returns NS_ERROR_INVALID_ARG if the index is out of bounds of the array
    */
  nsresult RemoveAt(PRUint32 aIndex);

  /**
    * Does not call delete on the elements since we have no idea how to
    *   reclaim the memory. Sets all array slots to nsnull.
    */
  void Clear();

  // accessors

  /**
    * @returns nsnull if the index is out of bounds OR if the slot indexed
    *          contains no element (nsnull)
    * @returns the element at the index given (could still be nsnull)
    */
  void* ElementAt(PRUint32 aIndex);

  /**
    * This isn't neccessarily the element at index 0.
    *
    * @returns the first non-null element in the collection
    * @returns nsnull if the collection is empty
    */
  void* FirstElement();

  /**
    * @returns the last non-null element in the collection
    * @reutrns nsnull if the collection is empty
    */
  void* LastElement();

  // utility

  /**
    * @returns the number of elements stored
    */
  PRUint32 Size() { return mSize; }

  /**
    * @returns the number of elements that can be held in the collection
    *          without needing to Grow() the backing store.
    */
  PRUint32 Capacity() { return mCapacity; }

  /**
    * This does a simple equality comparison on the pointer values
    *
    * @returns PR_TRUE if the pointer arg is contained in the collection
    * @returns PR_FALSE if the pointer arg is not contained in the collection
    */
  PRBool Contains(void *aElement);   // is the element in the collection

  // iteration

  /**
    * The iteration works in the following fashion. An initial call
    *   is made to Iterate(). This prepares the internal state of 
    *   the vector to walk through all the elements. Then a call is
    *   made to Next() to get each element of the vector, initially it
    *   would hand back the first. Each successive call to Next() hands
    *   the next element in the vector. If removal or caching of the index
    *   for the element last returned by Next() is  needed a call to 
    *   GetIterator() is made which hands back the index of the last
    *   element to be returned by a call to Next(). 
    *   
    *   The following code
    *   example demonstrates the cycling through all of the elements 
    *   and their subsequent removal from the vector:
    *   <tt>
    *   tmVector aVector;
    *   aVector.Init();
    *   ... populate the vector ...
    *
    *   aVector.Iterate();
    *   void* anElement = nsnull;
    *   while (anElement = aVector.Next() != nsnull) {
    *     aVector.RemoveAt(aVector.GetIterator());
    *   }
    */
  void Iterate();

  /**
    * Used during iteration through the collection
    *
    * @returns the next non-null element in the collection
    * @returns nsnull if there are no more elements in the collection
    */
  void* Next();

  /**
    * @returns the index of the element returned by the last Next() call
    * @returns 0 if called directly after an Iterate() call
    */
  PRUint32 GetIterator() { return mLastIterator; }

protected:

  nsresult Grow();     // increase the mCapacity by mGrowth - reallocs
  nsresult Shrink();   // decrease the mCapacity by mGrowth - dumb, reallocs

  ////////////////////////////////////////////////////////////////////////////
  // Protected Member Variables

  // utility variables
  PRUint32 mIterator;         // next element Next() will return (0 based)
  PRUint32 mLastIterator;     // last element returned by Next() (0 based)
  PRUint32 mIndex;            // generic reuseable index

  // bookkeeping variables
  PRUint32 mNext;             // next element insertion slot (0 based)
  PRUint32 mSize;             // how many elements in the Vector (1 based)
  PRUint32 mCapacity;         // current capacity of the Vector (1 based)
  PRUint32 mGrowthIncrement;  // how many slots to grow/shrink the array by

  // the actual array of objects being stored
  void **mElements;

private:

};

#endif
