/*
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
 * The Original Code is XSL:P XSLT processor.
 *
 * The Initial Developer of the Original Code is Keith Visco.
 * Portions created by Keith Visco (C) 1999, 2000 Keith Visco.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 * Olivier Gerardin,
 *    -- added document() function definition
 *
 * Jonas Sicking,
 *    -- added txXSLKey class
 *
 */

#ifndef TRANSFRMX_XSLT_FUNCTIONS_H
#define TRANSFRMX_XSLT_FUNCTIONS_H

#include "Expr.h"
#include "Map.h"
#include "NodeSet.h"
#include "txNamespaceMap.h"
#include "XMLUtils.h"

class NamedMap;
class txPattern;
class txStylesheet;
class txKeyValueHashKey;
class txExecutionState;

/**
 * The definition for the XSLT document() function
**/
class DocumentFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new document() function call
    **/
    DocumentFunctionCall(const String& aBaseURI);

    TX_DECL_FUNCTION;

private:
    String mBaseURI;
};

/*
 * The definition for the XSLT key() function
 */
class txKeyFunctionCall : public FunctionCall {

public:

    /*
     * Creates a new key() function call
     */
    txKeyFunctionCall(txNamespaceMap& aMappings);

    TX_DECL_FUNCTION;

private:
    txNamespaceMap mMappings;
};

/*
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

/**
 * The definition for the XSLT format-number() function
**/
class txFormatNumberFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new format-number() function call
    **/
    txFormatNumberFunctionCall(txStylesheet* aStylesheet, txNamespaceMap& aMappings);

    TX_DECL_FUNCTION;

private:
    static const UNICODE_CHAR FORMAT_QUOTE;

    enum FormatParseState {
        Prefix,
        IntDigit,
        IntZero,
        FracZero,
        FracDigit,
        Suffix,
        Finished
    };
    
    txStylesheet* mStylesheet;
    txNamespaceMap mMappings;
};

/**
 * DecimalFormat
 * A representation of the XSLT element <xsl:decimal-format>
 */
class txDecimalFormat : public TxObject {

public:
    /*
     * Creates a new decimal format and initilizes all properties with
     * default values
     */
    txDecimalFormat();
    MBool isEqual(txDecimalFormat* other);
    
    UNICODE_CHAR    mDecimalSeparator;
    UNICODE_CHAR    mGroupingSeparator;
    String          mInfinity;
    UNICODE_CHAR    mMinusSign;
    String          mNaN;
    UNICODE_CHAR    mPercent;
    UNICODE_CHAR    mPerMille;
    UNICODE_CHAR    mZeroDigit;
    UNICODE_CHAR    mDigit;
    UNICODE_CHAR    mPatternSeparator;
};

/**
 * The definition for the XSLT current() function
**/
class CurrentFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new current() function call
    **/
    CurrentFunctionCall();

    TX_DECL_FUNCTION;
};

/**
 * The definition for the XSLT unparsed-entity-uri() function
**/
class UnparsedEntityUriFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new unparsed-entity-uri() function call
    **/
    UnparsedEntityUriFunctionCall();

    TX_DECL_FUNCTION;

private:
};

/**
 * The definition for the XSLT generate-id() function
**/
class GenerateIdFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new generate-id() function call
    **/
    GenerateIdFunctionCall();

    TX_DECL_FUNCTION;

private:
    static const char printfFmt[];
};

/**
 * The definition for the XSLT system-property() function
**/
class SystemPropertyFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new system-property() function call
     * aNode is the Element in the stylesheet containing the 
     * Expr and is used for namespaceID resolution
    **/
    SystemPropertyFunctionCall(txNamespaceMap& aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    txNamespaceMap mMappings;
};

/**
 * The definition for the XSLT element-available() function
**/
class ElementAvailableFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new element-available() function call
     * aNode is the Element in the stylesheet containing the 
     * Expr and is used for namespaceID resolution
    **/
    ElementAvailableFunctionCall(txNamespaceMap& aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    txNamespaceMap mMappings;
};

/**
 * The definition for the XSLT function-available() function
**/
class FunctionAvailableFunctionCall : public FunctionCall {

public:

    /**
     * Creates a new function-available() function call
    **/
    FunctionAvailableFunctionCall(txNamespaceMap& aMappings);

    TX_DECL_FUNCTION;

private:
    /*
     * resolve namespaceIDs with this node
     */
    txNamespaceMap mMappings;
};

#endif
