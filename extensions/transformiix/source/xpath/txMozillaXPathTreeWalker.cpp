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
#include "nsUnicharUtils.h"

const PRUint32 kUnknownIndex = PRUint32(-1);

txXPathTreeWalker::txXPathTreeWalker(const txXPathTreeWalker& aOther)
    : mPosition(aOther.mPosition),
      mCurrentIndex(aOther.mCurrentIndex)
{
}

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
    mDescendants.Clear();

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstAttribute()
{
    if (!mPosition.isContent()) {
        return PR_FALSE;
    }

    return moveToValidAttribute(0);
}

PRBool
txXPathTreeWalker::moveToNextAttribute()
{
    // XXX an assertion should be enough here with the current code
    if (!mPosition.isAttribute()) {
        return PR_FALSE;
    }

    return moveToValidAttribute(mPosition.mIndex + 1);
}

PRBool
txXPathTreeWalker::moveToValidAttribute(PRUint32 aStartIndex)
{
    NS_ASSERTION(!mPosition.isDocument(), "documents doesn't have attrs");

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

PRBool
txXPathTreeWalker::moveToFirstChild()
{
    if (mPosition.isAttribute()) {
        return PR_FALSE;
    }

    if (mPosition.isDocument()) {
        nsIContent* child = mPosition.mDocument->GetChildAt(0);
        if (!child) {
            return PR_FALSE;
        }
        mPosition.mIndex = txXPathNode::eContent;
        mPosition.mContent = child;

        NS_ASSERTION(mCurrentIndex == kUnknownIndex && !mDescendants.Count(),
                     "we shouldn't have any position info at the document");
        mCurrentIndex = 0;

        return PR_TRUE;
    }

    nsIContent* child = mPosition.mContent->GetChildAt(0);
    if (!child) {
        return PR_FALSE;
    }
    mPosition.mContent = child;

    NS_ASSERTION(mCurrentIndex != kUnknownIndex || !mDescendants.Count(),
                 "Index should be known if parents index are");
    if (mCurrentIndex != kUnknownIndex) {
        mDescendants.AppendValue(mCurrentIndex);
    }
    mCurrentIndex = 0;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToLastChild()
{
    if (mPosition.isAttribute()) {
        return PR_FALSE;
    }

    if (mPosition.isDocument()) {
        PRUint32 total = mPosition.mDocument->GetChildCount();
        if (!total) {
            return PR_FALSE;
        }

        mPosition.mIndex = txXPathNode::eContent;
        mPosition.mContent = mPosition.mDocument->GetChildAt(total - 1);
        NS_ASSERTION(mCurrentIndex == kUnknownIndex && !mDescendants.Count(),
                     "we shouldn't have any position info at the document");
        mCurrentIndex = total - 1;

        return PR_TRUE;
    }

    PRUint32 total = mPosition.mContent->GetChildCount();
    if (!total) {
        return PR_FALSE;
    }
    mPosition.mContent = mPosition.mContent->GetChildAt(total - 1);

    NS_ASSERTION(mCurrentIndex != kUnknownIndex || !mDescendants.Count(),
                 "Index should be known if parents index are");
    if (mCurrentIndex != kUnknownIndex) {
        mDescendants.AppendValue(mCurrentIndex);
    }
    mCurrentIndex = total - 1;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextSibling()
{
    if (!mPosition.isContent()) {
        return PR_FALSE;
    }

    return moveToSibling(1);
}

PRBool
txXPathTreeWalker::moveToPreviousSibling()
{
    if (!mPosition.isContent()) {
        return PR_FALSE;
    }

    return moveToSibling(-1);
}

PRBool
txXPathTreeWalker::moveToParent()
{
    if (mPosition.isDocument()) {
        return PR_FALSE;
    }

    if (mPosition.isAttribute()) {
        mPosition.mIndex = txXPathNode::eContent;

        return PR_TRUE;
    }

    nsIContent *parent = mPosition.mContent->GetParent();
    if (parent) {
        mPosition.mContent = parent;
        PRInt32 count = mDescendants.Count();
        if (count) {
            mCurrentIndex = mDescendants.ValueAt(--count);
            mDescendants.RemoveValueAt(count);
        }
        else {
            mCurrentIndex = kUnknownIndex;
        }

        return PR_TRUE;
    }

    nsIDocument* document = mPosition.mContent->GetDocument();
    // XXX do we need this test? we assume in a bunch of other places that the node
    // is in the doc so we should probably enforce that during bootstrap
    if (!document) {
        return PR_FALSE;
    }
    mPosition.mIndex = txXPathNode::eDocument;
    mPosition.mDocument = document;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToSibling(PRInt32 aDir)
{
    NS_ASSERTION(mPosition.isContent(), "moveToSibling should only be called for content");

    nsIDocument* document;
    nsIContent* parent = mPosition.mContent->GetParent();
    if (!parent) {
        document = mPosition.mContent->GetDocument();
        if (!document) {
            return PR_FALSE; // XXX can this really happen?
        }
    }
    if (mCurrentIndex == kUnknownIndex) {
        mCurrentIndex = parent ? parent->IndexOf(mPosition.mContent)
                               : document->IndexOf(mPosition.mContent);
    }

    PRInt32 newIndex = mCurrentIndex + aDir;
    nsIContent* newChild = parent ? parent->GetChildAt(newIndex)
                                  : document->GetChildAt(newIndex);
    if (!newChild) {
        return PR_FALSE;
    }

    mPosition.mContent = newChild;
    mCurrentIndex = newIndex;

    return PR_TRUE;
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
void
txXPathNodeUtils::getLocalName(const txXPathNode& aNode, nsAString& aLocalName)
{
    if (aNode.isDocument()) {
        aLocalName.Truncate();

        return;
    }

    if (aNode.isContent()) {
        nsINodeInfo* nodeInfo = aNode.mContent->GetNodeInfo();
        if (nodeInfo) {
            nodeInfo->GetLocalName(aLocalName);

            // Check for html
            if (aNode.mContent->IsContentOfType(nsIContent::eHTML) &&
                nodeInfo->NamespaceEquals(kNameSpaceID_None)) {
                ToUpperCase(aLocalName);
            }

            return;
        }

        if (aNode.mContent->IsContentOfType(nsIContent::ePROCESSING_INSTRUCTION)) {
            // PIs don't have a nodeinfo but do have a name
            nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
            node->GetNodeName(aLocalName);

            return;
        }

        aLocalName.Truncate();

        return;
    }

    nsCOMPtr<nsIAtom> prefix, localName;
    PRInt32 namespaceID;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(localName),
                                  getter_AddRefs(prefix));
    localName->ToString(aLocalName);

    // Check for html
    if (aNode.mContent->IsContentOfType(nsIContent::eHTML) &&
        aNode.mContent->GetNodeInfo()->NamespaceEquals(kNameSpaceID_None)) {
        ToUpperCase(aLocalName);
    }
}

/* static */
void
txXPathNodeUtils::getNodeName(const txXPathNode& aNode, nsAString& aName)
{
    if (aNode.isDocument()) {
        aName.Truncate();

        return;
    }

    if (aNode.isContent()) {
        nsINodeInfo* nodeInfo = aNode.mContent->GetNodeInfo();
        if (nodeInfo) {
            nodeInfo->GetQualifiedName(aName);

            // Check for html
            if (aNode.mContent->IsContentOfType(nsIContent::eHTML) &&
                nodeInfo->NamespaceEquals(kNameSpaceID_None)) {
                ToUpperCase(aName);
            }

            return;
        }

        if (aNode.mContent->IsContentOfType(nsIContent::ePROCESSING_INSTRUCTION)) {
            // PIs don't have a nodeinfo but do have a name
            nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
            node->GetNodeName(aName);
        }

        return;
    }

    nsCOMPtr<nsIAtom> name, prefix;
    PRInt32 namespaceID;
    aNode.mContent->GetAttrNameAt(aNode.mIndex, &namespaceID,
                                  getter_AddRefs(name),
                                  getter_AddRefs(prefix));


    if (prefix) {
        prefix->ToString(aName);
        aName.Append(PRUnichar(':'));
    }
    else {
        aName.Truncate();
    }

    const char* attrName;
    name->GetUTF8String(&attrName);
    AppendUTF8toUTF16(attrName, aName);

    // Check for html
    if (aNode.mContent->IsContentOfType(nsIContent::eHTML) &&
        aNode.mContent->GetNodeInfo()->NamespaceEquals(kNameSpaceID_None)) {
        ToUpperCase(aName);
    }
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
    nsIContent* content = aElement->GetChildAt(0);
    PRInt32 i = 0;
    while (content) {
        if (content->IsContentOfType(nsIContent::eELEMENT)) {
            getTextContent(content, aResult);
        }
        else if (content->IsContentOfType(nsIContent::eTEXT)) {
            nsCOMPtr<nsITextContent> textContent = do_QueryInterface(content);
            textContent->AppendTextTo(aResult);
        }
        content = aElement->GetChildAt(++i);
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

    if (aNode.isDocument()) {
        nsIContent* content = aNode.mDocument->GetRootContent();
        if (content) {
            getTextContent(content, aResult);
        }
        return;
    }

    if (aNode.mContent->IsContentOfType(nsIContent::eELEMENT)) {
        getTextContent(aNode.mContent, aResult);
        return;
    }

    if (aNode.mContent->IsContentOfType(nsIContent::eTEXT)) {
        nsCOMPtr<nsITextContent> textContent = do_QueryInterface(aNode.mContent);
        textContent->AppendTextTo(aResult);
        return;
    }

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aNode.mContent);
#ifdef DEBUG
    PRUint16 nodeType;
    node->GetNodeType(&nodeType);
    NS_ASSERTION(nodeType == nsIDOMNode::PROCESSING_INSTRUCTION_NODE,
                 "Unexpected nodetype");
#endif
    nsAutoString result;
    node->GetNodeValue(result);
    aResult.Append(result);
}

/* static */
PRBool
txXPathNodeUtils::isWhitespace(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.isContent(), "Wrong type!");

    nsCOMPtr<nsITextContent> textCont = do_QueryInterface(aNode.mContent);
    if (!textCont) {
        return PR_TRUE;
    }
    PRBool onlyWhitespace;
    textCont->IsOnlyWhitespace(&onlyWhitespace);
    return onlyWhitespace;
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

        // XXX not all attributes support nsIAttribute
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
nsIContent*
txXPathNativeNode::getContent(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.isContent(),
                 "Only call getContent on nsIContent wrappers!");
    return aNode.mContent;
}

/* static */
nsIDocument*
txXPathNativeNode::getDocument(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.isDocument(),
                 "Only call getDocument on nsIDocument wrappers!");
    return aNode.mDocument;
}
