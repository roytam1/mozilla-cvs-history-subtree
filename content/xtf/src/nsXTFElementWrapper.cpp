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

#include "nsXTFElementWrapper.h"
#include "nsIXTFElement.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXTFInterfaceAggregator.h"
#include "nsIClassInfo.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
//XXX get rid of this:
#include "nsSVGAtoms.h"

nsXTFElementWrapper::nsXTFElementWrapper(nsINodeInfo* aNodeInfo)
    : nsXTFElementWrapperBase(aNodeInfo)
{
}

//----------------------------------------------------------------------
// nsISupports implementation

NS_IMPL_ADDREF_INHERITED(nsXTFElementWrapper,nsXTFElementWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFElementWrapper,nsXTFElementWrapperBase)

NS_IMETHODIMP
nsXTFElementWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv;
  
  if(aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    *aInstancePtr = NS_STATIC_CAST(nsIClassInfo*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if(aIID.Equals(NS_GET_IID(nsIXTFElementWrapper))) {
    *aInstancePtr = NS_STATIC_CAST(nsIXTFElementWrapper*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (NS_SUCCEEDED(rv = nsXTFElementWrapperBase::QueryInterface(aIID, aInstancePtr))) {
    return rv;
  }
  else if (AggregatesInterface(aIID)) {
#ifdef DEBUG
//    printf("nsXTFElementWrapper::QueryInterface(): creating aggregation tearoff\n");
#endif
    return NS_NewXTFInterfaceAggregator(aIID, GetXTFElement(), (nsIContent*)this,
                                        (nsISupports**)aInstancePtr);
  }

  return NS_ERROR_NO_INTERFACE;
}

//----------------------------------------------------------------------
// nsIContent:

void
nsXTFElementWrapper::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                 PRBool aCompileEventHandlers)
{
  GetXTFElement()->WillChangeDocument(aDocument);
  nsXTFElementWrapperBase::SetDocument(aDocument, aDeep, aCompileEventHandlers);
  GetXTFElement()->DocumentChanged(aDocument);
}

void
nsXTFElementWrapper::SetParent(nsIContent* aParent)
{
  GetXTFElement()->WillChangeParent(aParent);
  nsXTFElementWrapperBase::SetParent(aParent);
  GetXTFElement()->ParentChanged(aParent);
}

nsresult
nsXTFElementWrapper::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                   PRBool aNotify, PRBool aDeepSetDocument)
{
  nsresult rv;
  GetXTFElement()->WillInsertChild(aKid, aIndex);
  rv = nsXTFElementWrapperBase::InsertChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
  GetXTFElement()->ChildInserted(aKid, aIndex);
  return rv;
}

nsresult
nsXTFElementWrapper::AppendChildTo(nsIContent* aKid, PRBool aNotify,
                                   PRBool aDeepSetDocument)
{
  nsresult rv;
  GetXTFElement()->WillAppendChild(aKid);
  rv = nsXTFElementWrapperBase::AppendChildTo(aKid, aNotify, aDeepSetDocument);
  GetXTFElement()->ChildAppended(aKid);
  return rv;
}

nsresult
nsXTFElementWrapper::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv;
  GetXTFElement()->WillRemoveChild(aIndex);
  rv = nsXTFElementWrapperBase::RemoveChildAt(aIndex, aNotify);
  GetXTFElement()->ChildRemoved(aIndex);
  return rv;
}

nsIAtom *
nsXTFElementWrapper::GetIDAttributeName() const
{
  // XXX:
  return nsSVGAtoms::id;
}

nsresult
nsXTFElementWrapper::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  nsAutoString name;
  aName->ToString(name);
  
  nsresult rv = GetXTFElement()->SetAttribute(name, aValue);

  // XXX mutation events?
  
  return rv;
}

nsresult
nsXTFElementWrapper::GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, 
                             nsAString& aResult)const
{
  nsAutoString name;
  aName->ToString(name);
  nsresult rv = GetXTFElement()->GetAttribute(name, aResult);
  if (NS_FAILED(rv)) return rv;
  if (aResult.IsVoid()) return NS_CONTENT_ATTR_NOT_THERE;
  // XXX when should we return NS_CONTENT_ATTR_NO_VALUE ???
  
  return NS_CONTENT_ATTR_HAS_VALUE;
}

PRBool
nsXTFElementWrapper::HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const
{
  nsAutoString name;
  aName->ToString(name);
  PRBool rval = PR_FALSE;
  GetXTFElement()->HasAttribute(name, &rval);
  
  return rval;
}


nsresult
nsXTFElementWrapper::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                                      PRBool aNotify)
{
  nsAutoString name;
  aAttr->ToString(name);
  
  nsresult rv = GetXTFElement()->UnsetAttribute(name);

  // XXX mutation events?
  
  return rv;
}

nsresult
nsXTFElementWrapper::GetAttrNameAt(PRUint32 aIndex, PRInt32* aNameSpaceID,
                                   nsIAtom** aName, nsIAtom** aPrefix) const
{
  *aNameSpaceID = kNameSpaceID_None;
  *aPrefix = nsnull;
  
  nsAutoString name;
  nsresult rv = GetXTFElement()->GetAttributeNameAt(aIndex, name);
  if (NS_SUCCEEDED(rv))
    *aName = NS_NewAtom(name);
  else
    *aName = nsnull;
  
  return rv;
}

PRUint32
nsXTFElementWrapper::GetAttrCount() const
{
  PRUint32 rval = 0;
  GetXTFElement()->GetAttributeCount(&rval);
  return rval;
}


//----------------------------------------------------------------------
// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP 
nsXTFElementWrapper::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  return GetXTFElement()->GetScriptingInterfaces(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP 
nsXTFElementWrapper::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassID(nsCID * *aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP 
nsXTFElementWrapper::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

//----------------------------------------------------------------------
// nsIXTFElementWrapper implementation:

/* readonly attribute nsIDOMElement elementNode; */
NS_IMETHODIMP
nsXTFElementWrapper::GetElementNode(nsIDOMElement * *aElementNode)
{
  *aElementNode = (nsIDOMElement*)this;
  NS_ADDREF(*aElementNode);
  return NS_OK;
}

/* readonly attribute nsIDOMElement documentFrameElement; */
NS_IMETHODIMP
nsXTFElementWrapper::GetDocumentFrameElement(nsIDOMElement * *aDocumentFrameElement)
{
  *aDocumentFrameElement = nsnull;
  
  if (!mDocument) {
    NS_WARNING("no document");
    return NS_OK;
  }
  nsCOMPtr<nsISupports> container = mDocument->GetContainer();
  if (!container) {
    NS_ERROR("no docshell");
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsPIDOMWindow> pidomwin = do_GetInterface(container);
  if (!pidomwin) {
    NS_ERROR("no nsPIDOMWindow interface on docshell");
    return NS_ERROR_FAILURE;
  }
  *aDocumentFrameElement = pidomwin->GetFrameElementInternal();
  NS_IF_ADDREF(*aDocumentFrameElement);
  return NS_OK;
}

//----------------------------------------------------------------------
// implementation helpers:
PRBool
nsXTFElementWrapper::AggregatesInterface(REFNSIID aIID)
{
  // We must ensure that the inner element has a distinct xpconnect
  // identity, so we mustn't aggregate nsIXPConnectWrappedJS:
  if (aIID.Equals(NS_GET_IID(nsIXPConnectWrappedJS))) return PR_FALSE;

  nsCOMPtr<nsISupports> inst;
  GetXTFElement()->QueryInterface(aIID, getter_AddRefs(inst));
  return (inst!=nsnull);
//   // we aggregate all interfaces declared as 'public' in our inner
//   // object:
//   // XXX we call this so often, it should almost certainly be hashed.
//   PRUint32 count=0;
//   nsIID **iids=nsnull;
  
//   GetXTFElement()->GetPublicInterfaces(&count, &iids);
//   for (int i=0; i<count; ++i) {
//     if(aIID.Equals(*(iids[i]))) {
// #ifdef DEBUG
//       printf("nsXTFElementWrapper::AggregatesInterface(): found!\n");
// #endif
//       break;
//     }
//   }

//   if (iids!=nsnull)
//     NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, iids);
  
//   return i!=count;
}
