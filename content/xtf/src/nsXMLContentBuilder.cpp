/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ----- BEGIN LICENSE BLOCK -----
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla XTF project.
 *
 * The Initial Developer of the Original Code is 
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ----- END LICENSE BLOCK ----- */

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIXMLContentBuilder.h"
#include "nsISupportsArray.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsIContent.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIElementFactory.h"
#include "nsIAtom.h"
#include "nsContentCID.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"

static NS_DEFINE_CID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);


class nsXMLContentBuilder : public nsIXMLContentBuilder
{
protected:
  friend nsresult NS_NewXMLContentBuilder(nsIXMLContentBuilder** aResult);
  
  nsXMLContentBuilder();
  ~nsXMLContentBuilder();
  
public:
  // nsISupports interface
  NS_DECL_ISUPPORTS

  // nsIXMLContentBuilder interface
  NS_DECL_NSIXMLCONTENTBUILDER

private:
  nsIElementFactory* GetElementFactory();
  
  nsCOMPtr<nsIContent> mTop;
  nsCOMPtr<nsIContent> mCurrent;
  nsCOMPtr<nsIDocument> mDocument;
  nsCOMPtr<nsINameSpaceManager> mNamespaceManager;
  PRInt32 mNamespaceId;
  nsCOMPtr<nsIElementFactory> mElementFactory;
};

//----------------------------------------------------------------------
// implementation:

nsXMLContentBuilder::nsXMLContentBuilder()
    : mNamespaceId(kNameSpaceID_None)
{
#ifdef DEBUG
//  printf("nsXMLContentBuilder CTOR\n");
#endif

  mNamespaceManager = do_CreateInstance(NS_NAMESPACEMANAGER_CONTRACTID);

  mDocument = do_CreateInstance(kXMLDocumentCID);
  // XXX should probably do some doc initialization here, such as
  // setting the base url  
}

nsXMLContentBuilder::~nsXMLContentBuilder()
{
#ifdef DEBUG
//  printf("~nsXMLContentBuilder\n");
#endif
}

nsresult
NS_NewXMLContentBuilder(nsIXMLContentBuilder** aResult)
{
  nsXMLContentBuilder* result = new nsXMLContentBuilder();
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports methods

NS_IMPL_ISUPPORTS1(nsXMLContentBuilder, nsIXMLContentBuilder);

//----------------------------------------------------------------------
// nsIXMLContentBuilder implementation

// /* void clear (); */
// NS_IMETHODIMP nsXMLContentBuilder::Clear()
// {
//   mCurrent = nsnull;
//   mContent->Clear();
//   // XXX clear ns mgr?
//   return NS_OK;
// }

/* void clear (); */
NS_IMETHODIMP nsXMLContentBuilder::Clear()
{
  mCurrent = nsnull;
  mTop = nsnull;
  if (mNamespaceId != kNameSpaceID_None) {
    mNamespaceId = kNameSpaceID_None;
    mElementFactory = nsnull;
  }
  return NS_OK;
}

/* void setElementNamespace (in AString ns); */
NS_IMETHODIMP nsXMLContentBuilder::SetElementNamespace(const nsAString & ns)
{
  PRInt32 oldId = mNamespaceId;
  mNamespaceManager->RegisterNameSpace(ns, mNamespaceId);
  if (oldId != mNamespaceId)
    mElementFactory = nsnull; // invalidate so that it gets resolved
                              // again when needed
  return NS_OK;
}

/* void beginElement (in AString tagname); */
NS_IMETHODIMP nsXMLContentBuilder::BeginElement(const nsAString & tagname)
{
  nsCOMPtr<nsIContent> node;
  {
    nsCOMPtr<nsINodeInfo> nodeInfo;
    mDocument->GetNodeInfoManager()->GetNodeInfo(tagname, nsnull,
                                                 mNamespaceId,
                                                 getter_AddRefs(nodeInfo));
    NS_ASSERTION(nodeInfo, "could not get node info");
  
    GetElementFactory()->CreateInstanceByTag(nodeInfo, getter_AddRefs(node));
  }
  NS_ASSERTION(node, "could not create node");

  // ok, we created a content element. now either append it to our
  // top-level array or to the current element.
  if (!mCurrent) {
    if (mTop) {
      NS_ERROR("Building of multi-rooted trees not supported");
      return NS_ERROR_FAILURE;
    }
    mTop = node;
    mCurrent = mTop;
  }
  else {    
    mCurrent->AppendChildTo(node, PR_TRUE, PR_TRUE);
    mCurrent = node;
  }
  
  return NS_OK;
}

/* void endElement (); */
NS_IMETHODIMP nsXMLContentBuilder::EndElement()
{
  NS_ASSERTION(mCurrent, "unbalanced begin/endelement");
  mCurrent = mCurrent->GetParent();
  return NS_OK;
}

/* void attrib (in AString name, in AString value); */
NS_IMETHODIMP nsXMLContentBuilder::Attrib(const nsAString & name, const nsAString & value)
{
  NS_ASSERTION(mCurrent, "can't set attrib w/o open element");
  nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
  mCurrent->SetAttr(0, nameAtom, value, PR_TRUE);
  return NS_OK;
}

/* readonly attribute nsIDOMElement root; */
NS_IMETHODIMP nsXMLContentBuilder::GetRoot(nsIDOMElement * *aRoot)
{
  if (!mTop) {
    *aRoot = nsnull;
    return NS_OK;
  }
  return mTop->QueryInterface(nsIDOMElement::GetIID(), (void**)aRoot);
}

/* readonly attribute nsIDOMElement current; */
NS_IMETHODIMP nsXMLContentBuilder::GetCurrent(nsIDOMElement * *aCurrent)
{
  if (!mCurrent) {
    *aCurrent = nsnull;
    return NS_OK;
  }  
  return mCurrent->QueryInterface(nsIDOMElement::GetIID(), (void**)aCurrent);
}

//----------------------------------------------------------------------
// helpers

nsIElementFactory*
nsXMLContentBuilder::GetElementFactory() {
  if (!mElementFactory) {
    mNamespaceManager->GetElementFactory(mNamespaceId, getter_AddRefs(mElementFactory));
    NS_ASSERTION(mElementFactory, "could not get element factory");
  }
  return mElementFactory;
}
