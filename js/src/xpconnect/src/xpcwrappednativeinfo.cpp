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

/* new flattening stuff. */

#include "xpcprivate.h"

/***************************************************************************/
// XPCNativeMember

// static
XPCCallableInfo*
XPCNativeMember::GetCallableInfo(XPCCallContext& ccx, JSObject* funobj)
{
    JSContext* cx;
    JSFunction* fun;
    JSObject* realFunObj;
    jsid id;
    jsval val;

    // We expect funobj to be a clone, we need the real funobj.

    cx = ccx.GetJSContext();
    fun = (JSFunction*) JS_GetPrivate(cx, funobj);
    realFunObj = JS_GetFunctionObject(fun);
    id = ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_CALLABLE_INFO_PROP_NAME);

    if(OBJ_GET_PROPERTY(cx, realFunObj, id, &val) && JSVAL_IS_INT(val))
        return (XPCCallableInfo*) JSVAL_TO_PRIVATE(val);

    return nsnull;
}

void
XPCNativeMember::CleanupCallableInfo(XPCCallContext& ccx, JSObject* funobj)
{
    JSContext* cx;
    jsid id;
    jsval val;

    // We know this must be the *real* function object - not a clone.

    cx = ccx.GetJSContext();
    id = ccx.GetRuntime()->GetStringID(XPCJSRuntime::IDX_CALLABLE_INFO_PROP_NAME);

    if(OBJ_GET_PROPERTY(cx, funobj, id, &val) && JSVAL_IS_INT(val))
        delete ((XPCCallableInfo*) JSVAL_TO_PRIVATE(val));
}

JSBool
XPCNativeMember::Resolve(XPCCallContext& ccx, XPCNativeInterface* iface)
{
    // XXX locking!

    // XXX Is this necessary?  can we ever get here w/o being in a request?
    AutoJSRequest req(ccx); // scoped JS Request

    if(IsConstant())
    {
        const nsXPTConstant* constant;
        if(NS_FAILED(iface->GetInterfaceInfo()->GetConstant(mIndex, &constant)))
            return JS_FALSE;

        const nsXPTCMiniVariant& mv = *constant->GetValue();

        // XXX Big Hack!
        nsXPTCVariant v;
        v.flags = 0;
        v.type = constant->GetType();
        memcpy(&v.val, &mv.val, sizeof(mv.val));

        jsval old_val = mVal;

        if(!XPCConvert::NativeData2JS(ccx, &mVal, &v.val, v.type,
                                      nsnull, nsnull, nsnull))
            return JS_FALSE;

        mFlags |= RESOLVED;

        return JS_TRUE;
    }
    // else...

    // This is a method or attribute - we'll be needing a function object

    // We need to use the safe context for this thread because we don't want
    // to parent the new (and cached forever!) function object to the current
    // JSContext's global object. That would be bad!

    JSContext* cx = ccx.GetSafeJSContext();
    if(!cx)
        return JS_FALSE;

    intN argc;
    intN flags;
    JSNative callback;

    if(IsMethod())
    {
        const nsXPTMethodInfo* info;
        if(NS_FAILED(iface->GetInterfaceInfo()->GetMethodInfo(mIndex, &info)))
            return JS_FALSE;

        // XXX ASSUMES that retval is last arg.
        argc = (intN) info->GetParamCount();
        if(argc && info->GetParam((uint8)(argc-1)).IsRetval())
            argc-- ;

        flags = 0;
        callback = XPC_WN_CallMethod;
    }
    else
    {
        if(IsWritableAttribute())
        {
            flags = JSFUN_GETTER | JSFUN_SETTER;
            argc = 1;
        }
        else
        {
            flags = JSFUN_GETTER;
            argc = 0;
        }
        callback = XPC_WN_GetterSetter;
    }

    JSFunction *fun = JS_NewFunction(cx, callback, argc, flags, nsnull,
                                     iface->GetMemberName(ccx, this));
    if(!fun)
        return JS_FALSE;

    JSObject* funobj = JS_GetFunctionObject(fun);
    if(!funobj)
        return JS_FALSE;

    mVal = OBJECT_TO_JSVAL(funobj);
    jsid id = ccx.GetRuntime()->
        GetStringID(XPCJSRuntime::IDX_CALLABLE_INFO_PROP_NAME);

    XPCCallableInfo* ci = new XPCCallableInfo(iface, this);

    if(!ci)
        return JS_FALSE;

    if(!OBJ_DEFINE_PROPERTY(cx, funobj, id, PRIVATE_TO_JSVAL(ci),
                            nsnull, nsnull,
                            JSPROP_READONLY|JSPROP_PERMANENT, nsnull))
    {
        delete ci;
        return JS_FALSE;
    }

    mFlags |= RESOLVED;
    
    return JS_TRUE;
}


void
XPCNativeMember::Cleanup(XPCCallContext& ccx)
{
    if(IsResolved() && !JSVAL_IS_PRIMITIVE(mVal))
        CleanupCallableInfo(ccx, JSVAL_TO_OBJECT(mVal));
}

/***************************************************************************/
// XPCNativeInterface

// static
XPCNativeInterface*
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid)
{
    IID2NativeInterfaceMap* map = ccx.GetRuntime()->GetIID2NativeInterfaceMap();
    if(!map)
        return nsnull;

    // XXX add locking...

    XPCNativeInterface* iface = map->Find(*iid);
    if(iface)
        return iface;

    nsCOMPtr<nsIInterfaceInfoManager> iimgr =
        dont_AddRef(nsXPConnect::GetInterfaceInfoManager());
    if(!iimgr)
        return nsnull;

    nsCOMPtr<nsIInterfaceInfo> info;
    if(NS_FAILED(iimgr->GetInfoForIID(iid, getter_AddRefs(info))) ||!info)
        return nsnull;

    iface = NewInstance(ccx, info);
    if(!iface)
        return nsnull;

    map->Add(iface);

    return iface;
}

// static
XPCNativeInterface*
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, nsIInterfaceInfo* info)
{
    IID2NativeInterfaceMap* map = ccx.GetRuntime()->GetIID2NativeInterfaceMap();
    if(!map)
        return nsnull;

    const nsIID* iid;
    if(NS_FAILED(info->GetIIDShared(&iid)) || !iid)
        return nsnull;

    // XXX add locking...

    XPCNativeInterface* iface = map->Find(*iid);
    if(iface)
        return iface;

    iface = NewInstance(ccx, info);
    if(!iface)
        return nsnull;

    map->Add(iface);

    return iface;
}

// static 
XPCNativeInterface* 
XPCNativeInterface::GetNewOrUsed(XPCCallContext& ccx, const char* name)
{
    nsCOMPtr<nsIInterfaceInfoManager> iimgr =
        dont_AddRef(nsXPConnect::GetInterfaceInfoManager());
    if(!iimgr)
        return nsnull;

    nsCOMPtr<nsIInterfaceInfo> info;
    if(NS_FAILED(iimgr->GetInfoForName(name, getter_AddRefs(info))) || !info)
        return nsnull;
                
    return GetNewOrUsed(ccx, info);
}

XPCNativeInterface::XPCNativeInterface(nsIInterfaceInfo* aInfo, jsval aName)
    : nsIXPCNativeInterface(aInfo, aName)
{
}

// static
XPCNativeInterface*
XPCNativeInterface::NewInstance(XPCCallContext& ccx,
                                nsIInterfaceInfo* aInfo)
{
    static const PRUint16 MAX_LOCAL_MEMBER_COUNT = 16;
    XPCNativeMember local_members[MAX_LOCAL_MEMBER_COUNT];
    XPCNativeInterface* obj = nsnull;
    XPCNativeMember* members = nsnull;

    int i;
    JSBool failed = JS_FALSE;
    PRUint16 constCount;
    PRUint16 methodCount;
    PRUint16 totalCount;
    PRUint16 realTotalCount = 0;
    XPCNativeMember* cur;
    JSString*  str;
    jsval name;
    jsval interfaceName;


    // XXX Investigate lazy init? This is a problem given the
    // 'placement new' scheme - we need to at least know how big to make
    // the object. We might do a scan of methods to determine needed size,
    // then make our object, but avoid init'ing *any* members until asked?
    // Find out how often we create these objects w/o really looking at
    // (or using) the members.

    JSContext* cx = ccx.GetJSContext();

    if(!cx)
        return nsnull;

    if(NS_FAILED(aInfo->GetMethodCount(&methodCount)) ||
       NS_FAILED(aInfo->GetConstantCount(&constCount)))
        return nsnull;

    totalCount = methodCount + constCount;

    if(totalCount > MAX_LOCAL_MEMBER_COUNT)
    {
        members = new XPCNativeMember[totalCount];
        if(!members)
            return nsnull;
    }
    else
    {
        members = local_members;
    }

    // XXX since getters and setters share a member, we might not use all
    // of the member objects.

    for(i = 0; i < methodCount; i++)
    {
        const nsXPTMethodInfo* info;
        if(NS_FAILED(aInfo->GetMethodInfo(i, &info)))
        {
            failed = JS_TRUE;
            break;
        }

        // don't reflect Addref or Release
        if(i == 1 || i == 2)
            continue;

        if(!XPCConvert::IsMethodReflectable(*info))
            continue;

        str = JS_InternString(cx, info->GetName());
        if(!str)
        {
            NS_ASSERTION(0,"bad method name");
            failed = JS_TRUE;
            break;
        }
        name = STRING_TO_JSVAL(str);
        
        if(info->IsSetter())
        {
            NS_ASSERTION(realTotalCount,"bad setter");
            // XXX ASSUMES Getter/Setter pairs are next to each other
            cur = &members[realTotalCount-1];
            NS_ASSERTION(cur->GetName() == name,"bad setter");
            NS_ASSERTION(cur->IsReadOnlyAttribute(),"bad setter");
            NS_ASSERTION(cur->GetIndex() == i-1,"bad setter");
            cur->SetWritableAttribute();
        }
        else
        {
            // XXX need better way to find dups
            // NS_ASSERTION(!LookupMemberByID(name),"duplicate method name");
            cur = &members[realTotalCount++];
            cur->SetName(name);
            if(info->IsGetter())
                cur->SetReadOnlyAttribute(i);
            else
                cur->SetMethod(i);
        }
    }

    if(!failed)
    {
        for(i = 0; i < constCount; i++)
        {
            const nsXPTConstant* constant;
            if(NS_FAILED(aInfo->GetConstant(i, &constant)))
            {
                failed = JS_TRUE;
                break;
            }

            str = JS_InternString(cx, constant->GetName());
            if(!str)
            {
                NS_ASSERTION(0,"bad constant name");
                failed = JS_TRUE;
                break;
            }
            name = STRING_TO_JSVAL(str);

            // XXX need better way to find dups
            //NS_ASSERTION(!LookupMemberByID(name),"duplicate method/constant name");

            cur = &members[realTotalCount++];
            cur->SetName(name);
            cur->SetConstant(i);
        }
    }

    if(!failed)
    {
        const char* bytes;
        if(NS_FAILED(aInfo->GetNameShared(&bytes)) || !bytes ||
           nsnull == (str = JS_InternString(cx, bytes)))
        {
            failed = JS_TRUE;
        }
        interfaceName = STRING_TO_JSVAL(str);
    }



    if(!failed)
    {
        // Use placement new to create an object with the right amount of space
        // to hold the members array
        int size = sizeof(XPCNativeInterface);
        if(realTotalCount > 1)
            size += (realTotalCount - 1) * sizeof(XPCNativeMember);
        void* place = new char[size];
        if(place)
            obj = new(place) XPCNativeInterface(aInfo, interfaceName);

        if(obj)
        {
            obj->mMemberCount = realTotalCount;
            // copy valid members
            if(realTotalCount)
                memcpy(obj->mMembers, members,
                       realTotalCount * sizeof(XPCNativeMember));
        }
    }

    if(members && members != local_members)
        delete [] members;

    return obj;
}

// static
void
XPCNativeInterface::DestroyInstance(XPCCallContext& ccx, XPCNativeInterface* inst)
{
    int count = (int) inst->mMemberCount;
    XPCNativeMember* cur = inst->mMembers;
    for(int i = 0; i < count; i++, cur++)
        cur->Cleanup(ccx);

    inst->~XPCNativeInterface();
    delete [] (char*) inst;
}

const char*
XPCNativeInterface::GetMemberName(XPCCallContext& ccx,
                                  const XPCNativeMember* member) const
{
    return JS_GetStringBytes(JSVAL_TO_STRING(member->GetName()));
}

void 
XPCNativeInterface::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCNativeInterface @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("name is %s", GetNameString()));
        XPC_LOG_ALWAYS(("mMemberCount is %d", mMemberCount));
        XPC_LOG_ALWAYS(("mInfo @ %x", mInfo.get()));
        XPC_LOG_OUTDENT();
#endif
}

/***************************************************************************/
// XPCNativeSet


// static
XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx, const nsIID* iid)
{
    XPCNativeInterface* iface = XPCNativeInterface::GetNewOrUsed(ccx, iid);
    if(!iface)
        return nsnull;

    NativeSetMap* map = ccx.GetRuntime()->GetNativeSetMap();
    if(!map)
        return nsnull;

    // XXX add locking...

    XPCNativeSetKey key(nsnull, iface, 0);

    XPCNativeSet* set = map->Find(&key);
    if(set)
        return set;

    set = NewInstance(&iface, 1);
    map->Add(&key, set);
    return set;
}

// static
XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx, nsIClassInfo* classInfo)
{
    ClassInfo2NativeSetMap* map = ccx.GetRuntime()->GetClassInfo2NativeSetMap();
    if(!map)
        return nsnull;

    // XXX add locking...

    XPCNativeSet* set = map->Find(classInfo);
    if(set)
        return set;

    nsIID** iidArray = nsnull;
    XPCNativeInterface** interfaceArray = nsnull;
    PRUint32 iidCount = 0;

    if(NS_FAILED(classInfo->GetInterfaces(&iidCount, &iidArray)))
        return nsnull;

    NS_ASSERTION((iidCount && iidArray) || !(iidCount || iidArray), "GetInterfaces returned bad array");

    // !!! from here on we only exit through the 'out' label !!!

    if(iidCount)
    {
        interfaceArray = new XPCNativeInterface*[iidCount];
        if(!interfaceArray)
            goto out;

        XPCNativeInterface** currentInterface = interfaceArray;
        nsIID**              currentIID = iidArray;
        PRUint16             interfaceCount = 0;

        for(PRUint32 i = 0; i < iidCount; i++)
        {
            nsIID* iid = *(currentIID++);

            XPCNativeInterface* iface =
                XPCNativeInterface::GetNewOrUsed(ccx, iid);

            if(!iface)
            {
                // XXX warn here
                continue;
            }

            *(currentInterface++) = iface;
            interfaceCount++;
        }

        if(interfaceCount)
        {
            set = NewInstance(interfaceArray, interfaceCount);
            if(set)
            {
                NativeSetMap* map2 = ccx.GetRuntime()->GetNativeSetMap();
                if(!map2)
                    goto out;

                // XXX add locking...

                XPCNativeSetKey key(set, nsnull, 0);

                XPCNativeSet* oldSet = map2->Find(&key);
                if(oldSet)
                {
                    DestroyInstance(set);
                    set = oldSet;
                }
                else
                {
                    map2->Add(&key, set);
                }
            }
        }
        else
            set = GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));
    }
    else
        set = GetNewOrUsed(ccx, &NS_GET_IID(nsISupports));

    if(set)
        map->Add(classInfo, set);

out:
    if(iidArray)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(iidCount, iidArray);
    if(interfaceArray)
        delete [] interfaceArray;

    return set;
}

// static
XPCNativeSet*
XPCNativeSet::GetNewOrUsed(XPCCallContext& ccx,
                           XPCNativeSet* otherSet,
                           XPCNativeInterface* newInterface,
                           PRUint16 position)
{
    NativeSetMap* map = ccx.GetRuntime()->GetNativeSetMap();
    if(!map)
        return nsnull;

    // XXX add locking...

    XPCNativeSetKey key(otherSet, newInterface, position);

    XPCNativeSet* set = map->Find(&key);
    if(set)
        return set;

    if(otherSet)
        set = NewInstanceMutate(otherSet, newInterface, position);
    else
        set = NewInstance(&newInterface, 1);

    map->Add(&key, set);
    return set;
}

// static
XPCNativeSet*
XPCNativeSet::NewInstance(XPCNativeInterface** array, PRUint16 count)
{
    XPCNativeSet* obj = nsnull;

    if(!array || !count)
        return nsnull;

    // Use placement new to create an object with the right amount of space
    // to hold the members array
    int size = sizeof(XPCNativeSet);
    if(count > 1)
        size += (count - 1) * sizeof(XPCNativeInterface*);
    void* place = new char[size];
    if(place)
        obj = new(place) XPCNativeSet();

    if(obj)
    {
        obj->mMemberCount = 0;
        for(PRUint16 i = 0; i < count; i++)
            obj->mMemberCount += array[i]->GetMemberCount();
        obj->mInterfaceCount = count;
        memcpy(obj->mInterfaces, array, count * sizeof(XPCNativeInterface*));
    }

    return obj;
}

// static
XPCNativeSet*
XPCNativeSet::NewInstanceMutate(XPCNativeSet*       otherSet,
                                XPCNativeInterface* newInterface,
                                PRUint16            position)
{
    XPCNativeSet* obj = nsnull;

    if(!newInterface)
        return nsnull;
    if(otherSet && position > otherSet->mInterfaceCount)
        return nsnull;

    // Use placement new to create an object with the right amount of space
    // to hold the members array
    int size = sizeof(XPCNativeSet);
    if(otherSet)
        size += otherSet->mInterfaceCount * sizeof(XPCNativeInterface*);
    void* place = new char[size];
    if(place)
        obj = new(place) XPCNativeSet();

    if(obj)
    {
        if(otherSet)
        {
            obj->mMemberCount = otherSet->GetMemberCount() +
                                newInterface->GetMemberCount();
            obj->mInterfaceCount = otherSet->mInterfaceCount + 1;

            XPCNativeInterface** src = otherSet->mInterfaces;
            XPCNativeInterface** dest = obj->mInterfaces;
            for(PRUint16 i = 0; i < obj->mInterfaceCount; i++)
            {
                if(i == position)
                    *dest++ = newInterface;
                else
                    *dest++ = *src++;
            }
        }
        else
        {
            obj->mMemberCount = newInterface->GetMemberCount();
            obj->mInterfaceCount = 1;
            obj->mInterfaces[0] = newInterface;
        }
    }

    return obj;
}

// static
void
XPCNativeSet::DestroyInstance(XPCNativeSet* inst)
{
    inst->~XPCNativeSet();
    delete [] (char*) inst;
}

void 
XPCNativeSet::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCNativeSet @ %x", this));
        XPC_LOG_INDENT();
        
        XPC_LOG_ALWAYS(("mInterfaceCount of %d", mInterfaceCount));
        if(depth)
        {
            for(PRUint16 i = 0; i < mInterfaceCount; i++)
                mInterfaces[i]->DebugDump(depth);            
        }
        XPC_LOG_ALWAYS(("mMemberCount of %d", mMemberCount));
        XPC_LOG_OUTDENT();
#endif
}

