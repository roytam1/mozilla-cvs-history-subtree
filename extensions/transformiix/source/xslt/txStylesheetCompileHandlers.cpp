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

    txExpandedName mode;
    rv = getQNameAttr(aAttributes, aAttrCount, txXSLTAtoms::mode, PR_FALSE,
                      aState, mode);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txApplyTemplates(mode);
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

    instr = new txPushNewContext(select);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
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

    // txApplyTemplates
    rv = aState.addInstruction((txInstruction*)aState.popObject());
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

// xsl:call-template
nsresult
txFnStartCallTemplate(PRInt32 aNamespaceID,
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

// xsl:for-each
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

    txInstruction* instr = new txPushNewContext(select);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txForEach* forEach = new txForEach();
    NS_ENSURE_TRUE(forEach, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushPtr(forEach);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aState.addInstruction(forEach);
    NS_ENSURE_SUCCESS(rv, rv);

    instr = new txGoTo(forEach);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState.pushObject(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnEndForEach(txStylesheetCompilerState& aState)
{
    // txGoTo
    nsresult rv = aState.addInstruction((txInstruction*)aState.popObject());
    NS_ENSURE_SUCCESS(rv, rv);

    txForEach* forEach = (txForEach*)aState.popPtr();
    aState.addGotoTarget(&forEach->mEndTarget);

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
  { { kNameSpaceID_XSLT, "apply-templates", txFnStartApplyTemplates, txFnEndApplyTemplates },
    { kNameSpaceID_XSLT, "attribute", txFnStartAttribute, txFnEndAttribute },
    { kNameSpaceID_XSLT, "call-template", txFnStartCallTemplate, txFnEndCallTemplate },
    { kNameSpaceID_XSLT, "comment", txFnStartComment, txFnEndComment },
    { kNameSpaceID_XSLT, "element", txFnStartElement, txFnEndElement },
    { kNameSpaceID_XSLT, "fallback", txFnStartElementSetIgnore, txFnEndElementSetIgnore },
    { kNameSpaceID_XSLT, "for-each", txFnStartForEach, txFnEndForEach },
    { kNameSpaceID_XSLT, "if", txFnStartIf, txFnEndIf },
    { kNameSpaceID_XSLT, "message", txFnStartMessage, txFnEndMessage },
    { kNameSpaceID_XSLT, "processing-instruction", txFnStartPI, txFnEndPI },
    { kNameSpaceID_XSLT, "text", txFnStartText, txFnEndText },
    { kNameSpaceID_XSLT, "value-of", txFnStartValueOf, txFnEndValueOf },
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

txHandlerTableData gTxCallTemplateTableData = {
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
