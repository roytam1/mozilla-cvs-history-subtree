/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsString.h"
#include "nsSOAPStruct.h"
#include "nsSOAPUtils.h"
#include "nsSupportsArray.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"

NS_IMPL_ISUPPORTS2(nsSOAPStruct, nsISOAPStruct, nsISecurityCheckedComponent)

nsSOAPStruct::nsSOAPStruct(): mMembers(new nsSupportsHashtable)
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
}

nsSOAPStruct::~nsSOAPStruct()
{
  /* destructor code */
  delete mMembers;
}

class nsSOAPStructEnumerator : public nsISimpleEnumerator
{
public:
    NS_DECL_ISUPPORTS

    // nsISimpleEnumerator methods:
    NS_DECL_NSISIMPLEENUMERATOR

    // nsSOAPStructEnumerator methods:

    nsSOAPStructEnumerator(nsSOAPStruct* aStruct);
    virtual ~nsSOAPStructEnumerator();

protected:
  nsCOMPtr<nsSupportsArray> mMembers;
  PRUint32 mCurrent;
};

PRBool StructEnumFunc(nsHashKey *aKey, void *aData, void* aClosure)
{
  nsISupportsArray* members = NS_STATIC_CAST(nsISupportsArray*,aClosure);
  nsISOAPParameter* parameter = NS_STATIC_CAST(nsISOAPParameter*,aData);
  members->AppendElement(parameter);
  return PR_TRUE;
}

nsSOAPStructEnumerator::nsSOAPStructEnumerator(nsSOAPStruct* aStruct)
  : mMembers(new nsSupportsArray()), mCurrent(0)
{
  NS_INIT_REFCNT();
  aStruct->mMembers->Enumerate(&StructEnumFunc, mMembers);
}

nsSOAPStructEnumerator::~nsSOAPStructEnumerator()
{
}

NS_IMPL_ISUPPORTS1(nsSOAPStructEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP nsSOAPStructEnumerator::GetNext(nsISupports** aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  PRUint32 count;
  mMembers->Count(&count);
  if (mCurrent < count) {
      *aItem = mMembers->ElementAt(mCurrent++);
      return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsSOAPStructEnumerator::HasMoreElements(PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  PRUint32 count;
  mMembers->Count(&count);
  *aResult = mCurrent < count;
  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator members; */
NS_IMETHODIMP nsSOAPStruct::GetMembers(nsISimpleEnumerator * *aMembers)
{
  NS_ENSURE_ARG_POINTER(aMembers);
  *aMembers = new nsSOAPStructEnumerator(this);
  if (aMembers) {
    NS_ADDREF(*aMembers);
    return NS_OK;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP nsSOAPStruct::GetMember(const nsAReadableString& aName, nsISOAPParameter* *aMember)
{
  NS_ENSURE_ARG_POINTER(aMember);
  nsStringKey nameKey(aName);
  *aMember = NS_STATIC_CAST(nsISOAPParameter*, mMembers->Get(&nameKey));
  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPStruct::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPStruct::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPStruct::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPStruct::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPStruct))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
