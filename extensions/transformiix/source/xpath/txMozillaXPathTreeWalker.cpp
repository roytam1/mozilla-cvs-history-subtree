/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "txXPathTreeWalker.h"
#include "nsIAtom.h"
#include "nsIAttribute.h"
#include "nsIDOM3Node.h"
#include "nsIDOMAttr.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsINodeInfo.h"
#include "nsITextContent.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsTextFragment.h"
#include "XMLUtils.h"
#include "TxLog.h"

#define kUnknownIndex -1

txXPathTreeWalker::txXPathTreeWalker(const txXPathNode& aNode)
    : mPosition(aNode),
      mCurrentIndex(kUnknownIndex)
{
}

txXPathTreeWalker::~txXPathTreeWalker()
{
}

PRBool
txXPathTreeWalker::moveToElementById(const nsAString& aID)
{
    nsCOMPtr<nsIDOMDocument> document;
    if (mPosition.isDocument()) {
        document = do_QueryInterface(mPosition.mDocument);
    }
    else {
        nsCOMPtr<nsIDocument> doc = mPosition.mContent->GetDocument();
        document = do_QueryInterface(doc);
    }

    nsCOMPtr<nsIDOMElement> element;
    document->GetElementById(aID, getter_AddRefs(element));
    if (!element) {
        return PR_FALSE;
    }

    if (mPosition.isDocument()) {
        NS_RELEASE(mPosition.mDocument);
    }
    else {
        NS_RELEASE(mPosition.mContent);
    }
    mPosition.mIndex = txXPathNode::eContent;
    CallQueryInterface(element, &mPosition.mContent);
    mCurrentIndex = kUnknownIndex;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstAttribute()
{
    if (!mPosition.isContent() ||
        !mPosition.mContent->IsContentOfType(nsIContent::eELEMENT)) {
        return PR_FALSE;
    }

    PRInt32 total;
    mPosition.mContent->GetAttrCount(total);
    if (total <= 0) {
        return PR_FALSE;
    }

    // We need to ignore XMLNS attributes.
    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> name, prefix;
    PRInt32 index;
    for (index = 0; index < total; ++index) {
        mPosition.mContent->GetAttrNameAt(index, &namespaceID,
                                          getter_AddRefs(name),
                                          getter_AddRefs(prefix));
        if (namespaceID != kNameSpaceID_XMLNS) {
            break;
        }
    }

    if (index == total) {
        return PR_FALSE;
    }

    mPosition.mIndex = index;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstChild()
{
    if (mPosition.isAttribute()) {
        return PR_FALSE;
    }

    PRInt32 total;

    if (mPosition.isDocument()) {
        mPosition.mDocument->GetChildCount(total);
        if (total <= 0) {
            return PR_FALSE;
        }

        nsCOMPtr<nsIDocument> document = dont_AddRef(mPosition.mDocument);
        mPosition.mIndex = txXPathNode::eContent;
        document->ChildAt(0, &mPosition.mContent);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    mPosition.mContent->ChildCount(total);
    if (total <= 0) {
        return PR_FALSE;
    }

    nsCOMPtr<nsIContent> content = dont_AddRef(mPosition.mContent);
    content->ChildAt(0, &mPosition.mContent);
    mCurrentIndex = 0;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstDescendant()
{
    return moveToFirstChild();
}

PRBool
txXPathTreeWalker::moveToFirstFollowing()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    PRInt32 total;
    nsCOMPtr<nsIContent> content = mPosition.mContent;
    if (mPosition.isAttribute()) {
        // Check if the attribute's parent has any children.
        content->ChildCount(total);
        if (total > 0) {
            NS_RELEASE(mPosition.mContent);
            mPosition.mIndex = txXPathNode::eContent;
            content->ChildAt(0, &mPosition.mContent);
            mCurrentIndex = 0;

            return PR_TRUE;
        }
    }

    nsCOMPtr<nsIContent> child, parent = content->GetParent();

// XXXX Use mCurrentIndex?

    // Now walk up the parent chain to find the first ancestor that has a
    // following sibling.
    while (parent) {
        // First check if content has any following siblings
        parent->ChildCount(total);
        if (total > 1) {
            parent->ChildAt(total - 1, getter_AddRefs(child));
            // If content is the last child of its parent we can continue
            // walking up since it has no following sibling.
            if (child != content) {
                NS_RELEASE(mPosition.mContent);
                mPosition.mIndex = txXPathNode::eContent;
                parent->IndexOf(content, mCurrentIndex);
                parent->ChildAt(++mCurrentIndex, &mPosition.mContent);

                return PR_TRUE;
            }
        }

        content = parent;
        parent = content->GetParent();
    }

    nsCOMPtr<nsIDocument> document = content->GetDocument();

    document->GetChildCount(total);
    if (total <= 1) {
        return PR_FALSE;
    }

    document->ChildAt(total - 1, getter_AddRefs(child));
    if (child == content) {
        return PR_FALSE;
    }

    NS_RELEASE(mPosition.mContent);
    mPosition.mIndex = txXPathNode::eContent;
// XXXX Use mCurrentIndex?
    parent->IndexOf(content, mCurrentIndex);
    parent->ChildAt(++mCurrentIndex, &mPosition.mContent);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstFollowingSibling()
{
    if (mPosition.isDocument() || mPosition.isAttribute()) {
        return PR_FALSE;
    }

    return moveToSibling(PR_TRUE);
}

PRBool
txXPathTreeWalker::moveToFirstPreceding()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    // Find the first preceding sibling of the current content
    // or one of its ancestors.
    // Attributes behave just like their ownerElement, as the element is
    // part of the ancestor axis.
    nsCOMPtr<nsIDocument> document;
    PRInt32 index;
    nsCOMPtr<nsIContent> parent = mPosition.mContent->GetParent();
    if (mCurrentIndex > 0) {
        index = mCurrentIndex;
        if (!parent) {
            document = mPosition.mContent->GetDocument();
        }
    }
    else {
        nsCOMPtr<nsIContent> content = mPosition.mContent;
        while (parent) {
            parent->IndexOf(content, index);
            if (index > 0) {
                break;
            }

            content = parent;
            parent = content->GetParent();
        }

        if (!parent) {
            document = content->GetDocument();
            document->IndexOf(content, index);
            if (index <= 0) {
                return PR_FALSE;
            }
        }
    }

    NS_ASSERTION(parent || document, "UHOH");

    nsCOMPtr<nsIContent> descendant;
    if (parent) {
        parent->ChildAt(--index, getter_AddRefs(descendant));
    }
    else {
        document->ChildAt(--index, getter_AddRefs(descendant));
    }

    // Find the last child of the preceding sibling.
    PRInt32 total;
    descendant->ChildCount(total);
    while (total > 0) {
        index = total - 1;
        parent = descendant;
        parent->ChildAt(index, getter_AddRefs(descendant));
        descendant->ChildCount(total);
        if (total > 0) {
            if (!mDescendants) {
                mDescendants = new txInt32Array;
            }
            mDescendants->AppendValue(index);
        }
    }

    NS_RELEASE(mPosition.mContent);
    mPosition.mIndex = txXPathNode::eContent;
    descendant.swap(mPosition.mContent);
    mCurrentIndex = index;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstPrecedingInDocOrder()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    // find previous sibling or parent
    nsCOMPtr<nsIContent> parent;
    nsCOMPtr<nsIDocument> document;
    PRInt32 index = mCurrentIndex;
    if (mPosition.isContent()) {
        parent = mPosition.mContent->GetParent();
        if (parent && index == kUnknownIndex) {
            parent->IndexOf(mPosition.mContent, index);
        }
    }
    else {
        parent = mPosition.mContent;
        index = mPosition.mIndex;
    }

    nsCOMPtr<nsIContent> descendant;
    if (parent) {
        if (index == 0) {
            NS_RELEASE(mPosition.mContent);
            mPosition.mIndex = txXPathNode::eContent;
            parent.swap(mPosition.mContent);
            mCurrentIndex = kUnknownIndex;

            return PR_TRUE;
        }

        if (mPosition.isAttribute()) {
            --mPosition.mIndex;

            return PR_TRUE;
        }

        parent->ChildAt(--index, getter_AddRefs(descendant));
    }
    else {
        document = mPosition.mContent->GetDocument();
        if (index == kUnknownIndex) {
            document->IndexOf(mPosition.mContent, index);
        }
        if (index == 0) {
            NS_RELEASE(mPosition.mContent);
            mPosition.mIndex = txXPathNode::eDocument;
            mPosition.mDocument = document;

            return PR_TRUE;
        }

        document->ChildAt(--index, getter_AddRefs(descendant));
    }

    // find last child
    PRInt32 total;
    descendant->ChildCount(total);

    while (total > 0) {
        index = total - 1;
        parent = descendant;
        parent->ChildAt(index, getter_AddRefs(descendant));
        descendant->ChildCount(total);
        if (total > 0) {
            if (!mDescendants) {
                mDescendants = new txInt32Array;
            }
            mDescendants->AppendValue(index);
        }
    }

    NS_RELEASE(mPosition.mContent);
    mPosition.mIndex = txXPathNode::eContent;
    descendant.swap(mPosition.mContent);
    mCurrentIndex = index;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstPrecedingSibling()
{
    if (mPosition.isDocument() || mPosition.isAttribute()) {
        return PR_FALSE;
    }

    return moveToSibling(PR_FALSE);
}

PRBool
txXPathTreeWalker::moveToNextDescendant()
{
    NS_ASSERTION(mPosition.isContent(),
                 "Wrong type, maybe you called moveToNextDescendant without "
                 "moveToFirstDescendant first?");

    PRInt32 total;
    mPosition.mContent->ChildCount(total);
    if (total > 0) {
        if (!mDescendants) {
            mDescendants = new txInt32Array;
        }

        mDescendants->AppendValue(mCurrentIndex);
        nsCOMPtr<nsIContent> temp = dont_AddRef(mPosition.mContent);
        temp->ChildAt(0, &mPosition.mContent);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    PRInt32 index = mCurrentIndex;
    nsCOMPtr<nsIContent> parent = mPosition.mContent->GetParent();
    if (parent) {
        parent->ChildCount(total);
        if (index + 1 < total) {
            NS_RELEASE(mPosition.mContent);
            parent->ChildAt(++mCurrentIndex, &mPosition.mContent);

            return PR_TRUE;
        }

        if (!mDescendants) {
            return PR_FALSE;
        }

        PRInt32 currentIndex = mDescendants->Count();
        if (currentIndex == 0) {
            return PR_FALSE;
        }

        nsCOMPtr<nsIContent> content = parent;
        parent = content->GetParent();
        while (parent) {
            index = mDescendants->ValueAt(--currentIndex);
            parent->ChildCount(total);
            if (index + 1 < total) {
                NS_RELEASE(mPosition.mContent);
                mCurrentIndex = index + 1;
                parent->ChildAt(mCurrentIndex, &mPosition.mContent);
                mDescendants->RemoveValuesAt(currentIndex,
                                             mDescendants->Count() - currentIndex);

                return PR_TRUE;
            }

            if (currentIndex == 0) {
                return PR_FALSE;
            }

            content = parent;
            parent = content->GetParent();
        }

        NS_ASSERTION(currentIndex == 1, "HUH?");

        index = mDescendants->ValueAt(0);
    }

    nsCOMPtr<nsIDocument> document = mPosition.mContent->GetDocument();
    document->GetChildCount(total);
    if (index + 1 >= total) {
        return PR_FALSE;
    }

    NS_RELEASE(mPosition.mContent);
    mCurrentIndex = index + 1;
    document->ChildAt(mCurrentIndex, &mPosition.mContent);

    if (mDescendants) {
        mDescendants->Clear();
    }

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextFollowing()
{
    NS_ASSERTION(mPosition.isContent(),
                 "Wrong type, maybe you called moveToNextFollowing without "
                 "moveToFirstFollowing first?");

    // Check if the current content has any children.
    PRInt32 total;
    mPosition.mContent->ChildCount(total);
    if (total > 0) {
        nsCOMPtr<nsIContent> temp = dont_AddRef(mPosition.mContent);
        temp->ChildAt(0, &mPosition.mContent);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    nsCOMPtr<nsIContent> parent, child, content;
    // Look at next siblings of the current content.
    parent = mPosition.mContent->GetParent();
    if (parent) {
        parent->ChildCount(total);
        if (mCurrentIndex + 1 < total) {
            NS_RELEASE(mPosition.mContent);
            parent->ChildAt(++mCurrentIndex, &mPosition.mContent);

            return PR_TRUE;
        }

        content = parent;
        parent = content->GetParent();

        // Now walk up the parent chain to find the first ancestor that has a
        // following sibling.
        while (parent) {
            // First check if content has any following siblings
            parent->ChildCount(total);
            if (total > 1) {
                // If content is the last child of its parent we can continue
                // walking up since it has no following sibling.
                parent->ChildAt(total - 1, getter_AddRefs(child));
                if (child != content) {
                    NS_RELEASE(mPosition.mContent);
                    parent->IndexOf(content, mCurrentIndex);
                    parent->ChildAt(++mCurrentIndex, &mPosition.mContent);

                    return PR_TRUE;
                }
            }

            content = parent;
            parent = content->GetParent();
        }
    }

    nsCOMPtr<nsIDocument> document = mPosition.mContent->GetDocument();
    document->GetChildCount(total);
    if (total <= 1) {
        return PR_FALSE;
    }

    document->ChildAt(total - 1, getter_AddRefs(child));
    if (child == content) {
        return PR_FALSE;
    }

    NS_RELEASE(mPosition.mContent);
    document->IndexOf(content, mCurrentIndex);
    document->ChildAt(++mCurrentIndex, &mPosition.mContent);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextPreceding()
{
    NS_ASSERTION(mPosition.isContent(),
                 "Wrong type, maybe you called moveToNextPreceding without "
                 "moveToFirstPreceding first?");

    nsCOMPtr<nsIContent> descendant, parent;
    nsCOMPtr<nsIDocument> document;
    PRInt32 index = mCurrentIndex;
    if (index > 0) {
        parent = mPosition.mContent->GetParent();
        if (parent) {
            parent->ChildAt(--index, getter_AddRefs(descendant));
        }
        else {
            document = mPosition.mContent->GetDocument();
            document->ChildAt(--index, getter_AddRefs(descendant));
        }
    }
    else {
        parent = mPosition.mContent->GetParent();

        PRInt32 count = mDescendants ? mDescendants->Count() : 0;
        if (count > 0) {
            if (parent) {
                NS_RELEASE(mPosition.mContent);
                mCurrentIndex = mDescendants->ValueAt(--count);
                mDescendants->RemoveValueAt(count);
                parent.swap(mPosition.mContent);

                return PR_TRUE;
            }

            document = mPosition.mContent->GetDocument();
            NS_RELEASE(mPosition.mContent);
            mCurrentIndex = mDescendants->ValueAt(--count);
            mDescendants->RemoveValueAt(count);
            parent.swap(mPosition.mContent);

            return PR_TRUE;
        }

        if (!parent) {
            return PR_FALSE;
        }

        descendant = parent;
        parent = descendant->GetParent();
        while (parent) {
            parent->IndexOf(descendant, index);
            if (index > 0) {
                parent->ChildAt(--index, getter_AddRefs(descendant));
                break;
            }

            descendant = parent;
            parent = descendant->GetParent();
        }

        if (!parent) {
            document = descendant->GetDocument();
            document->IndexOf(descendant, index);
            if (index == 0) {
                return PR_FALSE;
            }
            document->ChildAt(--index, getter_AddRefs(descendant));
        }
    }

    PRInt32 total;
    descendant->ChildCount(total);
    if (total == 0) {
        NS_RELEASE(mPosition.mContent);
        descendant.swap(mPosition.mContent);
        mCurrentIndex = index;

        return PR_TRUE;
    }

    while (total > 0) {
        if (!mDescendants) {
            mDescendants = new txInt32Array;
        }
        mDescendants->AppendValue(index);

        index = total - 1;
        parent = descendant;
        parent->ChildAt(index, getter_AddRefs(descendant));
        descendant->ChildCount(total);
    }

    NS_RELEASE(mPosition.mContent);
    descendant.swap(mPosition.mContent);
    mCurrentIndex = index;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextPrecedingInDocOrder()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    nsCOMPtr<nsIContent> parent;
    nsCOMPtr<nsIDocument> document;
    PRInt32 index = mCurrentIndex;
    if (mPosition.isContent()) {
        parent = mPosition.mContent->GetParent();
        if (parent && index == kUnknownIndex) {
            parent->IndexOf(mPosition.mContent, index);
        }
    }
    else {
        parent = mPosition.mContent;
        index = mPosition.mIndex;
    }

    nsCOMPtr<nsIContent> descendant;
    if (parent) {
        if (index == 0) {
            NS_RELEASE(mPosition.mContent);
            mPosition.mIndex = txXPathNode::eContent;
            parent.swap(mPosition.mContent);

            PRInt32 count = mDescendants ? mDescendants->Count() : 0;
            if (count > 0) {
                mCurrentIndex = mDescendants->ValueAt(--count);
                mDescendants->RemoveValueAt(count);
            }
            else {
                mCurrentIndex = kUnknownIndex;
            }

            return PR_TRUE;
        }

        if (mPosition.isAttribute()) {
            --mPosition.mIndex;

            return PR_TRUE;
        }

        parent->ChildAt(--index, getter_AddRefs(descendant));
    }
    else {
        document = mPosition.mContent->GetDocument();
        if (index == kUnknownIndex) {
            document->IndexOf(mPosition.mContent, index);
        }
        if (index == 0) {
            NS_RELEASE(mPosition.mContent);
            mPosition.mIndex = txXPathNode::eDocument;
            mPosition.mDocument = document;

            return PR_TRUE;
        }

        document->ChildAt(--index, getter_AddRefs(descendant));
    }

    PRInt32 total;
    descendant->ChildCount(total);

    while (total > 0) {
        index = total - 1;
        parent = descendant;
        parent->ChildAt(index, getter_AddRefs(descendant));
        descendant->ChildCount(total);
        if (total > 0) {
            if (!mDescendants) {
                mDescendants = new txInt32Array;
            }
            mDescendants->AppendValue(index);
        }
    }

    NS_RELEASE(mPosition.mContent);
    mPosition.mIndex = txXPathNode::eContent;
    descendant.swap(mPosition.mContent);
    mCurrentIndex = index;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextSibling()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    return moveToSibling(PR_TRUE);
}

PRBool
txXPathTreeWalker::moveToPreviousSibling()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    return moveToSibling(PR_FALSE);
}

PRBool
txXPathTreeWalker::moveToParent()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    if (mPosition.isAttribute()) {
        mPosition.mIndex = txXPathNode::eContent;
        mCurrentIndex = kUnknownIndex;

        return PR_TRUE;
    }

    nsCOMPtr<nsIContent> parent = mPosition.mContent->GetParent();
    if (parent) {
        NS_RELEASE(mPosition.mContent);
        parent.swap(mPosition.mContent);
        mCurrentIndex = kUnknownIndex;

        return PR_TRUE;
    }

    nsCOMPtr<nsIDocument> document = mPosition.mContent->GetDocument();
    if (!document) {
        return PR_FALSE;
    }

    // XXX Can we rely on this nulling out mPosition.mDocument too?
    NS_RELEASE(mPosition.mContent);
    mPosition.mIndex = txXPathNode::eDocument;
    document.swap(mPosition.mDocument);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToSibling(PRBool aForward)
{
    if  (mPosition.isContent()) {
        nsCOMPtr<nsIDocument> document;
        nsCOMPtr<nsIContent> parent = mPosition.mContent->GetParent();
        if (!parent) {
            document = mPosition.mContent->GetDocument();
            if (!document) {
                return PR_FALSE;
            }
        }
        if (mCurrentIndex == kUnknownIndex) {
            if (parent) {
                parent->IndexOf(mPosition.mContent, mCurrentIndex);
            }
            else {
                document->IndexOf(mPosition.mContent, mCurrentIndex);
            }
        }

        PRInt32 newIndex, total;
        if (aForward) {
            newIndex = mCurrentIndex + 1;
            if (parent) {
                parent->ChildCount(total);
            }
            else {
                document->GetChildCount(total);
            }
        }
        else {
            newIndex = mCurrentIndex - 1;
            total = -1;
        }

        if (newIndex == total) {
            return PR_FALSE;
        }

        NS_RELEASE(mPosition.mContent);
        if (parent) {
            parent->ChildAt(newIndex, &mPosition.mContent);
        }
        else {
            document->ChildAt(newIndex, &mPosition.mContent);
        }
        mCurrentIndex = newIndex;

        return PR_TRUE;
    }

    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> name, prefix;

    PRInt32 total;
    mPosition.mContent->GetAttrCount(total);
    PRInt32 newIndex = mPosition.mIndex;
    while (++newIndex < total) {
        mPosition.mContent->GetAttrNameAt(newIndex, &namespaceID,
                                          getter_AddRefs(name),
                                          getter_AddRefs(prefix));

        // We need to ignore XMLNS attributes.
        if (namespaceID != kNameSpaceID_XMLNS) {
            mPosition.mIndex = newIndex;

            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

/* static */
void
txXPathTreeWalker::setTo(txXPathNode& aNode, const txXPathNode& aOtherNode)
{
    aNode.mIndex = aOtherNode.mIndex;

    if (aNode.isDocument()) {
        aNode.mDocument = aOtherNode.mDocument;
        NS_ADDREF(aNode.mDocument);
    }
    else {
        aNode.mContent = aOtherNode.mContent;
        NS_ADDREF(aNode.mContent);
    }
}

txXPathNode::txXPathNode(const txXPathNode& aNode) : mIndex(aNode.mIndex)
{
    if (aNode.mIndex == eDocument) {
        mDocument = aNode.mDocument;
        NS_ADDREF(mDocument);
    }
    else {
        mContent = aNode.mContent;
        NS_ADDREF(mContent);
    }
}

txXPathNode::~txXPathNode()
{
    if (mIndex == eDocument) {
        NS_RELEASE(mDocument);
    }
    else {
        NS_RELEASE(mContent);
    }
}

PRBool
txXPathNode::operator==(const txXPathNode& aNode) const
{
    if (mIndex != aNode.mIndex) {
        return PR_FALSE;
    }

    if (isDocument()) {
        return (mDocument == aNode.mDocument);
    }

    return (mContent == aNode.mContent);
}

/* static */
PRBool
txXPathNodeUtils::getAttr(const txXPathNode& aNode, nsIAtom* aLocalName,
                          PRInt32 aNSID, nsAString& aValue)
{
    if (aNode.isDocument() || aNode.isAttribute()) {
        return PR_FALSE;
    }

    if (!aNode.mContent->IsContentOfType(nsIContent::eELEMENT)) {
        return PR_FALSE;
    }

    nsresult rv = aNode.mContent->GetAttr(aNSID, aLocalName, aValue);
    return NS_SUCCEEDED(rv) && rv != NS_CONTENT_ATTR_NOT_THERE;
}

/* static */
PRBool
txXPathNodeUtils::getLocalName(const txXPathNode& aNode,
                               nsIAtom** aLocalName)
{
    if (aNode.isDocument()) {
        *aLocalName = nsnull;

        return PR_TRUE;
    }

    if (aNode.isContent()) {
        if (aNode.mContent->IsContentOfType(nsIContent::eELEMENT)) {
            aNode.mContent->GetTag(aLocalName);

            return PR_TRUE;
        }

        nsCOMPtr<nsIDOMProcessingInstruction> node =
            do_QueryInterface(aNode.mContent);
        if (node) {
            nsAutoString target;
            node->GetNodeName(target);

            *aLocalName = NS_NewAtom(target);
            NS_ENSURE_TRUE(*aLocalName, PR_FALSE);

            return PR_TRUE;
        }

        *aLocalName = nsnull;

        return PR_TRUE;
    }

    nsCOMPtr<nsIAtom> prefix;
    PRInt32 namespaceID;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  aLocalName, getter_AddRefs(prefix));

    return PR_TRUE;
}

/* static */
PRBool
txXPathNodeUtils::getLocalName(const txXPathNode& aNode, nsAString& aLocalName)
{
    if (aNode.isDocument()) {
        return PR_TRUE;
    }

    if (aNode.isContent()) {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
        node->GetLocalName(aLocalName);

        return PR_TRUE;
    }

    // XXX Is this correct?
    nsCOMPtr<nsIAtom> prefix, localName;
    PRInt32 namespaceID;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(localName),
                                  getter_AddRefs(prefix));
    localName->ToString(aLocalName);

    return PR_TRUE;
}

/* static */
PRBool
txXPathNodeUtils::getNodeName(const txXPathNode& aNode, nsAString& aName)
{
    if (aNode.isDocument()) {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mDocument);
        if (!node) {
            return PR_FALSE;
        }

        node->GetNodeName(aName);

        return PR_TRUE;
    }

    if (aNode.isContent()) {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
        if (!node) {
            return PR_FALSE;
        }

        node->GetNodeName(aName);

        return PR_TRUE;
    }

PRInt32 test;
aNode.mContent->GetAttrCount(test);
NS_ASSERTION(aNode.mIndex < test, "Huh?");

    nsCOMPtr<nsIAtom> name, prefix;
    PRInt32 namespaceID;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(name),
                                  getter_AddRefs(prefix));
    if (prefix) {
      prefix->ToString(aName);
      aName.Append(PRUnichar(':'));
    }

    nsAutoString localName;
    name->ToString(localName);
    aName.Append(localName);

    return PR_TRUE;
}

/* static */
PRInt32
txXPathNodeUtils::getNamespaceID(const txXPathNode& aNode)
{
    if (aNode.isDocument()) {
        return kNameSpaceID_None;
    }

    PRInt32 namespaceID;
    if (aNode.isContent()) {
        aNode.mContent->GetNameSpaceID(&namespaceID);

        return namespaceID;
    }

    nsCOMPtr<nsIAtom> name, prefix;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(name),
                                  getter_AddRefs(prefix));
    return namespaceID;
}

/* static */
void
txXPathNodeUtils::getNamespaceURI(const txXPathNode& aNode, nsAString& aURI)
{
    PRInt32 namespaceID = getNamespaceID(aNode);
    extern nsINameSpaceManager* gTxNameSpaceManager;
    gTxNameSpaceManager->GetNameSpaceURI(namespaceID, aURI);
}

/* static */
PRUint16
txXPathNodeUtils::getNodeType(const txXPathNode& aNode)
{
    if (aNode.isDocument()) {
        return txXPathNodeType::DOCUMENT_NODE;
    }

    if (aNode.isContent()) {
        PRUint16 type;
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
        node->GetNodeType(&type);

        return type;
    }

    return txXPathNodeType::ATTRIBUTE_NODE;
}

/*
PRBool
txXPathNodeUtils::isNodeOfType(const txXPathNode& aNode, PRUint16 aType)
{
    if (aNode.isDocument()) {
        return (aType & txXPathNodeFilter::DOCUMENT_NODE);
    }

    if (aNode.isAttribute()) {
        return (aType & txXPathNodeFilter::ATTRIBUTE_NODE);
    }

    if (aType & txXPathNodeFilter::ELEMENT_NODE &&
        aNode.mContent->IsContentOfType(nsIContent::eELEMENT)) {
        return PR_TRUE;
    }

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
    PRUint16 type;
    node->GetNodeType(&type);
    return (PR_TRUE);
}
*/

static void getTextContent(nsIContent* aElement, nsAString& aResult)
{
    PRInt32 total, i;
    aElement->ChildCount(total);
    for (i = 0; i < total; ++i) {
        nsCOMPtr<nsIContent> content;
        aElement->ChildAt(i, getter_AddRefs(content));
        if (content->IsContentOfType(nsIContent::eELEMENT)) {
            getTextContent(content, aResult);
        }
        else {
            nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);
            PRUint16 nodeType;
            node->GetNodeType(&nodeType);
            if (nodeType == nsIDOMNode::CDATA_SECTION_NODE ||
                nodeType == nsIDOMNode::TEXT_NODE) {
                nsCOMPtr<nsITextContent> textContent = do_QueryInterface(content);
                textContent->AppendTextTo(aResult);
            }
        }
    }
}

/* static */
void
txXPathNodeUtils::getNodeValue(const txXPathNode& aNode, nsAString& aResult)
{
    if (aNode.isAttribute()) {
        nsCOMPtr<nsIAtom> name, prefix;
        PRInt32 namespaceID;
        aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                      getter_AddRefs(name),
                                      getter_AddRefs(prefix));

        nsAutoString result;
        aNode.mContent->GetAttr(namespaceID, name, result);
        aResult.Append(result);

        return;
    }

    nsCOMPtr<nsIContent> content;
    if (aNode.isDocument()) {
        aNode.mDocument->GetRootContent(getter_AddRefs(content));
        if (!content) {
            return;
        }
    }
    else {
        content = aNode.mContent;
    }

    if (content->IsContentOfType(nsIContent::eELEMENT)) {
        getTextContent(content, aResult);
        return;
    }

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);
    PRUint16 nodeType;
    node->GetNodeType(&nodeType);
    if (nodeType == nsIDOMNode::CDATA_SECTION_NODE ||
        nodeType == nsIDOMNode::COMMENT_NODE ||
        nodeType == nsIDOMNode::TEXT_NODE) {
        nsCOMPtr<nsITextContent> textContent = do_QueryInterface(content);
        textContent->AppendTextTo(aResult);
    }
    else if (nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE) {
        nsAutoString result;
        node->GetNodeValue(result);
        aResult.Append(result);
    }
}

/* static */
PRBool
txXPathNodeUtils::isWhitespace(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.isContent(), "Wrong type!");

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
    nsAutoString text;
    node->GetNodeValue(text);
    return XMLUtils::isWhitespace(text);
}

/* static */
txXPathNode*
txXPathNodeUtils::getOwnerDocument(const txXPathNode& aNode)
{
    if (aNode.isDocument()) {
        return new txXPathNode(aNode);
    }

    nsCOMPtr<nsIDocument> document = aNode.mContent->GetDocument();
    txXPathNode* node = new txXPathNode(document);

    return node;
}

/* static */
txXPathNode*
txXPathNodeUtils::getElementById(const txXPathNode& aDocument,
                                 const nsAString& aID)
{
    if (!aDocument.isDocument()) {
        return nsnull;
    }

    nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(aDocument.mDocument);
    nsCOMPtr<nsIDOMElement> element;
    document->GetElementById(aID, getter_AddRefs(element));
    if (!element) {
        return nsnull;
    }

    return txXPathNativeNode::createXPathNode(element);
}

#ifndef HAVE_64BIT_OS
#define kFmtSize 13
#define kFmtSizeAttr 24
const char gPrintfFmt[] = "id0x%08p";
const char gPrintfFmtAttr[] = "id0x%08p-%010i";
#else
#define kFmtSize 21
#define kFmtSizeAttr 32
const char gPrintfFmt[] = "id0x%016p";
const char grintfFmtAttr[] = "id0x%016p-%010i";
#endif

/* static */
nsresult
txXPathNodeUtils::getXSLTId(const txXPathNode& aNode,
                            nsAString& aResult)
{
    if (aNode.isDocument()) {
        CopyASCIItoUCS2(nsPrintfCString(kFmtSize, gPrintfFmt, aNode.mDocument), aResult);

        return NS_OK;
    }

    if (aNode.isContent()) {
        CopyASCIItoUCS2(nsPrintfCString(kFmtSize, gPrintfFmt, aNode.mContent), aResult);

        return NS_OK;
    }

    CopyASCIItoUCS2(nsPrintfCString(kFmtSizeAttr, gPrintfFmtAttr, aNode.mContent,
                                    aNode.mIndex), aResult);

    return NS_OK;
}

/* static */
void
txXPathNodeUtils::getBaseURI(const txXPathNode& aNode, nsAString& aURI)
{
    nsCOMPtr<nsIDOM3Node> node;
    if (aNode.isDocument()) {
        node = do_QueryInterface(aNode.mDocument);
    }
    else {
        node = do_QueryInterface(aNode.mContent);
    }

    if (node) {
        node->GetBaseURI(aURI);
    }
}

/* static */
PRIntn
txXPathNodeUtils::comparePosition(const txXPathNode& aNode,
                                  const txXPathNode& aOtherNode)
{
    // First check for equal nodes or attribute-nodes on the same element.
    if (aNode.mContent == aOtherNode.mContent) {
        if (aNode.mIndex == aOtherNode.mIndex) {
            return 0;
        }

        // The isContent tests can be removed if we rely on that mIndex < 0
        // for content.
        if (aNode.isContent() || (!aOtherNode.isContent() &&
                                  aNode.mIndex < aOtherNode.mIndex)) {
            return -1;
        }

        return 1;
    }

    // Get document for both nodes.
    nsIDocument* document;
    if (aNode.isDocument()) {
        document = aNode.mDocument;
    }
    else {
        document = aNode.mContent->GetDocument();
    }

    nsIDocument* otherDocument;
    if (aOtherNode.isDocument()) {
        otherDocument = aOtherNode.mDocument;
    }
    else {
        otherDocument = aOtherNode.mContent->GetDocument();
    }

    // If the nodes have different ownerdocuments, compare the document
    // pointers.
    if (document != otherDocument) {
        return (document > otherDocument ? 1 : -1);
    }

    // Every node comes after its ownerdocument.
    if (aNode.isDocument()) {
        return -1;
    }

    if (aOtherNode.isDocument()) {
        return 1;
    }

    // Get parents up the tree.
    nsAutoVoidArray parents, otherParents;
    nsIContent* content = aNode.mContent;
    nsIContent* otherContent = aOtherNode.mContent;
    nsIContent* parent, *otherParent;
    PRInt32 index, otherIndex;
    while (content && otherContent) {
        parent = content->GetParent();
        otherParent = otherContent->GetParent();

        // Hopefully this is a common case.
        if (parent == otherParent) {
            if (parent) {
                parent->IndexOf(content, index);
                parent->IndexOf(otherContent, otherIndex);
            }
            else {
                document->IndexOf(content, index);
                document->IndexOf(otherContent, otherIndex);
            }
            return index < otherIndex ? -1 : 1;
        }

        parents.AppendElement(content);
        otherParents.AppendElement(otherContent);
        content = parent;
        otherContent = otherParent;
    }

    while (content) {
        parents.AppendElement(content);
        content = content->GetParent();
    }
    while (otherContent) {
        otherParents.AppendElement(otherContent);
        otherContent = otherContent->GetParent();
    }

    // Walk back down along the parent-chains until we find where they split.
    PRInt32 total = parents.Count() - 1;
    PRInt32 otherTotal = otherParents.Count() - 1;
    NS_ASSERTION(total != otherTotal, "Can't have same number of parents");

    PRInt32 lastIndex = PR_MIN(total, otherTotal);
    PRInt32 i;
    parent = nsnull;
    for (i = 0; i <= lastIndex; ++i) {
        content = NS_STATIC_CAST(nsIContent*, parents.ElementAt(total - i));
        otherContent = NS_STATIC_CAST(nsIContent*,
                                      otherParents.ElementAt(otherTotal - i));
        if (content != otherContent) {
            if (parent) {
                parent->IndexOf(content, index);
                parent->IndexOf(otherContent, otherIndex);
            }
            else {
                document->IndexOf(content, index);
                document->IndexOf(otherContent, otherIndex);
            }
            NS_ASSERTION(index != otherIndex && index >= 0 && otherIndex >= 0,
                         "invalid index in compareTreePosition");
            return index < otherIndex ? -1 : 1;
        }

        parent = content;
    }

    // One node is a descendant of the other. The one with the shortest
    // parent-chain is first in the document.
    return total < otherTotal ? -1 : 1;
}

/* static */
txXPathNode*
txXPathNativeNode::createXPathNode(nsIDOMNode* aNode)
{
    PRUint16 nodeType;
    aNode->GetNodeType(&nodeType);

    if (nodeType == nsIDOMNode::ATTRIBUTE_NODE) {
        nsCOMPtr<nsIAttribute> attr = do_QueryInterface(aNode);

        nsCOMPtr<nsIContent> parent;
        attr->GetContent(getter_AddRefs(parent));

        nsCOMPtr<nsINodeInfo> nodeInfo;
        attr->GetNodeInfo(getter_AddRefs(nodeInfo));

        nsCOMPtr<nsIAtom> attName, attPrefix;
        PRInt32 attNS;

        PRInt32 total, i;
        parent->GetAttrCount(total);
        for (i = 0; i < total; ++i) {
            parent->GetAttrNameAt(i, &attNS, getter_AddRefs(attName),
                                  getter_AddRefs(attPrefix));
            if (nodeInfo->Equals(attName, attNS)) {
                return new txXPathNode(parent, i);
            }
        }

        NS_ASSERTION(0, "Couldn't find the attribute in its parent!");

        return nsnull;
    }

    if (nodeType == nsIDOMNode::DOCUMENT_NODE) {
        nsCOMPtr<nsIDocument> document = do_QueryInterface(aNode);
        return new txXPathNode(document);
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(aNode);
    return new txXPathNode(content);
}

// XXXX Inline?

/* static */
txXPathNode*
txXPathNativeNode::createXPathNode(nsIDOMDocument* aDocument)
{
    nsCOMPtr<nsIDocument> document = do_QueryInterface(aDocument);
    return new txXPathNode(document);
}

// XXXX Inline?

/* static */
txXPathNode*
txXPathNativeNode::createXPathNode(nsIContent* aContent)
{
    nsCOMPtr<nsIContent> content = aContent;
    return new txXPathNode(content);
}

/* static */
nsresult
txXPathNativeNode::getNode(const txXPathNode& aNode, nsIDOMNode** aResult)
{
    if (aNode.isDocument()) {
        return CallQueryInterface(aNode.mDocument, aResult);
    }

    if (aNode.isContent()) {
        return CallQueryInterface(aNode.mContent, aResult);
    }

    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> name, prefix;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(name),
                                  getter_AddRefs(prefix));

    nsAutoString namespaceURI, localname;
    extern nsINameSpaceManager* gTxNameSpaceManager;
    gTxNameSpaceManager->GetNameSpaceURI(namespaceID, namespaceURI);
    name->ToString(localname);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode.mContent);
    nsCOMPtr<nsIDOMAttr> attr;
    element->GetAttributeNodeNS(namespaceURI, localname,
                                getter_AddRefs(attr));

    return CallQueryInterface(attr, aResult);
}

/* static */
nsresult
txXPathNativeNode::getContent(const txXPathNode& aNode, nsIContent** aResult)
{
    if (!aNode.isContent()) {
        return NS_ERROR_FAILURE;
    }

    *aResult = aNode.mContent;
    NS_ADDREF(*aResult);

    return NS_OK;
}

/* static */
nsresult
txXPathNativeNode::getDocument(const txXPathNode& aNode, nsIDocument** aResult)
{
    if (!aNode.isDocument()) {
        return NS_ERROR_FAILURE;
    }

    *aResult = aNode.mDocument;
    NS_ADDREF(*aResult);

    return NS_OK;
}
