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
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "nsSOAPTypeRegistry.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIDOMNodeList.h"
#include "nsDefaultSOAPEncoder.h"

NS_IMPL_ISUPPORTS2(nsSOAPTypeRegistry, nsISOAPTypeRegistry, nsISecurityCheckedComponent)

nsSOAPTypeRegistry::nsSOAPTypeRegistry(): mNativeTypes(new nsSupportsHashtable),
  mSchemaTypes(new nsSupportsHashtable), mDefault(do_GetService(NS_SOAPDEFAULTTYPEREGISTRY_CONTRACTID))
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

  nsDefaultSOAPEncoder::RegisterEncoders(this);
}

nsSOAPTypeRegistry::~nsSOAPTypeRegistry()
{
  /* destructor code */
  delete mNativeTypes;
  delete mSchemaTypes;
}

/* void addNativeType (in AString aType, in nsISOAPEncoder aEncoder); */
NS_IMETHODIMP nsSOAPTypeRegistry::AddNativeType(nsAReadableString & aEncoding, nsAReadableString & aType, nsISOAPEncoder* aEncoder)
{
  NS_ENSURE_ARG_POINTER(aEncoder);
  nsAutoString type(aEncoding);
  type.Append(nsSOAPUtils::kTypeSeparator);
  type.Append(aType);
  nsStringKey typeKey(type);
  if (mNativeTypes->Exists(&typeKey))
  {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  else {
    mNativeTypes->Put(&typeKey, aEncoder);
    return NS_OK;
  }
}

/* void addSchemaType (in AString aType, in nsISOAPDecoder); */
NS_IMETHODIMP nsSOAPTypeRegistry::AddSchemaType(nsAReadableString & aEncoding, nsAReadableString & aType, nsISOAPDecoder* aDecoder)
{
  NS_ENSURE_ARG_POINTER(aDecoder);
  nsAutoString type(aEncoding);
  type.Append(nsSOAPUtils::kTypeSeparator);
  type.Append(aType);
  nsStringKey typeKey(type);
  if (mNativeTypes->Exists(&typeKey))
  {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  else {
    mNativeTypes->Put(&typeKey, aDecoder);
    return NS_OK;
  }
}

/* nsISOAPType queryByNativeType (in DOMString aEncodingStyleURI, in DOMString aNativeType); */
nsresult nsSOAPTypeRegistry::QueryByNativeType(const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aNativeType, nsISOAPEncoder **_retval)
{
  nsAutoString type(aEncodingStyleURI);
  type.Append(nsSOAPUtils::kTypeSeparator);
  type.Append(aNativeType);
  nsStringKey typeKey(type);
  *_retval = NS_STATIC_CAST(nsISOAPEncoder*, mNativeTypes->Get(&typeKey));
  return NS_OK;
}

/* nsISOAPType queryBySchemaType (in DOMString aEncodingStyleURI, in DOMString aSchemaType); */
nsresult nsSOAPTypeRegistry::QueryBySchemaType(const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aSchemaType, nsISOAPDecoder **_retval)
{
  nsAutoString type(aEncodingStyleURI);
  type.Append(nsSOAPUtils::kTypeSeparator);
  type.Append(aSchemaType);
  nsStringKey schemaKey(type);
  *_retval = NS_STATIC_CAST(nsISOAPDecoder*, mSchemaTypes->Get(&schemaKey));
  return NS_OK;
}

NS_IMETHODIMP nsSOAPTypeRegistry::Encode(nsISOAPTypeRegistry *aTypes, nsISOAPParameter *aSource, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aNativeType, nsIDOMNode* aDestination, nsISOAPAttachments* aAttachments)
{
  nsAutoString typeID(aNativeType);
  nsCOMPtr<nsISOAPEncoder> encoder;
  for (;;) {
    QueryByNativeType(aEncodingStyleURI, typeID, getter_AddRefs(encoder));
    if (encoder)
      break;
    mDefault->QueryByNativeType(aEncodingStyleURI, typeID, getter_AddRefs(encoder));
    if (encoder)
      break;
    PRUint32 i = typeID.RFind(nsSOAPUtils::kTypeSeparator);
    if (i < 0)
      return NS_ERROR_NOT_IMPLEMENTED;
    typeID.Left(typeID, i);
  }
  return encoder->Encode(aTypes, aSource, aEncodingStyleURI, aNativeType, aDestination, aAttachments);
}

NS_IMETHODIMP nsSOAPTypeRegistry::Decode(nsISOAPTypeRegistry *aTypes, nsIDOMNode *aSource, const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aSchemaType, nsISOAPAttachments* aAttachments, nsISOAPParameter **_retval)
{
  *_retval = nsnull;
  nsAutoString schemaID(aSchemaType);
  nsCOMPtr<nsISOAPDecoder> decoder;
  for (;;) {
    QueryBySchemaType(aEncodingStyleURI, schemaID, getter_AddRefs(decoder));
    if (decoder)
      break;
    mDefault->QueryBySchemaType(aEncodingStyleURI, schemaID, getter_AddRefs(decoder));
    if (decoder)
      break;
#if 0
    PRUint32 i = schemaID.RFind(nsSOAPUtils::kTypeSeparator);
    if (i < 0)
#endif
      return NS_ERROR_NOT_IMPLEMENTED;
#if 0
    schemaID.Left(schemaID, i);
#endif
  }
  return decoder->Decode(aTypes, aSource, aEncodingStyleURI, aSchemaType, aAttachments, _retval);
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
