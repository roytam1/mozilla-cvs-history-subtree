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
#include "nsISupportsArray.h"

#include "nsIEventStateManager.h"
#include "nsDOMEvent.h"

////////////////////////////////////////////////////////////////////////
// RDF core vocabulary

#include "rdf.h"
#define RDF_NAMESPACE_URI  "http://www.w3.org/TR/WD-rdf-syntax#"
static const char kRDFNameSpaceURI[] = RDF_NAMESPACE_URI;
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, Alt);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, Bag);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, Description);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, ID);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, RDF);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, Seq);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, about);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, aboutEach);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, bagID);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, instanceOf);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, li);
DEFINE_RDF_VOCAB(RDF_NAMESPACE_URI, RDF, resource);

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIDocumentIID,           NS_IDOCUMENT_IID);
static NS_DEFINE_IID(kIRDFContentIID,         NS_IRDFCONTENT_IID);
static NS_DEFINE_IID(kIXMLContentIID,         NS_IXMLCONTENT_IID);
static NS_DEFINE_IID(kIRDFResourceManagerIID, NS_IRDFRESOURCEMANAGER_IID);
static NS_DEFINE_IID(kIRDFDocumentIID,        NS_IRDFDOCUMENT_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,      NS_IRDFDATASOURCE_IID);

static NS_DEFINE_CID(kRDFResourceManagerCID,  NS_RDFRESOURCEMANAGER_CID);

////////////////////////////////////////////////////////////////////////

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
      mResource(nsnull),
      mChildren(nsnull),
      mParent(nsnull)
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
    nsIDocument* doc;
    nsresult rv = mDocument->QueryInterface(kIDocumentIID, (void**) &doc);
    aResult = doc; // implicit AddRef() from QI
    return rv;
}

NS_IMETHODIMP
nsRDFElement::SetDocument(nsIDocument* aDocument, PRBool aDeep)
{
    NS_PRECONDITION(!aDeep, "nsRDFElement: deep SetDocument not implemented"); // XXX
    if (aDeep)
        return NS_ERROR_NOT_IMPLEMENTED;

    NS_PRECONDITION(aDocument, "null ptr");
    if (!aDocument)
        return NS_ERROR_NULL_POINTER;

    NS_IF_RELEASE(mDocument);
    return aDocument->QueryInterface(kIRDFDocumentIID,
                                     (void**) &mDocument); // implicit AddRef()
}

NS_IMETHODIMP
nsRDFElement::GetParent(nsIContent*& aResult) const
{
    // XXX traverse the property that represents "parenthood", and return
    // that result?

    if (!mParent)
        return NS_ERROR_NOT_INITIALIZED;

    aResult = mParent;
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::SetParent(nsIContent* aParent)
{
    // XXX don't allow modification of parents through this interface method?
    PR_ASSERT(! mParent);
    if (mParent)
        return NS_ERROR_ALREADY_INITIALIZED;

    PR_ASSERT(aParent);
    if (!aParent)
        return NS_ERROR_NULL_POINTER;

    mParent = aParent;
    NS_IF_ADDREF(mParent);
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
    nsresult rv;
    if (!mChildren) {
        if (NS_FAILED(rv = GenerateChildren()))
            return rv;
    }

    aResult = mChildren->Count();
    return NS_OK;
}

NS_IMETHODIMP
nsRDFElement::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
    nsresult rv;
    if (!mChildren) {
        if (NS_FAILED(rv = GenerateChildren()))
            return rv;
    }

    nsISupports* obj = mChildren->ElementAt(aIndex);
    nsIContent* content;
    rv = obj->QueryInterface(kIContentIID, (void**) &content);
    obj->Release();

    aResult = content;
    return rv;
}

NS_IMETHODIMP
nsRDFElement::IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRDFElement::InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsRDFElement::ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify)
{
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsRDFElement::AppendChildTo(nsIContent* aKid, PRBool aNotify)
{
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsRDFElement::RemoveChildAt(PRInt32 aIndex, PRBool aNotify)
{
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsRDFElement::IsSynthetic(PRBool& aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsRDFElement::GetTag(nsIAtom*& aResult) const
{
    PR_ASSERT(mResource);
    if (! mResource)
        return NS_ERROR_NOT_INITIALIZED;

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
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP
nsRDFElement::GetAttribute(const nsString& aName, nsString& aResult) const
{
    nsresult rv;
    nsIRDFResourceManager* mgr = NULL;
    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFResourceManagerCID,
                                                    kIRDFResourceManagerIID,
                                                    (nsISupports**) &mgr)))
        return rv;
    
    nsIRDFNode* property;
    if (NS_SUCCEEDED(rv = mgr->GetNode(aName, property))) {
        
        NS_RELEASE(property);
    }

    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);
    return rv;
}

NS_IMETHODIMP
nsRDFElement::UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify)
{
    PR_ASSERT(0); // this should be done via RDF
    return NS_ERROR_UNEXPECTED;
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
    
    rv = mgr->GetNode(aURI, mResource); // implicit AddRef()
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
    
    nsIRDFNode* property = NULL;
    nsIRDFNode* value = NULL;
    nsIRDFDataSource* ds = NULL;

    if (NS_FAILED(rv = mgr->GetNode(aPropertyURI, property)))
        goto done;

    if (NS_FAILED(rv = mgr->GetNode(aValue, value)))
        goto done;

    if (NS_FAILED(rv = mDocument->GetDataSource(ds)))
        goto done;

    rv = ds->Assert(mResource, property, value, PR_TRUE);

done:
    NS_IF_RELEASE(ds);
    NS_IF_RELEASE(value);
    NS_IF_RELEASE(property);
    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);

    return rv;
}

NS_IMETHODIMP
nsRDFElement::GetProperty(const nsString& aPropertyURI, nsString& rValue) const
{
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// Implementation methods

static PRBool
rdf_IsOrdinalProperty(const nsString& uri)
{
    if (uri.Find(kRDFNameSpaceURI) != 0)
        return PR_FALSE;

    nsAutoString tag(uri);
    tag.Cut(0, sizeof(kRDFNameSpaceURI) - 1);

    if (tag[0] != '_')
        return PR_FALSE;

    for (PRInt32 i = tag.Length() - 1; i >= 1; --i) {
        if (tag[i] < '0' || tag[i] > '9')
            return PR_FALSE;
    }

    return PR_TRUE;
}

nsresult
nsRDFElement::GenerateChildren(void) const
{
    // count the number of outbound assertions from this resource for
    // the property that represents "childhood".
    nsresult rv;

    if (!mResource || !mDocument)
        return NS_ERROR_NOT_INITIALIZED;

    if (! mChildren) {
        if (NS_FAILED(rv = NS_NewISupportsArray(&mChildren)))
            return rv;
    }
    else {
        mChildren->Clear();
    }

    nsIRDFResourceManager* mgr;
    if (NS_FAILED(rv = nsServiceManager::GetService(kRDFResourceManagerCID,
                                                    kIRDFResourceManagerIID,
                                                    (nsISupports**) &mgr)))
        return rv;

    nsIRDFDataSource* ds = NULL;
    nsIRDFNode *RDF_instanceOf = NULL, *RDF_Bag = NULL;
    nsIRDFCursor* properties = NULL;
    PRBool moreProperties;
    PRBool isBag;

    if (NS_FAILED(rv = mDocument->GetDataSource(ds)))
        goto done;

    if (NS_FAILED(rv = mgr->GetNode(kURIRDF_instanceOf, RDF_instanceOf)))
        goto done;
    
    if (NS_FAILED(rv = mgr->GetNode(kURIRDF_Bag, RDF_Bag)))
        goto done;

    if (NS_FAILED(rv = ds->HasAssertion(mResource, RDF_instanceOf, RDF_Bag, PR_TRUE, isBag)))
        goto done;

    if (! isBag)
        goto done;

    if (NS_FAILED(rv = ds->ArcLabelsOut(mResource, properties)))
        goto done;

    while (NS_SUCCEEDED(rv = properties->HasMoreElements(moreProperties)) && moreProperties) {
        nsIRDFNode* property = NULL;
        PRBool tv;

        if (NS_FAILED(rv = properties->GetNext(property, tv /* ignored */)))
            break;

        nsAutoString uri;
        if (NS_FAILED(rv = property->GetStringValue(uri))) {
            NS_RELEASE(property);
            break;
        }

        if (! rdf_IsOrdinalProperty(uri)) {
            NS_RELEASE(property);
            continue;
        }

        nsIRDFCursor* values;
        if (NS_SUCCEEDED(rv = ds->GetTargets(mResource, property, PR_TRUE, values))) {
            PRBool moreValues;
            while (NS_SUCCEEDED(rv = values->HasMoreElements(moreValues)) && moreValues) {
                nsIRDFNode* value = NULL;

                if (NS_FAILED(rv = values->GetNext(value, tv /* ignored */)))
                    break;

                nsRDFElement* child;

                // Create and initialize a new child element
                child = new nsRDFElement();
                if (! child) {
                    NS_RELEASE(value);
                    rv = NS_ERROR_OUT_OF_MEMORY;
                    break;
                }

                child->mDocument = mDocument;
                child->mResource = value;
                child->mParent   = static_cast<nsIContent*>(const_cast<nsRDFElement*>(this));

                mChildren->AppendElement(static_cast<nsIRDFContent*>(child));
            }
            NS_IF_RELEASE(values);
        }

        NS_RELEASE(property);

        if (NS_FAILED(rv))
            break;
    }

done:
    NS_IF_RELEASE(properties);
    NS_IF_RELEASE(RDF_Bag);
    NS_IF_RELEASE(RDF_instanceOf);
    NS_IF_RELEASE(ds);
    nsServiceManager::ReleaseService(kRDFResourceManagerCID, mgr);

    return rv;
}


