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
#include "nsContentCID.h"
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
#include "VariableBinding.h"
#include "XMLUtils.h"

static NS_DEFINE_CID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);

NS_IMPL_ADDREF(txMozillaXSLTProcessor)
NS_IMPL_RELEASE(txMozillaXSLTProcessor)
NS_INTERFACE_MAP_BEGIN(txMozillaXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY(nsIDocumentTransformer)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIXSLTProcessor)
    NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(XSLTProcessor)
NS_INTERFACE_MAP_END

txMozillaXSLTProcessor::txMozillaXSLTProcessor() : mVariables(nsnull)
{
    NS_INIT_ISUPPORTS();
}

txMozillaXSLTProcessor::~txMozillaXSLTProcessor()
{
    delete mVariables;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::TransformDocument(nsIDOMNode* aSourceDOM,
                                          nsIDOMNode* aStyleDOM,
                                          nsITransformObserver* aObserver,
                                          nsIDOMDocument** aOutputDoc)
{
    // We need source and result documents but no stylesheet.
    NS_ENSURE_ARG(aSourceDOM);
    NS_ENSURE_ARG(aOutputDoc);

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

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.
    {
        txMozillaHelper processorHelper(sourceDOMDocument, aObserver);

        // Create a new ProcessorState
        ProcessorState ps(&sourceDocument, &xslDocument, &processorHelper);

        // XXX Need to add error observers

        // Set current txIEvalContext
        txSingleNodeContext evalContext(&sourceDocument, &ps);
        ps.setEvalContext(&evalContext);

        // Index templates and process top level xslt elements
        nsCOMPtr<nsIDOMDocument> styleDoc = do_QueryInterface(aStyleDOM);
        nsresult rv;
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

        // Process root of XML source document
        transform(sourceNode, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.

    return NS_OK;
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
    nsCOMPtr<nsIDocument> document = do_QueryInterface(aOutput);
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);
    Document resultDocument(aOutput);

    // Start of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.
    {
        txMozillaHelper processorHelper(sourceDOMDocument, nsnull);

        // Create a new ProcessorState
        ProcessorState ps(&sourceDocument, &xslDocument, &processorHelper);

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
        document->GetScriptLoader(getter_AddRefs(loader));
        if (loader) {
            // Don't load scripts, we can't notify the caller when they're loaded.
            loader->Suspend();
        }

        if (mVariables) {
            txVariable* var;
            PRInt32 index;
            PRInt32 count = mVariables->Count();

            // XXX Change when sicking lands vars rewrite
            NamedMap* globalVars =
                (NamedMap*)ps.getVariableSetStack()->peek();

            nsCOMPtr<nsINameSpaceManager> namespaceManager;
            rv = document->GetNameSpaceManager(*getter_AddRefs(namespaceManager));
            NS_ENSURE_SUCCESS(rv, rv);

            for (index = 0; index < count; index++) {
                var = (txVariable*)mVariables->ElementAt(index);
                ExprResult* value = ConvertParameter(var->mValue);
                if (!value) {
                    // XXX Error? Signal to user?
                    continue;
                }
                PRInt32 nsId = kNameSpaceID_Unknown;
                rv = namespaceManager->GetNameSpaceID(var->mNamespaceURI, nsId);
                nsCOMPtr<nsIAtom> localName = do_GetAtom(var->mLocalName);
                txExpandedName name(nsId, localName);

                // XXX Change when sicking lands vars rewrite
                VariableBinding* binding = new VariableBinding(String(var->mLocalName), value);
                if (!binding) {
                    // XXX Error? Signal to user?
                    continue;
                }
                binding->allowShadowing();
                globalVars->put(String(var->mLocalName), binding);
            }
        }

        // Process root of XML source document
        transform(sourceNode, &ps);
    }
    // End of block to ensure the destruction of the ProcessorState
    // before the destruction of the documents.

    return NS_OK;
}

NS_IMETHODIMP
txMozillaXSLTProcessor::SetParameter(const nsAString & aNamespaceURI,
                                     const nsAString & aLocalName,
                                     nsIVariant *aValue)
{
    PRUint16 dataType;
    aValue->GetDataType(&dataType);
    switch (dataType) {
        // Number
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_INT16:
        case nsIDataType::VTYPE_INT32:
        case nsIDataType::VTYPE_INT64:
        case nsIDataType::VTYPE_UINT8:
        case nsIDataType::VTYPE_UINT16:
        case nsIDataType::VTYPE_UINT32:
        case nsIDataType::VTYPE_UINT64:
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE:

        // Boolean
        case nsIDataType::VTYPE_BOOL:

        // String
        case nsIDataType::VTYPE_CHAR:
        case nsIDataType::VTYPE_WCHAR:
        case nsIDataType::VTYPE_DOMSTRING:
        case nsIDataType::VTYPE_CHAR_STR:
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_STRING_SIZE_IS:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
        case nsIDataType::VTYPE_UTF8STRING:
        case nsIDataType::VTYPE_CSTRING:
        case nsIDataType::VTYPE_ASTRING:

        // Nodeset
        case nsIDataType::VTYPE_INTERFACE:
        case nsIDataType::VTYPE_INTERFACE_IS:
        case nsIDataType::VTYPE_ARRAY:
        {
            // This might still be an error, but we'll only
            // find out later since we lazily evaluate.
            break;
        }

        default:
        {
            return NS_ERROR_FAILURE;
        }        
    }

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

/* static*/
ExprResult*
txMozillaXSLTProcessor::ConvertParameter(nsIVariant *aValue)
{
    PRUint16 dataType;
    aValue->GetDataType(&dataType);
    switch (dataType) {
        // Number
        case nsIDataType::VTYPE_INT8:
        case nsIDataType::VTYPE_INT16:
        case nsIDataType::VTYPE_INT32:
        case nsIDataType::VTYPE_INT64:
        case nsIDataType::VTYPE_UINT8:
        case nsIDataType::VTYPE_UINT16:
        case nsIDataType::VTYPE_UINT32:
        case nsIDataType::VTYPE_UINT64:
        case nsIDataType::VTYPE_FLOAT:
        case nsIDataType::VTYPE_DOUBLE:
        {
            double value;
            nsresult rv = aValue->GetAsDouble(&value);
            NS_ENSURE_SUCCESS(rv, nsnull);
            return new NumberResult(value);
        }

        // Boolean
        case nsIDataType::VTYPE_BOOL:
        {
            PRBool value;
            nsresult rv = aValue->GetAsBool(&value);
            NS_ENSURE_SUCCESS(rv, nsnull);
            return new BooleanResult(value);
        }

        // String
        case nsIDataType::VTYPE_CHAR:
        case nsIDataType::VTYPE_WCHAR:
        case nsIDataType::VTYPE_DOMSTRING:
        case nsIDataType::VTYPE_CHAR_STR:
        case nsIDataType::VTYPE_WCHAR_STR:
        case nsIDataType::VTYPE_STRING_SIZE_IS:
        case nsIDataType::VTYPE_WSTRING_SIZE_IS:
        case nsIDataType::VTYPE_UTF8STRING:
        case nsIDataType::VTYPE_CSTRING:
        case nsIDataType::VTYPE_ASTRING:
        {
            String value;
            nsresult rv = aValue->GetAsAString(value.getNSString());
            NS_ENSURE_SUCCESS(rv, nsnull);
            return new StringResult(value);
        }

        // Nodeset
        case nsIDataType::VTYPE_INTERFACE:
        case nsIDataType::VTYPE_INTERFACE_IS:
        {
            nsID *iid;
            nsCOMPtr<nsISupports> supports;
            nsresult rv = aValue->GetAsInterface(&iid, getter_AddRefs(supports));
            NS_ENSURE_SUCCESS(rv, nsnull);
            if (iid) {
                // XXX Figure out what the user added and if we can do
                //     anything with it. Node, nsIDOMXPathResult
                nsMemory::Free(iid);
            }
            break;
        }

        case nsIDataType::VTYPE_ARRAY:
        {
            // XXX Figure out what the user added and if we can do
            //     anything with it. Array of Nodes. 
            break;
        }
    }
    return nsnull;
}

txMozillaHelper::txMozillaHelper(nsIDOMDocument* aSourceDocument,
                                 nsITransformObserver* aObserver) :
    mSourceDocument(aSourceDocument),
    mObserver(do_GetWeakReference(aObserver))
{
}

txMozillaHelper::~txMozillaHelper()
{
}

txOutputXMLEventHandler*
txMozillaHelper::getOutputHandler(txOutputMethod aMethod)
{
    if (mMozillaOutputHandler) {
        if (aMethod == eHTMLOutput || aMethod == eXMLOutput) {
            return mMozillaOutputHandler;
        }
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
        mMozillaOutputHandler->setSourceDocument(mSourceDocument);
        nsCOMPtr<nsITransformObserver> observer =
            do_QueryReferent(mObserver);
        mMozillaOutputHandler->setObserver(observer);
    }
    return mMozillaOutputHandler;
}

void
txMozillaHelper::logMessage(const String& aMessage)
{
    nsresult rv;
    nsCOMPtr<nsIConsoleService> consoleSvc = 
      do_GetService("@mozilla.org/consoleservice;1", &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "xsl:message couldn't get console service");
    if (consoleSvc) {
        nsAutoString logString(NS_LITERAL_STRING("xsl:message - "));
        logString.Append(aMessage.getConstNSString());
        rv = consoleSvc->LogStringMessage(logString.get());
        NS_ASSERTION(NS_SUCCEEDED(rv), "xsl:message couldn't log");
    }
}

Document*
txMozillaHelper::createRTFDocument(txOutputMethod aMethod)
{
    nsresult rv;
    nsCOMPtr<nsIDOMDocument> domDoc = do_CreateInstance(kXMLDocumentCID, &rv);
    NS_ENSURE_SUCCESS(rv, nsnull);
    return new Document(domDoc);
}
