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
#include "rdf.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFService.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsRDFCID.h"
#include "nsIEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "prprf.h"
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
#include "nsAutoLock.h"
#include "nsIProxyObjectManager.h"
#include "prprf.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"



#define RDF_AGGREGATION_BROKEN_IN_BOOKMARKS_DAMNIT  1       // someone will suffer for this, I promise

#define REMOTE_BOOKMARK_PREFIX         "moz-bookmark-"
#define REMOTE_BOOKMARK_PREFIX_LENGTH  (sizeof(REMOTE_BOOKMARK_PREFIX)-1)


#define NS_LDAPCONNECTION_CONTRACTID   "@mozilla.org/network/ldap-connection;1" 
#define NS_LDAPOPERATION_CONTRACTID    "@mozilla.org/network/ldap-operation;1" 
#define NS_LDAPMESSAGE_CONTRACTID      "@mozilla.org/network/ldap-message;1"
#define NS_LDAPURL_CONTRACTID          "@mozilla.org/network/ldap-url;1"

static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,   NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,       NS_RDFCONTAINERUTILS_CID);



static PRInt32 gRefCnt=0;
static nsIRDFService *gRDF;
static nsIRDFContainerUtils *gRDFC;

nsIRDFResource *nsRemoteBookmarks::kRDF_type;
nsIRDFResource *nsRemoteBookmarks::kNC_Bookmark;
nsIRDFResource *nsRemoteBookmarks::kNC_Folder;
nsIRDFResource *nsRemoteBookmarks::kNC_Child;
nsIRDFResource *nsRemoteBookmarks::kNC_Name;
nsIRDFResource *nsRemoteBookmarks::kNC_URL;
nsIRDFResource *nsRemoteBookmarks::kNC_LDAPURL;



////////////////////////////////////////////////////////////////////////
// RemoteBookmarkDataSourceImpl

NS_IMPL_THREADSAFE_QUERY_INTERFACE4(nsRemoteBookmarks,
                                    nsIRemoteBookmarks,
                                    nsIRDFDataSource,
                                    nsIRDFObserver,
                                    nsILDAPMessageListener)



nsRemoteBookmarks::nsRemoteBookmarks()
	: mInner(nsnull), mUpdateBatchNest(0)
{
	NS_INIT_REFCNT();
}



nsRemoteBookmarks::~nsRemoteBookmarks()
{
  if (--gRefCnt == 0)
  {
    NS_IF_RELEASE(kRDF_type);
    NS_IF_RELEASE(kNC_Bookmark);
    NS_IF_RELEASE(kNC_Folder);
    NS_IF_RELEASE(kNC_Child);
    NS_IF_RELEASE(kNC_Name);
    NS_IF_RELEASE(kNC_URL);
    NS_IF_RELEASE(kNC_LDAPURL);

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

    gRDF->GetResource(RDF_NAMESPACE_URI "type",    &kRDF_type);
    gRDF->GetResource(NC_NAMESPACE_URI "Bookmark", &kNC_Bookmark);
    gRDF->GetResource(NC_NAMESPACE_URI "Folder",   &kNC_Folder);
    gRDF->GetResource(NC_NAMESPACE_URI "child",    &kNC_Child);
    gRDF->GetResource(NC_NAMESPACE_URI "Name",     &kNC_Name);
    gRDF->GetResource(NC_NAMESPACE_URI "URL",      &kNC_URL);
    gRDF->GetResource(NC_NAMESPACE_URI "LDAPURL",  &kNC_LDAPURL);
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
    if (isRemoteBookmarkURI(aSource) && (aProperty == kNC_Child))
    {
      // Basically, lie (ie return anything) to the XUL Template
      // builder to ensure item is always an open-able container
      *aTarget = aSource;
      NS_IF_ADDREF(*aTarget);
      return(NS_OK);
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
      rv = GetLDAPExtension(aSource, "bindname=", bindname, &importantFlag);

    #ifdef  DEBUG
      printf("\n    GetTargets() bindname = '%s' \n\n", bindname.get());
    #endif

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

      rv = doLDAPQuery(ldapConnection, aSource, bindDN, password);
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
  return(NS_RDF_ASSERTION_REJECTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::Unassert(nsIRDFResource* aSource,
                            nsIRDFResource* aProperty,
                            nsIRDFNode* aTarget)
{
  return(NS_RDF_ASSERTION_REJECTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::Change(nsIRDFResource* aSource,
                          nsIRDFResource* aProperty,
                          nsIRDFNode* aOldTarget,
                          nsIRDFNode* aNewTarget)
{
  return(NS_RDF_ASSERTION_REJECTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::Move(nsIRDFResource* aOldSource,
                        nsIRDFResource* aNewSource,
                        nsIRDFResource* aProperty,
                        nsIRDFNode* aTarget)
{
  return(NS_RDF_ASSERTION_REJECTED);
}



NS_IMETHODIMP
nsRemoteBookmarks::HasAssertion(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget,
                                PRBool tv,
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

  if (mInner)
  {
    if (isRemoteBookmarkURI(aSource) && (aProperty == kNC_Child) && (tv == PR_TRUE))
    {
      *aHasAssertion = PR_TRUE;
      return(NS_OK);
    }
    // fallback to querying mInner
    nsresult rv = mInner->HasAssertion(aSource, aProperty, aTarget, tv, aHasAssertion);
    return(rv);
  }
	return(NS_OK);
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
      *_retval = PR_TRUE;
      return(NS_OK);
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
      array->AppendElement(kNC_Name);
      array->AppendElement(kNC_URL);
      array->AppendElement(kNC_LDAPURL);

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
	return(NS_ERROR_NOT_IMPLEMENTED);
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

      (void) obs->OnAssert(this, aSource, aProperty, aTarget);
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

      (void) obs->OnUnassert(this, aSource, aProperty, aTarget);
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

      (void) obs->OnChange(this, aSource, aProperty, aOldTarget, aNewTarget);
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

      (void) obs->OnMove(this, aOldSource, aNewSource, aProperty, aTarget);
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
// nsILDAPMessageListener



NS_IMETHODIMP
nsRemoteBookmarks::OnLDAPInit(nsresult aStatus)
{
  // Make sure that the Init() worked properly
  NS_ENSURE_SUCCESS(aStatus, aStatus);

  // Initiate the LDAP operation - XXX leak for now
  nsresult rv;
  mSearchOperation = do_CreateInstance(NS_LDAPOPERATION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILDAPMessageListener> proxyListener;
  rv = NS_GetProxyForObject(NS_UI_THREAD_EVENTQ,
    NS_GET_IID(nsILDAPMessageListener),
    NS_STATIC_CAST(nsILDAPMessageListener *, this),
    PROXY_SYNC | PROXY_ALWAYS,
    getter_AddRefs(proxyListener));
  NS_ENSURE_SUCCESS(rv, rv);

  //  XXX TO DO need to somehow get "mConnection" from somewhere
  rv = mSearchOperation->Init(mConnection, proxyListener);
  NS_ENSURE_SUCCESS(rv, rv);

  // simple Bind (password may be empty string, initially)
  rv = mSearchOperation->SimpleBind(mPassword.get());
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

          // get the window watcher service, so we can get a prompter
          //
          nsCOMPtr<nsIPrompt> prompter;
          rv = mWindowWatcher->GetNewPrompter(0, getter_AddRefs(prompter));
          NS_ENSURE_SUCCESS(rv, rv);
          if (!prompter)
            return(NS_ERROR_NULL_POINTER);

          nsAutoString errStr;
          rv = aMessage->GetErrorMessage(errStr);
          errStr.Trim(" \t");
          if (errStr.Length() == 0)
          {
            errStr = NS_LITERAL_STRING("Error # ");
            errStr.AppendInt(errorCode);
          }
          prompter->Alert(NS_LITERAL_STRING("Error").get(), errStr.get());

          // rebind

          nsCOMPtr<nsIRDFResource> containerRes;
          rv = mContainer->GetResource(getter_AddRefs(containerRes));
          NS_ENSURE_SUCCESS(rv, rv);

          nsAutoString bindDN, password;
          nsresult rv = doAuthentication(containerRes, bindDN, password);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = doLDAPQuery(mConnection, containerRes, bindDN, password);
          return(rv);
        }
        return(NS_ERROR_FAILURE);
      }

      nsXPIDLCString dn;
      rv = mLDAPURL->GetDn(getter_Copies (dn));
      NS_ENSURE_SUCCESS(rv, rv);

      nsXPIDLCString filter;
      rv = mLDAPURL->GetFilter(getter_Copies (filter));
      NS_ENSURE_SUCCESS(rv, rv);

      static const char *attrs[] = {
      "mozillaURL", "mozillaName", "objectclass", nsnull
      };

      // when we called SetSpec(), our spec contained UTF8 data
      // so when we call GetFilter(), it's not going to be ASCII.
      // it will be UTF8, so we need to convert from UTF8 to UCS2
      // see bug #124995
      rv = mSearchOperation->SearchExt(NS_ConvertUTF8toUCS2(dn).get(),
        nsILDAPURL::SCOPE_ONELEVEL, NS_ConvertUTF8toUCS2(filter).get(),
        3,                            /* attributes.GetSize () */
        (const char **)attrs,         /* attributes.GetArray () */
        nsILDAPOperation::NO_LIMIT,   /* TimeOut*/
        nsILDAPOperation::NO_LIMIT ); /* ResultLimit*/
      NS_ENSURE_SUCCESS(rv, rv);
      }
      break;

    // found a result
    case nsILDAPMessage::RES_SEARCH_ENTRY:
      {

#ifdef  DEBUG
      printf("    nsILDAPMessage::RES_SEARCH_ENTRY done \n");
#endif

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
      char* _ldapSearchUrlString = PR_smprintf ("ldap%s://%s:%d/%s",
          (options & nsILDAPURL::OPT_SECURE) ? "s" : "",
          host.get(), port, cdn.get());
      if (!_ldapSearchUrlString)
        return(NS_ERROR_OUT_OF_MEMORY);

#ifdef  DEBUG
      printf("    Search UTF8 DN - %s \n", _ldapSearchUrlString);
#endif

      ldapSearchUrlString = _ldapSearchUrlString;
      PR_smprintf_free (_ldapSearchUrlString);

      nsCOMPtr<nsIRDFResource> searchRes;

      nsAutoString classStr, urlStr, nameStr;
      GetLDAPMsgAttrValue(aMessage, "objectclass", classStr);
      classStr.Trim(" \t");
      if (classStr.EqualsIgnoreCase("mozillaBookmark"))
      {
        // its a Bookmark
        GetLDAPMsgAttrValue(aMessage, "mozillaName", nameStr);
        GetLDAPMsgAttrValue(aMessage, "mozillaURL", urlStr);
        urlStr.Trim(" \t");
        if (urlStr.Length() < 1)
          return(NS_OK);

#ifdef  RDF_AGGREGATION_BROKEN_IN_BOOKMARKS_DAMNIT
        rv = gRDF->GetUnicodeResource(urlStr.get(), getter_AddRefs(searchRes));
#else
        rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(searchRes));
#endif
        NS_ENSURE_SUCCESS(rv, rv);

        // set its type
        rv = mInner->Assert(searchRes, kRDF_type, kNC_Bookmark, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (classStr.EqualsIgnoreCase("mozillaFolder"))
      {
        // its a Folder
        GetLDAPMsgAttrValue(aMessage, "mozillaName", nameStr);

        // set its true URI
        ldapSearchUrlString.Insert(REMOTE_BOOKMARK_PREFIX, 0);
        rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(searchRes));
        NS_ENSURE_SUCCESS(rv, rv);

        // set its type
        rv = mInner->Assert(searchRes, kRDF_type, kNC_Folder, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (nameStr.Length() > 0)
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

      nsCOMPtr<nsIRDFResource> urlRes;
#ifdef  RDF_AGGREGATION_BROKEN_IN_BOOKMARKS_DAMNIT
      // with RDF aggregation non-functional, we can't just assert #URL;
      // so we have to be tricky and hang the true LDAP URL off of #LDAPURL
      rv = gRDF->GetUnicodeResource(urlStr.get(), getter_AddRefs(urlRes));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mInner->Assert(searchRes, kNC_LDAPURL, urlRes, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
#else
      // if RDF aggregation were functional, we'd just assert #URL
      rv = gRDF->GetResource(ldapSearchUrlString.get(), getter_AddRefs(urlRes));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mInner->Assert(searchRes, kNC_URL, urlRes, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
#endif

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
#ifdef  DEBUG
      printf("    nsILDAPMessage::RES_SEARCH_RESULT done \n");
#endif

      PRInt32 count = 0;
      if (errorCode == nsILDAPErrors::INAPPROPRIATE_AUTH ||
          errorCode == nsILDAPErrors::INVALID_CREDENTIALS ||
          errorCode == nsILDAPErrors::INSUFFICIENT_ACCESS ||
          ((errorCode == nsILDAPErrors::SUCCESS) &&
           (NS_SUCCEEDED(rv = mContainer->GetCount(&count)) &&
           (count == 0) && (mPassword.IsEmpty()))))
      {
        // rebind
        nsCOMPtr<nsIRDFResource> containerRes;
        rv = mContainer->GetResource(getter_AddRefs(containerRes));
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString bindDN, password;
        nsresult rv = doAuthentication(containerRes, bindDN, password);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doLDAPQuery(mConnection, containerRes, bindDN, password);
        return(rv);
      }

      if (errorCode != nsILDAPErrors::SUCCESS)
      {
        // display error
        nsCOMPtr<nsIPrompt> prompter;
        rv = mWindowWatcher->GetNewPrompter(0, getter_AddRefs(prompter));
        if (prompter)
        {
          nsAutoString errStr;
          rv = aMessage->GetErrorMessage(errStr);
          errStr.Trim(" \t");
          if (errStr.Length() == 0)
          {
            errStr = NS_LITERAL_STRING("Error # ");
            errStr.AppendInt(errorCode);
          }
          prompter->Alert(NS_LITERAL_STRING("Error").get(), errStr.get());
        }
      }

      // XXX XXX XXX hack TO DO
      // undo leakage from above
      mSearchOperation->Abandon();
      mSearchOperation = nsnull;
      mLDAPURL = nsnull;
      mConnection = nsnull;
      mContainer = nsnull;
      break;
  }
  return(NS_OK);
}



nsresult
nsRemoteBookmarks::doLDAPQuery(nsILDAPConnection *ldapConnection,
                               nsIRDFResource *aSource,
                               nsString bindDN,
                               nsString password)
{
  // XXX broken
  mLDAPURL = nsnull;
  mContainer = nsnull;
  mPassword.Truncate();

  nsCOMPtr<nsIRDFContainer> ldapContainer;
  nsresult rv = gRDFC->MakeSeq(mInner, aSource, getter_AddRefs(ldapContainer));
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make aSource a sequence");
  if (NS_FAILED(rv))
    return(rv);

  nsCOMPtr<nsILDAPURL> ldapURL;
  ldapURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const char *srcURI = nsnull;
  aSource->GetValueConst(&srcURI);
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
  mPassword = password;
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
    mPassword.Truncate();
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

  nsresult rv;
  nsCOMPtr<nsILDAPURL> ldapURL;
  ldapURL = do_CreateInstance(NS_LDAPURL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  const char *srcURI = nsnull;
  aSource->GetValueConst(&srcURI);
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
  //
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
