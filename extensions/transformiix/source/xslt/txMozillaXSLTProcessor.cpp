/* -*- Mode: IDL; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Peter Van der Beken <peterv@netscape.com> (original author)
 *
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

#include "txMozillaXSLTProcessor.h"
#include "nsIConsoleService.h"
#include "nsIContent.h"
#include "nsIDOMClassInfo.h"
#include "nsIScriptLoader.h"
#include "nsIServiceManagerUtils.h"
#include "nsITransformObserver.h"
#include "nsNetUtil.h"
#include "txMozillaTextOutput.h"
#include "txMozillaXMLOutput.h"
#include "txSingleNodeContext.h"
#include "txXMLEventHandler.h"

NS_IMPL_ADDREF(txMozillaXSLTProcessor)
NS_IMPL_RELEASE(txMozillaXSLTProcessor)
NS_INTERFACE_MAP_BEGIN(txMozillaXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY(nsIXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIScriptLoaderObserver)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(XSLTProcessor)
NS_INTERFACE_MAP_END

txMozillaXSLTProcessor::txMozillaXSLTProcessor() :
    mVariables(nsnull),
    mMozillaOutputHandler(nsnull)
{
    NS_INIT_ISUPPORTS();
}

txMozillaXSLTProcessor::~txMozillaXSLTProcessor()
{
    delete mVariables;
    delete mMozillaOutputHandler;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformDocument(nsIDOMNode* aSourceDOM,
                                          nsIDOMNode* aStyleDOM,
                                          nsIDOMDocument* aOutputDoc,
                                          nsITransformObserver* aObserver)
{
    // We need source and result documents but no stylesheet.
    NS_ENSURE_ARG(aSourceDOM);
    NS_ENSURE_ARG(aOutputDoc);

    delete mMozillaOutputHandler;
    mMozillaOutputHandler = nsnull;

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    aSourceDOM->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument) {
        sourceDOMDocument = do_QueryInterface(aSourceDOM);
    }
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(aSourceDOM);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    // Create wrapper for the style document.
    nsCOMPtr<nsIDOMDocument> styleDOMDocument;
    aStyleDOM->GetOwnerDocument(getter_AddRefs(styleDOMDocument));
    if (!styleDOMDocument) {
        styleDOMDocument = do_QueryInterface(aStyleDOM);
    }
    Document xslDocument(styleDOMDocument);

    // Create wrapper for the output document.
    mResultDocument = do_QueryInterface(aOutputDoc);
    NS_ENSURE_TRUE(mResultDocument, NS_ERROR_FAILURE);
    Document resultDocument(aOutputDoc);

    // Reset the output document.
    // Create a temporary channel to get nsIDocument->Reset to
    // do the right thing.
    nsCOMPtr<nsILoadGroup> loadGroup;
    nsCOMPtr<nsIChannel> channel;
    nsCOMPtr<nsIURI> docURL;
    nsCOMPtr<nsIDocument> source = do_QueryInterface(sourceDOMDocument);

    mResultDocument->GetDocumentURL(getter_AddRefs(docURL));
    if (!docURL && source) {
        source->GetDocumentURL(getter_AddRefs(docURL));
    }
    NS_ASSERTION(docURL, "No document URL");

    mResultDocument->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
    if (!loadGroup && source) {
        source->GetDocumentLoadGroup(getter_AddRefs(loadGroup));
    }

    nsresult rv = NS_NewChannel(getter_AddRefs(channel), docURL,
                                nsnull, loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    // Start of hack for keeping the scrollbars on an existing document.
    // Based on similar hack that jst wrote for document.open().
    // See bugs 78070 and 55334.
    nsCOMPtr<nsIContent> root;
    mResultDocument->GetRootContent(getter_AddRefs(root));
    if (root) {
        mResultDocument->SetRootContent(nsnull);
    }

    // Call Reset(), this will now do the full reset, except removing
    // the root from the document, doing that confuses the scrollbar
    // code in mozilla since the document in the root element and all
    // the anonymous content (i.e. scrollbar elements) is set to
    // null.
    rv = mResultDocument->Reset(channel, loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    if (root) {
        // Tear down the frames for the root element.
        mResultDocument->ContentRemoved(nsnull, root, 0);
    }
    // End of hack for keeping the scrollbars on an existing document.

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.
    {
        // Create a new ProcessorState
        ProcessorState ps(&sourceDocument, &xslDocument, &resultDocument);

        // XXX Need to add error observers

        // Set current txIEvalContext
        txSingleNodeContext evalContext(&sourceDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xslt elements
        nsCOMPtr<nsIDOMDocument> styleDoc = do_QueryInterface(aStyleDOM);
        if (styleDoc) {
            rv = processStylesheet(&sourceDocument, &xslDocument, &ps);
        }
        else {
            nsCOMPtr<nsIDOMElement> styleElem = do_QueryInterface(aStyleDOM);
            NS_ENSURE_TRUE(styleElem, NS_ERROR_FAILURE);
            Element* element = xslDocument.createElement(styleElem);
            NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
            rv = processTopLevel(&sourceDocument, element, &ps);
        }
        NS_ENSURE_SUCCESS(rv, rv);

        // Get the script loader of the result document.
        if (aObserver) {
            mResultDocument->GetScriptLoader(getter_AddRefs(mScriptLoader));
            if (mScriptLoader) {
                mScriptLoader->AddObserver(this);
            }
        }

        // Process root of XML source document
        transform(sourceNode, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.

    mMozillaOutputHandler->getRootContent(getter_AddRefs(root));
    if (root) {
        mResultDocument->ContentInserted(nsnull, root, 0);
    }

    mObserver = do_GetWeakReference(aObserver);
    SignalTransformEnd();

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::ScriptAvailable(nsresult aResult, 
                                        nsIDOMHTMLScriptElement *aElement, 
                                        PRBool aIsInline,
                                        PRBool aWasPending,
                                        nsIURI *aURI, 
                                        PRInt32 aLineNo,
                                        const nsAString& aScript)
{
    if (NS_FAILED(aResult) && mMozillaOutputHandler) {
        mMozillaOutputHandler->removeScriptElement(aElement);
        SignalTransformEnd();
    }

    return NS_OK;
}

NS_IMETHODIMP 
txMozillaXSLTProcessor::ScriptEvaluated(nsresult aResult, 
                                        nsIDOMHTMLScriptElement *aElement,
                                        PRBool aIsInline,
                                        PRBool aWasPending)
{
    if (mMozillaOutputHandler) {
        mMozillaOutputHandler->removeScriptElement(aElement);
        SignalTransformEnd();
    }

    return NS_OK;
}

void
txMozillaXSLTProcessor::SignalTransformEnd()
{
    nsCOMPtr<nsITransformObserver> observer = do_QueryReferent(mObserver);
    if (!observer) {
        return;
    }

    if (!mMozillaOutputHandler || !mMozillaOutputHandler->isDone()) {
        return;
    }

    if (mScriptLoader) {
        mScriptLoader->RemoveObserver(this);
        mScriptLoader = nsnull;
    }

    mObserver = nsnull;

    // XXX Need a better way to determine transform success/failure
    nsCOMPtr<nsIContent> rootContent;
    mMozillaOutputHandler->getRootContent(getter_AddRefs(rootContent));
    nsCOMPtr<nsIDOMNode> root = do_QueryInterface(rootContent);
    if (root) {
      nsCOMPtr<nsIDOMDocument> resultDoc;
      root->GetOwnerDocument(getter_AddRefs(resultDoc));
      observer->OnTransformDone(NS_OK, resultDoc);
    }
    else {
      // XXX Need better error message and code.
      observer->OnTransformDone(NS_ERROR_FAILURE, nsnull);
    }
}

NS_IMETHODIMP
txMozillaXSLTProcessor::Init(nsIDOMNode *aStyle)
{
    mStylesheet = aStyle;
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformNode(nsIDOMNode *aSource,
                                      nsIDOMDocument *aOutput,
                                      nsIDOMDocumentFragment **aResult)
{
    NS_ENSURE_ARG(aSource);
    NS_ENSURE_ARG(aOutput);
    NS_ENSURE_TRUE(mStylesheet, NS_ERROR_FAILURE);

    delete mMozillaOutputHandler;
    mMozillaOutputHandler = nsnull;

    // Create wrapper for the source document.
    nsCOMPtr<nsIDOMDocument> sourceDOMDocument;
    aSource->GetOwnerDocument(getter_AddRefs(sourceDOMDocument));
    if (!sourceDOMDocument) {
        sourceDOMDocument = do_QueryInterface(aSource);
    }
    NS_ENSURE_TRUE(sourceDOMDocument, NS_ERROR_FAILURE);
    Document sourceDocument(sourceDOMDocument);
    Node* sourceNode = sourceDocument.createWrapper(aSource);
    NS_ENSURE_TRUE(sourceNode, NS_ERROR_FAILURE);

    // Create wrapper for the style document.
    nsCOMPtr<nsIDOMDocument> styleDOMDocument;
    mStylesheet->GetOwnerDocument(getter_AddRefs(styleDOMDocument));
    if (!styleDOMDocument) {
        styleDOMDocument = do_QueryInterface(mStylesheet);
    }
    Document xslDocument(styleDOMDocument);

    // Create wrapper for the output document.
    mResultDocument = do_QueryInterface(aOutput);
    NS_ENSURE_TRUE(mResultDocument, NS_ERROR_FAILURE);
    Document resultDocument(aOutput);

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.
    {
        // Create a new ProcessorState
        ProcessorState ps(&sourceDocument, &xslDocument, &resultDocument);

        // XXX Need to add error observers

        // Set current txIEvalContext
        txSingleNodeContext evalContext(&sourceDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xslt elements
        nsCOMPtr<nsIDOMDocument> styleDoc = do_QueryInterface(mStylesheet);
        nsresult rv;
        if (styleDoc) {
            rv = processStylesheet(&sourceDocument, &xslDocument, &ps);
        }
        else {
            nsCOMPtr<nsIDOMElement> styleElem = do_QueryInterface(mStylesheet);
            NS_ENSURE_TRUE(styleElem, NS_ERROR_FAILURE);
            Element* element = xslDocument.createElement(styleElem);
            NS_ENSURE_TRUE(element, NS_ERROR_OUT_OF_MEMORY);
            rv = processTopLevel(&sourceDocument, element, &ps);
        }
        NS_ENSURE_SUCCESS(rv, rv);

        // Get the script loader of the result document.
        nsCOMPtr<nsIScriptLoader> loader;
        mResultDocument->GetScriptLoader(getter_AddRefs(loader));
        if (loader) {
            // Don't load scripts, we can't notify the caller when they're loaded.
            loader->Suspend();
        }

        if (mVariables) {
        }

        // Process root of XML source document
        transform(sourceNode, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.

    SignalTransformEnd();

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::SetParameter(const nsAString & aNamespaceURI,
                                     const nsAString & aLocalName,
                                     nsIVariant *aValue)
{
    if (!mVariables) {
        mVariables = new nsAutoVoidArray();
        NS_ENSURE_TRUE(mVariables, NS_ERROR_OUT_OF_MEMORY);
    }

    txVariable* var;
    PRInt32 index;
    PRInt32 count = mVariables->Count();
    for (index = 0; index < count; index++) {
        var = (txVariable*)mVariables->ElementAt(index);
        if (var->mNamespaceURI.Equals(aNamespaceURI) &&
            var->mLocalName.Equals(aLocalName)) {
            var->mValue = aValue;
            return NS_OK;
        }
    }

    var = new txVariable(aNamespaceURI, aLocalName, aValue);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    mVariables->AppendElement(var);
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::GetParameter(const nsAString& aNamespaceURI,
                                     const nsAString& aLocalName,
                                     nsIVariant **aResult)
{
    *aResult = nsnull;
    if (!mVariables) {
        return NS_OK;
    }

    txVariable* var;
    PRInt32 index;
    PRInt32 count = mVariables->Count();
    for (index = 0; index < count; index++) {
        var = (txVariable*)mVariables->ElementAt(index);
        if (var->mNamespaceURI.Equals(aNamespaceURI) &&
            var->mLocalName.Equals(aLocalName)) {
            *aResult = var->mValue;
            return NS_OK;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::RemoveParameter(const nsAString& aNamespaceURI,
                                        const nsAString& aLocalName)
{
    if (!mVariables) {
        return NS_OK;
    }

    txVariable* var;
    PRInt32 index;
    PRInt32 count = mVariables->Count();
    for (index = 0; index < count; index++) {
        var = (txVariable*)mVariables->ElementAt(index);
        if (var->mNamespaceURI.Equals(aNamespaceURI) &&
            var->mLocalName.Equals(aLocalName)) {
            if (mVariables->RemoveElementAt(index)) {
                delete var;
            }
            return NS_OK;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::ClearParameters()
{
    if (!mVariables) {
        return NS_OK;
    }

    txVariable* var;
    PRInt32 index;
    PRInt32 count = mVariables->Count();
    for (index = 0; index < count; index++) {
        var = (txVariable*)mVariables->ElementAt(index);
        if (mVariables->RemoveElementAt(index)) {
            delete var;
        }
    }
    return NS_OK;
}

txOutputXMLEventHandler*
txMozillaXSLTProcessor::getOutputHandler(txOutputMethod aMethod)
{
    if (mMozillaOutputHandler) {
        if (aMethod == eHTMLOutput || aMethod == eXMLOutput) {
            return mMozillaOutputHandler;
        }
        delete mMozillaOutputHandler;
        mMozillaOutputHandler = nsnull;
    }
    switch (aMethod) {
        case eMethodNotSet:
        case eXMLOutput:
        case eHTMLOutput:
        {
            mMozillaOutputHandler = new txMozillaXMLOutput();
            break;
        }
        case eTextOutput:
        {
            mMozillaOutputHandler = new txMozillaTextOutput();
            break;
        }
    }
    if (mMozillaOutputHandler) {
        nsCOMPtr<nsIDOMDocument> resultDocument =
            do_QueryInterface(mResultDocument);
        mMozillaOutputHandler->setOutputDocument(resultDocument);
    }
    return mMozillaOutputHandler;
}

void
txMozillaXSLTProcessor::logMessage(const String& aMessage)
{
    nsresult rv;
    nsCOMPtr<nsIConsoleService> consoleSvc = 
      do_GetService("@mozilla.org/consoleservice;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "xsl:message couldn't get console service");
    if (consoleSvc) {
        nsAutoString logString(NS_LITERAL_STRING("xsl:message - "));
        logString.Append(aMessage.getConstNSString());
        rv = consoleSvc->LogStringMessage(logString.get());
        NS_ASSERTION(NS_SUCCEEDED(rv), "xsl:message couldn't log");
    }
}
