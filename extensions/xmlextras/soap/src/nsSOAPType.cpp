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

/* attribute nsISOAPEncoder encoder; */
NS_IMETHODIMP nsSOAPType::GetEncoder(nsISOAPEncoder * *aEncoder)
{
  NS_ENSURE_ARG_POINTER(aEncoder);
  *aEncoder = mEncoder;
  NS_IF_ADDREF(*aEncoder);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetEncoder(nsISOAPEncoder * aEncoder)
{
  mEncoder = aEncoder;
  return NS_OK;
}

/* attribute nsISOAPDecoder decoder; */
NS_IMETHODIMP nsSOAPType::GetDecoder(nsISOAPDecoder * *aDecoder)
{
  NS_ENSURE_ARG_POINTER(aDecoder);
  *aDecoder = mDecoder;
  NS_IF_ADDREF(*aDecoder);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetDecoder(nsISOAPDecoder * aDecoder)
{
  mDecoder = aDecoder;
  return NS_OK;
}

/* attribute nsISupports encodeConfiguration; */
NS_IMETHODIMP nsSOAPType::GetEncodeConfiguration(nsISupports * *aEncodeConfiguration)
{
  NS_ENSURE_ARG_POINTER(aEncodeConfiguration);
  *aEncodeConfiguration = mEncodeConfiguration;
  NS_IF_ADDREF(*aEncodeConfiguration);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetEncodeConfiguration(nsISupports * aEncodeConfiguration)
{
  mEncodeConfiguration = aEncodeConfiguration;
  return NS_OK;
}

/* attribute nsISupports decodeConfiguration; */
NS_IMETHODIMP nsSOAPType::GetDecodeConfiguration(nsISupports * *aDecodeConfiguration)
{
  NS_ENSURE_ARG_POINTER(aDecodeConfiguration);
  *aDecodeConfiguration = mDecodeConfiguration;
  NS_IF_ADDREF(*aDecodeConfiguration);
  return NS_OK;
}
NS_IMETHODIMP nsSOAPType::SetDecodeConfiguration(nsISupports * aDecodeConfiguration)
{
  mDecodeConfiguration = aDecodeConfiguration;
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
