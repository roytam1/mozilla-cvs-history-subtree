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

#include "xpcprivate.h"

nsID NSID_IDISPATCH = { 0x00020400, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

/* Implementation file */
NS_IMPL_ISUPPORTS1(XPCCOMIDispatchInfo, nsIInterfaceInfo)

XPCCOMIDispatchInfo::XPCCOMIDispatchInfo()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

XPCCOMIDispatchInfo::~XPCCOMIDispatchInfo()
{
  /* destructor code */
}

/* readonly attribute string name; */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetName(char * *aName)
{
    *aName = "IDispatch";
    return NS_OK;
}

/* readonly attribute nsIIDPtr InterfaceIID; */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetInterfaceIID(nsIID * *aInterfaceIID)
{
    *aInterfaceIID = &NSID_IDISPATCH;
    return NS_OK;
}

/* PRBool isScriptable (); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::IsScriptable(PRBool *_retval)
{
    *_retval = PR_TRUE;
    return NS_OK;
}

/* readonly attribute nsIInterfaceInfo parent; */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetParent(nsIInterfaceInfo * *aParent)
{
    *aParent = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute PRUint16 methodCount; */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetMethodCount(PRUint16 *aMethodCount)
{
    *aMethodCount = 0;
    return NS_OK;
}

/* readonly attribute PRUint16 constantCount; */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetConstantCount(PRUint16 *aConstantCount)
{
    *aConstantCount = 0;
    return NS_OK;
}

/* void getMethodInfo (in PRUint16 index, [shared, retval] out nsXPTMethodInfoPtr info); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetMethodInfo(PRUint16 index, const nsXPTMethodInfo * *info)
{
    *info = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getMethodInfoForName (in string methodName, out PRUint16 index, [shared, retval] out nsXPTMethodInfoPtr info); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetMethodInfoForName(const char *methodName, PRUint16 *index,
                                      const nsXPTMethodInfo * *info)
{
    *info = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getConstant (in PRUint16 index, [shared, retval] out nsXPTConstantPtr constant); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetConstant(PRUint16 index, const nsXPTConstant * *constant)
{
    *constant = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIInterfaceInfo getInfoForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetInfoForParam(PRUint16 methodIndex, 
                                               const nsXPTParamInfo * param, 
                                               nsIInterfaceInfo **_retval)
{
    *_retval = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIIDPtr getIIDForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetIIDForParam(PRUint16 methodIndex,
                                const nsXPTParamInfo * param,
                                nsIID * *_retval)
{
    *_retval = nsnull;
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* nsXPTType getTypeForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param, in PRUint16 dimension); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::GetTypeForParam(PRUint16 methodIndex,
                                 const nsXPTParamInfo * param,
                                 PRUint16 dimension, nsXPTType *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint8 getSizeIsArgNumberForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param, in PRUint16 dimension); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::GetSizeIsArgNumberForParam(PRUint16 methodIndex, 
                                            const nsXPTParamInfo * param, 
                                            PRUint16 dimension, 
                                            PRUint8 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint8 getLengthIsArgNumberForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param, in PRUint16 dimension); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::GetLengthIsArgNumberForParam(PRUint16 methodIndex, 
                                              const nsXPTParamInfo * param, 
                                              PRUint16 dimension, 
                                              PRUint8 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRUint8 getInterfaceIsArgNumberForParam (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::GetInterfaceIsArgNumberForParam(PRUint16 methodIndex, 
                                                 const nsXPTParamInfo * param, 
                                                 PRUint8 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRBool isIID (in nsIIDPtr IID); */
NS_IMETHODIMP
XPCCOMIDispatchInfo::IsIID(const nsIID * IID, PRBool *_retval)
{
    *_retval = IID->Equals(NSID_IDISPATCH) ? 
        PR_TRUE : PR_FALSE;
    return NS_OK;
}

/* void getNameShared ([shared, retval] out string name); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::GetNameShared(const char **name)
{
    *name = "IDispatch";
    return NS_OK;
}

/* void getIIDShared ([shared, retval] out nsIIDPtrShared iid); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::GetIIDShared(const nsIID * *iid)
{
    *iid = &NSID_IDISPATCH;
    return NS_OK;
}

/* PRBool isFunction (); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::IsFunction(PRBool *_retval)
{
    *_retval = PR_FALSE;
    return NS_OK;
}

// We need to look into consolidating all these instances
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);

/* PRBool hasAncestor (in nsIIDPtr iid); */
NS_IMETHODIMP 
XPCCOMIDispatchInfo::HasAncestor(const nsIID * iid, PRBool *_retval)
{
    *_retval = iid->Equals(kISupportsIID);
    return NS_OK;
}

/* [notxpcom] nsresult getIIDForParamNoAlloc (in PRUint16 methodIndex, [const] in nsXPTParamInfoPtr param, out nsIID iid); */
NS_IMETHODIMP_(nsresult) 
XPCCOMIDispatchInfo::GetIIDForParamNoAlloc(PRUint16 methodIndex, 
                                       const nsXPTParamInfo * param, 
                                       nsIID *iid)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

