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

#include "txInstructions.h"
#include "txError.h"
#include "Expr.h"
#include "ExprResult.h"
#include "txStylesheet.h"
#include "txNodeSetContext.h"
#include "txTextHandler.h"
#include "nsIConsoleService.h"
#include "nsIServiceManagerUtils.h"
#include "txStringUtils.h"
#include "txAtoms.h"
#include "txRtfHandler.h"
#include "txNodeSorter.h"

txApplyTemplates::txApplyTemplates(const txExpandedName& aMode)
    : mMode(aMode)
{
}

nsresult
txApplyTemplates::execute(txExecutionState& aEs)
{
    txNodeSetContext* context = (txNodeSetContext*)aEs.getEvalContext();
    if (!context->hasNext()) {
        delete aEs.popEvalContext();
        
        return NS_OK;
    }

    context->next();
    
    txStylesheet::ImportFrame* frame = 0;
    txInstruction* templ =
        aEs.mStylesheet->findTemplate(context->getContextNode(), mMode, &aEs,
                                      nsnull, &frame);

    return aEs.runTemplate(templ, this);
}

txAttribute::txAttribute(Expr* aName, Expr* aNamespace,
                         const txNamespaceMap& aMappings)
    : mName(aName),
      mNamespace(aNamespace),
      mMappings(aMappings)
{
}

txAttribute::~txAttribute()
{
    delete mName;
    delete mNamespace;
}

nsresult
txAttribute::execute(txExecutionState& aEs)
{
    nsresult rv = NS_OK;

    ExprResult* exprRes = mName->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    nsAutoString name;
    exprRes->stringValue(name);
    delete exprRes;

    if (!XMLUtils::isValidQName(name) ||
        TX_StringEqualsAtom(name, txXMLAtoms::xmlns)) {
        // tunkate name to indicate failure
        name.Truncate();
    }

    nsCOMPtr<nsIAtom> prefix;
    XMLUtils::getPrefix(name, getter_AddRefs(prefix));

    PRInt32 nsId = kNameSpaceID_None;
    if (!name.IsEmpty()) {
        if (mNamespace) {
            exprRes = mNamespace->evaluate(aEs.getEvalContext());
            NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

            nsAutoString nspace;
            exprRes->stringValue(nspace);
            delete exprRes;

            if (!nspace.IsEmpty()) {
#ifdef TX_EXE
                nsId = txNamespaceManager::getNamespaceID(nspace);
#else
                NS_ASSERTION(gTxNameSpaceManager, "No namespace manager");
                rv = gTxNameSpaceManager->RegisterNameSpace(nspace, nsId);
                NS_ENSURE_SUCCESS(rv, rv);
#endif
            }
        }
        else if (prefix) {
            nsId = mMappings.lookupNamespace(prefix);
            if (nsId == kNameSpaceID_Unknown) {
                // tunkate name to indicate failure
                name.Truncate();
            }
        }
    }

    if (prefix == txXMLAtoms::xmlns) {
        // Cut xmlns: (6 characters)
        name.Cut(0, 6);
    }

    txTextHandler* handler = (txTextHandler*)aEs.popResultHandler();
    if (!name.IsEmpty()) {
        // add attribute if everything was ok
        aEs.mResultHandler->attribute(name, nsId, handler->mValue);
    }
    delete handler;

    return NS_OK;
}

txCallTemplate::txCallTemplate(const txExpandedName& aName)
    : mName(aName)
{
}

nsresult
txCallTemplate::execute(txExecutionState& aEs)
{
    txInstruction* instr = aEs.mStylesheet->getNamedTemplate(mName);
    NS_ENSURE_TRUE(instr, NS_ERROR_XSLT_EXECUTION_FAILURE);

    nsresult rv = aEs.runTemplate(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return NS_OK;
}

txConditionalGoto::txConditionalGoto(Expr* aCondition, txInstruction* aTarget)
    : mCondition(aCondition),
      mTarget(aTarget)
{
}

txConditionalGoto::~txConditionalGoto()
{
    delete mCondition;
}

nsresult
txConditionalGoto::execute(txExecutionState& aEs)
{
    ExprResult* exprRes = mCondition->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    if (!exprRes->booleanValue()) {
        aEs.gotoInstruction(mTarget);
    }
    delete exprRes;

    return NS_OK;
}

nsresult
txComment::execute(txExecutionState& aEs)
{
    txTextHandler* handler = (txTextHandler*)aEs.popResultHandler();
    PRUint32 length = handler->mValue.Length();
    PRInt32 pos = 0;
    while ((pos = handler->mValue.FindChar('-', (PRUint32)pos)) != kNotFound) {
        ++pos;
        if ((PRUint32)pos == length || handler->mValue.CharAt(pos) == '-') {
            handler->mValue.Insert(PRUnichar(' '), pos++);
            ++length;
        }
    }

    aEs.mResultHandler->comment(handler->mValue);
    delete handler;

    return NS_OK;
}

nsresult
txEndElement::execute(txExecutionState& aEs)
{
    PRInt32 namespaceID = aEs.popInt();
    nsAutoString nodeName;
    aEs.popString(nodeName);
    
    
    // For xsl:elements with a bad name we push an empty name
    if (!nodeName.IsEmpty()) {
        aEs.mResultHandler->endElement(nodeName, namespaceID);
    }

    return NS_OK;
}

txForEach::txForEach()
    : mEndTarget(nsnull)
{
}

nsresult
txForEach::execute(txExecutionState& aEs)
{
    txNodeSetContext* context = (txNodeSetContext*)aEs.getEvalContext();
    if (!context->hasNext()) {
        delete aEs.popEvalContext();
        aEs.gotoInstruction(mEndTarget);

        return NS_OK;
    }

    context->next();
    
    return NS_OK;
}

txGoTo::txGoTo(txInstruction* aTarget)
    : mTarget(aTarget)
{
}

nsresult
txGoTo::execute(txExecutionState& aEs)
{
    aEs.gotoInstruction(mTarget);

    return NS_OK;
}

txInsertAttrSet::txInsertAttrSet(const txExpandedName& aName)
    : mName(aName)
{
}

nsresult
txInsertAttrSet::execute(txExecutionState& aEs)
{
    txInstruction* instr = aEs.mStylesheet->getAttributeSet(mName);
    NS_ENSURE_TRUE(instr, NS_ERROR_XSLT_EXECUTION_FAILURE);

    nsresult rv = aEs.runTemplate(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return NS_OK;
}

txLREAttribute::txLREAttribute(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                               nsIAtom* aPrefix, Expr* aValue)
    : mNamespaceID(aNamespaceID),
      mLocalName(aLocalName),
      mPrefix(aPrefix),
      mValue(aValue)
{
}

txLREAttribute::~txLREAttribute()
{
    delete mValue;
}

nsresult
txLREAttribute::execute(txExecutionState& aEs)
{
    // We should atomize the resulthandler
    nsAutoString nodeName;
    if (mPrefix) {
        mPrefix->ToString(nodeName);
        nsAutoString localName;
        nodeName.Append((PRUnichar)':');
        mLocalName->ToString(localName);
        nodeName.Append(localName);
    }
    else {
        mLocalName->ToString(nodeName);
    }

    ExprResult* exprRes = mValue->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    nsAutoString value;
    exprRes->stringValue(value);
    delete exprRes;

    aEs.mResultHandler->attribute(nodeName, mNamespaceID, value);

    return NS_OK;
}

txMessage::txMessage(PRBool aTerminate)
    : mTerminate(aTerminate)
{
}

nsresult
txMessage::execute(txExecutionState& aEs)
{
    txTextHandler* handler = (txTextHandler*)aEs.popResultHandler();

    nsCOMPtr<nsIConsoleService> consoleSvc = 
      do_GetService("@mozilla.org/consoleservice;1");
    if (consoleSvc) {
        nsAutoString logString(NS_LITERAL_STRING("xsl:message - "));
        logString.Append(handler->mValue);
        consoleSvc->LogStringMessage(logString.get());
    }
    delete handler;

    return mTerminate ? NS_ERROR_XSLT_ABORTED : NS_OK;
}

txProcessingInstruction::txProcessingInstruction(Expr* aName)
    : mName(aName)
{
}

nsresult
txProcessingInstruction::execute(txExecutionState& aEs)
{
    nsresult rv = NS_OK;
    txTextHandler* handler = (txTextHandler*)aEs.popResultHandler();
    XMLUtils::normalizePIValue(handler->mValue);

    ExprResult* exprRes = mName->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    nsAutoString name;
    exprRes->stringValue(name);
    delete exprRes;

    // Check name validity (must be valid NCName and a PITarget)
    // XXX Need to check for NCName and PITarget
    if (!XMLUtils::isValidQName(name)) {
        // XXX ErrorReport: bad PI-target
        delete handler;
        return NS_ERROR_FAILURE;
    }

    aEs.mResultHandler->processingInstruction(name, handler->mValue);

    return NS_OK;
}

txPushNewContext::txPushNewContext(Expr* aSelect)
    : mSelect(aSelect)
{
}

txPushNewContext::~txPushNewContext()
{
    delete mSelect;

    PRInt32 i;
    for (i = 0; i < mSortKeys.Count(); ++i)
    {
        delete (SortKey*)mSortKeys[i];
    }
}

nsresult
txPushNewContext::execute(txExecutionState& aEs)
{
    nsresult rv = NS_OK;
    ExprResult* exprRes = mSelect->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    if (exprRes->getResultType() != ExprResult::NODESET) {
        delete exprRes;
        // XXX ErrorReport: nodeset expected
        return NS_ERROR_XSLT_NODESET_EXPECTED;
    }
    
    NodeSet* nodes = (NodeSet*)exprRes;
    
    if (!nodes->isEmpty()) {
        txNodeSorter sorter;
        PRInt32 i, count = mSortKeys.Count();
        for (i = 0; i < count; ++i) {
            SortKey* sort = (SortKey*)mSortKeys[i];
            rv = sorter.addSortElement(sort->mSelectExpr, sort->mLangExpr,
                                       sort->mDataTypeExpr, sort->mOrderExpr,
                                       sort->mCaseOrderExpr,
                                       aEs.getEvalContext());
            NS_ENSURE_SUCCESS(rv, rv);
        }
        rv = sorter.sortNodeSet(nodes, &aEs);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    
    txNodeSetContext* context = new txOwningNodeSetContext(nodes, &aEs);
    if (!context) {
        delete exprRes;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = aEs.pushEvalContext(context);
    if (NS_FAILED(rv)) {
        delete context;
        return rv;
    }
    
    return NS_OK;
}

nsresult
txPushNewContext::addSort(Expr* aSelectExpr, Expr* aLangExpr, Expr* aDataTypeExpr,
                          Expr* aOrderExpr, Expr* aCaseOrderExpr)
{
    SortKey* sort = new SortKey(aSelectExpr, aLangExpr, aDataTypeExpr,
                                aOrderExpr, aCaseOrderExpr);
    NS_ENSURE_TRUE(sort, NS_ERROR_OUT_OF_MEMORY);

    if (!mSortKeys.AppendElement(sort)) {
        delete sort;
        return NS_ERROR_OUT_OF_MEMORY;
    }
   
    return NS_OK;
}

txPushNewContext::SortKey::SortKey(Expr* aSelectExpr, Expr* aLangExpr,
                                   Expr* aDataTypeExpr, Expr* aOrderExpr,
                                   Expr* aCaseOrderExpr)
    : mSelectExpr(aSelectExpr), mLangExpr(aLangExpr),
      mDataTypeExpr(aDataTypeExpr), mOrderExpr(aOrderExpr),
      mCaseOrderExpr(aCaseOrderExpr)
{
}

txPushNewContext::SortKey::~SortKey()
{
    delete mSelectExpr;
    delete mLangExpr;
    delete mDataTypeExpr;
    delete mOrderExpr;
    delete mCaseOrderExpr;
}

nsresult
txPushRTFHandler::execute(txExecutionState& aEs)
{
    Document* rtfdoc;
    nsresult rv = aEs.getRTFDocument(&rtfdoc);
    NS_ENSURE_SUCCESS(rv, rv);

    txXMLEventHandler* handler = new txRtfHandler(rtfdoc);
    NS_ENSURE_TRUE(handler, NS_ERROR_OUT_OF_MEMORY);
    
    rv = aEs.pushResultHandler(handler);
    if (NS_FAILED(rv)) {
        delete handler;
        return rv;
    }

    return NS_OK;
}

txPushStringHandler::txPushStringHandler(PRBool aOnlyText)
    : mOnlyText(aOnlyText)
{
}

nsresult
txPushStringHandler::execute(txExecutionState& aEs)
{
    txXMLEventHandler* handler = new txTextHandler(mOnlyText);
    NS_ENSURE_TRUE(handler, NS_ERROR_OUT_OF_MEMORY);
    
    nsresult rv = aEs.pushResultHandler(handler);
    if (NS_FAILED(rv)) {
        delete handler;
        return rv;
    }

    return NS_OK;
}

nsresult
txRecursionCheckpointEnd::execute(txExecutionState& aEs)
{
    aEs.leaveRecursionCheckpoint();
    return NS_OK;
}

txRecursionCheckpointStart::txRecursionCheckpointStart(const nsAString& aName)
    : mName(aName)
{
}

nsresult
txRecursionCheckpointStart::execute(txExecutionState& aEs)
{
    // XXX will this work? what if the context is in two different states
    return aEs.enterRecursionCheckpoint(this, aEs.getEvalContext());
}

txRemoveVariable::txRemoveVariable(const txExpandedName& aName)
    : mName(aName)
{
}

nsresult
txRemoveVariable::execute(txExecutionState& aEs)
{
    aEs.removeVariable(mName);
    
    return NS_OK;
}

nsresult
txReturn::execute(txExecutionState& aEs)
{
    aEs.returnFromTemplate();

    return NS_OK;
}

txSetVariable::txSetVariable(const txExpandedName& aName, Expr* aValue)
    : mName(aName), mValue(aValue)
{
}

txSetVariable::~txSetVariable()
{
    delete mValue;
}

nsresult
txSetVariable::execute(txExecutionState& aEs)
{
    ExprResult* exprRes;
    if (mValue) {
        exprRes = mValue->evaluate(aEs.getEvalContext());
        NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);
    }
    else {
        txRtfHandler* handler = (txRtfHandler*)aEs.popResultHandler();
        exprRes = handler->mResultTreeFragment;
        handler->mResultTreeFragment = nsnull;
        delete handler;
    }
    
    nsresult rv = aEs.bindVariable(mName, exprRes, MB_TRUE);
    if (NS_FAILED(rv)) {
        delete exprRes;
        return rv;
    }
    
    return NS_OK;
}

txStartElement::txStartElement(Expr* aName, Expr* aNamespace,
                               const txNamespaceMap& aMappings)
    : mName(aName),
      mNamespace(aNamespace),
      mMappings(aMappings)
{
}

txStartElement::~txStartElement()
{
    delete mName;
    delete mNamespace;
}

nsresult
txStartElement::execute(txExecutionState& aEs)
{
    nsresult rv = NS_OK;

    ExprResult* exprRes = mName->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    nsAutoString name;
    exprRes->stringValue(name);
    delete exprRes;

    if (!XMLUtils::isValidQName(name)) {
        // tunkate name to indicate failure
        name.Truncate();
    }

    PRInt32 nsId = kNameSpaceID_None;
    if (!name.IsEmpty()) {
        if (mNamespace) {
            exprRes = mNamespace->evaluate(aEs.getEvalContext());
            NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

            nsAutoString nspace;
            exprRes->stringValue(nspace);
            delete exprRes;

            if (!nspace.IsEmpty()) {
#ifdef TX_EXE
                nsId = txNamespaceManager::getNamespaceID(nspace);
#else
                NS_ASSERTION(gTxNameSpaceManager, "No namespace manager");
                rv = gTxNameSpaceManager->RegisterNameSpace(nspace, nsId);
                NS_ENSURE_SUCCESS(rv, rv);
#endif
            }
        }
        else {
            nsCOMPtr<nsIAtom> prefix;
            XMLUtils::getPrefix(name, getter_AddRefs(prefix));
            nsId = mMappings.lookupNamespace(prefix);
            if (nsId == kNameSpaceID_Unknown) {
                // tunkate name to indicate failure
                name.Truncate();
            }
        }
    }

    if (!name.IsEmpty()) {
        // add element if everything was ok
        aEs.mResultHandler->startElement(name, nsId);
    }
    else {
        // we call characters with an empty string to "close" any element to
        // make sure that no attributes are added
        aEs.mResultHandler->characters(nsString(), PR_FALSE);
    }

    rv = aEs.pushString(name);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aEs.pushInt(nsId);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}


txStartLREElement::txStartLREElement(PRInt32 aNamespaceID,
                                     nsIAtom* aLocalName,
                                     nsIAtom* aPrefix)
    : mNamespaceID(aNamespaceID),
      mLocalName(aLocalName),
      mPrefix(aPrefix)
{
}

nsresult
txStartLREElement::execute(txExecutionState& aEs)
{
    // We should atomize the resulthandler
    nsAutoString nodeName;
    if (mPrefix) {
        mPrefix->ToString(nodeName);
        nsAutoString localName;
        nodeName.Append(PRUnichar(':'));
        mLocalName->ToString(localName);
        nodeName.Append(localName);
    }
    else {
        mLocalName->ToString(nodeName);
    }

    aEs.mResultHandler->startElement(nodeName, mNamespaceID);

    nsresult rv = aEs.pushString(nodeName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aEs.pushInt(mNamespaceID);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

txText::txText(const nsAString& aStr, PRBool aDOE)
    : mStr(aStr),
      mDOE(aDOE)
{
}

nsresult
txText::execute(txExecutionState& aEs)
{
    aEs.mResultHandler->characters(mStr, mDOE);
    return NS_OK;
}

txValueOf::txValueOf(Expr* aExpr, PRBool aDOE)
    : mExpr(aExpr),
      mDOE(aDOE)
{
}

txValueOf::~txValueOf()
{
    delete mExpr;
}

nsresult
txValueOf::execute(txExecutionState& aEs)
{
    ExprResult* exprRes = mExpr->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    nsAutoString value;
    exprRes->stringValue(value);
    delete exprRes;

    if (!value.IsEmpty()) {
        aEs.mResultHandler->characters(value, mDOE);
    }
    return NS_OK;
}
