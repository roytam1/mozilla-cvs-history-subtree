/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsRDFElement.h"
#include "nsIDocument.h"
#include "nsIAtom.h"
#include "nsIEventListenerManager.h"
#include "nsIHTMLAttributes.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsIRDFNode.h"
#include "nsIRDFResourceManager.h"
#include "nsIRDFDocument.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFCursor.h"

#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"

static NS_DEFINE_IID(kIRDFContentIID,         NS_IRDFCONTENT_IID);
static NS_DEFINE_IID(kIXMLContentIID,         NS_IXMLCONTENT_IID);
static NS_DEFINE_IID(kIRDFResourceManagerIID, NS_IRDFRESOURCEMANAGER_IID);
static NS_DEFINE_IID(kIRDFDocumentIID,        NS_IRDFDOCUMENT_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,      NS_IRDFDATASOURCE_IID);

static NS_DEFINE_CID(kRDFResourceManagerCID,  NS_RDFRESOURCEMANAGER_CID);

nsresult
NS_NewRDFElement(nsIRDFContent** result)
{
    NS_PRECONDITION(result, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsIRDFContent* element = new nsRDFElement();
    if (! element)
        return NS_ERROR_OUT_OF_MEMORY;

    return element->QueryInterface(kIRDFContentIID, (void**) result);
}

nsRDFElement::nsRDFElement(void)
    : mDocument(nsnull),
      mNameSpace(nsnull),
      mNameSpaceId(gNameSpaceId_Unknown),
      mScriptObject(nsnull),
      mResource(nsnull)
{
    NS_INIT_REFCNT();
}
 
nsRDFElement::~nsRDFElement()
{
    NS_IF_RELEASE(mNameSpace);
    NS_IF_RELEASE(mResource);
}

NS_IMPL_ADDREF(nsRDFElement);
NS_IMPL_RELEASE(nsRDFElement);

NS_IMETHODIMP 
nsRDFElement::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    if (iid.Equals(kIRDFContentIID) ||
        iid.Equals(kIXMLContentIID) ||
        iid.Equals(kIContentIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIRDFContent*>(this);
    }
    else if (iid.Equals(kIDOMElementIID) ||
             iid.Equals(kIDOMNodeIID)) {
        *result = static_cast<nsIDOMElement*>(this);
    }
    else if (iid.Equals(kIScriptObjectOwnerIID)) {
        *result = static_cast<nsIScriptObjectOwner*>(this);
    }
    else if (iid.Equals(kIDOMEventReceiverIID)) {
        *result = static_cast<nsIDOMEventReceiver*>(this);
    }
    else if (iid.Equals(kIJSScriptObjectIID)) {
        *result = static_cast<nsIJSScriptObject*>(this);
    }
    else {
        *result = NULL;
        return NS_NOINTERFACE;
    }

    // if we get here, we know one of the above IIDs was ok.
    AddRef();
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// nsIDOMNode

NS_IMETHODIMP
nsRDFElement::GetNodeName(nsString& aNodeName)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetNodeValue(nsString& aNodeValue)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetNodeValue(const nsString& aNodeValue)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetNodeType(PRUint16* aNodeType)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetParentNode(nsIDOMNode** aParentNode)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetFirstChild(nsIDOMNode** aFirstChild)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetLastChild(nsIDOMNode** aLastChild)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetNextSibling(nsIDOMNode** aNextSibling)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::HasChildNodes(PRBool* aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    nsRDFElement* it = new nsRDFElement();
    if (! it)
        return NS_ERROR_OUT_OF_MEMORY;

    return it->QueryInterface(kIDOMNodeIID, (void**) aReturn);
}


////////////////////////////////////////////////////////////////////////
// nsIDOMElement

NS_IMETHODIMP
nsRDFElement::GetTagName(nsString& aTagName)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetDOMAttribute(const nsString& aName, nsString& aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::SetDOMAttribute(const nsString& aName, const nsString& aValue)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::RemoveAttribute(const nsString& aName)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetAttributeNode(const nsString& aName, nsIDOMAttr** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetElementsByTagName(const nsString& aName, nsIDOMNodeList** aReturn)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::Normalize()
{
    return NS_OK;
}



////////////////////////////////////////////////////////////////////////
// nsIDOMEventReceiver

NS_IMETHODIMP
nsRDFElement::AddEventListener(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::RemoveEventListener(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetListenerManager(nsIEventListenerManager** aInstancePtrResult)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetNewListenerManager(nsIEventListenerManager **aInstancePtrResult)
{
    return NS_OK;
}



////////////////////////////////////////////////////////////////////////
// nsIScriptObjectOwner

NS_IMETHODIMP 
nsRDFElement::GetScriptObject(nsIScriptContext* aContext, void** aScriptObject)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsRDFElement::SetScriptObject(void *aScriptObject)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
// nsIJSScriptObject

PRBool
nsRDFElement::AddProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    return PR_FALSE;
}

PRBool
nsRDFElement::DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    return PR_FALSE;
}

PRBool
nsRDFElement::GetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    return PR_FALSE;
}

PRBool
nsRDFElement::SetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    return PR_FALSE;
}

PRBool
nsRDFElement::EnumerateProperty(JSContext *aContext)
{
    return PR_FALSE;
}


PRBool
nsRDFElement::Resolve(JSContext *aContext, jsval aID)
{
    return PR_FALSE;
}


PRBool
nsRDFElement::Convert(JSContext *aContext, jsval aID)
{
    return PR_FALSE;
}


void
nsRDFElement::Finalize(JSContext *aContext)
{
}



////////////////////////////////////////////////////////////////////////
// nsIConent

NS_IMETHODIMP
nsRDFElement::GetDocument(nsIDocument*& aResult) const
{
    NS_IF_ADDREF(mDocument);
    aResult = mDocument;
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetDocument(nsIDocument* aDocument, PRBool aDeep)
{
    NS_PRECONDITION(!aDeep, "nsRDFElement: deep SetDocument not implemented"); // XXX
    
    NS_IF_RELEASE(mDocument);
    mDocument = aDocument;
    NS_IF_ADDREF(mDocument);
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetParent(nsIContent*& aResult) const
{
    // traverse the property that represents "parenthood", and return
    // that result.
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetParent(nsIContent* aParent)
{
    // modify the property that represents "parenthood"
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::CanContainChildren(PRBool& aResult) const
{
    // XXX is this always the case? we probably need to examine the
    // resource and see if it's a container...
    aResult = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::ChildCount(PRInt32& aResult) const
{
    // count the number of outbound assertions from this resource for
    // the property that represents "childhood".

    if (!mResource || !mDocument)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;
    nsIRDFResourceManager* mgr;
    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFResourceManagerCID,
                                                    kIRDFResourceManagerIID,
                                                    (nsISupports**) &mgr)))
        return rv;

    aResult = 0;
    nsIRDFDocument* rdfDoc;
    if (NS_SUCCEEDED(rv = mDocument->QueryInterface(kIRDFDocumentIID, (void**) &rdfDoc))) {
        nsIRDFDataSource* ds;
        if (NS_SUCCEEDED(rv = rdfDoc->GetDataSource(ds))) {
            nsIRDFCursor* properties;
            if (NS_SUCCEEDED(rv = ds->ArcLabelsOut(mResource, properties))) {
                PRBool moreProperties;
                while (NS_SUCCEEDED(properties->HasMoreElements(moreProperties)) &&
                       moreProperties) {
                    nsIRDFNode* property;
                    PRBool b;
                    properties->GetNext(property, b);

                    nsIRDFCursor* values;
                    if (NS_SUCCEEDED(rv = ds->GetTargets(mResource, property, PR_TRUE, values))) {
                        PRBool moreValues;

                        while (NS_SUCCEEDED(values->HasMoreElements(moreValues)) &&
                               moreValues) {
                            nsIRDFNode* value;
                            PRBool tv;

                            values->GetNext(value, tv);
                            ++aResult;

                            NS_RELEASE(value);
                        }
                        NS_RELEASE(values);
                    }

                    NS_RELEASE(property);
                }
                NS_RELEASE(properties);
            }
            NS_RELEASE(ds);
        }
        NS_RELEASE(rdfDoc);
    }

    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);
    return rv;
}

NS_IMETHODIMP
nsRDFElement::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
    // return the index-th "child" by traversing the arcs leading from
    // this resource for the property that represents "childhood"
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::IsSynthetic(PRBool& aResult)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetTag(nsIAtom*& aResult) const
{
    // XXX the problem with this is that we only have a
    // fully-qualified URI, not "just" the tag. And I think the style
    // system ain't gonna work right with that...
    return mResource->GetAtomValue(aResult);
}


NS_IMETHODIMP 
nsRDFElement::SetAttribute(const nsString& aName, 
                           const nsString& aValue,
                           PRBool aNotify)
{
    return NS_OK;
}


NS_IMETHODIMP
nsRDFElement::GetAttribute(const nsString& aName, nsString& aResult) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify)
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetAllAttributeNames(nsISupportsArray* aArray, PRInt32& aResult) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::GetAttributeCount(PRInt32& aResult) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::List(FILE* out, PRInt32 aIndent) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::BeginConvertToXIF(nsXIFConverter& aConverter) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::ConvertContentToXIF(nsXIFConverter& aConverter) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::FinishConvertToXIF(nsXIFConverter& aConverter) const
{
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SizeOf(nsISizeOfHandler* aHandler) const
{
    return NS_OK;
}



NS_IMETHODIMP 
nsRDFElement::HandleDOMEvent(nsIPresContext& aPresContext,
                             nsEvent* aEvent,
                             nsIDOMEvent** aDOMEvent,
                             PRUint32 aFlags,
                             nsEventStatus& aEventStatus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


////////////////////////////////////////////////////////////////////////
// nsIXMLContent

NS_IMETHODIMP 
nsRDFElement::SetNameSpace(nsIAtom* aNameSpace)
{
    NS_IF_RELEASE(mNameSpace);
    mNameSpace = aNameSpace;
    NS_IF_ADDREF(mNameSpace);
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFElement::GetNameSpace(nsIAtom*& aNameSpace)
{
    aNameSpace = mNameSpace;
    NS_IF_ADDREF(mNameSpace);
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetNameSpaceIdentifier(PRInt32 aNameSpaceId)
{
    mNameSpaceId = aNameSpaceId;
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFElement::GetNameSpaceIdentifier(PRInt32& aNameSpaceId)
{
    aNameSpaceId = mNameSpaceId;
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// nsIRDFContent

NS_IMETHODIMP
nsRDFElement::SetResource(const nsString& aURI)
{
    if (mResource)
        return NS_ERROR_ALREADY_INITIALIZED;

    nsresult rv;

    nsIRDFResourceManager* mgr = NULL;
    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFResourceManagerCID,
                                                    kIRDFResourceManagerIID,
                                                    (nsISupports**) &mgr)))
        return rv;
    
    rv = mgr->GetNode(aURI, mResource);
    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);

    return rv;
}

NS_IMETHODIMP
nsRDFElement::GetResource(nsString& rURI) const
{
    if (! mResource)
        return NS_ERROR_NOT_INITIALIZED;

    return mResource->GetStringValue(rURI);
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetProperty(const nsString& aPropertyURI, const nsString& aValue)
{
    if (!mResource || !mDocument)
        return NS_ERROR_NOT_INITIALIZED;

    nsString resource;
    mResource->GetStringValue(resource);

#ifdef DEBUG_waterson
    char buf[256];
    printf("assert [\n");
    printf("  %s\n", resource.ToCString(buf, sizeof(buf)));
    printf("  %s\n", aPropertyURI.ToCString(buf, sizeof(buf)));
    printf("  %s\n", aValue.ToCString(buf, sizeof(buf)));
    printf("]\n");
#endif

    nsresult rv;
    nsIRDFResourceManager* mgr = NULL;

    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFResourceManagerCID,
                                                    kIRDFResourceManagerIID,
                                                    (nsISupports**) &mgr)))
        return rv;
    
    nsIRDFNode *property = NULL, *value = NULL;
    nsIRDFDocument* rdfDoc;

    if (NS_FAILED(rv = mgr->GetNode(aPropertyURI, property)))
        goto done;

    if (NS_FAILED(rv = mgr->GetNode(aValue, value)))
        goto done;

    if (NS_SUCCEEDED(rv = mDocument->QueryInterface(kIRDFDocumentIID, (void**) &rdfDoc))) {
        nsIRDFDataSource* ds;
        if (NS_SUCCEEDED(rv = rdfDoc->GetDataSource(ds))) {
            rv = ds->Assert(mResource, property, value, PR_TRUE);
            NS_RELEASE(ds);
        }
        NS_RELEASE(rdfDoc);
    }

done:
    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);
    NS_IF_RELEASE(property);
    NS_IF_RELEASE(value);

    return rv;
}

NS_IMETHODIMP
nsRDFElement::GetProperty(const nsString& aPropertyURI, nsString& rValue) const
{
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Implementation methods

