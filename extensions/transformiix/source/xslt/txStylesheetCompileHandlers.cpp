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
 * Portions created by the Initial Developer are Copyright (C) 2002
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

#include "txStylesheetCompiler.h"
#include "txStylesheetCompileHandlers.h"
#include "txTokenizer.h"
#include "txInstructions.h"
#include "txAtoms.h"
#include "primitives.h"
#include "txStringUtils.h"
#include "txStylesheet.h"
#include "txToplevelItems.h"

txHandlerTable* gTxIgnoreHandler = 0;
txHandlerTable* gTxRootHandler = 0;
txHandlerTable* gTxTopHandler = 0;
txHandlerTable* gTxTemplateHandler = 0;
txHandlerTable* gTxTextHandler = 0;
txHandlerTable* gTxApplyTemplatesHandler = 0;


txStylesheetAttr*
getStyleAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             PRInt32 aNamespace,
             nsIAtom* aName)
{
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        txStylesheetAttr* attr = aAttributes + i;
        if (attr->mNamespaceID == aNamespace &&
            attr->mLocalName == aName) {

            return attr;
        }
    }
    
    return nsnull;
}


txStylesheetAttr*
getStyleAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             nsIAtom* aName)
{
    return getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_None, aName);
}

#define TX_ENSURE_SUCCESS_OR_FCP(_rv, _state)                               \
    NS_ENSURE_TRUE(NS_SUCCEEDED(_rv) || _state.fcp(), _rv)

#define TX_RETURN_IF_WHITESPACE(_str, _state)                               \
    do {                                                                    \
      if (!_state.mElementContext->mPreserveWhitespace &&                   \
          XMLUtils::isWhitespace(_str)) {                                   \
          return NS_OK;                                                     \
      }                                                                     \
    } while(0)

/**
 * Standard handlers
 */
nsresult
txFnTextIgnore(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnTextError(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(PromiseFlatString(aStr), aState);

    return NS_ERROR_XSLT_PARSE_FAILURE;
}

nsresult
txFnStartElementIgnore(PRInt32 aNamespaceID,
                       nsIAtom* aLocalName,
                       nsIAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnEndElementIgnore(txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnStartElementSetIgnore(PRInt32 aNamespaceID,
                          nsIAtom* aLocalName,
                          nsIAtom* aPrefix,
                          txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount,
                          txStylesheetCompilerState& aState)
{
    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndElementSetIgnore(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnStartElementError(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    return NS_ERROR_XSLT_PARSE_FAILURE;
}

nsresult
txFnEndElementError(txStylesheetCompilerState& aState)
{
    NS_ERROR("txFnEndElementError shouldn't be called"); 
    return NS_ERROR_XSLT_PARSE_FAILURE;
}


/**
 * Root handlers
 */
nsresult
txFnStartStylesheet(PRInt32 aNamespaceID,
                    nsIAtom* aLocalName,
                    nsIAtom* aPrefix,
                    txStylesheetAttr* aAttributes,
                    PRInt32 aAttrCount,
                    txStylesheetCompilerState& aState)
{
    if (!getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::version)) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return aState.pushHandlerTable(gTxTopHandler);
}

nsresult
txFnEndStylesheet(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}


/**
 * Top handlers
 */
nsresult
txFnStartOtherTop(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    if (aNamespaceID == kNameSpaceID_None) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }
    
    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndOtherTop(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}


nsresult
txFnStartTemplate(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushHandlerTable(gTxTemplateHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txStylesheetAttr* attr = 0;

    txExpandedName name;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::name))) {
        rv = aState.parseQName(attr->mValue, name, MB_FALSE);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    txExpandedName mode;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::mode))) {
        rv = aState.parseQName(attr->mValue, mode, MB_FALSE);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    double prio = Double::NaN;
    if ((attr = getStyleAttr(aAttributes, aAttrCount,
                             txXSLTAtoms::priority))) {
        prio = Double::toDouble(attr->mValue);
        if (Double::isNaN(prio) && !aState.fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    txPattern* match = 0;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::match))) {
        rv = aState.parsePattern(attr->mValue, &match);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    txTemplateItem* templ = new txTemplateItem(match, name, mode, prio);
    if (!templ) {
        delete match;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    rv = aState.addToplevelItem(templ);
    if (NS_FAILED(rv)) {
        delete templ;
        return rv;
    }
    
    aState.openInstructionContainer(templ);

    return NS_OK;
}

nsresult
txFnEndTemplate(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txReturn();
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();
    aState.popHandlerTable();

    return NS_OK;
}

nsresult
txFnStartKey(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushHandlerTable(gTxIgnoreHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txStylesheetAttr* attr = 0;
    
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::name);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    txExpandedName name;
    rv = aState.parseQName(attr->mValue, name, MB_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    txPattern* match = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::match);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    rv = aState.parsePattern(attr->mValue, &match);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* use = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::use);
    if (!attr) {
        delete match;
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    rv = aState.parseExpr(attr->mValue, &use);
    if (NS_FAILED(rv)) {
        delete match;
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    rv = aState.mStylesheet->addKey(name, match, use);
    if (NS_FAILED(rv)) {
        delete match;
        delete use;
        return rv;
    }
    
    return NS_OK;
}

nsresult
txFnEndKey(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
}

/**
 * Template Handlers
 */
// LRE
nsresult
txFnStartLRE(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txInstruction* instr = new txStartLREElement(aNamespaceID, aLocalName,
                                                 aPrefix);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txStylesheetAttr* attr = 0;
    
    attr = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_XSLT,
                        txXSLTAtoms::name);
    if (attr) {
        txTokenizer tok(attr->mValue);
        while (tok.hasMoreTokens()) {
            txExpandedName name;
            rv = aState.parseQName(tok.nextToken(), name, MB_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);

            instr = new txInsertAttrSet(name);
            NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

            rv = aState.addInstruction(instr);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        attr = aAttributes + i;
        
        if (attr->mNamespaceID == kNameSpaceID_XSLT) {
            continue;
        }
        
        Expr* avt = 0;
        rv = aState.parseAVT(attr->mValue, &avt);
        NS_ENSURE_SUCCESS(rv, rv);

        instr = new txLREAttribute(attr->mNamespaceID, attr->mLocalName,
                                   attr->mPrefix, avt);
        if (!instr) {
            delete avt;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        
        rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txFnEndLRE(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txEndLREElement;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// "LRE text"
nsresult
txFnText(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(PromiseFlatString(aStr), aState);

    txInstruction* instr = new txText(aStr, MB_FALSE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:apply-templates
nsresult
txFnStartApplyTemplates(PRInt32 aNamespaceID,
                        nsIAtom* aLocalName,
                        nsIAtom* aPrefix,
                        txStylesheetAttr* aAttributes,
                        PRInt32 aAttrCount,
                        txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txStylesheetAttr* attr = 0;

    txExpandedName name;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::name);
    if (attr) {
        rv = aState.parseQName(attr->mValue, name, MB_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    txInstruction* applyTempl = new txApplyTemplates(name);
    NS_ENSURE_TRUE(applyTempl, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(applyTempl);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::select);
    if (attr) {
        rv = aState.parseExpr(attr->mValue, &select);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
        NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

        select = new LocationStep(nt, LocationStep::CHILD_AXIS);
        NS_ENSURE_TRUE(select, NS_ERROR_OUT_OF_MEMORY);
    }

    txPushNewContext* newContextInstr = new txPushNewContext(select);
    NS_ENSURE_TRUE(newContextInstr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(newContextInstr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxApplyTemplatesHandler);
}

nsresult
txFnEndApplyTemplates(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    // txPushNewContext
    nsresult rv = aState.addInstruction((txPushNewContext*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    // txApplyTemplates
    rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:if
nsresult
txFnStartIf(PRInt32 aNamespaceID,
            nsIAtom* aLocalName,
            nsIAtom* aPrefix,
            txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = 0;

    Expr* test = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::test);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    nsresult rv = aState.parseExpr(attr->mValue, &test);
    NS_ENSURE_SUCCESS(rv, rv);

    txConditionalGoto* condGoto = new txConditionalGoto(test);
    if (!condGoto) {
        delete test;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = aState.pushPtr(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndIf(txStylesheetCompilerState& aState)
{
    txConditionalGoto* condGoto = (txConditionalGoto*)aState.popPtr();
    return aState.addGotoTarget(&condGoto->mTarget);
}

// xsl:text
nsresult
txFnStartText(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = 0;

    NS_ASSERTION(!aState.mDOE, "nested d-o-e elements should not happen");

    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::disableOutputEscaping))) {
        if (TX_StringEqualsAtom(attr->mValue, txXSLTAtoms::yes)) {
            aState.mDOE = MB_TRUE;
        }
        else if (!TX_StringEqualsAtom(attr->mValue, txXSLTAtoms::no) &&
                 !aState.fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    return aState.pushHandlerTable(gTxTextHandler);
}

nsresult
txFnEndText(txStylesheetCompilerState& aState)
{
    aState.mDOE = MB_FALSE;
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnTextText(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txText(aStr, aState.mDOE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:value-of
nsresult
txFnStartValueOf(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = 0;

    MBool doe = MB_FALSE;

    if ((attr = getStyleAttr(aAttributes, aAttrCount,
                             txXSLTAtoms::disableOutputEscaping))) {
        if (TX_StringEqualsAtom(attr->mValue, txXSLTAtoms::yes)) {
            doe = MB_TRUE;
        }
        else if (!TX_StringEqualsAtom(attr->mValue, txXSLTAtoms::no) &&
                 !aState.fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    Expr* select = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::select);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    nsresult rv = aState.parseExpr(attr->mValue, &select);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txValueOf(select, doe);
    if (!instr) {
        delete select;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndValueOf(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}


/**
 * Table Datas
 */

txHandlerTableData gTxIgnoreTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // LRE
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // Text
  txFnTextIgnore
};

txHandlerTableData gTxRootTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "stylesheet", txFnStartStylesheet, txFnEndStylesheet },
    { kNameSpaceID_XSLT, "transform", txFnStartStylesheet, txFnEndStylesheet },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // LRE
  { 0, 0, txFnStartElementError, txFnEndElementError }, // XXX enable fcp if version attribute is missing
  // Text
  txFnTextError
};

txHandlerTableData gTxTopTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "template", txFnStartTemplate, txFnEndTemplate },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartOtherTop, txFnEndOtherTop },
  // LRE
  { 0, 0, txFnStartOtherTop, txFnEndOtherTop },
  // Text
  txFnTextIgnore
};

txHandlerTableData gTxTemplateTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "fallback", txFnStartElementSetIgnore, txFnEndElementSetIgnore },
    { kNameSpaceID_XSLT, "text", txFnStartText, txFnEndText },
    { kNameSpaceID_XSLT, "value-of", txFnStartValueOf, txFnEndValueOf },
    { kNameSpaceID_XSLT, "if", txFnStartIf, txFnEndIf },
    { kNameSpaceID_XSLT, "apply-templates", txFnStartApplyTemplates, txFnEndApplyTemplates },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // LRE
  { 0, 0, txFnStartLRE, txFnEndLRE },
  // Text
  txFnText
};

txHandlerTableData gTxTextTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // LRE
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // Text
  txFnTextText
};

txHandlerTableData gTxApplyTemplatesTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore }, // should this be error?
  // LRE
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  // Text
  txFnTextIgnore
};



/**
 * txHandlerTable
 */
txHandlerTable::txHandlerTable() : mHandlers(MB_FALSE)
{
}

nsresult
txHandlerTable::init(txHandlerTableData* aTableData)
{
    nsresult rv = NS_OK;

    mTextHandler = aTableData->mTextHandler;
    mLREHandler = &aTableData->mLREHandler;
    mOtherHandler = &aTableData->mOtherHandler;

    txElementHandler* handler = aTableData->mHandlers;
    while (handler->mLocalName) {
        nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(handler->mLocalName);
        txExpandedName name(handler->mNamespaceID, nameAtom);
        // XXX this sucks
        rv = mHandlers.add(name, (TxObject*)handler);
        NS_ENSURE_SUCCESS(rv, rv);

        handler++;
    }
    return NS_OK;
}

txElementHandler*
txHandlerTable::find(PRInt32 aNamespaceID, nsIAtom* aLocalName)
{
    txExpandedName name(aNamespaceID, aLocalName);
    txElementHandler* handler = (txElementHandler*)mHandlers.get(name);
    if (!handler) {
        handler = mOtherHandler;
    }
    return handler;
}

#define INIT_HANDLER(_name)                                 \
    gTx##_name##Handler = new txHandlerTable();             \
    if (!gTx##_name##Handler)                               \
        return MB_FALSE;                                    \
                                                            \
    rv = gTx##_name##Handler->init(&gTx##_name##TableData); \
    if (NS_FAILED(rv))                                      \
        return MB_FALSE

#define SHUTDOWN_HANDLER(_name)                             \
    delete gTx##_name##Handler;                             \
    gTx##_name##Handler = nsnull

// static
MBool
txHandlerTable::init()
{
    nsresult rv = NS_OK;

    INIT_HANDLER(Root);
    INIT_HANDLER(Top);
    INIT_HANDLER(Ignore);
    INIT_HANDLER(Template);
    INIT_HANDLER(Text);
    INIT_HANDLER(ApplyTemplates);

    return MB_TRUE;
}

// static
void
txHandlerTable::shutdown()
{
    SHUTDOWN_HANDLER(Root);
    SHUTDOWN_HANDLER(Top);
    SHUTDOWN_HANDLER(Ignore);
    SHUTDOWN_HANDLER(Template);
    SHUTDOWN_HANDLER(Text);
    SHUTDOWN_HANDLER(ApplyTemplates);
}
