/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 *
 */

#include "Expr.h"
#include "txAtoms.h"

txNameTest::txNameTest(String& aName, Node::NodeType aNodeType)
    :mNodeType(aNodeType)
{
    int idx = aName.indexOf(':');
    if (idx >= 0) {
        aName.subString(0, idx, mPrefix);
        String localName;
        aName.subString(idx+1, localName);
        mLocalName = TX_GET_ATOM(localName);
    }
    else {
        mLocalName = TX_GET_ATOM(aName);
    }
}

txNameTest::~txNameTest()
{
    TX_IF_RELEASE_ATOM(mLocalName);
}

/*
 * Determines whether this txNodeTest matches the given node
 */
MBool txNameTest::matches(Node* aNode, ContextState* aCs)
{
    if (!aNode || aNode->getNodeType() != mNodeType)
        return MB_FALSE;

    // Totally wild?
    if (mLocalName == txXPathAtoms::_asterix && mPrefix.isEmpty())
        return MB_TRUE;

    // Compare namespaces
    if (mPrefix.isEmpty()) {
        if (aNode->getNamespaceID() != kNameSpaceID_None)
            return MB_FALSE;
    }
    else {
        String nsURI;
        aCs->getNameSpaceURIFromPrefix(mPrefix, nsURI);
        if(!aNode->getNamespaceURI().isEqual(nsURI))
            return MB_FALSE;
    }

    // Name wild?
    if (mLocalName == txXPathAtoms::_asterix)
        return MB_TRUE;

    // Compare local-names
    txAtom* localName;
    aNode->getLocalName(&localName);
    MBool result = localName == mLocalName;
    TX_IF_RELEASE_ATOM(localName);

    return result;
}

/*
 * Returns the default priority of this txNodeTest
 */
double txNameTest::getDefaultPriority()
{
    if (mLocalName == txXPathAtoms::_asterix) {
        if (mPrefix.isEmpty())
            return -0.5;
        return -0.25;
    }
    return 0;
}

/*
 * Returns the String representation of this txNodeTest.
 * @param aDest the String to use when creating the string representation.
 *              The string representation will be appended to the string.
 */
void txNameTest::toString(String& aDest)
{
    if (!mPrefix.isEmpty()) {
        aDest.append(mPrefix);
        aDest.append(':');
    }
    String localName;
    TX_GET_ATOM_STRING(mLocalName, localName);
    aDest.append(localName);
}
