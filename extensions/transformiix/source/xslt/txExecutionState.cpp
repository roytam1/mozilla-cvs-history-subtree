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
    : mStylesheet(aStylesheet),
      mNextInstruction(nsnull)
{
}

txExecutionState::~txExecutionState()
{
    delete mEvalContext;
    
    // XXX ToDo: delete stack of resulthandlers. This is messy since the 
    // the top resulthandler is the outputhandler, which is refcounted
    // in module and not in standalone.


    // XXX ToDo: delete evalcontext-stack. mind the initial evalcontext, it
    // can occur more then once in the evalcontext-stack. If we refcount them
    // this won't be a problem.
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
    txInstruction* templ = mStylesheet->findTemplate(aNode, txExpandedName(),
                                                     this, nsnull, &frame);
    rv = runTemplate(templ);
    NS_ENSURE_SUCCESS(rv, rv);

    // XXX ToDo: set global parameters
    
    return NS_OK;
}

nsresult
txExecutionState::end()
{
    mOutputHandler->endDocument();
    
    return NS_OK;
}



nsresult
txExecutionState::getVariable(PRInt32 aNamespace, nsIAtom* aLName,
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
txExecutionState::receiveError(const nsAString& aMsg, nsresult aRes)
{
    // XXX implement me
}

nsresult
txExecutionState::pushEvalContext(txIEvalContext* aContext)
{
    nsresult rv = mEvalContextStack.push(mEvalContext);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mEvalContext = aContext;
    
    return NS_OK;
}

txIEvalContext*
txExecutionState::popEvalContext()
{
    txIEvalContext* prev = mEvalContext;
    mEvalContext = (txIEvalContext*)mEvalContextStack.pop();
    
    return prev;
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
    PRInt32 count = mStringStack.Count() - 1;
    NS_ASSERTION(count >= 0, "stack is empty");
    mStringStack.StringAt(count, aStr);
    mStringStack.RemoveStringAt(count);
}

nsresult
txExecutionState::pushInt(PRInt32 aInt)
{
    return mIntStack.push(NS_INT32_TO_PTR(aInt));
}

PRInt32
txExecutionState::popInt()
{
    return NS_PTR_TO_INT32(mIntStack.pop());
}

nsresult
txExecutionState::pushResultHandler(txXMLEventHandler* aHandler)
{
    nsresult rv = mResultHandlerStack.push(mResultHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mResultHandler = aHandler;

    return NS_OK;
}

txXMLEventHandler*
txExecutionState::popResultHandler()
{
    txXMLEventHandler* oldHandler = mResultHandler;
    mResultHandler = (txXMLEventHandler*)mResultHandlerStack.pop();

    return oldHandler;
}

txIEvalContext*
txExecutionState::getEvalContext()
{
    return mEvalContext;
}

txInstruction*
txExecutionState::getNextInstruction()
{
    txInstruction* instr = mNextInstruction;
    if (instr) {
        mNextInstruction = instr->mNext;
    }
    
    return instr;
}

nsresult
txExecutionState::runTemplate(txInstruction* aTemplate,
                              txInstruction* aReturnTo)
{
    // XXX ToDo: push local variables
    txInstruction* ret = aReturnTo ? aReturnTo : mNextInstruction;
    nsresult rv = mReturnStack.push(ret);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mNextInstruction = aTemplate;
    
    return NS_OK;
}

void
txExecutionState::gotoInstruction(txInstruction* aNext)
{
    mNextInstruction = aNext;
}

void
txExecutionState::returnFromTemplate()
{
    NS_ASSERTION(!mReturnStack.isEmpty(), "returnstack is empty");
    mNextInstruction = (txInstruction*)mReturnStack.pop();
    // XXX ToDo: pop local variables
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
