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
#include "txVariableMap.h"
#include "txRtfHandler.h"
#include "txXSLTProcessor.h"
#include "TxLog.h"
#include "txURIUtils.h"
#include "txXMLParser.h"

#ifndef TX_EXE
#include "nsIDOMDocument.h"
#include "nsContentCID.h"
static NS_DEFINE_CID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);
#endif

DHASH_WRAPPER(txLoadedDocumentsBase, txLoadedDocumentEntry, nsAString&)

nsresult txLoadedDocumentsHash::init(Document* aSourceDocument)
{
    nsresult rv = Init(8);
    NS_ENSURE_SUCCESS(rv, rv);

    mSourceDocument = aSourceDocument;
    Add(mSourceDocument);

    return NS_OK;
}

txLoadedDocumentsHash::~txLoadedDocumentsHash()
{
    if (!mHashTable.ops) {
        return;
    }

    nsAutoString baseURI;
    mSourceDocument->getBaseURI(baseURI);
    txLoadedDocumentEntry* entry = GetEntry(baseURI);
    if (entry) {
        entry->mDocument = nsnull;
    }
}

void txLoadedDocumentsHash::Add(Document* aDocument)
{
    nsAutoString baseURI;
    aDocument->getBaseURI(baseURI);
    txLoadedDocumentEntry* entry = AddEntry(baseURI);
    if (entry) {
        entry->mDocument = aDocument;
    }
}

Document* txLoadedDocumentsHash::Get(const nsAString& aURI)
{
    txLoadedDocumentEntry* entry = GetEntry(aURI);
    if (entry) {
        return entry->mDocument;
    }

    return nsnull;
}

txExecutionState::txExecutionState(txStylesheet* aStylesheet)
    : mStylesheet(aStylesheet),
      mNextInstruction(nsnull),
      mLocalVariables(nsnull),
      mTemplateParams(nsnull),
      mEvalContext(nsnull),
      mInitialEvalContext(nsnull),
      mRTFDocument(nsnull),
      mKeyHash(aStylesheet->getKeyMap())
{
}

txExecutionState::~txExecutionState()
{
    delete mEvalContext;
    delete mRTFDocument;
    delete mTemplateParams;
    
    // XXX ToDo: delete stack of resulthandlers. This is messy since the 
    // the top resulthandler is the outputhandler, which is refcounted
    // in module and not in standalone.


    // XXX ToDo: delete evalcontext-stack. mind the initial evalcontext, it
    // can occur more then once in the evalcontext-stack. If we refcount them
    // this won't be a problem.
    
    txStackIterator paramIter(&mParamStack);
    while (paramIter.hasNext()) {
        delete (txExpandedNameMap*)paramIter.next();
    }
}

nsresult
txExecutionState::init(Node* aNode,
                       txExpandedNameMap* aGlobalParams)
{
    nsresult rv = NS_OK;

    // Set up initial context
    mEvalContext = new txSingleNodeContext(aNode, this);
    NS_ENSURE_TRUE(mEvalContext, NS_ERROR_OUT_OF_MEMORY);

    mInitialEvalContext = mEvalContext;

    // Set up output and result-handler
    txIOutputXMLEventHandler* handler = 0;
    rv = mOutputHandlerFactory->
        createHandlerWith(mStylesheet->getOutputFormat(), &handler);
    NS_ENSURE_SUCCESS(rv, rv);

    mOutputHandler = handler;
    mResultHandler = handler;
    mOutputHandler->startDocument();

    // Initiate first instruction
    txStylesheet::ImportFrame* frame = 0;
    txInstruction* templ = mStylesheet->findTemplate(aNode, txExpandedName(),
                                                     this, nsnull, &frame);
    rv = runTemplate(templ);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set up loaded-documents-hash
    Document* sourceDoc;
    if (aNode->getNodeType() == Node::DOCUMENT_NODE) {
        sourceDoc = (Document*)aNode;
    }
    else {
        sourceDoc = aNode->getOwnerDocument();
    }
    rv = mLoadedDocuments.init(sourceDoc);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    rv = mKeyHash.init();
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
    txExpandedName name(aNamespace, aLName);

    // look for a local variable
    aResult = mLocalVariables->getVariable(name);
    if (aResult) {
        return NS_OK;
    }

    // look for an evaluated global variable
    aResult = mGlobalVariableValues.getVariable(name);
    if (aResult) {
        return NS_OK;
    }

    // Is there perchance a global variable not evaluated yet?
    Expr* expr;
    txInstruction* instr;
    nsresult rv = mStylesheet->getGlobalVariable(name, expr, instr);
    if (NS_FAILED(rv)) {
        // XXX ErrorReport: variable doesn't exist in this scope
        return rv;
    }
    
    NS_ASSERTION(expr && !instr || !expr && instr,
                 "global variable should have either instruction or expression");

    // evaluate the global variable
    pushEvalContext(mInitialEvalContext);
    if (expr) {
        aResult = expr->evaluate(getEvalContext());
        NS_ENSURE_TRUE(aResult, NS_ERROR_FAILURE);
    }
    else {
        Document* rtfdoc;
        rv = getRTFDocument(&rtfdoc);
        NS_ENSURE_SUCCESS(rv, rv);

        txRtfHandler* handler = new txRtfHandler(rtfdoc);
        NS_ENSURE_TRUE(handler, NS_ERROR_OUT_OF_MEMORY);

        rv = pushResultHandler(handler);
        if (NS_FAILED(rv)) {
            delete handler;
            return rv;
        }

        txInstruction* prevInstr = mNextInstruction;
        // set return to nsnull to stop execution
        rv = runTemplate(instr, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        // XXX reset current-template-rule

        rv = txXSLTProcessor::execute(*this);
        NS_ENSURE_SUCCESS(rv, rv);

        mNextInstruction = prevInstr;
        popResultHandler();
        aResult = handler->mResultTreeFragment;
        handler->mResultTreeFragment = nsnull;
        delete handler;
    }
    popEvalContext();

    rv = mGlobalVariableValues.bindVariable(name, aResult, PR_TRUE);
    if (NS_FAILED(rv)) {
        delete aResult;
        aResult = nsnull;
        return rv;
    }

    return NS_OK;
}

PRBool
txExecutionState::isStripSpaceAllowed(Node* aNode)
{
    // XXX implement me
    return PR_FALSE;
}

void*
txExecutionState::getPrivateContext()
{
    return this;
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

nsresult
txExecutionState::getRTFDocument(Document** aDocument)
{
    *aDocument = nsnull;
    if (!mRTFDocument) {
#ifdef TX_EXE
        mRTFDocument = new Document();
        NS_ENSURE_TRUE(mRTFDocument, NS_ERROR_OUT_OF_MEMORY);
#else
        nsresult rv;
        nsCOMPtr<nsIDOMDocument> domDoc = do_CreateInstance(kXMLDocumentCID,
                                                            &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        mRTFDocument = new Document(domDoc);
        NS_ENSURE_TRUE(mRTFDocument, NS_ERROR_OUT_OF_MEMORY);
#endif
    }

    *aDocument = mRTFDocument;

    return NS_OK;
}

txExpandedNameMap*
txExecutionState::getParamMap()
{
    return mTemplateParams;
}

Node*
txExecutionState::retrieveDocument(const nsAString& uri,
                                   const nsAString& baseUri)
{
    nsAutoString absUrl;
    URIUtils::resolveHref(uri, baseUri, absUrl);

    PRInt32 hash = absUrl.RFindChar(PRUnichar('#'));
    PRUint32 urlEnd, fragStart, fragEnd;
    if (hash == kNotFound) {
        urlEnd = absUrl.Length();
        fragStart = 0;
        fragEnd = 0;
    }
    else {
        urlEnd = hash;
        fragStart = hash + 1;
        fragEnd = absUrl.Length();
    }

    nsDependentSubstring docUrl(absUrl, 0, urlEnd);
    nsDependentSubstring frag(absUrl, fragStart, fragEnd);

    PR_LOG(txLog::xslt, PR_LOG_DEBUG,
           ("Retrieve Document %s, uri %s, baseUri %s, fragment %s\n", 
            NS_LossyConvertUCS2toASCII(docUrl).get(),
            NS_LossyConvertUCS2toASCII(uri).get(),
            NS_LossyConvertUCS2toASCII(baseUri).get(),
            NS_LossyConvertUCS2toASCII(frag).get()));

    // try to get already loaded document
    Document* xmlDoc = mLoadedDocuments.Get(docUrl);

    if (!xmlDoc) {
        // open URI
        nsAutoString errMsg, refUri;
        // XXX we should get the referrer from the actual node
        // triggering the load, but this will do for the time being
        mLoadedDocuments.mSourceDocument->getBaseURI(refUri);
        nsresult rv;
        rv = txParseDocumentFromURI(docUrl, refUri,
                                    mLoadedDocuments.mSourceDocument, errMsg,
                                    &xmlDoc);

        if (NS_FAILED(rv) || !xmlDoc) {
            receiveError(NS_LITERAL_STRING("Couldn't load document '") +
                         docUrl + NS_LITERAL_STRING("': ") + errMsg,
                         NS_ERROR_XSLT_INVALID_URL);
            return NULL;
        }
        // add to list of documents
        mLoadedDocuments.Add(xmlDoc);
    }

    // return element with supplied id if supplied
    if (!frag.IsEmpty())
        return xmlDoc->getElementById(frag);

    return xmlDoc;
}

nsresult
txExecutionState::getKeyNodes(const txExpandedName& aKeyName,
                              Document* aDocument,
                              const nsAString& aKeyValue,
                              PRBool aIndexIfNotFound,
                              const NodeSet** aResult)
{
    return mKeyHash.getKeyNodes(aKeyName, aDocument, aKeyValue,
                                aIndexIfNotFound, *this, aResult);
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
txExecutionState::runTemplate(txInstruction* aTemplate)
{
    return runTemplate(aTemplate, mNextInstruction);
}

nsresult
txExecutionState::runTemplate(txInstruction* aTemplate,
                              txInstruction* aReturnTo)
{
    nsresult rv = mLocalVarsStack.push(mLocalVariables);
    NS_ENSURE_SUCCESS(rv, rv);

    mLocalVariables = new txVariableMap;
    NS_ENSURE_TRUE(mLocalVariables, NS_ERROR_OUT_OF_MEMORY);

    rv = mReturnStack.push(aReturnTo);
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
    NS_ASSERTION(!mReturnStack.isEmpty() && !mLocalVarsStack.isEmpty(),
                 "return or variable stack is empty");
    delete mLocalVariables;
    mNextInstruction = (txInstruction*)mReturnStack.pop();
    mLocalVariables = (txVariableMap*)mLocalVarsStack.pop();
}

nsresult
txExecutionState::bindVariable(const txExpandedName& aName,
                               ExprResult* aValue, MBool aOwned)
{
    return mLocalVariables->bindVariable(aName, aValue, aOwned);
}

void
txExecutionState::removeVariable(const txExpandedName& aName)
{
    mLocalVariables->removeVariable(aName);
}

nsresult
txExecutionState::pushParamMap()
{
    nsresult rv = mParamStack.push(mTemplateParams);
    NS_ENSURE_SUCCESS(rv, rv);

    mTemplateParams = nsnull;
    
    return NS_OK;
}

void
txExecutionState::popParamMap()
{
    delete mTemplateParams;
    mTemplateParams = (txExpandedNameMap*)mParamStack.pop();
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
