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

#include "nsSOAPUtils.h"
#include "nsIDOMText.h"
#include "nsCOMPtr.h"

NS_NAMED_LITERAL_STRING(kSOAPEnvURI,"http://schemas.xmlsoap.org/soap/envelope/");
NS_NAMED_LITERAL_STRING(kSOAPEncodingURI,"http://schemas.xmlsoap.org/soap/encoding/");
NS_NAMED_LITERAL_STRING(kSOAPEnvPrefix,"SOAP-ENV");
NS_NAMED_LITERAL_STRING(kSOAPEncodingPrefix,"SOAP-ENC");
NS_NAMED_LITERAL_STRING(kXSIURI,"http://www.w3.org/1999/XMLSchema-instance");
NS_NAMED_LITERAL_STRING(kXSDURI,"http://www.w3.org/1999/XMLSchema");
NS_NAMED_LITERAL_STRING(kXSIPrefix,"xsi");
NS_NAMED_LITERAL_STRING(kXSDPrefix,"xsd");
NS_NAMED_LITERAL_STRING(kEncodingStyleAttribute,"encodingStyle");
NS_NAMED_LITERAL_STRING(kEnvelopeTagName,"Envelope");
NS_NAMED_LITERAL_STRING(kHeaderTagName,"Header");
NS_NAMED_LITERAL_STRING(kBodyTagName,"Body");
NS_NAMED_LITERAL_STRING(kFaultTagName,"Fault");
NS_NAMED_LITERAL_STRING(kFaultCodeTagName,"faultcode");
NS_NAMED_LITERAL_STRING(kFaultStringTagName,"faultstring");
NS_NAMED_LITERAL_STRING(kFaultActorTagName,"faultactor");
NS_NAMED_LITERAL_STRING(kFaultDetailTagName,"detail");

NS_NAMED_LITERAL_STRING(kSOAPCallType,"#nsSOAPUtils::kSOAPCallType");
NS_NAMED_LITERAL_STRING(kEmpty,"");

NS_NAMED_LITERAL_STRING(kWStringType,"#DOMString");
NS_NAMED_LITERAL_STRING(kPRBoolType,"#boolean");
NS_NAMED_LITERAL_STRING(kDoubleType,"#double");
NS_NAMED_LITERAL_STRING(kFloatType,"#float");
NS_NAMED_LITERAL_STRING(kPRInt64Type,"#long");
NS_NAMED_LITERAL_STRING(kPRInt32Type,"#int");
NS_NAMED_LITERAL_STRING(kPRInt16Type,"#short");
NS_NAMED_LITERAL_STRING(kCharType,"#byte");
NS_NAMED_LITERAL_STRING(kArrayType,"#array");
NS_NAMED_LITERAL_STRING(kJSObjectTypePrefix,"#js#");
NS_NAMED_LITERAL_STRING(kTypeSeparator,"#");
NS_NAMED_LITERAL_STRING(kIIDObjectTypePrefix,"#iid#");
NS_NAMED_LITERAL_STRING(kNullType,"#null");
NS_NAMED_LITERAL_STRING(kVoidType,"#void");
NS_NAMED_LITERAL_STRING(kUnknownType,"#unknown");

void 
nsSOAPUtils::GetSpecificChildElement(
  nsIDOMElement *aParent, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  GetFirstChildElement(aParent, getter_AddRefs(sibling));
  if (sibling)
  {
    GetSpecificSiblingElement(sibling,
      aNamespace, aType, aElement);
  }
}

void 
nsSOAPUtils::GetSpecificSiblingElement(
  nsIDOMElement *aSibling, 
  const nsAReadableString& aNamespace, 
  const nsAReadableString& aType, 
  nsIDOMElement * *aElement)
{
  nsCOMPtr<nsIDOMElement> sibling;

  *aElement = nsnull;
  sibling = aSibling;
  do {
    nsAutoString name, namespaceURI;
    sibling->GetLocalName(name);
    sibling->GetNamespaceURI(namespaceURI);
    if (name.Equals(aType)
      && namespaceURI.Equals(nsSOAPUtils::kSOAPEnvURI))
    {
      *aElement = sibling;
      NS_ADDREF(*aElement);
      return;
    }
    nsCOMPtr<nsIDOMElement> temp = sibling;
    GetNextSiblingElement(temp, getter_AddRefs(sibling));
  } while (sibling);
}

void
nsSOAPUtils::GetFirstChildElement(nsIDOMElement* aParent, 
                                  nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  *aElement = nsnull;
  aParent->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      child->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
}

void
nsSOAPUtils::GetNextSiblingElement(nsIDOMElement* aStart, 
                                   nsIDOMElement** aElement)
{
  nsCOMPtr<nsIDOMNode> sibling;

  *aElement = nsnull;
  GetNextSibling(aStart, getter_AddRefs(sibling));
  while (sibling) {
    PRUint16 type;
    sibling->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      sibling->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)aElement);
      break;
    }
    nsCOMPtr<nsIDOMNode> temp = sibling;
    GetNextSibling(temp, getter_AddRefs(sibling));
  }
}

void 
nsSOAPUtils::GetElementTextContent(nsIDOMElement* aElement, 
                                   nsAWritableString& aText)
{
  nsCOMPtr<nsIDOMNode> child;
  nsAutoString rtext;
  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::TEXT_NODE == type
        || nsIDOMNode::CDATA_SECTION_NODE == type) {
      nsCOMPtr<nsIDOMText> text = do_QueryInterface(child);
      nsAutoString data;
      text->GetData(data);
      rtext.Append(data);
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }
  aText.Assign(rtext);
}

PRBool
nsSOAPUtils::HasChildElements(nsIDOMElement* aElement)
{
  nsCOMPtr<nsIDOMNode> child;

  aElement->GetFirstChild(getter_AddRefs(child));
  while (child) {
    PRUint16 type;
    child->GetNodeType(&type);
    if (nsIDOMNode::ELEMENT_NODE == type) {
      return PR_TRUE;
    }
    nsCOMPtr<nsIDOMNode> temp = child;
    GetNextSibling(temp, getter_AddRefs(child));
  }

  return PR_FALSE;
}

void
nsSOAPUtils::GetNextSibling(nsIDOMNode* aSibling, nsIDOMNode **aNext)
{
  nsCOMPtr<nsIDOMNode> last;
  nsCOMPtr<nsIDOMNode> current;
  PRUint16 type;

  *aNext = nsnull;
  last = aSibling;

  last->GetNodeType(&type);
  if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
    last->GetFirstChild(getter_AddRefs(current));
    if (!last)
    {
      last->GetNextSibling(getter_AddRefs(current));
    }
  }
  else {
    last->GetNextSibling(getter_AddRefs(current));
  }
  while (!current)
  {
    last->GetParentNode(getter_AddRefs(current));
    current->GetNodeType(&type);
    if (nsIDOMNode::ENTITY_REFERENCE_NODE == type) {
      last = current;
      last->GetNextSibling(getter_AddRefs(current));
    }
    else {
      current = nsnull;
      break;
    }
  }
  *aNext = current;
  NS_IF_ADDREF(*aNext);
}
#if 0
nsresult 
GetNamespacePrefix(nsIDOMNode* aNode,
                   const nsAReadableString & aURI,
                   nsAWritableString & aPrefix)
{
  nsCOMPtr<nsIDOMNode> scope = aNode;
  for (;;) {
    nsCOMPtr<nsIDOMNamedNodeMap> attrs;
    scope->GetAttributes(getter_AddRefs(attrs));
    if (attrs) {
      PRUint32 i = 0;
      for (;;)
      {
        nsCOMPtr<nsIDOMAttr> attr;
        attrs->Item(i++, attr);
        if (!attr)
          break;
        nsAutoString temp;
        attr->GetNamespaceURI(temp);
        if (!tmp.Equals(kNamespaceNamespaceURI))
          continue;
        attr->GetValue(temp);
        if (!localName.Equals(aURI))
          continue;
        attr->GetLocalName(aPrefix);
        return NS_OK;
      }
    }
    nsCOMPtr<nsIDOMNode> next;
    scope->GetParentNode(getter_AddRefs(next));
    if (next)
      scope = next;
    else
      break;
  }
  aPrefix = nsSOAPUtils::kEmpty;
  return NS_OK;
}

nsresult 
GetNamespaceURI(nsIDOMElement* aElement,
                const nsAReadableString & aPrefix, 
                nsAWritableString & aURI)
{
  nsCOMPtr<nsIDOMElement> scope = aElement;
  for (;;) {
    PRBool exists
    scope->GetAttributeExistsNS(aPrefix, kNamespaceNamespaceURI, &exists);
    if (exists) {
      scope->GetAttributeNS(aPrefix, kNamespaceNamespaceURIi, aURI);
      return NS_OK;
    }
    nsCOMPtr<nsIDOMNode> next;
    scope->GetParentNode(getter_AddRefs(next));
    if (next)
      scope = next;
    else
      break;
  }
  aURI = nsSOAPUtils::kEmpty;
  return NS_OK;
}
#endif

void
nsSOAPUtils::GetInheritedEncodingStyle(nsIDOMElement* aEntry, 
                                       nsAWritableString & aEncodingStyle)
{
  nsCOMPtr<nsIDOMNode> node = aEntry;

  while (node) {
    nsAutoString value;
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(node);
    if (element) {
      element->GetAttributeNS(nsSOAPUtils::kSOAPEnvURI, nsSOAPUtils::kEncodingStyleAttribute,
                              value);
      if (value.Length() > 0) {
        aEncodingStyle.Assign(value);
        return;
      }
    }
    nsCOMPtr<nsIDOMNode> temp = node;
    temp->GetParentNode(getter_AddRefs(node));
  }
  aEncodingStyle.Assign(kSOAPEncodingURI);
}

