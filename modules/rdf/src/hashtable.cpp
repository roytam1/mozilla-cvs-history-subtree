/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/**
 * hashtable.cpp: Implementation of HashTable
 *
 * A simple class to wrapper our C based hashtable algorithm
 * and make it look like the java.util.HashTable for easy
 * porting from Java.
 *
 */

#include "hashtable.h"
#include <string.h>

static int hash_strcmp (const void *a, const void *b)
{
  return strcmp((const char *) a, (const char *) b);
}

HashTable::HashTable()
{
	table = PL_NewHashTable (100, /* Guess at # of items */
                            PL_HashString,
									 hash_strcmp,
                            PL_CompareValues,
                            0, 0);
}

HashTable::HashTable(int initialCapacity)
{
	table = PL_NewHashTable (initialCapacity,
                            PL_HashString,
									 PL_CompareStrings,
                            PL_CompareStrings,
                            0, 0);
}

HashTable::~HashTable()
{
   PL_HashTableDestroy(table);
}

void* HashTable::get(void* key)
{
	return PL_HashTableLookup(table, key);
}

void* HashTable::put(void* key, void* value)
{
   return PL_HashTableAdd(table, key, value);
}

void* HashTable::remove(void* key)
{
	void* value = get(key);
	if(value)
	{
     PL_HashTableRemove(table, key);
	  return value;
	}
	return 0;
}

