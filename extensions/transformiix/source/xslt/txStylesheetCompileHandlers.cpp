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
#include "txPatternParser.h"

txHandlerTable* gTxIgnoreHandler = 0;
txHandlerTable* gTxRootHandler = 0;
txHandlerTable* gTxTopHandler = 0;
txHandlerTable* gTxTemplateHandler = 0;
txHandlerTable* gTxTextHandler = 0;
txHandlerTable* gTxApplyTemplatesHandler = 0;
txHandlerTable* gTxCallTemplateHandler = 0;
txHandlerTable* gTxVariableHandler = 0;
txHandlerTable* gTxForEachHandler = 0;
txHandlerTable* gTxTopVariableHandler = 0;
txHandlerTable* gTxChooseHandler = 0;
txHandlerTable* gTxParamHandler = 0;

nsresult
txFnStartLRE(PRInt32 aNamespaceID,
             nsIAtom* aLocalName,
             nsIAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState);
nsresult
txFnEndLRE(txStylesheetCompilerState& aState);


#define TX_RETURN_IF_WHITESPACE(_str, _state)                               \
    do {                                                                    \
      if (!_state.mElementContext->mPreserveWhitespace &&                   \
          XMLUtils::isWhitespace(PromiseFlatString(_str))) {                \
          return NS_OK;                                                     \
      }                                                                     \
    } while(0)

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


nsresult
parseUseAttrSets(txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 PRBool aInXSLTNS,
                 txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount,
                                          aInXSLTNS ? kNameSpaceID_XSLT :
                                                      kNameSpaceID_None,
                                          txXSLTAtoms::useAttributeSets);
    if (attr) {
        txTokenizer tok(attr->mValue);
        while (tok.hasMoreTokens()) {
            txExpandedName name;
            nsresult rv = name.init(tok.nextToken(),
                                    aState.mElementContext->mMappings,
                                    PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);

            txInstruction* instr = new txInsertAttrSet(name);
            NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

            rv = aState.addInstruction(instr);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
    return NS_OK;
}

nsresult
getQNameAttr(txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             nsIAtom* aName,
             PRBool aRequired,
             txStylesheetCompilerState& aState,
             txExpandedName& aExpName)
{
    aExpName.reset();
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }


    nsresult rv = aExpName.init(attr->mValue,
                                aState.mElementContext->mMappings, PR_FALSE);
    if (!aRequired && NS_FAILED(rv) && aState.fcp()) {
        aExpName.reset();
        rv = NS_OK;
    }

    return rv;
}

nsresult
getExprAttr(txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            nsIAtom* aName,
            PRBool aRequired,
            txStylesheetCompilerState& aState,
            Expr*& aExpr)
{
    aExpr = nsnull;
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }

    aExpr = ExprParser::createExpr(attr->mValue, &aState);
    if (!aExpr && (aRequired || !aState.fcp())) {
        // XXX ErrorReport: XPath parse failure
        return NS_ERROR_XPATH_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getAVTAttr(txStylesheetAttr* aAttributes,
           PRInt32 aAttrCount,
           nsIAtom* aName,
           PRBool aRequired,
           txStylesheetCompilerState& aState,
           Expr*& aAVT)
{
    aAVT = nsnull;
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }

    aAVT = ExprParser::createAttributeValueTemplate(attr->mValue, &aState);
    if (!aAVT && (aRequired || !aState.fcp())) {
        // XXX ErrorReport: XPath parse failure
        return NS_ERROR_XPATH_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getPatternAttr(txStylesheetAttr* aAttributes,
               PRInt32 aAttrCount,
               nsIAtom* aName,
               PRBool aRequired,
               txStylesheetCompilerState& aState,
               txPattern*& aPattern)
{
    aPattern = nsnull;
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }

    aPattern = txPatternParser::createPattern(attr->mValue, &aState);
    if (!aPattern && (aRequired || !aState.fcp())) {
        // XXX ErrorReport: XSLT-Pattern parse failure
        return NS_ERROR_XPATH_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getNumberAttr(txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              nsIAtom* aName,
              PRBool aRequired,
              txStylesheetCompilerState& aState,
              double& aNumber)
{
    aNumber = Double::NaN;
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }

    aNumber = Double::toDouble(attr->mValue);
    if (Double::isNaN(aNumber) && (aRequired || !aState.fcp())) {
        // XXX ErrorReport: number parse failure
        return NS_ERROR_XPATH_PARSE_FAILURE;
    }

    return NS_OK;
}

nsresult
getAtomAttr(txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              nsIAtom* aName,
              PRBool aRequired,
              txStylesheetCompilerState& aState,
              nsIAtom** aAtom)
{
    *aAtom = nsnull;
    txStylesheetAttr* attr = getStyleAttr(aAttributes, aAttrCount, aName);
    if (!attr) {
        if (aRequired) {
            // XXX ErrorReport: missing required attribute
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }

        return NS_OK;
    }

    *aAtom = NS_NewAtom(attr->mValue);
    NS_ENSURE_TRUE(*aAtom, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

/**
 * Ignore and error handlers
 */
nsresult
txFnTextIgnore(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnTextError(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

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

nsresult
txFnStartLREStylesheet(PRInt32 aNamespaceID,
                       nsIAtom* aLocalName,
                       nsIAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    if (!getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_XSLT,
                      txXSLTAtoms::version)) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    nsresult rv = aState.pushHandlerTable(gTxTemplateHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txExpandedName nullExpr;
    double prio = Double::NaN;
    txPattern* match = new txRootPattern(MB_TRUE);
    NS_ENSURE_TRUE(match, NS_ERROR_OUT_OF_MEMORY);

    txTemplateItem* templ = new txTemplateItem(match, nullExpr, nullExpr,
                                               prio);
    NS_ENSURE_TRUE(templ, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addToplevelItem(templ);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aState.openInstructionContainer(templ);

    return txFnStartLRE(aNamespaceID, aLocalName, aPrefix, aAttributes,
                        aAttrCount, aState);
}

nsresult
txFnEndLREStylesheet(txStylesheetCompilerState& aState)
{
    nsresult rv = txFnEndLRE(aState);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txReturn();
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();
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


// xsl:template
nsresult
txFnStartTemplate(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushHandlerTable(gTxParamHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_FALSE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName mode;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::mode, PR_FALSE,
                      aState, mode);
    NS_ENSURE_SUCCESS(rv, rv);

    double prio = Double::NaN;
    rv = getNumberAttr(aAttributes, aAttrCount, txXSLTAtoms::priority, PR_FALSE,
                       aState, prio);
    NS_ENSURE_SUCCESS(rv, rv);

    txPattern* match = nsnull;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::match, PR_FALSE,
                        aState, match);
    NS_ENSURE_SUCCESS(rv, rv);

    txTemplateItem* templ = new txTemplateItem(match, name, mode, prio);
    NS_ENSURE_TRUE(templ, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addToplevelItem(templ);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aState.openInstructionContainer(templ);

    return NS_OK;
}

nsresult
txFnEndTemplate(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txReturn();
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.closeInstructionContainer();
    aState.popHandlerTable();

    return NS_OK;
}

// xsl:variable, xsl:param
nsresult
txFnStartTopVariable(PRInt32 aNamespaceID,
                     nsIAtom* aLocalName,
                     nsIAtom* aPrefix,
                     txStylesheetAttr* aAttributes,
                     PRInt32 aAttrCount,
                     txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txVariableItem* var =
        new txVariableItem(name, select, aLocalName == txXSLTAtoms::param);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    aState.openInstructionContainer(var);
    rv = aState.pushPtr(var);
    NS_ENSURE_SUCCESS(rv, rv);

    if (var->mValue) {
        // XXX should be gTxErrorHandler?
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxTopVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.addToplevelItem(var);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndTopVariable(txStylesheetCompilerState& aState)
{
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();
    txVariableItem* var = (txVariableItem*)aState.popPtr();

    if (prev == gTxTopVariableHandler) {
        // No children were found.
        NS_ASSERTION(!var->mValue, "There shouldn't be a select-expression here");
        var->mValue = new StringExpr(NS_LITERAL_STRING(""));
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (!var->mValue) {
        // If we don't have a select-expression there mush be children.
        txInstruction* instr = new txReturn();
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

        nsresult rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    aState.closeInstructionContainer();

    return NS_OK;
}

nsresult
txFnStartElementStartTopVar(PRInt32 aNamespaceID,
                            nsIAtom* aLocalName,
                            nsIAtom* aPrefix,
                            txStylesheetAttr* aAttributes,
                            PRInt32 aAttrCount,
                            txStylesheetCompilerState& aState)
{
    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextStartTopVar(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
}

// xsl:key
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
    
    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txPattern* match = nsnull;
    rv = getPatternAttr(aAttributes, aAttrCount, txXSLTAtoms::match, PR_TRUE,
                        aState, match);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* use = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::use, PR_TRUE,
                     aState, use);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.mStylesheet->addKey(name, match, use);
    NS_ENSURE_SUCCESS(rv, rv);
    
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

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_TRUE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    txStylesheetAttr* attr = nsnull;
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        attr = aAttributes + i;
        
        if (attr->mNamespaceID == kNameSpaceID_XSLT) {
            continue;
        }
        
        Expr* avt = ExprParser::createAttributeValueTemplate(attr->mValue,
                                                             &aState);
        NS_ENSURE_TRUE(avt, NS_ERROR_XPATH_PARSE_FAILURE);

        instr = new txLREAttribute(attr->mNamespaceID, attr->mLocalName,
                                   attr->mPrefix, avt);
        NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
        
        rv = aState.addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txFnEndLRE(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txEndElement;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// "LRE text"
nsresult
txFnText(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    txInstruction* instr = new txText(aStr, MB_FALSE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:apply-templates

  txPushParams
  [params]
  txPushNewContext  (holds <xsl:sort>s)
  txApplyTemplates
  txPopParams
*/
nsresult
txFnStartApplyTemplates(PRInt32 aNamespaceID,
                        nsIAtom* aLocalName,
                        nsIAtom* aPrefix,
                        txStylesheetAttr* aAttributes,
                        PRInt32 aAttrCount,
                        txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txInstruction* instr = new txPushParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName mode;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::mode, PR_FALSE,
                      aState, mode);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txApplyTemplates(mode);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!select) {
        txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
        NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

        select = new LocationStep(nt, LocationStep::CHILD_AXIS);
        NS_ENSURE_TRUE(select, NS_ERROR_OUT_OF_MEMORY);
    }

    txPushNewContext* pushcontext = new txPushNewContext(select);
    NS_ENSURE_TRUE(pushcontext, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushSorter(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.pushObject(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxApplyTemplatesHandler);
}

nsresult
txFnEndApplyTemplates(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    // txPushNewContext
    nsresult rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    aState.popSorter();

    // txApplyTemplates
    rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txPopParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:attribute
nsresult
txFnStartAttribute(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txInstruction* instr = new txPushStringHandler(PR_TRUE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* name = nsnull;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* nspace = nsnull;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::_namespace, PR_FALSE,
                    aState, nspace);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txAttribute(name, nspace, aState.mElementContext->mMappings);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndAttribute(txStylesheetCompilerState& aState)
{
    txInstruction* instr = (txInstruction*)aState.popObject();
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:call-template

  txPushParams
  [params]
  txCallTemplate
  txPopParams
*/
nsresult
txFnStartCallTemplate(PRInt32 aNamespaceID,
                      nsIAtom* aLocalName,
                      nsIAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txInstruction* instr = new txPushParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* callTempl = new txCallTemplate(name);
    NS_ENSURE_TRUE(callTempl, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(callTempl);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxCallTemplateHandler);
}

nsresult
txFnEndCallTemplate(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    // txCallTemplate
    nsresult rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txPopParams;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:choose

  txCondotionalGoto      --+        \
  [children]               |         | one for each xsl:when
  txGoTo           --+     |        /
                     |     |
  txCondotionalGoto  |   <-+  --+
  [children]         |          |
  txGoTo           --+          |
                     |          |
  [children]         |        <-+      for the xsl:otherwise, if there is one
                   <-+
*/
nsresult
txFnStartChoose(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushChooseGotoList();
    NS_ENSURE_SUCCESS(rv, rv);
    
    return aState.pushHandlerTable(gTxChooseHandler);
}

nsresult
txFnEndChoose(txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;
    aState.popHandlerTable();
    txListIterator iter(aState.mChooseGotoList);
    txGoTo* gotoinstr;
    while ((gotoinstr = (txGoTo*)iter.next())) {
        rv = aState.addGotoTarget(&gotoinstr->mTarget);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    aState.popChooseGotoList();

    return NS_OK;
}

// xsl:comment
nsresult
txFnStartComment(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txPushStringHandler(PR_TRUE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndComment(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txComment;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:copy

  txCopy        -+
  [children]     |
  txEndElement   |
               <-+
*/
nsresult
txFnStartCopy(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    txCopy* copy = new txCopy;
    NS_ENSURE_TRUE(copy, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.pushPtr(copy);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(copy);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_FALSE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndCopy(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txEndElement;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txCopy* copy = (txCopy*)aState.popPtr();
    rv = aState.addGotoTarget(&copy->mBailTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:copy-of

  txCopyOf
*/
nsresult
txFnStartCopyOf(PRInt32 aNamespaceID,
                nsIAtom* aLocalName,
                nsIAtom* aPrefix,
                txStylesheetAttr* aAttributes,
                PRInt32 aAttrCount,
                txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                    aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txCopyOf(select);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);
    
    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndCopyOf(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

// xsl:element
nsresult
txFnStartElement(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    Expr* name = nsnull;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* nspace = nsnull;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::_namespace, PR_FALSE,
                    aState, nspace);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr =
        new txStartElement(name, nspace, aState.mElementContext->mMappings);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = parseUseAttrSets(aAttributes, aAttrCount, PR_FALSE, aState);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndElement(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txEndElement;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
  xsl:for-each

  txPushNewContext  (holds <xsl:sort>s)
  txForEach  -+  <-+
  [children]  |    |
  txGoTo      |  --+
            <-+
*/
nsresult
txFnStartForEach(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txPushNewContext* pushcontext = new txPushNewContext(select);
    NS_ENSURE_TRUE(pushcontext, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushSorter(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(pushcontext);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txForEach* forEach = new txForEach();
    NS_ENSURE_TRUE(forEach, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(forEach);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(forEach);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txGoTo(forEach);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxForEachHandler);
}

nsresult
txFnEndForEach(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    // txGoTo
    nsresult rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    txForEach* forEach = (txForEach*)aState.popPtr();
    rv = aState.addGotoTarget(&forEach->mEndTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.popSorter();

    return NS_OK;
}

nsresult
txFnStartElementConinueTemplate(PRInt32 aNamespaceID,
                                nsIAtom* aLocalName,
                                nsIAtom* aPrefix,
                                txStylesheetAttr* aAttributes,
                                PRInt32 aAttrCount,
                                txStylesheetCompilerState& aState)
{
    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextConinueTemplate(const nsAString& aStr,
                        txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
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
    nsresult rv = NS_OK;

    Expr* test = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::test, PR_TRUE,
                     aState, test);
    NS_ENSURE_SUCCESS(rv, rv);

    txConditionalGoto* condGoto = new txConditionalGoto(test, nsnull);
    NS_ENSURE_TRUE(condGoto, NS_ERROR_OUT_OF_MEMORY);

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

// xsl:message
nsresult
txFnStartMessage(PRInt32 aNamespaceID,
                 nsIAtom* aLocalName,
                 nsIAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txPushStringHandler(PR_FALSE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIAtom> terminateAtom;
    rv = getAtomAttr(aAttributes, aAttrCount, txXSLTAtoms::terminate,
                     PR_FALSE, aState, getter_AddRefs(terminateAtom));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool terminate = PR_FALSE;
    if (terminateAtom == txXSLTAtoms::yes) {
        terminate = MB_TRUE;
    }
    else if (terminateAtom && terminateAtom != txXSLTAtoms::no &&
             !aState.fcp()) {
        // XXX ErrorReport: unknown value for terminate
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    instr = new txMessage(terminate);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndMessage(txStylesheetCompilerState& aState)
{
    txInstruction* instr = (txInstruction*)aState.popObject();
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
    xsl:otherwise
    
    (see xsl:choose)
*/
nsresult
txFnStartOtherwise(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    return aState.pushHandlerTable(gTxTemplateHandler);;
}

nsresult
txFnEndOtherwise(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    aState.mHandlerTable = gTxIgnoreHandler; // XXX should be gTxErrorHandler

    return NS_OK;
}

/*
    xsl:param
    
    txCheckParam    --+
    txPushRTFHandler  |   (for RTF-parameters)
    txSetVariable     |
                    <-+
*/
nsresult
txFnStartParam(PRInt32 aNamespaceID,
               nsIAtom* aLocalName,
               nsIAtom* aPrefix,
               txStylesheetAttr* aAttributes,
               PRInt32 aAttrCount,
               txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txCheckParam* checkParam = new txCheckParam(name);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = aState.pushPtr(checkParam);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(checkParam);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txSetVariable* var = new txSetVariable(name, select);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        // XXX should be gTxErrorHandler?
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndParam(txStylesheetCompilerState& aState)
{
    txSetVariable* var = (txSetVariable*)aState.popObject();
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        // No children were found.
        NS_ASSERTION(!var->mValue, "There shouldn't be a select-expression here");
        var->mValue = new StringExpr(NS_LITERAL_STRING(""));
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = aState.addVariable(var->mName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(var);
    NS_ENSURE_SUCCESS(rv, rv);

    txCheckParam* checkParam = (txCheckParam*)aState.popPtr();
    aState.addGotoTarget(&checkParam->mBailTarget);

    return NS_OK;
}

// xsl:processing-instruction
nsresult
txFnStartPI(PRInt32 aNamespaceID,
            nsIAtom* aLocalName,
            nsIAtom* aPrefix,
            txStylesheetAttr* aAttributes,
            PRInt32 aAttrCount,
            txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txPushStringHandler(PR_TRUE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* name;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                    aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txProcessingInstruction(name);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndPI(txStylesheetCompilerState& aState)
{
    txInstruction* instr = (txInstruction*)aState.popObject();
    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
    xsl:sort
    
    (no instructions)
*/
nsresult
txFnStartSort(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    Expr* select;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!select) {
        txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
        NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

        select = new LocationStep(nt, LocationStep::SELF_AXIS);
        NS_ENSURE_TRUE(select, NS_ERROR_OUT_OF_MEMORY);
    }

    Expr* lang;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::lang, PR_FALSE,
                    aState, lang);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* dataType;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::dataType, PR_FALSE,
                    aState, dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* order;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::order, PR_FALSE,
                    aState, order);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* caseOrder;
    rv = getAVTAttr(aAttributes, aAttrCount, txXSLTAtoms::caseOrder, PR_FALSE,
                    aState, caseOrder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.mSorter->addSort(select, lang, dataType, order, caseOrder);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndSort(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();

    return NS_OK;
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
    NS_ASSERTION(!aState.mDOE, "nested d-o-e elements should not happen");

    nsresult rv = NS_OK;
    nsCOMPtr<nsIAtom> doe;
    rv = getAtomAttr(aAttributes, aAttrCount,
                     txXSLTAtoms::disableOutputEscaping, PR_FALSE, aState,
                     getter_AddRefs(doe));
    NS_ENSURE_SUCCESS(rv, rv);

    if (doe == txXSLTAtoms::yes) {
        aState.mDOE = MB_TRUE;
    }
    else if (doe && doe != txXSLTAtoms::no && !aState.fcp()) {
        // XXX ErrorReport: unknown value for d-o-e
        return NS_ERROR_XSLT_PARSE_FAILURE;
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
    nsresult rv = NS_OK;

    MBool doe = MB_FALSE;
    nsCOMPtr<nsIAtom> doeAtom;
    rv = getAtomAttr(aAttributes, aAttrCount,
                     txXSLTAtoms::disableOutputEscaping, PR_FALSE, aState,
                     getter_AddRefs(doeAtom));
    NS_ENSURE_SUCCESS(rv, rv);

    if (doeAtom == txXSLTAtoms::yes) {
        doe = MB_TRUE;
    }
    else if (doeAtom && doeAtom != txXSLTAtoms::no && !aState.fcp()) {
        // XXX ErrorReport: unknown value for d-o-e
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_TRUE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txValueOf(select, doe);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

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

// xsl:variable
nsresult
txFnStartVariable(PRInt32 aNamespaceID,
                  nsIAtom* aLocalName,
                  nsIAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txSetVariable* var = new txSetVariable(name, select);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        // XXX should be gTxErrorHandler?
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndVariable(txStylesheetCompilerState& aState)
{
    txSetVariable* var = (txSetVariable*)aState.popObject();

    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        // No children were found.
        NS_ASSERTION(!var->mValue, "There shouldn't be a select-expression here");
        var->mValue = new StringExpr(NS_LITERAL_STRING(""));
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = aState.addVariable(var->mName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(var);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnStartElementStartRTF(PRInt32 aNamespaceID,
                         nsIAtom* aLocalName,
                         nsIAtom* aPrefix,
                         txStylesheetAttr* aAttributes,
                         PRInt32 aAttrCount,
                         txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txPushRTFHandler;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
}

nsresult
txFnTextStartRTF(const nsAString& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    txInstruction* instr = new txPushRTFHandler;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    aState.mHandlerTable = gTxTemplateHandler;

    return NS_ERROR_XSLT_GET_NEW_HANDLER;
}

/*
    xsl:when
    
    (see xsl:choose)
*/
nsresult
txFnStartWhen(PRInt32 aNamespaceID,
              nsIAtom* aLocalName,
              nsIAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    Expr* test = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::test, PR_TRUE,
                     aState, test);
    NS_ENSURE_SUCCESS(rv, rv);

    txConditionalGoto* condGoto = new txConditionalGoto(test, nsnull);
    NS_ENSURE_TRUE(condGoto, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(condGoto);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxTemplateHandler);
}

nsresult
txFnEndWhen(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    txGoTo* gotoinstr = new txGoTo(nsnull);
    NS_ENSURE_TRUE(gotoinstr, NS_ERROR_OUT_OF_MEMORY);
    
    nsresult rv = aState.mChooseGotoList->add(gotoinstr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = aState.addInstruction(gotoinstr);
    NS_ENSURE_SUCCESS(rv, rv);

    txConditionalGoto* condGoto = (txConditionalGoto*)aState.popPtr();
    rv = aState.addGotoTarget(&condGoto->mTarget);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

/*
    xsl:with-param
    
    txPushRTFHandler (for RTF-parameters)
    txSetParam
*/
nsresult
txFnStartWithParam(PRInt32 aNamespaceID,
                   nsIAtom* aLocalName,
                   nsIAtom* aPrefix,
                   txStylesheetAttr* aAttributes,
                   PRInt32 aAttrCount,
                   txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txExpandedName name;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::name, PR_TRUE,
                      aState, name);
    NS_ENSURE_SUCCESS(rv, rv);

    Expr* select = nsnull;
    rv = getExprAttr(aAttributes, aAttrCount, txXSLTAtoms::select, PR_FALSE,
                     aState, select);
    NS_ENSURE_SUCCESS(rv, rv);

    txSetParam* var = new txSetParam(name, select);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (var->mValue) {
        // XXX should be gTxErrorHandler?
        rv = aState.pushHandlerTable(gTxIgnoreHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
        rv = aState.pushHandlerTable(gTxVariableHandler);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = aState.pushObject(var);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndWithParam(txStylesheetCompilerState& aState)
{
    txSetParam* var = (txSetParam*)aState.popObject();
    txHandlerTable* prev = aState.mHandlerTable;
    aState.popHandlerTable();

    if (prev == gTxVariableHandler) {
        // No children were found.
        NS_ASSERTION(!var->mValue, "There shouldn't be a select-expression here");
        var->mValue = new StringExpr(NS_LITERAL_STRING(""));
        NS_ENSURE_TRUE(var->mValue, NS_ERROR_OUT_OF_MEMORY);
    }

    nsresult rv = aState.addVariable(var->mName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(var);
    NS_ENSURE_SUCCESS(rv, rv);

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
  { 0, 0, txFnStartLREStylesheet, txFnEndLREStylesheet },
  // Text
  txFnTextError
};

txHandlerTableData gTxTopTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "template", txFnStartTemplate, txFnEndTemplate },
    { kNameSpaceID_XSLT, "variable", txFnStartTopVariable, txFnEndTopVariable },
    { kNameSpaceID_XSLT, "param", txFnStartTopVariable, txFnEndTopVariable },
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
  { { kNameSpaceID_XSLT, "apply-templates", txFnStartApplyTemplates, txFnEndApplyTemplates },
    { kNameSpaceID_XSLT, "attribute", txFnStartAttribute, txFnEndAttribute },
    { kNameSpaceID_XSLT, "call-template", txFnStartCallTemplate, txFnEndCallTemplate },
    { kNameSpaceID_XSLT, "choose", txFnStartChoose, txFnEndChoose },
    { kNameSpaceID_XSLT, "comment", txFnStartComment, txFnEndComment },
    { kNameSpaceID_XSLT, "copy", txFnStartCopy, txFnEndCopy },
    { kNameSpaceID_XSLT, "copy-of", txFnStartCopyOf, txFnEndCopyOf },
    { kNameSpaceID_XSLT, "element", txFnStartElement, txFnEndElement },
    { kNameSpaceID_XSLT, "fallback", txFnStartElementSetIgnore, txFnEndElementSetIgnore },
    { kNameSpaceID_XSLT, "for-each", txFnStartForEach, txFnEndForEach },
    { kNameSpaceID_XSLT, "if", txFnStartIf, txFnEndIf },
    { kNameSpaceID_XSLT, "message", txFnStartMessage, txFnEndMessage },
    { kNameSpaceID_XSLT, "processing-instruction", txFnStartPI, txFnEndPI },
    { kNameSpaceID_XSLT, "text", txFnStartText, txFnEndText },
    { kNameSpaceID_XSLT, "value-of", txFnStartValueOf, txFnEndValueOf },
    { kNameSpaceID_XSLT, "variable", txFnStartVariable, txFnEndVariable },
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
  { { kNameSpaceID_XSLT, "sort", txFnStartSort, txFnEndSort },
    { kNameSpaceID_XSLT, "with-param", txFnStartWithParam, txFnEndWithParam },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore }, // should this be error?
  // LRE
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  // Text
  txFnTextIgnore
};

txHandlerTableData gTxCallTemplateTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "with-param", txFnStartWithParam, txFnEndWithParam },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore }, // should this be error?
  // LRE
  { 0, 0, txFnStartElementSetIgnore, txFnEndElementSetIgnore },
  // Text
  txFnTextIgnore
};

txHandlerTableData gTxVariableTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementStartRTF, 0 },
  // LRE
  { 0, 0, txFnStartElementStartRTF, 0 },
  // Text
  txFnTextStartRTF
};

txHandlerTableData gTxForEachTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "sort", txFnStartSort, txFnEndSort },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementConinueTemplate, 0 },
  // LRE
  { 0, 0, txFnStartElementConinueTemplate, 0 },
  // Text
  txFnTextConinueTemplate
};

txHandlerTableData gTxTopVariableTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementStartTopVar, 0 },
  // LRE
  { 0, 0, txFnStartElementStartTopVar, 0 },
  // Text
  txFnTextStartTopVar
};

txHandlerTableData gTxChooseTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "otherwise", txFnStartOtherwise, txFnEndOtherwise },
    { kNameSpaceID_XSLT, "when", txFnStartWhen, txFnEndWhen },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementError, 0 },
  // LRE
  { 0, 0, txFnStartElementError, 0 },
  // Text
  txFnTextError
};

txHandlerTableData gTxParamTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "param", txFnStartParam, txFnEndParam },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementConinueTemplate, 0 },
  // LRE
  { 0, 0, txFnStartElementConinueTemplate, 0 },
  // Text
  txFnTextConinueTemplate
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
    INIT_HANDLER(CallTemplate);
    INIT_HANDLER(Variable);
    INIT_HANDLER(ForEach);
    INIT_HANDLER(TopVariable);
    INIT_HANDLER(Choose);
    INIT_HANDLER(Param);

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
    SHUTDOWN_HANDLER(CallTemplate);
    SHUTDOWN_HANDLER(Variable);
    SHUTDOWN_HANDLER(ForEach);
    SHUTDOWN_HANDLER(TopVariable);
    SHUTDOWN_HANDLER(Choose);
    SHUTDOWN_HANDLER(Param);
}
