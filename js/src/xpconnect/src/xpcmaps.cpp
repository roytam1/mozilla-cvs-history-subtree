/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   John Bandhauer <jband@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

/* Private maps (hashtables). */

#include "xpcprivate.h"

/***************************************************************************/
// static shared...

JS_STATIC_DLL_CALLBACK(JSHashNumber)
hash_root(const void *key)
{
    return ((JSHashNumber) key) >> 2; /* help lame MSVC1.5 on Win16 */
}

JS_STATIC_DLL_CALLBACK(JSHashNumber)
hash_IID(const void *key)
{
    return (JSHashNumber) *((PRUint32*)key);
}

JS_STATIC_DLL_CALLBACK(intN)
compare_IIDs(const void *v1, const void *v2)
{
    return ((const nsID*)v1)->Equals(*((const nsID*)v2));
}

/***************************************************************************/
// implement JSContext2XPCContextMap...

// static
JSContext2XPCContextMap*
JSContext2XPCContextMap::newMap(int size)
{
    JSContext2XPCContextMap* map = new JSContext2XPCContextMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

JSContext2XPCContextMap::JSContext2XPCContextMap(int size)
{
    mTable = JS_NewHashTable(size, hash_root,
                             JS_CompareValues, JS_CompareValues,
                             nsnull, nsnull);
}

JSContext2XPCContextMap::~JSContext2XPCContextMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement JSObject2WrappedJSMap...

// static
JSObject2WrappedJSMap*
JSObject2WrappedJSMap::newMap(int size)
{
    JSObject2WrappedJSMap* map = new JSObject2WrappedJSMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

JSObject2WrappedJSMap::JSObject2WrappedJSMap(int size)
{
    mTable = JS_NewHashTable(size, hash_root,
                             JS_CompareValues, JS_CompareValues,
                             nsnull, nsnull);
}

JSObject2WrappedJSMap::~JSObject2WrappedJSMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement Native2WrappedNativeMap...

// static
Native2WrappedNativeMap*
Native2WrappedNativeMap::newMap(int size)
{
    Native2WrappedNativeMap* map = new Native2WrappedNativeMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

Native2WrappedNativeMap::Native2WrappedNativeMap(int size)
{
    mTable = JS_NewHashTable(size, hash_root,
                             JS_CompareValues, JS_CompareValues,
                             nsnull, nsnull);
}

Native2WrappedNativeMap::~Native2WrappedNativeMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement IID2WrappedJSClassMap...

// static
IID2WrappedJSClassMap*
IID2WrappedJSClassMap::newMap(int size)
{
    IID2WrappedJSClassMap* map = new IID2WrappedJSClassMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2WrappedJSClassMap::IID2WrappedJSClassMap(int size)
{
    mTable = JS_NewHashTable(size, hash_IID,
                             compare_IIDs, JS_CompareValues,
                             nsnull, nsnull);
}

IID2WrappedJSClassMap::~IID2WrappedJSClassMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}


/***************************************************************************/
// implement IID2NativeInterfaceMap...

// static
IID2NativeInterfaceMap*
IID2NativeInterfaceMap::newMap(int size)
{
    IID2NativeInterfaceMap* map = new IID2NativeInterfaceMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2NativeInterfaceMap::IID2NativeInterfaceMap(int size)
{
    mTable = JS_NewHashTable(size, hash_IID,
                             compare_IIDs, JS_CompareValues,
                             nsnull, nsnull);
}

IID2NativeInterfaceMap::~IID2NativeInterfaceMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement ClassInfo2NativeSetMap...

// static
ClassInfo2NativeSetMap*
ClassInfo2NativeSetMap::newMap(int size)
{
    ClassInfo2NativeSetMap* map = new ClassInfo2NativeSetMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

ClassInfo2NativeSetMap::ClassInfo2NativeSetMap(int size)
{
    mTable = JS_NewHashTable(size, hash_root,
                             JS_CompareValues, JS_CompareValues,
                             nsnull, nsnull);
}

ClassInfo2NativeSetMap::~ClassInfo2NativeSetMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement ClassInfo2WrappedNativeProtoMap...

// static
ClassInfo2WrappedNativeProtoMap*
ClassInfo2WrappedNativeProtoMap::newMap(int size)
{
    ClassInfo2WrappedNativeProtoMap* map = new ClassInfo2WrappedNativeProtoMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

ClassInfo2WrappedNativeProtoMap::ClassInfo2WrappedNativeProtoMap(int size)
{
    mTable = JS_NewHashTable(size, hash_root,
                             JS_CompareValues, JS_CompareValues,
                             nsnull, nsnull);
}

ClassInfo2WrappedNativeProtoMap::~ClassInfo2WrappedNativeProtoMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement NativeSetMap...

JS_STATIC_DLL_CALLBACK(JSHashNumber)
hash_NativeKey(const void *key)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) key;

    JSHashNumber h = 0;
    
    XPCNativeSet*       Set      = Key->GetBaseSet();
    XPCNativeInterface* Addition = Key->GetAddition();
    PRUint16            Position = Key->GetPosition();
    
    if(!Set)
    {
        NS_ASSERTION(Addition, "bad key");
        h ^= (JSHashNumber) Addition;    
    }
    else
    {
        XPCNativeInterface** Current = Set->GetInterfaceArray();
        PRUint16 count = Set->GetInterfaceCount() + (Addition ? 1 : 0);
        for(PRUint16 i = 0; i < count; i++)
        {
            if(Addition && i == Position)
                h ^= (JSHashNumber) Addition;    
            else
                h ^= (JSHashNumber) *(Current++);
        }   
    }
    
    return h;
}

JS_STATIC_DLL_CALLBACK(intN)
compare_NativeKeyToSet(const void *v1, const void *v2)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) v1;
    
    // See the comment in the XPCNativeSetKey declaration in xpcprivate.h.
    if(Key->IsAKey())
    {
        XPCNativeSet*    SetInTable = (XPCNativeSet*) v2;

        XPCNativeSet*       Set      = Key->GetBaseSet();
        XPCNativeInterface* Addition = Key->GetAddition();
    
        if(!Set)
        {
            return SetInTable->GetInterfaceCount() == 1 &&
                   *SetInTable->GetInterfaceArray() == Addition;
        }

        if(!Addition && Set == SetInTable)
            return 1;
        
        PRUint16 count = Set->GetInterfaceCount() + (Addition ? 1 : 0);
        if(count != SetInTable->GetInterfaceCount())
            return 0;
        
        PRUint16 Position = Key->GetPosition();
        XPCNativeInterface** CurrentInTable = SetInTable->GetInterfaceArray();
        XPCNativeInterface** Current = Set->GetInterfaceArray();
        for(PRUint16 i = 0; i < count; i++)
        {
            if(Addition && i == Position)
            {
                if(Addition != *(CurrentInTable++))
                    return 0;
            }
            else
            {
                if(*(Current++) != *(CurrentInTable++))
                    return 0;
            }
        }   
        
        return 1;
    }

    // else...

    XPCNativeSet* Set1 = (XPCNativeSet*) v1;
    XPCNativeSet* Set2 = (XPCNativeSet*) v2;

    if(Set1 == Set2)
        return 1;
    
    PRUint16 count = Set1->GetInterfaceCount();
    if(count != Set2->GetInterfaceCount())
        return 0;

    XPCNativeInterface** Current1 = Set1->GetInterfaceArray();
    XPCNativeInterface** Current2 = Set2->GetInterfaceArray();
    for(PRUint16 i = 0; i < count; i++)
    {
        if(*(Current1++) != *(Current2++))
            return 0;
    }   
    
    return 1;
}


// static
NativeSetMap*
NativeSetMap::newMap(int size)
{
    NativeSetMap* map = new NativeSetMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

NativeSetMap::NativeSetMap(int size)
{
    mTable = JS_NewHashTable(size, hash_NativeKey,
                             compare_NativeKeyToSet, JS_CompareValues,
                             nsnull, nsnull);
}

NativeSetMap::~NativeSetMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

