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

const PRUint32 kUnknownIndex = PRUint32(-1);

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
        document = do_QueryInterface(mPosition.mContent->GetDocument());
    }

    nsCOMPtr<nsIDOMElement> element;
    document->GetElementById(aID, getter_AddRefs(element));
    if (!element) {
        return PR_FALSE;
    }

    nsCOMPtr<nsIContent> content = do_QueryInterface(element);

    mPosition.mIndex = txXPathNode::eContent;
    mPosition.mContent = content;
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

    return moveToValidAttribute(0);
}

PRBool
txXPathTreeWalker::moveToFirstChild()
{
    if (mPosition.isAttribute()) {
        return PR_FALSE;
    }

    if (mPosition.isDocument()) {
        PRUint32 total = mPosition.mDocument->GetChildCount();
        if (total == 0) {
            return PR_FALSE;
        }

        mPosition.mIndex = txXPathNode::eContent;
        mPosition.mContent = mPosition.mDocument->GetChildAt(0);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    PRUint32 total = mPosition.mContent->GetChildCount();
    if (total == 0) {
        return PR_FALSE;
    }

    mPosition.mContent = mPosition.mContent->GetChildAt(0);
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

    if (mPosition.isAttribute()) {
        // Check if the attribute's parent has any children.
        PRUint32 total = mPosition.mContent->GetChildCount();
        if (total > 0) {
            mPosition.mIndex = txXPathNode::eContent;
            mPosition.mContent = mPosition.mContent->GetChildAt(0);
            mCurrentIndex = 0;

            return PR_TRUE;
        }
    }

    // Now walk up the parent chain to find the first ancestor that has a
    // following sibling.
    nsIContent *content = mPosition.mContent;
    nsIContent *parent;
    while ((parent = content->GetParent())) {
        // Check if content is the last child of its parent, if not we have
        // a following sibling.
        PRUint32 total = parent->GetChildCount();
        if (total > 1 && parent->GetChildAt(total - 1) != content) {
            mPosition.mIndex = txXPathNode::eContent;
            mCurrentIndex = (PRUint32)parent->IndexOf(content);
            mPosition.mContent = parent->GetChildAt(++mCurrentIndex);

            return PR_TRUE;
        }

        content = parent;
    }

    // Check if content is the last child of the document, if not we have
    // a following sibling.
    nsIDocument *document = content->GetDocument();
    PRUint32 total = document->GetChildCount();
    if (total == 0 || document->GetChildAt(total - 1) == content) {
        return PR_FALSE;
    }

    mPosition.mIndex = txXPathNode::eContent;
    mCurrentIndex = (PRUint32)document->IndexOf(content);
    mPosition.mContent = document->GetChildAt(++mCurrentIndex);

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
    nsIDocument *document = nsnull;
    PRUint32 index;
    nsIContent* parent = mPosition.mContent->GetParent();
    if (mCurrentIndex != kUnknownIndex && mCurrentIndex != 0) {
        index = mCurrentIndex;
        if (!parent) {
            document = mPosition.mContent->GetDocument();
        }
    }
    else {
        nsIContent *content = mPosition.mContent;
        while (parent) {
            index = (PRUint32)parent->IndexOf(content);
            if (index > 0) {
                break;
            }

            content = parent;
            parent = content->GetParent();
        }

        if (!parent) {
            document = content->GetDocument();
            index = (PRUint32)document->IndexOf(content);
            if (index == 0) {
                return PR_FALSE;
            }
        }
    }

    NS_ASSERTION(parent || document, "UHOH");

    // Find the last descendant of the preceding sibling.
    nsIContent *precedingSibling = parent ? parent->GetChildAt(--index) :
                                            document->GetChildAt(--index);

    mPosition.mIndex = txXPathNode::eContent;
    PRUint32 total = precedingSibling->GetChildCount();
    if (total == 0) {
        mPosition.mContent = precedingSibling;
        mCurrentIndex = index;

        return PR_TRUE;
    }

    --total;
    moveToLastDescendant(precedingSibling->GetChildAt(total), total);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstPrecedingInDocOrder()
{
    if (mDescendants) {
        // we're new, clean up previous handling
        mDescendants->Clear();
    }

    if (mPosition.isAttribute()) {
        mPosition.mIndex = txXPathNode::eContent;
        mCurrentIndex = kUnknownIndex;

        return PR_TRUE;
    }

    return moveToNextPrecedingInDocOrder();
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

    PRUint32 total = mPosition.mContent->GetChildCount();
    if (total > 0) {
        // Move to first child.
        if (!mDescendants) {
            mDescendants = new txUint32Array;
        }
    
        mDescendants->AppendValue(mCurrentIndex);
        mPosition.mContent = mPosition.mContent->GetChildAt(0);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    PRUint32 index = mCurrentIndex + 1;
    nsIContent *parent = mPosition.mContent->GetParent();
    if (parent) {
        // Check if the current position has a following sibling.
        total = parent->GetChildCount();
        if (index < total) {
            mPosition.mContent = parent->GetChildAt(index);
            mCurrentIndex = index;

            return PR_TRUE;
        }

        if (!mDescendants) {
            return PR_FALSE;
        }

        PRInt32 currentIndex = mDescendants->Count();
        if (currentIndex == 0) {
            return PR_FALSE;
        }

        nsIContent *content;
        while ((content = parent) && (parent = content->GetParent())) {
            index = mDescendants->ValueAt(--currentIndex) + 1;
            total = parent->GetChildCount();
            if (index < total) {
                mPosition.mContent = parent->GetChildAt(index);
                mCurrentIndex = index;
                mDescendants->RemoveValuesAt(currentIndex,
                                             mDescendants->Count() - currentIndex);

                return PR_TRUE;
            }

            if (currentIndex == 0) {
                return PR_FALSE;
            }
        }

        NS_ASSERTION(currentIndex == 1, "HUH?");

        index = mDescendants->ValueAt(0) + 1;
    }

    nsIDocument *document = mPosition.mContent->GetDocument();
    total = document->GetChildCount();
    if (index >= total) {
        return PR_FALSE;
    }

    mPosition.mContent = document->GetChildAt(index);
    mCurrentIndex = index;

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
    PRUint32 total = mPosition.mContent->GetChildCount();
    if (total > 0) {
        mPosition.mContent = mPosition.mContent->GetChildAt(0);
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    // Look at next siblings of the current content.
    nsIContent *content;
    nsIContent *parent = mPosition.mContent->GetParent();
    if (parent) {
        total = parent->GetChildCount();
        if (mCurrentIndex + 1 < total) {
            mPosition.mContent = parent->GetChildAt(++mCurrentIndex);

            return PR_TRUE;
        }

        // Now walk up the parent chain to find the first ancestor that has a
        // following sibling.
        while ((content = parent) && (parent = content->GetParent())) {
            // First check if content has any following siblings
            total = parent->GetChildCount();
            if (total > 1 && parent->GetChildAt(total - 1) != content) {
                // If content is the last child of its parent we can continue
                // walking up since it has no following sibling.
                mCurrentIndex = (PRUint32)parent->IndexOf(content);
                mPosition.mContent = parent->GetChildAt(++mCurrentIndex);

                return PR_TRUE;
            }
        }
    }
    else {
        content = mPosition.mContent;
    }

    nsIDocument *document = content->GetDocument();
    total = document->GetChildCount();
    if (total <= 1 || document->GetChildAt(total - 1) == content) {
        return PR_FALSE;
    }

    mCurrentIndex = (PRUint32)document->IndexOf(content);
    mPosition.mContent = document->GetChildAt(++mCurrentIndex);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextPreceding()
{
    NS_ASSERTION(mPosition.isContent(),
                 "Wrong type, maybe you called moveToNextPreceding without "
                 "moveToFirstPreceding first?");

    nsIContent *descendant, *parent = mPosition.mContent->GetParent();
    PRUint32 index;
    if (mCurrentIndex != kUnknownIndex && mCurrentIndex != 0) {
        index = mCurrentIndex;
        if (parent) {
            descendant = parent->GetChildAt(--index);
        }
        else {
            nsIDocument *document = mPosition.mContent->GetDocument();
            descendant = document->GetChildAt(--index);
        }
    }
    else {
        PRInt32 count = mDescendants ? mDescendants->Count() : 0;
        if (count > 0) {
            NS_ASSERTION(parent, "HUH?");

            mPosition.mContent = parent;
            mCurrentIndex = mDescendants->ValueAt(--count);
            mDescendants->RemoveValueAt(count);

            return PR_TRUE;
        }

        if (!parent) {
            descendant = mPosition.mContent;
        }
        else {
            while ((descendant = parent) && (parent = descendant->GetParent())) {
                index = (PRUint32)parent->IndexOf(descendant);
                if (index > 0) {
                    descendant = parent->GetChildAt(--index);
                    break;
                }
            }
        }

        if (!parent) {
            nsIDocument *document = descendant->GetDocument();
            index = (PRUint32)document->IndexOf(descendant);
            if (index == 0) {
                return PR_FALSE;
            }

            descendant = document->GetChildAt(--index);
        }
    }

    moveToLastDescendant(descendant, index);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextPrecedingInDocOrder()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }
    NS_ASSERTION(mPosition.isContent(), "we should only enumerate content");

    // find previous sibling or parent
    nsIContent *precedingSibling;
    PRUint32 index = mCurrentIndex;

    nsIContent *parent = mPosition.mContent->GetParent();
    if (parent) {
        if (index == kUnknownIndex) {
            index = (PRUint32)parent->IndexOf(mPosition.mContent);
        }

        if (index == 0) {
            mPosition.mIndex = txXPathNode::eContent;
            mPosition.mContent = parent;

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

        precedingSibling = parent->GetChildAt(--index);
    }
    else {
        nsIDocument *document = mPosition.mContent->GetDocument();
        if (index == kUnknownIndex) {
            index = (PRUint32)document->IndexOf(mPosition.mContent);
        }

        if (index == 0) {
            mPosition.mIndex = txXPathNode::eDocument;
            mPosition.mDocument = document;

            return PR_TRUE;
        }

        precedingSibling = document->GetChildAt(--index);
    }

    // Find last descendant of preceding sibling.
    mPosition.mIndex = txXPathNode::eContent;
    PRUint32 total = precedingSibling->GetChildCount();
    if (total == 0) {
        mPosition.mContent = precedingSibling;
        mCurrentIndex = index;

        return PR_TRUE;
    }

    --total;
    moveToLastDescendant(precedingSibling->GetChildAt(total), total);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextSibling()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    if (mPosition.isAttribute()) {
        return moveToValidAttribute(mPosition.mIndex + 1);
    }

    return moveToSibling(PR_TRUE);
}

PRBool
txXPathTreeWalker::moveToPreviousSibling()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    NS_ASSERTION(mPosition.isContent(),
                 "We don't support moveToPreviousSibling on attributes (yet?).");

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

    nsIContent *parent = mPosition.mContent->GetParent();
    if (parent) {
        mPosition.mContent = parent;
        mCurrentIndex = kUnknownIndex;

        return PR_TRUE;
    }

    nsIDocument *document = mPosition.mContent->GetDocument();
    if (!document) {
        return PR_FALSE;
    }

    mPosition.mIndex = txXPathNode::eDocument;
    mPosition.mDocument = document;

    return PR_TRUE;
}

void
txXPathTreeWalker::moveToLastDescendant(nsIContent* aDescendant,
                                        PRUint32 aDescendantIndex)
{
    mPosition.mContent = aDescendant;
    mCurrentIndex = aDescendantIndex;

    PRUint32 total = aDescendant->GetChildCount();
    while (total > 0) {
        if (!mDescendants) {
            mDescendants = new txUint32Array;
        }
        mDescendants->AppendValue(mCurrentIndex);

        mCurrentIndex = total - 1;
        mPosition.mContent = mPosition.mContent->GetChildAt(mCurrentIndex);
        total = mPosition.mContent->GetChildCount();
    }
}

PRBool
txXPathTreeWalker::moveToSibling(PRBool aForward)
{
    nsIDocument *document;
    nsIContent *parent = mPosition.mContent->GetParent();
    if (!parent) {
        document = mPosition.mContent->GetDocument();
        if (!document) {
            return PR_FALSE;
        }
    }

    if (mCurrentIndex == kUnknownIndex) {
        mCurrentIndex = parent ?
                        (PRUint32)parent->IndexOf(mPosition.mContent) :
                        (PRUint32)document->IndexOf(mPosition.mContent);
    }

    if (aForward) {
        PRUint32 total = parent ? parent->GetChildCount() :
                                  document->GetChildCount();
        if (mCurrentIndex + 1 == total) {
            return PR_FALSE;
        }

        ++mCurrentIndex;
    }
    else {
        if (mCurrentIndex == 0) {
            return PR_FALSE;
        }

        --mCurrentIndex;
    }

    mPosition.mContent = parent ? parent->GetChildAt(mCurrentIndex) :
                                  document->GetChildAt(mCurrentIndex);

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToValidAttribute(PRUint32 aStartIndex)
{
    PRUint32 total = mPosition.mContent->GetAttrCount();
    if (aStartIndex >= total) {
        return PR_FALSE;
    }

    PRInt32 namespaceID;
    nsCOMPtr<nsIAtom> name, prefix;

    PRUint32 index;
    for (index = aStartIndex; index < total; ++index) {
        mPosition.mContent->GetAttrNameAt(index, &namespaceID,
                                          getter_AddRefs(name),
                                          getter_AddRefs(prefix));

        // We need to ignore XMLNS attributes.
        if (namespaceID != kNameSpaceID_XMLNS) {
            mPosition.mIndex = index;

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
    }
    else {
        aNode.mContent = aOtherNode.mContent;
    }
}

txXPathNode::txXPathNode(const txXPathNode& aNode) : mIndex(aNode.mIndex)
{
    if (aNode.isDocument()) {
        mDocument = aNode.mDocument;
    }
    else {
        mContent = aNode.mContent;
    }
}

txXPathNode::~txXPathNode()
{
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
    if (aNode.isAttribute()) {
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

    nsCOMPtr<nsIDOMNode> node = aNode.isContent() ?
                                do_QueryInterface(aNode.mContent) :
                                do_QueryInterface(aNode.mDocument);
    if (!node) {
        return PR_FALSE;
    }

    node->GetNodeName(aName);

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

static void getTextContent(nsIContent* aElement, nsAString& aResult)
{
    PRUint32 i, total = aElement->GetChildCount();
    for (i = 0; i < total; ++i) {
        nsIContent* content = aElement->GetChildAt(i);
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

    if (content->IsContentOfType(nsIContent::eTEXT)) {
        nsCOMPtr<nsITextContent> textContent = do_QueryInterface(content);
        textContent->AppendTextTo(aResult);
        return;
    }

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(content);
    PRUint16 nodeType;
    node->GetNodeType(&nodeType);
    if (nodeType == nsIDOMNode::COMMENT_NODE) {
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

    return new txXPathNode(aNode.mContent->GetDocument());
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
    nsCOMPtr<nsIContent> content = do_QueryInterface(element);
    if (!content) {
        return nsnull;
    }

    return new txXPathNode(content);
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
    nsIDocument* document = aNode.isDocument() ?
                            aNode.mDocument :
                            aNode.mContent->GetDocument();

    nsIDocument* otherDocument = aOtherNode.isDocument() ?
                                 aOtherNode.mDocument :
                                 aOtherNode.mContent->GetDocument();

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
                index = (PRUint32)parent->IndexOf(content);
                otherIndex = (PRUint32)parent->IndexOf(otherContent);
            }
            else {
                index = (PRUint32)document->IndexOf(content);
                otherIndex = (PRUint32)document->IndexOf(otherContent);
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
                index = (PRUint32)parent->IndexOf(content);
                otherIndex = (PRUint32)parent->IndexOf(otherContent);
            }
            else {
                index = (PRUint32)document->IndexOf(content);
                otherIndex = (PRUint32)document->IndexOf(otherContent);
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

        PRUint32 i, total = parent->GetAttrCount();
        for (i = 0; i < total; ++i) {
            parent->GetAttrNameAt(i, &attNS, getter_AddRefs(attName),
                                  getter_AddRefs(attPrefix));
            if (nodeInfo->Equals(attName, attNS)) {
                return new txXPathNode(parent, i);
            }
        }

        NS_ERROR("Couldn't find the attribute in its parent!");

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

    NS_ADDREF(*aResult = aNode.mContent);

    return NS_OK;
}

/* static */
nsresult
txXPathNativeNode::getDocument(const txXPathNode& aNode, nsIDocument** aResult)
{
    if (!aNode.isDocument()) {
        return NS_ERROR_FAILURE;
    }

    NS_ADDREF(*aResult = aNode.mDocument);

    return NS_OK;
}
