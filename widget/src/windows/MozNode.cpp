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
 * Original Author: Aaron Leventhal (aaronl@netscape.com)
 *
 * Contributor(s): 
 */

#include "nsIAccessible.h"
#include "Accessible.h"
#include "IMozNode.h"
#include "IMozNode_iid.h"
#include "nsIWidget.h"
#include "nsWindow.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIAccessibleEventReceiver.h"
#include "nsReadableUtils.h"
#include "nsITextContent.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMDocument.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMViewCSS.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsILink.h"
#include "nsIAccessibilityService.h"
#include "nsIServiceManager.h"

/* For documentation of the accessibility architecture, 
 * see http://lxr.mozilla.org/seamonkey/source/accessible/accessible-docs.html
 */

//#define DEBUG_LEAKS

#ifdef DEBUG_LEAKS
static gMozNodes = 0;
#endif

/*
 * Class Accessible
 */


//-----------------------------------------------------
// construction 
//-----------------------------------------------------

MozNode::MozNode(nsIAccessible* aNSAcc, HWND aWnd): mWnd(aWnd), m_cRef(0)
{
  aNSAcc->AccGetDOMNode(getter_AddRefs(mDOMNode));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(mDOMNode));

#ifdef DEBUG_LEAKS
  printf("MozNodes=%d\n", ++gMozNodes);
#endif
}

MozNode::MozNode(nsIDOMNode *aNode, HWND aWnd): mDOMNode(aNode), mWnd(aWnd), m_cRef(0)
{
#ifdef DEBUG_LEAKS
  printf("MozNodes=%d\n", ++gMozNodes);
#endif
}

//-----------------------------------------------------
// destruction
//-----------------------------------------------------
MozNode::~MozNode()
{
  m_cRef = 0;
#ifdef DEBUG_LEAKS
  printf("MozNodes=%d\n", --gMozNodes);
#endif
}


//-----------------------------------------------------
// IUnknown interface methods - see inknown.h for documentation
//-----------------------------------------------------
STDMETHODIMP MozNode::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IUnknown == iid || IID_IMozNode == iid)
    *ppv = NS_STATIC_CAST(IMozNode*, this);

  if (NULL == *ppv)
    return E_NOINTERFACE;      //iid not supported.
   
  (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef(); 
  return S_OK;
}

//-----------------------------------------------------
STDMETHODIMP_(ULONG) MozNode::AddRef()
{
  return ++m_cRef;
}


//-----------------------------------------------------
STDMETHODIMP_(ULONG) MozNode::Release()
{
  if (0 != --m_cRef)
    return m_cRef;

  delete this;

  return 0;
}


//-----------------------------------------------------
// IMozNode methods
//-----------------------------------------------------

STDMETHODIMP MozNode::get_nodeInfo( 
    /* [out] */ BSTR __RPC_FAR *aTagName,
    /* [out] */ short __RPC_FAR *aNameSpaceID,
    /* [out] */ unsigned short __RPC_FAR *aNodeType,
    /* [out] */ BSTR __RPC_FAR *aNodeValue,
    /* [out] */ unsigned int __RPC_FAR *aNumChildren)
{
  *aTagName = NULL;
  nsCOMPtr<nsIDOMElement> domElement;
  nsCOMPtr<nsIContent> content;
  GetElementAndContentFor(domElement, content);
  
  nsAutoString tagName, nodeValue;

  mDOMNode->GetNodeValue(nodeValue);
  if (domElement) 
    domElement->GetTagName(tagName);
  
  PRUnichar *pszTagName  =  tagName.ToNewUnicode();
  PRUnichar *pszNodeValue = nodeValue.ToNewUnicode();

  *aTagName =   ::SysAllocString(pszTagName);
  *aNodeValue = ::SysAllocString(pszNodeValue);

  delete pszTagName; 
  delete pszNodeValue;

  PRInt32 nameSpaceID = 0;
  if (content)
    content->GetNameSpaceID(nameSpaceID);
  *aNameSpaceID = NS_STATIC_CAST(short, nameSpaceID);

  PRUint16 nodeType = 0;
  mDOMNode->GetNodeType(&nodeType);
  *aNodeType=NS_STATIC_CAST(unsigned short, nodeType);

  *aNumChildren = 0;
  PRUint32 numChildren = 0;
  nsCOMPtr<nsIDOMNodeList> nodeList;
  mDOMNode->GetChildNodes(getter_AddRefs(nodeList));
  if (nodeList && NS_OK == nodeList->GetLength(&numChildren))
    *aNumChildren = NS_STATIC_CAST(unsigned int, numChildren);

  return S_OK;
}


       
STDMETHODIMP MozNode::get_attributes( 
    /* [in] */ unsigned short aMaxAttribs,
    /* [out] */ unsigned short __RPC_FAR *aNumAttribs,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *aAttribNames,
    /* [length_is][size_is][out] */ short __RPC_FAR *aNameSpaceIDs,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *aAttribValues)
{
  nsCOMPtr<nsIDOMElement> domElement;
  nsCOMPtr<nsIContent> content;
  GetElementAndContentFor(domElement, content);

  if (!content || !domElement) 
    return S_FALSE;
  PRInt32 numAttribs;
  content->GetAttributeCount(numAttribs);
  if (numAttribs > aMaxAttribs)
    numAttribs = aMaxAttribs;
  *aNumAttribs = NS_STATIC_CAST(unsigned short, numAttribs);

  PRInt32 index, nameSpaceID;
  nsCOMPtr<nsIAtom> nameAtom, prefixAtom;

  for (index = 0; index < numAttribs; index++) {
    aNameSpaceIDs[index] = 0; aAttribValues[index] = aAttribNames[index] = NULL;
    nsAutoString attributeValue;
    PRUnichar *pszAttributeValue;
    const PRUnichar *pszAttributeName; 

    if (NS_SUCCEEDED(content->GetAttributeNameAt(index, nameSpaceID, *getter_AddRefs(nameAtom), *getter_AddRefs(prefixAtom)))) {
      aNameSpaceIDs[index] = NS_STATIC_CAST(short, nameSpaceID);
      nameAtom->GetUnicode(&pszAttributeName);
      aAttribNames[index] = ::SysAllocString(pszAttributeName);
      if (NS_SUCCEEDED(content->GetAttribute(nameSpaceID, nameAtom, attributeValue))) {
        pszAttributeValue = attributeValue.ToNewUnicode();
        aAttribValues[index] = ::SysAllocString(pszAttributeValue);
      }
    }
  }

  return S_OK; 
}
        

STDMETHODIMP MozNode::get_styleRules( 
    /* [in] */ unsigned short aMaxStyleRules,
    /* [out] */ unsigned short __RPC_FAR *aNumStyleRules,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *aStyleProperties,
    /* [length_is][size_is][out] */ short __RPC_FAR *aStyleMediaType,
    /* [length_is][size_is][out] */ BSTR __RPC_FAR *aStyleValues)
{
  *aNumStyleRules = 0;

  nsCOMPtr<nsIDOMElement> domElement;
  nsCOMPtr<nsIContent> content;
  GetElementAndContentFor(domElement, content);
  if (!domElement || !content) 
    return S_FALSE;

  nsCOMPtr<nsIDocument> doc;
  if (content) 
    content->GetDocument(*getter_AddRefs(doc));

  nsCOMPtr<nsIPresShell> shell;
  if (doc) 
    shell = doc->GetShellAt(0);

  if (!shell || !doc)
    return S_FALSE;

  nsCOMPtr<nsIScriptGlobalObject> global;
  doc->GetScriptGlobalObject(getter_AddRefs(global));
  nsCOMPtr<nsIDOMViewCSS> viewCSS(do_QueryInterface(global));

  if (!viewCSS)
    return S_FALSE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> cssDecl;
  nsAutoString empty;
  viewCSS->GetComputedStyle(domElement, empty, getter_AddRefs(cssDecl));

  if (!cssDecl) 
    return S_FALSE;

  PRUint32 length = 0, index, realIndex;
  cssDecl->GetLength(&length);
  if (length > aMaxStyleRules)
    length = aMaxStyleRules;
  *aNumStyleRules = NS_STATIC_CAST(unsigned short, length);
  for (index = realIndex = 0; index < length; index ++) {
    nsAutoString property, value;
    if (NS_SUCCEEDED(cssDecl->Item(index, property)) && property.CharAt(0) != '-')  // Ignore -moz-* properties
      cssDecl->GetPropertyValue(property, value);  // Get property value
    if (value.IsEmpty())    // If empty, don't bother to send it back as a style property
      -- *aNumStyleRules;
    else {
      PRUnichar *pszProperty = property.ToNewUnicode();
      PRUnichar *pszValue = value.ToNewUnicode();
      aStyleProperties[realIndex] =   ::SysAllocString(pszProperty);
      aStyleValues[realIndex]     =   ::SysAllocString(pszValue);
      aStyleMediaType[realIndex] = 0;
      delete pszProperty;
      delete pszValue;
      ++realIndex;
    }
  }

  return S_OK;
}

IMozNode* MozNode::MakeMozNode(nsIDOMNode *node)
{
  if (!node) 
    return NULL;

  nsCOMPtr<nsIContent> content(do_QueryInterface(node));
  if (!content)
    return NULL;

  nsCOMPtr<nsIDocument> doc;
  content->GetDocument(*getter_AddRefs(doc));
  if (!doc)
    return NULL;

  nsCOMPtr<nsIPresShell> shell;
  shell = doc->GetShellAt(0);
  if (!shell)
    return NULL;

  nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
  if (!accService)
    return NULL;

  IMozNode *newNode = NULL;
  
  nsCOMPtr<nsIAccessible> nsAcc;
  accService->GetAccessibleFor(shell, node, getter_AddRefs(nsAcc));
  if (nsAcc) {
    Accessible *acc = new Accessible(nsAcc, mWnd);
    acc->QueryInterface(IID_IMozNode, (void**)&newNode);
  }
  else 
    newNode = new MozNode(node, mWnd);

  if (newNode)
    newNode->AddRef();

  return newNode;
}


STDMETHODIMP MozNode::get_parentNode(IMozNode __RPC_FAR *__RPC_FAR *aNode)
{
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetParentNode(getter_AddRefs(node));
  *aNode = MakeMozNode(node);

  return S_OK;
}

STDMETHODIMP MozNode::get_firstChild(IMozNode __RPC_FAR *__RPC_FAR *aNode)
{
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetFirstChild(getter_AddRefs(node));
  *aNode = MakeMozNode(node);

  return S_OK;
}

STDMETHODIMP MozNode::get_lastChild(IMozNode __RPC_FAR *__RPC_FAR *aNode)
{
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetLastChild(getter_AddRefs(node));
  *aNode = MakeMozNode(node);

  return S_OK;
}

STDMETHODIMP MozNode::get_previousSibling(IMozNode __RPC_FAR *__RPC_FAR *aNode)
{
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetPreviousSibling(getter_AddRefs(node));
  *aNode = MakeMozNode(node);

  return S_OK;
}

STDMETHODIMP MozNode::get_nextSibling(IMozNode __RPC_FAR *__RPC_FAR *aNode)
{
  nsCOMPtr<nsIDOMNode> node;
  mDOMNode->GetNextSibling(getter_AddRefs(node));
  *aNode = MakeMozNode(node);

  return S_OK;
}


        
//------- Helper methods ---------

void MozNode::GetElementAndContentFor(nsCOMPtr<nsIDOMElement>& aElement, nsCOMPtr<nsIContent> &aContent)
{
  aElement = do_QueryInterface(mDOMNode);
  aContent = do_QueryInterface(mDOMNode);
}

