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
#include "nsIXTFXMLVisual.h"
#include "nsString.h"
#include "nsXMLElement.h"
#include "nsIAnonymousContentCreator.h"
#include "nsXTFInterfaceAggregator.h"
#include "nsXTFWeakTearoff.h"
#include "nsIClassInfo.h"
#include "nsIXTFXMLVisualWrapper.h"
//XXX get rid of this:
#include "nsSVGAtoms.h"

typedef nsXMLElement nsXTFXMLVisualWrapperBase;

////////////////////////////////////////////////////////////////////////
// nsXTFXMLVisualWrapper class
class nsXTFXMLVisualWrapper : public nsXTFXMLVisualWrapperBase,    // :nsIHTMLContent:nsIStyledContent:nsIContent
                              public nsIClassInfo,
                              public nsIAnonymousContentCreator,
                              public nsIXTFXMLVisualWrapper
{
protected:
  friend nsresult
  NS_NewXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement,
                            nsINodeInfo* ni,
                            nsIContent** aResult);

  nsXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement);
  virtual ~nsXTFXMLVisualWrapper();
  nsresult Init(nsINodeInfo*ni);
  
public:
  // nsISupports interface
  NS_DECL_ISUPPORTS_INHERITED

  // nsIContent specialisations:
  void SetDocument(nsIDocument* aDocument, PRBool aDeep,
                   PRBool aCompileEventHandlers);
  void SetParent(nsIContent* aParent);
  nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                         PRBool aNotify, PRBool aDeepSetDocument);
  nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify,
                         PRBool aDeepSetDocument);
  nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  nsIAtom *GetIDAttributeName() const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   nsIAtom* aPrefix, const nsAString& aValue,
                   PRBool aNotify);
  nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                     PRBool aNotify);
  
  // nsIClassInfo interface
  NS_DECL_NSICLASSINFO

  // nsIXTFXMLVisualWrapper interface
  NS_DECL_NSIXTFXMLVISUALWRAPPER
  
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
  // implementation helpers:  
  PRBool AggregatesInterface(REFNSIID aIID);

  
  nsCOMPtr<nsIXTFXMLVisual> mXTFElement;
};

//----------------------------------------------------------------------
// implementation:

nsXTFXMLVisualWrapper::nsXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement)
    : mXTFElement(xtfElement)
{
#ifdef DEBUG
  printf("nsXTFXMLVisualWrapper CTOR\n");
#endif
}

nsresult
nsXTFXMLVisualWrapper::Init(nsINodeInfo* ni)
{
  nsresult rv = nsXTFXMLVisualWrapperBase::Init(ni);

  // pass a weak wrapper (non base object ref-counted), so that
  // our mXTFElement can safely addref/release.
  nsISupports *weakWrapper=nsnull;
  NS_NewXTFWeakTearoff(NS_GET_IID(nsIXTFXMLVisualWrapper),
                       (nsIXTFXMLVisualWrapper*)this,
                       &weakWrapper);
  mXTFElement->OnCreated((nsIXTFXMLVisualWrapper*)weakWrapper);
  weakWrapper->Release();
  
  return rv;
}

nsXTFXMLVisualWrapper::~nsXTFXMLVisualWrapper()
{
  mXTFElement->OnDestroyed();
  mXTFElement = nsnull;
  
#ifdef DEBUG
  printf("nsXTFXMLVisualWrapper DTOR\n");
#endif
}

nsresult
NS_NewXTFXMLVisualWrapper(nsIXTFXMLVisual* xtfElement,
                          nsINodeInfo* ni,
                          nsIContent** aResult)
{
  NS_PRECONDITION(aResult != nsnull, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  if (!xtfElement) {
    NS_ERROR("can't construct an xtf wrapper without an xtf element");
    return NS_ERROR_FAILURE;
  }
  
  nsXTFXMLVisualWrapper* result = new nsXTFXMLVisualWrapper(xtfElement);
  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);

  // XXX this might fail.
  result->Init(ni);

  *aResult = result;
  return NS_OK;
}

//----------------------------------------------------------------------
// nsISupports implementation


NS_IMPL_ADDREF_INHERITED(nsXTFXMLVisualWrapper,nsXTFXMLVisualWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFXMLVisualWrapper,nsXTFXMLVisualWrapperBase)

NS_IMETHODIMP
nsXTFXMLVisualWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (AggregatesInterface(aIID)) {
#ifdef DEBUG
    printf("nsXTFXMLVisualWrapper::QueryInterface(): creating aggregation tearoff\n");
#endif
    return NS_NewXTFInterfaceAggregator(aIID, mXTFElement, (nsIContent*)this,
                                        (nsISupports**)aInstancePtr);
  }
  else if(aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    *aInstancePtr = NS_STATIC_CAST(nsIClassInfo*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if(aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {
    *aInstancePtr = NS_STATIC_CAST(nsIAnonymousContentCreator*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if(aIID.Equals(NS_GET_IID(nsIXTFXMLVisualWrapper))) {
    *aInstancePtr = NS_STATIC_CAST(nsIXTFXMLVisualWrapper*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else
    return nsXTFXMLVisualWrapperBase::QueryInterface(aIID, aInstancePtr);
}

//----------------------------------------------------------------------
// nsIContent:

void
nsXTFXMLVisualWrapper::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                   PRBool aCompileEventHandlers)
{
  mXTFElement->WillChangeDocument(aDocument);
  nsXTFXMLVisualWrapperBase::SetDocument(aDocument, aDeep, aCompileEventHandlers);
  mXTFElement->DocumentChanged(aDocument);
}

void
nsXTFXMLVisualWrapper::SetParent(nsIContent* aParent)
{
  mXTFElement->WillChangeParent(aParent);
  nsXTFXMLVisualWrapperBase::SetParent(aParent);
  mXTFElement->ParentChanged(aParent);
}

nsresult
nsXTFXMLVisualWrapper::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                     PRBool aNotify, PRBool aDeepSetDocument)
{
  nsresult rv;
  mXTFElement->WillInsertChild(aKid, aIndex);
  rv = nsXTFXMLVisualWrapperBase::InsertChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
  mXTFElement->ChildInserted(aKid, aIndex);
  return rv;
}

nsresult
nsXTFXMLVisualWrapper::AppendChildTo(nsIContent* aKid, PRBool aNotify,
                                     PRBool aDeepSetDocument)
{
  nsresult rv;
  mXTFElement->WillAppendChild(aKid);
  rv = nsXTFXMLVisualWrapperBase::AppendChildTo(aKid, aNotify, aDeepSetDocument);
  mXTFElement->ChildAppended(aKid);
  return rv;
}

nsresult
nsXTFXMLVisualWrapper::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv;
  mXTFElement->WillRemoveChild(aIndex);
  rv = nsXTFXMLVisualWrapperBase::RemoveChildAt(aIndex, aNotify);
  mXTFElement->ChildRemoved(aIndex);
  return rv;
}

nsIAtom *
nsXTFXMLVisualWrapper::GetIDAttributeName() const
{
  // XXX:
  return nsSVGAtoms::id;
}

nsresult
nsXTFXMLVisualWrapper::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                               nsIAtom* aPrefix, const nsAString& aValue,
                               PRBool aNotify)
{
  nsAutoString name;
  aName->ToString(name);
  
  nsresult rv;
  mXTFElement->WillSetAttribute(name, aValue);
  rv = nsXTFXMLVisualWrapperBase::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);
  mXTFElement->AttributeSet(name, aValue);
  return rv;
}

nsresult
nsXTFXMLVisualWrapper::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                                 PRBool aNotify)
{
  nsAutoString name;
  aAttr->ToString(name);
  
  nsresult rv;
  mXTFElement->WillUnsetAttribute(name);
  rv = nsXTFXMLVisualWrapperBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);
  mXTFElement->AttributeUnset(name);
  return rv;
}

//----------------------------------------------------------------------
// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  return mXTFElement->GetScriptingInterfaces(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetClassID(nsCID * *aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP 
nsXTFXMLVisualWrapper::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

//----------------------------------------------------------------------
// nsIXTFXMLVisualWrapper implementation:

/* readonly attribute nsIDOMElement elementNode; */
NS_IMETHODIMP
nsXTFXMLVisualWrapper::GetElementNode(nsIDOMElement * *aElementNode)
{
  *aElementNode = (nsIDOMElement*)this;
  NS_ADDREF(*aElementNode);
  return NS_OK;
}


//----------------------------------------------------------------------
// nsIAnonymousContentCreator implementation:

NS_IMETHODIMP
nsXTFXMLVisualWrapper::CreateAnonymousContent(nsIPresContext* aPresContext,
                                            nsISupportsArray& aAnonymousItems)
{
  nsCOMPtr<nsISupportsArray> arr;
  mXTFElement->GetVisualContent(getter_AddRefs(arr));
  aAnonymousItems.AppendElements(arr);
  
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

//----------------------------------------------------------------------
// implementation helpers:
PRBool
nsXTFXMLVisualWrapper::AggregatesInterface(REFNSIID aIID)
{
  nsCOMPtr<nsISupports> inst;
  mXTFElement->QueryInterface(aIID, getter_AddRefs(inst));
  return (inst!=nsnull);
//   // we aggregate all interfaces declared as 'public' in our inner
//   // object:
//   // XXX we call this so often, it should almost certainly be hashed.
//   PRUint32 count=0;
//   nsIID **iids=nsnull;
  
//   mXTFElement->GetPublicInterfaces(&count, &iids);
//   for (int i=0; i<count; ++i) {
//     if(aIID.Equals(*(iids[i]))) {
// #ifdef DEBUG
//       printf("nsXTFXMLVisualWrapper::AggregatesInterface(): found!\n");
// #endif
//       break;
//     }
//   }

//   if (iids!=nsnull)
//     NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, iids);
  
//   return i!=count;
}
