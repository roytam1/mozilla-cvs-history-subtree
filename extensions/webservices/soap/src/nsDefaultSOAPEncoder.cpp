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
#include "nsISOAPEncoding.h"
#include "nsISOAPEncoder.h"
#include "nsISOAPDecoder.h"
#include "prprf.h"
#include "prdtoa.h"
#include "nsReadableUtils.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"

NS_NAMED_LITERAL_STRING(kOne,"1");
NS_NAMED_LITERAL_STRING(kZero,"0");

NS_NAMED_LITERAL_STRING(kEmpty,"");

NS_NAMED_LITERAL_STRING(kSchemaNamespaceURI,"http://www.w3.org/2001/XMLSchema");
NS_NAMED_LITERAL_STRING(kSchemaDatatypesNamespaceURI,"http://www.w3.org/2001/XMLSchema-datatypes");

NS_NAMED_LITERAL_STRING(kAnyTypeSchemaType, "anyType");
NS_NAMED_LITERAL_STRING(kAnySimpleTypeSchemaType, "anySimpleType");
NS_NAMED_LITERAL_STRING(kArraySOAPType, "Array");

NS_NAMED_LITERAL_STRING(kStringSchemaType, "string");
NS_NAMED_LITERAL_STRING(kBooleanSchemaType, "boolean");
NS_NAMED_LITERAL_STRING(kDecimalSchemaType, "decimal");
NS_NAMED_LITERAL_STRING(kFloatSchemaType, "float");
NS_NAMED_LITERAL_STRING(kDoubleSchemaType, "double");
NS_NAMED_LITERAL_STRING(kDurationSchemaType, "duration");
NS_NAMED_LITERAL_STRING(kDateTimeSchemaType, "dateTime");
NS_NAMED_LITERAL_STRING(kTimeSchemaType, "time");
NS_NAMED_LITERAL_STRING(kDateSchemaType, "date");
NS_NAMED_LITERAL_STRING(kGYearMonthSchemaType, "gYearMonth");
NS_NAMED_LITERAL_STRING(kGYearSchemaType, "gYear");
NS_NAMED_LITERAL_STRING(kGMonthDaySchemaType, "gMonthDay");
NS_NAMED_LITERAL_STRING(kGDaySchemaType, "gDay");
NS_NAMED_LITERAL_STRING(kGMonthSchemaType, "gMonth");
NS_NAMED_LITERAL_STRING(kHexBinarySchemaType, "hexBinary");
NS_NAMED_LITERAL_STRING(kBase64BinarySchemaType, "base64Binary");
NS_NAMED_LITERAL_STRING(kAnyURISchemaType, "anyURI");
NS_NAMED_LITERAL_STRING(kQNameSchemaType, "QName");
NS_NAMED_LITERAL_STRING(kNOTATIONSchemaType, "NOTATION");
NS_NAMED_LITERAL_STRING(kNormalizedStringSchemaType, "normalizedString");
NS_NAMED_LITERAL_STRING(kTokenSchemaType, "token");
NS_NAMED_LITERAL_STRING(kLanguageSchemaType, "language");
NS_NAMED_LITERAL_STRING(kNMTOKENSchemaType, "NMTOKEN");
NS_NAMED_LITERAL_STRING(kNMTOKENSSchemaType, "NMTOKENS");
NS_NAMED_LITERAL_STRING(kNameSchemaType, "Name");
NS_NAMED_LITERAL_STRING(kNCNameSchemaType, "NCName");
NS_NAMED_LITERAL_STRING(kIDSchemaType, "ID");
NS_NAMED_LITERAL_STRING(kIDREFSchemaType, "IDREF");
NS_NAMED_LITERAL_STRING(kIDREFSSchemaType, "IDREFS");
NS_NAMED_LITERAL_STRING(kENTITYSchemaType, "ENTITY");
NS_NAMED_LITERAL_STRING(kENTITIESSchemaType, "ENTITIES");
NS_NAMED_LITERAL_STRING(kIntegerSchemaType, "integer");
NS_NAMED_LITERAL_STRING(kNonPositiveIntegerSchemaType, "nonPositiveInteger");
NS_NAMED_LITERAL_STRING(kNegativeIntegerSchemaType, "negativeInteger");
NS_NAMED_LITERAL_STRING(kLongSchemaType, "long");
NS_NAMED_LITERAL_STRING(kIntSchemaType, "int");
NS_NAMED_LITERAL_STRING(kShortSchemaType, "short");
NS_NAMED_LITERAL_STRING(kByteSchemaType, "byte");
NS_NAMED_LITERAL_STRING(kNonNegativeIntegerSchemaType, "nonNegativeInteger");
NS_NAMED_LITERAL_STRING(kUnsignedLongSchemaType, "unsignedLong");
NS_NAMED_LITERAL_STRING(kUnsignedIntSchemaType, "unsignedInt");
NS_NAMED_LITERAL_STRING(kUnsignedShortSchemaType, "unsignedShort");
NS_NAMED_LITERAL_STRING(kUnsignedByteSchemaType, "unsignedByte");
NS_NAMED_LITERAL_STRING(kPositiveIntegerSchemaType, "positiveInteger");

NS_NAMED_LITERAL_STRING(kTrue, "true");
NS_NAMED_LITERAL_STRING(kFalse, "false");

NS_NAMED_LITERAL_STRING(kTrueA, "1");
NS_NAMED_LITERAL_STRING(kFalseA, "0");

#define DECLARE_ENCODER(name) \
class ns##name##Encoder : \
  public nsISOAPEncoder, \
  public nsISOAPDecoder \
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

// All encoders must be first declared and then registered.
DECLARE_ENCODER(Default)
DECLARE_ENCODER(String)
DECLARE_ENCODER(Boolean)
DECLARE_ENCODER(Double)
DECLARE_ENCODER(Float)
DECLARE_ENCODER(Long)
DECLARE_ENCODER(Int)
DECLARE_ENCODER(Short)
DECLARE_ENCODER(Byte)
DECLARE_ENCODER(UnsignedLong)
DECLARE_ENCODER(UnsignedInt)
DECLARE_ENCODER(UnsignedShort)
DECLARE_ENCODER(UnsignedByte)
DECLARE_ENCODER(Array)

#define REGISTER_ENCODER(name) \
{\
  ns##name##Encoder *handler = new ns##name##Encoder();\
  SetEncoder(kSchemaNamespaceURI, k##name##SchemaType, handler); \
  SetEncoder(kSchemaDatatypesNamespaceURI, k##name##SchemaType, handler); \
  SetDecoder(kSchemaNamespaceURI, k##name##SchemaType, handler); \
  SetDecoder(kSchemaDatatypesNamespaceURI, k##name##SchemaType, handler); \
}

nsDefaultSOAPEncoder::nsDefaultSOAPEncoder(): nsSOAPEncoding(nsSOAPUtils::kSOAPEncodingURI, nsnull) 
{
  {
    nsDefaultEncoder *handler = new nsDefaultEncoder();
    SetDefaultEncoder(handler);
    SetDefaultDecoder(handler);
  }
  REGISTER_ENCODER(String)
  REGISTER_ENCODER(Boolean)
  REGISTER_ENCODER(Double)
  REGISTER_ENCODER(Float)
  REGISTER_ENCODER(Long)
  REGISTER_ENCODER(Int)
  REGISTER_ENCODER(Short)
  REGISTER_ENCODER(Byte)
  REGISTER_ENCODER(UnsignedLong)
  REGISTER_ENCODER(UnsignedInt)
  REGISTER_ENCODER(UnsignedShort)
  REGISTER_ENCODER(UnsignedByte)
  {
    nsArrayEncoder *handler = new nsArrayEncoder();
    SetEncoder(nsSOAPUtils::kSOAPEncodingURI, kArraySOAPType, handler); 
    SetDecoder(nsSOAPUtils::kSOAPEncodingURI, kArraySOAPType, handler); 
  }
}
//  Here is the implementation of the encoders.
static nsresult EncodeSimpleValue(
		                          const nsAReadableString & aValue, 
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsIDOMElement* aDestination,
					  nsIDOMElement** _retval)
{
  nsCOMPtr<nsIDOMDocument>document;
  nsresult rc = aDestination->GetOwnerDocument(getter_AddRefs(document));
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMElement>element;
  rc = document->CreateElementNS(aNamespaceURI,
		                 aName, 
				 _retval);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIDOMNode>ignore;
  rc = aDestination->AppendChild(*_retval, getter_AddRefs(ignore));
  if (NS_FAILED(rc)) return rc;
  if (!aValue.IsEmpty()) {
    nsCOMPtr<nsIDOMText> text;
    rc = document->CreateTextNode(aValue, getter_AddRefs(text));
    if (NS_FAILED(rc)) return rc;
    return (*_retval)->AppendChild(text, getter_AddRefs(ignore));
  }
  return rc;
}

//  Default

NS_IMETHODIMP nsDefaultEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsAutoString schemaType;
  nsAutoString schemaURI;
  nsCOMPtr<nsISchemaType> lookupType = aSchemaType;
  nsCOMPtr<nsISOAPEncoder> encoder;
  while(lookupType) {
    lookupType->GetName(schemaType);
    lookupType->GetTargetNamespace(schemaURI);
    aEncoding->GetEncoder(schemaType, schemaURI, getter_AddRefs(encoder));
    if (encoder) break;
    PRUint16 type;
    lookupType->GetSchemaType(&type);
    if (type == nsISchemaType::SCHEMA_TYPE_COMPLEX) {
      nsCOMPtr<nsISchemaComplexType> oldtype = do_QueryInterface(lookupType);
      oldtype->GetBaseType(getter_AddRefs(lookupType));
    }
    else {
      lookupType = nsnull;
    }
  }

  if (!encoder) {
    schemaURI.Assign(kSchemaDatatypesNamespaceURI);
    PRUint16 type;
    aSource->GetDataType(&type);
    switch(type) {
      case nsIDataType::VTYPE_INT8:
        schemaType.Assign(kByteSchemaType);
        break;
      case nsIDataType::VTYPE_INT16:
        schemaType.Assign(kShortSchemaType);
        break;
      case nsIDataType::VTYPE_INT32:
        schemaType.Assign(kIntSchemaType);
        break;
      case nsIDataType::VTYPE_INT64:
        schemaType.Assign(kLongSchemaType);
        break;
      case nsIDataType::VTYPE_UINT8:
        schemaType.Assign(kUnsignedByteSchemaType);
        break;
      case nsIDataType::VTYPE_UINT16:
        schemaType.Assign(kUnsignedShortSchemaType);
        break;
      case nsIDataType::VTYPE_UINT32:
        schemaType.Assign(kUnsignedIntSchemaType);
        break;
      case nsIDataType::VTYPE_UINT64:
        schemaType.Assign(kUnsignedLongSchemaType);
          break;
      case nsIDataType::VTYPE_FLOAT:
        schemaType.Assign(kFloatSchemaType);
        break;
      case nsIDataType::VTYPE_DOUBLE:
        schemaType.Assign(kDoubleSchemaType);
        break;
      case nsIDataType::VTYPE_BOOL:
        schemaType.Assign(kBooleanSchemaType);
        break;
      case nsIDataType::VTYPE_ARRAY:
        schemaType.Assign(kArraySOAPType);
        schemaURI.Assign(nsSOAPUtils::kSOAPEncodingURI);
        break;
#if 0
      {
	nsIID* iid;
	nsCOMPtr<nsISupports> ptr;
	nsresult rv = foo->GetAsInterface(&iid, getter_AddRefs(ptr));
	if (compare_IIDs(iid, NS_GET_IID(nsIPropertyBag)) == 0) {
          schemaType.Assign(kAnyTypeSchemaType);
	  break;
	}
      }
      {
	nsCOMPtr<nsISupports> ptr;
	nsresult rv = foo->GetAsISupports(getter_AddRefs(ptr));
      }
#endif
      case nsIDataType::VTYPE_INTERFACE_IS:
      case nsIDataType::VTYPE_INTERFACE:
      case nsIDataType::VTYPE_VOID:
      case nsIDataType::VTYPE_EMPTY:
        schemaType.Assign(kAnyTypeSchemaType);
        break;
//    case nsIDataType::VTYPE_CHAR_STR:
//    case nsIDataType::VTYPE_WCHAR_STR:
//    case nsIDataType::VTYPE_CHAR:
//    case nsIDataType::VTYPE_WCHAR:
//    case nsIDataType::VTYPE_ASTRING:
//    case nsIDataType::VTYPE_ID:
      default:
        schemaType.Assign(kStringSchemaType);
    }
    aEncoding->GetEncoder(schemaType, schemaURI, getter_AddRefs(encoder));
    if (!encoder) {
      return NS_ERROR_FAILURE;  //  Should return a better not supported error
    }
  }
  //  It might be desirable to switch on automatic type recording, especially of non-explicit types.
  nsresult rc = encoder->Encode(aEncoding, aSource, aNamespaceURI, aName, aSchemaType, aAttachments, aDestination, aReturnValue);
  if (NS_FAILED(rc)) return rc;
  if (*aReturnValue	//  If we are not schema-controlled, then add a type attribute as a hint about what we did.
    && !aSchemaType
    && !aName.IsEmpty()) {
    nsAutoString type;
    rc = nsSOAPUtils::MakeNamespacePrefixFixed(*aReturnValue, schemaURI, type);
    if (NS_FAILED(rc)) return rc;
    type.Append(nsSOAPUtils::kQualifiedSeparator);
    type.Append(schemaType);
    (*aReturnValue)->SetAttributeNS(nsSOAPUtils::kXSIURI, nsSOAPUtils::kXSITypeAttribute, type);
  }
  return rc;
}

//  String

NS_IMETHODIMP nsStringEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  nsAutoString value;
  rc = aSource->GetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kStringSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRBool

NS_IMETHODIMP nsBooleanEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRBool b;
  rc = aSource->GetAsBool(&b);
  if (NS_FAILED(rc)) return rc;
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(b ? kTrue : kFalse,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kBooleanSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(b ? kTrue : kFalse,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Double

NS_IMETHODIMP nsDoubleEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  double f;
  rc = aSource->GetAsDouble(&f);//  Check that double works.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%lf",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kDoubleSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Float

NS_IMETHODIMP nsFloatEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  float f;
  rc = aSource->GetAsFloat(&f);//  Check that float works.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%f",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kFloatSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt64

NS_IMETHODIMP nsLongEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt64 f;
  rc = aSource->GetAsInt64(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%lld",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kLongSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt32

NS_IMETHODIMP nsIntEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt32 f;
  rc = aSource->GetAsInt32(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kIntSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRInt16

NS_IMETHODIMP nsShortEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRInt16 f;
  rc = aSource->GetAsInt16(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",(PRInt32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kShortSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Byte

NS_IMETHODIMP nsByteEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint8 f;
  rc = aSource->GetAsInt8(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%d",(PRInt32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kByteSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint64

NS_IMETHODIMP nsUnsignedLongEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint64 f;
  rc = aSource->GetAsUint64(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%llu",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kLongSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint32

NS_IMETHODIMP nsUnsignedIntEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint32 f;
  rc = aSource->GetAsUint32(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kIntSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  PRUint16

NS_IMETHODIMP nsUnsignedShortEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint16 f;
  rc = aSource->GetAsUint16(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",(PRUint32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kShortSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

//  Unsigned Byte

NS_IMETHODIMP nsUnsignedByteEncoder::Encode(nsISOAPEncoding* aEncoding,
		                          nsIVariant* aSource,
		                          const nsAReadableString & aNamespaceURI, 
		                          const nsAReadableString & aName, 
					  nsISchemaType *aSchemaType,
					  nsISOAPAttachments* aAttachments,
					  nsIDOMElement* aDestination,
					  nsIDOMElement* * aReturnValue)
{
  nsresult rc;
  PRUint8 f;
  rc = aSource->GetAsUint8(&f);//  Get as a long number.
  if (NS_FAILED(rc)) return rc;
  char* ptr = PR_smprintf("%u",(PRUint32)f);
  if (!ptr) return NS_ERROR_OUT_OF_MEMORY;
  nsAutoString value;
  CopyASCIItoUCS2(nsDependentCString(ptr), value);
  PR_smprintf_free(ptr);
  if (aName.IsEmpty()) {
    return EncodeSimpleValue(value,
		       nsSOAPUtils::kSOAPEncodingURI,
		       kByteSchemaType,
		       aDestination,
		       aReturnValue);
  }
  return EncodeSimpleValue(value,
		       aNamespaceURI,
		       aName,
		       aDestination,
		       aReturnValue);
}

NS_IMETHODIMP nsStringEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID, &rc);
  if (NS_FAILED(rc)) return rc;
  rc = p->SetAsAString(value);
  if (NS_FAILED(rc)) return rc;
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsBooleanEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  bool b;
  if (value.Equals(kTrue)
    || value.Equals(kTrueA)) {
    b = PR_TRUE;
  } else if (value.Equals(kFalse)
    || value.Equals(kFalseA)) {
    b = PR_FALSE;
  } else return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsBool(b);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsDoubleEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  double f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %lf %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsDouble(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsFloatEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  float f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %f %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsFloat(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsLongEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt64 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %lld %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt64(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsIntEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt32 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %ld %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt32(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsShortEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt16 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %hd %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt16(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsByteEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRInt16 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %hd %n",&f,&n);
  if (r == 0 || n < strlen(ascii) || f < -128 || f > 127) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsInt8((PRUint8)f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedLongEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint64 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %llu %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint64(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedIntEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint32 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %lu %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint32(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedShortEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint16 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %hu %n",&f,&n);
  if (r == 0 || n < strlen(ascii)) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint16(f);
  *_retval = p;
  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP nsUnsignedByteEncoder::Decode(nsISOAPEncoding* aEncoding,
		                            nsIDOMElement *aSource, 
					    nsISchemaType *aSchemaType,
					    nsISOAPAttachments* aAttachments,
					    nsIVariant **_retval)
{
  nsAutoString value;
  nsresult rc = nsSOAPUtils::GetElementTextContent(aSource, value);
  if (NS_FAILED(rc)) return rc;
  PRUint16 f;
  const char* ascii = NS_ConvertUCS2toUTF8(value).get();
  unsigned int n;
  int r = PR_sscanf(ascii, " %hu %n",&f,&n);
  if (r == 0 || n < strlen(ascii) || f < 0 || f > 255) return NS_ERROR_ILLEGAL_VALUE;

  nsCOMPtr<nsIWritableVariant> p = do_CreateInstance(NS_VARIANT_CONTRACTID);
  p->SetAsUint8((PRUint8)f);
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
    nsCOMPtr<nsIVariant> param = do_QueryInterface(elemisup);
    
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
nsDefaultSOAPEncoder::EncodeParameter(nsIVariant* parameter,
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
    nsCOMPtr<nsIVariant> param;
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
      
      param = (nsIVariant*)newparam;
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
      nsCOMPtr<nsIVariant> param;
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
          
          param = (nsIVariant*)newparam;
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
nsDefaultSOAPEncoder::SerializeParameterValue(nsIVariant* parameter, 
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
    case nsIVariant::PARAMETER_TYPE_NULL:
    case nsIVariant::PARAMETER_TYPE_VOID:
    {
      nsAutoString attrNameStr, attrNameSpace, attrValueStr;
      attrNameStr.AssignWithConversion("xsi:null");
      attrNameSpace.AssignWithConversion(nsSOAPUtils::kXSIURI);
      attrValueStr.AssignWithConversion("true");
      
      element->SetAttributeNS(attrNameSpace, attrNameStr, attrValueStr);
      break;
    }
    
    case nsIVariant::PARAMETER_TYPE_STRING:
    {
      nsCOMPtr<nsISupportsWString> wstr = do_QueryInterface(isup);
      if (wstr) {
        wstr->ToString(getter_Copies(wstringData));
      }
      break;
    }
    
    case nsIVariant::PARAMETER_TYPE_BOOLEAN:
    {
      nsCOMPtr<nsISupportsPRBool> prb = do_QueryInterface(isup);
      if (prb) {
        prb->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_DOUBLE:
    {
      nsCOMPtr<nsISupportsDouble> dub = do_QueryInterface(isup);
      if (dub) {
        dub->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_FLOAT:
    {
      nsCOMPtr<nsISupportsFloat> flt = do_QueryInterface(isup);
      if (flt) {
        flt->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_LONG:
    {
      nsCOMPtr<nsISupportsPRInt64> val64 = do_QueryInterface(isup);
      if (val64) {
        val64->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_INT:
    {
      nsCOMPtr<nsISupportsPRInt32> val32 = do_QueryInterface(isup);
      if (val32) {
        val32->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_SHORT:
    {
      nsCOMPtr<nsISupportsPRInt16> val16 = do_QueryInterface(isup);
      if (val16) {
        val16->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_BYTE:
    {
      nsCOMPtr<nsISupportsPRInt16> val8 = do_QueryInterface(isup);
      if (val8) {
        val8->ToString(getter_Copies(stringData));
      }
      break;
    }

    case nsIVariant::PARAMETER_TYPE_ARRAY:
    {
      nsCOMPtr<nsISupportsArray> array = do_QueryInterface(isup);
      if (!array) return NS_ERROR_INVALID_ARG;

      rv = SerializeSupportsArray(array, element, document);
      if (NS_FAILED(rv)) return rv;

      break;
    }

    case nsIVariant::PARAMETER_TYPE_JAVASCRIPT_ARRAY:
    {
      rv = SerializeJavaScriptArray(obj, element, document);
      if (NS_FAILED(rv)) return rv;

      break;
    }

    case nsIVariant::PARAMETER_TYPE_JAVASCRIPT_OBJECT:
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

/* nsIDOMElement parameterToElement (in nsIVariant parameter, in string encodingStyle, in nsIDOMDocument document); */
NS_IMETHODIMP 
nsDefaultSOAPEncoder::ParameterToElement(nsIVariant *parameter, 
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
                                      nsIVariant **_retval)
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
  PRInt32 type = nsIVariant::PARAMETER_TYPE_UNKNOWN;
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
    nsCOMPtr<nsIVariant> param;

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
    nsCOMPtr<nsIVariant> param;
    rv = DecodeParameter(child, 
                         nsIVariant::PARAMETER_TYPE_UNKNOWN,
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
                                           nsIVariant **_retval)
{
  nsresult rv;
  nsAutoString text;
  nsCOMPtr<nsISupports> value;
  JSObject* jsvalue = nsnull;
  nsSOAPParameter* param = new nsSOAPParameter();
  if (!param) return NS_ERROR_OUT_OF_MEMORY;

  param->QueryInterface(NS_GET_IID(nsIVariant), (void**)_retval);
  
  switch (type) {
    case nsIVariant::PARAMETER_TYPE_NULL:
    case nsIVariant::PARAMETER_TYPE_VOID:
    {
      param->SetValueAndType(nsnull, type);
      break;
    }

    case nsIVariant::PARAMETER_TYPE_STRING:
    {
      nsCOMPtr<nsISupportsWString> wstr = do_CreateInstance(NS_SUPPORTS_WSTRING_CONTRACTID);
      if (!wstr) return NS_ERROR_FAILURE;

      nsSOAPUtils::GetElementTextContent(element, text);

      wstr->SetData(text.GetUnicode());     
      value = wstr;
      break;
    }

    case nsIVariant::PARAMETER_TYPE_BOOLEAN:
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

    case nsIVariant::PARAMETER_TYPE_DOUBLE:
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

    case nsIVariant::PARAMETER_TYPE_FLOAT:
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

    case nsIVariant::PARAMETER_TYPE_LONG:
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

    case nsIVariant::PARAMETER_TYPE_INT:
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

    case nsIVariant::PARAMETER_TYPE_SHORT:
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

    case nsIVariant::PARAMETER_TYPE_BYTE:
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

    case nsIVariant::PARAMETER_TYPE_ARRAY:
    {
      nsCOMPtr<nsISupportsArray> array;
      
      rv = DeserializeSupportsArray(element, getter_AddRefs(array));
      if (NS_FAILED(rv)) return rv;

      value = array;
      break;
    }

    case nsIVariant::PARAMETER_TYPE_JAVASCRIPT_OBJECT:
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

/* nsIVariant elementToParameter (in nsIDOMElement element, in string encodingStyle); */
NS_IMETHODIMP 
nsDefaultSOAPEncoder::ElementToParameter(nsIDOMElement *element, 
                                         const char *encodingStyle, 
                                         PRInt32 hintType,
                                         nsIVariant **_retval)
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

  if (nsIVariant::PARAMETER_TYPE_UNKNOWN == hintType) {
    // See if the element has a xsi:null attribute
    attrNS.AssignWithConversion(nsSOAPUtils::kXSIURI);
    attrName.AssignWithConversion("null");
    element->GetAttributeNS(attrNS, attrName, attrVal);
    if (attrVal.Length() > 0) {
      type = nsIVariant::PARAMETER_TYPE_NULL;
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
          type = nsIVariant::PARAMETER_TYPE_JAVASCRIPT_OBJECT;
        }
        else {
          type = nsIVariant::PARAMETER_TYPE_STRING;
        }
      }
    }
  }

  nsCOMPtr<nsIVariant> parameter;
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
