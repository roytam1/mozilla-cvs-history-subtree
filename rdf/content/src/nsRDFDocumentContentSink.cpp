/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

/*

  A content sink implementation that builds a feeds a content model
  via the nsIRDFDocument interface.

 */

#include "nsIContent.h"
#include "nsIDOMComment.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIPresContext.h"
#include "nsICSSStyleSheet.h"
#include "nsIRDFContent.h"
#include "nsIRDFDataBase.h"
#include "nsIRDFDocument.h"
#include "nsIRDFNode.h"
#include "nsIRDFService.h"
#include "nsIURL.h"
#include "nsIWebShell.h"
#include "nsLayoutCID.h"
#include "nsRDFContentSink.h"
#include "nsRDFContentUtils.h"
#include "nsINameSpaceManager.h"

////////////////////////////////////////////////////////////////////////

static NS_DEFINE_IID(kIDOMCommentIID,          NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIRDFContentSinkIID,      NS_IRDFCONTENTSINK_IID);
static NS_DEFINE_IID(kIRDFContentIID,          NS_IRDFCONTENT_IID);
static NS_DEFINE_IID(kIRDFDocumentIID,         NS_IRDFDOCUMENT_IID);
static NS_DEFINE_IID(kIScrollableViewIID,      NS_ISCROLLABLEVIEW_IID);


class nsRDFDocumentContentSink : public nsRDFContentSink
{
public:
    nsRDFDocumentContentSink(void);
    virtual ~nsRDFDocumentContentSink(void);

    virtual nsresult Init(nsIDocument* aDoc,
                          nsIURL* aURL);

    // nsIContentSink
    NS_IMETHOD WillBuildModel(void);
    NS_IMETHOD DidBuildModel(PRInt32 aQualityLevel);
    NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);

protected:
    virtual nsresult OpenObject(const nsIParserNode& aNode);

    // Style sheets
    nsresult LoadStyleSheet(nsIURL* aURL,
                            nsIUnicharInputStream* aUIN);

    nsresult StartLayout(void);

    nsIStyleSheet* mStyleSheet;

    // Document, webshell, etc.
    nsIDocument* mDocument;
    nsIContent*  mRootElement;
};

////////////////////////////////////////////////////////////////////////

nsRDFDocumentContentSink::nsRDFDocumentContentSink(void)
{
    mDocument = nsnull;
    mRootElement = nsnull;
}


nsRDFDocumentContentSink::~nsRDFDocumentContentSink(void)
{
    NS_IF_RELEASE(mDocument);
    NS_IF_RELEASE(mRootElement);
}


nsresult
nsRDFDocumentContentSink::Init(nsIDocument* aDoc,
                               nsIURL* aURL)
{
    NS_PRECONDITION(aDoc, "null ptr");
    if (!aDoc)
        return NS_ERROR_NULL_POINTER;

    nsINameSpaceManager* nameSpaceManager = nsnull;
    nsresult rv = aDoc->GetNameSpaceManager(nameSpaceManager);

    if (NS_SUCCEEDED(rv)) {
      rv = nsRDFContentSink::Init(aURL, nameSpaceManager);
      if (NS_SUCCEEDED(rv)) {
        mDocument = aDoc;
        NS_ADDREF(aDoc);
      }
      NS_RELEASE(nameSpaceManager);
    }

    return rv;
}




nsresult
nsRDFDocumentContentSink::StartLayout(void)
{
}


////////////////////////////////////////////////////////////////////////
// nsIContentSink interface

NS_IMETHODIMP 
nsRDFDocumentContentSink::WillBuildModel(void)
{
    mDocument->BeginLoad();
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFDocumentContentSink::DidBuildModel(PRInt32 aQualityLevel)
{
    //StartLayout();
    mDocument->EndLoad();
    return NS_OK;
}



NS_IMETHODIMP 
nsRDFDocumentContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
static const char kStyleSheetPI[] = "<?xml-stylesheet";
static const char kCSSType[] = "text/css";

static const char kDataSourcePI[] = "<?rdf-datasource";

    nsresult rv;
    if (NS_FAILED(rv = nsRDFContentSink::AddProcessingInstruction(aNode)))
        return rv;

    // XXX For now, we don't add the PI to the content model.
    // We just check for a style sheet PI
    const nsString& text = aNode.GetText();

    // If it's a stylesheet PI...
    if (0 == text.Find(kStyleSheetPI)) {
        nsAutoString href;
        rv = rdf_GetQuotedAttributeValue(text, "href", href);
        // If there was an error or there's no href, we can't do
        // anything with this PI
        if ((NS_OK != rv) || (0 == href.Length())) {
            return rv;
        }
    
        nsAutoString type;
        rv = rdf_GetQuotedAttributeValue(text, "type", type);
        if (NS_OK != rv) {
            return rv;
        }
    
        if (type.Equals(kCSSType)) {
            nsIURL* url = nsnull;
            nsAutoString absURL;
            nsIURL* docURL = mDocument->GetDocumentURL();
            nsAutoString emptyURL;
            emptyURL.Truncate();
            rv = NS_MakeAbsoluteURL(docURL, emptyURL, href, absURL);
            if (NS_OK != rv) {
                return rv;
            }
            NS_RELEASE(docURL);
            rv = NS_NewURL(&url, absURL);
            if (NS_OK != rv) {
                return rv;
            }
        }
    }
    else if (0 == text.Find(kDataSourcePI)) {
        nsAutoString href;
        rv = rdf_GetQuotedAttributeValue(text, "href", href);
        if (NS_FAILED(rv) || (0 == href.Length()))
            return rv;

        char uri[256];
        href.ToCString(uri, sizeof(uri));

        nsIRDFDataSource* ds;
        if (NS_SUCCEEDED(rv = mRDFService->GetNamedDataSource(uri, &ds))) {
            nsIRDFDocument* rdfDoc;
            if (NS_SUCCEEDED(mDocument->QueryInterface(kIRDFDocumentIID, (void**) &rdfDoc))) {
                nsIRDFDataBase* db;
                if (NS_SUCCEEDED(rv = rdfDoc->GetDataBase(db))) {
                    rv = db->AddDataSource(ds);
                    NS_RELEASE(db);
                }
                NS_RELEASE(rdfDoc);
            }
            NS_RELEASE(ds);
        }
    }

    return rv;
}


nsresult
nsRDFDocumentContentSink::OpenObject(const nsIParserNode& aNode)
{
    nsresult rv;

    if (NS_FAILED(rv = nsRDFContentSink::OpenObject(aNode)))
        return rv;

    // Arbitrarily make the document root be the first container
    // element in the serialized RDF/XML.
    if (! mRootElement) {
        nsAutoString uri;
        if (NS_FAILED(rv = GetIdAboutAttribute(aNode, uri)))
            return rv;

        nsIRDFResource* resource;
        if (NS_FAILED(rv = mRDFService->GetUnicodeResource(uri, &resource)))
            return rv;

        nsIRDFDocument* rdfDoc;
        if (NS_SUCCEEDED(rv = mDocument->QueryInterface(kIRDFDocumentIID, (void**) &rdfDoc))) {
            if (NS_SUCCEEDED(rv = rdfDoc->SetRootResource(resource))) {
                mRootElement = mDocument->GetRootContent();
            }
            NS_RELEASE(rdfDoc);
        }

        NS_RELEASE(resource);

        // Start layout. We need to wait until _now_ to ensure that we
        // actually have a root document element.
        StartLayout();

        // don't release the rdfElement since we're keeping
        // a reference to it in mRootElement
    }
    return rv;
}


////////////////////////////////////////////////////////////////////////

nsresult
NS_NewRDFDocumentContentSink(nsIRDFContentSink** aResult,
                             nsIDocument* aDoc,
                             nsIURL* aURL)
{
    NS_PRECONDITION(aResult, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsRDFDocumentContentSink* it;
    NS_NEWXPCOM(it, nsRDFDocumentContentSink);
    if (! it)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = it->Init(aDoc, aURL);
    if (NS_FAILED(rv)) {
        delete it;
        return rv;
    }
    return it->QueryInterface(kIRDFContentSinkIID, (void **)aResult);
}


