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

#ifndef _INDEXED_POOL_H_
#define _INDEXED_POOL_H_

#include "Fundamentals.h"
#include <string.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// IndexedPool<IndexedObjectSubclass> is an indexed pool of objects. The
// template parameter 'IndexedObjectSubclass' must be a subclass of the struct
// IndexedObject.
// 
// When the indexed pool is ask to allocate and initialize a new object (using
// the operator new(anIndexedPool) it will zero the memory used to store the
// object and initialize the field 'index' of this object to its position in
// the pool.
//
// An object allocated by the indexed pool can be freed by calling the method
// IndexedPool::release(IndexedElement& objectIndex).
//
// example:
//
//   IndexedPool<IndexedElement> elementPool;
//
//   IndexedElement& element1 = *new(elementPool) IndexedElement();
//   IndexedElement& element2 = *new(elementPool) IndexedElement();
//
//   indexedPool.release(element1);
//   IndexedElement& element3 = *new(elementPool) IndexedElement();
//
//  At this point element1 is no longer a valid object, element2 is at
//  index 2 and element3 is at index 1.
//

//------------------------------------------------------------------------------
// IndexedObject -
//

template<class Object>
struct IndexedObject
{
	Uint32			index;				// Index in the pool.
	Object*			next;				// Used to link IndexedObject together.

	Uint32 getIndex() {return index;}
};

//------------------------------------------------------------------------------
// IndexedPool<IndexedObject> -
//

template <class IndexedObject>
class IndexedPool
{
private:

	static const	blockSize = 4;		// Size of one block.

	Uint32			nBlocks;			// Number of blocks in the pool.
	IndexedObject**	block;				// Array of block pointers.
	IndexedObject*  freeObjects;		// Chained list of free IndexedObjects.
	Uint32			nextIndex;			// Index of the next free object in the last block. 

private:

	void allocateAnotherBlock();
	IndexedObject& newObject();

public:

	IndexedPool() : nBlocks(0), block(NULL), freeObjects(NULL), nextIndex(1) {}
	~IndexedPool();

	IndexedObject& get(Uint32 index) const;
	void release(IndexedObject& object);

	void setSize(Uint32 size) {assert(size < nextIndex); nextIndex = size;}

	// Return the universe size.
	Uint32 getSize() {return nextIndex;}

	friend void* operator new(size_t, IndexedPool<IndexedObject>& pool); // Needs to call newObject().
};

// Free all the memory allocated for this object.
//
template <class IndexedObject>
IndexedPool<IndexedObject>::~IndexedPool()
{
	for (Uint32 n = 0; n < nBlocks; n++)
		free(&((IndexedObject **) &block[n][n*blockSize])[-(n + 1)]);
}

// Release the given. This object will be iserted in the chained
// list of free IndexedObjects. To minimize the fragmentation the chained list 
// is ordered by ascending indexes.
//
template <class IndexedObject>
void IndexedPool<IndexedObject>::release(IndexedObject& object)
{
	Uint32 index = object.index;
	IndexedObject* list = freeObjects;

	assert(&object == &get(index)); // Make sure that object is owned by this pool.

	if (list == NULL) { // The list is empty.
		freeObjects = &object;
		object.next = NULL;
	} else { // The list contains at least 1 element.
		if (index < list->index) { // insert as first element.
			freeObjects = &object;
			object.next = list;
		} else { // Find this object's place.
			while ((list->next) != NULL && (list->next->index < index))
				list = list->next;

			object.next = list->next;
			list->next = &object;
		}
	}

#ifdef DEBUG
	// Sanity check to be sure that the list is correctly ordered.
	for (IndexedObject* obj = freeObjects; obj != NULL; obj = obj->next)
		if (obj->next != NULL)
			assert(obj->index < obj->next->index);
#endif
}

// Create a new block of IndexedObjects. We will allocate the memory to
// store IndexedPool::blockSize IndexedObject and the new Array of block
// pointers.
// The newly created IndexedObjects will not be initialized.
//
template <class IndexedObject>
void IndexedPool<IndexedObject>::allocateAnotherBlock()
{
	void* memory = (void *) malloc((nBlocks + 1) * sizeof(Uint32) + blockSize * sizeof(IndexedObject));

	memcpy(memory, block, nBlocks * sizeof(Uint32));

	block = (IndexedObject **) memory;
	IndexedObject* objects = (IndexedObject *) &block[nBlocks + 1];

	block[nBlocks] = &objects[-(nBlocks * blockSize)];
	nBlocks++;
}

// Return the IndexedObject at the position 'index' in the pool.
//
template <class IndexedObject>
IndexedObject& IndexedPool<IndexedObject>::get(Uint32 index) const
{
	Uint32 blockIndex = index / blockSize;
	assert(blockIndex < nBlocks);

	return block[blockIndex][index];
}

// Return the reference of an unused object in the pool.
//
template <class IndexedObject>
IndexedObject& IndexedPool<IndexedObject>::newObject()
{
	if (freeObjects != NULL) {
		IndexedObject& newObject = *freeObjects;
		freeObjects = newObject.next;
		return newObject;
	}
	
	Uint32 nextIndex = this->nextIndex++;
	Uint32 blockIndex = nextIndex / blockSize;

	while (blockIndex >= nBlocks)
		allocateAnotherBlock();
	
	IndexedObject& newObject = block[blockIndex][nextIndex];
	newObject.index = nextIndex;

	return newObject;
}

// Return the address of the next unsused object in the given
// indexed pool. The field index of the newly allocated object
// will be initialized to the corresponding index of this object
// in the pool.
//
template <class IndexedObject>
void* operator new(size_t size, IndexedPool<IndexedObject>& pool)
{
	assert(size == sizeof(IndexedObject));
	return (void *) &pool.newObject();
}

#endif // _INDEXED_POOL_H_
