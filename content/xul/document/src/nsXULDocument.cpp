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

  An implementation for the XUL document. This implementation serves
  as the basis for generating an NGLayout content model.

  To Do
  -----

  1. Implement DOM range constructors.

  2. Implement XIF conversion (this is really low priority).

  Notes
  -----

  1. We do some monkey business in the document observer methods to
     keep the element map in sync for HTML elements. Why don't we just
     do it for _all_ elements? Well, in the case of XUL elements,
     which may be lazily created during frame construction, the
     document observer methods will never be called because we'll be
     adding the XUL nodes into the content model "quietly".

  2. The "element map" maps an RDF resource to the elements whose 'id'
     or 'ref' attributes refer to that resource. We re-use the element
     map to support the HTML-like 'getElementById()' method.

*/

// Note the ALPHABETICAL ORDERING
#include "nsXULDocument.h"

#include "nsDOMCID.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsIDOMStyleSheetCollection.h"
#include "nsIDOMText.h"
#include "nsIDOMXULElement.h"
#include "nsIDTD.h"
#include "nsIDocumentObserver.h"
#include "nsIHTMLContent.h"
#include "nsIHTMLElementFactory.h"
#include "nsINameSpace.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIContentViewer.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFContentModelBuilder.h"
#include "nsIRDFNode.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIRDFService.h"
#include "nsIScriptContextOwner.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIServiceManager.h"
#include "nsIStreamListener.h"
#include "nsIStyleContext.h"
#include "nsIStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsITextContent.h"
#include "nsIURL.h"
#include "nsNeckoUtil.h"
#include "nsILoadGroup.h"
#include "nsIWebShell.h"
#include "nsIXMLContent.h"
#include "nsIXMLElementFactory.h"
#include "nsIXULContentSink.h"
#include "nsIXULPrototypeCache.h"
#include "nsLayoutCID.h"
#include "nsParserCIID.h"
#include "nsRDFCID.h"
#include "nsIXULContentUtils.h"
#include "nsRDFDOMNodeList.h"
#include "nsXPIDLString.h" // XXX should go away
#include "nsXULElement.h"
#include "plhash.h"
#include "plstr.h"
#include "prlog.h"
#include "rdfutil.h"
#include "rdf.h"

#include "nsIFrameReflow.h"
#include "nsIBrowserWindow.h"
#include "nsIXULCommandDispatcher.h"
#include "nsIXULContent.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOMEventListener.h"

#include "nsLWBrkCIID.h"

#include "nsIInputStream.h"

//----------------------------------------------------------------------
//
// CIDs
//

static NS_DEFINE_CID(kEventListenerManagerCID,   NS_EVENTLISTENERMANAGER_CID);
static NS_DEFINE_CID(kCSSParserCID,              NS_CSSPARSER_CID);
static NS_DEFINE_CID(kDOMScriptObjectFactoryCID, NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kHTMLElementFactoryCID,     NS_HTML_ELEMENT_FACTORY_CID);
static NS_DEFINE_CID(kHTMLStyleSheetCID,         NS_HTMLSTYLESHEET_CID);
static NS_DEFINE_CID(kHTMLCSSStyleSheetCID,      NS_HTML_CSS_STYLESHEET_CID);
static NS_DEFINE_CID(kCSSLoaderCID,              NS_CSS_LOADER_CID);
static NS_DEFINE_CID(kNameSpaceManagerCID,       NS_NAMESPACEMANAGER_CID);
static NS_DEFINE_CID(kParserCID,                 NS_PARSER_IID); // XXX
static NS_DEFINE_CID(kPresShellCID,              NS_PRESSHELL_CID);
static NS_DEFINE_CID(kRDFCompositeDataSourceCID, NS_RDFCOMPOSITEDATASOURCE_CID);
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,  NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kLocalStoreCID,             NS_LOCALSTORE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,      NS_RDFCONTAINERUTILS_CID);
static NS_DEFINE_CID(kRDFServiceCID,             NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFXMLDataSourceCID,       NS_RDFXMLDATASOURCE_CID);
static NS_DEFINE_CID(kTextNodeCID,               NS_TEXTNODE_CID);
static NS_DEFINE_CID(kWellFormedDTDCID,          NS_WELLFORMEDDTD_CID);
static NS_DEFINE_CID(kXULContentSinkCID,         NS_XULCONTENTSINK_CID);
static NS_DEFINE_CID(kXULContentUtilsCID,        NS_XULCONTENTUTILS_CID);
static NS_DEFINE_CID(kXULCommandDispatcherCID,   NS_XULCOMMANDDISPATCHER_CID);
static NS_DEFINE_CID(kXULPrototypeCacheCID,      NS_XULPROTOTYPECACHE_CID);
static NS_DEFINE_CID(kLWBrkCID,                  NS_LWBRK_CID);

static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);

//----------------------------------------------------------------------
//
// Miscellaneous Constants
//

#define XUL_NAMESPACE_URI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"

const nsForwardReference::Priority nsForwardReference::kPasses[] = {
    nsForwardReference::ePriority_Construction,
    nsForwardReference::ePriority_Hookup,
    nsForwardReference::ePriority_Done
};


//----------------------------------------------------------------------
//
// Statics
//

PRInt32 nsXULDocument::gRefCnt = 0;

nsIAtom* nsXULDocument::kAttributeAtom;
nsIAtom* nsXULDocument::kCommandUpdaterAtom;
nsIAtom* nsXULDocument::kElementAtom;
nsIAtom* nsXULDocument::kIdAtom;
nsIAtom* nsXULDocument::kObservesAtom;
nsIAtom* nsXULDocument::kOpenAtom;
nsIAtom* nsXULDocument::kOverlayAtom;
nsIAtom* nsXULDocument::kPersistAtom;
nsIAtom* nsXULDocument::kPositionAtom;
nsIAtom* nsXULDocument::kRefAtom;
nsIAtom* nsXULDocument::kRuleAtom;
nsIAtom* nsXULDocument::kTemplateAtom;

nsIRDFService* nsXULDocument::gRDFService;
nsIRDFResource* nsXULDocument::kNC_persist;
nsIRDFResource* nsXULDocument::kNC_attribute;
nsIRDFResource* nsXULDocument::kNC_value;

nsIHTMLElementFactory* nsXULDocument::gHTMLElementFactory;

nsINameSpaceManager* nsXULDocument::gNameSpaceManager;
PRInt32 nsXULDocument::kNameSpaceID_XUL;

nsIXULContentUtils* nsXULDocument::gXULUtils;
nsIXULPrototypeCache* nsXULDocument::gXULPrototypeCache;

PRLogModuleInfo* nsXULDocument::gXULLog;


//----------------------------------------------------------------------
//
// ctors & dtors
//

nsXULDocument::nsXULDocument(void)
    : mParentDocument(nsnull),
      mScriptContextOwner(nsnull),
      mScriptObject(nsnull),
      mCharSetID("UTF-8"),
      mDisplaySelection(PR_FALSE),
      mContentViewerContainer(nsnull),
      mParentContentSink(nsnull),
      mIsPopup(PR_FALSE),
      mForwardReferencesResolved(PR_FALSE)
{
    NS_INIT_REFCNT();
}

nsXULDocument::~nsXULDocument()
{
    // In case we failed somewhere early on and the forward observer
    // decls never got resolved.
    DestroyForwardReferences();

    // mParentDocument is never refcounted
    // Delete references to sub-documents
    {
        PRInt32 i = mSubDocuments.Count();
        while (--i >= 0) {
            nsIDocument* subdoc = (nsIDocument*) mSubDocuments.ElementAt(i);
            NS_RELEASE(subdoc);
        }
    }

    // Delete references to style sheets but only if we aren't a popup document.
    if (!mIsPopup) {
        PRInt32 i = mStyleSheets.Count();
        while (--i >= 0) {
            nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(i);
            sheet->SetOwningDocument(nsnull);
            NS_RELEASE(sheet);
        }
    }

    // set all builder references to document to nsnull -- out of band notification
    // to break ownership cycle
    if (mBuilders)
    {
        PRUint32 cnt = 0;
        nsresult rv = mBuilders->Count(&cnt);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");

#ifdef	DEBUG
        printf("# of builders: %lu\n", (unsigned long)cnt);
#endif

        for (PRUint32 i = 0; i < cnt; ++i) {
          nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

          NS_ASSERTION(builder != nsnull, "null ptr");
          if (! builder) continue;

          rv = builder->SetDocument(nsnull);
          NS_ASSERTION(NS_SUCCEEDED(rv), "error unlinking builder from document");
          // XXX ignore error code?

          rv = builder->SetDataBase(nsnull);
          NS_ASSERTION(NS_SUCCEEDED(rv), "error unlinking builder from database");

          NS_RELEASE(builder);
        }
    }

    if (mLocalStore) {
        nsCOMPtr<nsIRDFRemoteDataSource> remote = do_QueryInterface(mLocalStore);
        if (remote)
            remote->Flush();
    }

    if (mCSSLoader) {
      mCSSLoader->DropDocumentReference();
    }

#if 0
    PRInt32 i;
    for (i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
        observer->DocumentWillBeDestroyed(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
#endif

    if (--gRefCnt == 0) {
        NS_IF_RELEASE(kAttributeAtom);
        NS_IF_RELEASE(kCommandUpdaterAtom);
        NS_IF_RELEASE(kElementAtom);
        NS_IF_RELEASE(kIdAtom);
        NS_IF_RELEASE(kObservesAtom);
        NS_IF_RELEASE(kOpenAtom);
        NS_IF_RELEASE(kOverlayAtom);
        NS_IF_RELEASE(kPersistAtom);
        NS_IF_RELEASE(kPositionAtom);
        NS_IF_RELEASE(kRefAtom);
        NS_IF_RELEASE(kRuleAtom);
        NS_IF_RELEASE(kTemplateAtom);

        if (gRDFService) {
            nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
            gRDFService = nsnull;
        }

        NS_IF_RELEASE(kNC_persist);
        NS_IF_RELEASE(kNC_attribute);
        NS_IF_RELEASE(kNC_value);

        NS_IF_RELEASE(gHTMLElementFactory);

        if (gXULUtils) {
            nsServiceManager::ReleaseService(kXULContentUtilsCID, gXULUtils);
            gXULUtils = nsnull;
        }

        if (gXULPrototypeCache) {
            nsServiceManager::ReleaseService(kXULPrototypeCacheCID, gXULPrototypeCache);
            gXULPrototypeCache = nsnull;
        }
    }
}


nsresult
NS_NewXULDocument(nsIXULDocument** result)
{
    NS_PRECONDITION(result != nsnull, "null ptr");
    if (! result)
        return NS_ERROR_NULL_POINTER;

    nsXULDocument* doc = new nsXULDocument();
    if (! doc)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(doc);

    nsresult rv;
    if (NS_FAILED(rv = doc->Init())) {
        NS_RELEASE(doc);
        return rv;
    }

    *result = doc;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsISupports interface
//

NS_IMETHODIMP 
nsXULDocument::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    *result = nsnull;
    if (iid.Equals(NS_GET_IID(nsIDocument)) ||
        iid.Equals(NS_GET_IID(nsISupports))) {
        *result = NS_STATIC_CAST(nsIDocument*, this);
    }
    else if (iid.Equals(nsIXULParentDocument::GetIID())) {
        *result = NS_STATIC_CAST(nsIXULParentDocument*, this);
    }
    else if (iid.Equals(nsIXULChildDocument::GetIID())) {
        *result = NS_STATIC_CAST(nsIXULChildDocument*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIXULDocument)) ||
             iid.Equals(NS_GET_IID(nsIXMLDocument))) {
        *result = NS_STATIC_CAST(nsIXULDocument*, this);
    }
    else if (iid.Equals(nsIDOMXULDocument::GetIID()) ||
             iid.Equals(nsIDOMDocument::GetIID()) ||
             iid.Equals(nsIDOMNode::GetIID())) {
        *result = NS_STATIC_CAST(nsIDOMXULDocument*, this);
    }
    else if (iid.Equals(nsIDOMNSDocument::GetIID())) {
        *result = NS_STATIC_CAST(nsIDOMNSDocument*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIJSScriptObject))) {
        *result = NS_STATIC_CAST(nsIJSScriptObject*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIScriptObjectOwner))) {
        *result = NS_STATIC_CAST(nsIScriptObjectOwner*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIHTMLContentContainer))) {
        *result = NS_STATIC_CAST(nsIHTMLContentContainer*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventReceiver))) {
        *result = NS_STATIC_CAST(nsIDOMEventReceiver*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventTarget))) {
        *result = NS_STATIC_CAST(nsIDOMEventTarget*, this);
    }
    else if (iid.Equals(NS_GET_IID(nsIDOMEventCapturer))) {
        *result = NS_STATIC_CAST(nsIDOMEventCapturer*, this);
    }
    else if (iid.Equals(nsIStreamLoadableDocument::GetIID())) {
    		*result = NS_STATIC_CAST(nsIStreamLoadableDocument*, this);
    }
    else {
        *result = nsnull;
        return NS_NOINTERFACE;
    }
    NS_ADDREF(this);
    return NS_OK;
}

NS_IMPL_ADDREF(nsXULDocument);
NS_IMPL_RELEASE(nsXULDocument);

//----------------------------------------------------------------------
//
// nsIDocument interface
//

nsIArena*
nsXULDocument::GetArena()
{
    nsIArena* result = mArena;
    NS_IF_ADDREF(result);
    return result;
}

NS_IMETHODIMP 
nsXULDocument::GetContentType(nsString& aContentType) const
{
    aContentType.SetString("text/xul");
    return NS_OK;
}

nsresult
nsXULDocument::PrepareToLoad(nsCOMPtr<nsIParser>* created_parser,
                               nsIContentViewerContainer* aContainer,
                               const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup)
{
    nsresult rv;

    // Get the document's URL
    if (aChannel) {
        rv = aChannel->GetURI(getter_AddRefs(mDocumentURL));
        if (NS_FAILED(rv)) return rv;
    }
    else {
        // If there is no channel, we'll dummy up a URL. This only
        // happens when we are called from LoadFromStream().
        static int unique_per_session_index = 0;

        nsAutoString seed;
        seed.Append("x-anonymous-xul://");
        seed.Append(PRInt32(++unique_per_session_index), /*base*/ 10);

        rv = NS_NewURI(getter_AddRefs(mDocumentURL), seed);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to synthesize URL for stream doc");
        if (NS_FAILED(rv)) return rv;
    }

    // Set the content viewer container. Note that we'll only hold a
    // weak reference to it.
    mContentViewerContainer = aContainer;

    mDocumentTitle.Truncate();

    // Get the document's principal
    nsCOMPtr<nsISupports> owner;
    rv = aChannel->GetOwner(getter_AddRefs(owner));
    if (NS_FAILED(rv)) return rv;

    mDocumentPrincipal = do_QueryInterface(owner);

    // Set the document's load group
    mDocumentLoadGroup = getter_AddRefs(NS_GetWeakReference(aLoadGroup));

    // Prepare the document's style sheets
    rv = PrepareStyleSheets(mDocumentURL);
    if (NS_FAILED(rv)) return rv;

    // Create a XUL content sink, a parser, and kick off the load.
    nsCOMPtr<nsIXULContentSink> sink;
    rv = nsComponentManager::CreateInstance(kXULContentSinkCID,
                                            nsnull,
                                            NS_GET_IID(nsIXULContentSink),
                                            getter_AddRefs(sink));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create XUL content sink");
    if (NS_FAILED(rv)) return rv;

    rv = sink->Init(this);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to initialize datasource sink");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIParser> parser;
    rv = nsComponentManager::CreateInstance(kParserCID,
                                            nsnull,
                                            kIParserIID,
                                            getter_AddRefs(parser));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create parser");
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIDTD> dtd;
    rv = nsComponentManager::CreateInstance(kWellFormedDTDCID,
                                            nsnull,
                                            NS_GET_IID(nsIDTD),
                                            getter_AddRefs(dtd));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to construct DTD");
    if (NS_FAILED(rv)) return rv;

    mCommand = aCommand;

    parser->RegisterDTD(dtd);
    parser->SetCommand(aCommand);
    parser->SetDocumentCharset(nsAutoString("UTF-8"), kCharsetFromDocTypeDefault);
    parser->SetContentSink(sink); // grabs a reference to the parser

    *created_parser = parser;

    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::PrepareStyleSheets(nsIURI* anURL)
{
    nsresult rv;
    
    // Delete references to style sheets - this should be done in superclass...
    PRInt32 i = mStyleSheets.Count();
    while (--i >= 0) {
        nsIStyleSheet* sheet = (nsIStyleSheet*) mStyleSheets.ElementAt(i);
        sheet->SetOwningDocument(nsnull);
        NS_RELEASE(sheet);
    }
    mStyleSheets.Clear();

    // Create an HTML style sheet for the HTML content.
    nsCOMPtr<nsIHTMLStyleSheet> sheet;
    if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kHTMLStyleSheetCID,
                                                       nsnull,
                                                       NS_GET_IID(nsIHTMLStyleSheet),
                                                       getter_AddRefs(sheet)))) {
        if (NS_SUCCEEDED(rv = sheet->Init(anURL, this))) {
            mAttrStyleSheet = sheet;
            AddStyleSheet(mAttrStyleSheet);
        }
    }

    if (NS_FAILED(rv)) {
        NS_ERROR("unable to add HTML style sheet");
        return rv;
    }

    // Create an inline style sheet for inline content that contains a style 
    // attribute.
    nsIHTMLCSSStyleSheet* inlineSheet;
    if (NS_SUCCEEDED(rv = nsComponentManager::CreateInstance(kHTMLCSSStyleSheetCID,
                                                       nsnull,
                                                       NS_GET_IID(nsIHTMLCSSStyleSheet),
                                                       (void**)&inlineSheet))) {
        if (NS_SUCCEEDED(rv = inlineSheet->Init(anURL, this))) {
            mInlineStyleSheet = dont_QueryInterface(inlineSheet);
            AddStyleSheet(mInlineStyleSheet);
        }
        NS_RELEASE(inlineSheet);
    }

    if (NS_FAILED(rv)) {
        NS_ERROR("unable to add inline style sheet");
        return rv;
    }

    return NS_OK;
}

void
nsXULDocument::SetDocumentURLAndGroup(nsIURI* anURL)
{
    mDocumentURL = dont_QueryInterface(anURL);
#ifdef NECKO
    // XXX help
#else
    anURL->GetLoadGroup(getter_AddRefs(mDocumentLoadGroup));
#endif
}

NS_IMETHODIMP 
nsXULDocument::StartDocumentLoad(const char* aCommand,
                                   nsIChannel* aChannel,
                                   nsILoadGroup* aLoadGroup,
                                   nsIContentViewerContainer* aContainer,
                                   nsIStreamListener **aDocListener)
{
    nsresult rv;

    nsCOMPtr<nsIURI> url;
    rv = aChannel->GetURI(getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIParser> parser;
    rv = PrepareToLoad(&parser, aContainer, aCommand, aChannel, aLoadGroup);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "parser doesn't support nsIStreamListener");
    if (NS_FAILED(rv)) return rv;

    *aDocListener = listener;
    NS_IF_ADDREF(*aDocListener);

    parser->Parse(url);
    return NS_OK;
}

const nsString*
nsXULDocument::GetDocumentTitle() const
{
    return &mDocumentTitle;
}

nsIURI* 
nsXULDocument::GetDocumentURL() const
{
    nsIURI* result = mDocumentURL;
    NS_IF_ADDREF(result);
    return result;
}

nsIPrincipal* 
nsXULDocument::GetDocumentPrincipal()
{
  if (!mDocumentPrincipal) {
    nsresult rv;
    NS_WITH_SERVICE(nsIScriptSecurityManager, securityManager,
                    NS_SCRIPTSECURITYMANAGER_PROGID, &rv);
    if (NS_FAILED(rv)) 
        return nsnull;
    if (NS_FAILED(securityManager->CreateCodebasePrincipal(mDocumentURL, 
                    getter_AddRefs(mDocumentPrincipal))))
    {
        return nsnull;
    }
  }
  nsIPrincipal *result = mDocumentPrincipal;
  NS_ADDREF(result);
  return result;
}


NS_IMETHODIMP
nsXULDocument::GetDocumentLoadGroup(nsILoadGroup **aGroup) const
{
    nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);

    *aGroup = group;
    NS_IF_ADDREF(*aGroup);
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::GetBaseURL(nsIURI*& aURL) const
{
    aURL = mDocumentURL;
    NS_IF_ADDREF(aURL);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetDocumentCharacterSet(nsString& oCharSetID) 
{
    oCharSetID = mCharSetID;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetDocumentCharacterSet(const nsString& aCharSetID)
{
    mCharSetID = aCharSetID;
    return NS_OK;
}


NS_IMETHODIMP 
nsXULDocument::GetLineBreaker(nsILineBreaker** aResult) 
{
  if(! mLineBreaker) {
     // no line breaker, find a default one
     nsILineBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          NS_GET_IID(nsILineBreakerFactory),
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsILineBreaker *lb = nsnull ;
      nsAutoString lbarg("");
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mLineBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mLineBreaker;
  NS_IF_ADDREF(*aResult);
  return NS_OK; // XXX we should do error handling here
}

NS_IMETHODIMP 
nsXULDocument::SetLineBreaker(nsILineBreaker* aLineBreaker) 
{
  mLineBreaker = dont_QueryInterface(aLineBreaker);
  return NS_OK;
}
NS_IMETHODIMP 
nsXULDocument::GetWordBreaker(nsIWordBreaker** aResult) 
{
  if (! mWordBreaker) {
     // no line breaker, find a default one
     nsIWordBreakerFactory *lf;
     nsresult result;
     result = nsServiceManager::GetService(kLWBrkCID,
                                          NS_GET_IID(nsIWordBreakerFactory),
                                          (nsISupports **)&lf);
     if (NS_SUCCEEDED(result)) {
      nsIWordBreaker *lb = nsnull ;
      nsAutoString lbarg("");
      result = lf->GetBreaker(lbarg, &lb);
      if(NS_SUCCEEDED(result)) {
         mWordBreaker = lb;
      }
      result = nsServiceManager::ReleaseService(kLWBrkCID, lf);
     }
  }
  *aResult = mWordBreaker;
  NS_IF_ADDREF(*aResult);
  return NS_OK; // XXX we should do error handling here
}

NS_IMETHODIMP 
nsXULDocument::SetWordBreaker(nsIWordBreaker* aWordBreaker) 
{
  mWordBreaker = dont_QueryInterface(aWordBreaker);
  return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetHeaderData(nsIAtom* aHeaderField, nsString& aData) const
{
  return NS_OK;
}

NS_IMETHODIMP
nsXULDocument:: SetHeaderData(nsIAtom* aheaderField, const nsString& aData)
{
  return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateShell(nsIPresContext* aContext,
                             nsIViewManager* aViewManager,
                             nsIStyleSet* aStyleSet,
                             nsIPresShell** aInstancePtrResult)
{
    NS_PRECONDITION(aInstancePtrResult, "null ptr");
    if (! aInstancePtrResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsIPresShell* shell;
    if (NS_FAILED(rv = nsComponentManager::CreateInstance(kPresShellCID,
                                                    nsnull,
                                                    NS_GET_IID(nsIPresShell),
                                                    (void**) &shell)))
        return rv;

    if (NS_FAILED(rv = shell->Init(this, aContext, aViewManager, aStyleSet))) {
        NS_RELEASE(shell);
        return rv;
    }

    mPresShells.AppendElement(shell);
    *aInstancePtrResult = shell; // addref implicit in CreateInstance()

    return NS_OK;
}

PRBool 
nsXULDocument::DeleteShell(nsIPresShell* aShell)
{
    return mPresShells.RemoveElement(aShell);
}

PRInt32 
nsXULDocument::GetNumberOfShells()
{
    return mPresShells.Count();
}

nsIPresShell* 
nsXULDocument::GetShellAt(PRInt32 aIndex)
{
    nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[aIndex]);
    NS_IF_ADDREF(shell);
    return shell;
}

nsIDocument* 
nsXULDocument::GetParentDocument()
{
    NS_IF_ADDREF(mParentDocument);
    return mParentDocument;
}

void 
nsXULDocument::SetParentDocument(nsIDocument* aParent)
{
    // Note that we do *not* AddRef our parent because that would
    // create a circular reference.
    mParentDocument = aParent;
}

void 
nsXULDocument::AddSubDocument(nsIDocument* aSubDoc)
{
    NS_ADDREF(aSubDoc);
    mSubDocuments.AppendElement(aSubDoc);
}

PRInt32 
nsXULDocument::GetNumberOfSubDocuments()
{
    return mSubDocuments.Count();
}

nsIDocument* 
nsXULDocument::GetSubDocumentAt(PRInt32 aIndex)
{
    nsIDocument* doc = (nsIDocument*) mSubDocuments.ElementAt(aIndex);
    if (nsnull != doc) {
        NS_ADDREF(doc);
    }
    return doc;
}

nsIContent* 
nsXULDocument::GetRootContent()
{
    nsIContent* result = mRootContent;
    NS_IF_ADDREF(result);
    return result;
}

void 
nsXULDocument::SetRootContent(nsIContent* aRoot)
{
    if (mRootContent) {
        mRootContent->SetDocument(nsnull, PR_TRUE);
    }
    mRootContent = aRoot;
    if (mRootContent) {
        mRootContent->SetDocument(this, PR_TRUE);
    }
}

NS_IMETHODIMP 
nsXULDocument::AppendToProlog(nsIContent* aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsXULDocument::AppendToEpilog(nsIContent* aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsXULDocument::ChildAt(PRInt32 aIndex, nsIContent*& aResult) const
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsXULDocument::IndexOf(nsIContent* aPossibleChild, PRInt32& aIndex) const
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsXULDocument::GetChildCount(PRInt32& aCount)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

PRInt32 
nsXULDocument::GetNumberOfStyleSheets()
{
    return mStyleSheets.Count();
}

nsIStyleSheet* 
nsXULDocument::GetStyleSheetAt(PRInt32 aIndex)
{
    nsIStyleSheet* sheet = NS_STATIC_CAST(nsIStyleSheet*, mStyleSheets[aIndex]);
    NS_IF_ADDREF(sheet);
    return sheet;
}

PRInt32 
nsXULDocument::GetIndexOfStyleSheet(nsIStyleSheet* aSheet)
{
  return mStyleSheets.IndexOf(aSheet);
}

void 
nsXULDocument::AddStyleSheet(nsIStyleSheet* aSheet)
{
    NS_PRECONDITION(aSheet, "null arg");
    if (!aSheet)
        return;

    if (aSheet == mAttrStyleSheet.get()) {  // always first
      mStyleSheets.InsertElementAt(aSheet, 0);
    }
    else if (aSheet == (nsIHTMLCSSStyleSheet*)mInlineStyleSheet) {  // always last
      mStyleSheets.AppendElement(aSheet);
    }
    else {
      if ((nsIHTMLCSSStyleSheet*)mInlineStyleSheet == mStyleSheets.ElementAt(mStyleSheets.Count() - 1)) {
        // keep attr sheet last
        mStyleSheets.InsertElementAt(aSheet, mStyleSheets.Count() - 1);
      }
      else {
        mStyleSheets.AppendElement(aSheet);
      }
    }
    NS_ADDREF(aSheet);

    aSheet->SetOwningDocument(this);

    PRBool enabled;
    aSheet->GetEnabled(enabled);

    if (enabled) {
        PRInt32 count, i;

        count = mPresShells.Count();
        for (i = 0; i < count; i++) {
            nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);
            nsCOMPtr<nsIStyleSet> set;
            shell->GetStyleSet(getter_AddRefs(set));
            if (set) {
                set->AddDocStyleSheet(aSheet, this);
            }
        }

        // XXX should observers be notified for disabled sheets??? I think not, but I could be wrong
        for (i = 0; i < mObservers.Count(); i++) {
            nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
            observer->StyleSheetAdded(this, aSheet);
            if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
              i--;
            }
        }
    }
}

NS_IMETHODIMP
nsXULDocument::InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex, PRBool aNotify)
{
  NS_PRECONDITION(nsnull != aSheet, "null ptr");
  mStyleSheets.InsertElementAt(aSheet, aIndex + 1); // offset by one for attribute sheet

  NS_ADDREF(aSheet);
  aSheet->SetOwningDocument(this);

  PRBool enabled = PR_TRUE;
  aSheet->GetEnabled(enabled);

  PRInt32 count;
  PRInt32 i;
  if (enabled) {
    count = mPresShells.Count();
    for (i = 0; i < count; i++) {
      nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(i);
      nsCOMPtr<nsIStyleSet> set;
      shell->GetStyleSet(getter_AddRefs(set));
      if (set) {
        set->AddDocStyleSheet(aSheet, this);
      }
    }
  }
  if (aNotify) {  // notify here even if disabled, there may have been others that weren't notified
    for (i = 0; i < mObservers.Count(); i++) {
      nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
      observer->StyleSheetAdded(this, aSheet);
      if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
        i--;
      }
    }
  }
  return NS_OK;
}

void 
nsXULDocument::SetStyleSheetDisabledState(nsIStyleSheet* aSheet,
                                          PRBool aDisabled)
{
    NS_PRECONDITION(nsnull != aSheet, "null arg");
    PRInt32 count;
    PRInt32 i;

    // If we're actually in the document style sheet list
    if (-1 != mStyleSheets.IndexOf((void *)aSheet)) {
        count = mPresShells.Count();
        for (i = 0; i < count; i++) {
            nsIPresShell* shell = (nsIPresShell*)mPresShells.ElementAt(i);
            nsCOMPtr<nsIStyleSet> set;
            shell->GetStyleSet(getter_AddRefs(set));
            if (set) {
                if (aDisabled) {
                    set->RemoveDocStyleSheet(aSheet);
                }
                else {
                    set->AddDocStyleSheet(aSheet, this);  // put it first
                }
            }
        }
    }  

    for (i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers.ElementAt(i);
        observer->StyleSheetDisabledStateChanged(this, aSheet, aDisabled);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
}

NS_IMETHODIMP
nsXULDocument::GetCSSLoader(nsICSSLoader*& aLoader)
{
  nsresult result = NS_OK;
  if (! mCSSLoader) {
    result = nsComponentManager::CreateInstance(kCSSLoaderCID,
                                                nsnull,
                                                NS_GET_IID(nsICSSLoader),
                                                (void**)&mCSSLoader);
    if (NS_SUCCEEDED(result)) {
      result = mCSSLoader->Init(this);
      mCSSLoader->SetCaseSensitive(PR_TRUE);
      mCSSLoader->SetQuirkMode(PR_FALSE); // no quirks in XUL
    }
  }
  aLoader = mCSSLoader;
  NS_IF_ADDREF(aLoader);
  return result;
}

nsIScriptContextOwner *
nsXULDocument::GetScriptContextOwner()
{
    NS_IF_ADDREF(mScriptContextOwner);
    return mScriptContextOwner;
}

void 
nsXULDocument::SetScriptContextOwner(nsIScriptContextOwner *aScriptContextOwner)
{
    // XXX HACK ALERT! If the script context owner is null, the document
    // will soon be going away. So tell our content that to lose its
    // reference to the document. This has to be done before we
    // actually set the script context owner to null so that the
    // content elements can remove references to their script objects.
    if (!aScriptContextOwner && mRootContent)
        mRootContent->SetDocument(nsnull, PR_TRUE);

    mScriptContextOwner = aScriptContextOwner;
}

NS_IMETHODIMP
nsXULDocument::GetNameSpaceManager(nsINameSpaceManager*& aManager)
{
  aManager = mNameSpaceManager;
  NS_IF_ADDREF(aManager);
  return NS_OK;
}


// Note: We don't hold a reference to the document observer; we assume
// that it has a live reference to the document.
void 
nsXULDocument::AddObserver(nsIDocumentObserver* aObserver)
{
    // XXX Make sure the observer isn't already in the list
    if (mObservers.IndexOf(aObserver) == -1) {
        mObservers.AppendElement(aObserver);
    }
}

PRBool 
nsXULDocument::RemoveObserver(nsIDocumentObserver* aObserver)
{
    return mObservers.RemoveElement(aObserver);
}

NS_IMETHODIMP 
nsXULDocument::BeginLoad()
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
        observer->BeginLoad(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::EndLoad()
{
    // Set up the document's composite datasource, which will include
    // the main document datasource and the local store. Only do this
    // if we are a bona-fide top-level XUL document; (mParentContentSink !=
    // nsnull) implies we are a XUL overlay.
    NS_PRECONDITION(mParentContentSink == nsnull, "trying to notify an overlay");
    if (mParentContentSink)
        return NS_ERROR_UNEXPECTED;

    nsresult rv;

    // Do any initial hookup that needs to happen.
    rv = ResolveForwardReferences();
    if (NS_FAILED(rv)) return rv;

    rv = ApplyPersistentAttributes();
    if (NS_FAILED(rv)) return rv;

    StartLayout();

    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver* observer = (nsIDocumentObserver*) mObservers[i];
        observer->EndLoad(this);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }

    return NS_OK;
}


NS_IMETHODIMP 
nsXULDocument::ContentChanged(nsIContent* aContent,
                              nsISupports* aSubContent)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentChanged(this, aContent, aSubContent);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ContentStatesChanged(nsIContent* aContent1, nsIContent* aContent2)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentStatesChanged(this, aContent1, aContent2);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::AttributeChanged(nsIContent* aElement,
                                  PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aHint)
{
    nsresult rv;

    PRInt32 nameSpaceID;
    rv = aElement->GetNameSpaceID(nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    // First see if we need to update our element map.
    if ((aAttribute == kIdAtom) || (aAttribute == kRefAtom)) {

        rv = mElementMap.Enumerate(RemoveElementsFromMapByContent, aElement);
        if (NS_FAILED(rv)) return rv;

        // That'll have removed _both_ the 'ref' and 'id' entries from
        // the map. So add 'em back now.
        rv = AddElementToMap(aElement);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->AttributeChanged(this, aElement, aNameSpaceID, aAttribute, aHint);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
            i--;
        }
    }

    // Handle "special" cases. We do this handling _after_ we've
    // notified the observer to ensure that any frames that are
    // caching information (e.g., the tree widget and the 'open'
    // attribute) will notice things properly.
    if ((nameSpaceID == kNameSpaceID_XUL) && (aAttribute == kOpenAtom)) {
        nsAutoString open;
        rv = aElement->GetAttribute(kNameSpaceID_None, kOpenAtom, open);
        if (NS_FAILED(rv)) return rv;

        if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && (open.Equals("true"))) {
            OpenWidgetItem(aElement);
        }
        else {
            CloseWidgetItem(aElement);
        }
    }
    else if (aAttribute == kRefAtom) {
        RebuildWidgetItem(aElement);
    }


    // Finally, see if there is anything we need to persist in the
    // localstore.
    //
    // XXX Namespace handling broken :-(
    nsAutoString persist;
    rv = aElement->GetAttribute(kNameSpaceID_None, kPersistAtom, persist);
    if (NS_FAILED(rv)) return rv;

    if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
        nsAutoString attr;
        rv = aAttribute->ToString(attr);
        if (NS_FAILED(rv)) return rv;

        if (persist.Find(attr) >= 0) {
            rv = Persist(aElement, kNameSpaceID_None, aAttribute);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ContentAppended(nsIContent* aContainer,
                                 PRInt32 aNewIndexInContainer)
{
    // First update our element map
    {
        nsresult rv;

        PRInt32 count;
        rv = aContainer->ChildCount(count);
        if (NS_FAILED(rv)) return rv;

        for (PRInt32 i = aNewIndexInContainer; i < count; ++i) {
            nsCOMPtr<nsIContent> child;
            rv = aContainer->ChildAt(i, *getter_AddRefs(child));
            if (NS_FAILED(rv)) return rv;

            rv = AddSubtreeToDocument(child);
            if (NS_FAILED(rv)) return rv;
        }
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentAppended(this, aContainer, aNewIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ContentInserted(nsIContent* aContainer,
                                 nsIContent* aChild,
                                 PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = AddSubtreeToDocument(aChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentInserted(this, aContainer, aChild, aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ContentReplaced(nsIContent* aContainer,
                                 nsIContent* aOldChild,
                                 nsIContent* aNewChild,
                                 PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = RemoveSubtreeFromDocument(aOldChild);
        if (NS_FAILED(rv)) return rv;

        rv = AddSubtreeToDocument(aNewChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentReplaced(this, aContainer, aOldChild, aNewChild,
                                  aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ContentRemoved(nsIContent* aContainer,
                                nsIContent* aChild,
                                PRInt32 aIndexInContainer)
{
    {
        nsresult rv;
        rv = RemoveSubtreeFromDocument(aChild);
        if (NS_FAILED(rv)) return rv;
    }

    // Now notify external observers
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->ContentRemoved(this, aContainer, 
                                 aChild, aIndexInContainer);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                  nsIStyleRule* aStyleRule,
                                  PRInt32 aHint)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleChanged(this, aStyleSheet, aStyleRule, aHint);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleAdded(this, aStyleSheet, aStyleRule);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                  nsIStyleRule* aStyleRule)
{
    for (PRInt32 i = 0; i < mObservers.Count(); i++) {
        nsIDocumentObserver*  observer = (nsIDocumentObserver*)mObservers[i];
        observer->StyleRuleRemoved(this, aStyleSheet, aStyleRule);
        if (observer != (nsIDocumentObserver*)mObservers.ElementAt(i)) {
          i--;
        }
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::GetSelection(nsIDOMSelection** aSelection)
{
    if (!mSelection) {
        PR_ASSERT(0);
        *aSelection = nsnull;
        return NS_ERROR_NOT_INITIALIZED;
    }
    *aSelection = mSelection;
    NS_ADDREF(*aSelection);
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::SelectAll()
{

    nsIContent * start = nsnull;
    nsIContent * end   = nsnull;
    nsIContent * body  = nsnull;

    nsAutoString bodyStr("BODY");
    PRInt32 i, n;
    mRootContent->ChildCount(n);
    for (i=0;i<n;i++) {
        nsIContent * child;
        mRootContent->ChildAt(i, child);
        PRBool isSynthetic;
        child->IsSynthetic(isSynthetic);
        if (!isSynthetic) {
            nsIAtom * atom;
            child->GetTag(atom);
            if (bodyStr.EqualsIgnoreCase(atom)) {
                body = child;
                break;
            }

        }
        NS_RELEASE(child);
    }

    if (body == nsnull) {
        return NS_ERROR_FAILURE;
    }

    start = body;
    // Find Very first Piece of Content
    for (;;) {
        start->ChildCount(n);
        if (n <= 0) {
            break;
        }
        nsIContent * child = start;
        child->ChildAt(0, start);
        NS_RELEASE(child);
    }

    end = body;
    // Last piece of Content
    for (;;) {
        end->ChildCount(n);
        if (n <= 0) {
            break;
        }
        nsIContent * child = end;
        child->ChildAt(n-1, end);
        NS_RELEASE(child);
    }

    //NS_RELEASE(start);
    //NS_RELEASE(end);

#if 0 // XXX nsSelectionRange is in another DLL
    nsSelectionRange * range    = mSelection->GetRange();
    nsSelectionPoint * startPnt = range->GetStartPoint();
    nsSelectionPoint * endPnt   = range->GetEndPoint();
    startPnt->SetPoint(start, -1, PR_TRUE);
    endPnt->SetPoint(end, -1, PR_FALSE);
#endif
    SetDisplaySelection(PR_TRUE);

    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::FindNext(const nsString &aSearchStr, PRBool aMatchCase, PRBool aSearchDown, PRBool &aIsFound)
{
    aIsFound = PR_FALSE;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP 
nsXULDocument::CreateXIF(nsString & aBuffer, nsIDOMSelection* aSelection)
{
    PR_ASSERT(0);
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::ToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
    return NS_OK;
}

void 
nsXULDocument::BeginConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

void 
nsXULDocument::ConvertChildrenToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

void 
nsXULDocument::FinishConvertToXIF(nsXIFConverter& aConverter, nsIDOMNode* aNode)
{
    PR_ASSERT(0);
}

PRBool 
nsXULDocument::IsInRange(const nsIContent *aStartContent, const nsIContent* aEndContent, const nsIContent* aContent) const
{
    PRBool  result;

    if (aStartContent == aEndContent) {
            return PRBool(aContent == aStartContent);
    }
    else if (aStartContent == aContent || aEndContent == aContent) {
        result = PR_TRUE;
    }
    else {
        result = IsBefore(aStartContent,aContent);
        if (result)
            result = IsBefore(aContent, aEndContent);
    }
    return result;
}

PRBool 
nsXULDocument::IsBefore(const nsIContent *aNewContent, const nsIContent* aCurrentContent) const
{
    PRBool result = PR_FALSE;

    if (nsnull != aNewContent && nsnull != aCurrentContent && aNewContent != aCurrentContent) {
        nsIContent* test = FindContent(mRootContent, aNewContent, aCurrentContent);
        if (test == aNewContent)
            result = PR_TRUE;

        NS_RELEASE(test);
    }
    return result;
}

PRBool 
nsXULDocument::IsInSelection(nsIDOMSelection* aSelection, const nsIContent *aContent) const
{
    PRBool  result = PR_FALSE;

    if (mSelection != nsnull) {
#if 0 // XXX can't include this because nsSelectionPoint is in another DLL.
        nsSelectionRange* range = mSelection->GetRange();
        if (range != nsnull) {
            nsSelectionPoint* startPoint = range->GetStartPoint();
            nsSelectionPoint* endPoint = range->GetEndPoint();

            nsIContent* startContent = startPoint->GetContent();
            nsIContent* endContent = endPoint->GetContent();
            result = IsInRange(startContent, endContent, aContent);
            NS_IF_RELEASE(startContent);
            NS_IF_RELEASE(endContent);
        }
#endif
    }
    return result;
}

nsIContent* 
nsXULDocument::GetPrevContent(const nsIContent *aContent) const
{
    nsIContent* result = nsnull;
 
    // Look at previous sibling

    if (nsnull != aContent) {
        nsIContent* parent; 
        aContent->GetParent(parent);

        if (parent && parent != mRootContent.get()) {
            PRInt32 i;
            parent->IndexOf((nsIContent*)aContent, i);
            if (i > 0)
                parent->ChildAt(i - 1, result);
            else
                result = GetPrevContent(parent);
        }
        NS_IF_RELEASE(parent);
    }
    return result;
}

nsIContent* 
nsXULDocument::GetNextContent(const nsIContent *aContent) const
{
    nsIContent* result = nsnull;
   
    if (nsnull != aContent) {
        // Look at next sibling
        nsIContent* parent;
        aContent->GetParent(parent);

        if (parent != nsnull && parent != mRootContent.get()) {
            PRInt32 i;
            parent->IndexOf((nsIContent*)aContent, i);

            PRInt32 count;
            parent->ChildCount(count);
            if (i + 1 < count) {
                parent->ChildAt(i + 1, result);
                // Get first child down the tree
                for (;;) {
                    PRInt32 n;
                    result->ChildCount(n);
                    if (n <= 0)
                        break;

                    nsIContent * old = result;
                    old->ChildAt(0, result);
                    NS_RELEASE(old);
                    result->ChildCount(n);
                }
            } else {
                result = GetNextContent(parent);
            }
        }
        NS_IF_RELEASE(parent);
    }
    return result;
}

void 
nsXULDocument::SetDisplaySelection(PRBool aToggle)
{
    mDisplaySelection = aToggle;
}

PRBool 
nsXULDocument::GetDisplaySelection() const
{
    return mDisplaySelection;
}

NS_IMETHODIMP 
nsXULDocument::HandleDOMEvent(nsIPresContext& aPresContext, 
                            nsEvent* aEvent, 
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus& aEventStatus)
{
  nsresult ret = NS_OK;
  nsIDOMEvent* domEvent = nsnull;

  if (NS_EVENT_FLAG_INIT == aFlags) {
    aDOMEvent = &domEvent;
    aEvent->flags = NS_EVENT_FLAG_NONE;
  }
  
  //Capturing stage
  if (NS_EVENT_FLAG_BUBBLE != aFlags && nsnull != mScriptContextOwner) {
    nsIScriptGlobalObject* global;
    if (NS_OK == mScriptContextOwner->GetScriptGlobalObject(&global)) {
      global->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_CAPTURE, aEventStatus);
      NS_RELEASE(global);
    }
  }
  
  //Local handling stage
  if (mListenerManager && !(aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH)) {
    aEvent->flags = aFlags;
    mListenerManager->HandleEvent(aPresContext, aEvent, aDOMEvent, aFlags, aEventStatus);
  }

  //Bubbling stage
  if (NS_EVENT_FLAG_CAPTURE != aFlags && nsnull != mScriptContextOwner) {
    nsIScriptGlobalObject* global;
    if (NS_OK == mScriptContextOwner->GetScriptGlobalObject(&global)) {
      global->HandleDOMEvent(aPresContext, aEvent, aDOMEvent, NS_EVENT_FLAG_BUBBLE, aEventStatus);
      NS_RELEASE(global);
    }
  }

  if (NS_EVENT_FLAG_INIT == aFlags) {
    // We're leaving the DOM event loop so if we created a DOM event, release here.
    if (nsnull != *aDOMEvent) {
      nsrefcnt rc;
      NS_RELEASE2(*aDOMEvent, rc);
      if (0 != rc) {
      //Okay, so someone in the DOM loop (a listener, JS object) still has a ref to the DOM Event but
      //the internal data hasn't been malloc'd.  Force a copy of the data here so the DOM Event is still valid.
        nsIPrivateDOMEvent *privateEvent;
        if (NS_OK == (*aDOMEvent)->QueryInterface(NS_GET_IID(nsIPrivateDOMEvent), (void**)&privateEvent)) {
          privateEvent->DuplicatePrivateData();
          NS_RELEASE(privateEvent);
        }
      }
    }
    aDOMEvent = nsnull;
  }

  return ret;
}


//----------------------------------------------------------------------
//
// nsIXMLDocument interface
//

NS_IMETHODIMP 
nsXULDocument::GetContentById(const nsString& aName, nsIContent** aContent)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef XSL
NS_IMETHODIMP 
nsXULDocument::SetTransformMediator(nsITransformMediator* aMediator)
{
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

//----------------------------------------------------------------------
//
// nsIXULDocument interface
//

NS_IMETHODIMP
nsXULDocument::AddElementForID(const nsString& aID, nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Add(aID, aElement);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::RemoveElementForID(const nsString& aID, nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Remove(aID, aElement);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetElementsForID(const nsString& aID, nsISupportsArray* aElements)
{
    NS_PRECONDITION(aElements != nsnull, "null ptr");
    if (! aElements)
        return NS_ERROR_NULL_POINTER;

    mElementMap.Find(aID, aElements);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateContents(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    PRUint32 cnt = 0;
    nsresult rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->CreateContents(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error creating content");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::AddContentModelBuilder(nsIRDFContentModelBuilder* aBuilder)
{
    NS_PRECONDITION(aBuilder != nsnull, "null ptr");
    if (! aBuilder)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    if (! mBuilders) {
        rv = NS_NewISupportsArray(getter_AddRefs(mBuilders));
        if (NS_FAILED(rv)) return rv;
    }

    rv = aBuilder->SetDocument(this);
    if (NS_FAILED(rv)) return rv;

    return mBuilders->AppendElement(aBuilder) ? NS_OK : NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsXULDocument::GetForm(nsIDOMHTMLFormElement** aForm)
{
  *aForm = mHiddenForm;
  NS_IF_ADDREF(*aForm);
  return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::SetForm(nsIDOMHTMLFormElement* aForm)
{
    mHiddenForm = dont_QueryInterface(aForm);

    // Set the document.
    nsCOMPtr<nsIContent> formContent = do_QueryInterface(aForm);
    formContent->SetDocument(this, PR_TRUE);

    // Forms are containers, and as such take up a bit of space.
    // Set a style attribute to keep the hidden form from showing up.
    mHiddenForm->SetAttribute("style", "margin:0em");
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::AddForwardReference(nsForwardReference* aRef)
{
    if (! mForwardReferencesResolved) {
        mForwardReferences.AppendElement(aRef);
    }
    else {
        NS_ERROR("forward references have already been resolved");
        delete aRef;
    }
        
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::ResolveForwardReferences()
{
    if (mForwardReferencesResolved)
        return NS_OK;

    // So we're monotonic. This prevents a forward reference from
    // adding _yet another_ forward reference, which could cause the
    // 'annealing' process to diverge.
    mForwardReferencesResolved = PR_TRUE;

    // Resolve each outstanding 'forward' reference. We iterate
    // through the list of forward references until no more forward
    // references can be resolved. This annealing process is
    // guaranteed to converge because we've "closed the gate" to new
    // forward references.

    for (const nsForwardReference::Priority* pass = nsForwardReference::kPasses;
         *pass != nsForwardReference::ePriority_Done;
         ++pass) {
        PRInt32 previous = 0;
        while (mForwardReferences.Count() && mForwardReferences.Count() != previous) {
            previous = mForwardReferences.Count();

            for (PRInt32 i = 0; i < mForwardReferences.Count(); ++i) {
                nsForwardReference* fwdref = NS_REINTERPRET_CAST(nsForwardReference*, mForwardReferences[i]);

                if (fwdref->GetPriority() != *pass)
                    continue;

                nsForwardReference::Result result = fwdref->Resolve();

                switch (result) {
                case nsForwardReference::eResolve_Succeeded:
                case nsForwardReference::eResolve_Error:
                    mForwardReferences.RemoveElementAt(i);
                    delete fwdref;

                    // fixup because we removed from list
                    --i;
                    break;

                case nsForwardReference::eResolve_Later:
                    // do nothing. we'll try again later
                    ;
                }
            }
        }
    }

    DestroyForwardReferences();
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetPrototype(nsIXULPrototypeDocument** aPrototype)
{
    *aPrototype = mPrototype;
    NS_IF_ADDREF(*aPrototype);
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateFromPrototype(const char* aCommand,
                                     nsIXULPrototypeDocument* aPrototype,
                                     nsIPrincipal* aPrincipal,
                                     nsIContentViewerContainer* aContainer)
{
    nsresult rv;

    mContentViewerContainer = aContainer;
    mDocumentPrincipal = aPrincipal;

    rv = aPrototype->GetURI(getter_AddRefs(mDocumentURL));
    if (NS_FAILED(rv)) return rv;

    rv = PrepareStyleSheets(mDocumentURL);
    if (NS_FAILED(rv)) return rv;
    return NS_OK;

    mCommand = aCommand;

    // Now create the delegates from the prototype
    Builder* builder = new Builder(this);
    if (! builder)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = builder->Start();

    if (NS_FAILED(rv)) {
        delete builder;
        return rv;
    }

    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsIStreamLoadableDocument interface
//

NS_IMETHODIMP
nsXULDocument::LoadFromStream(nsIInputStream& xulStream,
                                nsIContentViewerContainer* aContainer,
                                const char* aCommand)
{
    nsresult rv;

    nsCOMPtr<nsIParser> parser;
    rv = PrepareToLoad(&parser, aContainer, aCommand, nsnull, nsnull);
    if (NS_FAILED(rv)) return rv;

    parser->Parse(xulStream);
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIDOMDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetDoctype(nsIDOMDocumentType** aDoctype)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetImplementation(nsIDOMDOMImplementation** aImplementation)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetDocumentElement(nsIDOMElement** aDocumentElement)
{
    NS_PRECONDITION(aDocumentElement != nsnull, "null ptr");
    if (! aDocumentElement)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMElement::GetIID(), (void**)aDocumentElement);
    }
    else {
        *aDocumentElement = nsnull;
        return NS_OK;
    }
}



NS_IMETHODIMP
nsXULDocument::CreateElement(const nsString& aTagName, nsIDOMElement** aReturn)
{
    NS_PRECONDITION(aReturn != nsnull, "null ptr");
    if (! aReturn)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
      char* tagCStr = aTagName.ToNewCString();

      PR_LOG(gXULLog, PR_LOG_DEBUG,
             ("xul[CreateElement] %s", tagCStr));

      nsCRT::free(tagCStr);
    }
#endif

    nsCOMPtr<nsIAtom> name;
    PRInt32 nameSpaceID;

    *aReturn = nsnull;

    // parse the user-provided string into a tag name and a namespace ID
    rv = ParseTagString(aTagName, *getter_AddRefs(name), nameSpaceID);
    if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
        char* tagNameStr = aTagName.ToNewCString();
        PR_LOG(gXULLog, PR_LOG_ERROR,
               ("xul[CreateElement] unable to parse tag '%s'; no such namespace.", tagNameStr));
        nsCRT::free(tagNameStr);
#endif
        return rv;
    }

    nsCOMPtr<nsIContent> result;
    rv = CreateElement(nameSpaceID, name, getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;

    // get the DOM interface
    rv = result->QueryInterface(nsIDOMElement::GetIID(), (void**) aReturn);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM element");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateDocumentFragment(nsIDOMDocumentFragment** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateTextNode(const nsString& aData, nsIDOMText** aReturn)
{
    NS_PRECONDITION(aReturn != nsnull, "null ptr");
    if (! aReturn)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

    nsCOMPtr<nsITextContent> text;
    rv = nsComponentManager::CreateInstance(kTextNodeCID, nsnull, nsITextContent::GetIID(), getter_AddRefs(text));
    if (NS_FAILED(rv)) return rv;

    rv = text->SetText(aData.GetUnicode(), aData.Length(), PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    rv = text->QueryInterface(nsIDOMText::GetIID(), (void**) aReturn);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOMText");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateComment(const nsString& aData, nsIDOMComment** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateCDATASection(const nsString& aData, nsIDOMCDATASection** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateProcessingInstruction(const nsString& aTarget, const nsString& aData, nsIDOMProcessingInstruction** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateAttribute(const nsString& aName, nsIDOMAttr** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateEntityReference(const nsString& aName, nsIDOMEntityReference** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::GetElementsByTagName(const nsString& aTagName, nsIDOMNodeList** aReturn)
{
    nsresult rv;
    nsRDFDOMNodeList* elements;
    if (NS_FAILED(rv = nsRDFDOMNodeList::Create(&elements))) {
        NS_ERROR("unable to create node list");
        return rv;
    }

    nsIContent* root = GetRootContent();
    NS_ASSERTION(root != nsnull, "no doc root");

    if (root != nsnull) {
        nsIDOMNode* domRoot;
        if (NS_SUCCEEDED(rv = root->QueryInterface(nsIDOMNode::GetIID(), (void**) &domRoot))) {
            rv = GetElementsByTagName(domRoot, aTagName, elements);
            NS_RELEASE(domRoot);
        }
        NS_RELEASE(root);
    }

    *aReturn = elements;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementsByAttribute(const nsString& aAttribute, const nsString& aValue, 
                                        nsIDOMNodeList** aReturn)
{
    nsresult rv;
    nsRDFDOMNodeList* elements;
    if (NS_FAILED(rv = nsRDFDOMNodeList::Create(&elements))) {
        NS_ERROR("unable to create node list");
        return rv;
    }

    nsIContent* root = GetRootContent();
    NS_ASSERTION(root != nsnull, "no doc root");

    if (root != nsnull) {
        nsIDOMNode* domRoot;
        if (NS_SUCCEEDED(rv = root->QueryInterface(nsIDOMNode::GetIID(), (void**) &domRoot))) {
            rv = GetElementsByAttribute(domRoot, aAttribute, aValue, elements);
            NS_RELEASE(domRoot);
        }
        NS_RELEASE(root);
    }

    *aReturn = elements;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::Persist(const nsString& aID, const nsString& aAttr)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> domelement;
    rv = GetElementById(aID, getter_AddRefs(domelement));
    if (NS_FAILED(rv)) return rv;

    if (! domelement)
        return NS_OK;

    nsCOMPtr<nsIContent> element = do_QueryInterface(domelement);
    NS_ASSERTION(element != nsnull, "null ptr");
    if (! element)
        return NS_ERROR_UNEXPECTED;

    PRInt32 nameSpaceID;
    nsCOMPtr<nsIAtom> tag;
    rv = element->ParseAttributeString(aAttr, *getter_AddRefs(tag), nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    rv = Persist(element, nameSpaceID, tag);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


nsresult
nsXULDocument::Persist(nsIContent* aElement, PRInt32 aNameSpaceID, nsIAtom* aAttribute)
{
    // First make sure we _have_ a local store to stuff the persited
    // information into. (We might not have one if profile information
    // hasn't been loaded yet...)
    if (! mLocalStore)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIRDFResource> element;
    rv = gXULUtils->GetElementResource(aElement, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    // No ID, so nothing to persist.
    if (! element)
        return NS_OK;

    // Ick. Construct a property from the attribute. Punt on
    // namespaces for now.
    const PRUnichar* attrstr;
    rv = aAttribute->GetUnicode(&attrstr);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> attr;
    rv = gRDFService->GetResource(nsCAutoString(attrstr), getter_AddRefs(attr));
    if (NS_FAILED(rv)) return rv;

    // Turn the value into a literal
    nsAutoString valuestr;
    rv = aElement->GetAttribute(kNameSpaceID_None, aAttribute, valuestr);
    if (NS_FAILED(rv)) return rv;

    PRBool novalue = (rv != NS_CONTENT_ATTR_HAS_VALUE);

    // See if there was an old value...
    nsCOMPtr<nsIRDFNode> oldvalue;
    rv = mLocalStore->GetTarget(element, attr, PR_TRUE, getter_AddRefs(oldvalue));
    if (NS_FAILED(rv)) return rv;

    if (oldvalue && novalue) {
        // ...there was an oldvalue, and they've removed it. XXXThis
        // handling isn't quite right...
        rv = mLocalStore->Unassert(element, attr, oldvalue);
    }
    else {
        // Now either 'change' or 'assert' based on whether there was
        // an old value.
        nsCOMPtr<nsIRDFLiteral> newvalue;
        rv = gRDFService->GetLiteral(valuestr.GetUnicode(), getter_AddRefs(newvalue));
        if (NS_FAILED(rv)) return rv;

        if (oldvalue) {
            rv = mLocalStore->Change(element, attr, oldvalue, newvalue);
        }
        else {
            rv = mLocalStore->Assert(element, attr, newvalue, PR_TRUE);
        }
    }

    if (NS_FAILED(rv)) return rv;

    // Add it to the persisted set for this document (if it's not
    // there already).
    {
        nsXPIDLCString docurl;
        rv = mDocumentURL->GetSpec(getter_Copies(docurl));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> doc;
        rv = gRDFService->GetResource(docurl, getter_AddRefs(doc));
        if (NS_FAILED(rv)) return rv;

        PRBool hasAssertion;
        rv = mLocalStore->HasAssertion(doc, kNC_persist, element, PR_TRUE, &hasAssertion);
        if (NS_FAILED(rv)) return rv;

        if (! hasAssertion) {
            rv = mLocalStore->Assert(doc, kNC_persist, element, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}



nsresult
nsXULDocument::DestroyForwardReferences()
{
    for (PRInt32 i = mForwardReferences.Count() - 1; i >= 0; --i) {
        nsForwardReference* fwdref = NS_REINTERPRET_CAST(nsForwardReference*, mForwardReferences[i]);
        delete fwdref;
    }

    mForwardReferences.Clear();
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIDOMNSDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetStyleSheets(nsIDOMStyleSheetCollection** aStyleSheets)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CreateElementWithNameSpace(const nsString& aTagName,
                                            const nsString& aNameSpace,
                                            nsIDOMElement** aResult)
{
    // Create a DOM element given a namespace URI and a tag name.
    NS_PRECONDITION(aResult != nsnull, "null ptr");
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
      char* namespaceCStr = aNameSpace.ToNewCString();
      char* tagCStr = aTagName.ToNewCString();

      PR_LOG(gXULLog, PR_LOG_DEBUG,
             ("xul[CreateElementWithNameSpace] [%s]:%s", namespaceCStr, tagCStr));

      nsCRT::free(tagCStr);
      nsCRT::free(namespaceCStr);
    }
#endif

    nsCOMPtr<nsIAtom> name = dont_AddRef(NS_NewAtom(aTagName.GetUnicode()));
    if (! name)
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 nameSpaceID;
    rv = mNameSpaceManager->GetNameSpaceID(aNameSpace, nameSpaceID);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIContent> result;
    rv = CreateElement(nameSpaceID, name, getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;

    // get the DOM interface
    rv = result->QueryInterface(nsIDOMElement::GetIID(), (void**) aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "not a DOM element");
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::CreateRange(nsIDOMRange** aRange)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP    
nsXULDocument::GetWidth(PRInt32* aWidth)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}
 
NS_IMETHODIMP    
nsXULDocument::GetHeight(PRInt32* aHeight)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------
//
// nsIDOMXULDocument interface
//

NS_IMETHODIMP
nsXULDocument::GetPopupNode(nsIDOMNode** aNode)
{
	*aNode = mPopupNode;
	NS_IF_ADDREF(*aNode);
	return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetPopupNode(nsIDOMNode* aNode)
{
	mPopupNode = dont_QueryInterface(aNode);
	return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetTooltipNode(nsIDOMNode** aNode)
{
	*aNode = mTooltipNode;
	NS_IF_ADDREF(*aNode);
	return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::SetTooltipNode(nsIDOMNode* aNode)
{
	mTooltipNode = dont_QueryInterface(aNode);
	return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetCommandDispatcher(nsIDOMXULCommandDispatcher** aTracker)
{
  *aTracker = mCommandDispatcher;
  NS_IF_ADDREF(*aTracker);
  return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetElementById(const nsString& aId, nsIDOMElement** aReturn)
{
    nsresult rv;

    nsCOMPtr<nsIContent> element;
    rv = mElementMap.FindFirst(aId, getter_AddRefs(element));
    if (NS_FAILED(rv)) return rv;

    if (element) {
        rv = element->QueryInterface(NS_GET_IID(nsIDOMElement), (void**) aReturn);
    }
    else {
        *aReturn = nsnull;
        rv = NS_OK;
    }

    return rv;
}

nsresult
nsXULDocument::AddSubtreeToDocument(nsIContent* aElement)
{
    // Do a bunch of work that's necessary when an element gets added
    // to the XUL Document.
    nsresult rv;

    // 1. Add the element to the resource-to-element map
    rv = AddElementToMap(aElement);
    if (NS_FAILED(rv)) return rv;

    // 2. If the element is a 'command updater' (i.e., has a
    // "commandupdater='true'" attribute), then add the element to the
    // document's command dispatcher
    nsAutoString value;
    rv = aElement->GetAttribute(kNameSpaceID_None, kCommandUpdaterAtom, value);
    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && value.Equals("true")) {
        rv = gXULUtils->SetCommandUpdater(this, aElement);
        if (NS_FAILED(rv)) return rv;
    }

    PRInt32 count;
    nsCOMPtr<nsIXULContent> xulcontent = do_QueryInterface(aElement);
    rv = xulcontent ? xulcontent->PeekChildCount(count) : aElement->ChildCount(count);
    if (NS_FAILED(rv)) return rv;

    while (--count >= 0) {
        nsCOMPtr<nsIContent> child;
        rv = aElement->ChildAt(count, *getter_AddRefs(child));
        if (NS_FAILED(rv)) return rv;

        rv = AddSubtreeToDocument(child);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

nsresult
nsXULDocument::RemoveSubtreeFromDocument(nsIContent* aElement)
{
    // Do a bunch of cleanup to remove an element from the XUL
    // document.
    nsresult rv;

    PRInt32 count;
    nsCOMPtr<nsIXULContent> xulcontent = do_QueryInterface(aElement);
    rv = xulcontent ? xulcontent->PeekChildCount(count) : aElement->ChildCount(count);
    if (NS_FAILED(rv)) return rv;

    while (--count >= 0) {
        nsCOMPtr<nsIContent> child;
        rv = aElement->ChildAt(count, *getter_AddRefs(child));
        if (NS_FAILED(rv)) return rv;

        rv = RemoveSubtreeFromDocument(child);
        if (NS_FAILED(rv)) return rv;
    }

    // 1. Remove the element from the resource-to-element map
    rv = RemoveElementFromMap(aElement);
    if (NS_FAILED(rv)) return rv;

    // 2. If the element is a 'command updater', then remove the
    // element from the document's command dispatcher.
    nsAutoString value;
    rv = aElement->GetAttribute(kNameSpaceID_None, kCommandUpdaterAtom, value);
    if ((rv == NS_CONTENT_ATTR_HAS_VALUE) && value.Equals("true")) {
        nsCOMPtr<nsIDOMElement> domelement = do_QueryInterface(aElement);
        NS_ASSERTION(domelement != nsnull, "not a DOM element");
        if (! domelement)
            return NS_ERROR_UNEXPECTED;

        rv = mCommandDispatcher->RemoveCommandUpdater(domelement);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

// Attributes that are used with getElementById() and the
// resource-to-element map.
nsIAtom** nsXULDocument::kIdentityAttrs[] = { &kIdAtom, &kRefAtom, nsnull };

nsresult
nsXULDocument::AddElementToMap(nsIContent* aElement)
{
    // Look at the element's 'id' and 'ref' attributes, and if set,
    // add pointers in the resource-to-element map to the element.
    nsresult rv;

    for (PRInt32 i = 0; kIdentityAttrs[i] != nsnull; ++i) {
        nsAutoString value;
        rv = aElement->GetAttribute(kNameSpaceID_None, *kIdentityAttrs[i], value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get attribute");
        if (NS_FAILED(rv)) return rv;

        if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
            rv = mElementMap.Add(value, aElement);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


nsresult
nsXULDocument::RemoveElementFromMap(nsIContent* aElement)
{
    // Remove the element from the resource-to-element map.
    nsresult rv;

    for (PRInt32 i = 0; kIdentityAttrs[i] != nsnull; ++i) {
        nsAutoString value;
        rv = aElement->GetAttribute(kNameSpaceID_None, *kIdentityAttrs[i], value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get attribute");
        if (NS_FAILED(rv)) return rv;

        if (rv == NS_CONTENT_ATTR_HAS_VALUE) {
            rv = mElementMap.Remove(value, aElement);
            if (NS_FAILED(rv)) return rv;
        }
    }

    return NS_OK;
}


PRIntn
nsXULDocument::RemoveElementsFromMapByContent(const nsString& aID,
                                                nsIContent* aElement,
                                                void* aClosure)
{
    nsIContent* content = NS_REINTERPRET_CAST(nsIContent*, aClosure);
    return (aElement == content) ? HT_ENUMERATE_REMOVE : HT_ENUMERATE_NEXT;
}



//----------------------------------------------------------------------
//
// nsIXULParentDocument interface
//

NS_IMETHODIMP 
nsXULDocument::GetContentViewerContainer(nsIContentViewerContainer** aContainer)
{
    NS_PRECONDITION ( aContainer, "Null Parameter into GetContentViewerContainer" );
    
    *aContainer = mContentViewerContainer;
    NS_IF_ADDREF(*aContainer);

    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetCommand(nsString& aCommand)
{
    aCommand = mCommand;
    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsIXULChildDocument interface
//

NS_IMETHODIMP 
nsXULDocument::SetContentSink(nsIXULContentSink* aParentContentSink)
{
    mParentContentSink = aParentContentSink;
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetContentSink(nsIXULContentSink** aParentContentSink)
{
    NS_IF_ADDREF(mParentContentSink);
    *aParentContentSink = mParentContentSink;
    return NS_OK;
}

//----------------------------------------------------------------------
//
// nsIDOMNode interface
//

NS_IMETHODIMP
nsXULDocument::GetNodeName(nsString& aNodeName)
{
    aNodeName.SetString("#document");
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNodeValue(nsString& aNodeValue)
{
    aNodeValue.Truncate();
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::SetNodeValue(const nsString& aNodeValue)
{
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNodeType(PRUint16* aNodeType)
{
    *aNodeType = nsIDOMNode::DOCUMENT_NODE;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetParentNode(nsIDOMNode** aParentNode)
{
    *aParentNode = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetChildNodes(nsIDOMNodeList** aChildNodes)
{
    NS_PRECONDITION(aChildNodes != nsnull, "null ptr");
    if (! aChildNodes)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        nsresult rv;

        *aChildNodes = nsnull;

        nsRDFDOMNodeList* children;
        rv = nsRDFDOMNodeList::Create(&children);

        if (NS_SUCCEEDED(rv)) {
            nsIDOMNode* domNode = nsnull;
            rv = mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**)&domNode);
            NS_ASSERTION(NS_SUCCEEDED(rv), "root content is not a DOM node");

            if (NS_SUCCEEDED(rv)) {
                rv = children->AppendNode(domNode);
                NS_RELEASE(domNode);

                *aChildNodes = children;
                return NS_OK;
            }
        }

        // If we get here, something bad happened.
        NS_RELEASE(children);
        return rv;
    }
    else {
        *aChildNodes = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::HasChildNodes(PRBool* aHasChildNodes)
{
    NS_PRECONDITION(aHasChildNodes != nsnull, "null ptr");
    if (! aHasChildNodes)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        *aHasChildNodes = PR_TRUE;
    }
    else {
        *aHasChildNodes = PR_FALSE;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetFirstChild(nsIDOMNode** aFirstChild)
{
    NS_PRECONDITION(aFirstChild != nsnull, "null ptr");
    if (! aFirstChild)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**) aFirstChild);
    }
    else {
        *aFirstChild = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::GetLastChild(nsIDOMNode** aLastChild)
{
    NS_PRECONDITION(aLastChild != nsnull, "null ptr");
    if (! aLastChild)
        return NS_ERROR_NULL_POINTER;

    if (mRootContent) {
        return mRootContent->QueryInterface(nsIDOMNode::GetIID(), (void**) aLastChild);
    }
    else {
        *aLastChild = nsnull;
        return NS_OK;
    }
}


NS_IMETHODIMP
nsXULDocument::GetPreviousSibling(nsIDOMNode** aPreviousSibling)
{
    NS_PRECONDITION(aPreviousSibling != nsnull, "null ptr");
    if (! aPreviousSibling)
        return NS_ERROR_NULL_POINTER;

    *aPreviousSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetNextSibling(nsIDOMNode** aNextSibling)
{
    NS_PRECONDITION(aNextSibling != nsnull, "null ptr");
    if (! aNextSibling)
        return NS_ERROR_NULL_POINTER;

    *aNextSibling = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetAttributes(nsIDOMNamedNodeMap** aAttributes)
{
    NS_PRECONDITION(aAttributes != nsnull, "null ptr");
    if (! aAttributes)
        return NS_ERROR_NULL_POINTER;

    *aAttributes = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::GetOwnerDocument(nsIDOMDocument** aOwnerDocument)
{
    NS_PRECONDITION(aOwnerDocument != nsnull, "null ptr");
    if (! aOwnerDocument)
        return NS_ERROR_NULL_POINTER;

    *aOwnerDocument = nsnull;
    return NS_OK;
}


NS_IMETHODIMP
nsXULDocument::InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
{
    NS_NOTYETIMPLEMENTED("write me!");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsXULDocument::CloneNode(PRBool aDeep, nsIDOMNode** aReturn)
{
    // We don't allow cloning of a document
    *aReturn = nsnull;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIJSScriptObject interface
//

PRBool
nsXULDocument::AddProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::GetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    PRBool result = PR_TRUE;

    if (JSVAL_IS_STRING(aID) && 
        PL_strcmp("location", JS_GetStringBytes(JS_ValueToString(aContext, aID))) == 0) {
        if (nsnull != mScriptContextOwner) {
            nsIScriptGlobalObject *global;
            mScriptContextOwner->GetScriptGlobalObject(&global);
            if (nsnull != global) {
                nsIJSScriptObject *window;
                if (NS_OK == global->QueryInterface(NS_GET_IID(nsIJSScriptObject), (void **)&window)) {
                    result = window->GetProperty(aContext, aID, aVp);
                    NS_RELEASE(window);
                }
                else {
                    result = PR_FALSE;
                }
                NS_RELEASE(global);
            }
        }
    }

    return result;
}


PRBool
nsXULDocument::SetProperty(JSContext *aContext, jsval aID, jsval *aVp)
{
    nsresult rv;

    if (JSVAL_IS_STRING(aID)) {
        char* s = JS_GetStringBytes(JS_ValueToString(aContext, aID));
        if (PL_strcmp("title", s) == 0) {
            nsAutoString title("get me out of aVp somehow");
            for (PRInt32 i = mPresShells.Count() - 1; i >= 0; --i) {
                nsIPresShell* shell = NS_STATIC_CAST(nsIPresShell*, mPresShells[i]);
                nsCOMPtr<nsIPresContext> context;
                rv = shell->GetPresContext(getter_AddRefs(context));
                if (NS_FAILED(rv)) return PR_FALSE;

                nsCOMPtr<nsISupports> container;
                rv = context->GetContainer(getter_AddRefs(container));
                if (NS_FAILED(rv)) return PR_FALSE;

                if (! container) continue;

                nsCOMPtr<nsIWebShell> webshell = do_QueryInterface(container);
                if (! webshell) continue;

                rv = webshell->SetTitle(title.GetUnicode());
                if (NS_FAILED(rv)) return PR_FALSE;
            }
        }
        else if (PL_strcmp("location", s) == 0) {
            NS_NOTYETIMPLEMENTED("write me");
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}


PRBool
nsXULDocument::EnumerateProperty(JSContext *aContext)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


PRBool
nsXULDocument::Resolve(JSContext *aContext, jsval aID)
{
    return PR_TRUE;
}


PRBool
nsXULDocument::Convert(JSContext *aContext, jsval aID)
{
    NS_NOTYETIMPLEMENTED("write me");
    return PR_TRUE;
}


void
nsXULDocument::Finalize(JSContext *aContext)
{
    NS_NOTYETIMPLEMENTED("write me");
}



//----------------------------------------------------------------------
//
// nsIScriptObjectOwner interface
//

NS_IMETHODIMP
nsXULDocument::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    nsresult res = NS_OK;
    nsIScriptGlobalObject *global = aContext->GetGlobalObject();

    if (nsnull == mScriptObject) {
        res = NS_NewScriptXULDocument(aContext, (nsISupports *)(nsIDOMXULDocument *)this, global, (void**)&mScriptObject);
    }
    *aScriptObject = mScriptObject;

    NS_RELEASE(global);
    return res;
}


NS_IMETHODIMP
nsXULDocument::SetScriptObject(void *aScriptObject)
{
    mScriptObject = aScriptObject;
    return NS_OK;
}


//----------------------------------------------------------------------
//
// nsIHTMLContentContainer interface
//

NS_IMETHODIMP 
nsXULDocument::GetAttributeStyleSheet(nsIHTMLStyleSheet** aResult)
{
    NS_PRECONDITION(nsnull != aResult, "null ptr");
    if (nsnull == aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = mAttrStyleSheet;
    if (! mAttrStyleSheet) {
        return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
    }
    else {
        NS_ADDREF(*aResult);
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsXULDocument::GetInlineStyleSheet(nsIHTMLCSSStyleSheet** aResult)
{
    NS_NOTYETIMPLEMENTED("get the inline stylesheet!");

    NS_PRECONDITION(nsnull != aResult, "null ptr");
    if (nsnull == aResult) {
        return NS_ERROR_NULL_POINTER;
    }
    *aResult = mInlineStyleSheet;
    if (!mInlineStyleSheet) {
        return NS_ERROR_NOT_AVAILABLE;  // probably not the right error...
    }
    else {
        NS_ADDREF(*aResult);
    }
    return NS_OK;
}

//----------------------------------------------------------------------
//
// Implementation methods
//

nsIContent*
nsXULDocument::FindContent(const nsIContent* aStartNode,
                             const nsIContent* aTest1, 
                             const nsIContent* aTest2) const
{
    PRInt32 count;
    aStartNode->ChildCount(count);

    PRInt32 i;
    for(i = 0; i < count; i++) {
        nsIContent* child;
        aStartNode->ChildAt(i, child);
        nsIContent* content = FindContent(child,aTest1,aTest2);
        if (content != nsnull) {
            NS_IF_RELEASE(child);
            return content;
        }
        if (child == aTest1 || child == aTest2) {
            NS_IF_RELEASE(content);
            return child;
        }
        NS_IF_RELEASE(child);
        NS_IF_RELEASE(content);
    }
    return nsnull;
}


nsresult
nsXULDocument::Init(void)
{
    nsresult rv;

    rv = NS_NewHeapArena(getter_AddRefs(mArena), nsnull);
    if (NS_FAILED(rv)) return rv;

    // Create a namespace manager so we can manage tags
    rv = nsComponentManager::CreateInstance(kNameSpaceManagerCID,
                                            nsnull,
                                            NS_GET_IID(nsINameSpaceManager),
                                            getter_AddRefs(mNameSpaceManager));
    if (NS_FAILED(rv)) return rv;

    // Create our focus tracker and hook it up.
    nsCOMPtr<nsIXULCommandDispatcher> commandDis;
    rv = nsComponentManager::CreateInstance(kXULCommandDispatcherCID,
                                            nsnull,
                                            NS_GET_IID(nsIXULCommandDispatcher),
                                            getter_AddRefs(commandDis));
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a focus tracker");
    if (NS_FAILED(rv)) return rv;

    mCommandDispatcher = do_QueryInterface(commandDis);

    nsCOMPtr<nsIDOMEventListener> CommandDispatcher =
        do_QueryInterface(mCommandDispatcher);

    if (CommandDispatcher) {
      // Take the focus tracker and add it as an event listener for focus and blur events.
        AddEventListener("focus", CommandDispatcher, PR_TRUE);
        AddEventListener("blur", CommandDispatcher, PR_TRUE);
    }

    // Get the local store. Yeah, I know. I wish GetService() used a
    // 'void**', too.
    nsIRDFDataSource* localstore;
    rv = nsServiceManager::GetService(kLocalStoreCID,
                                      NS_GET_IID(nsIRDFDataSource),
                                      (nsISupports**) &localstore);

    // this _could_ fail; e.g., if we've tried to grab the local store
    // before profiles have initialized. If so, no big deal; nothing
    // will persist.

    if (NS_SUCCEEDED(rv)) {
        mLocalStore = localstore;
        NS_IF_RELEASE(localstore);
    }

#if 0
    // construct a selection object
    if (NS_FAILED(rv = nsComponentManager::CreateInstance(kRangeListCID,
                                                    nsnull,
                                                    kIDOMSelectionIID,
                                                    (void**) &mSelection))) {
        NS_ERROR("unable to create DOM selection");
    }
#endif

    if (gRefCnt++ == 0) {
        kAttributeAtom      = NS_NewAtom("attribute");
        kCommandUpdaterAtom = NS_NewAtom("commandupdater");
        kElementAtom        = NS_NewAtom("element");
        kIdAtom             = NS_NewAtom("id");
        kObservesAtom       = NS_NewAtom("observes");
        kOpenAtom           = NS_NewAtom("open");
        kOverlayAtom        = NS_NewAtom("overlay");
        kPersistAtom        = NS_NewAtom("persist");
        kPositionAtom       = NS_NewAtom("position");
        kRefAtom            = NS_NewAtom("ref");
        kRuleAtom           = NS_NewAtom("rule");
        kTemplateAtom       = NS_NewAtom("template");

        // Keep the RDF service cached in a member variable to make using
        // it a bit less painful
        rv = nsServiceManager::GetService(kRDFServiceCID,
                                          NS_GET_IID(nsIRDFService),
                                          (nsISupports**) &gRDFService);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF Service");
        if (NS_FAILED(rv)) return rv;

        gRDFService->GetResource(NC_NAMESPACE_URI "persist",   &kNC_persist);
        gRDFService->GetResource(NC_NAMESPACE_URI "attribute", &kNC_attribute);
        gRDFService->GetResource(NC_NAMESPACE_URI "value",     &kNC_value);

        rv = nsComponentManager::CreateInstance(kHTMLElementFactoryCID,
                                                nsnull,
                                                NS_GET_IID(nsIHTMLElementFactory),
                                                (void**) &gHTMLElementFactory);

        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get HTML element factory");
        if (NS_FAILED(rv)) return rv;

        rv = nsServiceManager::GetService(kNameSpaceManagerCID,
                                          NS_GET_IID(nsINameSpaceManager),
                                          (nsISupports**) &gNameSpaceManager);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get namespace manager");
        if (NS_FAILED(rv)) return rv;

#define XUL_NAMESPACE_URI "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"
static const char kXULNameSpaceURI[] = XUL_NAMESPACE_URI;
        gNameSpaceManager->RegisterNameSpace(kXULNameSpaceURI, kNameSpaceID_XUL);


        rv = nsServiceManager::GetService(kXULContentUtilsCID,
                                          NS_GET_IID(nsIXULContentUtils),
                                          (nsISupports**) &gXULUtils);
        if (NS_FAILED(rv)) return rv;

        rv = nsServiceManager::GetService(kXULPrototypeCacheCID,
                                          NS_GET_IID(nsIXULPrototypeCache),
                                          (nsISupports**) &gXULPrototypeCache);
        if (NS_FAILED(rv)) return rv;
    }

#ifdef PR_LOGGING
    if (! gXULLog)
        gXULLog = PR_NewLogModule("nsXULDocument");
#endif

    return NS_OK;
}



nsresult
nsXULDocument::StartLayout(void)
{
    NS_PRECONDITION(mRootContent != nsnull, "Error in XUL file. Love to tell ya where if only I knew.");
    if (!mRootContent)
      return NS_ERROR_UNEXPECTED;
    
    PRInt32 count = GetNumberOfShells();
    for (PRInt32 i = 0; i < count; i++) {
      nsIPresShell* shell = GetShellAt(i);
      if (nsnull == shell)
          continue;

      // Resize-reflow this time
      nsCOMPtr<nsIPresContext> cx;
      shell->GetPresContext(getter_AddRefs(cx));

      PRBool intrinsic = PR_FALSE;
      nsCOMPtr<nsIWebShell> webShell;
      nsCOMPtr<nsIBrowserWindow> browser;

		  if (cx) {
			  nsCOMPtr<nsISupports> container;
			  cx->GetContainer(getter_AddRefs(container));
			  if (container) {
			    webShell = do_QueryInterface(container);
			    if (webShell) {
					  webShell->SetScrolling(NS_STYLE_OVERFLOW_HIDDEN);
            nsCOMPtr<nsIWebShellContainer> webShellContainer;
            webShell->GetContainer(*getter_AddRefs(webShellContainer));
            if (webShellContainer) {
              browser = do_QueryInterface(webShellContainer);
              if (browser)
                browser->IsIntrinsicallySized(intrinsic);
            }
			    }
			  }
		  }
        
		  nsRect r;
      cx->GetVisibleArea(r);
      if (intrinsic) {
        // Flow at an unconstrained width and height
        r.width = NS_UNCONSTRAINEDSIZE;
        r.height = NS_UNCONSTRAINEDSIZE;
      }

      if (browser) {
        // We're top-level.
        // See if we have attributes on our root tag that set the width and height.
        // read "height" attribute// Convert r.width and r.height to twips.
        float p2t;
        cx->GetPixelsToTwips(&p2t);
        
        nsCOMPtr<nsIDOMElement> windowElement = do_QueryInterface(mRootContent);
        nsString sizeString;
        PRInt32 specSize;
        PRInt32 errorCode;
        if (NS_SUCCEEDED(windowElement->GetAttribute("height", sizeString))) {
          specSize = sizeString.ToInteger(&errorCode);
          if (NS_SUCCEEDED(errorCode) && specSize > 0)
            r.height = NSIntPixelsToTwips(specSize, p2t);
        }

        // read "width" attribute
        if (NS_SUCCEEDED(windowElement->GetAttribute("width", sizeString))) {
          specSize = sizeString.ToInteger(&errorCode);
          if (NS_SUCCEEDED(errorCode) || specSize > 0)
            r.width = NSIntPixelsToTwips(specSize, p2t);
        }
      }

      cx->SetVisibleArea(r);
      
      // XXX Copy of the code below. See XXX below for details...
      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsCOMPtr<nsIContentViewer> contentViewer;
        nsresult rv = webShell->GetContentViewer(getter_AddRefs(contentViewer));
        if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
          PRBool enabled;
          contentViewer->GetEnableRendering(&enabled);
          if (enabled) {
            vm->EnableRefresh();
          }
        }
      }

      shell->InitialReflow(r.width, r.height);

      if (browser) {
        // We're top level.
        // Retrieve the answer.
        cx->GetVisibleArea(r);

        // Perform the resize
        PRInt32 chromeX,chromeY,chromeWidth,chromeHeight;
        webShell->GetBounds(chromeX,chromeY,chromeWidth,chromeHeight);

        float t2p;
        cx->GetTwipsToPixels(&t2p);
        PRInt32 width = PRInt32((float)r.width*t2p);
        PRInt32 height = PRInt32((float)r.height*t2p);
      
        PRInt32 widthDelta = width - chromeWidth;
        PRInt32 heightDelta = height - chromeHeight;

        nsRect windowBounds;
        browser->GetWindowBounds(windowBounds);
        browser->SizeWindowTo(windowBounds.width + widthDelta, 
                              windowBounds.height + heightDelta);
      }
      
      // XXX Moving this call up before the call to InitialReflow(), because
      // the view manager's UpdateView() function is dropping dirty rects if
      // refresh is disabled rather than accumulating them until refresh is
      // enabled and then triggering a repaint...
#if 0
      // Now trigger a refresh
      nsCOMPtr<nsIViewManager> vm;
      shell->GetViewManager(getter_AddRefs(vm));
      if (vm) {
        nsCOMPtr<nsIContentViewer> contentViewer;
        nsresult rv = webShell->GetContentViewer(getter_AddRefs(contentViewer));
        if (NS_SUCCEEDED(rv) && (contentViewer != nsnull)) {
          PRBool enabled;
          contentViewer->GetEnableRendering(&enabled);
          if (enabled) {
            vm->EnableRefresh();
          }
        }
      }
#endif
 
      // Start observing the document _after_ we do the initial
      // reflow. Otherwise, we'll get into an trouble trying to
      // create kids before the root frame is established.
      shell->BeginObservingDocument();

      NS_RELEASE(shell);
    }
    return NS_OK;
}


nsresult
nsXULDocument::GetElementsByTagName(nsIDOMNode* aNode,
                                      const nsString& aTagName,
                                      nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> element;
    element = do_QueryInterface(aNode);
    if (!element)
      return NS_OK;

    if (aTagName.Equals("*")) {
        if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
            NS_ERROR("unable to append element to node list");
            return rv;
        }
    }
    else {
        nsAutoString name;
        if (NS_FAILED(rv = aNode->GetNodeName(name))) {
            NS_ERROR("unable to get node name");
            return rv;
        }

        if (aTagName.Equals(name)) {
            if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
                NS_ERROR("unable to append element to node list");
                return rv;
            }
        }
    }

    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        if (NS_FAILED(rv = GetElementsByTagName(child, aTagName, aElements))) {
            NS_ERROR("unable to recursively get elements by tag name");
            return rv;
        }
    }

    return NS_OK;
}

nsresult
nsXULDocument::GetElementsByAttribute(nsIDOMNode* aNode,
                                        const nsString& aAttribute,
                                        const nsString& aValue,
                                        nsRDFDOMNodeList* aElements)
{
    nsresult rv;

    nsCOMPtr<nsIDOMElement> element;
    element = do_QueryInterface(aNode);
    if (!element)
        return NS_OK;

    nsAutoString attrValue;
    if (NS_FAILED(rv = element->GetAttribute(aAttribute, attrValue))) {
        NS_ERROR("unable to get attribute value");
        return rv;
    }

    if ((attrValue == aValue) || (attrValue.Length() > 0 && aValue == "*")) {
        if (NS_FAILED(rv = aElements->AppendNode(aNode))) {
            NS_ERROR("unable to append element to node list");
            return rv;
        }
    }
       
    nsCOMPtr<nsIDOMNodeList> children;
    if (NS_FAILED(rv = aNode->GetChildNodes( getter_AddRefs(children) ))) {
        NS_ERROR("unable to get node's children");
        return rv;
    }

    // no kids: terminate the recursion
    if (! children)
        return NS_OK;

    PRUint32 length;
    if (NS_FAILED(children->GetLength(&length))) {
        NS_ERROR("unable to get node list's length");
        return rv;
    }

    for (PRUint32 i = 0; i < length; ++i) {
        nsCOMPtr<nsIDOMNode> child;
        if (NS_FAILED(rv = children->Item(i, getter_AddRefs(child) ))) {
            NS_ERROR("unable to get child from list");
            return rv;
        }

        if (NS_FAILED(rv = GetElementsByAttribute(child, aAttribute, aValue, aElements))) {
            NS_ERROR("unable to recursively get elements by attribute");
            return rv;
        }
    }

    return NS_OK;
}



nsresult
nsXULDocument::ParseTagString(const nsString& aTagName, nsIAtom*& aName, PRInt32& aNameSpaceID)
{
    // Parse the tag into a name and a namespace ID. This is slightly
    // different than nsIContent::ParseAttributeString() because we
    // take the default namespace into account (rather than just
    // assigning "no namespace") in the case that there is no
    // namespace prefix present.

static char kNameSpaceSeparator = ':';

    // XXX this is a gross hack, but it'll carry us for now. We parse
    // the tag name using the root content, which presumably has all
    // the namespace info we need.
    NS_PRECONDITION(mRootContent != nsnull, "not initialized");
    if (! mRootContent)
        return NS_ERROR_NOT_INITIALIZED;

    nsCOMPtr<nsIXMLContent> xml( do_QueryInterface(mRootContent) );
    if (! xml) return NS_ERROR_UNEXPECTED;
    
    nsresult rv;
    nsCOMPtr<nsINameSpace> ns;
    rv = xml->GetContainingNameSpace(*getter_AddRefs(ns));
    if (NS_FAILED(rv)) return rv;

    NS_ASSERTION(ns != nsnull, "expected xml namespace info to be available");
    if (! ns)
        return NS_ERROR_UNEXPECTED;

    nsAutoString prefix;
    nsAutoString name(aTagName);
    PRInt32 nsoffset = name.FindChar(kNameSpaceSeparator);
    if (-1 != nsoffset) {
        name.Left(prefix, nsoffset);
        name.Cut(0, nsoffset+1);
    }

    // Figure out the namespace ID
    nsCOMPtr<nsIAtom> nameSpaceAtom;
    if (0 < prefix.Length())
        nameSpaceAtom = getter_AddRefs(NS_NewAtom(prefix));

    rv = ns->FindNameSpaceID(nameSpaceAtom, aNameSpaceID);
    if (NS_FAILED(rv)) return rv;

    aName = NS_NewAtom(name);
    return NS_OK;
}


// nsIDOMEventCapturer and nsIDOMEventReceiver Interface Implementations

NS_IMETHODIMP
nsXULDocument::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    nsIEventListenerManager *manager;

    if (NS_OK == GetListenerManager(&manager)) {
        manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        NS_RELEASE(manager);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
    if (mListenerManager) {
        mListenerManager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::AddEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                 PRBool aUseCapture)
{
  nsIEventListenerManager *manager;

  if (NS_OK == GetListenerManager(&manager)) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    manager->AddEventListenerByType(aListener, aType, flags);
    NS_RELEASE(manager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::RemoveEventListener(const nsString& aType, nsIDOMEventListener* aListener, 
                                    PRBool aUseCapture)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

    mListenerManager->RemoveEventListenerByType(aListener, aType, flags);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsXULDocument::GetListenerManager(nsIEventListenerManager** aResult)
{
    if (! mListenerManager) {
        nsresult rv;
        rv = nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                                nsnull,
                                                NS_GET_IID(nsIEventListenerManager),
                                                getter_AddRefs(mListenerManager));

        if (NS_FAILED(rv)) return rv;
    }
    *aResult = mListenerManager;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsXULDocument::GetNewListenerManager(nsIEventListenerManager **aResult)
{
    return nsComponentManager::CreateInstance(kEventListenerManagerCID,
                                        nsnull,
                                        NS_GET_IID(nsIEventListenerManager),
                                        (void**) aResult);
}

nsresult
nsXULDocument::CaptureEvent(const nsString& aType)
{
  nsIEventListenerManager *mManager;

  if (NS_OK == GetListenerManager(&mManager)) {
    //mManager->CaptureEvent(aListener);
    NS_RELEASE(mManager);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsXULDocument::ReleaseEvent(const nsString& aType)
{
  if (mListenerManager) {
    //mListenerManager->ReleaseEvent(aListener);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsXULDocument::OpenWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc open-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->OpenContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error opening container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}

nsresult
nsXULDocument::CloseWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc close-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->CloseContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error closing container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


nsresult
nsXULDocument::RebuildWidgetItem(nsIContent* aElement)
{
    NS_PRECONDITION(aElement != nsnull, "null ptr");
    if (! aElement)
        return NS_ERROR_NULL_POINTER;

    if (! mBuilders)
        return NS_ERROR_NOT_INITIALIZED;

    nsresult rv;

#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULLog, PR_LOG_DEBUG)) {
        nsCOMPtr<nsIAtom> tag;
        rv = aElement->GetTag(*getter_AddRefs(tag));
        if (NS_FAILED(rv)) return rv;

        nsAutoString tagStr;
        tag->ToString(tagStr);

        PR_LOG(gXULLog, PR_LOG_DEBUG,
               ("xuldoc close-widget-item %s",
                (const char*) nsCAutoString(tagStr)));
    }
#endif

    PRUint32 cnt = 0;
    rv = mBuilders->Count(&cnt);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Count failed");
    for (PRUint32 i = 0; i < cnt; ++i) {
        // XXX we should QueryInterface() here
        nsIRDFContentModelBuilder* builder
            = (nsIRDFContentModelBuilder*) mBuilders->ElementAt(i);

        NS_ASSERTION(builder != nsnull, "null ptr");
        if (! builder)
            continue;

        rv = builder->RebuildContainer(aElement);
        NS_ASSERTION(NS_SUCCEEDED(rv), "error rebuilding container");
        // XXX ignore error code?

        NS_RELEASE(builder);
    }

    return NS_OK;
}


nsresult
nsXULDocument::CreateElement(PRInt32 aNameSpaceID,
                               nsIAtom* aTag,
                               nsIContent** aResult)
{
    nsresult rv;
    nsCOMPtr<nsIContent> result;

    if (aNameSpaceID == kNameSpaceID_HTML) {
        nsCOMPtr<nsIHTMLContent> element;
        const PRUnichar *tagName;
        aTag->GetUnicode(&tagName);

        rv = gHTMLElementFactory->CreateInstanceByTag(tagName, getter_AddRefs(element));
        if (NS_FAILED(rv)) return rv;

        result = do_QueryInterface(element);
        if (! result)
            return NS_ERROR_UNEXPECTED;
    }
    else {
        rv = nsXULElement::Create(aNameSpaceID, aTag, getter_AddRefs(result));
        if (NS_FAILED(rv)) return rv;
    }

    rv = result->SetDocument(this, PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to set element's document");
    if (NS_FAILED(rv)) return rv;

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributes()
{
    // Add all of the 'persisted' attributes into the content
    // model.
    if (! mLocalStore)
        return NS_OK;

    nsresult rv;
    nsCOMPtr<nsISupportsArray> array;

    nsXPIDLCString docurl;
    rv = mDocumentURL->GetSpec(getter_Copies(docurl));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource> doc;
    rv = gRDFService->GetResource(docurl, getter_AddRefs(doc));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsISimpleEnumerator> elements;
    rv = mLocalStore->GetTargets(doc, kNC_persist, PR_TRUE, getter_AddRefs(elements));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        rv = elements->HasMoreElements(&hasmore);
        if (NS_FAILED(rv)) return rv;

        if (! hasmore)
            break;

        if (! array) {
            rv = NS_NewISupportsArray(getter_AddRefs(array));
            if (NS_FAILED(rv)) return rv;
        }

        nsCOMPtr<nsISupports> isupports;
        rv = elements->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> resource = do_QueryInterface(isupports);
        if (! resource) {
            NS_WARNING("expected element to be a resource");
            continue;
        }

        const char* uri;
        rv = resource->GetValueConst(&uri);
        if (NS_FAILED(rv)) return rv;

        nsAutoString id;
        rv = gXULUtils->MakeElementID(this, nsAutoString(uri), id);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to compute element ID");
        if (NS_FAILED(rv)) return rv;

        rv = GetElementsForID(id, array);
        if (NS_FAILED(rv)) return rv;

        PRUint32 cnt;
        rv = array->Count(&cnt);
        if (NS_FAILED(rv)) return rv;

        if (! cnt)
            continue;

        rv = ApplyPersistentAttributesToElements(resource, array);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


nsresult
nsXULDocument::ApplyPersistentAttributesToElements(nsIRDFResource* aResource, nsISupportsArray* aElements)
{
    nsresult rv;

    nsCOMPtr<nsISimpleEnumerator> attrs;
    rv = mLocalStore->ArcLabelsOut(aResource, getter_AddRefs(attrs));
    if (NS_FAILED(rv)) return rv;

    while (1) {
        PRBool hasmore;
        rv = attrs->HasMoreElements(&hasmore);
        if (NS_FAILED(rv)) return rv;

        if (! hasmore)
            break;

        nsCOMPtr<nsISupports> isupports;
        rv = attrs->GetNext(getter_AddRefs(isupports));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
        if (! property) {
            NS_WARNING("expected a resource");
            continue;
        }

        const char* attrname;
        rv = property->GetValueConst(&attrname);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIAtom> attr = dont_AddRef(NS_NewAtom(attrname));
        if (! attr)
            return NS_ERROR_OUT_OF_MEMORY;

        // XXX could hang namespace off here, as well...

        nsCOMPtr<nsIRDFNode> node;
        rv = mLocalStore->GetTarget(aResource, property, PR_TRUE, getter_AddRefs(node));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(node);
        if (! literal) {
            NS_WARNING("expected a literal");
            continue;
        }

        const PRUnichar* value;
        rv = literal->GetValueConst(&value);
        if (NS_FAILED(rv)) return rv;

        PRInt32 len = nsCRT::strlen(value);
        CBufDescriptor wrapper(value, PR_TRUE, len + 1, len);

        PRUint32 cnt;
        rv = aElements->Count(&cnt);
        if (NS_FAILED(rv)) return rv;
           
        for (PRInt32 i = PRInt32(cnt) - 1; i >= 0; --i) {
            nsISupports* isupports2 = aElements->ElementAt(i);
            if (! isupports2)
                continue;

            nsCOMPtr<nsIContent> element = do_QueryInterface(isupports2);
            NS_RELEASE(isupports2);

            rv = element->SetAttribute(/* XXX */ kNameSpaceID_None,
                                       attr,
                                       nsAutoString(wrapper),
                                       PR_FALSE);
        }
    }

    return NS_OK;
}

