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
 * The Initial Developer of the Original Code is Jonas Sicking.
 * Portions created by Jonas Sicking are Copyright (C) 2001, Jonas Sicking.
 * All rights reserved.
 *
 * Contributor(s):
 * Jonas Sicking, sicking@bigfoot.com
 *   -- original author.
 */

#include "txExecutionState.h"
#include "txAtoms.h"
#include "txSingleNodeContext.h"
#include "XMLDOMUtils.h"
#include "XSLTFunctions.h"
#include "nsReadableUtils.h"
#include "txKey.h"
#include "txXSLTPatterns.h"

/*
 * txKeyFunctionCall
 * A representation of the XSLT additional function: key()
 */

/*
 * Creates a new key function call
 */
txKeyFunctionCall::txKeyFunctionCall(txNamespaceMap& aMappings)
    : mMappings(aMappings)
{
}

/*
 * Evaluates a key() xslt-function call. First argument is name of key
 * to use, second argument is value to look up.
 * @param aContext the context node for evaluation of this Expr
 * @param aCs      the ContextState containing the stack information needed
 *                 for evaluation
 * @return the result of the evaluation
 */
ExprResult* txKeyFunctionCall::evaluate(txIEvalContext* aContext)
{
    if (!aContext || !requireParams(2, 2, aContext))
        return new StringResult("error");

    NodeSet* res = new NodeSet;
    if (!res) {
        // ErrorReport: out of memory
        return 0;
    }

    txListIterator iter(&params);
    String keyQName;
    evaluateToString((Expr*)iter.next(), aContext, keyQName);

    txExpandedName keyName;
    txXSLKey* key = 0;
    nsresult rv = keyName.init(keyQName, mMappings, MB_FALSE);

    Expr* param = (Expr*)iter.next();
    ExprResult* exprResult = param->evaluate(aContext);
    if (!exprResult)
        return res;

    Document* contextDoc;
    Node* contextNode = aContext->getContextNode();
    if (contextNode->getNodeType() == Node::DOCUMENT_NODE)
        contextDoc = (Document*)contextNode;
    else
        contextDoc = contextNode->getOwnerDocument();

    if (exprResult->getResultType() == ExprResult::NODESET) {
        NodeSet* nodeSet = (NodeSet*) exprResult;
        int i;
        for (i = 0; i < nodeSet->size(); ++i) {
            String val;
            XMLDOMUtils::getNodeValue(nodeSet->get(i), val);
            NodeSet* nodes = 0;
            //rv = mProcessorState->getKeyValue(keyName, contextDoc, val,
            //                                  i == 0, &nodes);
            if (NS_FAILED(rv)) {
                delete res;
                delete exprResult;
                return new StringResult("error");
            }
            if (nodes) {
                res->add(nodes);
            }
        }
    }
    else {
        String val;
        exprResult->stringValue(val);
        NodeSet* nodes = 0;
        //rv = mProcessorState->getKeyValue(keyName, contextDoc, val,
        //                                  PR_TRUE, &nodes);
        if (NS_FAILED(rv)) {
            delete res;
            delete exprResult;
            return new StringResult("error");
        }
        if (nodes) {
            res->append(nodes);
        }
    }
    delete exprResult;
    return res;
}

nsresult txKeyFunctionCall::getNameAtom(txAtom** aAtom)
{
    *aAtom = txXSLTAtoms::key;
    TX_ADDREF_ATOM(*aAtom);
    return NS_OK;
}

/**
 * Hash functions
 */

PR_STATIC_CALLBACK(const void *)
txKeyValueHashGetKey(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    txKeyValueHashEntry *e =
        NS_STATIC_CAST(txKeyValueHashEntry *, entry);
    return &e->mKey;
}

PR_STATIC_CALLBACK(PLDHashNumber)
txKeyValueHashHashKey(PLDHashTable *table, const void *key)
{
    const txKeyValueHashKey* valueKey =
        NS_STATIC_CAST(const txKeyValueHashKey *, key);
    return valueKey->GetHash();
}

PR_STATIC_CALLBACK(PRBool)
txKeyValueHashMatchEntry(PLDHashTable *table,
                         const PLDHashEntryHdr *entry,
                         const void *key)
{
    const txKeyValueHashEntry *e =
        NS_STATIC_CAST(const txKeyValueHashEntry *, entry);
    const txKeyValueHashKey* hashKey =
        NS_STATIC_CAST(const txKeyValueHashKey *, key);

    return hashKey->Equals(e->mKey);
}

PR_STATIC_CALLBACK(void)
txKeyValueHashClearEntry(PLDHashTable *table,
                         PLDHashEntryHdr *entry)
{
    txKeyValueHashEntry *e =
        NS_STATIC_CAST(txKeyValueHashEntry *, entry);

    // Clear the entry using the dtor
    e->~txKeyValueHashEntry();
}

PR_STATIC_CALLBACK(void)
txKeyValueHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                        const void *key)
{
    const txKeyValueHashKey* hashKey =
        NS_STATIC_CAST(const txKeyValueHashKey *, key);

    // Inititlize the entry with placement new
    new (entry) txKeyValueHashEntry(*hashKey);
}

PLDHashTableOps gTxKeyValueHashOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    txKeyValueHashGetKey,
    txKeyValueHashHashKey,
    txKeyValueHashMatchEntry,
    PL_DHashMoveEntryStub,
    txKeyValueHashClearEntry,
    PL_DHashFinalizeStub,
    txKeyValueHashInitEntry
};


PR_STATIC_CALLBACK(const void *)
txIndexedKeyHashGetKey(PLDHashTable *table, PLDHashEntryHdr *entry)
{
    txIndexedKeyHashEntry *e =
        NS_STATIC_CAST(txIndexedKeyHashEntry *, entry);
    return &e->mKey;
}

PR_STATIC_CALLBACK(PLDHashNumber)
txIndexedKeyHashHashKey(PLDHashTable *table, const void *key)
{
    const txIndexedKeyHashKey* indexKey =
        NS_STATIC_CAST(const txIndexedKeyHashKey *, key);
    return indexKey->GetHash();
}

PR_STATIC_CALLBACK(PRBool)
txIndexedKeyHashMatchEntry(PLDHashTable *table,
                           const PLDHashEntryHdr *entry,
                           const void *key)
{
    const txIndexedKeyHashEntry *e =
        NS_STATIC_CAST(const txIndexedKeyHashEntry *, entry);
    const txIndexedKeyHashKey *hashKey =
        NS_STATIC_CAST(const txIndexedKeyHashKey *, key);

    return hashKey->Equals(e->mKey);
}

PR_STATIC_CALLBACK(void)
txIndexedKeyHashClearEntry(PLDHashTable *table,
                           PLDHashEntryHdr *entry)
{
    txIndexedKeyHashEntry *e =
        NS_STATIC_CAST(txIndexedKeyHashEntry *, entry);

    // Clear the entry using the dtor
    e->~txIndexedKeyHashEntry();
}

PR_STATIC_CALLBACK(void)
txIndexedKeyHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                          const void *key)
{
    const txIndexedKeyHashKey* hashKey =
        NS_STATIC_CAST(const txIndexedKeyHashKey *, key);

    // Inititlize the entry with placement new
    new (entry) txIndexedKeyHashEntry(*hashKey);
}

PLDHashTableOps gTxIndexedKeyHashOps =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    txIndexedKeyHashGetKey,
    txIndexedKeyHashHashKey,
    txIndexedKeyHashMatchEntry,
    PL_DHashMoveEntryStub,
    txIndexedKeyHashClearEntry,
    PL_DHashFinalizeStub,
    txIndexedKeyHashInitEntry
};



/*
 * Class representing an <xsl:key>. Or in the case where several <xsl:key>s
 * have the same name one object represents all <xsl:key>s with that name
 */

txXSLKey::~txXSLKey()
{
    txListIterator iter(&mKeys);
    Key* key;
    while ((key = (Key*)iter.next())) {
        delete key->matchPattern;
        delete key->useExpr;
        delete key;
    }
}

/*
 * Adds a match/use pair. Returns MB_FALSE if matchString or useString
 * can't be parsed.
 * @param aMatch  match-pattern
 * @param aUse    use-expression
 * @return MB_FALSE if an error occured, MB_TRUE otherwise
 */
MBool txXSLKey::addKey(txPattern* aMatch, Expr* aUse)
{
    if (!aMatch || !aUse)
        return MB_FALSE;

    Key* key = new Key;
    if (!key)
        return MB_FALSE;

    key->matchPattern = aMatch;
    key->useExpr = aUse;
    mKeys.add(key);
    return MB_TRUE;
}

/**
 * Indexes a document and adds it to the hash of key values
 * @param aDocument     Document to index and add
 * @param aKeyName      Name of this key
 * @param aKeyValueHash Hash to add values to
 * @param aEs           txExecutionState to use for XPath evaluation
 */
nsresult txXSLKey::indexDocument(Document* aDocument,
                                 const txExpandedName& aKeyName,
                                 PLDHashTable* aKeyValueHash,
                                 txExecutionState* aEs)
{
    txKeyValueHashKey key(aKeyName, aDocument, NS_LITERAL_STRING(""));
    return indexTree(aDocument, &key, aKeyValueHash, aEs);
}

/**
 * Recursively searches a node, its attributes and its subtree for
 * nodes matching any of the keys match-patterns.
 * @param aNode         Node to search
 * @param aKey          Key to use when adding into the hash
 * @param aKeyValueHash Hash to add values to
 * @param aEs           txExecutionState to use for XPath evaluation
 */
nsresult txXSLKey::indexTree(Node* aNode, txKeyValueHashKey* aKey,
                             PLDHashTable* aKeyValueHash, txExecutionState* aEs)
{
    nsresult rv = testNode(aNode, aKey, aKeyValueHash, aEs);
    NS_ENSURE_SUCCESS(rv, rv);

    // check if the nodes attributes matches
    NamedNodeMap* attrs = aNode->getAttributes();
    if (attrs) {
        for (PRUint32 i=0; i<attrs->getLength(); i++) {
            rv = testNode(attrs->item(i), aKey, aKeyValueHash, aEs);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    Node* child = aNode->getFirstChild();
    while (child) {
        rv = indexTree(child, aKey, aKeyValueHash, aEs);
        NS_ENSURE_SUCCESS(rv, rv);

        child = child->getNextSibling();
    }
    
    return NS_OK;
}

/**
 * Tests one node if it matches any of the keys match-patterns. If
 * the node matches its values are added to the index.
 * @param aNode         Node to test
 * @param aKey          Key to use when adding into the hash
 * @param aKeyValueHash Hash to add values to
 * @param aEs           txExecutionState to use for XPath evaluation
 */
nsresult txXSLKey::testNode(Node* aNode, txKeyValueHashKey* aKey,
                            PLDHashTable* aKeyValueHash, txExecutionState* aEs)
{
    String val;
    txListIterator iter(&mKeys);
    while (iter.hasNext())
    {
        Key* key=(Key*)iter.next();
        if (key->matchPattern->matches(aNode, aEs)) {
            txSingleNodeContext evalContext(aNode, aEs);
            aEs->pushEvalContext(&evalContext);
            ExprResult* exprResult = key->useExpr->evaluate(&evalContext);
            aEs->popEvalContext();
            if (exprResult->getResultType() == ExprResult::NODESET) {
                NodeSet* res = (NodeSet*)exprResult;
                for (int i=0; i<res->size(); i++) {
                    val.clear();
                    XMLDOMUtils::getNodeValue(res->get(i), val);

                    aKey->mKeyValue.Assign(val);
                    txKeyValueHashEntry* entry = 
                        NS_STATIC_CAST(txKeyValueHashEntry *,
                                       PL_DHashTableOperate(aKeyValueHash,
                                                            aKey,
                                                            PL_DHASH_ADD));
                    NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

                    if (entry->mNodeSet.isEmpty() ||
                        entry->mNodeSet.get(entry->mNodeSet.size()-1) !=
                                            aNode) {
                        entry->mNodeSet.append(aNode);
                    }
                }
            }
            else {
                exprResult->stringValue(val);

                aKey->mKeyValue.Assign(val);
                txKeyValueHashEntry* entry = 
                    NS_STATIC_CAST(txKeyValueHashEntry *,
                                   PL_DHashTableOperate(aKeyValueHash,
                                                        aKey,
                                                        PL_DHASH_ADD));
                NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

                if (entry->mNodeSet.isEmpty() ||
                    entry->mNodeSet.get(entry->mNodeSet.size()-1) != aNode) {
                    entry->mNodeSet.append(aNode);
                }
            }
            delete exprResult;
        }
    }
    
    return NS_OK;
}
