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

#include "txExecutionState.h"
#include "txSingleNodeContext.h"
#include "txInstructions.h"
#include "txStylesheet.h"

txExecutionState::txExecutionState(txStylesheet* aStylesheet)
    : mStylesheet(aStylesheet)
{
}

txExecutionState::~txExecutionState()
{
    delete mEvalContext;
    
    // XXX mind the initial evalcontext, it can occur more then once in the
    // evalcontext-stack
}

nsresult
txExecutionState::init(Node* aNode,
                       txExpandedNameMap* aGlobalParams)
{
    nsresult rv = NS_OK;
    mEvalContext = new txSingleNodeContext(aNode, this);
    NS_ENSURE_TRUE(mEvalContext, NS_ERROR_OUT_OF_MEMORY);

    mInitialEvalContext = mEvalContext;


    txIOutputXMLEventHandler* handler = 0;
    rv = mOutputHandlerFactory->
        createHandlerWith(mStylesheet->getOutputFormat(), &handler);
    NS_ENSURE_SUCCESS(rv, rv);

    mOutputHandler = handler;
    mResultHandler = handler;
    mOutputHandler->startDocument();

    txStylesheet::ImportFrame* frame = 0;
    mNextInstruction = mStylesheet->findTemplate(aNode, txExpandedName(),
                                                 this, nsnull, &frame);
    rv = mReturnStack.push(0);
    NS_ENSURE_SUCCESS(rv, rv);

    // XXX set global parameters
    
    return NS_OK;
}

nsresult
txExecutionState::end()
{
    mOutputHandler->endDocument();
    
    return NS_OK;
}

nsresult
txExecutionState::getVariable(PRInt32 aNamespace, txAtom* aLName,
                              ExprResult*& aResult)
{
    // XXX implement me
    return NS_OK;
}

PRBool
txExecutionState::isStripSpaceAllowed(Node* aNode)
{
    // XXX implement me
    return PR_FALSE;
}

void
txExecutionState::receiveError(const String& aMsg, nsresult aRes)
{
    // XXX implement me
}

nsresult
txExecutionState::pushString(const nsAString& aStr)
{
    if (!mStringStack.AppendString(aStr)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    return NS_OK;
}


void
txExecutionState::popString(nsAString& aStr)
{
    mStringStack.StringAt(mStringStack.Count() - 1, aStr);
}

txIEvalContext*
txExecutionState::getEvalContext()
{
    return mEvalContext;
}

txInstruction*
txExecutionState::getNextInstruction()
{
    if (!mNextInstruction) {
        mNextInstruction = (txInstruction*)mReturnStack.pop();
        if (!mNextInstruction) {
            return nsnull;
        }
        // XXX pop local variables
    }
    
    txInstruction* instr = mNextInstruction;
    mNextInstruction = instr->mNext;
    
    return instr;
}

nsresult
txExecutionState::runTemplate(txInstruction* aInstruction)
{
    // XXX push local variables
    nsresult rv = mReturnStack.push(mNextInstruction);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mNextInstruction = aInstruction;
    
    return NS_OK;
}

void
txExecutionState::gotoInstruction(txInstruction* aNext)
{
    mNextInstruction = aNext;
}

nsresult
txExecutionState::enterRecursionCheckpoint(txRecursionCheckpointStart* aChk,
                                           txIEvalContext* aContext)
{
    PRInt32 i;
    for (i = 0; i < mRecursionInstructions.Count(); ++i) {
        if (mRecursionInstructions[i] == aChk &&
            mRecursionContexts[i] == aContext) {
            return NS_ERROR_XSLT_BAD_RECURSION;
        }
    }
    
    if (!mRecursionInstructions.AppendElement(aChk)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    if (!mRecursionContexts.AppendElement(aContext)) {
        mRecursionInstructions.RemoveElementAt(mRecursionInstructions.Count() - 1);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    return NS_OK;
}

void
txExecutionState::leaveRecursionCheckpoint()
{
    mRecursionInstructions.RemoveElementAt(mRecursionInstructions.Count() - 1);
    mRecursionContexts.RemoveElementAt(mRecursionContexts.Count() - 1);
}
