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

#include "nsXULDocument.h"
#include "nsIComponentManager.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLElementFactory.h"
#include "nsIScriptContextOwner.h"
#include "nsITextContent.h"
#include "nsIUnicharStreamLoader.h"
#include "nsIXMLElementFactory.h"
#include "nsIXULContentUtils.h"
#include "nsIXULPrototypeCache.h"
#include "nsLayoutCID.h"
#include "nsNeckoUtil.h"
#include "nsXPIDLString.h"
#include "nsXULElement.h"
#include "prlog.h"

//----------------------------------------------------------------------
//
// CIDs
//

static NS_DEFINE_CID(kTextNodeCID,               NS_TEXTNODE_CID);

//----------------------------------------------------------------------
//
// nsXULDocument::Builder::ContextStack
//

nsXULDocument::Builder::ContextStack::ContextStack()
    : mTop(nsnull), mDepth(0)
{
}

nsXULDocument::Builder::ContextStack::~ContextStack()
{
    while (mTop) {
        Entry* doomed = mTop;
        mTop = mTop->mNext;
        NS_IF_RELEASE(doomed->mElement);
        delete doomed;
    }
}

nsresult
nsXULDocument::Builder::ContextStack::Push(nsXULPrototypeElement* aPrototype, nsIContent* aElement)
{
    Entry* entry = new Entry;
    if (! entry)
        return NS_ERROR_OUT_OF_MEMORY;

    entry->mPrototype = aPrototype;
    entry->mElement   = aElement;
    NS_IF_ADDREF(entry->mElement);
    entry->mIndex     = 0;

    entry->mNext = mTop;
    mTop = entry;

    ++mDepth;
    return NS_OK;
}

nsresult
nsXULDocument::Builder::ContextStack::Pop()
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    Entry* doomed = mTop;
    mTop = mTop->mNext;
    --mDepth;

    NS_RELEASE(doomed->mElement);
    delete doomed;
    return NS_OK;
}

nsresult
nsXULDocument::Builder::ContextStack::Peek(nsXULPrototypeElement** aPrototype,
                                             nsIContent** aElement,
                                             PRInt32* aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    *aPrototype = mTop->mPrototype;
    *aElement   = mTop->mElement;
    NS_IF_ADDREF(*aElement);
    *aIndex     = mTop->mIndex;

    return NS_OK;
}


nsresult
nsXULDocument::Builder::ContextStack::SetTopIndex(PRInt32 aIndex)
{
    if (mDepth == 0)
        return NS_ERROR_UNEXPECTED;

    mTop->mIndex = aIndex;
    return NS_OK;
}


PRBool
nsXULDocument::Builder::ContextStack::IsInsideXULTemplate()
{
    if (mDepth) {
        nsCOMPtr<nsIContent> element = dont_QueryInterface(mTop->mElement);
        while (element) {
            PRInt32 nameSpaceID;
            element->GetNameSpaceID(nameSpaceID);
            if (nameSpaceID == kNameSpaceID_XUL) {
                nsCOMPtr<nsIAtom> tag;
                element->GetTag(*getter_AddRefs(tag));
                if (tag.get() == kTemplateAtom) {
                    return PR_TRUE;
                }
            }

            nsCOMPtr<nsIContent> parent;
            element->GetParent(*getter_AddRefs(parent));
            element = parent;
        }
    }
    return PR_FALSE;
}


//----------------------------------------------------------------------
//
// nsXULDocument::Builder
//

nsXULDocument::Builder::Builder(nsXULDocument* aDocument)
    : mDocument(aDocument), mState(eState_Master)
{
    NS_ADDREF(mDocument);
}


nsXULDocument::Builder::~Builder()
{
    NS_RELEASE(mDocument);
}


nsresult
nsXULDocument::Builder::Start()
{
    nsresult rv;

    rv = NS_NewISupportsArray(getter_AddRefs(mOverlays));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIXULPrototypeDocument> protodoc;
    rv = mDocument->GetPrototype(getter_AddRefs(protodoc));
    if (NS_FAILED(rv)) return rv;

    nsVoidArray overlays;
    rv = protodoc->GetOverlayReferences(overlays);
    if (NS_FAILED(rv)) return rv;

    // Push the overlay references onto our overlay processing stack.
    for (PRInt32 i = overlays.Count() - 1; i >= 0; --i) {
        mOverlays->InsertElementAt(NS_REINTERPRET_CAST(nsIURI*, overlays[i]), 0);
    }

    nsXULPrototypeElement* proto;
    rv = protodoc->GetRootElement(&proto);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIContent> root;
    rv = CreateElement(proto, getter_AddRefs(root));
    if (NS_FAILED(rv)) return rv;

    mDocument->SetRootContent(root);

    rv = mContextStack.Push(proto, root);
    if (NS_FAILED(rv)) return rv;

    rv = Build();
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}


nsresult
nsXULDocument::Builder::Build()
{
    // Walk the prototype and build the delegate content model. The
    // walk is performed in a top-down, left-to-right fashion. That
    // is, a parent is built before any of its children; a node is
    // only built after all of its siblings to the left are fully
    // constructed.
    //
    // It is interruptable so that transcluded documents (e.g.,
    // <html:script src="..." />) can be properly re-loaded if the
    // cached copy of the document becomes stale.
    nsresult rv;

    while (1) {
        // Begin (or resume) walking the current prototype.

        while (mContextStack.Depth() > 0) {
            // Look at the top of the stack to determine what we're
            // currently working on.
            nsXULPrototypeElement* proto;
            nsCOMPtr<nsIContent> element;
            PRInt32 indx;
            rv = mContextStack.Peek(&proto, getter_AddRefs(element), &indx);
            if (NS_FAILED(rv)) return rv;

            // If we've processed all of the prototype's children, then
            // pop back up to the parent node.
            if (indx >= proto->mNumChildren) {
                mContextStack.Pop();
                continue;
            }

            // Grab the next child, and advance the current context stack
            // to the next sibling to our right.
            nsXULPrototypeNode* childproto = proto->mChildren[indx];
            mContextStack.SetTopIndex(++indx);

            switch (childproto->mType) {
            case nsXULPrototypeNode::eType_Element: {
                // An 'element', which may contain more content.
                nsXULPrototypeElement* protoele =
                    NS_REINTERPRET_CAST(nsXULPrototypeElement*, childproto);

                nsCOMPtr<nsIContent> child;

                if ((mState == eState_Master) || (mContextStack.Depth() > 2)) {
                    // Either we're in the master document, or we're
                    // in an overlay and far enough down into the
                    // overlay's content that we can simply build the
                    // delegates and attach them to the parent node.
                    rv = CreateElement(protoele, getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;

                    // ...and append it to the content model.
                    rv = element->AppendChildTo(child, PR_FALSE);
                    if (NS_FAILED(rv)) return rv;

                    rv = mDocument->AddElementToMap(element);
                    if (NS_FAILED(rv)) return rv;
                }
                else if (mContextStack.Depth() == 2) {
                    // We're in the "first ply" of an overlay: the
                    // "hookup" nodes. Create an 'overlay' element so
                    // that we can continue to build content, and
                    // enter a forward reference so we can hook it up
                    // later.
                    rv = CreateOverlayElement(protoele, getter_AddRefs(child));
                    if (NS_FAILED(rv)) return rv;
                }
                else {
                    // We're at the root of an overlay document.
                }

                // If it has children, push the element onto the context
                // stack and begin to process them.
                if (protoele->mNumChildren > 0) {
                    rv = mContextStack.Push(protoele, child);
                    if (NS_FAILED(rv)) return rv;
                }
            }
            break;

            case nsXULPrototypeNode::eType_Script: {
                // A script reference. Evaluate the script immediately;
                // this may have side effects in the content model.
                nsXULPrototypeScript* scriptproto =
                    NS_REINTERPRET_CAST(nsXULPrototypeScript*, childproto);

                if (scriptproto->mInlineScript) {
                    // An inline script
                    nsCOMPtr<nsIURI> url = dont_AddRef(mDocument->GetDocumentURL());
                    if (! url)
                        return NS_ERROR_UNEXPECTED;

                    // Use CBufDescriptor to avoid copying script text
                    CBufDescriptor desc(scriptproto->mInlineScript, PR_TRUE,
                                        scriptproto->mLength,
                                        scriptproto->mLength);

                    rv = EvaluateScript(url, nsAutoString(desc), scriptproto->mLineNo);
                    if (NS_FAILED(rv)) return rv;
                }

                if (scriptproto->mSrcURI) {
                    // A transcluded script reference; this may
                    // "block" our prototype walk if the script isn't
                    // cached, or the cached copy of the script is
                    // stale and must be reloaded.
                    PRBool blocked;
                    rv = LoadScript(scriptproto->mSrcURI, &blocked);
                    if (NS_FAILED(rv)) return rv;

                    if (blocked)
                        return NS_OK;
                }
            }
            break;
            
            case nsXULPrototypeNode::eType_Text: {
                // A simple text node.
                nsCOMPtr<nsITextContent> text;
                rv = nsComponentManager::CreateInstance(kTextNodeCID,
                                                        nsnull,
                                                        NS_GET_IID(nsITextContent),
                                                        getter_AddRefs(text));
                if (NS_FAILED(rv)) return rv;

                nsXULPrototypeText* textproto =
                    NS_REINTERPRET_CAST(nsXULPrototypeText*, childproto);

                rv = text->SetText(textproto->mValue, textproto->mLength, PR_FALSE);
                if (NS_FAILED(rv)) return rv;

                nsCOMPtr<nsIContent> child = do_QueryInterface(text);
                if (! child)
                    return NS_ERROR_UNEXPECTED;

                rv = element->AppendChildTo(child, PR_FALSE);
                if (NS_FAILED(rv)) return rv;
            }
            break;

            }
        }

        // Once we get here, the context stack will have been
        // depleted. That means that the entire prototype has been
        // walked and content has been constructed.

        // If we're not already, mark us as now processing overlays.
        mState = eState_Overlay;

        nsCOMPtr<nsIURI> uri = dont_AddRef(NS_REINTERPRET_CAST(nsIURI*, mOverlays->ElementAt(0)));

        // If there are no URIs in the queue, then we're done.
        if (! uri)
            break;

        nsCOMPtr<nsIXULPrototypeDocument> protodoc;
        rv = gXULPrototypeCache->Get(uri, getter_AddRefs(protodoc));
        if (NS_FAILED(rv)) return rv;

        
    }

    // If we get here, there is nothing left for us to do! Destroy
    // ourself.
    delete this;

    return NS_OK;
}


nsresult
nsXULDocument::Builder::LoadScript(const char* aURI, PRBool* aBlock)
{
    // Load a transcluded script
    nsresult rv;

    nsCOMPtr<nsIURI> baseurl = dont_AddRef(mDocument->GetDocumentURL());

    nsCOMPtr<nsIURI> url;
    rv = NS_NewURI(getter_AddRefs(url), aURI, baseurl);
    if (NS_FAILED(rv)) return rv;

    // XXX Look in a script cache to see if we already have it

    if (/* cached */ PR_FALSE) {
        *aBlock = PR_FALSE;

        // XXX brendan, shaver: do some magic here
    }
    else {
        // Set the current script URL so that the DoneLoadingScript()
        // call can get report the right file if there are errors in
        // the script.
        mCurrentScriptURL = url;

        nsCOMPtr<nsILoadGroup> group;
        rv = mDocument->GetDocumentLoadGroup(getter_AddRefs(group));
        if (NS_FAILED(rv)) return rv;

        // N.B., the loader will be released in DoneLoadingScript()
        nsIUnicharStreamLoader* loader;
        rv = NS_NewUnicharStreamLoader(&loader,
                                       url, 
                                       group,
                                       (nsStreamCompleteFunc)DoneLoadingScript, 
                                       this);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::Builder::DoneLoadingScript(nsIUnicharStreamLoader* aLoader,
                                            nsString& aData,
                                            void* aRef,
                                            nsresult aStatus)
{
    // This is the completion routine that will be called when a
    // transcluded script completes. Evaluate the script if the load
    // was successful, then continue building content from the
    // prototype.
    nsresult rv;
    Builder* builder = NS_REINTERPRET_CAST(Builder*, aRef);

    if (NS_SUCCEEDED(aStatus)) {
        rv = builder->EvaluateScript(builder->mCurrentScriptURL, aData, 1);
    }

    // balance the addref we added in LoadScript()
    NS_RELEASE(aLoader);

    rv = builder->Build();
    return rv;
}


nsresult
nsXULDocument::Builder::EvaluateScript(nsIURI* aURL, const nsString& aScript, PRInt32 aLineNo)
{
    // Evaluate the script text in aScript, whose source is aURL
    // starting at aLineNo.
    nsresult rv;

    nsCOMPtr<nsIScriptContextOwner> owner =
        dont_AddRef(mDocument->GetScriptContextOwner());

    NS_ASSERTION(owner != nsnull, "document has no script context owner");
    if (! owner)
        return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIScriptContext> context;
    rv = owner->GetScriptContext(getter_AddRefs(context));
    if (NS_FAILED(rv)) return rv;

    nsXPIDLCString urlspec;
    rv = aURL->GetSpec(getter_Copies(urlspec));
    if (NS_FAILED(rv)) return rv;

    nsAutoString result;
    PRBool isUndefined;
    rv = context->EvaluateString(aScript, urlspec, aLineNo, 
                                 result, &isUndefined);
    return rv;
}


nsresult
nsXULDocument::Builder::CreateElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult)
{
    // Create a content model element.
    nsresult rv;

    nsCOMPtr<nsIContent> result;

    if (aPrototype->mNameSpaceID == kNameSpaceID_HTML) {
        // If it's an HTML element, it's gonna be heavyweight no matter
        // what. So we need to copy everything out of the prototype
        // into the element.
        nsAutoString tagStr;
        rv = aPrototype->mTag->ToString(tagStr);
        if (NS_FAILED(rv)) return rv;
        
        nsCOMPtr<nsIHTMLContent> element;
        gHTMLElementFactory->CreateInstanceByTag(tagStr.GetUnicode(), getter_AddRefs(element));
        if (NS_FAILED(rv)) return rv;

        result = do_QueryInterface(element);
        if (! result)
            return NS_ERROR_UNEXPECTED;

        rv = AddAttributes(aPrototype, result);
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // If it's a XUL element, it'll be lightweight until somebody
        // monkeys with it.
        rv = nsXULElement::Create(aPrototype, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }

    rv = result->SetDocument(mDocument, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

nsresult
nsXULDocument::Builder::CreateOverlayElement(nsXULPrototypeElement* aPrototype, nsIContent** aResult)
{
    nsresult rv;

    nsCOMPtr<nsIXMLContent> xml;
    rv = gXMLElementFactory->CreateInstanceByTag(nsAutoString("overlay"), getter_AddRefs(xml));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create overlay element");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIContent> element = do_QueryInterface(xml);
    NS_ASSERTION(element != nsnull, "xml element is not an nsIXMLContent");
    if (! xml)
        return NS_ERROR_UNEXPECTED;

    // Initialize the overlay element
    rv = element->SetDocument(mDocument, PR_FALSE);
    if (NS_FAILED(rv)) return rv;
            
    // Set up namespace junk so that subsequent parsing won't
    // freak out.
    rv = xml->SetContainingNameSpace(aPrototype->mNameSpace);
    if (NS_FAILED(rv)) return rv;

    rv = AddAttributes(aPrototype, element);
    if (NS_FAILED(rv)) return rv;

    OverlayForwardReference* fwdref = new OverlayForwardReference(element);
    if (! fwdref)
        return NS_ERROR_OUT_OF_MEMORY;

    // transferring ownership to ya...
    rv = mDocument->AddForwardReference(fwdref);
    if (NS_FAILED(rv)) return rv;

    *aResult = element;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::Builder::AddAttributes(nsXULPrototypeElement* aPrototype, nsIContent* aElement)
{
    nsresult rv;

    for (PRInt32 i = 0; i < aPrototype->mNumAttributes; ++i) {
        nsXULPrototypeAttribute* protoattr = &(aPrototype->mAttributes[i]);

        PRInt32 len = nsCRT::strlen(protoattr->mValue);
        CBufDescriptor desc(protoattr->mValue, PR_TRUE, len, len);

        rv = aElement->SetAttribute(protoattr->mNameSpaceID,
                                    protoattr->mName,
                                    nsAutoString(desc),
                                    PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsXULDocument::Builder::Stop()
{
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsXULDocument::Builder::OverlayForwardReference
//

nsForwardReference::Result
nsXULDocument::Builder::OverlayForwardReference::Resolve()
{
    // Resolve a forward reference from an overlay element; attempt to
    // hook it up into the main document.
    nsresult rv;

    nsCOMPtr<nsIDocument> doc;
    rv = mContent->GetDocument(*getter_AddRefs(doc));
    if (NS_FAILED(rv)) return eResolve_Error;

    nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(doc);
    if (! xuldoc)
        return eResolve_Error;

    nsAutoString id;
    rv = mContent->GetAttribute(kNameSpaceID_XUL, kIdAtom, id);
    if (NS_FAILED(rv)) return eResolve_Error;

    nsCOMPtr<nsIDOMElement> domoverlay;
    rv = xuldoc->GetElementById(id, getter_AddRefs(domoverlay));
    if (NS_FAILED(rv)) return eResolve_Error;

    // If we can't find the element in the document, defer the hookup
    // until later.
    if (! domoverlay)
        return eResolve_Later;

    nsCOMPtr<nsIContent> overlay = do_QueryInterface(domoverlay);
    NS_ASSERTION(overlay != nsnull, "not an nsIContent");
    if (! overlay)
        return eResolve_Error;

    rv = Merge(mContent, overlay);
    if (NS_FAILED(rv)) return eResolve_Error;

    PR_LOG(gXULLog, PR_LOG_ALWAYS,
           ("xul: overlay resolved '%s'",
            (const char*) nsCAutoString(id)));

    mResolved = PR_TRUE;
    return eResolve_Succeeded;
}



nsresult
nsXULDocument::Builder::OverlayForwardReference::Merge(nsIContent* aOriginalNode,
                                                       nsIContent* aOverlayNode)
{
    nsresult rv;

    {
        // Whack the attributes from aOverlayNode onto aOriginalNode
        PRInt32 count;
        rv = aOverlayNode->GetAttributeCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < count; ++i) {
            PRInt32 nameSpaceID;
            nsCOMPtr<nsIAtom> tag;
            rv = aOverlayNode->GetAttributeNameAt(i, nameSpaceID, *getter_AddRefs(tag));
            if (NS_FAILED(rv)) return rv;

            nameSpaceID = kNameSpaceID_None;

            if (nameSpaceID == kNameSpaceID_None && tag.get() == kIdAtom)
                continue;

            nsAutoString value;
            rv = aOverlayNode->GetAttribute(nameSpaceID, tag, value);
            if (NS_FAILED(rv)) return rv;

            rv = aOriginalNode->SetAttribute(nameSpaceID, tag, value, PR_FALSE);
            if (NS_FAILED(rv)) return rv;
        }

        rv = ProcessCommonAttributes(aOriginalNode);
    }

    {
        // Now move any kids
        PRInt32 count;
        rv = aOverlayNode->ChildCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < count; ++i) {
            nsCOMPtr<nsIContent> child;
            rv = aOverlayNode->ChildAt(i, *getter_AddRefs(child));
            if (NS_FAILED(rv)) return rv;

            rv = InsertElement(aOriginalNode, child);
            if (NS_FAILED(rv)) return rv;
        }

        // Ok, now we _don't_ need to add these to the
        // document-to-element map because normal construction of the
        // nodes will have done this.
    }

    return NS_OK;
}



nsXULDocument::Builder::OverlayForwardReference::~OverlayForwardReference()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS) && !mResolved) {
        nsAutoString id;
        mContent->GetAttribute(kNameSpaceID_None, kIdAtom, id);
        
        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: overlay failed to resolve '%s'",
                (const char*) nsCAutoString(id)));
    }
#endif
}


//----------------------------------------------------------------------
//
// nsXULDocument::Builder::BroadcasterHookup
//

nsForwardReference::Result
nsXULDocument::Builder::BroadcasterHookup::Resolve()
{
    // Resolve a broadcaster hookup. Look at the element that we're
    // trying to resolve: it could be an '<observes>' element, or just
    // a vanilla element with an 'observes' attribute on it.
    nsresult rv;

    nsCOMPtr<nsIAtom> tag;
    rv = mObservesElement->GetTag(*getter_AddRefs(tag));
    if (NS_FAILED(rv)) return eResolve_Error;

    nsCOMPtr<nsIDOMElement> listener;
    nsAutoString broadcasterID;
    nsAutoString attribute;

    if (tag.get() == kObservesAtom) {
        // It's an <observes> element, which means that the actual
        // listener is the _parent_ node. This element should have an
        // 'element' attribute that specifies the ID of the
        // broadcaster element, and an 'attribute' element, which
        // specifies the name of the attribute to observe.
        nsCOMPtr<nsIContent> parent;
        rv = mObservesElement->GetParent(*getter_AddRefs(parent));
        if (NS_FAILED(rv)) return eResolve_Error;

        nsCOMPtr<nsIAtom> parentTag;
        rv = parent->GetTag(*getter_AddRefs(parentTag));
        if (NS_FAILED(rv)) return eResolve_Error;

        // If we're still parented by an 'overlay' tag, then we haven't
        // made it into the real document yet. Defer hookup.
        if (parentTag.get() == kOverlayAtom)
            return eResolve_Later;

        listener = do_QueryInterface(parent);

        rv = mObservesElement->GetAttribute(kNameSpaceID_None, kElementAtom, broadcasterID);
        if (NS_FAILED(rv)) return eResolve_Error;

        rv = mObservesElement->GetAttribute(kNameSpaceID_None, kAttributeAtom, attribute);
        if (NS_FAILED(rv)) return eResolve_Error;
    }
    else {
        // It's a generic element, which means that we'll use the
        // value of the 'observes' attribute to determine the ID of
        // the broadcaster element, and we'll watch _all_ of its
        // values.
        listener = do_QueryInterface(mObservesElement);

        rv = mObservesElement->GetAttribute(kNameSpaceID_None, kObservesAtom, broadcasterID);
        if (NS_FAILED(rv)) return eResolve_Error;

        attribute = "*";
    }

    // Try to find the broadcaster element in the document.
    nsCOMPtr<nsIDocument> doc;
    rv = mObservesElement->GetDocument(*getter_AddRefs(doc));
    if (NS_FAILED(rv)) return eResolve_Error;

    nsCOMPtr<nsIDOMXULDocument> xuldoc = do_QueryInterface(doc);
    if (! xuldoc)
        return eResolve_Error;

    nsCOMPtr<nsIDOMElement> target;
    rv = xuldoc->GetElementById(broadcasterID, getter_AddRefs(target));
    if (NS_FAILED(rv)) return eResolve_Error;

    // If we can't find the broadcaster, then we'll need to defer the
    // hookup. We may need to resolve some of the other overlays
    // first.
    if (! target)
        return eResolve_Later;

    nsCOMPtr<nsIDOMXULElement> broadcaster = do_QueryInterface(target);
    if (NS_FAILED(rv))
        return eResolve_Error; // not a XUL element, so we can't subscribe

    rv = broadcaster->AddBroadcastListener(attribute, listener);
    if (NS_FAILED(rv)) return eResolve_Error;

#ifdef PR_LOGGING
    // Tell the world we succeeded
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS)) {
        nsCOMPtr<nsIContent> content =
            do_QueryInterface(listener);

        NS_ASSERTION(content != nsnull, "not an nsIContent");
        if (! content)
            return eResolve_Error;

        nsCOMPtr<nsIAtom> tag;
        rv = content->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return eResolve_Error;

        nsAutoString tagStr;
        rv = tag->ToString(tagStr);
        if (NS_FAILED(rv)) return eResolve_Error;

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: broadcaster hookup <%s attribute='%s'> to %s",
                (const char*) nsCAutoString(tagStr),
                (const char*) nsCAutoString(attribute),
                (const char*) nsCAutoString(broadcasterID)));
    }
#endif

    mResolved = PR_TRUE;
    return eResolve_Succeeded;
}


nsXULDocument::Builder::BroadcasterHookup::~BroadcasterHookup()
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_ALWAYS) && !mResolved) {
        // Tell the world we failed
        nsresult rv;

        nsCOMPtr<nsIAtom> tag;
        rv = mObservesElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return;

        nsAutoString broadcasterID;
        nsAutoString attribute;

        if (tag.get() == kObservesAtom) {
            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kElementAtom, broadcasterID);
            if (NS_FAILED(rv)) return;

            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kAttributeAtom, attribute);
            if (NS_FAILED(rv)) return;
        }
        else {
            rv = mObservesElement->GetAttribute(kNameSpaceID_None, kObservesAtom, broadcasterID);
            if (NS_FAILED(rv)) return;

            attribute = "*";
        }

        nsAutoString tagStr;
        rv = tag->ToString(tagStr);
        if (NS_FAILED(rv)) return;

        PR_LOG(gXULLog, PR_LOG_ALWAYS,
               ("xul: broadcaster hookup failed <%s attribute='%s'> to %s",
                (const char*) nsCAutoString(tagStr),
                (const char*) nsCAutoString(attribute),
                (const char*) nsCAutoString(broadcasterID)));
    }
#endif
}


//----------------------------------------------------------------------


nsresult
nsXULDocument::Builder::InsertElement(nsIContent* aParent, nsIContent* aChild)
{
    // Insert aChild appropriately into aParent, accountinf for a
    // 'pos' attribute set on aChild.
    nsresult rv;

    nsAutoString posStr;
    rv = aChild->GetAttribute(kNameSpaceID_None, kPositionAtom, posStr);
    if (NS_FAILED(rv)) return rv;

    PRBool wasInserted = PR_FALSE;

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        PRInt32 pos = posStr.ToInteger(NS_REINTERPRET_CAST(PRInt32*, &rv));
        if (NS_SUCCEEDED(rv)) {
            rv = aParent->InsertChildAt(aChild, pos, PR_FALSE);
            if (NS_FAILED(rv)) return rv;

            wasInserted = PR_TRUE;
        }
    }

    if (! wasInserted) {
        rv = aParent->AppendChildTo(aChild, PR_FALSE);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::Builder::ProcessCommonAttributes(nsIContent* aElement)
{
    // Common code for handling 'magic' attributes

    nsresult rv;

    nsCOMPtr<nsIDocument> doc;
    rv = aElement->GetDocument(*getter_AddRefs(doc));
    if (NS_FAILED(rv)) return rv;

    nsAutoString value;

    // Check for a 'commandupdater' attribute, in which case we'll
    // hook the node up as a command updater.
    rv = aElement->GetAttribute(kNameSpaceID_None, kCommandUpdaterAtom, value);
    if (NS_FAILED(rv)) return rv;

    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && value.Equals("true")) {
        rv = gXULUtils->SetCommandUpdater(doc, aElement);
        if (NS_FAILED(rv)) return rv;
    }


    // Check for an 'observes' attribute, in which case we'll hook the
    // node up as a listener on a broadcaster.
    rv = aElement->GetAttribute(kNameSpaceID_None, kObservesAtom, value);
    if (NS_FAILED(rv)) return rv;

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        nsCOMPtr<nsIDOMXULDocument> domxuldoc = do_QueryInterface(doc);
        if (! domxuldoc)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIDOMElement> broadcaster;
        rv = domxuldoc->GetElementById(value, getter_AddRefs(broadcaster));
        if (NS_FAILED(rv)) return rv;

        if (broadcaster) {
            nsCOMPtr<nsIDOMXULElement> xulbroadcaster = do_QueryInterface(broadcaster);
            if (xulbroadcaster) {
                nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
                NS_ASSERTION(domelement != nsnull, "not an nsIDOMElement");
                if (! domelement)
                    return NS_ERROR_UNEXPECTED;

                rv = xulbroadcaster->AddBroadcastListener("*", domelement);
                if (NS_FAILED(rv)) return rv;
            }
        }
        else {
            nsCOMPtr<nsIXULDocument> xuldoc = do_QueryInterface(doc);
            if (! xuldoc)
                return NS_ERROR_UNEXPECTED;

            BroadcasterHookup* fwdref = 
                new BroadcasterHookup(aElement);

            if (! fwdref)
                return NS_ERROR_OUT_OF_MEMORY;

            rv = xuldoc->AddForwardReference(fwdref);
            if (NS_FAILED(rv)) return rv;
        }
    }

#if 0
    // Add any script event listeners to XUL elements, because a XUL
    // element's SetAttribute() won't do it automatically.
    nsCOMPtr<nsIXULContent> xulele = do_QueryInterface(aElement);
    if (xulele) {
        PRInt32 count;
        rv = aElement->GetAttributeCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = 0; i < count; ++i) {
            PRInt32 nameSpaceID;
            nsCOMPtr<nsIAtom> tag;
            rv = aElement->GetAttributeNameAt(i, nameSpaceID, *getter_AddRefs(tag));
            if (NS_FAILED(rv)) return rv;

            if (nameSpaceID != kNameSpaceID_None)
                continue;

            rv = aElement->GetAttribute(nameSpaceID, tag, value);
            if (NS_FAILED(rv)) return rv;

            // Check for event handlers and add a script listener if necessary.
            nsIID iid;
            PRBool found;
            rv = gXULUtils->GetEventHandlerIID(tag.get(), &iid, &found);
            if (NS_FAILED(rv)) return rv;

            if (found) {
                xulele->AddScriptEventListener(tag, value, iid);
            }
        }
    }
#endif

    return NS_OK;
}

