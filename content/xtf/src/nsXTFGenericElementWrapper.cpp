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
#include "nsIXTFGenericElement.h"
#include "nsString.h"
#include "nsXMLElement.h"
#include "nsXTFInterfaceAggregator.h"
#include "nsXTFWeakTearoff.h"
#include "nsIClassInfo.h"
#include "nsIXTFGenericElementWrapper.h"
//XXX get rid of this:
#include "nsSVGAtoms.h"

typedef nsXMLElement nsXTFGenericElementWrapperBase;

////////////////////////////////////////////////////////////////////////
// nsXTFGenericElementWrapper class
class nsXTFGenericElementWrapper : public nsXTFGenericElementWrapperBase,    // :nsIHTMLContent:nsIStyledContent:nsIContent
                              public nsIClassInfo,
                              public nsIXTFGenericElementWrapper
{
protected:
  friend nsresult
  NS_NewXTFGenericElementWrapper(nsIXTFGenericElement* xtfElement,
                            nsINodeInfo* ni,
                            nsIContent** aResult);

  nsXTFGenericElementWrapper(nsINodeInfo* aNodeInfo, nsIXTFGenericElement* xtfElement);
  virtual ~nsXTFGenericElementWrapper();

  nsresult Init();
  
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

  // nsIXTFGenericElementWrapper interface
  NS_DECL_NSIXTFGENERICELEMENTWRAPPER
  
private:
  // implementation helpers:  
  PRBool AggregatesInterface(REFNSIID aIID);

  
  nsCOMPtr<nsIXTFGenericElement> mXTFElement;
};

//----------------------------------------------------------------------
// implementation:

nsXTFGenericElementWrapper::nsXTFGenericElementWrapper(nsINodeInfo* aNodeInfo,
                                                       nsIXTFGenericElement* xtfElement)
    : nsXTFGenericElementWrapperBase(aNodeInfo), mXTFElement(xtfElement)
{
#ifdef DEBUG
  printf("nsXTFGenericElementWrapper CTOR\n");
#endif
  NS_ASSERTION(mXTFElement, "xtfElement is null");
}

nsresult
nsXTFGenericElementWrapper::Init()
{
  // pass a weak wrapper (non base object ref-counted), so that
  // our mXTFElement can safely addref/release.
  nsISupports *weakWrapper=nsnull;
  NS_NewXTFWeakTearoff(NS_GET_IID(nsIXTFGenericElementWrapper),
                       (nsIXTFGenericElementWrapper*)this,
                       &weakWrapper);
  if (!weakWrapper) {
    NS_ERROR("could not construct weak wrapper");
    return NS_ERROR_FAILURE;
  }
  
  mXTFElement->OnCreated((nsIXTFGenericElementWrapper*)weakWrapper);
  weakWrapper->Release();
  
  return NS_OK;
}

nsXTFGenericElementWrapper::~nsXTFGenericElementWrapper()
{
  mXTFElement->OnDestroyed();
  mXTFElement = nsnull;
  
#ifdef DEBUG
  printf("nsXTFGenericElementWrapper DTOR\n");
#endif
}

nsresult
NS_NewXTFGenericElementWrapper(nsIXTFGenericElement* xtfElement,
                               nsINodeInfo* ni,
                               nsIContent** aResult)
{
  *aResult = nsnull;
  
  if (!xtfElement) {
    NS_ERROR("can't construct an xtf wrapper without an xtf element");
    return NS_ERROR_FAILURE;
  }
  
  nsXTFGenericElementWrapper* result = new nsXTFGenericElementWrapper(ni, xtfElement);
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


NS_IMPL_ADDREF_INHERITED(nsXTFGenericElementWrapper,nsXTFGenericElementWrapperBase)
NS_IMPL_RELEASE_INHERITED(nsXTFGenericElementWrapper,nsXTFGenericElementWrapperBase)

NS_IMETHODIMP
nsXTFGenericElementWrapper::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  nsresult rv;
  
  if(aIID.Equals(NS_GET_IID(nsIClassInfo))) {
    *aInstancePtr = NS_STATIC_CAST(nsIClassInfo*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if(aIID.Equals(NS_GET_IID(nsIXTFGenericElementWrapper))) {
    *aInstancePtr = NS_STATIC_CAST(nsIXTFGenericElementWrapper*, this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  else if (NS_SUCCEEDED(rv = nsXTFGenericElementWrapperBase::QueryInterface(aIID, aInstancePtr))) {
    return rv;
  }
  else if (AggregatesInterface(aIID)) {
#ifdef DEBUG
    printf("nsXTFGenericElementWrapper::QueryInterface(): creating aggregation tearoff\n");
#endif
    return NS_NewXTFInterfaceAggregator(aIID, mXTFElement, (nsIContent*)this,
                                        (nsISupports**)aInstancePtr);
  }

  return NS_ERROR_NO_INTERFACE;
}

//----------------------------------------------------------------------
// nsIContent:

void
nsXTFGenericElementWrapper::SetDocument(nsIDocument* aDocument, PRBool aDeep,
                                        PRBool aCompileEventHandlers)
{
  mXTFElement->WillChangeDocument(aDocument);
  nsXTFGenericElementWrapperBase::SetDocument(aDocument, aDeep, aCompileEventHandlers);
  mXTFElement->DocumentChanged(aDocument);
}

void
nsXTFGenericElementWrapper::SetParent(nsIContent* aParent)
{
  mXTFElement->WillChangeParent(aParent);
  nsXTFGenericElementWrapperBase::SetParent(aParent);
  mXTFElement->ParentChanged(aParent);
}

nsresult
nsXTFGenericElementWrapper::InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                          PRBool aNotify, PRBool aDeepSetDocument)
{
  nsresult rv;
  mXTFElement->WillInsertChild(aKid, aIndex);
  rv = nsXTFGenericElementWrapperBase::InsertChildAt(aKid, aIndex, aNotify, aDeepSetDocument);
  mXTFElement->ChildInserted(aKid, aIndex);
  return rv;
}

nsresult
nsXTFGenericElementWrapper::AppendChildTo(nsIContent* aKid, PRBool aNotify,
                                          PRBool aDeepSetDocument)
{
  nsresult rv;
  mXTFElement->WillAppendChild(aKid);
  rv = nsXTFGenericElementWrapperBase::AppendChildTo(aKid, aNotify, aDeepSetDocument);
  mXTFElement->ChildAppended(aKid);
  return rv;
}

nsresult
nsXTFGenericElementWrapper::RemoveChildAt(PRUint32 aIndex, PRBool aNotify)
{
  nsresult rv;
  mXTFElement->WillRemoveChild(aIndex);
  rv = nsXTFGenericElementWrapperBase::RemoveChildAt(aIndex, aNotify);
  mXTFElement->ChildRemoved(aIndex);
  return rv;
}

nsIAtom *
nsXTFGenericElementWrapper::GetIDAttributeName() const
{
  // XXX:
  return nsSVGAtoms::id;
}

nsresult
nsXTFGenericElementWrapper::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                    nsIAtom* aPrefix, const nsAString& aValue,
                                    PRBool aNotify)
{
  nsAutoString name;
  aName->ToString(name);
  
  nsresult rv;
  mXTFElement->WillSetAttribute(name, aValue);
  rv = nsXTFGenericElementWrapperBase::SetAttr(aNameSpaceID, aName, aPrefix, aValue, aNotify);
  mXTFElement->AttributeSet(name, aValue);
  return rv;
}

nsresult
nsXTFGenericElementWrapper::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttr, 
                                      PRBool aNotify)
{
  nsAutoString name;
  aAttr->ToString(name);
  
  nsresult rv;
  mXTFElement->WillUnsetAttribute(name);
  rv = nsXTFGenericElementWrapperBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);
  mXTFElement->AttributeUnset(name);
  return rv;
}



//----------------------------------------------------------------------
// nsIClassInfo implementation

/* void getInterfaces (out PRUint32 count, [array, size_is (count), retval] out nsIIDPtr array); */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetInterfaces(PRUint32 *count, nsIID * **array)
{
  return mXTFElement->GetScriptingInterfaces(count, array);
}

/* nsISupports getHelperForLanguage (in PRUint32 language); */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
  *_retval = nsnull;
  return NS_OK;
}

/* readonly attribute string contractID; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetContractID(char * *aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

/* readonly attribute string classDescription; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetClassDescription(char * *aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

/* readonly attribute nsCIDPtr classID; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetClassID(nsCID * *aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

/* readonly attribute PRUint32 implementationLanguage; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::UNKNOWN;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::DOM_OBJECT;
  return NS_OK;
}

/* [notxpcom] readonly attribute nsCID classIDNoAlloc; */
NS_IMETHODIMP 
nsXTFGenericElementWrapper::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

//----------------------------------------------------------------------
// nsIXTFGenericElementWrapper implementation:

/* readonly attribute nsIDOMElement elementNode; */
NS_IMETHODIMP
nsXTFGenericElementWrapper::GetElementNode(nsIDOMElement * *aElementNode)
{
  *aElementNode = (nsIDOMElement*)this;
  NS_ADDREF(*aElementNode);
  return NS_OK;
}


//----------------------------------------------------------------------
// implementation helpers:
PRBool
nsXTFGenericElementWrapper::AggregatesInterface(REFNSIID aIID)
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
//       printf("nsXTFGenericElementWrapper::AggregatesInterface(): found!\n");
// #endif
//       break;
//     }
//   }

//   if (iids!=nsnull)
//     NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(count, iids);
  
//   return i!=count;
}
