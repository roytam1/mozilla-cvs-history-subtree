/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Seth Spitzer <sspitzer@netscape.com>
 *   Scott MacGregor <mscott@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */

#include "msgCore.h"    // precompiled header...
#include "nntpCore.h"
#include "nsMsgNewsCID.h"
#include "nsINntpUrl.h"
#include "nsNNTPProtocol.h"
#include "nsNNTPNewsgroupPost.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIdentity.h"
#include "nsString.h"
#include "nsNewsUtils.h"
#include "nsNewsDatabase.h"
#include "nsMsgDBCID.h"
#include "nsMsgBaseCID.h"
#include "nsMsgNewsCID.h"
#include "nsIPref.h"
#include "nsCRT.h"  // for nsCRT::strtok
#include "nsNntpService.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsCOMPtr.h"
#include "nsIDirectoryService.h"
#include "nsIMsgAccountManager.h"
#include "nsIMessengerMigrator.h"
#include "nsINntpIncomingServer.h"
#include "nsICmdLineHandler.h"
#include "nsICategoryManager.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIMessengerWindowService.h"
#include "nsIMsgSearchSession.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIWebNavigation.h"
#include "nsIIOService.h"
#include "nsIPrompt.h"
#include "nsIRDFService.h"

#undef GetPort  // XXX Windows!
#undef SetPort  // XXX Windows!

#define PREF_NETWORK_HOSTS_NNTP_SERVER	"network.hosts.nntp_server"
#define PREF_MAIL_ROOT_NNTP 	"mail.root.nntp"

static NS_DEFINE_CID(kCNntpUrlCID, NS_NNTPURL_CID);
static NS_DEFINE_CID(kCNewsDB, NS_NEWSDB_CID);
static NS_DEFINE_CID(kCNNTPNewsgroupPostCID, NS_NNTPNEWSGROUPPOST_CID);
static NS_DEFINE_CID(kCPrefServiceCID, NS_PREF_CID); 
static NS_DEFINE_CID(kMsgAccountManagerCID, NS_MSGACCOUNTMANAGER_CID);
static NS_DEFINE_CID(kMessengerMigratorCID, NS_MESSENGERMIGRATOR_CID);
static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
                    
nsNntpService::nsNntpService()
{
    NS_INIT_REFCNT();
    mPrintingOperation = PR_FALSE;
	mOpenAttachmentOperation = PR_FALSE;
}

nsNntpService::~nsNntpService()
{
	// do nothing
}

NS_IMPL_THREADSAFE_ADDREF(nsNntpService);
NS_IMPL_THREADSAFE_RELEASE(nsNntpService);

NS_IMPL_QUERY_INTERFACE6(nsNntpService,
                         nsINntpService,
                         nsIMsgMessageService,
                         nsIProtocolHandler,
                         nsIMsgProtocolInfo,
                         nsICmdLineHandler,
						 nsIContentHandler)

////////////////////////////////////////////////////////////////////////////////////////
// nsIMsgMessageService support
////////////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP 
nsNntpService::SaveMessageToDisk(const char *aMessageURI, 
                                 nsIFileSpec *aFile, 
                                 PRBool aAddDummyEnvelope, 
                                 nsIUrlListener *aUrlListener, 
                                 nsIURI **aURL,
                                 PRBool canonicalLineEnding,
								 nsIMsgWindow *aMsgWindow)
{
    nsresult rv = NS_OK;
    NS_ENSURE_ARG_POINTER(aMessageURI);
 
    // double check it is a news_message:/ uri   
    if (PL_strncmp(aMessageURI, kNewsMessageRootURI, kNewsMessageRootURILen)) {
        rv = NS_ERROR_UNEXPECTED;
        NS_ENSURE_SUCCESS(rv,rv);
    }

    nsCOMPtr<nsIURI> url;

    rv = ConstructNntpUrl(aMessageURI, aUrlListener, aMsgWindow, getter_AddRefs(url));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(url);
    nsCOMPtr<nsIMsgMessageUrl> msgUrl = do_QueryInterface(url);
    nntpUrl->SetNewsAction(nsINntpUrl::ActionSaveMessageToDisk);
    if (msgUrl) {
		    msgUrl->SetMessageFile(aFile);
        msgUrl->SetAddDummyEnvelope(aAddDummyEnvelope);
        msgUrl->SetCanonicalLineEnding(canonicalLineEnding);
    }   
    
    rv = RunNewsUrl(url, nsnull, nsnull);
    NS_ENSURE_SUCCESS(rv,rv);

    if (aURL)
    {
	    *aURL = url;
	    NS_IF_ADDREF(*aURL);
    }

  return rv;
}

NS_IMETHODIMP 
nsNntpService::DisplayMessage(const char* aMessageURI, nsISupports * aDisplayConsumer, 
                                       nsIMsgWindow *aMsgWindow, nsIUrlListener * aUrlListener, const PRUnichar * aCharsetOverride, nsIURI ** aURL)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(aMessageURI);

  nsCAutoString uri(aMessageURI);

  // rhp: If we are displaying this message for the purposes of printing, append
  // the magic operand.
  if (mPrintingOperation)
    uri.Append("?header=print");

  nsCOMPtr<nsIURI> url;
  rv = ConstructNntpUrl(uri.get(), aUrlListener, aMsgWindow, getter_AddRefs(url));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr <nsIMsgFolder> folder;
  nsMsgKey key = nsMsgKey_None;
  rv = DecomposeNewsMessageURI(uri.get(), getter_AddRefs(folder), &key);
  NS_ENSURE_SUCCESS(rv,rv);

  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsINntpUrl> nntpUrl (do_QueryInterface(url));
    nsCOMPtr<nsIMsgMailNewsUrl> msgUrl (do_QueryInterface(nntpUrl));
    msgUrl->SetMsgWindow(aMsgWindow);
    nntpUrl->SetNewsAction(nsINntpUrl::ActionDisplayArticle);
    nsCOMPtr<nsIMsgI18NUrl> i18nurl (do_QueryInterface(msgUrl));

    i18nurl->SetCharsetOverRide(aCharsetOverride);

    PRBool shouldStoreMsgOffline = PR_FALSE;
    PRBool hasMsgOffline = PR_FALSE;

    if (folder)
    {
      nsCOMPtr <nsIMsgNewsFolder> newsFolder = do_QueryInterface(folder);
      if (newsFolder)
      {
        folder->ShouldStoreMsgOffline(key, &shouldStoreMsgOffline);
        folder->HasMsgOffline(key, &hasMsgOffline);
        msgUrl->SetMsgIsInLocalCache(hasMsgOffline);
        if (WeAreOffline())
        {
          if (!hasMsgOffline)
          {
            nsCOMPtr<nsIMsgIncomingServer> server;

            rv = folder->GetServer(getter_AddRefs(server));
            if (server)
              return server->DisplayOfflineMsg(aMsgWindow);
          }
        }
      }
      newsFolder->SetSaveArticleOffline(shouldStoreMsgOffline);
    }

    // now is where our behavior differs....if the consumer is the docshell then we want to 
    // run the url in the webshell in order to display it. If it isn't a docshell then just
    // run the news url like we would any other news url. 
	  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aDisplayConsumer, &rv));
	  if (NS_SUCCEEDED(rv) && docShell) {
		nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
		// DIRTY LITTLE HACK --> if we are opening an attachment we want the docshell to
        // treat this load as if it were a user click event. Then the dispatching stuff will be much
        // happier.
        if (mOpenAttachmentOperation) {
			docShell->CreateLoadInfo(getter_AddRefs(loadInfo));
			loadInfo->SetLoadType(nsIDocShellLoadInfo::loadLink);
		}
	    rv = docShell->LoadURI(url, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE);
	  }
	  else {
		rv = RunNewsUrl(url, aMsgWindow, aDisplayConsumer);
	  }
  }

  if (aURL) {
	  *aURL = url;
	  NS_IF_ADDREF(*aURL);
  }

  return rv;
}

NS_IMETHODIMP 
nsNntpService::FetchMessage(nsIMsgFolder *newsFolder, nsMsgKey key, nsIMsgWindow *aMsgWindow, nsISupports * aConsumer, nsIUrlListener * aUrlListener, nsIURI ** aURL)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(newsFolder);

  // now create a url for the message
  nsCOMPtr<nsIMsgNewsFolder> msgNewsFolder = do_QueryInterface(newsFolder);
  if (!msgNewsFolder)
    return NS_ERROR_INVALID_ARG;

  nsCOMPtr<nsIMsgDatabase> db;
  nsCOMPtr<nsIMsgDBHdr> msgHdr;

  rv = newsFolder->GetMsgDatabase(aMsgWindow, getter_AddRefs(db));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->GetMsgHdrForKey(key, getter_AddRefs(msgHdr));
  if (!NS_SUCCEEDED(rv) || !msgHdr)
    return NS_ERROR_INVALID_ARG;

  nsXPIDLCString uri;
  newsFolder->GetUriForMsg(msgHdr, getter_Copies(uri));
  nsCOMPtr<nsIURI> url;

  rv = ConstructNntpUrl((const char *)uri, aUrlListener, aMsgWindow, getter_AddRefs(url));
  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsINntpUrl> nntpUrl (do_QueryInterface(url));
    nsCOMPtr<nsIMsgMailNewsUrl> msgUrl (do_QueryInterface(nntpUrl));
    msgUrl->SetMsgWindow(aMsgWindow);
    nntpUrl->SetNewsAction(nsINntpUrl::ActionDisplayArticle);

		rv = RunNewsUrl(url, aMsgWindow, aConsumer);
  }
  if (aURL) 
  {
	  *aURL = url;
	  NS_IF_ADDREF(*aURL);
  }

  return rv;
}

NS_IMETHODIMP nsNntpService::OpenAttachment(const char *aContentType, 
                                            const char *aFileName,
                                            const char *aUrl, 
                                            const char *aMessageUri, 
                                            nsISupports *aDisplayConsumer, 
                                            nsIMsgWindow *aMsgWindow, 
                                            nsIUrlListener *aUrlListener)
{
  nsCAutoString partMsgUrl(aMessageUri);
  
  // try to extract the specific part number out from the url string
  partMsgUrl += "?";
  const char *part = PL_strstr(aUrl, "part=");
  partMsgUrl += part;
  partMsgUrl += "&type=";
  partMsgUrl += aContentType;
  mOpenAttachmentOperation = PR_TRUE;
  nsresult rv = DisplayMessage(partMsgUrl, aDisplayConsumer,
                      aMsgWindow, aUrlListener, nsnull, nsnull);
  mOpenAttachmentOperation = PR_FALSE;
  return rv;
}

NS_IMETHODIMP nsNntpService::GetUrlForUri(const char *aMessageURI, nsIURI **aURL, nsIMsgWindow *aMsgWindow) 
{
  nsresult rv = NS_OK;
   
  NS_ENSURE_ARG_POINTER(aMessageURI);

  // double check that it is a news_message:/ uri
  if (PL_strncmp(aMessageURI, kNewsMessageRootURI, kNewsMessageRootURILen) == 0)
  {
      rv = ConstructNntpUrl(aMessageURI, nsnull, aMsgWindow, aURL);
      NS_ENSURE_SUCCESS(rv,rv);
  }
  else {
    rv = NS_ERROR_UNEXPECTED;
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;

}

NS_IMETHODIMP
nsNntpService::DecomposeNewsURI(const char *uri, nsIMsgFolder **folder, nsMsgKey *aMsgKey)
{
  nsresult rv;

  if (nsCRT::strncmp(uri, kNewsMessageRootURI, kNewsMessageRootURILen) == 0) {
    rv = DecomposeNewsMessageURI(uri, folder, aMsgKey);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  else {
    rv = GetFolderFromUri(uri, folder);
    NS_ENSURE_SUCCESS(rv,rv);
    *aMsgKey = nsMsgKey_None;
  }
  return rv;
}

nsresult
nsNntpService::DecomposeNewsMessageURI(const char * aMessageURI, nsIMsgFolder ** aFolder, nsMsgKey *aMsgKey)
{
    NS_ENSURE_ARG_POINTER(aMessageURI);
    NS_ENSURE_ARG_POINTER(aFolder);
    NS_ENSURE_ARG_POINTER(aMsgKey);

    nsresult rv = NS_OK;
    nsCAutoString folderURI;

    rv = nsParseNewsMessageURI(aMessageURI, folderURI, aMsgKey);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = GetFolderFromUri(folderURI, aFolder);
    NS_ENSURE_SUCCESS(rv,rv);

    return NS_OK;
}

nsresult
nsNntpService::GetFolderFromUri(const char *uri, nsIMsgFolder **folder)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(uri);
    NS_ENSURE_ARG_POINTER(folder);

    nsCOMPtr <nsIRDFService> rdf = do_GetService("@mozilla.org/rdf/rdf-service;1",&rv);
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr<nsIRDFResource> res;
    if (PL_strchr (uri, '@') || PL_strstr(uri,"%40")) {
      // this is a message id url, so we want to get the server for the uri.
      nsCAutoString serverURI(uri);
      PRInt32 pos = serverURI.RFindChar('/');
      serverURI.Cut(pos, serverURI.Length() - pos);

      rv = rdf->GetResource(serverURI.get(), getter_AddRefs(res));
    }
    else {
      rv = rdf->GetResource(uri, getter_AddRefs(res));
    }
    NS_ENSURE_SUCCESS(rv,rv);

    rv = res->QueryInterface(NS_GET_IID(nsIMsgFolder), (void **)folder);
    NS_ENSURE_SUCCESS(rv,rv);
    return NS_OK;
}


NS_IMETHODIMP
nsNntpService::CopyMessage(const char * aSrcMailboxURI, nsIStreamListener * aMailboxCopyHandler, PRBool moveMessage,
						   nsIUrlListener * aUrlListener, nsIMsgWindow *aMsgWindow, nsIURI **aURL)
{
    nsresult rv = NS_ERROR_NULL_POINTER;
    nsCOMPtr<nsISupports> streamSupport;
    if (!aSrcMailboxURI || !aMailboxCopyHandler) return rv;
    streamSupport = do_QueryInterface(aMailboxCopyHandler, &rv);
    if (NS_SUCCEEDED(rv))
        rv = DisplayMessage(aSrcMailboxURI, streamSupport, aMsgWindow, aUrlListener, nsnull, aURL);
	return rv;
}

NS_IMETHODIMP
nsNntpService::CopyMessages(nsMsgKeyArray *keys, nsIMsgFolder *srcFolder, nsIStreamListener * aMailboxCopyHandler, PRBool moveMessage,
						   nsIUrlListener * aUrlListener, nsIMsgWindow *aMsgWindow, nsIURI **aURL)
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

typedef struct _findNewsServerEntry {
  const char *newsgroup;
  nsINntpIncomingServer *server;
} findNewsServerEntry;


PRBool 
nsNntpService::findNewsServerWithGroup(nsISupports *aElement, void *data)
{
	nsresult rv;

	nsCOMPtr<nsINntpIncomingServer> newsserver = do_QueryInterface(aElement, &rv);
	if (NS_FAILED(rv) || ! newsserver) return PR_TRUE;

	findNewsServerEntry *entry = (findNewsServerEntry*) data;

	PRBool containsGroup = PR_FALSE;

	rv = newsserver->ContainsNewsgroup((const char *)(entry->newsgroup), &containsGroup);
	if (NS_FAILED(rv)) return PR_TRUE;

	if (containsGroup) {	
		entry->server = newsserver;
		return PR_FALSE;            // stop on first find
	}
	else {
		return PR_TRUE;
	}
}

nsresult
nsNntpService::FindServerWithNewsgroup(nsCString &host, nsCString &groupName)
{
	nsresult rv;

    nsCOMPtr <nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
	nsCOMPtr<nsISupportsArray> servers;
	
	rv = accountManager->GetAllServers(getter_AddRefs(servers));
    NS_ENSURE_SUCCESS(rv,rv);

	findNewsServerEntry serverInfo;
	serverInfo.server = nsnull;
  	serverInfo.newsgroup = (const char *)groupName;

#ifdef DEBUG_seth
    printf("this only looks at the list of subscribed newsgroups.  fix to use the hostinfo.dat information\n");
#endif

	servers->EnumerateForwards(findNewsServerWithGroup, (void *)&serverInfo);
	if (serverInfo.server) {
		nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(serverInfo.server);
		nsXPIDLCString thisHostname;
  		rv = server->GetHostName(getter_Copies(thisHostname));
        NS_ENSURE_SUCCESS(rv,rv);

		host = (const char *)thisHostname;
	}
    
    return NS_OK;
}

nsresult nsNntpService::FindHostFromGroup(nsCString &host, nsCString &groupName)
{
  nsresult rv = NS_OK;
  // host always comes in as ""
  NS_ASSERTION(host.IsEmpty(), "host is not empty");
  if (!host.IsEmpty()) return NS_ERROR_FAILURE;
 
  rv = FindServerWithNewsgroup(host, groupName);
  NS_ENSURE_SUCCESS(rv,rv);

  // host can be empty
  return NS_OK;
}

nsresult 
nsNntpService::SetUpNntpUrlForPosting(nsINntpUrl *nntpUrl, const char *newsgroupsNames, const char *newshost, char **newsUrlSpec)
{
  nsresult rv = NS_OK;
  nsCAutoString host;

  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;

  // newsgroupsNames can be a comma seperated list of these:
  // news://host/group
  // news://group
  // host/group
  // group

  //nsCRT::strtok is going destroy what we pass to it, so we need to make a copy of newsgroupsNames.
  char *list = nsCRT::strdup(newsgroupsNames);
  char *token = nsnull;
  char *rest = list;
  nsCAutoString str;
  PRUint32 numGroups = 0;   // the number of newsgroup we are attempt to post to
  nsCAutoString currentGroup;

  token = nsCRT::strtok(rest, ",", &rest);
  while (token && *token) {
    str = token;
    str.StripWhitespace();

    if (!str.IsEmpty()) {
      nsCAutoString theRest;
      nsCAutoString currentHost;
      
      // does str start with "news:/"?
      if (str.Find(kNewsRootURI) == 0) {
        // we have news://group or news://host/group
        // set theRest to what's after news://
        str.Right(theRest, str.Length() - kNewsRootURILen /* for news:/ */ - 1 /* for the slash */);
      }
      else if (str.Find(":/") != -1) {
        // we have x:/y where x != news. this is bad, return failure
        CRTFREEIF(list);
        return NS_ERROR_FAILURE;
      }
      else {
        theRest = str;
      }
      
      // theRest is "group" or "host/group"
      PRInt32 slashpos = theRest.FindChar('/');
      if (slashpos > 0 ) {
        // theRest is "host/group"
        theRest.Left(currentHost, slashpos);
        theRest.Right(currentGroup, slashpos);
      }
      else if (newshost && nsCRT::strlen(newshost) > 0) {
        currentHost.Assign(newshost);
      }
      else {
        // str is "group"
        rv = FindHostFromGroup(currentHost, str);
        currentGroup = str;
        if (NS_FAILED(rv)) {
          CRTFREEIF(list);
		  return rv;
	    }
      }

      numGroups++;
      if (!currentHost.IsEmpty()) {
        if (host.IsEmpty()) {
          host = currentHost;
        }
        else {
          if (!host.Equals(currentHost)) {
            // yikes, we are trying to cross post
            CRTFREEIF(list);
            return NS_ERROR_NNTP_NO_CROSS_POSTING;
          }
        }
      }
      
      str = "";
      currentHost = "";
    }
    token = nsCRT::strtok(rest, ",", &rest);
  }    
  CRTFREEIF(list);
  
  // if we don't have a news host, find the first news server and use it
  if (host.IsEmpty()) {
    nsCOMPtr<nsIMsgIncomingServer> server;
    nsCOMPtr <nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = accountManager->FindServer("","","nntp", getter_AddRefs(server));
    if (NS_SUCCEEDED(rv) && server) {
        nsXPIDLCString newsHostName;
        rv = server->GetHostName(getter_Copies(newsHostName));
        if (NS_SUCCEEDED(rv)) {
            host = (const char *)newsHostName;
        }
    }
  }

  // if we *still* don't have a hostname, use "news"
  if (host.IsEmpty()) {
    host = "news";
  }

  *newsUrlSpec = PR_smprintf("%s/%s",kNewsRootURI,(const char *)host);
  if (!*newsUrlSpec) return NS_ERROR_FAILURE;

  return NS_OK;
}
////////////////////////////////////////////////////////////////////////////////////////
// nsINntpService support
////////////////////////////////////////////////////////////////////////////////////////
NS_IMETHODIMP
nsNntpService::ConvertNewsgroupsString(const char *newsgroupsNames, char **_retval)
{
  nsresult rv = NS_OK;
  
  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;

#ifdef DEBUG_NEWS
  printf("newsgroupsNames == %s\n",newsgroupsNames);
#endif
  
  // newsgroupsNames can be a comma seperated list of these:
  // news://host/group
  // news://group
  // host/group
  // group
  //
  // we are not going to allow the user to cross post to multiple hosts.
  // if we detect that, we stop and return error.

  // nsCRT::strtok is going destroy what we pass to it, so we need to make a copy of newsgroupsNames.
  char *list = nsCRT::strdup(newsgroupsNames);
  char *token = nsnull;
  char *rest = list;
  nsCAutoString host;
  nsCAutoString str;
  nsCAutoString retvalStr;
    
  token = nsCRT::strtok(rest, ",", &rest);
  while (token && *token) {
    str = token;
    str.StripWhitespace();

    if (!str.IsEmpty()) {
      nsCAutoString currentHost;
      nsCAutoString theRest;

      // does str start with "news:/"?
      if (str.Find(kNewsRootURI) == 0) {
        // we have news://group or news://host/group
        // set theRest to what's after news://
        str.Right(theRest, str.Length() - kNewsRootURILen /* for news:/ */ - 1 /* for the slash */);
      }
      else if (str.Find(":/") != -1) {
        // we have x:/y where x != news. this is bad, return failure
        CRTFREEIF(list);
        return NS_ERROR_FAILURE;
      }
      else {
        theRest = str;
      }
      
      // theRest is "group" or "host/group"
      PRInt32 slashpos = theRest.FindChar('/');
      if (slashpos > 0 ) {
        nsCAutoString currentGroup;
        
        // theRest is "host/group"
        theRest.Left(currentHost, slashpos);
        
        // from "host/group", put "group" into currentGroup;
        theRest.Right(currentGroup, theRest.Length() - currentHost.Length() - 1);

        NS_ASSERTION(!currentGroup.IsEmpty(), "currentGroup is empty");
        if (currentGroup.IsEmpty()) {
          CRTFREEIF(list);
          return NS_ERROR_FAILURE;
        }
        
        // build up the retvalStr;
        if (!retvalStr.IsEmpty()) {
          retvalStr += ",";
        }
        retvalStr += currentGroup;
      }
      else {
        // str is "group"
        rv = FindHostFromGroup(currentHost, str);
        if (NS_FAILED(rv)) {
            CRTFREEIF(list);
            return rv;
        }

        // build up the retvalStr;
        if (!retvalStr.IsEmpty()) {
          retvalStr += ",";
        }
        retvalStr += str;
      }

      if (!currentHost.IsEmpty()) {
        if (host.IsEmpty()) {
          host = currentHost;
        }
        else {
          if (!host.Equals(currentHost)) {
            CRTFREEIF(list);
            return NS_ERROR_NNTP_NO_CROSS_POSTING;
          }
        }
      }

      str = "";
      currentHost = "";
    }
    token = nsCRT::strtok(rest, ",", &rest);
  }
  CRTFREEIF(list);
  
  *_retval = nsCRT::strdup(retvalStr.GetBuffer());
  if (!*_retval) return NS_ERROR_OUT_OF_MEMORY;
  
  return NS_OK;
}

NS_IMETHODIMP
nsNntpService::PostMessage(nsIFileSpec *fileToPost, const char *newsgroupsNames, const char *newshost, nsIUrlListener * aUrlListener, nsIMsgWindow *aMsgWindow, nsIURI **_retval)
{
#ifdef DEBUG_NEWS
  printf("nsNntpService::PostMessage(??,%s,??,??)\n",newsgroupsNames);
#endif
  if (!aMsgWindow) return NS_ERROR_NULL_POINTER;
  if (!newsgroupsNames) return NS_ERROR_NULL_POINTER;
  if (PL_strlen(newsgroupsNames) == 0) return NS_ERROR_FAILURE;
    
  NS_LOCK_INSTANCE();
  
  nsCOMPtr <nsINntpUrl> nntpUrl;
  nsresult rv = NS_OK;
  
  rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, NS_GET_IID(nsINntpUrl), getter_AddRefs(nntpUrl));
  if (NS_FAILED(rv) || !nntpUrl) return rv;

  nntpUrl->SetNewsAction(nsINntpUrl::ActionPostArticle);

  nsXPIDLCString newsUrlSpec;
  rv = SetUpNntpUrlForPosting(nntpUrl, newsgroupsNames, newshost, getter_Copies(newsUrlSpec));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(nntpUrl);
  if (!mailnewsurl) return NS_ERROR_FAILURE;

  mailnewsurl->SetSpec((const char *)newsUrlSpec);
  
  if (aUrlListener) // register listener if there is one...
    mailnewsurl->RegisterListener(aUrlListener);
  
  // almost there...now create a nntp protocol instance to run the url in...
  nsCOMPtr <nsIURI> nntpURI = do_QueryInterface(nntpUrl);;
  nsCOMPtr<nsINNTPProtocol> nntpProtocol ;
  rv = GetProtocolForUri(nntpURI, aMsgWindow, getter_AddRefs(nntpProtocol));

  if (!nntpProtocol) return NS_ERROR_OUT_OF_MEMORY;
  
  rv = nntpProtocol->Initialize(mailnewsurl, aMsgWindow);
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr <nsINNTPNewsgroupPost> post;
  rv = nsComponentManager::CreateInstance(kCNNTPNewsgroupPostCID, nsnull, NS_GET_IID(nsINNTPNewsgroupPost), getter_AddRefs(post));
  if (NS_FAILED(rv) || !post) return rv;

  rv = post->SetPostMessageFile(fileToPost);
  if (NS_FAILED(rv)) return rv;
  
  rv = nntpUrl->SetMessageToPost(post);
  if (NS_FAILED(rv)) return rv;
            
  rv = nntpProtocol->LoadNewsUrl(mailnewsurl, /* aConsumer */ nsnull);
		
  if (_retval)
	  nntpUrl->QueryInterface(NS_GET_IID(nsIURI), (void **) _retval);
    
  NS_UNLOCK_INSTANCE();

  return rv;
}

nsresult 
nsNntpService::ConstructNntpUrl(const char * urlString, nsIUrlListener *aUrlListener, nsIMsgWindow * aMsgWindow, nsIURI ** aUrl)
{
  nsCOMPtr <nsINntpUrl> nntpUrl;
  nsresult rv = NS_OK;

  rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, NS_GET_IID(nsINntpUrl), getter_AddRefs(nntpUrl));
  if (NS_FAILED(rv) || !nntpUrl) return rv;
  
  nsCOMPtr <nsIMsgMailNewsUrl> mailnewsurl = do_QueryInterface(nntpUrl);
  mailnewsurl->SetMsgWindow(aMsgWindow);
  nsCOMPtr <nsIMsgMessageUrl> msgUrl = do_QueryInterface(nntpUrl);
  msgUrl->SetUri(urlString);
  mailnewsurl->SetSpec(urlString);

  if (aUrlListener) // register listener if there is one...
    mailnewsurl->RegisterListener(aUrlListener);

  (*aUrl) = mailnewsurl;
  NS_IF_ADDREF(*aUrl);
  return rv;

}

nsresult
nsNntpService::CreateNewsAccount(const char *username, const char *hostname, PRBool isSecure, PRInt32 port, nsIMsgIncomingServer **server)
{
	nsresult rv;
	// username can be null.
	if (!hostname || !server) return NS_ERROR_NULL_POINTER;
	
	nsCOMPtr <nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

	nsCOMPtr <nsIMsgAccount> account;
	rv = accountManager->CreateAccount(getter_AddRefs(account));
	if (NS_FAILED(rv)) return rv;

	rv = accountManager->CreateIncomingServer(username, hostname, "nntp", server);
	if (NS_FAILED(rv)) return rv;

	rv = (*server)->SetIsSecure(isSecure);
	if (NS_FAILED(rv)) return rv;
	
	rv = (*server)->SetPort(port);
	if (NS_FAILED(rv)) return rv;

	nsCOMPtr <nsIMsgIdentity> identity;
	rv = accountManager->CreateIdentity(getter_AddRefs(identity));
	if (NS_FAILED(rv)) return rv;
	if (!identity) return NS_ERROR_FAILURE;

    // by default, news accounts should be composing in plain text
    rv = identity->SetComposeHtml(PR_FALSE);
    NS_ENSURE_SUCCESS(rv,rv);

	// the identity isn't filled in, so it is not valid.
	rv = (*server)->SetValid(PR_FALSE);
	if (NS_FAILED(rv)) return rv;

	// hook them together
	rv = account->SetIncomingServer(*server);
	if (NS_FAILED(rv)) return rv;
	rv = account->AddIdentity(identity);
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}

nsresult
nsNntpService::GetProtocolForUri(nsIURI *aUri, nsIMsgWindow *aMsgWindow, nsINNTPProtocol **aProtocol)
{
  nsXPIDLCString hostName;
  nsXPIDLCString userName;
  nsXPIDLCString scheme;
  nsXPIDLCString path;
  PRInt32 port = 0;
  nsresult rv;
  
  rv = aUri->GetHost(getter_Copies(hostName));
  rv = aUri->GetPreHost(getter_Copies(userName));
  rv = aUri->GetScheme(getter_Copies(scheme));
  rv = aUri->GetPort(&port);
  rv = aUri->GetPath(getter_Copies(path));

  nsCOMPtr <nsIMsgAccountManager> accountManager = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // find the incoming server, it if exists.
  // migrate if necessary, before searching for it.
  // if it doesn't exist, create it.
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsCOMPtr<nsINntpIncomingServer> nntpServer;

#ifdef DEBUG_sspitzer
  printf("for bug, #36661, see if there are any accounts, if not, try migrating.  should this be pushed into FindServer()?\n");
#endif
  nsCOMPtr <nsISupportsArray> accounts;
  rv = accountManager->GetAccounts(getter_AddRefs(accounts));
  if (NS_FAILED(rv)) return rv;

  PRUint32 accountCount;
  rv = accounts->Count(&accountCount);
  if (NS_FAILED(rv)) return rv;

  if (accountCount == 0) {
	nsCOMPtr <nsIMessengerMigrator> messengerMigrator = do_GetService(kMessengerMigratorCID, &rv);
    if (NS_FAILED(rv)) return rv;
	if (!messengerMigrator) return NS_ERROR_FAILURE;

	// migration can fail;
 	messengerMigrator->UpgradePrefs(); 
  }

  // news:group becomes news://group, so we have three types of urls:
  // news://group       (autosubscribing without a host)
  // news://host/group  (autosubscribing with a host)
  // news://host        (updating the unread message counts on a server)
  //
  // first, check if hostName is really a server or a group
  // by looking for a server with hostName
  //
  // xxx todo what if we have two servers on the same host, but different ports?
  // or no port, but isSecure (snews:// vs news://) is different?
  rv = accountManager->FindServer((const char *)userName,
                                (const char *)hostName,
                                "nntp",
                                getter_AddRefs(server));

  // if we didn't find the server, and path was "/", this is a news://group url
  if (!server && !(nsCRT::strcmp("/",(const char *)path))) {
    // the uri was news://group and we want to turn that into news://host/group
    // step 1, set the path to be the hostName;
    rv = aUri->SetPath((const char *)hostName);
    NS_ENSURE_SUCCESS(rv,rv);

    // until we support default news servers, use the first nntp server we find
    rv = accountManager->FindServer("","","nntp", getter_AddRefs(server));
    if (NS_FAILED(rv) || !server) {
        // step 2, set the uri's hostName and the local variable hostName
        // to be "news"
        rv = aUri->SetHost("news");
        NS_ENSURE_SUCCESS(rv,rv);

        rv = aUri->GetHost(getter_Copies(hostName));
        NS_ENSURE_SUCCESS(rv,rv);
    }
    else {
        // step 2, set the uri's hostName and the local variable hostName
        // to be the host name of the server we found
        rv = server->GetHostName(getter_Copies(hostName));
        NS_ENSURE_SUCCESS(rv,rv);
    
        rv = aUri->SetHost((const char *)hostName);
        NS_ENSURE_SUCCESS(rv,rv);
    }
  }

  if (NS_FAILED(rv) || !server) {
	  PRBool isSecure = PR_FALSE;
	  if (nsCRT::strcasecmp("snews",(const char *)scheme) == 0) {
		  isSecure = PR_TRUE;
          if ((port == 0) || (port == -1)) {
              port = SECURE_NEWS_PORT;
          }
	  }
	  rv = CreateNewsAccount((const char *)userName,(const char *)hostName,isSecure,port,getter_AddRefs(server));
  }
   
  if (NS_FAILED(rv)) return rv;
  if (!server) return NS_ERROR_FAILURE;
  
  nntpServer = do_QueryInterface(server, &rv);

  if (!nntpServer || NS_FAILED(rv))
    return rv;

  rv = nntpServer->GetNntpConnection(aUri, aMsgWindow, aProtocol);
  if (NS_FAILED(rv) || !*aProtocol) 
    return NS_ERROR_OUT_OF_MEMORY;
  return rv;
}

PRBool nsNntpService::WeAreOffline()
{
	nsresult rv = NS_OK;
  PRBool offline = PR_FALSE;

  NS_WITH_SERVICE(nsIIOService, netService, kIOServiceCID, &rv);
  if (NS_SUCCEEDED(rv) && netService)
  {
    netService->GetOffline(&offline);
  }
  return offline;
}

nsresult 
nsNntpService::RunNewsUrl(nsIURI * aUri, nsIMsgWindow *aMsgWindow, nsISupports * aConsumer)
{
  nsresult rv;

  if (WeAreOffline())
    return NS_MSG_ERROR_OFFLINE;

  // almost there...now create a nntp protocol instance to run the url in...
  nsCOMPtr <nsINNTPProtocol> nntpProtocol;
  rv = GetProtocolForUri(aUri, aMsgWindow, getter_AddRefs(nntpProtocol));

  if (NS_SUCCEEDED(rv))
    rv = nntpProtocol->Initialize(aUri, aMsgWindow);
  if (NS_FAILED(rv)) return rv;
  
  rv = nntpProtocol->LoadNewsUrl(aUri, aConsumer);
  return rv;
}

NS_IMETHODIMP nsNntpService::GetNewNews(nsINntpIncomingServer *nntpServer, const char *uri, PRBool aGetOld, nsIUrlListener * aUrlListener, nsIMsgWindow *aMsgWindow, nsIURI **_retval)
{
  if (!uri) return NS_ERROR_NULL_POINTER;

  NS_LOCK_INSTANCE();
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIMsgIncomingServer> server;
  server = do_QueryInterface(nntpServer);
 
  /* double check that it is a "news:/" url */
  if (nsCRT::strncmp(uri, kNewsRootURI, kNewsRootURILen) == 0) {
    nsCOMPtr<nsIURI> aUrl;
    rv = ConstructNntpUrl(uri, aUrlListener, aMsgWindow, getter_AddRefs(aUrl));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(aUrl);
    if (nntpUrl) {
		  rv = nntpUrl->SetNewsAction(nsINntpUrl::ActionGetNewNews);
		  if (NS_FAILED(rv)) return rv;
		
		  rv = nntpUrl->SetGetOldMessages(aGetOld);
		  if (NS_FAILED(rv)) return rv;
	  }

	  nsCOMPtr<nsIMsgMailNewsUrl> mailNewsUrl = do_QueryInterface(aUrl);
	  if (mailNewsUrl) {
      mailNewsUrl->SetUpdatingFolder(PR_TRUE);
      mailNewsUrl->SetMsgWindow(aMsgWindow);
	  }

    rv = RunNewsUrl(aUrl, aMsgWindow, nsnull);  
	
    if (_retval) {
      *_retval = aUrl;
      NS_IF_ADDREF(*_retval);
    }
  }
  else {
    NS_ASSERTION(0,"not a news:/ url");
    rv = NS_ERROR_FAILURE;
  }
  
      
  NS_UNLOCK_INSTANCE();
  return rv;
}

NS_IMETHODIMP 
nsNntpService::CancelMessage(const char *uri, nsISupports * aConsumer, nsIUrlListener * aUrlListener, nsIMsgWindow *aMsgWindow, nsIURI ** aURL)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(uri);

  nsCOMPtr<nsIURI> url;
  // the url should have "?cancel" already on it
  rv = ConstructNntpUrl(uri, aUrlListener,  aMsgWindow, getter_AddRefs(url));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsINntpUrl> nntpUrl = do_QueryInterface(url);
  if (nntpUrl) {
	  nntpUrl->SetNewsAction(nsINntpUrl::ActionCancelArticle);
  }

  rv = RunNewsUrl(url, aMsgWindow, aConsumer);  
  NS_ENSURE_SUCCESS(rv,rv);

  if (aURL) {
    *aURL = url;
    NS_IF_ADDREF(*aURL);
  }

  return rv; 
}

NS_IMETHODIMP nsNntpService::GetScheme(char * *aScheme)
{
	nsresult rv = NS_OK;
	if (aScheme)
		*aScheme = nsCRT::strdup("news");
	else
		rv = NS_ERROR_NULL_POINTER;
	return rv; 
}

NS_IMETHODIMP nsNntpService::GetDefaultDoBiff(PRBool *aDoBiff)
{
    NS_ENSURE_ARG_POINTER(aDoBiff);
    // by default, don't do biff for NNTP servers
    *aDoBiff = PR_FALSE;    
    return NS_OK;
}

NS_IMETHODIMP nsNntpService::GetDefaultPort(PRInt32 *aDefaultPort)
{
    NS_ENSURE_ARG_POINTER(aDefaultPort);
    *aDefaultPort = NEWS_PORT;
	return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetDefaultServerPort(PRBool isSecure, PRInt32 *aDefaultPort)
{
    nsresult rv = NS_OK;

    // Return Secure NNTP Port if secure option chosen i.e., if isSecure is TRUE
    if (isSecure)
        *aDefaultPort = SECURE_NEWS_PORT;
    else
        rv = GetDefaultPort(aDefaultPort);
 
    return rv;
}

NS_IMETHODIMP nsNntpService::NewURI(const char *aSpec, nsIURI *aBaseURI, nsIURI **_retval)
{
	nsresult rv = NS_OK;

	nsCOMPtr<nsINntpUrl> nntpUrl;
	rv = nsComponentManager::CreateInstance(kCNntpUrlCID, nsnull, NS_GET_IID(nsINntpUrl), getter_AddRefs(nntpUrl));
	if (NS_FAILED(rv)) return rv;
	nntpUrl->SetNewsAction(nsINntpUrl::ActionDisplayArticle);

	nntpUrl->QueryInterface(NS_GET_IID(nsIURI), (void **) _retval);

	(*_retval)->SetSpec(aSpec);
	return rv;
}

NS_IMETHODIMP nsNntpService::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
	nsresult rv = NS_OK;
  nsCOMPtr <nsINNTPProtocol> nntpProtocol;
  rv = GetProtocolForUri(aURI, nsnull, getter_AddRefs(nntpProtocol));
  if (NS_SUCCEEDED(rv))
	  rv = nntpProtocol->Initialize(aURI, nsnull);
  if (NS_FAILED(rv)) return rv;

  return nntpProtocol->QueryInterface(NS_GET_IID(nsIChannel), (void **) _retval);
}

NS_IMETHODIMP
nsNntpService::SetDefaultLocalPath(nsIFileSpec *aPath)
{
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kCPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = prefs->SetFilePref(PREF_MAIL_ROOT_NNTP, aPath, PR_FALSE /* set default */);
    return rv;
}

NS_IMETHODIMP
nsNntpService::GetDefaultLocalPath(nsIFileSpec ** aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;
    
    nsresult rv;
    NS_WITH_SERVICE(nsIPref, prefs, kCPrefServiceCID, &rv);
    if (NS_FAILED(rv)) return rv;
    
    PRBool havePref = PR_FALSE;
    nsCOMPtr<nsIFile> localFile;
    nsCOMPtr<nsILocalFile> prefLocal;
    rv = prefs->GetFileXPref(PREF_MAIL_ROOT_NNTP, getter_AddRefs(prefLocal));
    if (NS_SUCCEEDED(rv)) {
        localFile = prefLocal;
        havePref = PR_TRUE;
    }
    if (!localFile) {
        rv = NS_GetSpecialDirectory(NS_APP_NEWS_50_DIR, getter_AddRefs(localFile));
        if (NS_FAILED(rv)) return rv;
        havePref = PR_FALSE;
    }
        
    PRBool exists;
    rv = localFile->Exists(&exists);
    if (NS_FAILED(rv)) return rv;
    if (!exists) {
        rv = localFile->Create(nsIFile::DIRECTORY_TYPE, 0775);
        if (NS_FAILED(rv)) return rv;
    }
    
    // Make the resulting nsIFileSpec
    // TODO: Convert arg to nsILocalFile and avoid this
    nsXPIDLCString pathBuf;
    rv = localFile->GetPath(getter_Copies(pathBuf));
    if (NS_FAILED(rv)) return rv;
    nsCOMPtr<nsIFileSpec> outSpec;
    rv = NS_NewFileSpec(getter_AddRefs(outSpec));
    if (NS_FAILED(rv)) return rv;
    outSpec->SetNativePath(pathBuf);
    
    if (!havePref || !exists)
        rv = SetDefaultLocalPath(outSpec);
        
    *aResult = outSpec;
    NS_IF_ADDREF(*aResult);
    return rv;
}
    
NS_IMETHODIMP
nsNntpService::GetServerIID(nsIID* *aServerIID)
{
    *aServerIID = new nsIID(NS_GET_IID(nsINntpIncomingServer));
    return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetRequiresUsername(PRBool *aRequiresUsername)
{
        NS_ENSURE_ARG_POINTER(aRequiresUsername);
        *aRequiresUsername = PR_FALSE;
        return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetPreflightPrettyNameWithEmailAddress(PRBool *aPreflightPrettyNameWithEmailAddress)
{
        NS_ENSURE_ARG_POINTER(aPreflightPrettyNameWithEmailAddress);
        *aPreflightPrettyNameWithEmailAddress = PR_FALSE;
        return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetCanLoginAtStartUp(PRBool *aCanLoginAtStartUp)
{
        NS_ENSURE_ARG_POINTER(aCanLoginAtStartUp);
        *aCanLoginAtStartUp = PR_FALSE;
        return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetCanDelete(PRBool *aCanDelete)
{
        NS_ENSURE_ARG_POINTER(aCanDelete);
        *aCanDelete = PR_TRUE;
        return NS_OK;
}

NS_IMETHODIMP
nsNntpService::GetCanDuplicate(PRBool *aCanDuplicate)
{
        NS_ENSURE_ARG_POINTER(aCanDuplicate);
        *aCanDuplicate = PR_TRUE;
        return NS_OK;
}        

NS_IMETHODIMP
nsNntpService::GetCanGetMessages(PRBool *aCanGetMessages)
{
    NS_ENSURE_ARG_POINTER(aCanGetMessages);
    *aCanGetMessages = PR_FALSE;
    return NS_OK;
}  

NS_IMETHODIMP
nsNntpService::GetDefaultCopiesAndFoldersPrefsToServer(PRBool *aDefaultCopiesAndFoldersPrefsToServer)
{
	NS_ENSURE_ARG_POINTER(aDefaultCopiesAndFoldersPrefsToServer);
	// when a news account is created, the copies and folder prefs for the associated identity
	// don't point to folders on the server. 
	// this makes sense, since there is no "Drafts" folder on a news server.
	// they'll point to the ones on "Local Folders"
	*aDefaultCopiesAndFoldersPrefsToServer = PR_FALSE;
    return NS_OK;
}

//
// rhp: Right now, this is the same as simple DisplayMessage, but it will change
// to support print rendering.
//
NS_IMETHODIMP nsNntpService::DisplayMessageForPrinting(const char* aMessageURI, nsISupports * aDisplayConsumer, 
                                                  nsIMsgWindow *aMsgWindow, nsIUrlListener * aUrlListener, nsIURI ** aURL)
{
  mPrintingOperation = PR_TRUE;
  nsresult rv = DisplayMessage(aMessageURI, aDisplayConsumer, aMsgWindow, aUrlListener, nsnull, aURL);
  mPrintingOperation = PR_FALSE;
  return rv;
}

NS_IMETHODIMP nsNntpService::Search(nsIMsgSearchSession *aSearchSession, nsIMsgWindow *aMsgWindow, nsIMsgFolder *aMsgFolder, const char *aSearchUri)
{
  NS_ENSURE_ARG(aMsgFolder);
  nsCOMPtr <nsIMsgIncomingServer> server;
  nsresult rv = aMsgFolder->GetServer(getter_AddRefs(server));
  if (NS_SUCCEEDED(rv) && server)
  {
  	nsCOMPtr<nsIURI> uri;
    nsXPIDLCString serverUri;
    nsXPIDLCString newsgroupName;

    rv = server->GetServerURI(getter_Copies(serverUri));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr <nsIMsgNewsFolder> newsFolder = do_QueryInterface(aMsgFolder, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = newsFolder->GetAsciiName(getter_Copies(newsgroupName));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCAutoString searchUrl((const char *)serverUri);
    searchUrl += aSearchUri;
    nsCOMPtr <nsIUrlListener> urlListener = do_QueryInterface(aSearchSession);

    rv = ConstructNntpUrl(searchUrl.get(), urlListener, aMsgWindow, getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv,rv);

    nsCOMPtr<nsIMsgMailNewsUrl> msgurl (do_QueryInterface(uri));
    if (msgurl)
      msgurl->SetSearchSession(aSearchSession);

    // run the url to update the counts
    rv = RunNewsUrl(uri, nsnull, nsnull);  
    NS_ENSURE_SUCCESS(rv,rv);
  }
  return rv;
}


NS_IMETHODIMP
nsNntpService::UpdateCounts(nsINntpIncomingServer *aNntpServer, nsIMsgWindow *aMsgWindow)
{
	nsresult rv;
#ifdef DEBUG_NEWS
	printf("in UpdateCountsForNewsgroup()\n");
#endif
	if (!aNntpServer) return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsIURI> uri;
	nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(aNntpServer, &rv);
	if (NS_FAILED(rv)) return rv;
	if (!server) return NS_ERROR_FAILURE;

	nsXPIDLCString serverUri;
	rv = server->GetServerURI(getter_Copies(serverUri));
	if (NS_FAILED(rv)) return rv;

	rv = ConstructNntpUrl((const char *)serverUri, nsnull, aMsgWindow, getter_AddRefs(uri));
	if (NS_FAILED(rv)) return rv;

	// run the url to update the counts
    rv = RunNewsUrl(uri, aMsgWindow, nsnull);  
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}

NS_IMETHODIMP 
nsNntpService::GetListOfGroupsOnServer(nsINntpIncomingServer *aNntpServer, nsIMsgWindow *aMsgWindow)
{
	nsresult rv;

  NS_ENSURE_ARG_POINTER(aNntpServer);

	nsCOMPtr<nsIURI> uri;
	nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(aNntpServer, &rv);
	if (NS_FAILED(rv)) return rv;
	if (!server) return NS_ERROR_FAILURE;

	nsXPIDLCString serverUri;
	rv = server->GetServerURI(getter_Copies(serverUri));

	nsCAutoString uriStr;
	uriStr += (const char *)serverUri;
	uriStr += "/*";
		
	nsCOMPtr <nsIUrlListener> listener = do_QueryInterface(aNntpServer, &rv);
	if (NS_FAILED(rv)) return rv;
	if (!listener) return NS_ERROR_FAILURE;
	rv = ConstructNntpUrl((const char *)uriStr, listener, aMsgWindow, getter_AddRefs(uri));
	if (NS_FAILED(rv)) return rv;

	// now run the url to add the rest of the groups
        rv = RunNewsUrl(uri, aMsgWindow, nsnull);
	if (NS_FAILED(rv)) return rv;

	return NS_OK;
}

CMDLINEHANDLER3_IMPL(nsNntpService,"-news","general.startup.news","Start with news.",NS_NEWSSTARTUPHANDLER_CONTRACTID,"News Cmd Line Handler", PR_FALSE,"", PR_TRUE)

NS_IMETHODIMP nsNntpService::GetChromeUrlForTask(char **aChromeUrlForTask) 
{ 
    if (!aChromeUrlForTask) return NS_ERROR_FAILURE; 
	nsresult rv;
	NS_WITH_SERVICE(nsIPref, prefService, kCPrefServiceCID, &rv);
	if (NS_SUCCEEDED(rv))
	{
		PRInt32 layout;
		rv = prefService->GetIntPref("mail.pane_config", &layout);		
		if(NS_SUCCEEDED(rv))
		{
			if(layout == 0)
				*aChromeUrlForTask = PL_strdup("chrome://messenger/content/messenger.xul");
			else
				*aChromeUrlForTask = PL_strdup("chrome://messenger/content/mail3PaneWindowVertLayout.xul");

			return NS_OK;

		}	
	}
	*aChromeUrlForTask = PL_strdup("chrome://messenger/content/messenger.xul"); 
    return NS_OK; 
}



NS_IMETHODIMP 
nsNntpService::HandleContent(const char * aContentType, const char * aCommand, const char * aWindowTarget, nsISupports * aWindowContext, nsIChannel * aChannel)
{
  nsresult rv = NS_OK;
  if (!aChannel) return NS_ERROR_NULL_POINTER;

  if (nsCRT::strcasecmp(aContentType, "x-application-newsgroup") == 0) {
      nsCOMPtr<nsIURI> uri;
      rv = aChannel->GetURI(getter_AddRefs(uri));
	  if (NS_FAILED(rv)) return rv;

      if (uri) { 	
		nsCOMPtr <nsIMessengerWindowService> messengerWindowService = do_GetService(NS_MESSENGERWINDOWSERVICE_CONTRACTID,&rv);
		if (NS_FAILED(rv)) return rv;

		rv = messengerWindowService->OpenMessengerWindowWithUri(uri);
		if (NS_FAILED(rv)) return rv;
	  }
  }

  return rv;
}

NS_IMETHODIMP
nsNntpService::MessageURIToMsgHdr(const char *uri, nsIMsgDBHdr **_retval)
{
  NS_ENSURE_ARG_POINTER(uri);
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_OK;

  nsCOMPtr <nsIMsgFolder> folder;
  nsMsgKey msgKey;

  rv = DecomposeNewsMessageURI(uri, getter_AddRefs(folder), &msgKey);
  NS_ENSURE_SUCCESS(rv,rv);

  rv = folder->GetMessageHeader(msgKey, _retval);
  NS_ENSURE_SUCCESS(rv,rv);
  return NS_OK;
}
