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

#include "nsSOAPType.h"
#include "nsSOAPUtils.h"
#include "nsIDOMNodeList.h"

NS_IMPL_ISUPPORTS2(nsSOAPType, nsISOAPType, nsISecurityCheckedComponent)

nsSOAPType::nsSOAPType()
{
  NS_INIT_ISUPPORTS();
  /* member initializers and constructor code */
}

nsSOAPType::~nsSOAPType()
{
  /* destructor code */
}

/* attribute DOMString encodingStyleURI; */
NS_IMETHODIMP nsSOAPType::GetEncodingStyleURI(nsAWritableString & aEncodingStyleURI)
{
  NS_ENSURE_ARG_POINTER(&aEncodingStyleURI);
  aEncodingStyleURI.Assign(mEncodingStyleURI);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetEncodingStyleURI(const nsAReadableString & aEncodingStyleURI)
{
  mEncodingStyleURI.Assign(aEncodingStyleURI);
  return NS_OK;
}

/* attribute DOMString typeID; */
NS_IMETHODIMP nsSOAPType::GetTypeID(nsAWritableString & aTypeID)
{
  NS_ENSURE_ARG_POINTER(&aTypeID);
  aTypeID.Assign(mTypeID);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetTypeID(const nsAReadableString & aTypeID)
{
  mTypeID.Assign(aTypeID);
  return NS_OK;
}

/* attribute DOMString schemaID; */
NS_IMETHODIMP nsSOAPType::GetSchemaID(nsAWritableString & aSchemaID)
{
  NS_ENSURE_ARG_POINTER(&aSchemaID);
  aSchemaID.Assign(mSchemaID);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetSchemaID(const nsAReadableString & aSchemaID)
{
  mSchemaID.Assign(aSchemaID);
  return NS_OK;
}

/* attribute nsISOAPMarshaller marshaller; */
NS_IMETHODIMP nsSOAPType::GetMarshaller(nsISOAPMarshaller * *aMarshaller)
{
  NS_ENSURE_ARG_POINTER(aMarshaller);
  *aMarshaller = mMarshaller;
  NS_IF_ADDREF(*aMarshaller);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetMarshaller(nsISOAPMarshaller * aMarshaller)
{
  mMarshaller = aMarshaller;
  return NS_OK;
}

/* attribute nsISOAPUnmarshaller unmarshaller; */
NS_IMETHODIMP nsSOAPType::GetUnmarshaller(nsISOAPUnmarshaller * *aUnmarshaller)
{
  NS_ENSURE_ARG_POINTER(aUnmarshaller);
  *aUnmarshaller = mUnmarshaller;
  NS_IF_ADDREF(*aUnmarshaller);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetUnmarshaller(nsISOAPUnmarshaller * aUnmarshaller)
{
  mUnmarshaller = aUnmarshaller;
  return NS_OK;
}

/* attribute nsISupports marshallConfiguration; */
NS_IMETHODIMP nsSOAPType::GetMarshallConfiguration(nsISupports * *aMarshallConfiguration)
{
  NS_ENSURE_ARG_POINTER(aMarshallConfiguration);
  *aMarshallConfiguration = mMarshallConfiguration;
  NS_IF_ADDREF(*aMarshallConfiguration);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetMarshallConfiguration(nsISupports * aMarshallConfiguration)
{
  mMarshallConfiguration = aMarshallConfiguration;
  return NS_OK;
}

/* attribute nsISupports unmarshallConfiguration; */
NS_IMETHODIMP nsSOAPType::GetUnmarshallConfiguration(nsISupports * *aUnmarshallConfiguration)
{
  NS_ENSURE_ARG_POINTER(aUnmarshallConfiguration);
  *aUnmarshallConfiguration = mUnmarshallConfiguration;
  NS_IF_ADDREF(*aUnmarshallConfiguration);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetUnmarshallConfiguration(nsISupports * aUnmarshallConfiguration)
{
  mUnmarshallConfiguration = aUnmarshallConfiguration;
  return NS_OK;
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPType::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPType))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPType::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPType))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPType::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPType))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPType::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPType))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
