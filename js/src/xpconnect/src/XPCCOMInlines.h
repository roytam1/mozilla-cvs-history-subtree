/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2002 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   David Bradley <dbradley@netscape.com> (original author)
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

/**
 * \file XPCCOMInlines.h inline implementations
 * Implementations for inline members of classes found in XPCCOMPrivate.h
 */

// XPCCOMObject inlines 
inline
void XPCCOMObject::CleanupVariant(VARIANT & var)
{
    // TODO: we need to more types here
    switch (var.vt & ~VT_BYREF)
    {
        case VT_BSTR:
        {
            ::SysFreeString(*var.pbstrVal);
        }
        break;
    }
}

// XPCCOMIDispatchInterface inlines

inline
XPCCOMIDispatchInterface::Member::ParamInfo::ParamInfo(
	const ELEMDESC * paramInfo) : mParamInfo(paramInfo) 
{
}

inline
JSBool XPCCOMIDispatchInterface::Member::ParamInfo::InitializeOutputParam(
    char * varBuffer, VARIANT & var) const
{
    var.vt = GetType() | VT_BYREF;
    // This is a bit hacky, but we just pick one of the pointer types;
    var.pbstrVal = NS_REINTERPRET_CAST(BSTR*,varBuffer);
    return JS_TRUE;
}

inline
PRBool XPCCOMIDispatchInterface::Member::ParamInfo::IsFlagSet(
	unsigned short flag) const 
{
	return mParamInfo->paramdesc.wParamFlags & flag ? PR_TRUE : PR_FALSE; 
}

inline
PRBool XPCCOMIDispatchInterface::Member::ParamInfo::IsIn() const 
{
	return IsFlagSet(PARAMFLAG_FIN);
}

inline
PRBool XPCCOMIDispatchInterface::Member::ParamInfo::IsOut() const 
{
    return IsFlagSet(PARAMFLAG_FOUT);
}

inline
PRBool XPCCOMIDispatchInterface::Member::ParamInfo::IsOptional() const 
{
    return IsFlagSet(PARAMFLAG_FOPT);
}

inline
PRBool XPCCOMIDispatchInterface::Member::ParamInfo::IsRetVal() const 
{
    return IsFlagSet(PARAMFLAG_FRETVAL);
}

// TODO: Handle VT_ARRAY as well
inline
VARTYPE XPCCOMIDispatchInterface::Member::ParamInfo::GetType() const 
{
    return mParamInfo->tdesc.vt == VT_PTR ? mParamInfo->tdesc.lptdesc->vt : mParamInfo->tdesc.vt;
}

inline
XPCCOMIDispatchInterface::Member::Member() : 
    mType(UNINITIALIZED), mFuncDesc(0),
    mTypeInfo(0) 
{
}

inline
XPCCOMIDispatchInterface::Member::~Member() 
{
    if(mTypeInfo && mFuncDesc) 
        mTypeInfo->ReleaseFuncDesc(mFuncDesc);
}

inline
void* XPCCOMIDispatchInterface::Member::operator new(size_t, Member* p)
{
    return p;
}

inline
void XPCCOMIDispatchInterface::Member::MakeGetter() 
{
    NS_ASSERTION(!IsFunction(), "Can't be function and property"); 
    mType |= GET_PROPERTY; 
}

inline
void XPCCOMIDispatchInterface::Member::MakeSetter() 
{ 
    NS_ASSERTION(!IsFunction(), "Can't be function and property"); 
    mType |= SET_PROPERTY; 
}

inline
void XPCCOMIDispatchInterface::Member::ResetType() 
{
    mType = UNINITIALIZED;
}

inline
void XPCCOMIDispatchInterface::Member::SetFunction() 
{ 
    NS_ASSERTION(!IsProperty(), "Can't be function and property"); 
    mType = FUNCTION; 
}

inline
PRBool XPCCOMIDispatchInterface::Member::IsFlagSet(unsigned short flag) const 
{
    return mType & flag ? PR_TRUE : PR_FALSE; 
}

inline
PRBool XPCCOMIDispatchInterface::Member::IsSetter() const
{
    return IsFlagSet(GET_PROPERTY);
}

inline
PRBool XPCCOMIDispatchInterface::Member::IsGetter() const
{
    return IsFlagSet(SET_PROPERTY);
}

inline
PRBool XPCCOMIDispatchInterface::Member::IsProperty() const
{
    return IsFlagSet(SET_PROPERTY) || IsFlagSet(GET_PROPERTY); 
}

inline
PRBool XPCCOMIDispatchInterface::Member::IsFunction() const
{
    return IsFlagSet(FUNCTION);
}

inline
jsval XPCCOMIDispatchInterface::Member::GetName() const
{
    return mName;
}

inline
void XPCCOMIDispatchInterface::Member::SetName(jsval name)
{
    mName = name;
}

inline
PRUint32 XPCCOMIDispatchInterface::Member::GetDispID() const
{
    return mFuncDesc->memid;
}

inline
PRUint32 XPCCOMIDispatchInterface::Member::GetParamCount() const
{
    return mFuncDesc->cParams;
}

inline
XPCCOMIDispatchInterface::Member::ParamInfo XPCCOMIDispatchInterface::Member::GetParamInfo(PRUint32 index) 
{
    NS_ASSERTION(index < GetParamCount(), "Array bounds error");
    return ParamInfo(mFuncDesc->lprgelemdescParam + index);
}

inline
void XPCCOMIDispatchInterface::Member::SetTypeInfo(DISPID dispID, 
                                                ITypeInfo* pTypeInfo, 
                                                FUNCDESC* funcdesc)
{
    mTypeInfo = pTypeInfo; 
    mFuncDesc = funcdesc;
}

inline
PRUint16 XPCCOMIDispatchInterface::Member::GetParamType(PRUint32 index) const 
{
    return mFuncDesc->lprgelemdescParam[index].paramdesc.wParamFlags; 
}

inline
JSObject* XPCCOMIDispatchInterface::GetJSObject() const
{
    return mJSObject;
}

inline
void XPCCOMIDispatchInterface::SetJSObject(JSObject* jsobj) 
{
    mJSObject = jsobj;
}

inline
XPCCOMIDispatchInterface::Member & XPCCOMIDispatchInterface::GetMember(PRUint32 index)
{ 
    NS_ASSERTION(index < mMemberCount, "invalid index");
    return mMembers[index]; 
}

inline
PRUint32 XPCCOMIDispatchInterface::GetMemberCount() const 
{
    return mMemberCount;
}

inline
void XPCCOMIDispatchInterface::operator delete(void * p) 
{
    free(p);
}

inline
XPCCOMIDispatchInterface::XPCCOMIDispatchInterface(JSContext* cx, JSObject* jsobj, 
                                             ITypeInfo * pTypeInfo, 
                                             PRUint32 members) : 
    mJSObject(jsobj) 
{
    InspectIDispatch(cx, pTypeInfo, members);
}

inline
void * XPCCOMIDispatchInterface::operator new (size_t, PRUint32 members) 
{
    return malloc(sizeof(XPCCOMIDispatchInterface) + sizeof(Member) * (members - 1));
}

// XPCNameArray inlines

inline
XPCNameArray::XPCNameArray() : mCount(0), mNames(0) 
{
}

inline
XPCNameArray::~XPCNameArray() 
{ 
    delete [] mNames;
}

inline
void XPCNameArray::SetSize(PRUint32 size) 
{
    NS_ASSERTION(mCount == 0, "SetSize called more than once");
    mCount = size;
    mNames = (size ? new nsCString[size] : 0);
}

inline
PRUint32 XPCNameArray::GetSize() const 
{
    return mCount;
}

inline
void XPCNameArray::SetName(PRUint32 dispid, nsACString const & name) 
{
    mNames[dispid - 1] = name;
}

inline
const nsACString & XPCNameArray::Get(PRUint32 dispid) const 
{
    return mNames[dispid - 1];
}

inline
PRUint32 XPCNameArray::Find(const nsACString &target) const
{
    for(PRUint32 index = 0; index < mCount; ++index) 
    {
        if(mNames[index] == target) 
            return index + 1; 
    }
    return 0; 
}

// XPCIDArray inlines

inline
PRUint32 XPCIDArray::Length() const
{
    return mIDArray.Count();
}

inline
jsval XPCIDArray::operator [](PRUint32 index) const
{
    return NS_REINTERPRET_CAST(jsval,mIDArray.ElementAt(index));
}

inline
void XPCIDArray::Unmark()
{
    mMarked = JS_FALSE;
}

inline
JSBool XPCIDArray::IsMarked() const
{
    return mMarked;
}

    // NOP. This is just here to make the AutoMarkingPtr code compile.
inline
void XPCIDArray::MarkBeforeJSFinalize(JSContext*) 
{
}

// XPCCOMTypeInfo

inline
FUNCDESC* XPCCOMTypeInfo::FuncDescArray::Get(PRUint32 index) 
{
    return NS_REINTERPRET_CAST(FUNCDESC*,mArray[index]);
}

inline
void XPCCOMTypeInfo::FuncDescArray::Release(FUNCDESC *) 
{
}

inline
PRUint32 XPCCOMTypeInfo::FuncDescArray::Length() const 
{
    return mArray.Count();
}

inline
const nsACString & XPCCOMTypeInfo::GetNameForDispID(DISPID dispID)
{
    return mNameArray.Get(dispID);
}

