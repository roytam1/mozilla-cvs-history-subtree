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

#include "nsRDFContentSink.h"
#include "nsIRDFContent.h"
#include "nsHTMLEntities.h" // XXX for NS_EntityToUnicode()
#include "nsIStyleSheet.h"
#include "nsIUnicharInputStream.h"
#include "nsIDocument.h"
#include "nsIXMLDocument.h"
#include "nsIXMLContent.h"
#include "nsIRDFDocument.h"
#include "nsIRDFContent.h"
#include "nsIScriptObjectOwner.h"
#include "nsIURL.h"
#include "nsIWebShell.h"
#include "nsIContent.h"
#include "nsITextContent.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIViewManager.h"
#include "nsIDOMComment.h"
#include "nsVoidArray.h"
#include "nsCRT.h"
#include "nsICSSParser.h"
#include "nsIScriptContext.h"
#include "nsIScriptContextOwner.h"
#include "prtime.h"
#include "prlog.h"
#include "prmem.h"

#define RDF_NAMESPACE_URI "http://www.w3.org/TR/WD-rdf-syntax#"

static const char kNameSpaceSeparator[] = ":";
static const char kNameSpaceDef[] = "xmlns";
static const char kRDFNameSpaceURI[] = RDF_NAMESPACE_URI;
static const char kRDFRDFTag[] = "RDF";
static const char kRDFDescriptionTag[] = "Description";
static const char  kRDFBag[] = RDF_NAMESPACE_URI "Bag";
static const char* kRDFBagTag = kRDFBag + sizeof(RDF_NAMESPACE_URI) - 1;
static const char  kRDFSeq[] = RDF_NAMESPACE_URI "Seq";
static const char* kRDFSeqTag = kRDFSeq + sizeof(RDF_NAMESPACE_URI) - 1;
static const char  kRDFAlt[] = RDF_NAMESPACE_URI "Alt";
static const char* kRDFAltTag = kRDFAlt + sizeof(RDF_NAMESPACE_URI) - 1;
static const char kRDFIDAttr[] = "ID";
static const char kRDFAboutAttr[] = "about";
static const char kRDFBagIDAttr[] = "bagID";
static const char kRDFAboutEachAttr[] = "aboutEach";
static const char kRDFResourceAttr[] = "resource";
static const char kRDFInstanceOf[] = RDF_NAMESPACE_URI "instanceOf";
static const char kStyleSheetPI[] = "<?xml-stylesheet";
static const char kCSSType[] = "text/css";
static const char kQuote = '\"';
static const char kApostrophe = '\'';

static NS_DEFINE_IID(kIContentSinkIID,         NS_ICONTENT_SINK_IID); // XXX grr...
static NS_DEFINE_IID(kIDOMCommentIID,          NS_IDOMCOMMENT_IID);
static NS_DEFINE_IID(kIRDFContainerContentIID, NS_IRDFCONTAINERCONTENT_IID);
static NS_DEFINE_IID(kIRDFContentSinkIID,      NS_IRDFCONTENTSINK_IID);
static NS_DEFINE_IID(kIRDFDocumentIID,         NS_IRDFDOCUMENT_IID);
static NS_DEFINE_IID(kIScrollableViewIID,      NS_ISCROLLABLEVIEW_IID);
static NS_DEFINE_IID(kISupportsIID,            NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIXMLContentSinkIID,      NS_IXMLCONTENT_SINK_IID);
static NS_DEFINE_IID(kIXMLDocumentIID,         NS_IXMLDOCUMENT_IID);

// XXX Open Issues:
// 1) factoring code with nsXMLContentSink - There's some amount of
//    common code between this and the HTML content sink. This will
//    increase as we support more and more HTML elements. How can code
//    from the code be factored?


////////////////////////////////////////////////////////////////////////
// Utility routines

static nsresult
rdf_GetQuotedAttributeValue(nsString& aSource, 
                        const nsString& aAttribute,
                        nsString& aValue)
{
    PRInt32 offset;
    PRInt32 endOffset = -1;
    nsresult result = NS_OK;

    offset = aSource.Find(aAttribute);
    if (-1 != offset) {
        offset = aSource.Find('=', offset);

        PRUnichar next = aSource.CharAt(++offset);
        if (kQuote == next) {
            endOffset = aSource.Find(kQuote, ++offset);
        }
        else if (kApostrophe == next) {
            endOffset = aSource.Find(kApostrophe, ++offset);	  
        }
  
        if (-1 != endOffset) {
            aSource.Mid(aValue, offset, endOffset-offset);
        }
        else {
            // Mismatched quotes - return an error
            result = NS_ERROR_FAILURE;
        }
    }
    else {
        aValue.Truncate();
    }

    return result;
}

// XXX Code copied from nsHTMLContentSink. It should be shared.
static void
rdf_ConvertEntityReferences(nsString& aResult)
{
    // Strip quotes if present
    PRUnichar first = aResult.First();
    if ((first == '"') || (first == '\'')) {
        if (aResult.Last() == first) {
            aResult.Cut(0, 1);
            PRInt32 pos = aResult.Length() - 1;
            if (pos >= 0) {
                aResult.Cut(pos, 1);
            }
        } else {
            // Mismatched quotes - leave them in
        }
    }

    // Reduce any entities
    // XXX Note: as coded today, this will only convert well formed
    // entities.  This may not be compatible enough.
    // XXX there is a table in navigator that translates some numeric entities
    // should we be doing that? If so then it needs to live in two places (bad)
    // so we should add a translate numeric entity method from the parser...
    char cbuf[100];
    PRInt32 index = 0;
    while (index < aResult.Length()) {
        // If we have the start of an entity (and it's not at the end of
        // our string) then translate the entity into it's unicode value.
        if ((aResult.CharAt(index++) == '&') && (index < aResult.Length())) {
            PRInt32 start = index - 1;
            PRUnichar e = aResult.CharAt(index);
            if (e == '#') {
                // Convert a numeric character reference
                index++;
                char* cp = cbuf;
                char* limit = cp + sizeof(cbuf) - 1;
                PRBool ok = PR_FALSE;
                PRInt32 slen = aResult.Length();
                while ((index < slen) && (cp < limit)) {
                    PRUnichar e = aResult.CharAt(index);
                    if (e == ';') {
                        index++;
                        ok = PR_TRUE;
                        break;
                    }
                    if ((e >= '0') && (e <= '9')) {
                        *cp++ = char(e);
                        index++;
                        continue;
                    }
                    break;
                }
                if (!ok || (cp == cbuf)) {
                    continue;
                }
                *cp = '\0';
                if (cp - cbuf > 5) {
                    continue;
                }
                PRInt32 ch = PRInt32( ::atoi(cbuf) );
                if (ch > 65535) {
                    continue;
                }

                // Remove entity from string and replace it with the integer
                // value.
                aResult.Cut(start, index - start);
                aResult.Insert(PRUnichar(ch), start);
                index = start + 1;
            }
            else if (((e >= 'A') && (e <= 'Z')) ||
                     ((e >= 'a') && (e <= 'z'))) {
                // Convert a named entity
                index++;
                char* cp = cbuf;
                char* limit = cp + sizeof(cbuf) - 1;
                *cp++ = char(e);
                PRBool ok = PR_FALSE;
                PRInt32 slen = aResult.Length();
                while ((index < slen) && (cp < limit)) {
                    PRUnichar e = aResult.CharAt(index);
                    if (e == ';') {
                        index++;
                        ok = PR_TRUE;
                        break;
                    }
                    if (((e >= '0') && (e <= '9')) ||
                        ((e >= 'A') && (e <= 'Z')) ||
                        ((e >= 'a') && (e <= 'z'))) {
                        *cp++ = char(e);
                        index++;
                        continue;
                    }
                    break;
                }
                if (!ok || (cp == cbuf)) {
                    continue;
                }
                *cp = '\0';
                PRInt32 ch = NS_EntityToUnicode(cbuf);
                if (ch < 0) {
                    continue;
                }

                // Remove entity from string and replace it with the integer
                // value.
                aResult.Cut(start, index - start);
                aResult.Insert(PRUnichar(ch), start);
                index = start + 1;
            }
            else if (e == '{') {
                // Convert a script entity
                // XXX write me!
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////
// Factory method

nsresult
NS_NewRDFContentSink(nsIRDFContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURL* aURL,
                     nsIWebShell* aWebShell)
{
    NS_PRECONDITION(nsnull != aResult, "null ptr");
    if (nsnull == aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    nsRDFContentSink* it;
    NS_NEWXPCOM(it, nsRDFContentSink);
    if (nsnull == it) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult rv = it->Init(aDoc, aURL, aWebShell);
    if (NS_OK != rv) {
        delete it;
        return rv;
    }
    return it->QueryInterface(kIRDFContentSinkIID, (void **)aResult);
}


////////////////////////////////////////////////////////////////////////


nsRDFContentSink::nsRDFContentSink()
{
    NS_INIT_REFCNT();
    mDocument = nsnull;
    mDocumentURL = nsnull;
    mWebShell = nsnull;
    mRootElement = nsnull;
    mNameSpaces = nsnull;
    mNestLevel = 0;
    mContextStack = nsnull;
    mText = nsnull;
    mTextLength = 0;
    mTextSize = 0;
    mConstrainSize = PR_TRUE;
}

nsRDFContentSink::~nsRDFContentSink()
{
    NS_IF_RELEASE(mDocument);
    NS_IF_RELEASE(mDocumentURL);
    NS_IF_RELEASE(mWebShell);
    NS_IF_RELEASE(mRootElement);
    if (mNameSpaces) {
        // There shouldn't be any here except in an error condition
        PRInt32 i, count = mNameSpaces->Count();
    
        for (i=0; i < count; i++) {
            NameSpaceStruct *ns = (NameSpaceStruct *)mNameSpaces->ElementAt(i);
      
            if (nsnull != ns) {
                NS_IF_RELEASE(ns->mPrefix);
                delete ns;
            }
        }
        delete mNameSpaces;
    }
    if (mContextStack) {
        NS_PRECONDITION(GetCurrentNestLevel() == 0, "content stack not empty");

        // XXX we should never need to do this, but, we'll write the
        // code all the same. If someone left the content stack dirty,
        // pop all the elements off the stack and release them.
        while (GetCurrentNestLevel() > 0) {
            nsIRDFContent* content;
            RDFContentSinkState state;
            PopContext(content, state);
            NS_IF_RELEASE(content);
        }

        delete mContextStack;
    }
    PR_FREEIF(mText);
}

////////////////////////////////////////////////////////////////////////

nsresult
nsRDFContentSink::Init(nsIDocument* aDoc,
                       nsIURL* aURL,
                       nsIWebShell* aContainer)
{
    NS_PRECONDITION(nsnull != aDoc, "null ptr");
    NS_PRECONDITION(nsnull != aURL, "null ptr");
    NS_PRECONDITION(nsnull != aContainer, "null ptr");
    if ((nsnull == aDoc) || (nsnull == aURL) || (nsnull == aContainer)) {
        return NS_ERROR_NULL_POINTER;
    }

    mDocument = aDoc;
    NS_ADDREF(aDoc);

    mDocumentURL = aURL;
    NS_ADDREF(aURL);
    mWebShell = aContainer;
    NS_ADDREF(aContainer);

    mState = eRDFContentSinkState_InProlog;

    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// nsISupports interface

NS_IMPL_ADDREF(nsRDFContentSink);
NS_IMPL_RELEASE(nsRDFContentSink);

NS_IMETHODIMP
nsRDFContentSink::QueryInterface(REFNSIID iid, void** result)
{
    NS_PRECONDITION(result, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = NULL;
    if (iid.Equals(kIRDFContentSinkIID) ||
        iid.Equals(kIXMLContentSinkIID) ||
        iid.Equals(kIContentSinkIID) ||
        iid.Equals(kISupportsIID)) {
        *result = static_cast<nsIRDFContentSink*>(this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////
// nsIContentSink interface

NS_IMETHODIMP 
nsRDFContentSink::WillBuildModel(void)
{
    // Notify document that the load is beginning
    mDocument->BeginLoad();
    nsresult result = NS_OK;

    return result;
}

NS_IMETHODIMP 
nsRDFContentSink::DidBuildModel(PRInt32 aQualityLevel)
{
    // XXX this is silly; who cares?
    PRInt32 i, ns = mDocument->GetNumberOfShells();
    for (i = 0; i < ns; i++) {
        nsIPresShell* shell = mDocument->GetShellAt(i);
        if (nsnull != shell) {
            nsIViewManager* vm = shell->GetViewManager();
            if(vm) {
                vm->SetQuality(nsContentQuality(aQualityLevel));
            }
            NS_RELEASE(vm);
            NS_RELEASE(shell);
        }
    }

    StartLayout();

    // XXX Should scroll to ref when that makes sense
    // ScrollToRef();

    mDocument->EndLoad();
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFContentSink::WillInterrupt(void)
{
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFContentSink::WillResume(void)
{
    return NS_OK;
}



NS_IMETHODIMP 
nsRDFContentSink::OpenContainer(const nsIParserNode& aNode)
{
    // XXX Hopefully the parser will flag this before we get here. If
    // we're in the epilog, there should be no new elements
    NS_PRECONDITION(mState != eRDFContentSinkState_InEpilog, "tag in RDF doc epilog");

    FlushText();

    // We must register namespace declarations found in the attribute
    // list of an element before creating the element. This is because
    // the namespace prefix for an element might be declared within
    // the attribute list.
    FindNameSpaceAttributes(aNode);

    nsIContent* content = NULL;
    nsresult result;

    RDFContentSinkState lastState = mState;
    switch (mState) {
    case eRDFContentSinkState_InProlog:
        result = OpenRDF(aNode);
        break;

    case eRDFContentSinkState_InDocumentElement:
        result = OpenObject(aNode);
        break;

    case eRDFContentSinkState_InDescriptionElement:
        result = OpenProperty(aNode);
        break;

    case eRDFContentSinkState_InContainerElement:
        result = OpenMember(aNode);
        break;

    case eRDFContentSinkState_InPropertyElement:
        result = OpenValue(aNode);
        break;

    case eRDFContentSinkState_InMemberElement:
        result = OpenValue(aNode);
        break;

    case eRDFContentSinkState_InEpilog:
        PR_ASSERT(0);
        result = NS_ERROR_UNEXPECTED; // XXX
        break;
    }

    return result;
}

NS_IMETHODIMP 
nsRDFContentSink::CloseContainer(const nsIParserNode& aNode)
{
    FlushText();

    nsIRDFContent* content;

    if (NS_FAILED(PopContext(content, mState))) {
        // XXX parser didn't catch unmatched tags?
        PR_ASSERT(0);
        return NS_ERROR_UNEXPECTED; // XXX
    }

    PRInt32 nestLevel = GetCurrentNestLevel();
    if (nestLevel == 0)
        mState = eRDFContentSinkState_InEpilog;

    CloseNameSpacesAtNestLevel(nestLevel);
      
    NS_RELEASE(content);
    return NS_OK;
}


NS_IMETHODIMP 
nsRDFContentSink::AddLeaf(const nsIParserNode& aNode)
{
    // XXX For now, all leaf content is character data
    AddCharacterData(aNode);
    return NS_OK;
}

NS_IMETHODIMP
nsRDFContentSink::NotifyError(nsresult aErrorResult)
{
    printf("nsRDFContentSink::NotifyError\n");
    return NS_OK;
}

// nsIXMLContentSink
NS_IMETHODIMP 
nsRDFContentSink::AddXMLDecl(const nsIParserNode& aNode)
{
    // XXX We'll ignore it for now
    printf("nsRDFContentSink::AddXMLDecl\n");
    return NS_OK;
}


NS_IMETHODIMP 
nsRDFContentSink::AddComment(const nsIParserNode& aNode)
{
    FlushText();
    nsAutoString text;
    //nsIDOMComment *domComment;
    nsresult result = NS_OK;

    text = aNode.GetText();

    // XXX add comment here...

    return result;
}


NS_IMETHODIMP 
nsRDFContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
    FlushText();

    // XXX For now, we don't add the PI to the content model.
    // We just check for a style sheet PI
    nsAutoString text, type, href;
    PRInt32 offset;
    nsresult result = NS_OK;

    text = aNode.GetText();

    offset = text.Find(kStyleSheetPI);
    // If it's a stylesheet PI...
    if (0 == offset) {
        result = rdf_GetQuotedAttributeValue(text, "href", href);
        // If there was an error or there's no href, we can't do
        // anything with this PI
        if ((NS_OK != result) || (0 == href.Length())) {
            return result;
        }
    
        result = rdf_GetQuotedAttributeValue(text, "type", type);
        if (NS_OK != result) {
            return result;
        }
    
        if (type.Equals(kCSSType)) {
            nsIURL* url = nsnull;
            nsIUnicharInputStream* uin = nsnull;
            nsAutoString absURL;
            nsIURL* docURL = mDocument->GetDocumentURL();
            nsAutoString emptyURL;
            emptyURL.Truncate();
            result = NS_MakeAbsoluteURL(docURL, emptyURL, href, absURL);
            if (NS_OK != result) {
                return result;
            }
            NS_RELEASE(docURL);
            result = NS_NewURL(&url, nsnull, absURL);
            if (NS_OK != result) {
                return result;
            }
            PRInt32 ec;
            nsIInputStream* iin = url->Open(&ec);
            if (nsnull == iin) {
                NS_RELEASE(url);
                return (nsresult) ec;/* XXX fix url->Open */
            }
            result = NS_NewConverterStream(&uin, nsnull, iin);
            NS_RELEASE(iin);
            if (NS_OK != result) {
                NS_RELEASE(url);
                return result;
            }
      
            result = LoadStyleSheet(url, uin, PR_FALSE);
            NS_RELEASE(uin);
            NS_RELEASE(url);
        }
    }

    return result;
}

NS_IMETHODIMP 
nsRDFContentSink::AddDocTypeDecl(const nsIParserNode& aNode)
{
    printf("nsRDFContentSink::AddDocTypeDecl\n");
    return NS_OK;
}


NS_IMETHODIMP 
nsRDFContentSink::AddCharacterData(const nsIParserNode& aNode)
{
    nsAutoString text = aNode.GetText();

    PRInt32 addLen = text.Length();
    if (0 == addLen) {
        return NS_OK;
    }

    // Create buffer when we first need it
    if (0 == mTextSize) {
        mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * 4096);
        if (nsnull == mText) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        mTextSize = 4096;
    }

    // Copy data from string into our buffer; flush buffer when it fills up
    PRInt32 offset = 0;
    while (0 != addLen) {
        PRInt32 amount = mTextSize - mTextLength;
        if (amount > addLen) {
            amount = addLen;
        }
        if (0 == amount) {
            if (mConstrainSize) {
                nsresult rv = FlushText();
                if (NS_OK != rv) {
                    return rv;
                }
            }
            else {
                mTextSize += addLen;
                mText = (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * mTextSize);
                if (nsnull == mText) {
                    return NS_ERROR_OUT_OF_MEMORY;
                }
            }
        }
        memcpy(&mText[mTextLength], text.GetUnicode() + offset,
               sizeof(PRUnichar) * amount);
        mTextLength += amount;
        offset += amount;
        addLen -= amount;
    }

    return NS_OK;
}

NS_IMETHODIMP 
nsRDFContentSink::AddUnparsedEntity(const nsIParserNode& aNode)
{
    printf("nsRDFContentSink::AddUnparsedEntity\n");
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFContentSink::AddNotation(const nsIParserNode& aNode)
{
    printf("nsRDFContentSink::AddNotation\n");
    return NS_OK;
}

NS_IMETHODIMP 
nsRDFContentSink::AddEntityReference(const nsIParserNode& aNode)
{
    printf("nsRDFContentSink::AddEntityReference\n");
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////
// Implementation methods

void
nsRDFContentSink::StartLayout()
{
    PRInt32 i, ns = mDocument->GetNumberOfShells();
    for (i = 0; i < ns; i++) {
        nsIPresShell* shell = mDocument->GetShellAt(i);
        if (nsnull != shell) {
            // Make shell an observer for next time
            shell->BeginObservingDocument();

            // Resize-reflow this time
            nsIPresContext* cx = shell->GetPresContext();
            nsRect r;
            cx->GetVisibleArea(r);
            shell->InitialReflow(r.width, r.height);
            NS_RELEASE(cx);

            // Now trigger a refresh
            nsIViewManager* vm = shell->GetViewManager();
            if (nsnull != vm) {
                vm->EnableRefresh();
                NS_RELEASE(vm);
            }

            NS_RELEASE(shell);
        }
    }

    // If the document we are loading has a reference or it is a top level
    // frameset document, disable the scroll bars on the views.
    const char* ref = mDocumentURL->GetRef();
    PRBool topLevelFrameset = PR_FALSE;
    if (mWebShell) {
        nsIWebShell* rootWebShell;
        mWebShell->GetRootWebShell(rootWebShell);
        if (mWebShell == rootWebShell) {
            topLevelFrameset = PR_TRUE;
        }
        NS_IF_RELEASE(rootWebShell);
    }

    if ((nsnull != ref) || topLevelFrameset) {
        // XXX support more than one presentation-shell here

        // Get initial scroll preference and save it away; disable the
        // scroll bars.
        PRInt32 i, ns = mDocument->GetNumberOfShells();
        for (i = 0; i < ns; i++) {
            nsIPresShell* shell = mDocument->GetShellAt(i);
            if (nsnull != shell) {
                nsIViewManager* vm = shell->GetViewManager();
                if (nsnull != vm) {
                    nsIView* rootView = nsnull;
                    vm->GetRootView(rootView);
                    if (nsnull != rootView) {
                        nsIScrollableView* sview = nsnull;
                        rootView->QueryInterface(kIScrollableViewIID, (void**) &sview);
                        if (nsnull != sview) {
                            if (topLevelFrameset)
                                mOriginalScrollPreference = nsScrollPreference_kNeverScroll;
                            else
                                sview->GetScrollPreference(mOriginalScrollPreference);
                            sview->SetScrollPreference(nsScrollPreference_kNeverScroll);
                        }
                    }
                    NS_RELEASE(vm);
                }
                NS_RELEASE(shell);
            }
        }
    }
}


// XXX Borrowed from HTMLContentSink. Should be shared.
nsresult
nsRDFContentSink::LoadStyleSheet(nsIURL* aURL,
                                 nsIUnicharInputStream* aUIN,
                                 PRBool aInline)
{
    /* XXX use repository */
    nsICSSParser* parser;
    nsresult rv = NS_NewCSSParser(&parser);
    if (NS_OK == rv) {
        if (aInline && (nsnull != mStyleSheet)) {
            parser->SetStyleSheet(mStyleSheet);
            // XXX we do probably need to trigger a style change reflow
            // when we are finished if this is adding data to the same sheet
        }
        nsIStyleSheet* sheet = nsnull;
        // XXX note: we are ignoring rv until the error code stuff in the
        // input routines is converted to use nsresult's
        parser->Parse(aUIN, aURL, sheet);
        if (nsnull != sheet) {
            if (aInline) {
                if (nsnull == mStyleSheet) {
                    // Add in the sheet the first time; if we update the sheet
                    // with new data (mutliple style tags in the same document)
                    // then the sheet will be updated by the css parser and
                    // therefore we don't need to add it to the document)
                    mDocument->AddStyleSheet(sheet);
                    mStyleSheet = sheet;
                }
            }
            else {
                mDocument->AddStyleSheet(sheet);
            }
            rv = NS_OK;
        } else {
            rv = NS_ERROR_OUT_OF_MEMORY;/* XXX */
        }
        NS_RELEASE(parser);
    }
    return rv;
}

////////////////////////////////////////////////////////////////////////
// Text buffering

static PRBool
rdf_IsDataInBuffer(PRUnichar* buffer, PRInt32 length)
{
    for (PRInt32 i = 0; i < length; ++i) {
        if (buffer[i] == ' ' ||
            buffer[i] == '\t' ||
            buffer[i] == '\n' ||
            buffer[i] == '\r')
            continue;

        return PR_TRUE;
    }
    return PR_FALSE;
}

static nsresult
rdf_Assert(nsIRDFContent* resource,
           const nsIRDFContent* property,
           const nsString& value)
{
    NS_PRECONDITION(resource && property, "null pointer");
    if (!resource || !property)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    nsAutoString propertyURI;
    if (NS_FAILED(rv = property->GetResource(propertyURI)))
        return rv;

    return resource->SetProperty(propertyURI, value);
}


nsresult
nsRDFContentSink::FlushText(PRBool aCreateTextNode, PRBool* aDidFlush)
{
    nsresult rv = NS_OK;
    PRBool didFlush = PR_FALSE;
    if (0 != mTextLength) {
        if (aCreateTextNode && rdf_IsDataInBuffer(mText, mTextLength)) {
            // XXX if there's anything but whitespace, then we'll
            // create a text node.

            switch (mState) {
            case eRDFContentSinkState_InMemberElement:
            case eRDFContentSinkState_InPropertyElement: {
                nsAutoString value;
                value.Append(mText, mTextLength);

                rv = rdf_Assert(GetContextElement(1),
                                GetContextElement(0),
                                value);

                } break;

            default:
                // just ignore it
                break;
            }
        }
        mTextLength = 0;
        didFlush = PR_TRUE;
    }
    if (nsnull != aDidFlush) {
        *aDidFlush = didFlush;
    }
    return rv;
}



////////////////////////////////////////////////////////////////////////
// Qualified name resolution

nsresult
nsRDFContentSink::SplitQualifiedName(const nsString& aQualifiedName,
                                     nsString& rNameSpaceURI,
                                     nsString& rProperty)
{
    rProperty = aQualifiedName;

    nsAutoString nameSpace;
    PRInt32 nsoffset = rProperty.Find(kNameSpaceSeparator);
    if (-1 != nsoffset) {
        rProperty.Left(nameSpace, nsoffset);
        rProperty.Cut(0, nsoffset+1);
    }
    else {
        nameSpace.Truncate(); // XXX isn't it empty already?
    }

    nsresult rv;
    nsIXMLDocument* xmlDoc;
    if (NS_FAILED(rv = mDocument->QueryInterface(kIXMLDocumentIID, (void**) &xmlDoc)))
        return rv;

    PRInt32 nameSpaceId = GetNameSpaceId(nameSpace);
    rv = xmlDoc->GetNameSpaceURI(nameSpaceId, rNameSpaceURI);
    NS_RELEASE(xmlDoc);

    return rv;
}


nsresult
nsRDFContentSink::GetDescriptionResource(const nsIParserNode& aNode,
                                         nsString& rResource)
{
    // Add tag attributes to the content attributes
    nsAutoString k;
    nsAutoString ns, attr;
    PRInt32 ac = aNode.GetAttributeCount();

    for (PRInt32 i = 0; i < ac; i++) {
        // Get upper-cased key
        const nsString& key = aNode.GetKeyAt(i);
        if (NS_FAILED(SplitQualifiedName(key, ns, attr)))
            continue;

        if (! ns.Equals(kRDFNameSpaceURI))
            continue;

        if (attr.Equals(kRDFAboutAttr) ||
            attr.Equals(kRDFIDAttr)) {
            // XXX you can't specify both, but we'll just pick up the
            // first thing that was specified and ignore the other.
            rResource = aNode.GetValueAt(i);
            rdf_ConvertEntityReferences(rResource);

            return NS_OK;
        }
    }

    // Otherwise, we couldn't find anything, so just gensym one...
    nsresult rv;
    nsIRDFDocument* rdfDoc;
    if (NS_FAILED(rv = mDocument->QueryInterface(kIRDFDocumentIID, (void**) &rdfDoc)))
        return rv;

    rdfDoc->GenerateAnonymousResource(rResource);

    NS_RELEASE(rdfDoc);
    return NS_OK;
}

nsresult
nsRDFContentSink::AddProperties(const nsIParserNode& aNode,
                                nsIRDFContent* aContent)
{
    // Add tag attributes to the content attributes
    nsAutoString k, v;
    nsAutoString ns, attr;
    PRInt32 ac = aNode.GetAttributeCount();

    for (PRInt32 i = 0; i < ac; i++) {
        // Get upper-cased key
        const nsString& key = aNode.GetKeyAt(i);
        if (NS_FAILED(SplitQualifiedName(key, ns, attr)))
            continue;

        // skip rdf:about and rdf:ID attributes; we already have them.
        if (ns.Equals(kRDFNameSpaceURI) &&
            (attr.Equals(kRDFAboutAttr) ||
             attr.Equals(kRDFIDAttr)))
            continue;

        v = aNode.GetValueAt(i);
        rdf_ConvertEntityReferences(v);

        k.Truncate();
        k.Append(ns);
        k.Append(attr);

        // Add attribute to content
        aContent->SetProperty(k, v);
    }
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////
// RDF-specific routines used to build the model

nsresult
nsRDFContentSink::OpenRDF(const nsIParserNode& aNode)
{
    // ensure that we're actually reading RDF by making sure that the
    // opening tag is <rdf:RDF>, where "rdf:" corresponds to whatever
    // they've declared the standard RDF namespace to be.
    nsAutoString ns, tag;
    
    if (NS_FAILED(SplitQualifiedName(aNode.GetText(), ns, tag)))
        return NS_ERROR_UNEXPECTED;

    if (! ns.Equals(kRDFNameSpaceURI))
        return NS_ERROR_UNEXPECTED;

    if (! tag.Equals(kRDFRDFTag))
        return NS_ERROR_UNEXPECTED;

    nsIRDFContent* rdfElement;
    if (NS_FAILED(NS_NewRDFElement(&rdfElement)))
        return NS_ERROR_UNEXPECTED;

    rdfElement->SetDocument(mDocument, PR_FALSE);

    // XXX this doesn't seem appropriate so, I'm not gonna do it.
    //mDocument->SetRootContent(rdfElement);

    PushContext(rdfElement, mState);
    mState = eRDFContentSinkState_InDocumentElement;
    return NS_OK;
}


nsresult
nsRDFContentSink::OpenObject(const nsIParserNode& aNode)
{
    nsresult rv;

    // an "object" non-terminal is either a "description", a "typed
    // node", or a "container", so this change the content sink's
    // state appropriately.
    nsAutoString ns, tag;

    if (NS_FAILED(rv = SplitQualifiedName(aNode.GetText(), ns, tag)))
        return rv;

    nsIRDFContent* rdfElement;

    if (ns.Equals(kRDFNameSpaceURI) &&
        (tag.Equals(kRDFBagTag) ||
         tag.Equals(kRDFSeqTag) ||
         tag.Equals(kRDFAltTag))) {
        // create a container element
        nsIRDFContainerContent* rdfContainerElement;
        if (NS_FAILED(rv = NS_NewRDFContainerElement(&rdfContainerElement)))
            return rv;

        rdfElement = static_cast<nsIRDFContent*>(rdfContainerElement);
    }
    else {
        // create a normal element
        if (NS_FAILED(rv = NS_NewRDFElement(&rdfElement)))
            return rv;
    }

    rdfElement->SetDocument(mDocument, PR_FALSE);

    // Arbitrarily make the document root be the first container
    // element in the RDF.
    if (! mRootElement) {
        mRootElement = rdfElement;
        NS_ADDREF(mRootElement);
        mDocument->SetRootContent(mRootElement);
    }

    nsAutoString uri;
    if (NS_FAILED(rv = GetDescriptionResource(aNode, uri)))
        return rv;

    if (NS_FAILED(rv = rdfElement->SetResource(uri)))
        return rv;

    // If we're in a member or property element, then this is the cue
    // that we need to hook the object up into the graph via the
    // member/property.
    switch (mState) {
    case eRDFContentSinkState_InMemberElement:
    case eRDFContentSinkState_InPropertyElement: {
        nsAutoString value;
        value.Append(mText, mTextLength);

        rv = rdf_Assert(GetContextElement(1),
                        GetContextElement(0),
                        uri);

        } break;

    default:
        break;
    }

    // Push the element onto the context stack
    PushContext(rdfElement, mState);

    // Now figure out what kind of state transition we need to
    // make. We'll either be going into a mode where we parse a
    // description or a container.
    PRBool isaTypedNode = PR_TRUE;

    if (ns.Equals(kRDFNameSpaceURI)) {
        isaTypedNode = PR_FALSE;

        if (tag.Equals(kRDFDescriptionTag)) {
            // it's a description
            mState = eRDFContentSinkState_InDescriptionElement;
        }
        else if (tag.Equals(kRDFBagTag)) {
            // it's a bag container
            rdfElement->SetProperty(kRDFInstanceOf, kRDFBag);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else if (tag.Equals(kRDFSeqTag)) {
            // it's a seq container
            rdfElement->SetProperty(kRDFInstanceOf, kRDFSeq);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else if (tag.Equals(kRDFAltTag)) {
            // it's an alt container
            rdfElement->SetProperty(kRDFInstanceOf, kRDFAlt);
            mState = eRDFContentSinkState_InContainerElement;
        }
        else {
            // heh, that's not *in* the RDF namespace: just treat it
            // like a typed node
            isaTypedNode = PR_TRUE;
        }
    }
    if (isaTypedNode) {
        // XXX destructively alter "ns" to contain the fully qualified
        // tag name. We can do this 'cause we don't need it anymore...
        ns.Append(tag);
        rdfElement->SetProperty(kRDFInstanceOf, ns);

        mState = eRDFContentSinkState_InDescriptionElement;
    }

    AddProperties(aNode, rdfElement);
    return NS_OK;
}


nsresult
nsRDFContentSink::OpenProperty(const nsIParserNode& aNode)
{
    nsresult rv;

    // an "object" non-terminal is either a "description", a "typed
    // node", or a "container", so this change the content sink's
    // state appropriately.
    nsAutoString ns, tag;

    if (NS_FAILED(rv = SplitQualifiedName(aNode.GetText(), ns, tag)))
        return rv;

    nsIRDFContent* rdfElement;
    if (NS_FAILED(rv = NS_NewRDFElement(&rdfElement)))
        return rv;

    rdfElement->SetDocument(mDocument, PR_FALSE);

    // XXX destructively alter "ns" to contain the fully qualified
    // tag name. We can do this 'cause we don't need it anymore...
    ns.Append(tag);
    if (NS_FAILED(rv = rdfElement->SetResource(ns)))
        return rv;

    // XXX I'm not sure what it means to property attributes on a
    // property (but you can!). I guess that I need to read the spec
    // more carefully to figure that out...
    //AddProperties(aNode, rdfElement);

    // Push the element onto the context stack and change state.
    PushContext(rdfElement, mState);
    mState = eRDFContentSinkState_InPropertyElement;

    return NS_OK;
}


nsresult
nsRDFContentSink::OpenMember(const nsIParserNode& aNode)
{
    nsIRDFContent* contextElement = GetContextElement(0);
    if (! contextElement)
        return NS_ERROR_NULL_POINTER;

    nsIRDFContainerContent* rdfContainer;
    if (NS_FAILED(contextElement->QueryInterface(kIRDFContainerContentIID,
                                                 (void**) &rdfContainer)))
        return NS_ERROR_UNEXPECTED;

    PRUint32 count;
    rdfContainer->GetElementCount(count);
    NS_RELEASE(rdfContainer);

    nsresult rv;
    nsIRDFContent* rdfElement;
    if (NS_FAILED(rv = NS_NewRDFElement(&rdfElement)))
        return rv;

    rdfElement->SetDocument(mDocument, PR_FALSE);

    nsAutoString uri(kRDFNameSpaceURI);
    uri.Append('_');
    uri.Append(count + 1, 10);

    rdfElement->SetResource(uri);

    // Push it on to the content stack and change state.
    PushContext(rdfElement, mState);
    mState = eRDFContentSinkState_InMemberElement;
    return NS_OK;
}

nsresult
nsRDFContentSink::OpenValue(const nsIParserNode& aNode)
{
    // a "value" can either be an object or a string: we'll only get
    // *here* if it's an object, as raw text is added as a leaf.
    return OpenObject(aNode);
}


////////////////////////////////////////////////////////////////////////
// Content stack management

struct RDFContextStackElement {
    nsIRDFContent*      mContent;
    RDFContentSinkState mState;
};

nsIRDFContent* 
nsRDFContentSink::GetContextElement(PRInt32 ancestor /* = 0 */)
{
    if ((nsnull == mContextStack) ||
        (ancestor >= mNestLevel)) {
        return nsnull;
    }

    RDFContextStackElement* e =
        static_cast<RDFContextStackElement*>(mContextStack->ElementAt(mNestLevel-ancestor-1));

    return e->mContent;
}

PRInt32 
nsRDFContentSink::PushContext(nsIRDFContent *aContent, RDFContentSinkState aState)
{
    if (! mContextStack) {
        mContextStack = new nsVoidArray();
        if (! mContextStack)
            return 0;
    }

    RDFContextStackElement* e = new RDFContextStackElement;
    if (! e)
        return mNestLevel;

    e->mContent = aContent;
    e->mState   = aState;
  
    mContextStack->AppendElement(static_cast<void*>(e));
    return ++mNestLevel;
}
 
nsresult
nsRDFContentSink::PopContext(nsIRDFContent*& rContent, RDFContentSinkState& rState)
{
    RDFContextStackElement* e;
    if ((nsnull == mContextStack) ||
        (0 == mNestLevel)) {
        return NS_ERROR_NULL_POINTER;
    }
  
    --mNestLevel;
    e = static_cast<RDFContextStackElement*>(mContextStack->ElementAt(mNestLevel));
    mContextStack->RemoveElementAt(mNestLevel);

    rContent = e->mContent;
    rState   = e->mState;

    delete e;
    return NS_OK;
}
 
PRInt32 
nsRDFContentSink::GetCurrentNestLevel()
{
    return mNestLevel;
}


////////////////////////////////////////////////////////////////////////
// Namespace management

void
nsRDFContentSink::FindNameSpaceAttributes(const nsIParserNode& aNode)
{
    nsAutoString k, uri, prefix;
    PRInt32 ac = aNode.GetAttributeCount();
    PRInt32 offset;
    nsresult result = NS_OK;

    for (PRInt32 i = 0; i < ac; i++) {
        const nsString& key = aNode.GetKeyAt(i);
        k.Truncate();
        k.Append(key);
        // Look for "xmlns" at the start of the attribute name
        offset = k.Find(kNameSpaceDef);
        if (0 == offset) {
            prefix.Truncate();

            PRUnichar next = k.CharAt(sizeof(kNameSpaceDef)-1);
            // If the next character is a :, there is a namespace prefix
            if (':' == next) {
                k.Right(prefix, k.Length()-sizeof(kNameSpaceDef));
            }

            // Get the attribute value (the URI for the namespace)
            uri = aNode.GetValueAt(i);
            rdf_ConvertEntityReferences(uri);
      
            // Open a local namespace
            OpenNameSpace(prefix, uri);
        }
    }
}

PRInt32 
nsRDFContentSink::OpenNameSpace(const nsString& aPrefix, const nsString& aURI)
{
    nsIAtom *nameSpaceAtom = nsnull;
    PRInt32 id = gNameSpaceId_Unknown;

    nsIXMLDocument *xmlDoc;
    nsresult result = mDocument->QueryInterface(kIXMLDocumentIID, 
                                                (void **)&xmlDoc);
    if (NS_OK != result)
        return id;

    if (0 < aPrefix.Length())
        nameSpaceAtom = NS_NewAtom(aPrefix);
  
    result = xmlDoc->RegisterNameSpace(nameSpaceAtom, aURI, id);
    if (NS_OK == result) {
        NameSpaceStruct *ns;
    
        ns = new NameSpaceStruct;
        if (nsnull != ns) {
            ns->mPrefix = nameSpaceAtom;
            NS_IF_ADDREF(nameSpaceAtom);
            ns->mId = id;
            ns->mNestLevel = GetCurrentNestLevel();
      
            if (nsnull == mNameSpaces)
                mNameSpaces = new nsVoidArray();

            // XXX Should check for duplication
            mNameSpaces->AppendElement((void *)ns);
        }
    }

    NS_IF_RELEASE(nameSpaceAtom);
    NS_RELEASE(xmlDoc);

    return id;
}

PRInt32 
nsRDFContentSink::GetNameSpaceId(const nsString& aPrefix)
{
    nsIAtom *nameSpaceAtom = nsnull;
    PRInt32 id = gNameSpaceId_Unknown;
    PRInt32 i, count;
  
    if (nsnull == mNameSpaces)
        return id;

    if (0 < aPrefix.Length())
        nameSpaceAtom = NS_NewAtom(aPrefix);

    count = mNameSpaces->Count();
    for (i = 0; i < count; i++) {
        NameSpaceStruct *ns = (NameSpaceStruct *)mNameSpaces->ElementAt(i);
    
        if ((nsnull != ns) && (ns->mPrefix == nameSpaceAtom)) {
            id = ns->mId;
            break;
        }
    }

    NS_IF_RELEASE(nameSpaceAtom);
    return id;
}

void    
nsRDFContentSink::CloseNameSpacesAtNestLevel(PRInt32 mNestLevel)
{
    PRInt32 nestLevel = GetCurrentNestLevel();

    if (nsnull == mNameSpaces) {
        return;
    }

    PRInt32 i, count;
    count = mNameSpaces->Count();
    // Go backwards so that we can delete as we go along
    for (i = count; i >= 0; i--) {
        NameSpaceStruct *ns = (NameSpaceStruct *)mNameSpaces->ElementAt(i);
    
        if ((nsnull != ns) && (ns->mNestLevel == nestLevel)) {
            NS_IF_RELEASE(ns->mPrefix);
            mNameSpaces->RemoveElementAt(i);
            delete ns;
        }
    }
}
