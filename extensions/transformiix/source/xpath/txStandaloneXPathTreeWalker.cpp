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
 *   Axel Hecht <axel@pike.org>
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
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "XMLUtils.h"

txXPathTreeWalker::txXPathTreeWalker(const txXPathNode& aNode)
    : mPosition(aNode), mLevel(0)
{
}

txXPathTreeWalker::~txXPathTreeWalker()
{
}

#define INNER mPosition.mInner

PRBool
txXPathTreeWalker::moveToElementById(const nsAString& aID)
{
    Document* document;
    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        document = NS_STATIC_CAST(Document*, INNER);
    }
    else {
        document = INNER->ownerDocument;
    }

    Element* element =
        document->getElementById(aID);
    if (!element) {
        return PR_FALSE;
    }

    INNER = element;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstAttribute()
{
    if (INNER->nodeType != Node::ELEMENT_NODE) {
        return PR_FALSE;
    }

    Element* element = NS_STATIC_CAST(Element*, INNER);
    NamedNodeMap* attrs = element->getAttributes();
    NodeListDefinition::ListItem* item = attrs->firstItem;
    // skip XMLNS attributes
    while (item && item->node->getNamespaceID() == kNameSpaceID_XMLNS) {
        item = item->next;
    }
    if (!item) {
        return PR_FALSE;
    }

    INNER = NS_STATIC_CAST(NodeDefinition*, item->node);
    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstChild()
{
    if (!INNER->firstChild) {
        return PR_FALSE;
    }

    INNER = INNER->firstChild;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstDescendant()
{
    mLevel = 0;
    return moveToFirstChild();
}

PRBool
txXPathTreeWalker::moveToFirstFollowing()
{
    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        return PR_FALSE;
    }

    NodeDefinition* node = INNER;
    if (INNER->nodeType == Node::ATTRIBUTE_NODE) {
        node = NS_STATIC_CAST(NodeDefinition*, INNER->getXPathParent());
        if (node->firstChild) {
            INNER = node->firstChild;
            return PR_TRUE;
        }
    }

    // Now walk up the parent chain to find the first ancestor that has a
    // following sibling.
    while (node) {
        // First check if content has any following siblings
        if (node->nextSibling) {
            INNER = node->nextSibling;
            return PR_TRUE;
        }
        node = node->parentNode;
    }

    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToFirstFollowingSibling()
{
    if (INNER->nodeType == Node::DOCUMENT_NODE ||
        INNER->nodeType == Node::ATTRIBUTE_NODE ||
        !INNER->nextSibling) {
        return PR_FALSE;
    }

    INNER = INNER->nextSibling;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToFirstPreceding()
{
    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        return PR_FALSE;
    }

    NodeDefinition* node = INNER;
    if (INNER->nodeType == Node::ATTRIBUTE_NODE) {
        node = NS_STATIC_CAST(NodeDefinition*, node->getXPathParent());
    }

    mLevel = 0;

    return moveToPreceding(node);
}

PRBool
txXPathTreeWalker::moveToFirstPrecedingInDocOrder()
{
    return moveToNextPrecedingInDocOrder();
}

PRBool
txXPathTreeWalker::moveToFirstPrecedingSibling()
{
    if (INNER->nodeType == Node::DOCUMENT_NODE ||
        INNER->nodeType == Node::ATTRIBUTE_NODE ||
        !INNER->previousSibling) {
        return PR_FALSE;
    }

    INNER = INNER->previousSibling;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextDescendant()
{
    NS_ASSERTION(INNER->nodeType != Node::DOCUMENT_NODE ||
                 INNER->nodeType != Node::ATTRIBUTE_NODE,
                 "Wrong type, maybe you called moveToNextDescendant without "
                 "moveToFirstDescendant first?");

    if (moveToFirstChild()) {
        ++mLevel;
        return PR_TRUE;
    }
    NodeDefinition* node = INNER;
    while (!node->nextSibling && mLevel) {
        --mLevel;
        if (!node->parentNode) {
            return PR_FALSE;
        }
        node = node->parentNode;
    }
    if (node->nextSibling) {
        INNER = node->nextSibling;
        return PR_TRUE;
    }
    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToNextFollowing()
{
    NS_ASSERTION(INNER->nodeType != Node::DOCUMENT_NODE ||
                 INNER->nodeType != Node::ATTRIBUTE_NODE,
                 "Wrong type, maybe you called moveToNextFollowing without "
                 "moveToFirstFollowing first?");

    // try descendants, as this is not starting with self
    if (moveToFirstChild()) {
        return PR_TRUE;
    }
    // find the first nextSibling of an ancestor-or-self
    NodeDefinition* node = INNER;
    do {
        if (node->nextSibling) {
            INNER = node->nextSibling;
            return PR_TRUE;
        }
        node = node->parentNode;
    } while (node && node->nodeType != Node::DOCUMENT_NODE);

    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToNextPreceding()
{
    NS_ASSERTION(INNER->nodeType != Node::DOCUMENT_NODE ||
                 INNER->nodeType != Node::ATTRIBUTE_NODE,
                 "Wrong type, maybe you called moveToNextPreceding without "
                 "moveToFirstPreceding first?");

    NodeDefinition* node = INNER;
    return moveToPreceding(node);
}

PRBool
txXPathTreeWalker::moveToPreceding(NodeDefinition* aNode)
{
    while (aNode) {
        if (aNode->previousSibling) {
            aNode = aNode->previousSibling;
            while (aNode->lastChild) {
                aNode = aNode->lastChild;
                ++mLevel;
            }
            INNER = aNode;
            return PR_TRUE;
        }
        aNode = aNode->parentNode;
        if (mLevel) {
            --mLevel;
            INNER = aNode;
            return PR_TRUE;
        }
    }        

    return PR_FALSE;
}

PRBool
txXPathTreeWalker::moveToNextPrecedingInDocOrder()
{
    // Almost the same as moveToNextPreceding, just include ancestors as well.
    NS_ASSERTION(INNER->nodeType != Node::ATTRIBUTE_NODE, 
                 "Attributes are excluded");
    NodeDefinition* node = INNER;
    if (!node->previousSibling) {
        return moveToParent();
    }
    node = node->previousSibling;
    while (node->lastChild) {
        node = node->lastChild;
    }
    INNER = node;
    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToNextSibling()
{
    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        return PR_FALSE;
    }

    if (INNER->nodeType != Node::ATTRIBUTE_NODE) {
        if (INNER->nextSibling) {
            INNER = INNER->nextSibling;
            return PR_TRUE;
        }
        return PR_FALSE;
    }

    Node* element = INNER->getXPathParent();
    NamedNodeMap* attrs = element->getAttributes();
    // find the ListItem for the current Attr
    NodeListDefinition::ListItem* item = attrs->firstItem;
    while (item && item->node != INNER) {
        item = item->next;
    }
    NS_ASSERTION(item, "Attr not attribute of it's owner?");
    // next item
    item = item->next;
    // skip XMLNS attributes
    while (item && item->node->getNamespaceID() == kNameSpaceID_XMLNS) {
        item = item->next;
    }
    if (!item) {
        return PR_FALSE;
    }

    INNER = NS_STATIC_CAST(NodeDefinition*, item->node);
    return PR_TRUE;

}

PRBool
txXPathTreeWalker::moveToPreviousSibling()
{
    NS_ASSERTION(INNER->nodeType != Node::ATTRIBUTE_NODE,
                 "reversed attribute axis does not exist");
    if (INNER->nodeType == Node::DOCUMENT_NODE ||
        !INNER->previousSibling) {
        return PR_FALSE;
    }

    INNER = INNER->previousSibling;

    return PR_TRUE;
}

PRBool
txXPathTreeWalker::moveToParent()
{
    if (INNER->nodeType == Node::ATTRIBUTE_NODE) {
        INNER = NS_STATIC_CAST(NodeDefinition*, INNER->getXPathParent());
        return PR_TRUE;
    }

    if (INNER->nodeType == Node::DOCUMENT_NODE) {
        return PR_FALSE;
    }

    NS_ASSERTION(INNER->parentNode, "orphaned node shouldn't happen");

    INNER = INNER->parentNode;

    return PR_TRUE;
}

/* static */
void
txXPathTreeWalker::setTo(txXPathNode& aNode, const txXPathNode& aOtherNode)
{
    aNode.mInner = aOtherNode.mInner;
}

txXPathNode::txXPathNode(const txXPathNode& aNode)
    : mInner(aNode.mInner)
{
}

txXPathNode::~txXPathNode()
{
}

PRBool
txXPathNode::operator==(const txXPathNode& aNode) const
{
    return (mInner == aNode.mInner);
}

/* static */
PRBool
txXPathNodeUtils::getAttr(const txXPathNode& aNode, nsIAtom* aLocalName,
                          PRInt32 aNSID, nsAString& aValue)
{
    if (aNode.mInner->getNodeType() != Node::ELEMENT_NODE) {
        return PR_FALSE;
    }

    Element* elem = NS_STATIC_CAST(Element*, aNode.mInner);
    return elem->getAttr(aLocalName, aNSID, aValue);
}

/* static */
PRBool
txXPathNodeUtils::getLocalName(const txXPathNode& aNode,
                               nsIAtom** aLocalName)
{
    return aNode.mInner->getLocalName(aLocalName);
}

/* static */
PRBool
txXPathNodeUtils::getLocalName(const txXPathNode& aNode, nsAString& aLocalName)
{
    nsCOMPtr<nsIAtom> localName;
    PRBool hasName = aNode.mInner->getLocalName(getter_AddRefs(localName));
    if (hasName && localName) {
        localName->ToString(aLocalName);
        return PR_TRUE;
    }
    return PR_FALSE;
}

/* static */
PRBool
txXPathNodeUtils::getNodeName(const txXPathNode& aNode, nsAString& aName)
{
    nsresult rv = aNode.mInner->getNodeName(aName);

    return NS_SUCCEEDED(rv);
}

/* static */
PRInt32
txXPathNodeUtils::getNamespaceID(const txXPathNode& aNode)
{
    return aNode.mInner->getNamespaceID();
}

/* static */
void
txXPathNodeUtils::getNamespaceURI(const txXPathNode& aNode, nsAString& aURI)
{
    aNode.mInner->getNamespaceURI(aURI);
}

/* static */
PRUint16
txXPathNodeUtils::getNodeType(const txXPathNode& aNode)
{
    return aNode.mInner->getNodeType();
}

/*
PRBool
txXPathNodeUtils::isNodeOfType(const txXPathNode& aNode, PRUint16 aType)
{
    PRBool isType = PR_FALSE;
    switch (aNode.mInner->nodeType) {
        case Node::ELEMENT_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::eELEMENT);
            break;
        case Node::ATTRIBUTE_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::eATTRIBUTE);
            break;
        case Node::TEXT_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::eTEXT);
            break;
        case Node::PROCESSING_INSTRUCTION_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::ePI);
            break;
        case Node::COMMENT_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::eCOMMENT);
            break;
        case Node::DOCUMENT_NODE:
            isType = !(aFlags & ~txXPathNodeFilter::eDOCUMENT);
            break;
        case Node::DOCUMENT_FRAGMENT_NODE:
        default:
            NS_NOTREACHED("unexpected content in isNodeOfType");
    }
    return isType;
}
*/

/* static */
void
txXPathNodeUtils::getNodeValue(const txXPathNode& aNode, nsAString& aResult)
{
    unsigned short type = aNode.mInner->getNodeType();
    if (type == Node::ATTRIBUTE_NODE ||
        type == Node::COMMENT_NODE ||
        type == Node::PROCESSING_INSTRUCTION_NODE ||
        type == Node::TEXT_NODE) {
        nsAutoString result;
        aNode.mInner->getNodeValue(result);
        aResult.Append(result);

        return;
    }

    NS_ASSERTION(type == Node::ELEMENT_NODE || type == Node::DOCUMENT_NODE,
                 "Element or Document expected");

    txXPathTreeWalker walker(aNode.mInner);
    if (!walker.moveToFirstDescendant()) {
        return;
    }
    nsAutoString result;
    do {
        if (walker.getCurrentPosition().mInner->nodeType ==
            Node::TEXT_NODE) {
            walker.getCurrentPosition().mInner->getNodeValue(result);
            aResult.Append(result);
        }
    } while (walker.moveToNextDescendant());
}

/* static */
PRBool
txXPathNodeUtils::isWhitespace(const txXPathNode& aNode)
{
    NS_ASSERTION(aNode.mInner->nodeType == Node::TEXT_NODE, "Wrong type!");

    
    return XMLUtils::isWhitespace(aNode.mInner->nodeValue);
}

/* static */
txXPathNode*
txXPathNodeUtils::getOwnerDocument(const txXPathNode& aNode)
{
    if (aNode.mInner->nodeType == Node::DOCUMENT_NODE) {
        return new txXPathNode(aNode);
    }

    return new txXPathNode(aNode.mInner->ownerDocument);
}

/* static */
txXPathNode*
txXPathNodeUtils::getElementById(const txXPathNode& aDocument,
                                 const nsAString& aID)
{
    if (aDocument.mInner->nodeType != Node::DOCUMENT_NODE) {
        return nsnull;
    }

    Document* doc = NS_STATIC_CAST(Document*, aDocument.mInner);
    Element* element = doc->getElementById(aID);
    if (!element) {
        return nsnull;
    }

    return txXPathNativeNode::createXPathNode(element);
}

#ifndef HAVE_64BIT_OS
#define kFmtSize 13
#define kFmtSizeAttr 24
const char gPrintfFmt[] = "id0x%08p";
#else
#define kFmtSize 21
#define kFmtSizeAttr 32
const char gPrintfFmt[] = "id0x%016p";
#endif

/* static */
nsresult
txXPathNodeUtils::getXSLTId(const txXPathNode& aNode,
                            nsAString& aResult)
{
    CopyASCIItoUCS2(nsPrintfCString(kFmtSize, gPrintfFmt, aNode.mInner),
                    aResult);

    return NS_OK;
}

/* static */
void
txXPathNodeUtils::getBaseURI(const txXPathNode& aNode, nsAString& aURI)
{
    aNode.mInner->getBaseURI(aURI);
}

/* static */
PRIntn
txXPathNodeUtils::comparePosition(const txXPathNode& aNode,
                                  const txXPathNode& aOtherNode)
{
    // First check for equal nodes.
    if (aNode == aOtherNode) {
        return 0;
    }
    return aNode.mInner->compareDocumentPosition(aOtherNode.mInner);
}

/* static */
txXPathNode*
txXPathNativeNode::createXPathNode(Node* aNode)
{
    if (aNode != nsnull) {
        return new txXPathNode(NS_STATIC_CAST(NodeDefinition*, aNode));
    }
    return nsnull;
}

/* static */
nsresult
txXPathNativeNode::getElement(const txXPathNode& aNode, Element** aResult)
{
    if (aNode.mInner->getNodeType() != Node::ELEMENT_NODE) {
        return NS_ERROR_FAILURE;
    }

    *aResult = NS_STATIC_CAST(Element*, aNode.mInner);

    return NS_OK;

}

/* static */
nsresult
txXPathNativeNode::getDocument(const txXPathNode& aNode, Document** aResult)
{
    if (aNode.mInner->getNodeType() != Node::DOCUMENT_NODE) {
        return NS_ERROR_FAILURE;
    }

    *aResult = NS_STATIC_CAST(Document*, aNode.mInner);

    return NS_OK;
}
