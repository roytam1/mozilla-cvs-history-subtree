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

#ifndef TRANSFRMX_TXMOZILLAXSLTPROCESSOR_H
#define TRANSFRMX_TXMOZILLAXSLTPROCESSOR_H

#include "XSLTProcessor.h"
#include "nsIDocumentTransformer.h"
#include "nsIScriptLoaderObserver.h"
#include "nsIVariant.h"
#include "nsIXSLTProcessor.h"
#include "nsVoidArray.h"
#include "nsWeakPtr.h"

class txMozillaXMLEventHandler;

/* bacd8ad0-552f-11d3-a9f7-000064657374 */
#define TRANSFORMIIX_XSLT_PROCESSOR_CID   \
{ 0xbacd8ad0, 0x552f, 0x11d3, {0xa9, 0xf7, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74} }

#define TRANSFORMIIX_XSLT_PROCESSOR_CONTRACTID \
"@mozilla.org/document-transformer;1?type=text/xsl"

struct txVariable
{
public:
    txVariable(const nsAString & aNamespaceURI,
               const nsAString & aLocalName,
               nsIVariant *aValue) : mNamespaceURI(aNamespaceURI),
                                     mLocalName(aLocalName),
                                     mValue(aValue)
    {
    };
    ~txVariable()
    {
    };
    nsString mNamespaceURI;
    nsString mLocalName;
    nsCOMPtr<nsIVariant> mValue;
};

/*
 * txMozillaXSLTProcessor is a front-end to the XSLT Processor.
 */
class txMozillaXSLTProcessor : public txXSLTProcessor,
                               public nsIDocumentTransformer,
                               public nsIXSLTProcessor,
                               public nsIScriptLoaderObserver
{
public:
    /*
     * Creates a new txMozillaXSLTProcessor
     */
    txMozillaXSLTProcessor();

    /*
     * Default destructor for txMozillaXSLTProcessor
     */
    virtual ~txMozillaXSLTProcessor();

    // nsISupports interface
    NS_DECL_ISUPPORTS

    // nsIDocumentTransformer interface
    NS_DECL_NSIDOCUMENTTRANSFORMER

    // nsIXSLTProcessor interface
    NS_DECL_NSIXSLTPROCESSOR

    // nsIScriptLoaderObserver interface
    NS_DECL_NSISCRIPTLOADEROBSERVER

private:
    virtual txOutputXMLEventHandler* getOutputHandler(txOutputMethod aMethod);
    virtual void logMessage(const String& aMessage);

    void SignalTransformEnd();

    nsCOMPtr<nsIDocument> mResultDocument;
    nsCOMPtr<nsIDOMNode> mStylesheet;
    nsCOMPtr<nsIScriptLoader> mScriptLoader;
    nsWeakPtr mObserver;
    nsVoidArray* mVariables;
    txMozillaXMLEventHandler* mMozillaOutputHandler;
};

#endif
