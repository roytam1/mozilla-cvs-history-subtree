#ifndef HASHTABLE
#define HASHTABLE

/**
 * hashtable.h : declaration of HashTable
 *
 * A simple class to wrapper our C based hashtable algorithm
 * and make it look like the java.util.HashTable for easy
 * porting from Java.
 *
 * @author Dan Libby
 * @version %I%, %G%
 */

#include "plhash.h"

class HashTable
{
protected:
   PLHashTable *table;

public:
	HashTable();
   HashTable(int initialCapacity);
	virtual ~HashTable();
	void clear();
	void* get(void* key);
	void* put(void* key, void* value);
	void* remove(void *key);
};

#endif 
