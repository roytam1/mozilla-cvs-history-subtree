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
#include "TxString.h"
#include "Expr.h"
#include "ExprResult.h"
#include "txStylesheet.h"

txStartLREElement::txStartLREElement(PRInt32 aNamespaceID,
                                     nsIAtom* aLocalName,
                                     nsIAtom* aPrefix)
    : mNamespaceID(aNamespaceID),
      mLocalName(aLocalName),
      mPrefix(aPrefix)
{
}

nsresult txStartLREElement::execute(txExecutionState& aEs)
{
    // We should atomize the resulthandler
    String nodeName;
    if (mPrefix) {
        mPrefix->ToString(nodeName);
        String localName;
        nodeName.append(':');
        mLocalName->ToString(localName);
        nodeName.append(localName);
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

nsresult txEndLREElement::execute(txExecutionState& aEs)
{
    PRInt32 namespaceID = aEs.popInt();
    String nodeName;
    aEs.popString(nodeName);

    aEs.mResultHandler->endElement(nodeName, namespaceID);

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

nsresult txLREAttribute::execute(txExecutionState& aEs)
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

    String value;
    exprRes->stringValue(value);
    delete exprRes;

    aEs.mResultHandler->attribute(String(nodeName), mNamespaceID, value);

    return NS_OK;
}

txInsertAttrSet::txInsertAttrSet(const txExpandedName& aName)
    : mName(aName)
{
}

nsresult txInsertAttrSet::execute(txExecutionState& aEs)
{
    txInstruction* instr = aEs.mStylesheet->getAttributeSet(mName);
    NS_ENSURE_TRUE(instr, NS_ERROR_XSLT_EXECUTION_FAILURE);

    nsresult rv = aEs.runTemplate(instr);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return NS_OK;
}

txTextInstruction::txTextInstruction(const String& aStr, PRBool aDOE)
    : mStr(aStr),
      mDOE(aDOE)
{
}

nsresult txTextInstruction::execute(txExecutionState& aEs)
{
    aEs.mResultHandler->characters(mStr, mDOE);
    return NS_OK;
}

txValueOfInstruction::txValueOfInstruction(Expr* aExpr, PRBool aDOE)
    : mExpr(aExpr),
      mDOE(aDOE)
{
}

txValueOfInstruction::~txValueOfInstruction()
{
    delete mExpr;
}

nsresult txValueOfInstruction::execute(txExecutionState& aEs)
{
    ExprResult* exprRes = mExpr->evaluate(aEs.getEvalContext());
    NS_ENSURE_TRUE(exprRes, NS_ERROR_FAILURE);

    String value;
    exprRes->stringValue(value);
    delete exprRes;

    aEs.mResultHandler->characters(value, mDOE);
}

txRecursionCheckpointStart::txRecursionCheckpointStart(const String& aName)
    : mName(aName)
{
}

nsresult txRecursionCheckpointStart::execute(txExecutionState& aEs)
{
    // XXX will this work? what if the context is in two different states
    return aEs.enterRecursionCheckpoint(this, aEs.getEvalContext());
}

nsresult txRecursionCheckpointEnd::execute(txExecutionState& aEs)
{
    aEs.leaveRecursionCheckpoint();
    return NS_OK;
}

