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
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is
 * Jonas Sicking.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * Jonas Sicking. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <jonas@sicking.cc>
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

#ifndef TRANSFRMX_TXKEY_H
#define TRANSFRMX_TXKEY_H

#include "pldhash.h"
#include "dom.h"
#include "XMLUtils.h"

class txKeyValueHashKey
{
public:
    txKeyValueHashKey(const txExpandedName& aKeyName,
                      Document* aDocument,
                      const nsAString& aKeyValue)
        : mKeyName(aKeyName),
          mDocument(aDocument),
          mKeyValue(aKeyValue)
    {
    }
    PRBool Equals(const txKeyValueHashKey& aKeyValueHashKey) const
    {
        return mKeyName == aKeyValueHashKey.mKeyName &&
               mDocument == aKeyValueHashKey.mDocument &&
               mKeyValue.Equals(aKeyValueHashKey.mKeyValue);
    }
    PLDHashNumber GetHash() const
    {
        return mKeyName.mNamespaceID ^
               NS_PTR_TO_INT32(mKeyName.mLocalName) ^
               NS_PTR_TO_INT32(mDocument) ^
               HashString(mKeyValue);
    }

    txExpandedName mKeyName;
    Document* mDocument;
    nsString mKeyValue;
};

struct txKeyValueHashEntry : public PLDHashEntryHdr
{
    txKeyValueHashEntry(const txKeyValueHashKey& aKey) : mKey(aKey)
    {
    }
    
    txKeyValueHashKey mKey;
    NodeSet mNodeSet;
};

extern PLDHashTableOps gTxKeyValueHashOps;

class txIndexedKeyHashKey
{
public:
    txIndexedKeyHashKey(txExpandedName aKeyName,
                        Document* aDocument)
        : mKeyName(aKeyName),
          mDocument(aDocument)
    {
    }
    PRBool Equals(const txIndexedKeyHashKey& aIndexedKeyHashKey) const
    {
        return mKeyName == aIndexedKeyHashKey.mKeyName &&
               mDocument == aIndexedKeyHashKey.mDocument;
    }
    PLDHashNumber GetHash() const
    {
        return mKeyName.mNamespaceID ^
               NS_PTR_TO_INT32(mKeyName.mLocalName) ^
               NS_PTR_TO_INT32(mDocument);
    }

    txExpandedName mKeyName;
    Document* mDocument;
};

struct txIndexedKeyHashEntry : public PLDHashEntryHdr
{
    txIndexedKeyHashEntry(const txIndexedKeyHashKey& aKey) : mKey(aKey),
                                                             mIndexed(0)
    {
    }
    
    txIndexedKeyHashKey mKey;
    PRBool mIndexed;
};

extern PLDHashTableOps gTxIndexedKeyHashOps;

/**
 * Class representing an <xsl:key>. Or in the case where several <xsl:key>s
 * have the same name one object represents all <xsl:key>s with that name
 */
class txXSLKey : public TxObject {
    
public:
    ~txXSLKey();
    
    /**
     * Adds a match/use pair. Returns MB_FALSE if matchString or useString
     * can't be parsed.
     * @param aMatch  match-pattern
     * @param aUse    use-expression
     * @return MB_FALSE if an error occured, MB_TRUE otherwise
     */
    MBool addKey(txPattern* aMatch, Expr* aUse);

    /**
     * Indexes a document and adds it to the hash of key values
     * @param aDocument     Document to index and add
     * @param aKeyName      Name of this key
     * @param aKeyValueHash Hash to add values to
     * @param aEs           txExecutionState to use for XPath evaluation
     */
    nsresult indexDocument(Document* aDocument,
                           const txExpandedName& aKeyName,
                           PLDHashTable* aKeyValueHash, txExecutionState* aEs);

private:
    /**
     * Recursively searches a node, its attributes and its subtree for
     * nodes matching any of the keys match-patterns.
     * @param aNode         Node to search
     * @param aKey          Key to use when adding into the hash
     * @param aKeyValueHash Hash to add values to
     * @param aEs           txExecutionState to use for XPath evaluation
     */
    nsresult indexTree(Node* aNode, txKeyValueHashKey* aKey,
                       PLDHashTable* aKeyValueHash, txExecutionState* aEs);

    /**
     * Tests one node if it matches any of the keys match-patterns. If
     * the node matches its values are added to the index.
     * @param aNode         Node to test
     * @param aKey          Key to use when adding into the hash
     * @param aKeyValueHash Hash to add values to
     * @param aEs           txExecutionState to use for XPath evaluation
     */
    nsresult testNode(Node* aNode, txKeyValueHashKey* aKey,
                      PLDHashTable* aKeyValueHash, txExecutionState* aEs);

    /**
     * represents one match/use pair
     */
    struct Key {
        txPattern* matchPattern;
        Expr* useExpr;
    };

    /**
     * List of all match/use pairs. The items as |Key|s
     */
    List mKeys;
};

#endif //TRANSFRMX_TXKEY_H
