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
#include "nsXTFElementWrapper.h"
#include "nsIXTFXMLVisual.h"
#include "nsIAnonymousContentCreator.h"
#include "nsXTFWeakTearoff.h"
#include "nsIXTFXMLVisualWrapper.h"
#include "nsISupportsArray.h"

typedef nsXTFElementWrapper nsXTFXMLVisualWrapperBase;

////////////////////////////////////////////////////////////////////////
// nsXTFXMLVisualWrapper class
class nsXTFXMLVisualWrapper : public nsXTFXMLVisualWrapperBase,
                              public nsIAnonymousContentCreator,
                              public nsIXTFXMLVisualWrapper
{
protected:
  friend nsresult
  NS_NewXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement,
                            nsINodeInfo* ni,
                            nsIContent** aResult);

  nsXTFXMLVisualWrapper(nsINodeInfo* ni, nsIXTFXMLVisual* xtfElement);
  virtual ~nsXTFXMLVisualWrapper();
  nsresult Init();
  
public:
  // nsISupports interface
  NS_DECL_ISUPPORTS_INHERITED

  // nsIXTFXMLVisualWrapper interface
  NS_DECL_NSIXTFXMLVISUALWRAPPER

  // nsIXTFElementWrapper interface
  NS_FORWARD_NSIXTFELEMENTWRAPPER(nsXTFXMLVisualWrapperBase::)
  
  // nsIAnonymousContentCreator
  NS_IMETHOD CreateAnonymousContent(nsIPresContext* aPresContext,
                                    nsISupportsArray& aAnonymousItems);
  
  // If the creator doesn't want to create special frame for frame hierarchy
  // then it should null out the style content arg and return NS_ERROR_FAILURE
  NS_IMETHOD CreateFrameFor(nsIPresContext*   aPresContext,
                            nsIContent *      aContent,
                            nsIFrame**        aFrame) {
    if (aFrame) *aFrame = nsnull; return NS_ERROR_FAILURE; }

private:
  virtual nsIXTFElement *GetXTFElement()const { return mXTFElement; }
  
  nsCOMPtr<nsIXTFXMLVisual> mXTFElement;
};

//----------------------------------------------------------------------
// implementation:

nsXTFXMLVisualWrapper::nsXTFXMLVisualWrapper(nsINodeInfo* aNodeInfo,
                                             nsIXTFXMLVisual* xtfElement)
    : nsXTFXMLVisualWrapperBase(aNodeInfo), mXTFElement(xtfElement)
{
#ifdef DEBUG
//  printf("nsXTFXMLVisualWrapper CTOR\n");
#endif
  NS_ASSERTION(mXTFElement, "xtfElement is null");
}

nsresult
nsXTFXMLVisualWrapper::Init()
{
  // pass a weak wrapper (non base object ref-counted), so that
  // our mXTFElement can safely addref/release.
  nsISupports *weakWrapper=nsnull;
  NS_NewXTFWeakTearoff(NS_GET_IID(nsIXTFXMLVisualWrapper),
                       (nsIXTFXMLVisualWrapper*)this,
                       &weakWrapper);
  if (!weakWrapper) {
    NS_ERROR("could not construct weak wrapper");
    return NS_ERROR_FAILURE;
  }

  mXTFElement->OnCreated((nsIXTFXMLVisualWrapper*)weakWrapper);
  weakWrapper->Release();
  
  return NS_OK;
}

nsXTFXMLVisualWrapper::~nsXTFXMLVisualWrapper()
{
  mXTFElement->OnDestroyed();
  mXTFElement = nsnull;
  
#ifdef DEBUG
//  printf("nsXTFXMLVisualWrapper DTOR\n");
#endif
}

nsresult
NS_NewXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement,
                          nsINodeInfo* ni,
                          nsIContent** aResult)
{
  *aResult = nsnull;
  
  if (!xtfElement) {
    NS_ERROR("can't construct an xtf wrapper without an xtf element");
    return NS_ERROR_FAILURE;
  }
  
  nsXTFXMLVisualWrapper* result = new nsXTFXMLVisualWrapper(ni, xtfElement);
  if (!result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);

  nsresult rv = result->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(result);
    return rv;
  }

  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports implementation


NS_IMPL_ADDREF_INHERITED(nsXTFXMLVisualWrapper,nsXTFXMLVisualWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFXMLVisualWrapper,nsXTFXMLVisualWrapperBase)

NS_INTERFACE_MAP_BEGIN(nsXTFXMLVisualWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIXTFXMLVisualWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIAnonymousContentCreator)
NS_INTERFACE_MAP_END_INHERITING(nsXTFXMLVisualWrapperBase)

//----------------------------------------------------------------------
// nsIXTFXMLVisualWrapper implementation:

// XXX nothing yet

//----------------------------------------------------------------------
// nsIAnonymousContentCreator implementation:

NS_IMETHODIMP
nsXTFXMLVisualWrapper::CreateAnonymousContent(nsIPresContext* aPresContext,
                                            nsISupportsArray& aAnonymousItems)
{
  nsCOMPtr<nsIDOMElement> element;
  mXTFElement->GetVisualContent(getter_AddRefs(element));
  
  aAnonymousItems.AppendElement(element);
  
//   NS_ASSERTION(mDocument, "no document; cannot create anonymous content");

//   nsCOMPtr<nsINodeInfoManager> nim;
//   mDocument->GetNodeInfoManager(*getter_AddRefs(nim));
  
//   nsCOMPtr<nsINameSpaceManager> nsm;
//   nsServiceManager::GetService(kNameSpaceManagerCID,
//                                NS_GET_IID(nsINameSpaceManager),
//                                getter_AddRefs(nsm));
  
//   XTLContentDescriptor* cd = GetViewDescription().mDescriptorTree;
  
// 	// see if we're using the primary press shell or not
// 	nsCOMPtr<nsIPresShell> presShell, primaryShell;
// 	aPresContext->GetShell(getter_AddRefs(presShell));
	
// 	mDocument->GetShellAt(0, getter_AddRefs(primaryShell));
// 	bool isPrimaryShell = (primaryShell == presShell);
	
//   // ensure we clear out the old inheritedAttributes map
//   mInheritedAttributes.clear();

// 	bool is_root=true;
// 	if (! isPrimaryShell) { // don't build content, just clone what we have
// #ifdef DEBUG
// 		// assert if someone tries this with multiple top-level content descriptors
// 		int contentDescriptorCount=0;
// 		for (; cd != NULL; cd = cd->nextSibling) ++contentDescriptorCount;
// 		NS_ASSERTION(contentDescriptorCount==1, "nsXTFXMLVisualWrapper::CreateAnonymousContent: \
// 			multiple top level elements not supported at present");
// #endif		
// 		nsCOMPtr<nsIDOMNode> clonedViewTree;
// 		nsresult rv = mViewTree->CloneNode(PR_TRUE, getter_AddRefs(clonedViewTree));
// 		if (NS_FAILED(rv)) return rv;
			
// 		aAnonymousItems.AppendElement(clonedViewTree);
// 		return NS_OK;
// 	}
	
//   while (cd) {
//     nsCOMPtr<nsIContent> content;
//     CreateContentElement(cd->mNamespaceID, cd->mTag,cd->firstAttribute, nim, nsm, getter_AddRefs(content));

//     NS_ASSERTION(content, "could not create content");
    
//     if (is_root && content) {
//       // map first element to member variable:
//       content->QueryInterface(nsIDOMElement::GetIID(), getter_AddRefs(mViewTree));
// 	is_root=false;
//     }
    
//     if ( cd->firstChild )
//       CreateNode( content, cd->firstChild, nim, nsm );
    
//     aAnonymousItems.AppendElement(content);
//     cd = cd->nextSibling;
//   }

//   ViewTreeConstructedHook();
  
  return NS_OK;
}

// void nsXTFXMLVisualWrapper::CreateNode(nsIContent* parent, XTLContentDescriptor* descriptor,
//                               nsINodeInfoManager* nim, nsINameSpaceManager* nsm,
//                               nsIDOMNode**node)
// {    
//   nsCOMPtr<nsIContent> content;
//   CreateContentElement(descriptor->mNamespaceID, descriptor->mTag, descriptor->firstAttribute, 
//                        nim, nsm, getter_AddRefs(content)); 
//   NS_ASSERTION(content, "could not create content");
//   if (content && node) {
//     content->QueryInterface(nsIDOMNode::GetIID(), (void**)node);
//   }
  
//   // iterate over children, if there are any
//   if (descriptor->firstChild)
//     CreateNode(content, descriptor->firstChild, nim, nsm);

//   // append ourselves to 
//   parent->AppendChildTo( content, false, true );

//   // iterate over siblings:
//   if (descriptor->nextSibling)
//     CreateNode(parent, descriptor->nextSibling, nim, nsm);
// }

// void nsXTFXMLVisualWrapper::CreateContentElement(PRInt32 ns, nsIAtom* tagname, XTLAttributeDescriptor* attribs,
//                                         nsINodeInfoManager* nim,
//                                         nsINameSpaceManager* nsm, nsIContent **content)
// {
//   nsCOMPtr<nsIElementFactory> elementFactory;
//   nsm->GetElementFactory(ns, getter_AddRefs(elementFactory));
//   if (!elementFactory) {
//     NS_ERROR("no element factory in nsXTFXMLVisualWrapper");
//     return;
//   }
  
//   nsCOMPtr<nsINodeInfo> nodeinfo;
//   nim->GetNodeInfo(tagname, nsnull, ns, *getter_AddRefs(nodeinfo));
  
//   elementFactory->CreateInstanceByTag(nodeinfo, content);
//   while (attribs) {
//     if (!attribs->mIsInherited)
//       (*content)->SetAttr(kNameSpaceID_None, attribs->mName, attribs->mValue, PR_FALSE);
//     else {

//       mInheritedAttributes.insert( std::pair< nsCOMPtr<nsIAtom>, Content >( attribs->mName, Content( attribs->mMapTo, *content ) ) );

//       nsString value;
//       GetAttr(kNameSpaceID_None, attribs->mName, value);
//       (*content)->SetAttr(kNameSpaceID_None, attribs->mMapTo, value, PR_FALSE);
//     }
//     attribs = attribs->nextSibling;
//   }

// }

