/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Robert John Churchill    <rjc@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
  The remote bookmarks service.
  
  See RFC 2255 for additional LDAP info, including "bindname" extension usage.
 */



#include "nsRemoteBookmarks.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "prprf.h"
#include "prlong.h"
#include "rdf.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsAutoLock.h"
#include "nsRDFCID.h"
#include "nsIEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsILDAPErrors.h"
#include "nsILDAPOperation.h"
#include "nsILDAPURL.h"
#include "nsILDAPConnection.h"
#include "nsILDAPMessage.h"
#include "nsILDAPMessageListener.h"
#include "nsIProxyObjectManager.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"
#include "nsIProxyObjectManager.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"

// #define LDAP_MOD_SUPPORT   1
#ifdef  LDAP_MOD_SUPPORT
#include "nsILDAPModification.h"
#endif

#define REMOTE_BOOKMARK_PREFIX         "moz-bookmark-"
#define REMOTE_BOOKMARK_PREFIX_LENGTH  (sizeof(REMOTE_BOOKMARK_PREFIX)-1)

// Mozilla Schema
#define MOZ_SCHEMA_OBJ_CLASS      "objectclass"
#define MOZ_SCHEMA_BMK_CLASS      "mozillaBookmark"
#define MOZ_SCHEMA_FOLDER_CLASS   "mozillaFolder"
#define MOZ_SCHEMA_URL            "mozillaURL"
#define MOZ_SCHEMA_NAME           "mozillaName"
#define MOZ_SCHEMA_KEYWORD        "mozillaKeyword"
#define MOZ_SCHEMA_DESCRIPTION    "mozillaDesc"

#define NS_LDAPCONNECTION_CONTRACTID   "@mozilla.org/network/ldap-connection;1" 
#define NS_LDAPOPERATION_CONTRACTID    "@mozilla.org/network/ldap-operation;1" 
#define NS_LDAPMESSAGE_CONTRACTID      "@mozilla.org/network/ldap-message;1"
#define NS_LDAPURL_CONTRACTID          "@mozilla.org/network/ldap-url;1"
#ifdef  LDAP_MOD_SUPPORT
#define NS_LDAPMODIFICATION_CONTRACTID "@mozilla.org/network/ldap-modification;1" 
#endif

static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,   NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,       NS_RDFCONTAINERUTILS_CID);



static PRInt32 gRefCnt=0;
static nsIRDFService *gRDF;
static nsIRDFContainerUtils *gRDFC;

nsIRDFResource *nsRemoteBookmarks::kRDF_type;
nsIRDFResource *nsRemoteBookmarks::kNC_Bookmark;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkSeparator;
nsIRDFResource *nsRemoteBookmarks::kNC_Folder;
nsIRDFResource *nsRemoteBookmarks::kNC_Parent;
nsIRDFResource *nsRemoteBookmarks::kNC_Child;
nsIRDFResource *nsRemoteBookmarks::kNC_URL;
nsIRDFResource *nsRemoteBookmarks::kNC_Name;
nsIRDFResource *nsRemoteBookmarks::kNC_ShortcutURL;
nsIRDFResource *nsRemoteBookmarks::kNC_Description;

nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_NewBookmark;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_NewFolder;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_NewSeparator;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_DeleteBookmark;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_DeleteBookmarkFolder;
nsIRDFResource *nsRemoteBookmarks::kNC_BookmarkCommand_DeleteBookmarkSeparator;



////////////////////////////////////////////////////////////////////////
// RemoteBookmarkDataSourceImpl

NS_IMPL_THREADSAFE_QUERY_INTERFACE5(nsRemoteBookmarks,
                                    nsIRemoteBookmarks,
                                    nsIRDFDataSource,
                                    nsIRDFObserver,
                                    nsITimerCallback,
                                    nsILDAPMessageListener)



nsRemoteBookmarks::nsRemoteBookmarks()
	: mInner(nsnull), mTimer(nsnull), mUpdateBatchNest(0)
{
	NS_INIT_REFCNT();
}



nsRemoteBookmarks::~nsRemoteBookmarks()
{
  if (--gRefCnt == 0)
  {
    NS_IF_RELEASE(kRDF_type);
    NS_IF_RELEASE(kNC_Bookmark);
    NS_IF_RELEASE(kNC_BookmarkSeparator);
    NS_IF_RELEASE(kNC_Folder);
    NS_IF_RELEASE(kNC_Parent);
    NS_IF_RELEASE(kNC_Child);
    NS_IF_RELEASE(kNC_URL);
    NS_IF_RELEASE(kNC_Name);
    NS_IF_RELEASE(kNC_ShortcutURL);
    NS_IF_RELEASE(kNC_Description);

    NS_IF_RELEASE(kNC_BookmarkCommand_NewBookmark);
    NS_IF_RELEASE(kNC_BookmarkCommand_NewFolder);
    NS_IF_RELEASE(kNC_BookmarkCommand_NewSeparator);
    NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmark);
    NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmarkFolder);
    NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmarkSeparator);

    if (mTimer)
    {
      mTimer->Cancel();
      mTimer = nsnull;
    }

		if (gRDFC)
		{
			nsServiceManager::ReleaseService(kRDFContainerUtilsCID, gRDFC);
			gRDFC = nsnull;
		}

    if (gRDF)
    {
      nsServiceManager::ReleaseService(kRDFServiceCID, gRDF);
      gRDF = nsnull;
    }
  }

	NS_IF_RELEASE(mInner);

	// Unregister ourselves from the RDF service
	if (gRDF)
	    gRDF->UnregisterDataSource(this);
}



nsresult
nsRemoteBookmarks::Init()
{
  nsresult rv;

  if (gRefCnt++ == 0)
  {
    // get the window watcher service, so we can get an auth prompter
    //
    mWindowWatcher = do_GetService("@mozilla.org/embedcomp/window-watcher;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = nsServiceManager::GetService(kRDFServiceCID,
      NS_GET_IID(nsIRDFService),
      (nsISupports**) &gRDF);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF service");
    if (NS_FAILED(rv))
      return(rv);

		rv = nsServiceManager::GetService(kRDFContainerUtilsCID,
						  NS_GET_IID(nsIRDFContainerUtils),
						  (nsISupports**) &gRDFC);
		NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get RDF container utils");
		if (NS_FAILED(rv))
		  return(rv);

    rv = CallCreateInstance(kRDFInMemoryDataSourceCID, &mInner);
    if (NS_FAILED(rv))
      return(rv);

    rv = mInner->AddObserver(this);
    if (NS_FAILED(rv))
      return(rv);

    gRDF->GetResource(RDF_NAMESPACE_URI "type",             &kRDF_type);
    gRDF->GetResource(NC_NAMESPACE_URI "Bookmark",          &kNC_Bookmark);
		gRDF->GetResource(NC_NAMESPACE_URI "BookmarkSeparator", &kNC_BookmarkSeparator);
    gRDF->GetResource(NC_NAMESPACE_URI "Folder",            &kNC_Folder);
    gRDF->GetResource(NC_NAMESPACE_URI "parent",            &kNC_Parent);
    gRDF->GetResource(NC_NAMESPACE_URI "child",             &kNC_Child);
    gRDF->GetResource(NC_NAMESPACE_URI "URL",               &kNC_URL);
    gRDF->GetResource(NC_NAMESPACE_URI "Name",              &kNC_Name);
    gRDF->GetResource(NC_NAMESPACE_URI "ShortcutURL",       &kNC_ShortcutURL);
    gRDF->GetResource(NC_NAMESPACE_URI "Description",       &kNC_Description);

		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=newbookmark",             &kNC_BookmarkCommand_NewBookmark);
		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=newfolder",               &kNC_BookmarkCommand_NewFolder);
		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=newseparator",            &kNC_BookmarkCommand_NewSeparator);
		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=deletebookmark",          &kNC_BookmarkCommand_DeleteBookmark);
		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=deletebookmarkfolder",    &kNC_BookmarkCommand_DeleteBookmarkFolder);
		gRDF->GetResource(NC_NAMESPACE_URI "command?cmd=deletebookmarkseparator", &kNC_BookmarkCommand_DeleteBookmarkSeparator);
  }

	// register this as a named data source with the RDF
	// service. Do this *last*, because if Init() fails, then the
	// object will be destroyed (leaving the RDF service with a
	// dangling pointer).
	rv = gRDF->RegisterDataSource(this, PR_FALSE);
	if (NS_FAILED(rv))
	  return(rv);

	return(NS_OK);
}



////////////////////////////////////////////////////////////////////////

NS_IMPL_ADDREF(nsRemoteBookmarks);

NS_IMETHODIMP_(nsrefcnt)
nsRemoteBookmarks::Release()
{
  // We need a special implementation of Release() because
  // our mInner holds a circular reference back to us.
  NS_PRECONDITION(PRInt32(mRefCnt) > 0, "duplicate release");
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "nsBookmarksService");

  if (mInner && mRefCnt == 1)
  {
    nsIRDFDataSource* tmp = mInner;
    mInner = nsnull;
    NS_IF_RELEASE(tmp);
    return(0);
  }
  else if (mRefCnt == 0)
  {
    delete this;
    return(0);
  }
  return(mRefCnt);
}



////////////////////////////////////////////////////////////////////////



PRBool
nsRemoteBookmarks::isRemoteBookmarkURI(nsIRDFResource *r)
{
  static const char kRemoteBookmarkProtocol[]         = REMOTE_BOOKMARK_PREFIX "ldap://";
  static const char kSecureRemoteBookmarkProtocol[]   = REMOTE_BOOKMARK_PREFIX "ldaps://";

  PRBool      isURIFlag = PR_FALSE;
  const char  *uri = nsnull;

  r->GetValueConst(&uri);
  if ((uri) && ((!strncmp(uri, kRemoteBookmarkProtocol, sizeof(kRemoteBookmarkProtocol) - 1)) ||
    (!strncmp(uri, kSecureRemoteBookmarkProtocol, sizeof(kSecureRemoteBookmarkProtocol) - 1))))
  {
    isURIFlag = PR_TRUE;
  }
  return(isURIFlag);
}



////////////////////////////////////////////////////////////////////////
// nsIRDFDataSource



NS_IMETHODIMP
nsRemoteBookmarks::GetURI(char* *aURI)
{
  NS_PRECONDITION(aURI != nsnull, "null ptr");
  if (! aURI)
    return(NS_ERROR_NULL_POINTER);

	*aURI = nsCRT::strdup("rdf:remotebookmarks");
	if (! *aURI)
		return(NS_ERROR_OUT_OF_MEMORY);

	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::GetSource(nsIRDFResource* aProperty,
                             nsIRDFNode* aTarget,
                             PRBool tv,
                             nsIRDFResource** aSource)
{
  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  *aSource = nsnull;

  if (mInner)
  {
    nsresult rv = mInner->GetSource(aProperty, aTarget, tv, aSource);
    return(rv);
  }

  return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
nsRemoteBookmarks::GetSources(nsIRDFResource* aProperty,
                              nsIRDFNode* aTarget,
                              PRBool tv,
                              nsISimpleEnumerator** aSources)
{
  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aSources != nsnull, "null ptr");
  if (! aSources)
    return(NS_ERROR_NULL_POINTER);

  *aSources = nsnull;

  if (mInner)
  {
    nsresult rv = mInner->GetSources(aProperty, aTarget, tv, aSources);
    return(rv);
  }

  return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
nsRemoteBookmarks::GetTarget(nsIRDFResource* aSource,
                             nsIRDFResource* aProperty,
                             PRBool tv,
                             nsIRDFNode** aTarget)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  *aTarget = nsnull;

  if (mInner)
  {
    if (isRemoteBookmarkURI(aSource) && (aProperty == kNC_Child) && (tv == PR_TRUE))
    {
      if (isRemoteContainer(aSource))
      {
        *aTarget = aSource;
        NS_IF_ADDREF(*aTarget);
        return(NS_OK);
      }
    }
    // fallback to querying mInner
    nsresult rv = mInner->GetTarget(aSource, aProperty, tv, aTarget);
    return(rv);
  }
	return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
nsRemoteBookmarks::GetTargets(nsIRDFResource* aSource,
                              nsIRDFResource* aProperty,
                              PRBool tv,
                              nsISimpleEnumerator** aTargets)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTargets != nsnull, "null ptr");
  if (! aTargets)
    return(NS_ERROR_NULL_POINTER);

  *aTargets = nsnull;

  if (mInner)
  {
    nsresult rv;
    PRBool isEmptyFlag = PR_FALSE;

    if (isRemoteBookmarkURI(aSource) && (aProperty == kNC_Child) && (tv == PR_TRUE) &&
      NS_SUCCEEDED(rv = gRDFC->IsEmpty(mInner, aSource, &isEmptyFlag)) && (isEmptyFlag))
    {
      PRBool importantFlag;
      nsCAutoString bindname;
      nsresult rv = GetLDAPExtension(aSource, "bindname=", bindname, &importantFlag);

      nsAutoString bindDN, password;
      bindDN.AssignWithConversion(bindname.get());
      if (bindDN.Length() > 0)
      {
        rv = doAuthentication(aSource, bindDN, password);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Get the ldap connection
      nsCOMPtr<nsILDAPConnection> ldapConnection;
      ldapConnection = do_CreateInstance(NS_LDAPCONNECTION_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = doLDAPQuery(ldapConnection, aSource, nsnull, bindDN, password,
        nsRemoteBookmarks::LDAP_SEARCH);
      NS_ENSURE_SUCCESS(rv, rv);

      return(NS_RDF_NO_VALUE);
    }
    // fallback to querying mInner
    rv = mInner->GetTargets(aSource, aProperty, tv, aTargets);
    return(rv);
  }
	return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
nsRemoteBookmarks::Assert(nsIRDFResource* aSource,
                          nsIRDFResource* aProperty,
                          nsIRDFNode* aTarget,
                          PRBool aTruthValue)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  nsresult rv = NS_RDF_ASSERTION_REJECTED;

  if ((!mInner) || (aTruthValue == PR_FALSE))
    return(rv);

  if (isMutableProperty(aProperty))
  {
    rv = updateLDAPBookmarkItem(aSource, aProperty, nsnull, aTarget);
  }
  return(rv);
}



NS_IMETHODIMP
nsRemoteBookmarks::Unassert(nsIRDFResource* aSource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode* aTarget)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  nsresult rv = NS_RDF_ASSERTION_REJECTED;

  if (!mInner)
    return(rv);

  if (isMutableProperty(aProperty))
  {
    rv = updateLDAPBookmarkItem(aSource, aProperty, aTarget, nsnull);
  }
  return(rv);
}



NS_IMETHODIMP
nsRemoteBookmarks::Change(nsIRDFResource* aSource,
                          nsIRDFResource* aProperty,
                          nsIRDFNode* aOldTarget,
                          nsIRDFNode* aNewTarget)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aOldTarget != nsnull, "null ptr");
  if (! aOldTarget)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aNewTarget != nsnull, "null ptr");
  if (! aNewTarget)
    return(NS_ERROR_NULL_POINTER);

  nsresult rv = NS_RDF_ASSERTION_REJECTED;

  if (!mInner)
    return(rv);

  if (isMutableProperty(aProperty))
  {
    rv = updateLDAPBookmarkItem(aSource, aProperty, aOldTarget, aNewTarget);
  }
  return(rv);
}



NS_IMETHODIMP
nsRemoteBookmarks::Move(nsIRDFResource* aOldSource,
                        nsIRDFResource* aNewSource,
                        nsIRDFResource* aProperty,
                        nsIRDFNode* aTarget)
{
  NS_PRECONDITION(aOldSource != nsnull, "null ptr");
  if (! aOldSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aNewSource != nsnull, "null ptr");
  if (! aNewSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  nsresult rv = NS_RDF_ASSERTION_REJECTED;

  if (!mInner)
    return(rv);

  if (isMutableProperty(aProperty))
  {
    // XXX to do
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }
  return(rv);
}



NS_IMETHODIMP
nsRemoteBookmarks::HasAssertion(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget,
                                PRBool aTruthValue,
                                PRBool* aHasAssertion)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aProperty != nsnull, "null ptr");
  if (! aProperty)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aTarget != nsnull, "null ptr");
  if (! aTarget)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aHasAssertion != nsnull, "null ptr");
  if (! aHasAssertion)
    return(NS_ERROR_NULL_POINTER);

  *aHasAssertion = PR_FALSE;

  nsresult rv = NS_OK;

  if ((!mInner) || (aTruthValue == PR_FALSE))
    return(rv);

  if (isRemoteBookmarkURI(aSource) && (aProperty == kNC_Child))
  {
    if (isRemoteContainer(aSource))
    {
      *aHasAssertion = PR_TRUE;
    }
  }
  else
  {
    // fallback to querying mInner
    rv = mInner->HasAssertion(aSource, aProperty, aTarget, aTruthValue,
      aHasAssertion);
  }
	return(rv);
}



NS_IMETHODIMP
nsRemoteBookmarks::AddObserver(nsIRDFObserver* aObserver)
{
  NS_PRECONDITION(aObserver != nsnull, "null ptr");
  if (! aObserver)
    return(NS_ERROR_NULL_POINTER);

  if (! mObservers)
  {
    nsresult rv;
    rv = NS_NewISupportsArray(getter_AddRefs(mObservers));
    if (NS_FAILED(rv))
      return(rv);
  }

  mObservers->AppendElement(aObserver);
  return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::RemoveObserver(nsIRDFObserver* aObserver)
{
  NS_PRECONDITION(aObserver != nsnull, "null ptr");
  if (! aObserver)
    return(NS_ERROR_NULL_POINTER);

  if (mObservers)
  {
    mObservers->RemoveElement(aObserver);
  }
  return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::HasArcIn(nsIRDFNode *aNode,
                            nsIRDFResource *aArc,
                            PRBool *_retval)
{
  NS_PRECONDITION(aNode != nsnull, "null ptr");
  if (! aNode)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aArc != nsnull, "null ptr");
  if (! aArc)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(_retval != nsnull, "null ptr");
  if (! _retval)
    return(NS_ERROR_NULL_POINTER);

  *_retval = PR_FALSE;

  if (mInner)
  {
    nsresult rv = mInner->HasArcIn(aNode, aArc, _retval);
    return(rv);
  }

  return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::HasArcOut(nsIRDFResource *aSource,
                             nsIRDFResource *aArc,
                             PRBool *_retval)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aArc != nsnull, "null ptr");
  if (! aArc)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(_retval != nsnull, "null ptr");
  if (! _retval)
    return(NS_ERROR_NULL_POINTER);

  *_retval = PR_FALSE;

  if (mInner)
  {
    if (isRemoteBookmarkURI(aSource) && (aArc == kNC_Child))
    {
      if (isRemoteContainer(aSource))
      {
        *_retval = PR_TRUE;
        return(NS_OK);
      }
    }
    // fallback to querying mInner
    nsresult rv = mInner->HasArcOut(aSource, aArc, _retval);
    return(rv);
  }
	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::ArcLabelsIn(nsIRDFNode* aNode,
                               nsISimpleEnumerator** aLabels)
{
  NS_PRECONDITION(aNode != nsnull, "null ptr");
  if (! aNode)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aLabels != nsnull, "null ptr");
  if (! aLabels)
    return(NS_ERROR_NULL_POINTER);

  *aLabels = nsnull;

  if (mInner)
  {
    nsresult rv = mInner->ArcLabelsIn(aNode, aLabels);
    return(rv);
  }

  return(NS_NewEmptyEnumerator(aLabels));
}



NS_IMETHODIMP
nsRemoteBookmarks::ArcLabelsOut(nsIRDFResource* aSource,
                                nsISimpleEnumerator** aLabels)
{
  NS_PRECONDITION(aSource != nsnull, "null ptr");
  if (! aSource)
    return(NS_ERROR_NULL_POINTER);

  NS_PRECONDITION(aLabels != nsnull, "null ptr");
  if (! aLabels)
    return(NS_ERROR_NULL_POINTER);

  *aLabels = nsnull;

  nsresult rv;
  if (mInner)
  {
    if (isRemoteBookmarkURI(aSource))
    {
      nsCOMPtr<nsISupportsArray> array;
      rv = NS_NewISupportsArray(getter_AddRefs(array));
      if (NS_FAILED(rv)) return rv;

      array->AppendElement(kNC_Bookmark);
      array->AppendElement(kNC_Folder);
      array->AppendElement(kNC_Child);
      array->AppendElement(kNC_URL);
      array->AppendElement(kNC_Name);
      array->AppendElement(kNC_ShortcutURL);
      array->AppendElement(kNC_Description);

      nsISimpleEnumerator* result = new nsArrayEnumerator(array);
      if (! result)
        return(NS_ERROR_OUT_OF_MEMORY);
      NS_ADDREF(result);
      *aLabels = result;
      return(NS_OK);
    }
    // fallback to querying mInner
    rv = mInner->ArcLabelsOut(aSource, aLabels);
    return(rv);
  }
  return(NS_NewEmptyEnumerator(aLabels));
}



NS_IMETHODIMP
nsRemoteBookmarks::GetAllResources(nsISimpleEnumerator** aResults)
{
  NS_PRECONDITION(aResults != nsnull, "null ptr");
  if (! aResults)
    return(NS_ERROR_NULL_POINTER);

  *aResults = nsnull;

  if (mInner)
  {
    nsresult rv = mInner->GetAllResources(aResults);
    return(rv);
  }

  return(NS_NewEmptyEnumerator(aResults));
}



NS_IMETHODIMP
nsRemoteBookmarks::GetAllCommands(nsIRDFResource* source,
                                  nsIEnumerator/*<nsIRDFResource>*/** commands)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::GetAllCmds(nsIRDFResource* source,
                              nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                                    nsIRDFResource* aCommand,
                                    nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                                    PRBool* aResult)
{
	return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                             nsIRDFResource* aCommand,
                             nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
  nsresult		rv = NS_OK;
  PRInt32			loop;
  PRUint32		numSources;
  if (NS_FAILED(rv = aSources->Count(&numSources)))	return(rv);
  if (numSources < 1)
  {
    return(NS_ERROR_ILLEGAL_VALUE);
  }

  // Note: some commands only run once (instead of looping over selection);
  //       if that's the case, be sure to "break" (if success)

  for (loop=((PRInt32)numSources)-1; loop>=0; loop--)
  {
    nsCOMPtr<nsISupports>	aSource = aSources->ElementAt(loop);
    if (!aSource)	return(NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIRDFResource>	src = do_QueryInterface(aSource);
    if (!src)	return(NS_ERROR_NO_INTERFACE);

    if (aCommand == kNC_BookmarkCommand_NewBookmark)
    {
      rv = insertLDAPBookmarkItem(src, aArguments, kNC_Bookmark);
      if (NS_FAILED(rv))
        return(rv);
      break;
    }
    else if (aCommand == kNC_BookmarkCommand_NewFolder)
    {
      rv = insertLDAPBookmarkItem(src, aArguments, kNC_Folder);
      if (NS_FAILED(rv))
        return(rv);
      break;
    }
    else if (aCommand == kNC_BookmarkCommand_NewSeparator)
    {
      rv = insertLDAPBookmarkItem(src, aArguments, kNC_BookmarkSeparator);
      if (NS_FAILED(rv))
        return(rv);
      break;
    }
    else if (aCommand == kNC_BookmarkCommand_DeleteBookmark)
    {
      rv = deleteLDAPBookmarkItem(src, aArguments, loop, kNC_Bookmark);
      if (NS_FAILED(rv))
        return(rv);
    }
    else if (aCommand == kNC_BookmarkCommand_DeleteBookmarkFolder)
    {
      rv = deleteLDAPBookmarkItem(src, aArguments, loop, kNC_Folder);
      if (NS_FAILED(rv))
        return(rv);
    }
    else if (aCommand == kNC_BookmarkCommand_DeleteBookmarkSeparator)
    {
      rv = deleteLDAPBookmarkItem(src, aArguments, loop, kNC_BookmarkSeparator);
      if (NS_FAILED(rv))
        return(rv);
    }
  }
  return(NS_OK);
}



////////////////////////////////////////////////////////////////////////
// nsIRDFObserver



NS_IMETHODIMP
nsRemoteBookmarks::OnAssert(nsIRDFDataSource* aDataSource,
                            nsIRDFResource* aSource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode* aTarget)
{
  if (mUpdateBatchNest != 0)
    return(NS_OK);

  if (mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
        NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->OnAssert(aDataSource, aSource, aProperty, aTarget);
      NS_RELEASE(obs);
    }
  }

	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::OnUnassert(nsIRDFDataSource* aDataSource,
                              nsIRDFResource* aSource,
                              nsIRDFResource* aProperty,
                              nsIRDFNode* aTarget)
{
  if (mUpdateBatchNest != 0)
    return(NS_OK);

  if (mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
        NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->OnUnassert(aDataSource, aSource, aProperty, aTarget);
      NS_RELEASE(obs);
    }
  }

	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::OnChange(nsIRDFDataSource* aDataSource,
                            nsIRDFResource* aSource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode* aOldTarget,
                            nsIRDFNode* aNewTarget)
{
  if (mUpdateBatchNest != 0)
    return(NS_OK);

  if (mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
      NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->OnChange(aDataSource, aSource, aProperty, aOldTarget, aNewTarget);
      NS_RELEASE(obs);
    }
  }

	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::OnMove(nsIRDFDataSource* aDataSource,
                           nsIRDFResource* aOldSource,
                           nsIRDFResource* aNewSource,
                           nsIRDFResource* aProperty,
                           nsIRDFNode* aTarget)
{
  if (mUpdateBatchNest != 0)
    return(NS_OK);

  if (mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
      NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->OnMove(aDataSource, aOldSource, aNewSource, aProperty, aTarget);
      NS_RELEASE(obs);
    }
  }

	return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::BeginUpdateBatch(nsIRDFDataSource* aDataSource)
{
  if ((mUpdateBatchNest++ == 0) && mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
        NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->BeginUpdateBatch(aDataSource);
      NS_RELEASE(obs);
    }
  }

  return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::EndUpdateBatch(nsIRDFDataSource* aDataSource)
{
  if (mUpdateBatchNest > 0)
  {
    --mUpdateBatchNest;
  }

  if ((mUpdateBatchNest == 0) && mObservers)
  {
    nsresult rv;

    PRUint32 count;
    rv = mObservers->Count(&count);
    if (NS_FAILED(rv))
      return(rv);

    for (PRInt32 i = 0; i < PRInt32(count); ++i)
    {
      nsIRDFObserver* obs =
        NS_REINTERPRET_CAST(nsIRDFObserver*, mObservers->ElementAt(i));
      if (!obs) continue;

      (void) obs->EndUpdateBatch(aDataSource);
      NS_RELEASE(obs);
    }
  }

  return(NS_OK);
}



////////////////////////////////////////////////////////////////////////
// nsITimerCallback



NS_IMETHODIMP_(void)
nsRemoteBookmarks::Notify(nsITimer* aTimer)
{
}



////////////////////////////////////////////////////////////////////////
// nsILDAPMessageListener



NS_IMETHODIMP
nsRemoteBookmarks::OnLDAPInit(nsresult aStatus)
{
  // Make sure that the Init() worked properly
  NS_ENSURE_SUCCESS(aStatus, aStatus);

  // Initiate the LDAP operation - XXX leak for now
  nsresult rv;
  mLDAPOperation = do_CreateInstance(NS_LDAPOPERATION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILDAPMessageListener> proxyListener;
  rv = NS_GetProxyForObject(NS_UI_THREAD_EVENTQ,
    NS_GET_IID(nsILDAPMessageListener),
    NS_STATIC_CAST(nsILDAPMessageListener *, this),
    PROXY_SYNC | PROXY_ALWAYS,
    getter_AddRefs(proxyListener));
  NS_ENSURE_SUCCESS(rv, rv);

  //  XXX TO DO need to somehow get "mConnection" from somewhere
  rv = mLDAPOperation->Init(mConnection, proxyListener);
  NS_ENSURE_SUCCESS(rv, rv);

  // simple Bind (password may be empty string, initially)
  rv = mLDAPOperation->SimpleBind(mPassword.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return(NS_OK);
}



NS_IMETHODIMP
nsRemoteBookmarks::OnLDAPMessage(nsILDAPMessage *aMessage)
{
  nsresult rv;

  PRInt32 messageType;
  rv = aMessage->GetType(&messageType);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 errorCode;
  rv = aMessage->GetErrorCode(&errorCode);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (messageType)
  {
    // bind in progress
    case nsILDAPMessage::RES_BIND:
      {
      if (errorCode != nsILDAPErrors::SUCCESS)
      {
        if ( errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
             errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
             errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS)
        {
          // rebind and try again
          rv = doLDAPRebind();
          return(rv);
        }
        showLDAPError(aMessage);
        return(NS_ERROR_FAILURE);
      }

      if (!mLDAPURL)
        return(NS_ERROR_NULL_POINTER);

      nsXPIDLCString dn;
      rv = mLDAPURL->GetDn(getter_Copies(dn));
      NS_ENSURE_SUCCESS(rv, rv);

      switch(mOpcode)
      {
        case nsRemoteBookmarks::LDAP_SEARCH:
        {
          nsXPIDLCString filter;
          rv = mLDAPURL->GetFilter(getter_Copies (filter));
          NS_ENSURE_SUCCESS(rv, rv);

          static const PRUint32 numLDAPAttrs = 5;
          static const char *attrs[numLDAPAttrs] = {
            MOZ_SCHEMA_URL, MOZ_SCHEMA_NAME, MOZ_SCHEMA_KEYWORD,
            MOZ_SCHEMA_DESCRIPTION, MOZ_SCHEMA_OBJ_CLASS
          };

          // when we called SetSpec(), our spec contained UTF8 data
          // so when we call GetFilter(), it's not going to be ASCII.
          // it will be UTF8, so we need to convert from UTF8 to UCS2
          // see bug #124995
          rv = mLDAPOperation->SearchExt(NS_ConvertUTF8toUCS2(dn).get(),
            nsILDAPURL::SCOPE_ONELEVEL, NS_ConvertUTF8toUCS2(filter).get(),
            numLDAPAttrs,                 /* attributes.GetSize () */
            (const char **)attrs,         /* attributes.GetArray () */
            nsILDAPOperation::NO_LIMIT,   /* TimeOut */
            nsILDAPOperation::NO_LIMIT ); /* ResultLimit */
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

#ifdef  LDAP_MOD_SUPPORT

        case nsRemoteBookmarks::LDAP_ADD:
        {
          nsCOMPtr<nsIRDFResource> containerRes;
          rv = mContainer->GetResource(getter_AddRefs(containerRes));
          NS_ENSURE_SUCCESS(rv, rv);
          nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(containerRes);
          if (!rdfRes)
            return(NS_ERROR_NULL_POINTER);

          nsresult rv;
          const char *uri = nsnull;
          rv = rdfRes->GetValueConst(&uri);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!uri)
            return(NS_ERROR_NULL_POINTER);

          nsCAutoString dn(uri);

          nsCAutoString randomNum;
          PRTime now64 = PR_Now();
      		PRInt32 now32;
      		LL_L2I(now32, now64);
      		randomNum.Truncate();
          randomNum.AppendInt(now32);
          
          // XXX searching for "cn=" is bogus, perhaps should munge into a nsIURI, get
          // the DN, munge that, set the DN back. then get the complete URI from nsIURI
          PRInt32 offset = dn.Find("cn=");  // XXX bogus
          dn.Insert(",", offset);
          dn.Insert(randomNum, offset);
          dn.Insert("cn=bookmark", offset);

          // get a fully-qualified (with REMOTE_BOOKMARK_PREFIX) node
          // then chop off everything before the start of the DN
          rv = gRDF->GetResource(dn.get(), getter_AddRefs(mNode));
          NS_ENSURE_SUCCESS(rv, rv);
          dn.Cut(0, offset);                // cut off REMOTE_BOOKMARK_PREFIX and everything up to dn start

          // keep these in scope
          nsCOMPtr<nsILDAPModification> ldapModObjClass;
          nsCOMPtr<nsILDAPModification> ldapModURL;
          nsCOMPtr<nsILDAPModification> ldapModName;
          nsCOMPtr<nsILDAPModification> ldapModKeyword;
          nsCOMPtr<nsILDAPModification> ldapModDesc;

          // set up objectclass
          ldapModObjClass = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = ldapModObjClass->SetType(MOZ_SCHEMA_OBJ_CLASS);
          NS_ENSURE_SUCCESS(rv, rv);

          const char *classValue = nsnull;
          if (mProperty == kNC_Bookmark)
            classValue = MOZ_SCHEMA_BMK_CLASS;
          else if (mProperty == kNC_Folder)
            classValue = MOZ_SCHEMA_FOLDER_CLASS;
          else
            return NS_ERROR_UNEXPECTED;
          rv = ldapModObjClass->SetValues(1, &classValue);
          NS_ENSURE_SUCCESS(rv, rv);

          // add all ldapMods into operation
          // need to grow array size if/when new attributes are added
          static const PRUint32 maxLDAPMods = 6;
          nsILDAPModification *mods[maxLDAPMods];
          PRInt32 numLDAPMods = 0;
          mods[numLDAPMods++] = ldapModObjClass;
          mods[1] = nsnull;
          mods[2] = nsnull;
          mods[3] = nsnull;
          mods[4] = nsnull;
          mods[5] = nsnull;

          // set up name (if we have one)
          if (mNameLiteral)
          {
            const PRUnichar *nameUni = nsnull;
            mNameLiteral->GetValueConst(&nameUni);

            ldapModName = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = ldapModName->SetType(MOZ_SCHEMA_NAME);
            NS_ENSURE_SUCCESS(rv, rv);

            const char *nameValue = NS_ConvertUCS2toUTF8(nameUni).get();
            rv = ldapModName->SetValues(1, &nameValue);
            NS_ENSURE_SUCCESS(rv, rv);

            mods[numLDAPMods++] = ldapModName;
          }

          // set up desc (if we have one)
          if (mDescLiteral)
          {
            const PRUnichar *descUni = nsnull;
            mDescLiteral->GetValueConst(&descUni);

            ldapModDesc = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = ldapModDesc->SetType(MOZ_SCHEMA_DESCRIPTION);
            NS_ENSURE_SUCCESS(rv, rv);

            const char *descValue = NS_ConvertUCS2toUTF8(descUni).get();
            rv = ldapModDesc->SetValues(1, &descValue);
            NS_ENSURE_SUCCESS(rv, rv);

            mods[numLDAPMods++] = ldapModDesc;
          }

          // set up keyword (if bookmark, and if we have one)
          if ((mProperty == kNC_Bookmark) && (mShortcutLiteral))
          {
            const PRUnichar *keywordUni = nsnull;
            mShortcutLiteral->GetValueConst(&keywordUni);

            ldapModKeyword = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = ldapModKeyword->SetType(MOZ_SCHEMA_KEYWORD);
            NS_ENSURE_SUCCESS(rv, rv);

            const char *keywordValue = NS_ConvertUCS2toUTF8(keywordUni).get();
            rv = ldapModKeyword->SetValues(1, &keywordValue);
            NS_ENSURE_SUCCESS(rv, rv);

            mods[numLDAPMods++] = ldapModKeyword;
          }

          // set up URL (if bookmark, and if we have one)
          if ((mProperty == kNC_Bookmark) && (mURLLiteral))
          {
            const PRUnichar *urlUni = nsnull;
            mURLLiteral->GetValueConst(&urlUni);

            ldapModURL = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = ldapModURL->SetType(MOZ_SCHEMA_URL);
            NS_ENSURE_SUCCESS(rv, rv);

            const char *urlValue = NS_ConvertUCS2toUTF8(urlUni).get();
            rv = ldapModURL->SetValues(1, &urlValue);
            NS_ENSURE_SUCCESS(rv, rv);

            mods[numLDAPMods++] = ldapModURL;
          }

          rv = mLDAPOperation->AddExt(NS_ConvertUTF8toUCS2(dn).get(), numLDAPMods, mods);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

        case nsRemoteBookmarks::LDAP_MODIFY:
        {
          nsresult rv;
          nsCOMPtr<nsILDAPModification> ldapMod = do_CreateInstance(NS_LDAPMODIFICATION_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          if (mOldTarget && mNewTarget)
            rv = ldapMod->SetOperation(nsILDAPModification::MOD_REPLACE);
          else if (mNewTarget)
            rv = ldapMod->SetOperation(nsILDAPModification::MOD_ADD);
          else if (mOldTarget)
            rv = ldapMod->SetOperation(nsILDAPModification::MOD_DELETE);
          NS_ENSURE_SUCCESS(rv, rv);

          // set type of LDAP mod based on RDF property change
          const char *schemaAttrib = getPropertySchemaName(mProperty);
          if (!schemaAttrib)
            return(NS_OK);
          rv = ldapMod->SetType(schemaAttrib);
          NS_ENSURE_SUCCESS(rv, rv);

          // build up LDAP array of UTF-8 values
          nsCOMPtr<nsIRDFLiteral> rdfLit;
          if (mNewTarget)
          {
            rdfLit = do_QueryInterface(mNewTarget);
          }
          else if (mOldTarget)
          {
            rdfLit = do_QueryInterface(mOldTarget);
          }
          if (!rdfLit)
            return(NS_ERROR_UNEXPECTED);
          const PRUnichar *value = nsnull;
          rv = rdfLit->GetValueConst(&value);
          NS_ENSURE_SUCCESS(rv, rv);
          nsCAutoString utf8Value = NS_ConvertUCS2toUTF8(value);
          
          const char *values[2];
          values[0] = utf8Value.get();
          values[1] = nsnull;
          rv = ldapMod->SetValues(1, values);
          NS_ENSURE_SUCCESS(rv, rv);

          nsILDAPModification *mods[2];
          mods[0] = ldapMod;
          mods[1] = nsnull;

          rv = mLDAPOperation->ModifyExt(NS_ConvertUTF8toUCS2(dn).get(), 1, mods);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

        case nsRemoteBookmarks::LDAP_DELETE:
        {
          rv = mLDAPOperation->DeleteExt(NS_ConvertUTF8toUCS2(dn).get());
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;

#endif

      }

      }
      break;

    // found a result
    case nsILDAPMessage::RES_SEARCH_ENTRY:
      {
      // calculate true URI to result
      nsCAutoString spec;
      rv = mLDAPURL->GetSpec(spec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsILDAPURL> ldapURL;
      ldapURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      ldapURL->SetSpec(spec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsXPIDLString dn;
      rv = aMessage->GetDn(getter_Copies(dn));
      NS_ENSURE_SUCCESS(rv, rv);

      ldapURL->SetDn(NS_ConvertUCS2toUTF8(dn).get());
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString host;
      rv = ldapURL->GetAsciiHost(host);
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 port;
      rv = ldapURL->GetPort(&port);
      NS_ENSURE_SUCCESS(rv, rv);

      nsXPIDLCString cdn;
      rv = ldapURL->GetDn(getter_Copies(cdn));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 options;
      rv = ldapURL->GetOptions(&options);
      NS_ENSURE_SUCCESS(rv,rv);

      nsCString ldapSearchUrlString;
      char* _ldapSearchUrlString = PR_smprintf ("%sldap%s://%s:%d/%s",
        REMOTE_BOOKMARK_PREFIX,
        (options & nsILDAPURL::OPT_SECURE) ? "s" : "",
        host.get(), port, cdn.get());
      if (!_ldapSearchUrlString)
        return(NS_ERROR_OUT_OF_MEMORY);

      ldapSearchUrlString = _ldapSearchUrlString;
      PR_smprintf_free (_ldapSearchUrlString);

      nsCOMPtr<nsIRDFResource> searchType;
      nsCOMPtr<nsIRDFResource> searchRes;
      nsCOMPtr<nsIRDFLiteral> searchResLit;

      // check objectclass to determine what we are dealing with
      nsAutoString classStr, urlStr, nameStr, keywordStr, descStr;

      GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_OBJ_CLASS, classStr);
      classStr.Trim(" \t");
      if (classStr.EqualsIgnoreCase(MOZ_SCHEMA_BMK_CLASS))
      {
        // it is a Bookmark
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_URL, urlStr);
        urlStr.Trim(" \t");
        if (urlStr.Length() < 1)
          return(NS_OK);
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_NAME, nameStr);
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_KEYWORD, keywordStr);
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_DESCRIPTION, descStr);

        rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(searchRes));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = gRDF->GetLiteral(urlStr.get(), getter_AddRefs(searchResLit));
        NS_ENSURE_SUCCESS(rv, rv);

        // set its type
        searchType = kNC_Bookmark;
      }
      else if (classStr.EqualsIgnoreCase(MOZ_SCHEMA_FOLDER_CLASS))
      {
        // it is a Folder
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_NAME, nameStr);
        GetLDAPMsgAttrValue(aMessage, MOZ_SCHEMA_DESCRIPTION, descStr);

        rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(searchRes));
        NS_ENSURE_SUCCESS(rv, rv);

        // set its type
        searchType = kNC_Folder;
      }
      else
      {
        // XXX other types, such as separators?
      }

      if (!nameStr.IsEmpty())
      {
        nsCOMPtr<nsIRDFLiteral> nameLiteral;
        rv = gRDF->GetLiteral(nameStr.get(), getter_AddRefs(nameLiteral));
        if (NS_SUCCEEDED(rv))
        {
          // assert #Name
          rv = mInner->Assert(searchRes, kNC_Name, nameLiteral, PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (!keywordStr.IsEmpty())
      {
        nsCOMPtr<nsIRDFLiteral> keywordLiteral;
        rv = gRDF->GetLiteral(keywordStr.get(), getter_AddRefs(keywordLiteral));
        if (NS_SUCCEEDED(rv))
        {
          // assert #Keyword
          rv = mInner->Assert(searchRes, kNC_ShortcutURL, keywordLiteral, PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (!descStr.IsEmpty())
      {
        nsCOMPtr<nsIRDFLiteral> descLiteral;
        rv = gRDF->GetLiteral(descStr.get(), getter_AddRefs(descLiteral));
        if (NS_SUCCEEDED(rv))
        {
          // assert #Description
          rv = mInner->Assert(searchRes, kNC_Description, descLiteral, PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }

      if (searchType)
      {
        rv = mInner->Assert(searchRes, kRDF_type, searchType, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsCOMPtr<nsIRDFResource> urlRes;
      rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(urlRes));
      NS_ENSURE_SUCCESS(rv, rv);

      if (searchResLit)
      {
        rv = mInner->Assert(searchRes, kNC_URL, searchResLit, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // prevent duplicates                                                       
      PRInt32 aIndex;                                                             
      nsCOMPtr<nsIRDFResource> containerRes;
      (void)mContainer->GetResource(getter_AddRefs(containerRes));
      if (containerRes && NS_SUCCEEDED(gRDFC->IndexOf(mInner, containerRes,
        searchRes, &aIndex)) && (aIndex < 0))
      {
        // assert parent <-> child relationship LAST
        rv = mContainer->AppendElement(searchRes);                               
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add bookmark to container");  
      }
      }
      break;

    // search finished
    case nsILDAPMessage::RES_SEARCH_RESULT:
      PRInt32 count = 0;
      if (errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
          errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
          errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS ||
          ((errorCode == nsILDAPErrors::SUCCESS) &&
           (NS_SUCCEEDED(rv = mContainer->GetCount(&count)) &&
           (count == 0) && (mPassword.IsEmpty()))))
      {
        rv = doLDAPRebind();
        return(rv);
      }

      if (errorCode != nsILDAPErrors::SUCCESS)
      {
        showLDAPError(aMessage);
      }

      // XXX XXX XXX hack TO DO
      // undo leakage from above
      mLDAPOperation->Abandon();
      mLDAPOperation = nsnull;
      mLDAPURL = nsnull;
      mConnection = nsnull;
      mContainer = nsnull;
      break;

#ifdef  LDAP_MOD_SUPPORT

    // node attributes modified
    case nsILDAPMessage::RES_MODIFY:
    {
    if (errorCode != nsILDAPErrors::SUCCESS)
    {
      if ( errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
           errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
           errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS)
      {
        rv = doLDAPRebind();
        return(rv);
      }
      showLDAPError(aMessage);
      return(NS_ERROR_FAILURE);
    }

    // success, update the internal graph
    nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(mNode);
    if (mOldTarget && mNewTarget)
    {
      rv = mInner->Change(rdfRes, mProperty, mOldTarget, mNewTarget);
    }
    else if (mNewTarget)
    {
      rv = mInner->Assert(rdfRes, mProperty, mNewTarget, PR_TRUE);
    }
    else if (mOldTarget)
    {
      rv = mInner->Unassert(rdfRes, mProperty, mOldTarget);
    }
    NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    // node added
    case nsILDAPMessage::RES_ADD:
    {
    if (errorCode != nsILDAPErrors::SUCCESS)
    {
      if ( errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
           errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
           errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS)
      {
        rv = doLDAPRebind();
        return(rv);
      }
      showLDAPError(aMessage);
      return(NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIRDFResource> aNode;
    aNode = mNode;
    if (mURLLiteral)
    {
      rv = mInner->Assert(aNode, kNC_URL, mURLLiteral, PR_TRUE);
      mURLLiteral = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // success, update the internal graph
    if (mNameLiteral)
    {
      rv = mInner->Assert(aNode, kNC_Name, mNameLiteral, PR_TRUE);
      mNameLiteral = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mShortcutLiteral)
    {
      rv = mInner->Assert(aNode, kNC_ShortcutURL, mShortcutLiteral, PR_TRUE);
      mShortcutLiteral = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mDescLiteral)
    {
      rv = mInner->Assert(aNode, kNC_Description, mDescLiteral, PR_TRUE);
      mDescLiteral = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (mProperty == kNC_Folder)
    {
      nsCOMPtr<nsIRDFContainer> ldapContainer;
      rv = gRDFC->MakeSeq(mInner, aNode, getter_AddRefs(ldapContainer));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mContainer->AppendElement(aNode);
    NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    // node deleted
    case nsILDAPMessage::RES_DELETE:
    {
    if (errorCode != nsILDAPErrors::SUCCESS)
    {
      if ( errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
           errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
           errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS)
      {
        rv = doLDAPRebind();
        return(rv);
      }
      showLDAPError(aMessage);
      return(NS_ERROR_FAILURE);
    }
    // success, update the internal graph
    // XXX need to recursively handle containers being deleted
    rv = mContainer->RemoveElement(mNode, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    // node moved
    case nsILDAPMessage::RES_MODDN:
    break;

    case nsILDAPMessage::RES_COMPARE:
    break;

    case nsILDAPMessage::RES_EXTENDED:
    break;

#endif

  }
  return(NS_OK);
}



nsresult
nsRemoteBookmarks::showLDAPError(nsILDAPMessage *aMessage)
{
  nsCOMPtr<nsIPrompt> prompter;
  nsresult rv = mWindowWatcher->GetNewPrompter(0, getter_AddRefs(prompter));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!prompter)
    return(NS_ERROR_NULL_POINTER);

  nsAutoString errStr;
  rv = aMessage->GetErrorMessage(errStr);
  errStr.Trim(" \t");
  if (errStr.Length() == 0)
  {
    PRInt32 errorCode;
    rv = aMessage->GetErrorCode(&errorCode);
    NS_ENSURE_SUCCESS(rv, rv);

    errStr = NS_LITERAL_STRING("Error # ");
    errStr.AppendInt(errorCode);
  }
  prompter->Alert(NS_LITERAL_STRING("Error").get(), errStr.get());
  return(NS_OK);
}



nsresult
nsRemoteBookmarks::doLDAPRebind()
{
  nsresult rv;

  // make sure the wallet service has been created, and in doing so,
  // pass in a login-failed message to tell it to forget this passwd.
  //
  // apparently getting passwords stored in the wallet
  // doesn't require the service to be running, which is why
  // this might not exist yet.
  //
  rv = NS_CreateServicesFromCategory("passwordmanager",
    mLDAPURL, "login-failed");
  if (NS_FAILED(rv))
  {
    // not much to do at this point, though conceivably we could 
    // pop up a dialog telling the user to go manually delete
    // this password in the password manager.
    NS_ERROR("nsLDAPAutoCompleteSession::ForgetPassword(): error"
    " creating password manager service");
  }

  nsCOMPtr<nsIRDFResource> containerRes;
  if (mContainer)
  {
    rv = mContainer->GetResource(getter_AddRefs(containerRes));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    containerRes = mNode;
  }

  nsAutoString bindDN, password;
  rv = doAuthentication(containerRes, bindDN, password);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRDFResource> aNode = mNode;
  rv = doLDAPQuery(mConnection, containerRes, aNode, bindDN, password, mOpcode);
  return(rv);
}



nsresult
nsRemoteBookmarks::doLDAPQuery(nsILDAPConnection *ldapConnection,
                               nsIRDFResource *aParent,
                               nsIRDFResource *aNode,
                               nsString bindDN,
                               nsString password,
                               PRUint32 ldapOpcode)
{
  // XXX broken
  mLDAPURL = nsnull;
  mContainer = nsnull;
  mNode = nsnull;
  mPassword.Truncate();

  nsresult rv;
  const char *srcURI = nsnull;
  nsCOMPtr<nsIRDFContainer> ldapContainer;
  if (aParent)
  {
    rv = gRDFC->MakeSeq(mInner, aParent, getter_AddRefs(ldapContainer));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if ((ldapOpcode == nsRemoteBookmarks::LDAP_ADD) ||
      (ldapOpcode == nsRemoteBookmarks::LDAP_SEARCH))
  {
    aParent->GetValueConst(&srcURI);
  }
  else if ((ldapOpcode == nsRemoteBookmarks::LDAP_DELETE) ||
    (ldapOpcode == nsRemoteBookmarks::LDAP_MODIFY))
  {
    nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(aNode);
    if (rdfRes)
    {
      rdfRes->GetValueConst(&srcURI);
    }
  }
  if (!srcURI)
    return(NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsILDAPURL> ldapURL;
  ldapURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // trim off standard prefix
  nsCString cURI(&srcURI[REMOTE_BOOKMARK_PREFIX_LENGTH]);
  
  // XXX hack
  PRInt32 offset = cURI.Find("?bindname=", PR_TRUE);
  if (offset > 0)
    cURI.Truncate(offset);
  
  rv = ldapURL->SetSpec(cURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString host;
  rv = ldapURL->GetAsciiHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 port;
  rv = ldapURL->GetPort(&port);
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString dn;
  rv = ldapURL->GetDn(getter_Copies(dn));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 options;
  rv = ldapURL->GetOptions(&options);
  NS_ENSURE_SUCCESS(rv,rv);


  // NOTE: set these session variables before calling
  //       LDAPConnection->Init() as OnLDAPInit() [which
  //       accesses them] could fire before it returns
  mConnection = ldapConnection;
  mLDAPURL = ldapURL;
  mContainer = ldapContainer;
  mNode = aNode;
  mPassword = password;
  mOpcode = ldapOpcode;
  // XXX XXX XXX - dangle/leak the above vars here, for now.
  // We need them in other spots of code.


  // Now lets initialize the LDAP connection properly. We'll kick
  // off the bind operation in the callback function, |OnLDAPInit()|.
  rv = ldapConnection->Init(host.get(), port, options /* SSL */,
    bindDN.get(), this /* don't ADDREF "this" */);
  if (NS_FAILED(rv))
  {
    mConnection = nsnull;
    mLDAPURL = nsnull;
    mContainer = nsnull;
    mNode = nsnull;
    mPassword.Truncate();
    mOpcode = LDAP_READY;
  }
  return(rv);
}



nsresult
nsRemoteBookmarks::doAuthentication(nsIRDFResource *aSource,
                                    nsString &aBindDN,
                                    nsString &aPassword)
{
  // XXX hack until nsUTF8AutoString exists
  #define nsUTF8AutoString nsCAutoString
          nsUTF8AutoString spec;

  if (!aSource)
    return(NS_ERROR_NULL_POINTER);

  nsresult rv;
  nsCOMPtr<nsILDAPURL> ldapURL;
  ldapURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const char *srcURI = nsnull;
  nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(aSource);
  if (rdfRes)
    rdfRes->GetValueConst(&srcURI);
  if (!srcURI)
    return(NS_ERROR_NULL_POINTER);

  // trim off standard prefix
  nsCString cURI(&srcURI[REMOTE_BOOKMARK_PREFIX_LENGTH]);
  
  // XXX hack
  PRInt32 offset = cURI.Find("?bindname=", PR_TRUE);
  if (offset > 0)
    cURI.Truncate(offset);
  
  rv = ldapURL->SetSpec(cURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // use the URL spec of the LDAP server as the "realm" for wallet
  rv = ldapURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the host name for the auth prompt
  //
  nsCAutoString host;
  rv = ldapURL->GetAsciiHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  // get active window
  nsCOMPtr<nsIDOMWindow> activeDOMWindow;
  rv = mWindowWatcher->GetActiveWindow(getter_AddRefs(activeDOMWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the auth prompter itself
  //
  nsCOMPtr<nsIAuthPrompt> authPrompter;
  rv = mWindowWatcher->GetNewAuthPrompter(activeDOMWindow,
    getter_AddRefs(authPrompter));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool status = PR_FALSE;
  nsXPIDLString userDN, userPassword;

  PRBool important;
  nsCAutoString value;
  rv = GetLDAPExtension(aSource, "bindname=", value, &important);
  NS_ENSURE_SUCCESS(rv, rv);
  if (value.Length() > 0)
  {
    aBindDN.AssignWithConversion(value.get());
  }

  // get authentication password, prompting the user if necessary
  // XXX localization
  nsAutoString authPromptTitle(NS_LITERAL_STRING("Remote Bookmark Server Authentication"));
  nsAutoString authPromptText;
  authPromptText.AssignWithConversion(host.get());
  authPromptText.Append(NS_LITERAL_STRING("\r\n"));
  authPromptText.Append(aBindDN);

  if (nsCRT::strlen(value.get()) > 0)
  {
    // should set userDN to value
    nsAutoString temp;
    temp.AssignWithConversion(value.get());
    userDN = temp;

    rv = authPrompter->PromptPassword(authPromptTitle.get(), authPromptText.get(),
      NS_ConvertUTF8toUCS2(value).get(), nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
      getter_Copies(userPassword), &status);
  }
  else
  {
    rv = authPrompter->PromptUsernameAndPassword(authPromptTitle.get(), authPromptText.get(),
      NS_ConvertUTF8toUCS2(spec).get(), nsIAuthPrompt::SAVE_PASSWORD_PERMANENTLY,
      getter_Copies(userDN), getter_Copies(userPassword), &status);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  if (status == PR_FALSE)
    return(NS_ERROR_FAILURE);

  aBindDN = userDN;
  aPassword = userPassword;

  return(NS_OK);
}



PRBool
nsRemoteBookmarks::GetLDAPMsgAttrValue(nsILDAPMessage *aMessage, const char *aAttrib, nsString &aValue)
{
  PRBool   foundFlag = PR_FALSE;
  PRUint32 count = 1;
  PRUnichar **values = nsnull;
  nsresult rv = aMessage->GetValues(aAttrib, &count, &values);
  if (NS_SUCCEEDED(rv)&& (count == 1) && (values))
  {
    aValue = values[0];
    foundFlag = PR_TRUE;
  }
  if (values)
  {
    if (values[0])
      nsCRT::free(values[0]);
    nsCRT::free((PRUnichar *)values);
  }
  return(foundFlag);
}



nsresult
nsRemoteBookmarks::GetLDAPExtension(nsIRDFResource *aNode, const char *name,
                                    nsCString &value, PRBool *important)
{
  // See RFC 2255 for additional LDAP info, including "bindname" extension usage.

  value.Truncate();
  *important = PR_FALSE;

  nsresult rv;
  const char *uri = nsnull;
  rv = aNode->GetValueConst(&uri);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString spec(uri);

  // XXX this is just a quick hackjob for now, really need to fully parse
  // the LDAP URL for extension name/value pairs
  PRInt32 offset = spec.Find(name, PR_TRUE);
  if (offset > 0)
  {
    offset += nsCRT::strlen(name);
    PRInt32 comma = spec.FindChar(PRUnichar(','), offset);
    if (comma == kNotFound)
      comma = spec.Length();
    spec.Mid(value, offset, comma-offset);
  }
  
  // XXX shall we unescape this?  For now, let's at least do commas!
  while ((offset = value.Find("%2C", PR_TRUE)) >= 0)
  {
    value.SetCharAt(PRUnichar(','), offset);
    value.Cut(offset+1, 2);
  }

  return(NS_OK);
}



nsresult
nsRemoteBookmarks::insertLDAPBookmarkItem(nsIRDFResource *aNode, 
                                          nsISupportsArray *aArguments, 
                                          nsIRDFResource *aItemType)
{
  nsresult rv;
  nsCOMPtr<nsIRDFNode> argNode;
  if (NS_FAILED(rv = getArgumentN(aArguments, kNC_Parent,
    0, getter_AddRefs(argNode))))
    return(rv);
  nsCOMPtr<nsIRDFResource> argParent = do_QueryInterface(argNode);
  if (!argParent)
    return(NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIRDFResource> rdfRes;
  PRBool isContainerFlag = PR_FALSE;
  if (NS_SUCCEEDED(rv = gRDFC->IsSeq(mInner, aNode, &isContainerFlag)) &&
    (isContainerFlag == PR_TRUE))
  {
    rdfRes = getLDAPUrl(aNode);
  }
  else
  {
    rdfRes = getLDAPUrl(argParent);
  }
  if (!rdfRes)
    return(NS_OK);
  if (!isRemoteBookmarkURI(rdfRes))
    return(NS_OK);

  // XXX broken
  mProperty = aItemType;

  mNameLiteral = nsnull;
  mURLLiteral = nsnull;
  mShortcutLiteral = nsnull;
  mDescLiteral = nsnull;

  nsCOMPtr<nsIRDFNode> nameNode;
  getArgumentN(aArguments, kNC_Name, 0, getter_AddRefs(nameNode));
  if (nameNode)
    mNameLiteral = do_QueryInterface(nameNode);

  if (aItemType == kNC_Bookmark)
  {
    nsCOMPtr<nsIRDFNode> urlNode;
    getArgumentN(aArguments, kNC_URL, 0, getter_AddRefs(urlNode));
    if (urlNode)
      mURLLiteral = do_QueryInterface(urlNode);

    nsCOMPtr<nsIRDFNode> shortcutNode;
    getArgumentN(aArguments, kNC_ShortcutURL, 0, getter_AddRefs(shortcutNode));
    if (shortcutNode)
      mShortcutLiteral = do_QueryInterface(shortcutNode);
  }

  nsCOMPtr<nsIRDFNode> descNode;
  getArgumentN(aArguments, kNC_Description, 0, getter_AddRefs(descNode));
  if (descNode)
    mDescLiteral = do_QueryInterface(descNode);

  // Get the ldap connection
  nsCOMPtr<nsILDAPConnection> ldapConnection;
  ldapConnection = do_CreateInstance(NS_LDAPCONNECTION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString dummy;
  rv = doLDAPQuery(ldapConnection, rdfRes, aNode, dummy, dummy,
    nsRemoteBookmarks::LDAP_ADD);
  return(rv);
}



nsresult
nsRemoteBookmarks::deleteLDAPBookmarkItem(nsIRDFResource *aNode,
                                          nsISupportsArray *aArguments,
                                          PRInt32 parentArgIndex,
                                          nsIRDFResource *aItemType)
{
  nsresult rv;
  nsCOMPtr<nsIRDFNode> argNode;
  if (NS_FAILED(rv = getArgumentN(aArguments, kNC_Parent,
    parentArgIndex, getter_AddRefs(argNode))))
    return(rv);
  nsCOMPtr<nsIRDFResource> argParent = do_QueryInterface(argNode);
  if (!argParent)
    return(NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(aNode);
  if (!rdfRes)
    return(NS_OK);
  if (!isRemoteBookmarkURI(rdfRes))
    return(NS_OK);

  // Get the ldap connection
  nsCOMPtr<nsILDAPConnection> ldapConnection;
  ldapConnection = do_CreateInstance(NS_LDAPCONNECTION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString dummy;
  rv = doLDAPQuery(ldapConnection, argParent, aNode, dummy, dummy,
    nsRemoteBookmarks::LDAP_DELETE);
  return(rv);
}



nsresult
nsRemoteBookmarks::updateLDAPBookmarkItem(nsIRDFResource *aSource,
                                          nsIRDFResource *aProperty,
                                          nsIRDFNode *aOldTarget,
                                          nsIRDFNode *aNewTarget)
{
  nsresult rv;
  nsCOMPtr<nsIRDFResource> rdfRes = getLDAPUrl(aSource);
  if (!rdfRes)
    return(NS_OK);
  if (!isRemoteBookmarkURI(rdfRes))
    return(NS_OK);

  // Get the ldap connection
  nsCOMPtr<nsILDAPConnection> ldapConnection;
  ldapConnection = do_CreateInstance(NS_LDAPCONNECTION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXX broken
  mProperty = aProperty;
  mOldTarget = aOldTarget;
  mNewTarget = aNewTarget;

  nsAutoString dummy;
  rv = doLDAPQuery(ldapConnection, nsnull, aSource, dummy, dummy,
    nsRemoteBookmarks::LDAP_MODIFY);
  return(rv);
}



nsresult
nsRemoteBookmarks::getArgumentN(nsISupportsArray *arguments,
                                nsIRDFResource *res,
                                PRInt32 offset,
                                nsIRDFNode **argValue)
{
  nsresult		rv;
  PRUint32		loop, numArguments;

  *argValue = nsnull;

  if (NS_FAILED(rv = arguments->Count(&numArguments)))
    return(rv);

  // format is argument, value, argument, value, ... [you get the idea]
  // multiple arguments can be the same, by the way, thus the "offset"
  for (loop = 0; loop < numArguments; loop += 2)
  {
    nsCOMPtr<nsISupports>	aSource = arguments->ElementAt(loop);
    if (!aSource)
      return(NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsIRDFResource>	src = do_QueryInterface(aSource);
    if (!src)
      return(NS_ERROR_NO_INTERFACE);

    if (src.get() == res)
    {
      if (offset > 0)
      {
        --offset;
        continue;
      }

      nsCOMPtr<nsISupports>	aValue = arguments->ElementAt(loop + 1);
      if (!aSource)
        return(NS_ERROR_NULL_POINTER);
      nsCOMPtr<nsIRDFNode>	val = do_QueryInterface(aValue);
      if (!val)
        return(NS_ERROR_NO_INTERFACE);

      *argValue = val;
      NS_ADDREF(*argValue);
      return(NS_OK);
    }
  }
  return(NS_ERROR_INVALID_ARG);
}



PRBool
nsRemoteBookmarks::isMutableProperty(nsIRDFResource *aProperty)
{
  PRBool mutableFlag = PR_FALSE;
  if ((aProperty == kNC_Name) /* ||
      (aProperty == kNC_URL)  ||
      (aProperty == kNC_Description)  ||
      (aProperty == kNC_ShortcutURL) */ )
  {
    mutableFlag = PR_TRUE;
  }
  return(mutableFlag);
}



const char *
nsRemoteBookmarks::getPropertySchemaName(nsIRDFResource *aProperty)
{
  const char *propName = nsnull;
  if (mProperty == kNC_Name)              propName = MOZ_SCHEMA_NAME;
  else if (mProperty == kNC_ShortcutURL)  propName = MOZ_SCHEMA_KEYWORD;
  else if (mProperty == kNC_Description)  propName = MOZ_SCHEMA_DESCRIPTION;
//  else if (mProperty == kNC_URL)        propName = MOZ_SCHEMA_URL;
  return(propName);
}



nsCOMPtr<nsIRDFResource>
nsRemoteBookmarks::getLDAPUrl(nsIRDFResource *aSource)
{
  NS_IF_ADDREF(aSource);
  return(aSource);
}



PRBool
nsRemoteBookmarks::isRemoteContainer(nsIRDFResource *aNode)
{
  PRBool  isContainerFlag = PR_FALSE;
  if (isRemoteBookmarkURI(aNode) == PR_TRUE)
  {
    // Note: could be a remote URL but *not* annotated (such as a
    // top-levelremote URL coming from a different datasource)
    nsCOMPtr<nsIRDFNode> nodeType;
    nsresult rv = mInner->GetTarget(aNode, kRDF_type, PR_TRUE, getter_AddRefs(nodeType));
    nsCOMPtr<nsIRDFResource> nodeRes = do_QueryInterface(nodeType);
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE) || (nodeRes != kNC_Bookmark))
    {
      isContainerFlag = PR_TRUE;
    }
  }
  return(isContainerFlag);
}
