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

#include "nsDefaultSOAPEncoder.h"
#include "nsSOAPUtils.h"
#include "nsSOAPJSValue.h"
#include "nsSOAPParameter.h"
#include "nsISOAPAttachments.h"
#include "nsXPIDLString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMText.h"
#include "nsCOMPtr.h"
#include "nsISchema.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIXPConnect.h"
#include "nsISupportsPrimitives.h"
#include "nsIDOMParser.h"
#include "nsSOAPUtils.h"
#include "nsISOAPEncodingRegistry.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"


NS_NAMED_LITERAL_STRING(kOne,"1");
NS_NAMED_LITERAL_STRING(kZero,"0");

#define DECLARE_ENCODER(name) \
class ns##name##Encoder : \
  public nsISOAPEncoder, \
  public nsISOAPDecoder, \
  public nsDefaultSOAPEncoder \
{\
public:\
  ns##name##Encoder();\
  virtual ~ns##name##Encoder();\
  NS_DECL_ISUPPORTS\
  NS_DECL_NSISOAPENCODER\
  NS_DECL_NSISOAPDECODER\
};\
ns##name##Encoder::ns##name##Encoder() {NS_INIT_ISUPPORTS();}\
ns##name##Encoder::~ns##name##Encoder() {}

#define REGISTER_ENCODER(name) \
{\
  ns##name##Encoder *handler = new ns##name##Encoder();\
  registry->AddEncoder(nsSOAPUtils::kSOAPEncodingURI, nsSOAPUtils::k##name##Type, handler);\
  registry->AddDecoder(nsSOAPUtils::kSOAPEncodingURI, nsSOAPUtils::k##name##SchemaNamespaceURI, nsSOAPUtils::k##name##SchemaType, handler);\
}

// All encoders must be first declared and then registered.
DECLARE_ENCODER(String)
DECLARE_ENCODER(Boolean)
DECLARE_ENCODER(Double)
DECLARE_ENCODER(Float)
DECLARE_ENCODER(Long)
DECLARE_ENCODER(Int)
DECLARE_ENCODER(Short)
DECLARE_ENCODER(Byte)

NS_IMETHODIMP nsDefaultSOAPEncoder::RegisterEncoders(nsISOAPEncodingRegistry* registry)
{
  REGISTER_ENCODER(String)
  REGISTER_ENCODER(Boolean)
  REGISTER_ENCODER(Double)
  REGISTER_ENCODER(Float)
  REGISTER_ENCODER(Long)
  REGISTER_ENCODER(Int)
  REGISTER_ENCODER(Short)
  REGISTER_ENCODER(Byte)
  return NS_OK;
}
//  Here is the implementation of the encoders.

NS_IMETHODIMP nsDefaultSOAPEncoder::EncodeSimpleValue(
		                          const nsAReadableString & aValue, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  
					  const nsAReadableString & aSchemaNamespaceURI, 
					  const nsAReadableString & aSchemaType, 
					  nsIDOMElement* aDestination)
{
  nsCOMPtr<nsIDOMDocument>document;
  nsresult rc = aDestination->GetOwnerDocument(getter_AddRefs(document));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMElement>element;
  rc = document->CreateElementNS(aNamespaceURI,
		                 aName, 
				 getter_AddRefs(element));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMNode>ignore;
  rc = aDestination->AppendChild(element, getter_AddRefs(ignore));
  if (NS_FAILED(rc)) return rc;
  if (!aSchemaType.IsEmpty()) {
    nsAutoString type;
    rc = nsSOAPUtils::MakeNamespacePrefixFixed(element, aSchemaNamespaceURI, type);
    if (NS_FAILED(rc)) return rc;
    type.Append(nsSOAPUtils::kQualifiedSeparator);
    type.Append(aSchemaType);
    element->SetAttributeNS(nsSOAPUtils::kXSIURI, nsSOAPUtils::kXSITypeAttribute, type);
  }
  if (!aValue.IsEmpty()) {
    nsCOMPtr<nsIDOMText> text;
    rc = document->CreateTextNode(aValue, getter_AddRefs(text));
    if (NS_FAILED(rc)) return rc;
    return element->AppendChild(text, getter_AddRefs(ignore));
  }
  return rc;
}

//  String

NS_IMETHODIMP nsStringEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsWString> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  PRUnichar* pointer;
  rc = object->GetData(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  if (aSchemaType) {
    rc = aSchemaType->GetTargetNamespace(schemaNamespaceURI);
    if (NS_FAILED(rc)) return rc;
    rc = aSchemaType->GetName(schemaType);
    if (NS_FAILED(rc)) return rc;
  }
  else {
    schemaType = nsSOAPUtils::kStringSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kStringSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaType,
		       schemaNamespaceURI,
		       aDestination);
}

//  PRBool

NS_IMETHODIMP nsBooleanEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination, 
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsPRBool> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  PRBool b;
  rc = object->GetData(&b);
  if (NS_FAILED(rc)) return rc;
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kBooleanSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kBooleanSchemaNamespaceURI;
  }
  return EncodeSimpleValue(b ? kOne : kZero, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  Double

NS_IMETHODIMP nsDoubleEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsDouble> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kDoubleSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kDoubleSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  Float

NS_IMETHODIMP nsFloatEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsFloat> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kFloatSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kFloatSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  PRInt64

NS_IMETHODIMP nsLongEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsPRInt64> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kLongSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kLongSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  PRInt32

NS_IMETHODIMP nsIntEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsPRInt32> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kIntSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kIntSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  PRInt16

NS_IMETHODIMP nsShortEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsPRInt16> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kShortSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kShortSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

//  Byte

NS_IMETHODIMP nsByteEncoder::Encode(nsISOAPEncodingRegistry* aEncodings,
					  const nsAReadableString & aEncodingStyleURI, 
		                          nsISOAPParameter *aSource, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsIDOMElement* aDestination,
					  nsISOAPAttachments* aAttachments)
{
  nsresult rc;
  nsCOMPtr<nsISupports> value;
  rc = aSource->GetValue(getter_AddRefs(value));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISupportsPRInt16> object = do_QueryInterface(value);
  if (!object) return NS_ERROR_FAILURE;
  char* pointer;
  rc = object->ToString(&pointer);
  if (NS_FAILED(rc)) return rc;
  nsAutoString string;
  string.AssignWithConversion(pointer);// Get the textual representation into string
  nsAutoString schemaType;
  nsAutoString schemaNamespaceURI;
  rc = aSource->GetSchemaNamespaceURI(schemaNamespaceURI);
  if (NS_FAILED(rc)) return rc;
  rc = aSource->GetSchemaType(schemaType);
  if (NS_FAILED(rc)) return rc;
  if (schemaType.IsEmpty()) {
    schemaType = nsSOAPUtils::kByteSchemaType;
    schemaNamespaceURI = nsSOAPUtils::kByteSchemaNamespaceURI;
  }
  return EncodeSimpleValue(string, 
		       aNamespaceURI,
		       aName,
		       schemaNamespaceURI,
		       schemaType,
		       aDestination);
}

/*
Long
Int
Short
Byte
Struct
Literal
Null
Void
Unknown
*/

NS_IMETHODIMP nsStringEncoder::Decode(nsISOAPEncodingRegistry* aEncodings,
					    const nsAReadableString & aEncodingStyleURI, 
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsISOAPParameter **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsISOAPParameter> p = new nsSOAPParameter();
  p->SetAsString(value);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsBooleanEncoder::Decode(nsISOAPEncodingRegistry* aEncodings,
					    const nsAReadableString & aEncodingStyleURI, 
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsISOAPParameter **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  bool b;
  if (value.Equals(nsSOAPUtils::kTrue)
    || value.Equals(nsSOAPUtils::kTrueA)) {
    b = PR_TRUE;
  } else if (value.Equals(nsSOAPUtils::kFalse)
    || value.Equals(nsSOAPUtils::kFalseA)) {
    b = PR_FALSE;
  } else return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsISOAPParameter> p = new nsSOAPParameter();
  p->SetAsBoolean(b);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

#if 0

NS_NAMED_LITERAL_STRING(kXSDStringName, "string");
NS_NAMED_LITERAL_STRING(kXSDBooleanName, "boolean");
NS_NAMED_LITERAL_STRING(kXSDDoubleName, "double");
NS_NAMED_LITERAL_STRING(kXSDFloatName, "float");
NS_NAMED_LITERAL_STRING(kXSDLongName, "long");
NS_NAMED_LITERAL_STRING(kXSDIntName, "int");
NS_NAMED_LITERAL_STRING(kXSDShortName, "short");
NS_NAMED_LITERAL_STRING(kXSDByteName, "byte");
NS_NAMED_LITERAL_STRING(kSOAPEncArrayAttrName, "Array");
NS_NAMED_LITERAL_STRING(kXSDStructName, "complexType");
NS_NAMED_LITERAL_STRING(kXSDUrTypeName, "ur-type");

static void
GetXSDTypeForType(const nsAReadableString & aType, nsAWritableString & aNamespace, nsAWritableString & aLocalName)
{
  if (aType.Equals(nsSOAPUtils::kStringType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDStringName;
  }
  else if (aType.Equals(nsSOAPUtils::kBooleanType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDBooleanName;
  }
  else if (aType.Equals(nsSOAPUtils::kDoubleType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDDoubleName;
  }
  else if (aType.Equals(nsSOAPUtils::kFloatType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDFloatName;
  }
  else if (aType.Equals(nsSOAPUtils::kLongType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDLongName;
  }
  else if (aType.Equals(nsSOAPUtils::kIntType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDIntName;
  }
  else if (aType.Equals(nsSOAPUtils::kShortType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDShortName;
  }
  else if (aType.Equals(nsSOAPUtils::kByteType)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDByteName;
  }
  else if (aType.Equals(nsSOAPUtils::kArrayType)) {
    aNamespace = nsSOAPUtils::kSOAPEncodingURI;
    aLocalName = kSOAPEncArrayAttrName;
  }
  else if (nsSOAPUtils::StartsWith(aType,nsSOAPUtils::kStructTypePrefix)) {
    aNamespace = nsSOAPUtils::kXSDURI;
    aLocalName = kXSDStructName;
  }
  else {
    aNamespace = nsSOAPUtils::kEmpty;
    aLocalName = nsSOAPUtils::kEmpty;
  }
}

static void
GetTypeForXSDType(const nsAReadableString & aNamespace, const nsAReadableString & aLocalName, nsAWritableString & aType)
{
  if (aNamespace.Equals(nsSOAPUtils::kXSDURI)) {
    if (aLocalName.Equals(kXSDStringName))
      aType = nsSOAPUtils::kStringType;
    else if (aLocalName.Equals(kXSDBooleanName))
      aType = nsSOAPUtils::kBooleanType;
    else if (aLocalName.Equals(kXSDDoubleName))
      aType = nsSOAPUtils::kDoubleType;
    else if (aLocalName.Equals(kXSDFloatName))
      aType = nsSOAPUtils::kFloatType;
    else if (aLocalName.Equals(kXSDLongName))
      aType = nsSOAPUtils::kLongType;
    else if (aLocalName.Equals(kXSDIntName))
      aType = nsSOAPUtils::kIntType;
    else if (aLocalName.Equals(kXSDShortName))
      aType = nsSOAPUtils::kShortType;
    else if (aLocalName.Equals(kXSDByteName))
      aType = nsSOAPUtils::kByteType;
    else if (aLocalName.Equals(kXSDStructName))
      aType = nsSOAPUtils::kStructTypePrefix;
    else if (aLocalName.Equals(kXSDUrTypeName))
      aType = nsSOAPUtils::kUnknownType;
    else
      aType = nsSOAPUtils::kNullType;
  }
  else if (aNamespace.Equals(nsSOAPUtils::kSOAPEncodingURI)) {
    if (aLocalName.Equals(kSOAPEncArrayAttrName))
      aType = nsSOAPUtils::kArrayType;
    else
      aType = nsSOAPUtils::kNullType;
  }
}


static const char* kArrayTypeQualifiedName = "SOAP-ENC:arrayType";
static const char* kArrayTypeName = "arrayType";
static const char* kArrayTypeVal = "xsd:ur-type[]";

nsresult
nsDefaultSOAPEncoder::SerializeSupportsArray(nsISupportsArray* array,
                                             nsIDOMElement* element, 
                                             nsIDOMDocument* document)
{
  // Add a SOAP-ENC:arrayType parameter. We always assume it's a
  // heterogeneous array and report a value of "xsd:ur-type[]"
  nsAutoString attrName, attrNS, attrVal;
  attrName.AssignWithConversion(kArrayTypeQualifiedName);
  attrNS.AssignWithConversion(nsSOAPUtils::kSOAPEncodingURI);
  attrVal.AssignWithConversion(kArrayTypeVal);
  element->SetAttributeNS(attrNS, attrName, attrVal);
  
  PRUint32 index, count;
  array->Count(&count);
  
  for (index = 0; index < count; index++) {
    nsCOMPtr<nsISupports> elemisup = getter_AddRefs(array->ElementAt(index));
    nsCOMPtr<nsISOAPParameter> param = do_QueryInterface(elemisup);
    
    if (param) {
      nsCOMPtr<nsIDOMElement> child;
      nsresult rv = EncodeParameter(param, document, getter_AddRefs(child));
      if (NS_FAILED(rv)) return rv;
      
      nsCOMPtr<nsIDOMNode> node;
      element->AppendChild(child, getter_AddRefs(node));
    }
  }

  return NS_OK;
}

nsresult
nsDefaultSOAPEncoder::EncodeParameter(nsISOAPParameter* parameter,
                                      nsIDOMDocument* document,
                                      nsIDOMElement** element)
{
  nsXPIDLCString encodingStyle;

  parameter->GetEncodingStyleURI(getter_Copies(encodingStyle));
  // If the parameter doesn't have its own encoding style or it has
  // one that is the default SOAP encoding style, just call
  // ourself.
  if (!encodingStyle || 
      (nsCRT::strcmp(encodingStyle, nsSOAPUtils::kSOAPEncodingURI))) {
    return ParameterToElement(parameter, nsSOAPUtils::kSOAPEncodingURI,
                              document, element);
  }
  // Otherwise find a new encoder for it
  else {
    // Find the corresponding encoder
    nsCAutoString encoderContractid;
    encoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
    encoderContractid.Append(encodingStyle);

    nsCOMPtr<nsISOAPEncoder> encoder = do_CreateInstance(encoderContractid);
    if (!encoder) return NS_ERROR_INVALID_ARG;
    
    return encoder->ParameterToElement(parameter, encodingStyle,
                                       document, element);
  }
}

nsresult
nsDefaultSOAPEncoder::SerializeJavaScriptArray(JSObject* arrayobj,
                                               nsIDOMElement* element, 
                                               nsIDOMDocument* document)
{
  nsresult rv;
  nsCOMPtr<nsIXPCNativeCallContext> cc;
  NS_WITH_SERVICE(nsIXPConnect, xpc, nsIXPConnect::GetCID(), &rv);
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  // Add a SOAP-ENC:arrayType parameter. We always assume it's a
  // heterogeneous array and report a value of "xsd:ur-type[]"
  nsAutoString attrName, attrNS, attrVal;
  attrName.AssignWithConversion(kArrayTypeQualifiedName);
  attrNS.AssignWithConversion(nsSOAPUtils::kSOAPEncodingURI);
  attrVal.AssignWithConversion(kArrayTypeVal);
  element->SetAttributeNS(attrNS, attrName, attrVal);
  
  JSContext* cx = nsSOAPJSValue::GetSafeContext();
  if (!JS_IsArrayObject(cx, arrayobj)) return NS_ERROR_INVALID_ARG;

  jsuint index, count;
  if (!JS_GetArrayLength(cx, arrayobj, &count)) {
    return NS_ERROR_INVALID_ARG;
  }

  // For each element in the array
  for (index = 0; index < count; index++) {
    nsCOMPtr<nsISOAPParameter> param;
    jsval val;
    if (!JS_GetElement(cx, arrayobj, (jsint)index, &val)) {
      return NS_ERROR_FAILURE;
    }

    // First check if it's a parameter
    if (JSVAL_IS_OBJECT(val)) {
      JSObject* paramobj;
      paramobj = JSVAL_TO_OBJECT(val);

      // Check if it's a wrapped native
      nsCOMPtr<nsIXPConnectWrappedNative> wrapper;
      xpc->GetWrappedNativeOfJSObject(cx, paramobj, getter_AddRefs(wrapper));
      
      if (wrapper) {
        // Get the native and see if it's a SOAPParameter
        nsCOMPtr<nsISupports> native;
        wrapper->GetNative(getter_AddRefs(native));
        if (native) {
          param = do_QueryInterface(native);
        }
      }
    }

    // Otherwise create a new parameter with the value
    if (!param) {
      nsSOAPParameter* newparam = new nsSOAPParameter();
      if (!newparam) return NS_ERROR_OUT_OF_MEMORY;
      
      param = (nsISOAPParameter*)newparam;
      rv = newparam->SetValue(cx, val);
      if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIDOMElement> child;
    rv = EncodeParameter(param, document, getter_AddRefs(child));
    if (NS_FAILED(rv)) return rv;
    
    nsCOMPtr<nsIDOMNode> node;
    element->AppendChild(child, getter_AddRefs(node));
  }

  return NS_OK;
}

nsresult
nsDefaultSOAPEncoder::SerializeJavaScriptObject(JSObject* obj,
                                                nsIDOMElement* element, 
                                                nsIDOMDocument* document)
{
  JSContext* cx = nsSOAPJSValue::GetSafeContext();
  JSIdArray* idarray = JS_Enumerate(cx, obj);

  // Tread lightly here. Only serialize what we believe we can.
  if (idarray) {
    jsint index, count = idarray->length;

    // For each property
    for (index = 0; index < count; index++) {
      nsCOMPtr<nsISOAPParameter> param;
      jsid id = idarray->vector[index];
      jsval idval;
        
      if (JS_IdToValue(cx, id, &idval)) {
        JSString* str = JS_ValueToString(cx, idval);       
        if (!str) continue;
        
        char* name = JS_GetStringBytes(str);
        jsval val;
        
        if (JS_GetProperty(cx, obj, name, &val)) {
          nsSOAPParameter* newparam = new nsSOAPParameter();
          if (!newparam) return NS_ERROR_OUT_OF_MEMORY;
          
          param = (nsISOAPParameter*)newparam;
          nsresult rv = newparam->SetValue(cx, val);
          if (NS_FAILED(rv)) return rv;

          param->SetName(NS_ConvertASCIItoUCS2(name).get());
          
          nsCOMPtr<nsIDOMElement> child;
          rv = EncodeParameter(param, document, getter_AddRefs(child));
          if (NS_FAILED(rv)) return rv;
          
          nsCOMPtr<nsIDOMNode> node;
          element->AppendChild(child, getter_AddRefs(node));
        }
      }
    }
  }

  return NS_OK;
}
  
nsresult
nsDefaultSOAPEncoder::SerializeParameterValue(nsISOAPParameter* parameter, 
                                              nsIDOMElement* element, 
                                              nsIDOMDocument* document)
{
  nsresult rv;
  PRInt32 type;
  nsCOMPtr<nsISupports> isup;
  nsXPIDLString wstringData;
  nsXPIDLCString stringData;
  JSObject* obj;

  parameter->GetType(&type);
  parameter->GetValue(getter_AddRefs(isup));
  parameter->GetJSValue(&obj);

  switch(type) {
    case nsISOAPParameter::PARAMETER_TYPE_NULL:
    case nsISOAPParameter::PARAMETER_TYPE_VOID:
    {
      nsAutoString attrNameStr, attrNameSpace, attrValueStr;
      attrNameStr.AssignWithConversion("xsi:null");
      attrNameSpace.AssignWithConversion(nsSOAPUtils::kXSIURI);
      attrValueStr.AssignWithConversion("true");
      
      element->SetAttributeNS(attrNameSpace, attrNameStr, attrValueStr);
      break;
    }
    
    case nsISOAPParameter::PARAMETER_TYPE_STRING:
    {
      nsCOMPtr<nsISupportsWString> wstr = do_QueryInterface(isup);
      if (wstr) {
        wstr->ToString(getter_Copies(wstringData));
      }
      break;
    }
    
    case nsISOAPParameter::PARAMETER_TYPE_BOOLEAN:
    {
      nsCOMPtr<nsISupportsPRBool> prb = do_QueryInterface(isup);
      if (prb) {
        prb->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_DOUBLE:
    {
      nsCOMPtr<nsISupportsDouble> dub = do_QueryInterface(isup);
      if (dub) {
        dub->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_FLOAT:
    {
      nsCOMPtr<nsISupportsFloat> flt = do_QueryInterface(isup);
      if (flt) {
        flt->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_LONG:
    {
      nsCOMPtr<nsISupportsPRInt64> val64 = do_QueryInterface(isup);
      if (val64) {
        val64->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_INT:
    {
      nsCOMPtr<nsISupportsPRInt32> val32 = do_QueryInterface(isup);
      if (val32) {
        val32->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_SHORT:
    {
      nsCOMPtr<nsISupportsPRInt16> val16 = do_QueryInterface(isup);
      if (val16) {
        val16->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_BYTE:
    {
      nsCOMPtr<nsISupportsPRInt16> val8 = do_QueryInterface(isup);
      if (val8) {
        val8->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_ARRAY:
    {
      nsCOMPtr<nsISupportsArray> array = do_QueryInterface(isup);
      if (!array) return NS_ERROR_INVALID_ARG;

      rv = SerializeSupportsArray(array, element, document);
      if (NS_FAILED(rv)) return rv;

      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_ARRAY:
    {
      rv = SerializeJavaScriptArray(obj, element, document);
      if (NS_FAILED(rv)) return rv;

      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_OBJECT:
    {
      rv = SerializeJavaScriptObject(obj, element, document);
      if (NS_FAILED(rv)) return rv;

      break;
    }
  }

  // Deal with the common case of a single text child
  if (wstringData || stringData) {
    nsCOMPtr<nsIDOMText> text;
    nsAutoString textData;
    if (wstringData) {
      textData.Assign(wstringData);
    }
    else {
      textData.AssignWithConversion(stringData);
    }
    rv = document->CreateTextNode(textData, 
                                  getter_AddRefs(text));
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;
    
    if (text) {
      nsCOMPtr<nsIDOMNode> node;
      element->AppendChild(text, getter_AddRefs(node));
    }
  }

  return NS_OK;
}

/* nsIDOMElement parameterToElement (in nsISOAPParameter parameter, in string encodingStyle, in nsIDOMDocument document); */
NS_IMETHODIMP 
nsDefaultSOAPEncoder::ParameterToElement(nsISOAPParameter *parameter, 
                                         const char *encodingStyle, 
                                         nsIDOMDocument *document, 
                                         nsIDOMElement **_retval)
{
  NS_ENSURE_ARG(parameter);
  NS_ENSURE_ARG(encodingStyle);
  NS_ENSURE_ARG(document);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 type;
  nsXPIDLString elementName, typeAttrVal;
  const char* elementNS = nsnull;
  const char* elementPrefix = nsnull;

  parameter->GetType(&type);
  parameter->GetName(getter_Copies(elementName));
  
  // If it's an unnamed parameter, we use the type names defined
  // by SOAP
  if (!elementName) {
    GetElementNameForType(type, getter_Copies(elementName));
    if (!elementName) return NS_ERROR_INVALID_ARG;
    elementNS = nsSOAPUtils::kSOAPEncodingURI;
    elementPrefix = nsSOAPUtils::kSOAPEncodingPrefix;
  }
  // Else we use the xsi:type attribute
  else {
    GetXSDTypeForType(type, getter_Copies(typeAttrVal));
  }

  // Create the new element
  nsCOMPtr<nsIDOMElement> element;
  nsAutoString elementNameStr(elementName);
  nsAutoString elementNSStr;
  if (elementNS) {
    elementNSStr.AssignWithConversion(elementNS);
  }
  rv = document->CreateElementNS(elementNSStr, elementNameStr, 
                                 getter_AddRefs(element));
  if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

  // Set a suggested prefix, if we have one
  if (elementPrefix) {
    nsAutoString elementPrefixStr;
    elementPrefixStr.AssignWithConversion(elementPrefix);
    element->SetPrefix(elementPrefixStr);
  }

  // Set it's xsi:type if necessary
  if (typeAttrVal) {
    nsAutoString attrNameStr, attrNameSpace, attrValueStr(typeAttrVal);
    attrNameStr.AssignWithConversion("xsi:type");
    attrNameSpace.AssignWithConversion(nsSOAPUtils::kXSIURI);

    element->SetAttributeNS(attrNameSpace, attrNameStr, attrValueStr);
  }

  // If the parameter has its own encodingStyleURI, set that 
  // as an attribute.
  nsXPIDLCString encodingStyleURI;
  parameter->GetEncodingStyleURI(getter_Copies(encodingStyleURI));
  if (encodingStyleURI) {
    nsAutoString encAttr, encAttrNS, encAttrVal;
    encAttr.AssignWithConversion(nsSOAPUtils::kEncodingStyleAttribute);
    encAttrNS.AssignWithConversion(nsSOAPUtils::kSOAPEnvURI);
    encAttrVal.AssignWithConversion(encodingStyleURI);

    element->SetAttributeNS(encAttrNS, encAttr, encAttrVal);
  }

  // Now serialize the parameter's content
  rv = SerializeParameterValue(parameter, element, document);
  if (NS_FAILED(rv)) return rv;

  *_retval = element;
  NS_ADDREF(*_retval);

  return NS_OK;
}

nsresult
nsDefaultSOAPEncoder::DecodeParameter(nsIDOMElement* element,
                                      PRInt32 type,
                                      nsISOAPParameter **_retval)
{
  // If the element doesn't have its own encoding style attribute or 
  // it has one that is the default SOAP encoding style, just call
  // ourself.
  nsAutoString attrNS, attrName, attrVal;
  attrNS.AssignWithConversion(nsSOAPUtils::kSOAPEnvURI);
  attrName.AssignWithConversion(nsSOAPUtils::kEncodingStyleAttribute);
  element->GetAttributeNS(attrNS, attrName, attrVal);

  if ((attrVal.Length() == 0) ||
      (attrVal.EqualsWithConversion(nsSOAPUtils::kSOAPEncodingURI))) {
    return ElementToParameter(element, nsSOAPUtils::kSOAPEncodingURI,
                              type, _retval);
  }
  // Otherwise find a new encoder for it
  else {
    // Find the corresponding encoder
    nsCAutoString encoderContractid;
    encoderContractid.Assign(NS_SOAPENCODER_CONTRACTID_PREFIX);
    encoderContractid.Append(NS_ConvertUCS2toUTF8(attrVal));

    nsCOMPtr<nsISOAPEncoder> encoder = do_CreateInstance(encoderContractid);
    if (!encoder) return NS_ERROR_INVALID_ARG;
    
    return encoder->ElementToParameter(element, 
                                       NS_ConvertUCS2toUTF8(attrVal).get(),
                                       type,
                                       _retval);
  }
}

nsresult
nsDefaultSOAPEncoder::DeserializeSupportsArray(nsIDOMElement *element,
                                               nsISupportsArray **_retval)
{
  nsCOMPtr<nsISupportsArray> array = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
  if (!array) return NS_ERROR_FAILURE;

  nsAutoString attrNS, attrName, attrVal;
  attrNS.AssignWithConversion(nsSOAPUtils::kSOAPEncodingURI);
  attrName.AssignWithConversion(kArrayTypeName);
  element->GetAttributeNS(attrNS, attrName, attrVal);

  // See if the type can be predetermined from the arrayType attribute
  PRInt32 type = nsISOAPParameter::PARAMETER_TYPE_UNKNOWN;
  if (attrVal.Length() > 0) {
    PRInt32 offset = attrVal.Find("[");
    if (-1 != offset) {
      nsAutoString arrayType;
      attrVal.Left(arrayType, offset);
      GetTypeForXSDType(arrayType.GetUnicode(), &type);
    }
  }

  nsCOMPtr<nsIDOMElement> child;
  nsSOAPUtils::GetFirstChildElement(element, getter_AddRefs(child));
  while (child) {
    nsCOMPtr<nsISOAPParameter> param;

    nsresult rv = DecodeParameter(child, type, getter_AddRefs(param));
    if (NS_FAILED(rv)) return rv;

    array->AppendElement(param);

    nsCOMPtr<nsIDOMElement> temp = child;
    nsSOAPUtils::GetNextSiblingElement(temp, getter_AddRefs(child));
  }

  *_retval = array;
  NS_ADDREF(*_retval);

  return NS_OK;
}

nsresult
nsDefaultSOAPEncoder::DeserializeJavaScriptObject(nsIDOMElement *element,
                                                  JSObject** _retval)
{
  nsresult rv;
  JSContext* cx = nsSOAPJSValue::GetSafeContext();
  JSObject* obj = JS_NewObject(cx, nsnull, nsnull, nsnull);
  if (!obj) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> child;
  nsSOAPUtils::GetFirstChildElement(element, getter_AddRefs(child));
  PRInt32 index = 0;
  while (child) {
    nsCOMPtr<nsISOAPParameter> param;
    rv = DecodeParameter(child, 
                         nsISOAPParameter::PARAMETER_TYPE_UNKNOWN,
                         getter_AddRefs(param));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString name;
    nsCOMPtr<nsISupports> value;
    JSObject* jsvalue;
    PRInt32 type;
    param->GetName(getter_Copies(name));
    param->GetValue(getter_AddRefs(value));
    param->GetJSValue(&jsvalue);
    param->GetType(&type);

    jsval val;
    rv = nsSOAPJSValue::ConvertValueToJSVal(cx, value, jsvalue, type, &val);
    if (NS_FAILED(rv)) return rv;

    if (name) {
      JS_SetProperty(cx, obj, NS_ConvertUCS2toUTF8(name).get(), &val);
    }
    else {
      JS_SetElement(cx, obj, (jsint)index, &val);
    }

    nsCOMPtr<nsIDOMElement> temp = child;
    nsSOAPUtils::GetNextSiblingElement(temp, getter_AddRefs(child));
    index++;
  }

  *_retval = obj;

  return NS_OK;
}
                                               
nsresult
nsDefaultSOAPEncoder::DeserializeParameter(nsIDOMElement *element,
                                           PRInt32 type,
                                           nsISOAPParameter **_retval)
{
  nsresult rv;
  nsAutoString text;
  nsCOMPtr<nsISupports> value;
  JSObject* jsvalue = nsnull;
  nsSOAPParameter* param = new nsSOAPParameter();
  if (!param) return NS_ERROR_OUT_OF_MEMORY;

  param->QueryInterface(NS_GET_IID(nsISOAPParameter), (void**)_retval);
  
  switch (type) {
    case nsISOAPParameter::PARAMETER_TYPE_NULL:
    case nsISOAPParameter::PARAMETER_TYPE_VOID:
    {
      param->SetValueAndType(nsnull, type);
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_STRING:
    {
      nsCOMPtr<nsISupportsWString> wstr = do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID);
      if (!wstr) return NS_ERROR_FAILURE;

      nsSOAPUtils::GetElementTextContent(element, text);

      wstr->SetData(text.GetUnicode());     
      value = wstr;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_BOOLEAN:
    {
      nsCOMPtr<nsISupportsPRBool> isupbool = do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);
      if (!isupbool) return NS_ERROR_FAILURE;
      
      nsSOAPUtils::GetElementTextContent(element, text);
      if (text.EqualsWithConversion("false") || 
          text.EqualsWithConversion("0")) {
        isupbool->SetData(PR_FALSE);
      }
      else {
        isupbool->SetData(PR_TRUE);
      }               
      value = isupbool;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_DOUBLE:
    {
      nsCOMPtr<nsISupportsDouble> dub = do_CreateInstance(NS_SUPPORTS_DOUBLE_CONTRACTID);
      if (!dub) return NS_ERROR_FAILURE;
      
      float val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%f", &val);
      
      dub->SetData((double)val);
      value = dub;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_FLOAT:
    {
      nsCOMPtr<nsISupportsFloat> flt = do_CreateInstance(NS_SUPPORTS_FLOAT_CONTRACTID);
      if (!flt) return NS_ERROR_FAILURE;

      float val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%f", &val);
      
      flt->SetData(val);
      value = flt;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_LONG:
    {
      nsCOMPtr<nsISupportsPRInt64> isup64 = do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
      if (!isup64) return NS_ERROR_FAILURE;
      
      PRInt64 val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%lld", &val);
      
      isup64->SetData(val);
      value = isup64;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_INT:
    {
      nsCOMPtr<nsISupportsPRInt32> isup32 = do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID);
      if (!isup32) return NS_ERROR_FAILURE;
      
      PRInt32 val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%ld", &val);
      
      isup32->SetData(val);
      value = isup32;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_SHORT:
    {
      nsCOMPtr<nsISupportsPRInt16> isup16 = do_CreateInstance(NS_SUPPORTS_PRINT16_CONTRACTID);
      if (!isup16) return NS_ERROR_FAILURE;
      
      PRInt16 val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%hd", &val);
      
      isup16->SetData(val);
      value = isup16;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_BYTE:
    {
      nsCOMPtr<nsISupportsPRInt16> isup8 = do_CreateInstance(NS_SUPPORTS_PRInt16_CONTRACTID);
      if (!isup8) return NS_ERROR_FAILURE;
      
      PRInt16 val;
      nsSOAPUtils::GetElementTextContent(element, text);
      PR_sscanf(NS_ConvertUCS2toUTF8(text).get(), "%ld", &val);
      
      isup8->SetData(val);
      value = isup8;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_ARRAY:
    {
      nsCOMPtr<nsISupportsArray> array;
      
      rv = DeserializeSupportsArray(element, getter_AddRefs(array));
      if (NS_FAILED(rv)) return rv;

      value = array;
      break;
    }

    case nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_OBJECT:
    {
      rv = DeserializeJavaScriptObject(element, &jsvalue);
      if (NS_FAILED(rv)) return rv;
      break;
    }
  }

  if (value) {
    param->SetValueAndType(value, type);
  }
  else if (jsvalue) {
    JSContext* cx = nsSOAPJSValue::GetSafeContext();
    param->SetValue(cx, OBJECT_TO_JSVAL(jsvalue));
  }

  return NS_OK;
}

/* nsISOAPParameter elementToParameter (in nsIDOMElement element, in string encodingStyle); */
NS_IMETHODIMP 
nsDefaultSOAPEncoder::ElementToParameter(nsIDOMElement *element, 
                                         const char *encodingStyle, 
                                         PRInt32 hintType,
                                         nsISOAPParameter **_retval)
{
  NS_ENSURE_ARG(element);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 type = hintType;
  nsAutoString attrNS, attrName, attrVal;
  nsAutoString name;
  PRBool useName = PR_TRUE;

  // This will be the name of the parameter
  element->GetLocalName(name);

  if (nsISOAPParameter::PARAMETER_TYPE_UNKNOWN == hintType) {
    // See if the element has a xsi:null attribute
    attrNS.AssignWithConversion(nsSOAPUtils::kXSIURI);
    attrName.AssignWithConversion("null");
    element->GetAttributeNS(attrNS, attrName, attrVal);
    if (attrVal.Length() > 0) {
      type = nsISOAPParameter::PARAMETER_TYPE_NULL;
    }
    // Look for xsi:type attributes to figure out what it is
    else {
      attrName.AssignWithConversion("type");
      element->GetAttributeNS(attrNS, attrName, attrVal);
      if (attrVal.Length() > 0) {
        GetTypeForXSDType(attrVal.GetUnicode(), &type);
      }
      // See if the element name gives us anything
      else {
        nsAutoString ns;
        
        element->GetNamespaceURI(ns);
        
        // If the element namespace is the soap encoding namespace
        // map it back to a type we know
        if (ns.EqualsWithConversion(nsSOAPUtils::kSOAPEncodingURI)) {
          GetTypeForElementName(name.GetUnicode(), &type);
          useName = PR_FALSE;
        }
        // If we still don't know, assume that it is a string if 
        // it has no children and a struct if it does have children
        else if (nsSOAPUtils::HasChildElements(element)) {
          type = nsISOAPParameter::PARAMETER_TYPE_JAVASCRIPT_OBJECT;
        }
        else {
          type = nsISOAPParameter::PARAMETER_TYPE_STRING;
        }
      }
    }
  }

  nsCOMPtr<nsISOAPParameter> parameter;
  rv = DeserializeParameter(element, type, getter_AddRefs(parameter));
  if (NS_FAILED(rv)) return rv;

  if (useName) {
    parameter->SetName(name.GetUnicode());
  }

  *_retval = parameter;
  NS_ADDREF(*_retval);

  return NS_OK;
}

#endif
