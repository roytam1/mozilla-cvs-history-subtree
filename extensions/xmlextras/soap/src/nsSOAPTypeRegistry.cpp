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
#include "nsSOAPMessage.h"
#include "nsSOAPType.h"
#include "nsSOAPTypeRegistry.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIDOMNodeList.h"
#include "nsDefaultSOAPEncoder.h"

NS_IMPL_ISUPPORTS2(nsSOAPTypeRegistry, nsISOAPTypeRegistry, nsISecurityCheckedComponent)

nsSOAPTypeRegistry::nsSOAPTypeRegistry(): mTypeIDs(new nsSupportsHashtable),
  mSchemaIDs(new nsSupportsHashtable), mDefault(do_GetService(NS_SOAPDEFAULTTYPEREGISTRY_CONTRACTID))
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
}

nsSOAPDefaultTypeRegistry::nsSOAPDefaultTypeRegistry()
{
  mDefault = nsnull;
#if 0
  addConfiguration() parse some default configuration;
#endif

  nsCOMPtr<nsISOAPType> type;
  type = new nsSOAPType();
  type->SetEncodingStyleURI(nsSOAPUtils::kSOAPEncodingURI);

  nsDefaultSOAPEncoder* encoder = new nsDefaultSOAPEncoder();
  type->SetMarshaller(encoder);
  type->SetUnmarshaller(encoder);
  AddType(type);
}

nsSOAPTypeRegistry::~nsSOAPTypeRegistry()
{
  /* destructor code */
  delete mTypeIDs;
  delete mSchemaIDs;
}

/* boolean addConfiguration (in nsIDOMElement aConfiguration); */
NS_IMETHODIMP nsSOAPTypeRegistry::AddConfiguration(nsIDOMElement *aConfiguration, PRBool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addType (in nsISOAPType aType); */
NS_IMETHODIMP nsSOAPTypeRegistry::AddType(nsISOAPType *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  nsresult rc;
  nsAutoString typeID;
  nsAutoString schemaID;
  aType->GetTypeID(typeID);
  aType->GetSchemaID(typeID);
  nsStringKey typeKey(typeID);
  nsStringKey schemaKey(schemaID);
  if (mTypeIDs->Exists(&typeKey) ||
    mSchemaIDs->Exists(&schemaKey)) 
  {
    rc = NS_ERROR_ALREADY_INITIALIZED;
  }
  else {
    mTypeIDs->Put(&typeKey, aType);
    if (NS_FAILED(rc))
      return rc;
    mSchemaIDs->Put(&schemaKey, aType);
    if (NS_FAILED(rc)) {
      mTypeIDs->Remove(&typeKey);
    }
  }
  return rc;
}

/* nsISOAPType queryByTypeID (in DOMString aEncodingStyleURI, in DOMString aTypeID); */
NS_IMETHODIMP nsSOAPTypeRegistry::QueryByTypeID(const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aTypeID, nsISOAPType **_retval)
{
  nsStringKey typeKey(aTypeID);
  *_retval = NS_STATIC_CAST(nsISOAPType*, mTypeIDs->Get(&typeKey));
  return NS_OK;
}

/* nsISOAPType queryBySchemaID (in DOMString aEncodingStyleURI, in DOMString aSchemaID); */
NS_IMETHODIMP nsSOAPTypeRegistry::QueryBySchemaID(const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aSchemaID, nsISOAPType **_retval)
{
  nsStringKey schemaKey(aSchemaID);
  *_retval = NS_STATIC_CAST(nsISOAPType*, mSchemaIDs->Get(&schemaKey));
  return NS_OK;
}

/* nsISupports marshall (in nsISOAPMessage aMessage, in nsISupports aSource, in DOMString aEncodingStyleURI, in DOMString aTypeID); */
NS_IMETHODIMP nsSOAPTypeRegistry::Marshall(nsISOAPMessage *aMessage, nsISupports *aSource, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aTypeID, nsISupports **_retval)
{
  *_retval = nsnull;
  nsAutoString typeID(aTypeID);

  nsCOMPtr<nsISOAPType> type;
  nsAutoString schemaID;
  nsCOMPtr<nsIDOMElement> configuration;
  for (;;) {
    QueryByTypeID(aEncodingStyleURI, typeID, getter_AddRefs(type));
    if (type)
      break;
    mDefault->QueryByTypeID(aEncodingStyleURI, typeID, getter_AddRefs(type));
    if (type)
      break;
    PRUint32 i = typeID.RFind(nsSOAPUtils::kTypeSeparator);
    if (i < 0)
      return NS_ERROR_NOT_IMPLEMENTED;
    typeID.Left(typeID, i);
  }
  nsCOMPtr<nsISOAPMarshaller> marshaller;
  type->GetSchemaID(schemaID);
  type->GetMarshallConfiguration(getter_AddRefs(configuration));
  type->GetMarshaller(getter_AddRefs(marshaller));
  if (!marshaller)
    return NS_ERROR_NOT_IMPLEMENTED;
  return marshaller->Marshall(aMessage, aSource, aEncodingStyleURI, aTypeID, schemaID, configuration, _retval);
}

/* nsISupports unmarshall (in nsISOAPMessage aMessage, in nsISupports aSource, in DOMString aEncodingStyleURI, in DOMString aSchemaID); */
NS_IMETHODIMP nsSOAPTypeRegistry::Unmarshall(nsISOAPMessage *aMessage, nsISupports *aSource, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aSchemaID, nsISupports **_retval)
{
  *_retval = nsnull;
  nsAutoString schemaID(aSchemaID);

  nsCOMPtr<nsISOAPType> type;
  nsAutoString typeID;
  nsCOMPtr<nsIDOMElement> configuration;
  for (;;) {
    QueryBySchemaID(aEncodingStyleURI, schemaID, getter_AddRefs(type));
    if (type)
      break;
    mDefault->QueryBySchemaID(aEncodingStyleURI, schemaID, getter_AddRefs(type));
    if (type)
      break;
#if 0
    PRUint32 i = schemaID.RFind(nsSOAPUtils::kTypeSeparator);
    if (i < 0)
#endif
      return NS_ERROR_NOT_IMPLEMENTED;
#if 0
    typeID.Left(schemaID, i);
#endif
  }
  nsCOMPtr<nsISOAPUnmarshaller> unmarshaller;
  type->GetSchemaID(typeID);
  type->GetUnmarshallConfiguration(getter_AddRefs(configuration));
  type->GetUnmarshaller(getter_AddRefs(unmarshaller));
  if (!unmarshaller)
    return NS_ERROR_NOT_IMPLEMENTED;
  return unmarshaller->Unmarshall(aMessage, aSource, aEncodingStyleURI, aSchemaID, typeID, configuration, _retval);
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPTypeRegistry::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPTypeRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPTypeRegistry::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPTypeRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPTypeRegistry::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPTypeRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPTypeRegistry::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPTypeRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
