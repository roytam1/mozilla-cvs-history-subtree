/**
 * hashtable.cpp: Implementation of HashTable
 *
 * A simple class to wrapper our C based hashtable algorithm
 * and make it look like the java.util.HashTable for easy
 * porting from Java.
 *
 * @author Dan Libby
 * @version %I%, %G%
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

