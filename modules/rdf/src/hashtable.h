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
