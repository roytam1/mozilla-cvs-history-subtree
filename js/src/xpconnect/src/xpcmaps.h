/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef xpcmaps_h___
#define xpcmaps_h___

// Maps...

// no virtuals in the maps - all the common stuff inlined
// templates could be used to good effect here.

class JSContext2XPCContextMap
{
public:
    static JSContext2XPCContextMap* newMap(int size);

    inline XPCContext* Find(JSContext* cx)
    {
        NS_PRECONDITION(cx,"bad param");
        return (XPCContext*) JS_HashTableLookup(mTable, cx);
    }

    inline XPCContext* Add(XPCContext* xpcc)
    {
        NS_PRECONDITION(xpcc,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, xpcc->GetJSContext(), xpcc);
        return he ? (XPCContext*) he->value : nsnull;
    }

    inline void Remove(XPCContext* xpcc)
    {
        NS_PRECONDITION(xpcc,"bad param");
        JS_HashTableRemove(mTable, xpcc->GetJSContext());
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~JSContext2XPCContextMap();
private:
    JSContext2XPCContextMap();    // no implementation
    JSContext2XPCContextMap(int size);
private:
    JSHashTable *mTable;
};

/*************************/

class JSObject2WrappedJSMap
{
public:
    static JSObject2WrappedJSMap* newMap(int size);

    inline nsXPCWrappedJS* Find(JSObject* Obj)
    {
        NS_PRECONDITION(Obj,"bad param");
        return (nsXPCWrappedJS*) JS_HashTableLookup(mTable, Obj);
    }

    inline nsXPCWrappedJS* Add(nsXPCWrappedJS* Wrapper)
    {
        NS_PRECONDITION(Wrapper,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Wrapper->GetJSObject(), Wrapper);
        return he ? (nsXPCWrappedJS*) he->value : nsnull;
    }

    inline void Remove(nsXPCWrappedJS* Wrapper)
    {
        NS_PRECONDITION(Wrapper,"bad param");
        JS_HashTableRemove(mTable, Wrapper->GetJSObject());
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~JSObject2WrappedJSMap();
private:
    JSObject2WrappedJSMap();    // no implementation
    JSObject2WrappedJSMap(int size);
private:
    JSHashTable *mTable;
};

/*************************/

class Native2WrappedNativeMap
{
public:
    static Native2WrappedNativeMap* newMap(int size);

    inline XPCWrappedNative* Find(nsISupports* Obj)
    {
        NS_PRECONDITION(Obj,"bad param");
        return (XPCWrappedNative*) JS_HashTableLookup(mTable, Obj);
    }

    inline XPCWrappedNative* Add(XPCWrappedNative* Wrapper)
    {
        NS_PRECONDITION(Wrapper,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Wrapper->GetIdentityObject(), Wrapper);
        return he ? (XPCWrappedNative*) he->value : nsnull;
    }

    inline void Remove(XPCWrappedNative* Wrapper)
    {
        NS_PRECONDITION(Wrapper,"bad param");
        JS_HashTableRemove(mTable, Wrapper->GetIdentityObject());
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~Native2WrappedNativeMap();
private:
    Native2WrappedNativeMap();    // no implementation
    Native2WrappedNativeMap(int size);
private:
    JSHashTable *mTable;
};

/*************************/

class IID2WrappedJSClassMap
{
public:
    static IID2WrappedJSClassMap* newMap(int size);

    inline nsXPCWrappedJSClass* Find(REFNSIID iid)
    {
        return (nsXPCWrappedJSClass*) JS_HashTableLookup(mTable, &iid);
    }

    inline nsXPCWrappedJSClass* Add(nsXPCWrappedJSClass* Class)
    {
        NS_PRECONDITION(Class,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, &Class->GetIID(), Class);
        return he ? (nsXPCWrappedJSClass*) he->value : nsnull;
    }

    inline void Remove(nsXPCWrappedJSClass* Class)
    {
        NS_PRECONDITION(Class,"bad param");
        JS_HashTableRemove(mTable, &Class->GetIID());
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~IID2WrappedJSClassMap();
private:
    IID2WrappedJSClassMap();    // no implementation
    IID2WrappedJSClassMap(int size);
private:
    JSHashTable *mTable;
};

/*************************/

class IID2NativeInterfaceMap
{
public:
    static IID2NativeInterfaceMap* newMap(int size);

    inline XPCNativeInterface* Find(REFNSIID iid)
    {
        return (XPCNativeInterface*) JS_HashTableLookup(mTable, &iid);
    }

    inline XPCNativeInterface* Add(XPCNativeInterface* Interface)
    {
        NS_PRECONDITION(Interface,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Interface->GetIID(), Interface);
        return he ? (XPCNativeInterface*) he->value : nsnull;
    }

    inline void Remove(XPCNativeInterface* Interface)
    {
        NS_PRECONDITION(Interface,"bad param");
        JS_HashTableRemove(mTable, Interface->GetIID());
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~IID2NativeInterfaceMap();
private:
    IID2NativeInterfaceMap();    // no implementation
    IID2NativeInterfaceMap(int size);
private:
    JSHashTable*        mTable;
};

/*************************/

class ClassInfo2NativeSetMap
{
public:
    static ClassInfo2NativeSetMap* newMap(int size);

    inline XPCNativeSet* Find(nsIClassInfo* Info)
    {
        return (XPCNativeSet*) JS_HashTableLookup(mTable, Info);
    }

    inline XPCNativeSet* Add(nsIClassInfo* Info, XPCNativeSet* Set)
    {
        NS_PRECONDITION(Info,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Info, Set);
        return he ? (XPCNativeSet*) he->value : nsnull;
    }

    inline void Remove(nsIClassInfo* Info)
    {
        NS_PRECONDITION(Info,"bad param");
        JS_HashTableRemove(mTable, Info);
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~ClassInfo2NativeSetMap();
private:
    ClassInfo2NativeSetMap();    // no implementation
    ClassInfo2NativeSetMap(int size);
private:
    JSHashTable*        mTable;
};

/*************************/

class ClassInfo2WrappedNativeProtoMap
{
public:
    static ClassInfo2WrappedNativeProtoMap* newMap(int size);

    inline XPCWrappedNativeProto* Find(nsIClassInfo* Info)
    {
        return (XPCWrappedNativeProto*) JS_HashTableLookup(mTable, Info);
    }

    inline XPCWrappedNativeProto* Add(nsIClassInfo* Info, XPCWrappedNativeProto* Proto)
    {
        NS_PRECONDITION(Info,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Info, Proto);
        return he ? (XPCWrappedNativeProto*) he->value : nsnull;
    }

    inline void Remove(nsIClassInfo* Info)
    {
        NS_PRECONDITION(Info,"bad param");
        JS_HashTableRemove(mTable, Info);
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~ClassInfo2WrappedNativeProtoMap();
private:
    ClassInfo2WrappedNativeProtoMap();    // no implementation
    ClassInfo2WrappedNativeProtoMap(int size);
private:
    JSHashTable*        mTable;
};

/*************************/

class NativeSetMap
{
public:
    static NativeSetMap* newMap(int size);

    inline XPCNativeSet* Find(XPCNativeSetKey* Key)
    {
        return (XPCNativeSet*) JS_HashTableLookup(mTable, Key);
    }

    inline XPCNativeSet* Add(const XPCNativeSetKey* Key, XPCNativeSet* Set)
    {
        NS_PRECONDITION(Key,"bad param");
        NS_PRECONDITION(Set,"bad param");
        JSHashEntry* he = JS_HashTableAdd(mTable, Key, Set);
        if(!he)
            return nsnull;
        XPCNativeSet* setInTable = (XPCNativeSet*) he->value;
        if(setInTable != Set)
            return setInTable;
        he->key = Set;
        return Set;
    }

    inline XPCNativeSet* Add(XPCNativeSet* Set)
    {
        XPCNativeSetKey Key(Set, nsnull, 0);
        return Add(&Key, Set);
    }

    inline void Remove(XPCNativeSet* Set)
    {
        NS_PRECONDITION(Set,"bad param");

        XPCNativeSetKey Key(Set, nsnull, 0);
        JS_HashTableRemove(mTable, &Key);
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~NativeSetMap();
private:
    NativeSetMap();    // no implementation
    NativeSetMap(int size);
private:
    JSHashTable*        mTable;
};

/***************************************************************************/

class IID2ThisTranslatorMap
{
public:
    static IID2ThisTranslatorMap* newMap(int size);

    inline nsIXPCFunctionThisTranslator* Find(REFNSIID iid)
    {
        return (nsIXPCFunctionThisTranslator*) JS_HashTableLookup(mTable, &iid);
    }

    inline nsIXPCFunctionThisTranslator* Add(REFNSIID iid, 
                                             nsIXPCFunctionThisTranslator* Obj)
    {
        nsID* newID = (nsID*) nsMemory::Clone(&iid, sizeof(nsID));
        NS_IF_ADDREF(Obj);

        JSHashEntry* he = JS_HashTableAdd(mTable, newID, Obj);
        if(!he)
            return nsnull;

        nsIXPCFunctionThisTranslator* current = 
            (nsIXPCFunctionThisTranslator*) he->value;

        // We have recycled the hash entry, our key was not absorbed.
        if(current != Obj)
        {    
            NS_IF_RELEASE(Obj);
            nsMemory::Free(newID);
        }

        return current;
    }

    inline void Remove(REFNSIID iid)
    {
        JS_HashTableRemove(mTable, &iid);
    }

    inline uint32 Count() {return mTable->nentries;}
    inline intN Enumerate(JSHashEnumerator f, void *arg)
        {return JS_HashTableEnumerateEntries(mTable, f, arg);}

    ~IID2ThisTranslatorMap();
private:
    IID2ThisTranslatorMap();    // no implementation
    IID2ThisTranslatorMap(int size);
private:
    JSHashTable *mTable;
};



#endif /* xpcmaps_h___ */
