/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

  Implementation for an mail RDF data store.
  It supports the following kinds of objects.
  - mail message
  - mail folders

  There is a single Mail Data Source Object. When the data source is
  initialized, it reads in the list of accounts and their folders.
  The summary file for a folder is read in on demand (either when the
  folder is asked for its children or when one of the properties of a
  mail message is queried).

  Mail Accounts, folders and messages are represented by
  nsIMailAccount, nsIMailFolder and nsIMailMessage, which are
  subclasses of nsIRDFResource.

  The implementations of these interfaces provided here assume certain
  standard fields for each of these kinds of objects.

  The MailDataSource can only store information about and answer
  queries pertaining to the standard mail related properties of mail
  objects. Other properties of mail objects and properties about
  non-mail objects which might be in mail folders are taken care off
  by other data sources (such as the default local store).

  TO DO

  1) Create nsIRDFPerson.

  2) Move more of the data to nsMiscMailData

  3) I am missing about 60% of the add and release refs.

  4) hook up observers.
  
 */

#include "nsMailDataSource.h"

/********************************** MailDataSource **************************************
 ************************************************************************************/

NS_IMPL_ISUPPORTS(MailDataSource, kIRDFMailDataSourceIID);


NS_IMETHODIMP MailDataSource::AddAccount (nsIRDFMailAccount* folder) {
    return gMailDataSource->mMiscMailData->Assert(gMailDataSource->mMailRoot, 
                                                  gMailDataSource->mResourceFolder,
                                                  folder, 1);
}

NS_IMETHODIMP MailDataSource::RemoveAccount (nsIRDFMailAccount* folder) {
    return gMailDataSource->mMiscMailData->Unassert(gMailDataSource->mMailRoot, 
                                                    gMailDataSource->mResourceFolder,
                                                    folder);
}

MailDataSource::MailDataSource(void) {
    NS_INIT_REFCNT();
    
    nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
                                               kIRDFServiceIID,
                                               (nsISupports**) &gRDFService);
    rv = nsRepository::CreateInstance(kRDFInMemoryDataSourceCID,
                                      nsnull,
                                      kIRDFDataSourceIID,
                                      (void**) &mMiscMailData);    
    gMailDataSource = this;
    AddColumns();
    PR_ASSERT(NS_SUCCEEDED(rv));
}

MailDataSource::~MailDataSource (void) {
    PL_strfree(mURI);
    if (mObservers) {
        for (PRInt32 i = mObservers->Count(); i >= 0; --i) {
            nsIRDFObserver* obs = (nsIRDFObserver*) mObservers->ElementAt(i);
            NS_RELEASE(obs);
        }
        delete mObservers;
    }
    NS_RELEASE(mResourceChild);
    NS_RELEASE(mResourceFolder);
    NS_RELEASE(mResourceFrom);
    NS_RELEASE(mResourceSubject);
    NS_RELEASE(mResourceDate);
    NS_RELEASE(mResourceUser);
    NS_RELEASE(mResourceHost);
    NS_RELEASE(mResourceAccount);
    NS_RELEASE(mResourceName);
    NS_RELEASE(mMailRoot);
    NS_RELEASE(mResourceColumns);
    
    gMailDataSource = nsnull;
    
    nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
    gRDFService = nsnull;
}
    
NS_IMETHODIMP MailDataSource::Init(const char* uri) {
    if ((mURI = PL_strdup(uri)) == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    gRDFService->GetResource(kURINC_child, &mResourceChild);
    gRDFService->GetResource(kURINC_Folder, &mResourceFolder);  
    gRDFService->GetResource(kURINC_from, &mResourceFrom);
    gRDFService->GetResource(kURINC_subject, &mResourceSubject);
    gRDFService->GetResource(kURINC_date, &mResourceDate);
    gRDFService->GetResource(kURINC_user, &mResourceUser);
    gRDFService->GetResource(kURINC_host, &mResourceHost);
    gRDFService->GetResource(kURINC_account, &mResourceAccount);
    gRDFService->GetResource(kURINC_Name, &mResourceName);
    gRDFService->GetResource(kURINC_Columns, &mResourceColumns);
    gRDFService->GetResource("MailRoot", &mMailRoot);
    
    InitAccountList();
    return NS_OK;
}

NS_IMETHODIMP MailDataSource::GetURI(const char* *uri) const {
    *uri = mURI;
    return NS_OK;
}

PRBool MailDataSource::peq (nsIRDFResource* r1, nsIRDFResource* r2) {
    PRBool result;
    if ((NS_OK == r1->EqualsResource(r2, &result)) && result) {
        return 1;
    } else return 0;
}

NS_IMETHODIMP MailDataSource::GetSource(nsIRDFResource* property, nsIRDFNode* target,
                                         PRBool tv,  nsIRDFResource** source /* out */) {
    nsIRDFMailMessage* msg ;
    nsIRDFMailFolder*  fl;
    nsresult rv = NS_ERROR_RDF_NO_VALUE;
    if (!tv) return rv;
    if (NS_OK == target->QueryInterface(kIRDFMailMessageIID, (void**)&msg)) {
        if (peq(mResourceChild, property)) {
            rv = msg->GetFolder((nsIRDFMailFolder**)source);
        }
        NS_RELEASE(msg);
        return rv;
    } else  if (NS_OK == target->QueryInterface(kIRDFMailFolderIID, (void**)&fl)) {
        if (peq(mResourceAccount, property)) {
            rv = fl->GetAccount((nsIRDFMailAccount**)source);
        }
        NS_RELEASE(fl);
        return rv;
    } else return rv;
}


NS_IMETHODIMP MailDataSource::GetTarget(nsIRDFResource* source,
                                        nsIRDFResource* property,   PRBool tv,  nsIRDFNode** target /* out */) {
    nsIRDFMailMessage* msg ;
    nsIRDFMailFolder*  fl;
    nsresult rv = NS_ERROR_RDF_NO_VALUE;
    if (!tv) return rv;
    if (NS_OK == source->QueryInterface(kIRDFMailMessageIID, (void**)&msg)) {
        // XXX maybe something here to make sure that the folder corresponding to the message
        // has been initialized?        
        if (peq(mResourceFrom, property)) {
            rv = msg->GetSender((nsIRDFResource**)target);
        }
        else if (peq(mResourceSubject, property)) {
            rv = msg->GetSubject((nsIRDFLiteral**)target);
        }
        else if (peq(mResourceDate, property)) {
            rv = msg->GetDate((nsIRDFLiteral**)target);
        }
        NS_RELEASE(msg);
        return rv;    
    } else {
        rv = gMailDataSource->mMiscMailData->GetTarget(source, property, tv, target);
       
        return rv;
    }
}


NS_IMETHODIMP MailDataSource::Assert(nsIRDFResource* source, nsIRDFResource* property, 
                  nsIRDFNode* target,  PRBool tv) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailDataSource::Unassert(nsIRDFResource* source,
                                       nsIRDFResource* property, nsIRDFNode* target) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailDataSource::HasAssertion(nsIRDFResource* source,
                                           nsIRDFResource* property, nsIRDFNode* target,
                                           PRBool tv,    PRBool* hasAssertion /* out */) {
    return mMiscMailData->HasAssertion(source, property, target, tv, hasAssertion);
}

NS_IMETHODIMP MailDataSource::AddObserver(nsIRDFObserver* n) {
    if (! mObservers) {
        if ((mObservers = new nsVoidArray()) == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    mObservers->AppendElement(n);
    return NS_OK;
}

NS_IMETHODIMP MailDataSource::RemoveObserver(nsIRDFObserver* n) {
    if (! mObservers)
        return NS_OK;
    mObservers->RemoveElement(n);
    return NS_OK;
}

NS_IMETHODIMP MailDataSource::ArcLabelsIn(nsIRDFNode* node,
                                          nsIRDFArcsInCursor** labels /* out */) {
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP MailDataSource::Flush() {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MailDataSource::GetSources(nsIRDFResource* property,
                           nsIRDFNode* target, PRBool tv,
                           nsIRDFAssertionCursor** sources /* out */) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

               
NS_IMETHODIMP 
MailDataSource::GetTargets(nsIRDFResource* source,
                           nsIRDFResource* property,
                           PRBool tv,   nsIRDFAssertionCursor** targets /* out */) {
    nsIRDFMailMessage* msg;
    nsIRDFMailFolder*  fl;
    nsVoidArray*       array = 0;
    nsresult rv = NS_ERROR_FAILURE;
    if (peq(mResourceChild, property) && 
        (NS_OK == source->QueryInterface(kIRDFMailFolderIID, (void**)&fl))) {
        rv = fl->GetMessageList(&array);
        *targets = new ArrayMailCursor(source, property, array);
        NS_ADDREF(*targets);    
        NS_IF_RELEASE(fl);
        return rv;
    } else if ((peq(mResourceDate, property) ||
                peq(mResourceFrom, property) ||
                peq(mResourceSubject, property)) &&
               (NS_OK == source->QueryInterface(kIRDFMailMessageIID, (void**)&msg))) { 
        *targets = new SingletonMailCursor(source, property, 0);
        NS_IF_RELEASE(msg);
        return rv;
    } else {
        return mMiscMailData->GetTargets(source, property, tv, targets);
    }
}

NS_IMETHODIMP 
MailDataSource::ArcLabelsOut(nsIRDFResource* source,
                             nsIRDFArcsOutCursor** labels /* out */) {
    nsVoidArray* temp = new nsVoidArray();        
    temp->AppendElement(mResourceChild);        
    temp->AppendElement(mResourceFolder);        
    temp->AppendElement(mResourceName);        
    temp->AppendElement(mResourceSubject);        
    temp->AppendElement(mResourceFrom);        
    temp->AppendElement(mResourceDate);        
    temp->AppendElement(mResourceColumns);        
    *labels = new ArrayMailCursor(source, mResourceChild, temp);
    return NS_OK;
}

nsresult
MailDataSource::InitAccountList (void) {
    char*  fileurl = (char*) getMem(100);
    int32 n = PR_SKIP_BOTH;
    PRDirEntry	*de;
    PRDir* dir ;
    nsIRDFMailAccount* account;
    sprintf(fileurl, "Mail");
    dir =  PR_OpenDir(fileurl);
    if (dir == NULL) {
        //if (CallPRMkDirUsingFileURL(fileurl, 00700) > -1) dir = OpenDir(fileurl);
    }
    while ((dir != NULL) && ((de = PR_ReadDir(dir, (PRDirFlags)(n++))) != NULL)) {
        if (strchr(de->name, '@')) {              
            sprintf(fileurl, "mailaccount:%s", de->name);
            gRDFService->GetResource(fileurl, (nsIRDFResource**)&account);
            rdf_Assert(gRDFService, mMiscMailData, account, mResourceName, de->name);
            rdf_Assert(gRDFService, mMiscMailData, account, mResourceSubject, de->name);
            AddAccount(account);
        }
    }
    free(fileurl);
    if (dir) PR_CloseDir(dir);
    return NS_OK;
}


nsresult
MailDataSource::AddColumns(void)
{
    // XXX this is unsavory. I really don't like hard-coding the
    // columns that should be displayed here. What we should do is
    // merge in a "style" graph that contains just a wee bit of
    // information about columns, etc.
    nsresult rv;

    nsIRDFResource* columns = nsnull;

    static const char* gColumnTitles[] = {
        "subject", 
        "date",
        "from",
        nsnull
    };

    static const char* gColumnURIs[] = {
        kURINC_subject,
        kURINC_date,
        kURINC_from,
        nsnull
    };

    const char* const* columnTitle = gColumnTitles;
    const char* const* columnURI   = gColumnURIs;

    if (NS_FAILED(rv = rdf_CreateAnonymousResource(gRDFService, &columns)))
        goto done;

    if (NS_FAILED(rv = rdf_MakeSeq(gRDFService, mMiscMailData, columns)))
        goto done;

    while (*columnTitle && *columnURI) {
        nsIRDFResource* column = nsnull;

        if (NS_SUCCEEDED(rv = rdf_CreateAnonymousResource(gRDFService, &column))) {
            rdf_Assert(gRDFService, mMiscMailData, column, kURINC_Title,  *columnTitle);
            rdf_Assert(gRDFService, mMiscMailData, column, kURINC_Column, *columnURI);

            rdf_ContainerAddElement(gRDFService, mMiscMailData, columns, column);
            NS_IF_RELEASE(column);
        }

        ++columnTitle;
        ++columnURI; 

        if (NS_FAILED(rv))
            break;
    }

    rdf_Assert(gRDFService, mMiscMailData, kMailRoot, kURINC_Columns, columns);

done:
    NS_IF_RELEASE(columns);
    return rv;
}

/********************************** MailAccount **************************************
 ************************************************************************************/



NS_IMETHODIMP MailAccount::GetUser(nsIRDFLiteral**  result) const {
    return gMailDataSource->mMiscMailData->GetTarget((nsIRDFResource*)this, 
                                      gMailDataSource->mResourceUser, 1,
                                      (nsIRDFNode**)result);
}

NS_IMETHODIMP MailAccount::GetName(nsIRDFLiteral**  result) const {
    // NS_ADDREF(mName);
    return gMailDataSource->mMiscMailData->GetTarget((nsIRDFResource*)this, 
                                      gMailDataSource->mResourceName, 1,
                                      (nsIRDFNode**)result);
}


NS_IMETHODIMP MailAccount::GetHost(nsIRDFLiteral**  result) const {
    return gMailDataSource->mMiscMailData->GetTarget((nsIRDFResource*)this, 
                                      gMailDataSource->mResourceHost, 1,
                                      (nsIRDFNode**)result);
}

 
NS_IMETHODIMP MailAccount::AddFolder (nsIRDFMailFolder* folder) {
    return gMailDataSource->mMiscMailData->Assert((nsIRDFResource*)this, 
                                                  gMailDataSource->mResourceFolder,
                                                  folder, 1);
}

NS_IMETHODIMP MailAccount::RemoveFolder (nsIRDFMailFolder* folder) {
    return gMailDataSource->mMiscMailData->Unassert((nsIRDFResource*)this, 
                                                    gMailDataSource->mResourceFolder,
                                                    folder);
}

MailAccount::MailAccount (const char* uri) {
    mURI = PL_strdup(uri);
    InitMailAccount(uri);
}
        

MailAccount::~MailAccount (void) {    
    // Need to uncache before freeing mURI, due to the
    // implementation of the RDF service.
    gRDFService->UnCacheResource(this);
    PL_strfree(mURI);
}


NS_IMPL_ADDREF(MailAccount);
NS_IMPL_RELEASE(MailAccount);


FILE *
openFileWR (char* path)
{
	FILE* ans = fopen(path, "r+");
	if (!ans) {
		ans = fopen(path, "w");
		if (ans) fclose(ans);
		ans = fopen(path, "r+");
	}
	return ans;
}


PRBool
startsWith (const char* pattern, const char* uuid)
{
  size_t l1 = strlen(pattern);
  size_t l2 = strlen(uuid);
  if (l2 < l1) return 0;
  return (strncmp(pattern, uuid, l1)  == 0);
}


PRBool
endsWith (const char* pattern, const char* uuid)
{
  short l1 = strlen(pattern);
  short l2 = strlen(uuid);
  short index;
  
  if (l2 < l1) return false;
  
  for (index = 1; index <= l1; index++) {
    if (toupper(pattern[l1-index]) != toupper(uuid[l2-index])) return false;
  }
  
  return true;
}

void
convertSlashes (char* str) {
	size_t len = strlen(str);
	size_t n = 0;
	while (n < len) {
		if (str[n] == '/') str[n] = '\\';
        n++;
	}
}


NS_IMETHODIMP
MailAccount::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;
    
    *result = nsnull;
    if (iid.Equals(kIRDFResourceIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kIRDFMailAccountIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFMailAccount*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


nsresult
MailAccount::InitMailAccount (const char* url) {
    char*  fileurl = (char*) getMem(100);
    int32 n = PR_SKIP_BOTH;
    PRDirEntry	*de;
    PRDir* dir ;
    nsIRDFMailFolder* folder;
    sprintf(fileurl, "Mail\\%s",  &url[12]);
    dir =  PR_OpenDir(fileurl);
    if (dir == NULL) {
        //if (CallPRMkDirUsingFileURL(fileurl, 00700) > -1) dir = OpenDir(fileurl);
    }
    while ((dir != NULL) && ((de = PR_ReadDir(dir, (PRDirFlags)(n++))) != NULL)) {
        if ((!endsWith(".ssf", de->name)) && (!endsWith(".dat", de->name)) && 
            (!endsWith(".snm", de->name)) && (!endsWith("~", de->name))) {              
            sprintf(fileurl, "mailbox://%s/%s/", &url[12], de->name);
            gRDFService->GetResource(fileurl, (nsIRDFResource**)&folder);
            rdf_Assert(gRDFService, gMailDataSource->mMiscMailData, folder, 
                       gMailDataSource->mResourceName, de->name);
            rdf_Assert(gRDFService, gMailDataSource->mMiscMailData, folder, 
                       gMailDataSource->mResourceSubject, de->name);
            AddFolder(folder);
        }
    }
    free(fileurl);
    if (dir) PR_CloseDir(dir);
    return NS_OK;
}




/********************************** MailFolder **************************************
 ************************************************************************************/

NS_IMETHODIMP MailFolder::GetAccount(nsIRDFMailAccount** account) {
    return gMailDataSource->mMiscMailData->GetTarget((nsIRDFResource*)this, 
                                                    gMailDataSource->mResourceAccount, 1,
                                                    (nsIRDFNode**)account);
}

NS_IMETHODIMP MailFolder::GetName(nsIRDFLiteral**  result) const {
    return gMailDataSource->mMiscMailData->GetTarget((nsIRDFResource*)this, 
                                      gMailDataSource->mResourceName, 1,
                                      (nsIRDFNode**)result);
}
    
NS_IMETHODIMP MailFolder::GetMessageList (nsVoidArray** result) {
    if (mStatus == UNINITIALIZED) {
        mStatus = OK;
        ReadSummaryFile(mURI);
    }
    *result = &mMessages;
    return NS_OK;
}

NS_IMETHODIMP MailFolder::AddMessage (nsIRDFMailMessage* msg) {
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailFolder::RemoveMessage (nsIRDFMailMessage* msg) {
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

MailFolder::MailFolder (const char* uri) {
    mURI = PL_strdup(uri);
    mStatus = UNINITIALIZED;
    NS_INIT_REFCNT();
}
    
MailFolder::~MailFolder (void) {
    gRDFService->UnCacheResource(this);
    PL_strfree(mURI);
}


NS_IMPL_ADDREF(MailFolder);
NS_IMPL_RELEASE(MailFolder);

NS_IMETHODIMP
MailFolder::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;
    
    *result = nsnull;
    if (iid.Equals(kIRDFResourceIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kIRDFMailFolderIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFMailFolder*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


nsresult 
MailFolder::AddMessage(PRUnichar* uri, MailFolder* folder,
                       nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                       int summaryFileOffset, int mailFileOffset, char* flags, 
                       nsIRDFLiteral* messageID) {
    MailMessage* msg;
    gRDFService->GetUnicodeResource(uri, (nsIRDFResource**)&msg);
    msg->SetupMessage(folder, from, subject, date, summaryFileOffset, mailFileOffset, flags, 
                      messageID);
    mMessages.AppendElement(msg);
    return NS_OK;
}


void
GetCValue (nsIRDFLiteral *node, char** result) {
    const PRUnichar* str ;
    node->GetValue(&str);
    nsAutoString pstr(str);
    *result = pstr.ToNewCString();
    
}

nsresult 
MailFolder::AddMessage(PRUnichar* uri, MailFolder* folder,
                       nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                       int mailFileOffset, char* flags, nsIRDFLiteral* messageID) {
    MailMessage* msg;
	char buff[1000];
    int summaryFileOffset;
    char  *xfrom, *xsubject, *xdate;
    gRDFService->GetUnicodeResource(uri, (nsIRDFResource**)&msg);
    if (!flags) flags = "0000";
    fseek(mSummaryFile, 0L, SEEK_END);
    summaryFileOffset = ftell(mSummaryFile);
    
    from->GetValue((const char**) &xfrom);

    GetCValue(date, &xdate);
    GetCValue(subject, &xsubject);
    
    sprintf(buff,  
            "Status: %s\nSOffset: %d\nFrom: %s\nSubject: %s\nDate: %s\nMOffset: %d\n", 
            flags, summaryFileOffset, xfrom, xsubject, xdate, mailFileOffset); 
    fprintf(mSummaryFile, buff);
    delete(xsubject);
    delete(xdate);
     fflush(mSummaryFile);
    msg->SetupMessage(folder, from, subject, date, summaryFileOffset, mailFileOffset, flags, 
                      messageID);
    mMessages.AppendElement(msg);
    return NS_OK;
}



nsresult
MailFolder::ReadSummaryFile (char* url)
{
  if (startsWith("mailbox://", url)) {
    char* folderURL = &url[10];	
	int32 flen = strlen("Mail") + strlen(folderURL) + 4;
    char* fileurl = (char*) getMem(flen);
    char* nurl = (char*) getMem(strlen(url) + 20);
    char* buff = (char*) getMem(BUFF_SIZE);
    FILE *mf;
    int   summOffset, messageOffset;
    nsIRDFLiteral *rSubject, *rDate;
    nsIRDFResource * rFrom;
    PRBool summaryFileFound = 0;
    char* flags = 0;
     
    sprintf(fileurl, "Mail\\%sssf",  folderURL);
	fileurl[strlen(fileurl)-4] = '.'; //XXX how do you spell kludge?
	convertSlashes(fileurl);
    // fileurl = MCDepFileURL(fileurl);
    mSummaryFile = openFileWR(fileurl);
    sprintf(fileurl, "Mail\\%s",  folderURL);
    //	fileurl = MCDepFileURL(fileurl);
    mf = openFileWR(fileurl);


    while (0 && mSummaryFile && fgets(buff, BUFF_SIZE, mSummaryFile)) {
      if (startsWith("Status:", buff)) {
          summaryFileFound = 1;
          flags = PL_strdup(&buff[8]);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          sscanf(&buff[9], "%d", &summOffset);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          nsAutoString pfrom(&buff[6]);
          gRDFService->GetUnicodeResource(pfrom, &rFrom);
           
          fgets(buff, BUFF_SIZE, mSummaryFile);
          nsAutoString psubject(&buff[8]);
          gRDFService->GetLiteral(psubject, &rSubject);
           
          fgets(buff, BUFF_SIZE, mSummaryFile);
          nsAutoString pdate(&buff[6]);
          gRDFService->GetLiteral(pdate, &rDate);
          
          fgets(buff, BUFF_SIZE, mSummaryFile);
          sscanf(&buff[9], "%d", &messageOffset);
          sprintf(nurl, "%s?%d", url, messageOffset);
          nsAutoString purl(nurl);
          AddMessage(purl, this, rFrom, rSubject, rDate, summOffset,
                     messageOffset, flags, 0);
          PL_strfree(flags);
      }
    }

    rFrom = nsnull;
    if (!summaryFileFound) {
      /* either a new mailbox or need to read BMF to recreate */
      while (mf && fgets(buff, BUFF_SIZE, mf)) {
        if (strncmp("From ", buff, 5) ==0)  { 
          if (rFrom) {
              nsAutoString purl(nurl);
              AddMessage(purl, this, rFrom, rSubject, rDate, messageOffset, flags, 0);
             
          }
          messageOffset = ftell(mf);
          if (flags) PL_strfree(flags);
          sprintf(nurl, "%s?%i", url, messageOffset);
          flags = nsnull;
          rFrom = nsnull;
          rSubject = rDate = nsnull;
        }
		buff[strlen(buff)-1] = '\0';
        if ((!rFrom) && (startsWith("From:", buff))) {
            nsAutoString pfrom(&buff[6]);
            gRDFService->GetUnicodeResource(pfrom, &rFrom);
             
        } else if ((!rDate) && (startsWith("Date:", buff))) {
            nsAutoString pdate(&buff[6]);
            gRDFService->GetLiteral(pdate, &rDate);
        } else if ((!rSubject) && (startsWith("Subject:", buff))) {
            nsAutoString psubject(&buff[8]);
            gRDFService->GetLiteral(psubject, &rSubject);
            
        } else if ((!flags) && (startsWith("X-Mozilla-Status:", buff))) {
          flags = PL_strdup(&buff[17]);
        }        
      }
     if (rFrom){
         nsAutoString purl(nurl);
         AddMessage(purl, this, rFrom, rSubject, rDate, messageOffset, flags, 0);
     }
      fflush(mSummaryFile);
    }
   
    free(buff);
    free(nurl);
  }
  return NS_OK;
}

/********************************** MailMessage **************************************
 ************************************************************************************/
NS_IMETHODIMP MailMessage::GetFolder(nsIRDFMailFolder**  result) {
    *result = mFolder;
    return NS_OK;
}

        
NS_IMETHODIMP MailMessage::GetSubject(nsIRDFLiteral**  result) {
    NS_ADDREF(mSubject);
    *result = mSubject;
    return NS_OK;
}

NS_IMETHODIMP MailMessage::GetSender(nsIRDFResource**  result) {
    NS_ADDREF(mFrom);
    *result = mFrom;
    return NS_OK;
}
        
NS_IMETHODIMP MailMessage::GetDate(nsIRDFLiteral**  result) {
    NS_ADDREF(mDate);
    *result = mDate;
    return NS_OK;
}

NS_IMETHODIMP MailMessage::GetContent(char** result) {
    PR_ASSERT(0);
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailMessage::GetMessageID(nsIRDFLiteral** id) {
    NS_ADDREF(mMessageID);
    *id = mMessageID;
    return NS_OK;
}

NS_IMETHODIMP MailMessage::GetFlags(char** result) {
    *result = mFlags;
    return NS_OK;
}
        
NS_IMETHODIMP MailMessage::SetFlags(const char* result) {
    memcpy(mFlags, result, 4);
    //xxx write this into the summary file
    return NS_OK;
}

nsresult MailMessage::SetupMessage (MailFolder* folder,
                                    nsIRDFResource* from, nsIRDFLiteral* subject, 
                                    nsIRDFLiteral* date,
                                    int summaryFileOffset, int mailFileOffset, 
                                    char* flags, nsIRDFLiteral* messageID) 
{
    NS_INIT_REFCNT();
    mFolder = folder;
    mFrom = from;
    mSubject = subject;
    mDate    = date;
    mSummaryFileOffset = summaryFileOffset;
    mMailFileOffset    = mailFileOffset;
    memcpy(mFlags, flags, 4);
    mMessageID = messageID;
    
    NS_IF_ADDREF(mFrom);
    NS_IF_ADDREF(mSubject);
    NS_IF_ADDREF(mDate);
    return NS_OK;
}

MailMessage::MailMessage (const char* uri)        
{
    mURI = PL_strdup(uri);
    NS_INIT_REFCNT();
}

MailMessage::~MailMessage (void) {
    gRDFService->UnCacheResource(this);
    PL_strfree(mURI);
    NS_IF_RELEASE(mFrom);
    NS_IF_RELEASE(mSubject);
    NS_IF_RELEASE(mDate);
    NS_IF_RELEASE(mMessageID);
}

NS_IMPL_ADDREF(MailMessage);
NS_IMPL_RELEASE(MailMessage);

NS_IMETHODIMP
MailMessage::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;
    
    *result = nsnull;
    if (iid.Equals(kIRDFResourceIID) ||
        iid.Equals(kIRDFNodeIID) ||
        iid.Equals(kIRDFMailMessageIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFMailMessage*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


/********************************** nsIRDFPerson **************************************
 * nsIRDFPerson is not really a person. It corresponds to someone you can recieve
 * mail from.
 ************************************************************************************/

// XXX --- needs to be done.

/********************************** Cursors **************************************
 * There are two kinds of cursors for the mail data source.
 * --- those that cough up a singleton value (sender, subject, etc.)
 * --- those that cough up a sequence of values from an nsVoidArray (children of folders, folders
 *     in an account
 ************************************************************************************/

SingletonMailCursor::SingletonMailCursor(nsIRDFNode* u,
                                         nsIRDFResource* s,
                                         PRBool inversep) {
    if (!inversep) {
        mSource = (nsIRDFResource*)u;
        mTarget = nsnull;
    } else {
        mSource = nsnull;
        mTarget = u;
    }
    NS_ADDREF(u);
    mProperty = s;
    NS_ADDREF(mProperty);
    mValueReturnedp = 0;
    mInversep = inversep;
    mValue = nsnull;
}
            
    
SingletonMailCursor::~SingletonMailCursor(void) {
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mValue);
    NS_IF_RELEASE(mProperty);
    NS_IF_RELEASE(mTarget);
}
        

   
    // nsIRDFCursor interface
NS_IMETHODIMP SingletonMailCursor::Advance(void) {
    nsresult rv = NS_ERROR_RDF_CURSOR_EMPTY;
    if (mValueReturnedp) {
        NS_IF_RELEASE(mValue);
        mValue = nsnull;
        return NS_ERROR_RDF_CURSOR_EMPTY;
    }
    mValueReturnedp = 1;
    if (mInversep) {
        rv = gMailDataSource->GetSource(mProperty, mTarget, 1, (nsIRDFResource**)&mValue);
        mSource = (nsIRDFResource*)mValue;
    } else {
        rv = gMailDataSource->GetTarget(mSource, mProperty,  1, &mValue);
        mTarget = mValue;
    }
    if (mValue) {
        NS_ADDREF(mValue);
        NS_ADDREF(mValue); 
    }
    // yes, its required twice, one for the value and one for the source/target
    if (rv == NS_ERROR_RDF_NO_VALUE) return NS_ERROR_RDF_CURSOR_EMPTY;
    return rv;
}

NS_IMETHODIMP SingletonMailCursor::GetValue(nsIRDFNode** aValue) {
    NS_ADDREF(mValue);
    *aValue = mValue;
    return NS_OK;
}

    // nsIRDFAssertionCursor interface
NS_IMETHODIMP SingletonMailCursor::GetDataSource(nsIRDFDataSource** aDataSource) {
    NS_ADDREF(gMailDataSource);
    *aDataSource = gMailDataSource;
    return NS_OK;
}

NS_IMETHODIMP SingletonMailCursor::GetSubject(nsIRDFResource** aResource) {
    NS_ADDREF(mSource);
    *aResource = mSource;
    return NS_OK;
}

NS_IMETHODIMP SingletonMailCursor::GetPredicate(nsIRDFResource** aPredicate) {
    NS_ADDREF(mProperty);
    *aPredicate = mProperty;
    return NS_OK;
}

NS_IMETHODIMP SingletonMailCursor::GetObject(nsIRDFNode** aObject) {
    NS_ADDREF(mTarget);
    *aObject = mTarget;
    return NS_OK;
}

NS_IMETHODIMP SingletonMailCursor::GetTruthValue(PRBool* aTruthValue) {
    *aTruthValue = 1;
    return NS_OK;
}


NS_IMPL_ISUPPORTS(SingletonMailCursor, kIRDFAssertionCursorIID);

ArrayMailCursor::ArrayMailCursor(nsIRDFResource* u, nsIRDFResource* s, nsVoidArray* array) {
    //  getsources and gettargets will call this with the array
    mSource = u;
    mProperty = s;
    mArray = array;
    NS_ADDREF(mProperty);
    NS_ADDREF(u);
    mCount = 0;
    mTarget = nsnull;
    mValue = nsnull;
}
                
ArrayMailCursor::~ArrayMailCursor(void) {
    NS_IF_RELEASE(mSource);
    NS_IF_RELEASE(mValue);
    NS_IF_RELEASE(mProperty);
    NS_IF_RELEASE(mTarget);
}
        
   
    // nsIRDFCursor interface
NS_IMETHODIMP ArrayMailCursor::Advance(void) {
    if (mArray->Count() <= mCount) return  NS_ERROR_RDF_CURSOR_EMPTY;
    NS_IF_RELEASE(mValue);
    mTarget = mValue = (nsIRDFNode*) mArray->ElementAt(mCount++);
    NS_ADDREF(mValue);
    return NS_OK;
}
        
NS_IMETHODIMP ArrayMailCursor::GetValue(nsIRDFNode** aValue) {
    NS_ADDREF(mValue);
    *aValue = mValue;
    return NS_OK;
}

    // nsIRDFAssertionCursor interface
NS_IMETHODIMP ArrayMailCursor::GetDataSource(nsIRDFDataSource** aDataSource) {
    NS_ADDREF(gMailDataSource);
    *aDataSource = gMailDataSource;
    return NS_OK;
}

NS_IMETHODIMP ArrayMailCursor::GetSubject(nsIRDFResource** aResource) {
    NS_ADDREF(mSource);
    *aResource = mSource;
    return NS_OK;
}

NS_IMETHODIMP ArrayMailCursor::GetPredicate(nsIRDFResource** aPredicate) {
    NS_ADDREF(mProperty);
    *aPredicate = mProperty;
    return NS_OK;
}

NS_IMETHODIMP ArrayMailCursor::GetObject(nsIRDFNode** aObject) {
    NS_ADDREF(mTarget);
    *aObject = mTarget;
    return NS_OK;
}

NS_IMETHODIMP ArrayMailCursor::GetTruthValue(PRBool* aTruthValue) {
    *aTruthValue = 1;
    return NS_OK;
}

NS_IMPL_ADDREF(ArrayMailCursor);
NS_IMPL_RELEASE(ArrayMailCursor);

NS_IMETHODIMP
ArrayMailCursor::QueryInterface(REFNSIID iid, void** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;
    
    *result = nsnull;
    if (iid.Equals(kIRDFAssertionCursorIID) ||
        iid.Equals(kIRDFCursorIID) ||
        iid.Equals(kIRDFArcsOutCursorIID) ||
        iid.Equals(kISupportsIID)) {
        *result = NS_STATIC_CAST(nsIRDFAssertionCursor*, this);
        AddRef();
        return NS_OK;
    }
    return NS_NOINTERFACE;
}


////////////////////////////////////////////////////////////////////////

/**
 * Return the singleton mail data source, constructing it if necessary.
 */
nsresult
NS_NewRDFMailDataSource(nsIRDFDataSource** result)
{
    if (! result)
        return NS_ERROR_NULL_POINTER;

    // Only one mail data source
    if (! gMailDataSource) {
        if ((gMailDataSource = new MailDataSource()) == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(gMailDataSource);
    *result = gMailDataSource;
    return NS_OK;
}


////////////////////////////////////////////////////////////////////////

/**
 * This class creates resources for account URIs. It should be
 * registered for the "mailaccount:" prefix.
 */
class MailAccountResourceFactoryImpl : public nsIRDFResourceFactory
{
public:
    MailAccountResourceFactoryImpl(void);
    virtual ~MailAccountResourceFactoryImpl(void);

    NS_DECL_ISUPPORTS

    NS_IMETHOD CreateResource(const char* aURI, nsIRDFResource** aResult);
};

MailAccountResourceFactoryImpl::MailAccountResourceFactoryImpl(void)
{
    NS_INIT_REFCNT();
}

MailAccountResourceFactoryImpl::~MailAccountResourceFactoryImpl(void)
{
}

NS_IMPL_ISUPPORTS(MailAccountResourceFactoryImpl, kIRDFResourceFactoryIID);

NS_IMETHODIMP
MailAccountResourceFactoryImpl::CreateResource(const char* aURI, nsIRDFResource** aResult)
{
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    MailAccount* account = new MailAccount(aURI);
    if (! account)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(account);
    *aResult = account;
    return NS_OK;
}

nsresult
NS_NewRDFMailAccountResourceFactory(nsIRDFResourceFactory** aResult)
{
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    MailAccountResourceFactoryImpl* factory =
        new MailAccountResourceFactoryImpl();

    if (! factory)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(factory);
    *aResult = factory;
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////

/**
 * This class creates resources for folder and message URIs. It should
 * be registered for the "mailbox:" prefix.
 */
class MailResourceFactoryImpl : public nsIRDFResourceFactory
{
public:
    MailResourceFactoryImpl(void);
    virtual ~MailResourceFactoryImpl(void);

    NS_DECL_ISUPPORTS

    NS_IMETHOD CreateResource(const char* aURI, nsIRDFResource** aResult);
};

MailResourceFactoryImpl::MailResourceFactoryImpl(void)
{
    NS_INIT_REFCNT();
}

MailResourceFactoryImpl::~MailResourceFactoryImpl(void)
{
}

NS_IMPL_ISUPPORTS(MailResourceFactoryImpl, kIRDFResourceFactoryIID);

NS_IMETHODIMP
MailResourceFactoryImpl::CreateResource(const char* aURI, nsIRDFResource** aResult)
{
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    nsIRDFResource* resource;

    if (aURI[PL_strlen(aURI) - 1] == '/') {
        resource = new MailFolder(aURI);
    } else {
        resource = new MailMessage(aURI);
    }

    if (! resource)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(resource);
    *aResult = resource;
    return NS_OK;
}

nsresult
NS_NewRDFMailResourceFactory(nsIRDFResourceFactory** aResult)
{
    if (! aResult)
        return NS_ERROR_NULL_POINTER;

    MailResourceFactoryImpl* factory =
        new MailResourceFactoryImpl();

    if (! factory)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(factory);
    *aResult = factory;
    return NS_OK;
}

