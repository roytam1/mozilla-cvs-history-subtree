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

JS_STATIC_DLL_CALLBACK(intN)
compare_WrappedJS(const void *v1, const void *v2)
{
    nsXPCWrappedJS* Wrapper1 = (nsXPCWrappedJS*) v1;
    nsXPCWrappedJS* Wrapper2 = (nsXPCWrappedJS*) v2;

    return (intN)
        (Wrapper1 == Wrapper2 ||
         Wrapper1->GetJSObject() == Wrapper2->GetJSObject());
}

JS_STATIC_DLL_CALLBACK(intN)
compare_WrappedNative(const void *v1, const void *v2)
{
    XPCWrappedNative* Wrapper1 = (XPCWrappedNative*) v1;
    XPCWrappedNative* Wrapper2 = (XPCWrappedNative*) v2;

    return (intN)
        (Wrapper1 == Wrapper2 ||
         Wrapper1->GetIdentityObject() == Wrapper2->GetIdentityObject());
}

JS_STATIC_DLL_CALLBACK(intN)
compare_WrappedJSClass(const void *v1, const void *v2)
{
    nsXPCWrappedJSClass* Class1 = (nsXPCWrappedJSClass*) v1;
    nsXPCWrappedJSClass* Class2 = (nsXPCWrappedJSClass*) v2;

    return (intN)
        (Class1 == Class2 ||
         Class1->GetIID().Equals(Class2->GetIID()));
}

JS_STATIC_DLL_CALLBACK(intN)
compare_NativeInterface(const void *v1, const void *v2)
{
    XPCNativeInterface* Interface1 = (XPCNativeInterface*) v1;
    XPCNativeInterface* Interface2 = (XPCNativeInterface*) v2;

    return (intN)
        (Interface1 == Interface2 ||
         Interface1->GetIID()->Equals(*Interface2->GetIID()));
}

JS_STATIC_DLL_CALLBACK(intN)
compare_WrappedNativeProto(const void *v1, const void *v2)
{
    XPCWrappedNativeProto* Proto1 = (XPCWrappedNativeProto*) v1;
    XPCWrappedNativeProto* Proto2 = (XPCWrappedNativeProto*) v2;

    return (intN)
        (Proto1 == Proto2 ||
         Proto1->GetClassInfo() == Proto2->GetClassInfo());
}

JS_STATIC_DLL_CALLBACK(intN)
compare_NativeSets(const void *v1, const void *v2)
{
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

JS_STATIC_DLL_CALLBACK(intN)
compare_NativeKeyToSet(const void *v1, const void *v2)
{
    XPCNativeSetKey* Key = (XPCNativeSetKey*) v1;

    // See the comment in the XPCNativeSetKey declaration in xpcprivate.h.
    if(!Key->IsAKey())
        return compare_NativeSets(v1, v2);

    XPCNativeSet*       SetInTable = (XPCNativeSet*) v2;
    XPCNativeSet*       Set        = Key->GetBaseSet();
    XPCNativeInterface* Addition   = Key->GetAddition();

    if(!Set)
    {
        // This is a special case to deal with the invariant that says:
        // "All sets have exactly one nsISupports interface and it comes first."
        // See XPCNativeSet::NewInstance for details.
        //
        // Though we might have a key that represents only one interface, we
        // know that if that one interface were contructed into a set then
        // it would end up really being a set with two interfaces (except for
        // the case where the one interface happened to be nsISupports).

        return (intN)
               ((SetInTable->GetInterfaceCount() == 1 &&
                 SetInTable->GetInterfaceAt(0) == Addition) ||
                (SetInTable->GetInterfaceCount() == 2 &&
                 SetInTable->GetInterfaceAt(1) == Addition));
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
                             JS_CompareValues, compare_WrappedJS,
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
                             JS_CompareValues, compare_WrappedNative,
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
                             compare_IIDs, compare_WrappedJSClass,
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
                             compare_IIDs, compare_NativeInterface,
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
                             JS_CompareValues, compare_NativeSets,
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
                             compare_NativeKeyToSet, compare_NativeSets,
                             nsnull, nsnull);
}

NativeSetMap::~NativeSetMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}

/***************************************************************************/
// implement IID2ThisTranslatorMap...


JS_STATIC_DLL_CALLBACK(void *)
AllocSpace(void *priv, size_t size)
{
    return malloc(size);
}

JS_STATIC_DLL_CALLBACK(void)
FreeSpace(void *priv, void *item)
{
    free(item);
}

JS_STATIC_DLL_CALLBACK(JSHashEntry *)
AllocEntry(void *priv, const void *key)
{
    return (JSHashEntry*) malloc(sizeof(JSHashEntry));
}

JS_STATIC_DLL_CALLBACK(void)
FreeEntry(void *priv, JSHashEntry *he, uintN flag)
{
    nsIXPCFunctionThisTranslator* obj =
        (nsIXPCFunctionThisTranslator*) he->value;
    NS_IF_RELEASE(obj);

    if (flag != HT_FREE_ENTRY)
        return;

    nsMemory::Free((char*)he->key);
    free(he);
}

static JSHashAllocOps IID2ThisTranslatorOps = {
    AllocSpace,    FreeSpace,
    AllocEntry,    FreeEntry
};


// static
IID2ThisTranslatorMap*
IID2ThisTranslatorMap::newMap(int size)
{
    IID2ThisTranslatorMap* map = new IID2ThisTranslatorMap(size);
    if(map && map->mTable)
        return map;
    delete map;
    return nsnull;
}

IID2ThisTranslatorMap::IID2ThisTranslatorMap(int size)
{
    mTable = JS_NewHashTable(size, hash_IID,
                             compare_IIDs, JS_CompareValues,
                             &IID2ThisTranslatorOps, nsnull);
}

IID2ThisTranslatorMap::~IID2ThisTranslatorMap()
{
    if(mTable)
        JS_HashTableDestroy(mTable);
}


