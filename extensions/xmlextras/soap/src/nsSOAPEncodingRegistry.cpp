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
#include "nsISOAPParameter.h"
#include "nsSOAPMessage.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "nsSOAPEncodingRegistry.h"
#include "nsSOAPUtils.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIDOMNodeList.h"
#include "nsDefaultSOAPEncoder.h"
#include "nsISchema.h"

NS_IMPL_ISUPPORTS2(nsSOAPEncodingRegistry, nsISOAPEncodingRegistry, nsISecurityCheckedComponent)

nsSOAPEncodingRegistry::nsSOAPEncodingRegistry(): mNativeTypes(new nsSupportsHashtable),
  mSchemaTypes(new nsSupportsHashtable), mDefault(do_GetService(NS_SOAPDEFAULTENCODINGREGISTRY_CONTRACTID))
{
  NS_INIT_ISUPPORTS();

  /* member initializers and constructor code */
}

nsSOAPDefaultEncodingRegistry::nsSOAPDefaultEncodingRegistry()
{
  mDefault = nsnull;
#if 0
  addConfiguration() parse some default configuration;
#endif

  nsDefaultSOAPEncoder::RegisterEncoders(this);
}

nsSOAPEncodingRegistry::~nsSOAPEncodingRegistry()
{
  /* destructor code */
  delete mNativeTypes;
  delete mSchemaTypes;
}

NS_IMETHODIMP nsSOAPEncodingRegistry::AddEncoder(nsAReadableString & aEncoding, nsAReadableString & aNativeType, nsISOAPEncoder* aEncoder)
{
  NS_ENSURE_ARG_POINTER(aEncoder);
  nsAutoString encoding(aEncoding);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  encoding.Append(aNativeType);
  nsStringKey encodingKey(encoding);
  if (mNativeTypes->Exists(&encodingKey))
  {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  else {
    mNativeTypes->Put(&encodingKey, aEncoder);
    return NS_OK;
  }
}

NS_IMETHODIMP nsSOAPEncodingRegistry::AddDecoder(nsAReadableString & aEncoding, nsAReadableString & aSchemaNamespaceURI, nsAReadableString & aSchemaType, nsISOAPDecoder* aDecoder)
{
  NS_ENSURE_ARG_POINTER(aDecoder);
  nsAutoString encoding(aEncoding);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  encoding.Append(aSchemaNamespaceURI);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  encoding.Append(aSchemaType);
  nsStringKey encodingKey(encoding);
  if (mNativeTypes->Exists(&encodingKey))
  {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  else {
    mNativeTypes->Put(&encodingKey, aDecoder);
    return NS_OK;
  }
}

/* nsISOAPEncoding queryByNativeType (in DOMString aEncodingStyleURI, in DOMString aNativeType); */
nsresult nsSOAPEncodingRegistry::QueryByNativeType(const nsAReadableString & aEncodingStyleURI, const nsAReadableString & aNativeType, nsISOAPEncoder **_retval)
{
  nsAutoString encoding(aEncodingStyleURI);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  encoding.Append(aNativeType);
  nsStringKey encodingKey(encoding);
  *_retval = NS_STATIC_CAST(nsISOAPEncoder*, mNativeTypes->Get(&encodingKey));
  return NS_OK;
}

nsresult nsSOAPEncodingRegistry::QueryBySchemaType(const nsAReadableString & aEncodingStyleURI, nsISchemaType* aSchemaType, nsISOAPDecoder **_retval)
{
  nsresult rc;
  nsAutoString encoding(aEncodingStyleURI);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  nsAutoString type;
  rc = aSchemaType->GetTargetNamespace(type);
  if (NS_FAILED(rc)) return rc;
  encoding.Append(type);
  encoding.Append(nsSOAPUtils::kEncodingSeparator);
  rc = aSchemaType->GetName(type);
  if (NS_FAILED(rc)) return rc;
  encoding.Append(type);
  nsStringKey schemaKey(encoding);
  *_retval = NS_STATIC_CAST(nsISOAPDecoder*, mSchemaTypes->Get(&schemaKey));
  return NS_OK;
}

NS_IMETHODIMP nsSOAPEncodingRegistry::Encode(nsISOAPEncodingRegistry *aEncodings, const nsAReadableString & aEncodingStyleURI, nsISOAPParameter *aSource, nsAReadableString & aNamespaceURI, nsAReadableString & aName, nsISchemaType *aSchemaType, nsIDOMElement* aDestination, nsISOAPAttachments* aAttachments)
{
  nsAutoString nativeType;
  nsresult rc = aSource->GetType(nativeType);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISOAPEncoder> encoder;
  for (;;) {
    if (encoder)
      break;
    mDefault->QueryByNativeType(aEncodingStyleURI, nativeType, getter_AddRefs(encoder));
    if (encoder)
      break;
    PRUint32 i = nativeType.RFind(nsSOAPUtils::kEncodingSeparator);
    if (i < 0)
      return NS_ERROR_NOT_IMPLEMENTED;
    nativeType.Left(nativeType, i);
  }
  return encoder->Encode(aEncodings, aEncodingStyleURI, aSource, aNamespaceURI, aName, aSchemaType, aDestination, aAttachments);
}

NS_IMETHODIMP nsSOAPEncodingRegistry::Decode(nsISOAPEncodingRegistry *aEncodings, const nsAReadableString & aEncodingStyleURI, nsIDOMElement *aSource, nsISchemaType* aSchemaType, nsISOAPAttachments* aAttachments, nsISOAPParameter **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISOAPDecoder> decoder;
  QueryBySchemaType(aEncodingStyleURI, aSchemaType, getter_AddRefs(decoder));
  if (!decoder) {
    mDefault->QueryBySchemaType(aEncodingStyleURI, aSchemaType, getter_AddRefs(decoder));
  }
  if (!decoder)
    return NS_ERROR_NOT_IMPLEMENTED;
  return decoder->Decode(aEncodings, aEncodingStyleURI, aSource, aSchemaType, aAttachments, _retval);
}

static const char* kAllAccess = "AllAccess";

/* string canCreateWrapper (in nsIIDPtr iid); */
NS_IMETHODIMP 
nsSOAPEncodingRegistry::CanCreateWrapper(const nsIID * iid, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncodingRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canCallMethod (in nsIIDPtr iid, in wstring methodName); */
NS_IMETHODIMP 
nsSOAPEncodingRegistry::CanCallMethod(const nsIID * iid, const PRUnichar *methodName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncodingRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canGetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPEncodingRegistry::CanGetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncodingRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}

/* string canSetProperty (in nsIIDPtr iid, in wstring propertyName); */
NS_IMETHODIMP 
nsSOAPEncodingRegistry::CanSetProperty(const nsIID * iid, const PRUnichar *propertyName, char **_retval)
{
  if (iid->Equals(NS_GET_IID(nsISOAPEncodingRegistry))) {
    *_retval = nsCRT::strdup(kAllAccess);
  }

  return NS_OK;
}
