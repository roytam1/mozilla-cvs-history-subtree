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

#ifndef nsRDFContainerElement_h___
#define nsRDFContainerElement_h___

#include "nsRDFElement.h"

class nsIContent;
class nsIDocument;
class nsIAtom;
class nsIEventListenerManager;
class nsIHTMLAttributes;

class nsRDFContainerElement : public nsRDFElement,
		     public nsIRDFContainerContent
{
public:
    nsRDFContainerElement(void);
    ~nsRDFContainerElement(void);

    // nsISupports
    NS_IMETHOD_(nsresult) AddRef(void) {
        return nsRDFElement::AddRef();
    }

    NS_IMETHOD_(nsresult) Release(void) {
        return nsRDFElement::Release();
    }

    NS_IMETHOD QueryInterface(REFNSIID iid, void** result);

    // nsIContent (from nsIRDFContent via nsIXMLContent)
    NS_IMETHOD GetDocument(nsIDocument*& aResult) const {
        return nsRDFElement::GetDocument(aResult);
    }
    NS_IMETHOD SetDocument(nsIDocument* aDocument, PRBool aDeep) {
        return nsRDFElement::SetDocument(aDocument, aDeep);
    }
    NS_IMETHOD GetParent(nsIContent*& aResult) const {
        return nsRDFElement::GetParent(aResult);
    }
    NS_IMETHOD SetParent(nsIContent* aParent) {
        return nsRDFElement::SetParent(aParent);
    }
    NS_IMETHOD CanContainChildren(PRBool& aResult) const {
        return nsRDFElement::CanContainChildren(aResult);
    }
    NS_IMETHOD ChildCount(PRInt32& aResult) const {
        return nsRDFElement::ChildCount(aResult);
    }
    NS_IMETHOD ChildAt(PRInt32 aIndex, nsIContent*& aResult) const {
        return nsRDFElement::ChildAt(aIndex, aResult);
    }
    NS_IMETHOD IndexOf(nsIContent* aPossibleChild, PRInt32& aResult) const {
        return nsRDFElement::IndexOf(aPossibleChild, aResult);
    }
    NS_IMETHOD InsertChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify) {
        return nsRDFElement::InsertChildAt(aKid, aIndex, aNotify);
    }
    NS_IMETHOD ReplaceChildAt(nsIContent* aKid, PRInt32 aIndex, PRBool aNotify) {
        return nsRDFElement::ReplaceChildAt(aKid, aIndex, aNotify);
    }
    NS_IMETHOD AppendChildTo(nsIContent* aKid, PRBool aNotify) {
        return nsRDFElement::AppendChildTo(aKid, aNotify);
    }
    NS_IMETHOD RemoveChildAt(PRInt32 aIndex, PRBool aNotify) {
        return nsRDFElement::RemoveChildAt(aIndex, aNotify);
    }
    NS_IMETHOD IsSynthetic(PRBool& aResult) {
        return nsRDFElement::IsSynthetic(aResult);
    }
    NS_IMETHOD GetTag(nsIAtom*& aResult) const {
        return nsRDFElement::GetTag(aResult);
    }
    NS_IMETHOD SetAttribute(const nsString& aName, const nsString& aValue, PRBool aNotify) {
        return nsRDFElement::SetAttribute(aName, aValue, aNotify);
    }
    NS_IMETHOD GetAttribute(const nsString& aName, nsString& aResult) const {
        return nsRDFElement::GetAttribute(aName, aResult);
    }
    NS_IMETHOD UnsetAttribute(nsIAtom* aAttribute, PRBool aNotify) {
        return nsRDFElement::UnsetAttribute(aAttribute, aNotify);
    }
    NS_IMETHOD GetAllAttributeNames(nsISupportsArray* aArray, PRInt32& aResult) const {
        return nsRDFElement::GetAllAttributeNames(aArray, aResult);
    }
    NS_IMETHOD GetAttributeCount(PRInt32& aResult) const {
        return nsRDFElement::GetAttributeCount(aResult);
    }
    NS_IMETHOD List(FILE* out, PRInt32 aIndent) const {
        return nsRDFElement::List(out, aIndent);
    }
    NS_IMETHOD BeginConvertToXIF(nsXIFConverter& aConverter) const {
        return nsRDFElement::BeginConvertToXIF(aConverter);
    }
    NS_IMETHOD ConvertContentToXIF(nsXIFConverter& aConverter) const {
        return nsRDFElement::ConvertContentToXIF(aConverter);
    }
    NS_IMETHOD FinishConvertToXIF(nsXIFConverter& aConverter) const {
        return nsRDFElement::FinishConvertToXIF(aConverter);
    }
    NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const {
        return nsRDFElement::SizeOf(aHandler);
    }
    NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext,
                              nsEvent* aEvent,
                              nsIDOMEvent** aDOMEvent,
                              PRUint32 aFlags,
                              nsEventStatus& aEventStatus) {
        return nsRDFElement::HandleDOMEvent(aPresContext,
                                            aEvent,
                                            aDOMEvent,
                                            aFlags,
                                            aEventStatus);
    }

    // nsIXMLContent (from nsIRDFContent)
    NS_IMETHOD SetNameSpace(nsIAtom* aNameSpace) {
        return nsRDFElement::SetNameSpace(aNameSpace);
    }
    NS_IMETHOD GetNameSpace(nsIAtom*& aNameSpace) {
        return nsRDFElement::GetNameSpace(aNameSpace);
    }
    NS_IMETHOD SetNameSpaceIdentifier(PRInt32 aNameSpaceId) {
        return nsRDFElement::SetNameSpaceIdentifier(aNameSpaceId);
    }
    NS_IMETHOD GetNameSpaceIdentifier(PRInt32& aNameSpaceId) {
        return nsRDFElement::GetNameSpaceIdentifier(aNameSpaceId);
    }

    // nsIRDFContent
    NS_IMETHOD SetResource(const nsString& aURI) {
        return nsRDFElement::SetResource(aURI);
    }
    NS_IMETHOD GetResource(nsString& rURI) const {
        return nsRDFElement::GetResource(rURI);
    }

    NS_IMETHOD SetProperty(const nsString& aPropertyURI, const nsString& aValue) {
        return nsRDFElement::SetProperty(aPropertyURI, aValue);
    }
    NS_IMETHOD GetProperty(const nsString& aPropertyURI, nsString& rValue) const {
        return nsRDFElement::GetProperty(aPropertyURI, rValue);
    }

    // nsIRDFContainerContent
    NS_IMETHOD GetElementCount(PRUint32& rCount);

protected:
};

#endif // nsRDFContainerElement_h___
