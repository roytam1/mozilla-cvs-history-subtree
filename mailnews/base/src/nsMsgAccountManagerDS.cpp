/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*
 * RDF datasource for the account manager
 */

#include "nsMsgAccountManagerDS.h"


#include "rdf.h"
#include "nsRDFCID.h"
#include "nsIRDFDataSource.h"
#include "nsEnumeratorUtils.h"
#include "nsIServiceManager.h"
#include "nsIMsgMailSession.h"

#include "nsXPIDLString.h"

#include "nsMsgRDFUtils.h"
#include "nsIMsgFolder.h"
#include "nsMsgBaseCID.h"

// turn this on to see useful output
#undef DEBUG_amds

static NS_DEFINE_CID(kMsgMailSessionCID, NS_MSGMAILSESSION_CID);
static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

#define NC_RDF_IDENTITYROOT "identity://"
#define NC_RDF_IDENTITYROOT_LEN (sizeof("identity://")-1)/sizeof(char)

#define NC_RDF_PAGETITLE_MAIN     NC_NAMESPACE_URI "PageTitleMain"
#define NC_RDF_PAGETITLE_SERVER   NC_NAMESPACE_URI "PageTitleServer"
#define NC_RDF_PAGETITLE_COPIES   NC_NAMESPACE_URI "PageTitleCopies"
#define NC_RDF_PAGETITLE_ADVANCED NC_NAMESPACE_URI "PageTitleAdvanced"
#define NC_RDF_PAGETITLE_SMTP     NC_NAMESPACE_URI "PageTitleSMTP"
#define NC_RDF_PAGETAG NC_NAMESPACE_URI "PageTag"

typedef struct _serverCreationEntry {
  nsISupportsArray *serverArray;
  nsIRDFService *rdfService;
} serverCreationEntry;

typedef struct _identityCreationEntry {
  nsISupportsArray *identityArray;
  nsIRDFService *rdfService;
} identityCreationEntry;

typedef struct _getUniqueIdentitiesEntry {
  nsISupportsArray *uniqueIdentities;
} getUniqueIdentitiesEntry;

typedef struct _findSimilarIdentityEntry {
  const char *fullName;
  const char *email;
  nsIMsgIdentity *similarIdentity;
} findSimilarIdentityEntry;

// static members
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Child=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_UniqueChild=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Name=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_NameSort=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTag=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Settings=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_ServerRoot=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_IdentityRoot=nsnull;

// properties corresponding to interfaces
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Account=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Server=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_Identity=nsnull;

// individual pages
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleMain=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleServer=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleCopies=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleAdvanced=nsnull;
nsIRDFResource* nsMsgAccountManagerDataSource::kNC_PageTitleSMTP=nsnull;

nsrefcnt nsMsgAccountManagerDataSource::gAccountManagerResourceRefCnt = 0;

// RDF to match
#define NC_RDF_ACCOUNT NC_NAMESPACE_URI "Account"
#define NC_RDF_SERVER  NC_NAMESPACE_URI "Server"
#define NC_RDF_IDENTITY NC_NAMESPACE_URI "Identity"
#define NC_RDF_SETTINGS NC_NAMESPACE_URI "Settings"


nsMsgAccountManagerDataSource::nsMsgAccountManagerDataSource():
  mAccountManager(null_nsCOMPtr())
{

#ifdef DEBUG_amds
  printf("nsMsgAccountManagerDataSource() being created\n");
#endif

  // XXX This call should be moved to a NS_NewMsgFooDataSource()
  // method that the factory calls, so that failure to construct
  // will return an error code instead of returning a partially
  // initialized object.
  nsresult rv = Init();
  NS_ASSERTION(NS_SUCCEEDED(rv), "uh oh, initialization failed");
  if (NS_FAILED(rv)) return /* rv */;

  return /* NS_OK */;
}

nsMsgAccountManagerDataSource::~nsMsgAccountManagerDataSource()
{
	if (--gAccountManagerResourceRefCnt == 0)
	{
      NS_IF_RELEASE(kNC_Child);
      NS_IF_RELEASE(kNC_UniqueChild);
      NS_IF_RELEASE(kNC_Name);
      NS_IF_RELEASE(kNC_NameSort);
      NS_IF_RELEASE(kNC_PageTag);
      NS_IF_RELEASE(kNC_Account);
      NS_IF_RELEASE(kNC_Server);
      NS_IF_RELEASE(kNC_Identity);
      NS_IF_RELEASE(kNC_PageTitleMain);
      NS_IF_RELEASE(kNC_PageTitleServer);
      NS_IF_RELEASE(kNC_PageTitleCopies);
      NS_IF_RELEASE(kNC_PageTitleAdvanced);
      NS_IF_RELEASE(kNC_PageTitleSMTP);
      
      NS_IF_RELEASE(kNC_ServerRoot);
      NS_IF_RELEASE(kNC_IdentityRoot);
      
      // eventually these need to exist in some kind of array
      // that's easily extensible
      NS_IF_RELEASE(kNC_Settings);
	}

}

nsresult
nsMsgAccountManagerDataSource::Init()
{
    nsresult rv=NS_OK;
    
    if (!mAccountManager) {
        NS_WITH_SERVICE(nsIMsgMailSession, mailSession,
                        kMsgMailSessionCID, &rv);
        if (NS_FAILED(rv)) return rv;
        
        // maybe the account manager should be a service too? not sure.
        rv = mailSession->GetAccountManager(getter_AddRefs(mAccountManager));
        if (NS_FAILED(rv)) return rv;
    }
    
	if (gAccountManagerResourceRefCnt++ == 0) {
      getRDFService()->GetResource(NC_RDF_CHILD, &kNC_Child);
      getRDFService()->GetResource(NC_RDF_CHILD, &kNC_UniqueChild);
      getRDFService()->GetResource(NC_RDF_NAME, &kNC_Name);
      getRDFService()->GetResource(NC_RDF_NAME_SORT, &kNC_NameSort);
      getRDFService()->GetResource(NC_RDF_PAGETAG, &kNC_PageTag);
      getRDFService()->GetResource(NC_RDF_ACCOUNT, &kNC_Account);
      getRDFService()->GetResource(NC_RDF_SERVER, &kNC_Server);
      getRDFService()->GetResource(NC_RDF_IDENTITY, &kNC_Identity);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_MAIN, &kNC_PageTitleMain);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_SERVER, &kNC_PageTitleServer);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_COPIES, &kNC_PageTitleCopies);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_ADVANCED, &kNC_PageTitleAdvanced);
      getRDFService()->GetResource(NC_RDF_PAGETITLE_SMTP, &kNC_PageTitleSMTP);
      
      getRDFService()->GetResource(NC_RDF_MSGSERVERROOT, &kNC_ServerRoot);
      getRDFService()->GetResource(NC_RDF_MSGIDENTITYROOT, &kNC_IdentityRoot);
      
      // eventually these need to exist in some kind of array
      // that's easily extensible
      getRDFService()->GetResource(NC_RDF_SETTINGS, &kNC_Settings);
    }
    return NS_OK;
}

void nsMsgAccountManagerDataSource::Close()
{
	mAccountManager = null_nsCOMPtr();
	nsMsgRDFDataSource::Close();
}

/* nsIRDFNode GetTarget (in nsIRDFResource aSource, in nsIRDFResource property, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::GetTarget(nsIRDFResource *source,
                                         nsIRDFResource *property,
                                         PRBool aTruthValue,
                                         nsIRDFNode **target)
{
  nsresult rv;
  
  
  rv = NS_RDF_NO_VALUE;

  nsString str="";

  
  nsXPIDLCString uri;
  source->GetValue(getter_Copies(uri));

  nsCOMPtr<nsIMsgFolder> folder;
  //
  // nsIMsgIdentity
  //
  if (uri &&
      !nsCRT::strncmp(uri, NC_RDF_IDENTITYROOT, NC_RDF_IDENTITYROOT_LEN)) {
    const char *key = uri + NC_RDF_IDENTITYROOT_LEN;
    
    nsCOMPtr<nsIMsgIdentity> identity;
    rv = mAccountManager->GetIdentity(key, getter_AddRefs(identity));

    if (NS_FAILED(rv)) return rv;
    if (!identity) return NS_ERROR_NULL_POINTER;
    
    if (property == kNC_Name) {

      nsXPIDLCString fullName;
      rv = identity->GetFullName(getter_Copies(fullName));
      if (NS_SUCCEEDED(rv))
        str += fullName;

      nsXPIDLCString email;
      rv = identity->GetEmail(getter_Copies(email));
      if (NS_SUCCEEDED(rv)) {
        str += " <";
        str += email;
        str += ">";
      }
    }
  }

  //
  // nsIMsgIncomingServer
  //
  else if ((folder = do_QueryInterface(source))) {
    PRBool isServer=PR_FALSE;
    folder->GetIsServer(&isServer);
    if (!isServer)
      return NS_RDF_NO_VALUE;


    // name of server
    if (property == kNC_Name) {
      nsXPIDLString prettyName;
      rv = folder->GetPrettyName(getter_Copies(prettyName));
      if (NS_SUCCEEDED(rv))
        str = prettyName;
    }
    
    // handle sorting of servers
    else if (property == kNC_NameSort) {
    
      nsCOMPtr<nsIMsgIncomingServer> server;
      rv = folder->GetServer(getter_AddRefs(server));
      if (NS_FAILED(rv)) return rv;
      
      PRInt32 accountNum;
      rv = mAccountManager->FindServerIndex(server, &accountNum);
      if (NS_FAILED(rv)) return rv;
      
      accountNum += 1000;
      str.Append(accountNum);
    }
    
    // this is a hack, it makes the account manager work
    else if (property == kNC_PageTag)
      str = "am-main.xul";
  }

  //
  // Account manager pages
  //
  else if (source == kNC_PageTitleMain) {
    if (property == kNC_Name)
      str = "Main";
    else if (property == kNC_PageTag)
      str = "am-main.xul";
  }
  
  else if (source == kNC_PageTitleServer) {
    if (property == kNC_Name)
      str = "Server";
    else if (property == kNC_PageTag)
      str = "am-server.xul";
  }
  
  else if (source == kNC_PageTitleCopies) {
    if (property == kNC_Name)
      str = "Copies and Folders";
    else if (property == kNC_PageTag)
      str = "am-copies.xul";
  }
  
  else if (source == kNC_PageTitleAdvanced) {
    if (property == kNC_Name)
      str = "Advanced";
    else if (property == kNC_PageTag)
      str = "am-advanced.xul";
  }
  
  else if (source == kNC_PageTitleSMTP) {
    if (property == kNC_Name)
      str = "Outgoing (SMTP) Server";
    else if (property == kNC_PageTag)
      str = "am-smtp.xul";
  }

  if (str!="")
    rv = createNode(str, target, getRDFService());
  //if we have an empty string and we don't have an error value, then 
  //we don't have a value for RDF.
  else if(NS_SUCCEEDED(rv))
	rv = NS_RDF_NO_VALUE;

  return rv;
}



/* nsISimpleEnumerator GetTargets (in nsIRDFResource aSource, in nsIRDFResource property, in boolean aTruthValue); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::GetTargets(nsIRDFResource *source,
                                   nsIRDFResource *property,
                                   PRBool aTruthValue,
                                   nsISimpleEnumerator **_retval)
{
  nsresult rv = NS_RDF_NO_VALUE;

  // create array and enumerator
  // even if we're not handling this we need to return something empty?
  nsCOMPtr<nsISupportsArray> nodes;
  rv = NS_NewISupportsArray(getter_AddRefs(nodes));
  if (NS_FAILED(rv)) return rv;
  
  nsISimpleEnumerator* enumerator =
    new nsArrayEnumerator(nodes);
  if (!enumerator) return NS_ERROR_OUT_OF_MEMORY;

  *_retval = enumerator;
  NS_ADDREF(*_retval);

  // if the property is "account" or "child" then return the
  // list of accounts
  // if the property is "server" return a union of all servers
  // in the account (mainly for the folder pane)

#ifdef DEBUG_amds
  nsXPIDLCString source_value;
  rv = source->GetValue(getter_Copies(source_value));

  nsXPIDLCString property_arc;
  rv = property->GetValue(getter_Copies(property_arc));
  if (NS_FAILED(rv)) return rv;
  
  printf("GetTargets(%s with arc %s...)\n",
         (const char*)source_value,
         (const char*)property_arc);
#endif
  
  if (source == kNC_ServerRoot) {

    if (property == kNC_Child ||
        property == kNC_Settings) {
      
      nsCOMPtr<nsISupportsArray> servers;
      rv = mAccountManager->GetAllServers(getter_AddRefs(servers));
      
      // fill up the nodes array with the RDF Resources for the servers
      serverCreationEntry entry = { nodes, getRDFService() };
      servers->EnumerateForwards(createServerResources, (void*)&entry);
#ifdef DEBUG_amds
        PRUint32 nodecount;
        nodes->Count(&nodecount);
      printf("GetTargets(): added %d servers on %s\n", nodecount,
             (const char*)property_arc);
#endif

      // for the "settings" arc, we also want to do an SMTP tag
      if (property == kNC_Settings) {
        nodes->AppendElement(kNC_PageTitleSMTP);
      }
    }
#ifdef DEBUG_amds
    else {
      printf("unknown arc %s on " NC_RDF_MSGSERVERROOT "\n", (const char*)property_arc);
    }
#endif

  } else if (source == kNC_IdentityRoot) {
    if (property == kNC_Child ||
        property == kNC_UniqueChild) {
      nsCOMPtr<nsISupportsArray> identities;

      mAccountManager->GetAllIdentities(getter_AddRefs(identities));

      if (property == kNC_UniqueChild) {
        // filter out the unique identities
        nsCOMPtr<nsISupportsArray> uniqueIdentities;
        NS_NewISupportsArray(getter_AddRefs(uniqueIdentities));
        
        getUniqueIdentitiesEntry entry = { uniqueIdentities };
        identities->EnumerateForwards(getUniqueIdentities, (void *)&entry);

      }

      identityCreationEntry entry = { nodes, getRDFService() };
      identities->EnumerateForwards(createIdentityResources, (void *)&entry);
      
    }


  } else {
    /* if this is a server, then support the settings */
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(source, &rv);

    if (NS_SUCCEEDED(rv)) {
      // all the Settings - main, server, copies, advanced, etc
      if (property == kNC_Settings) {
        
        if (NS_SUCCEEDED(rv)) nodes->AppendElement(kNC_PageTitleServer);
        if (NS_SUCCEEDED(rv)) nodes->AppendElement(kNC_PageTitleCopies);
        
      }
    } else if (source == kNC_PageTitleSMTP) {
      if (property == kNC_Settings) {
        //        nodes->AppendElement(kNC_PageTitleAdvanced);
      }
    }
    
        
#ifdef DEBUG_amds
    else {
      printf("GetTargets(): Unknown source %s\n", (const char*)source_value);
    }
#endif
  }
  
  return NS_OK;
}

// enumeration function to convert each server (element)
// to an nsIRDFResource and append it to the array (in data)
// always return PR_TRUE to try on every element instead of aborting early
PRBool
nsMsgAccountManagerDataSource::createServerResources(nsISupports *element,
                                                     void *data)
{
  nsresult rv;
  // get parameters out of the data argument
  serverCreationEntry *entry = (serverCreationEntry*)data;
  nsCOMPtr<nsISupportsArray> servers = dont_QueryInterface(entry->serverArray);
  nsCOMPtr<nsIRDFService> rdf = dont_QueryInterface(entry->rdfService);

  // the server itself is in the element argument
  nsCOMPtr<nsIMsgIncomingServer> server = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return PR_TRUE;

	nsCOMPtr <nsIFolder> serverFolder;
	rv = server->GetRootFolder(getter_AddRefs(serverFolder));
	if(NS_FAILED(rv)) return PR_TRUE;

  if (serverFolder) {
    nsXPIDLString serverName;
    server->GetPrettyName(getter_Copies(serverName));
    serverFolder->SetPrettyName(serverName);
  }

  // add the resource to the array
  nsCOMPtr<nsIRDFResource> serverResource = do_QueryInterface(serverFolder);
	if(!serverResource)
		return PR_TRUE;

  rv = servers->AppendElement(serverResource);
  if (NS_FAILED(rv)) return PR_TRUE;
  
  return PR_TRUE;
}


// for each identity, get the resource and append it to the given array
PRBool
nsMsgAccountManagerDataSource::createIdentityResources(nsISupports* element,
                                                       void *data)
{
  nsresult rv;

  // demarshall
  identityCreationEntry *entry = (identityCreationEntry*)data;
  nsCOMPtr<nsISupportsArray> identities =
    dont_QueryInterface(entry->identityArray);
  nsCOMPtr<nsIRDFService> rdf = dont_QueryInterface(entry->rdfService);
  nsCOMPtr<nsIMsgIdentity> identity = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return PR_TRUE;

  // the identity resources are identitified by the key
  nsXPIDLCString key;
  rv = identity->GetKey(getter_Copies(key));
  if (NS_FAILED(rv)) return PR_TRUE;

  // create URI: "identity://<key>"
  nsCAutoString identityURI("identity://");
  identityURI += key;

  // get the resource and append it to the list
  nsCOMPtr<nsIRDFResource> identityResource;
  rv = rdf->GetResource(identityURI.GetBuffer(),
                        getter_AddRefs(identityResource));
  if (NS_FAILED(rv)) return PR_TRUE;
  identities->AppendElement(identityResource);
  
  return PR_TRUE;
}

// for each identity and an array of "found" identities,
// add the identity to the array only if it's not already there
PRBool
nsMsgAccountManagerDataSource::getUniqueIdentities(nsISupports* element,
                                                   void *data)
{
  nsresult rv;

  // demarshall
  getUniqueIdentitiesEntry *entry = (getUniqueIdentitiesEntry*)data;
  nsCOMPtr<nsISupportsArray> uniqueIdentities =
    dont_QueryInterface(entry->uniqueIdentities);
  nsCOMPtr<nsIMsgIdentity> identity = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return PR_TRUE;

  // extract fields we want to search on
  nsXPIDLCString fullName;
  rv = identity->GetFullName(getter_Copies(fullName));
  if (NS_FAILED(rv)) return PR_TRUE;
  
  nsXPIDLCString email;
  identity->GetEmail(getter_Copies(email));
  if (NS_FAILED(rv)) return PR_TRUE;

  // now search the array for the entry with these fields
  findSimilarIdentityEntry findEntry = { fullName, email, nsnull };
  uniqueIdentities->EnumerateForwards(findSimilarIdentity, (void *)&findEntry);

  // if the identity isn't found, it's unique, so add it to the list
  if (!findEntry.similarIdentity)
    uniqueIdentities->AppendElement(identity);

  return PR_TRUE;
}

// for each identity, if the fullName and email are similar
// then return that identity
PRBool
nsMsgAccountManagerDataSource::findSimilarIdentity(nsISupports* element,
                                                   void *data)
{
  nsresult rv;

  // demarshal
  findSimilarIdentityEntry *entry = (findSimilarIdentityEntry*)data;
  nsCOMPtr<nsIMsgIdentity> candidate = do_QueryInterface(element, &rv);
  if (NS_FAILED(rv)) return PR_TRUE;

  if (similarIdentities(entry->fullName, entry->email, candidate)) {
    entry->similarIdentity = candidate;
    return PR_FALSE;            // stop when found
  }
  return PR_TRUE;
}

PRBool
nsMsgAccountManagerDataSource::similarIdentities(const char *fullName,
                                                 const char *email,
                                                 nsIMsgIdentity *id)
{
  nsXPIDLCString idFullName;

  nsXPIDLCString idEmail;

  nsresult rv;
  rv = id->GetFullName(getter_Copies(idFullName));
  if (NS_FAILED(rv)) return PR_FALSE;

  rv = id->GetEmail(getter_Copies(idEmail));
  if (NS_FAILED(rv)) return PR_FALSE;

  if ((fullName && idFullName && !PL_strcmp(fullName, idFullName)) &&
      (email    && idEmail    && !PL_strcmp(email,    idEmail)))
    return PR_TRUE;

  return PR_FALSE;
}

/* nsISimpleEnumerator ArcLabelsOut (in nsIRDFResource aSource); */
NS_IMETHODIMP
nsMsgAccountManagerDataSource::ArcLabelsOut(nsIRDFResource *source,
                                            nsISimpleEnumerator **_retval)
{
  nsresult rv;
  
  // we have to return something, so always create the array/enumerators
  nsCOMPtr<nsISupportsArray> arcs;
  rv = NS_NewISupportsArray(getter_AddRefs(arcs));

  if (NS_FAILED(rv)) return rv;
  
  nsArrayEnumerator* enumerator =
    new nsArrayEnumerator(arcs);

  if (!enumerator) return NS_ERROR_OUT_OF_MEMORY;
  
  *_retval = enumerator;
  NS_ADDREF(*_retval);
  
  if (source == kNC_ServerRoot) {
    arcs->AppendElement(kNC_Server);
	arcs->AppendElement(kNC_Child);
  }

  if (source == kNC_IdentityRoot) {
    arcs->AppendElement(kNC_Child);
    arcs->AppendElement(kNC_UniqueChild);
  }

  arcs->AppendElement(kNC_Settings);
  arcs->AppendElement(kNC_Name);
  arcs->AppendElement(kNC_NameSort);
  arcs->AppendElement(kNC_PageTag);

#ifdef DEBUG_amds_
  printf("GetArcLabelsOut(%s): Adding child, settings, and name arclabels\n", value);
#endif
  
  return NS_OK;
}


