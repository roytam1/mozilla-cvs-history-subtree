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

  1) Need to keep track of all the messages from a person. I don't
     want to create nsIRDFPerson. Maybe we can just keep a hash table
     of vectors? Or maybe nsIRDFPerson is inevitable.

  2) need factories for all these interfaces. Need hooks into
     GetResource for some of them.

  3) I am missing about 60% of the add and release refs.
  
 */

#include "nscore.h"
#include "nsIRDFCursor.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFResourceFactory.h"
#include "nsIServiceManager.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFMail.h"
#include "nsIRDFService.h"
#include "plhash.h"
#include "plstr.h"
#include "stdio.h"
#include "prmem.h"
#include "prio.h"

static NS_DEFINE_IID(kIRDFArcsInCursorIID,     NS_IRDFARCSINCURSOR_IID);
static NS_DEFINE_IID(kIRDFArcsOutCursorIID,    NS_IRDFARCSOUTCURSOR_IID);
static NS_DEFINE_IID(kIRDFAssertionCursorIID,  NS_IRDFASSERTIONCURSOR_IID);
static NS_DEFINE_IID(kIRDFCursorIID,           NS_IRDFCURSOR_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,       NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,          NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFMailAccountIID,      NS_IMAILACCOUNT_IID);
static NS_DEFINE_IID(kIRDFMailDataSourceIID,   NS_IRDFMAILDATAOURCE_IID);
static NS_DEFINE_IID(kIRDFMailFolderIID,       NS_IMAILFOLDER_IID);
static NS_DEFINE_IID(kIRDFMailMessageIID,      NS_IMAILMESSAGE_IID);
static NS_DEFINE_IID(kIRDFNodeIID,             NS_IRDFNODE_IID);
static NS_DEFINE_IID(kIRDFResourceIID,         NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFResourceFactoryIID,  NS_IRDFRESOURCEFACTORY_IID);
static NS_DEFINE_IID(kIRDFServiceIID,          NS_IRDFSERVICE_IID);
static NS_DEFINE_IID(kISupportsIID,            NS_ISUPPORTS_IID);

static NS_DEFINE_CID(kRDFServiceCID,           NS_RDFSERVICE_CID);

// THE mail data source.
static nsIRDFDataSource* gMailDataSource = nsnull;

// The RDF service manager. Cached in the mail data source's
// constructor
static nsIRDFService* gRDFService = nsnull;


#define RDF_RESOURCE_METHODS \
NS_IMETHODIMP \
EqualsNode(nsIRDFNode* node, PRBool* result) const {\
    nsresult rv;\
    nsIRDFResource* resource;\
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFResourceIID, (void**) &resource))) {\
        rv = EqualsResource(resource, result);\
        NS_RELEASE(resource);\
    }\
    else {\
        *result = PR_FALSE;\
        rv = NS_OK;\
    }\
    return rv;\
}\
NS_IMETHODIMP \
GetValue(const char* *uri) const{\
    if (!uri)\
        return NS_ERROR_NULL_POINTER;\
    *uri = mURI;\
    return NS_OK;\
}\
NS_IMETHODIMP \
EqualsResource(const nsIRDFResource* resource, PRBool* result) const {\
    if (!resource || !result)  return NS_ERROR_NULL_POINTER;\
    *result = (resource == (nsIRDFResource*) this);\
    return NS_OK;\
}\
NS_IMETHODIMP \
EqualsString(const char* uri, PRBool* result) const {\
    if (!uri || !result)  return NS_ERROR_NULL_POINTER;\
    *result = (PL_strcmp(uri, mURI) == 0);\
    return NS_OK;\
}

/********************************** MailDataSource **************************************
 ************************************************************************************/

class MailDataSource :  public nsIRDFMailDataSource 
{
private:
    char*       mURI;
    nsVoidArray mAccounts;
    nsVoidArray* mObservers;
    nsIRDFService* mSrv;

    // Resources that we use often XXX : got to initialize these and release them!
    nsIRDFResource* mResourceChild;
    nsIRDFResource* mResourceFrom;
    nsIRDFResource* mResourceSubject;
    nsIRDFResource* mResourceDate;
    nsIRDFResource* mResourceUser;
    nsIRDFResource* mResourceHost;
    nsIRDFResource* mResourceAccount;
    nsIRDFResource* mResourceName;

public:
  
    NS_DECL_ISUPPORTS
    
    NS_IMETHOD GetAccountList (nsVoidArray* result) {
        *result = mAccounts;
        return NS_OK;
    }

    NS_IMETHOD AddAccount (nsIRDFMailAccount* folder) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD RemoveAccount (nsIRDFMailAccount* folder) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    MailDataSource(void) {
        NS_INIT_REFCNT();

        nsresult rv = nsServiceManager::GetService(kRDFServiceCID,
                                                   kIRDFServiceIID,
                                                   (nsISupports**) &gRDFService);

        PR_ASSERT(NS_SUCCEEDED(rv));
    }

    ~MailDataSource (void) {
        PL_strfree(mURI);
        if (mObservers) {
            for (PRInt32 i = mObservers->Count(); i >= 0; --i) {
                nsIRDFObserver* obs = (nsIRDFObserver*) mObservers->ElementAt(i);
                NS_RELEASE(obs);
            }
            delete mObservers;
        }

        gMailDataSource = nsnull;

        nsServiceManager::ReleaseService(kRDFServiceCID, gRDFService);
        gRDFService = nsnull;
    }
    
    NS_IMETHOD Init(const char* uri) {
        if ((mURI = PL_strdup(uri)) == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;

        //XXX get the service manager, initialize the resources (mResourceChild, etc.)
        
        return NS_OK;
    }

    PRBool peq (nsIRDFResource* r1, nsIRDFResource* r2) {
        PRBool result;
        if ((NS_OK == r1->EqualsResource(r2, &result)) && result) {
            return 1;
        } else return 0;
    }

    NS_IMETHOD GetSource(nsIRDFResource* property, nsIRDFNode* target,
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


    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,   PRBool tv,  nsIRDFNode** target /* out */) {
        nsIRDFMailMessage* msg ;
        nsIRDFMailAccount* ac;
        nsIRDFMailFolder*  fl;
        nsresult rv = NS_ERROR_RDF_NO_VALUE;
        if (!tv) return rv;
        if (NS_OK == source->QueryInterface(kIRDFMailMessageIID, (void**)&msg)) {
            // maybe something here to make sure that the folder corresponding to the message
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
        } else if (NS_OK == source->QueryInterface(kIRDFMailFolderIID, (void**)&fl)) {
            if (peq(mResourceName, property)) {
                rv = fl->GetName((nsIRDFLiteral**)target);
            } 
            NS_RELEASE(fl);
            return rv;        
        } else if (NS_OK == source->QueryInterface(kIRDFMailAccountIID, (void**) &ac)) {
            if (peq(mResourceUser, property)) {
                rv = ac->GetUser((nsIRDFLiteral**)target);
            } 
            else if (peq(mResourceHost, property)) {
                rv = ac->GetHost((nsIRDFLiteral**)target);
            }
            NS_RELEASE(ac);
            return rv;
        }
        return rv;
    }

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target, PRBool tv,
                          nsIRDFAssertionCursor** sources /* out */) ;
               
    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,   nsIRDFAssertionCursor** targets /* out */) ;

    NS_IMETHOD Assert(nsIRDFResource* source, nsIRDFResource* property, 
                      nsIRDFNode* target,  PRBool tv) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property, nsIRDFNode* target) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property, nsIRDFNode* target,
                            PRBool tv,    PRBool* hasAssertion /* out */) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD AddObserver(nsIRDFObserver* n) {
        if (! mObservers) {
            if ((mObservers = new nsVoidArray()) == nsnull)
                return NS_ERROR_OUT_OF_MEMORY;
        }
        mObservers->AppendElement(n);
        return NS_OK;
    }

    NS_IMETHOD RemoveObserver(nsIRDFObserver* n) {
        if (! mObservers)
            return NS_OK;
        mObservers->RemoveElement(n);
        return NS_OK;
    }

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsIRDFArcsInCursor** labels /* out */) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsIRDFArcsOutCursor** labels /* out */) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD Flush() {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
};

NS_IMPL_ISUPPORTS(MailDataSource, kIRDFMailDataSourceIID);



/********************************** MailAccount **************************************
 ************************************************************************************/

class MailAccount : public nsIRDFMailAccount 
{
private:
  nsIRDFLiteral*       mUser;
  nsIRDFLiteral*       mHost;
  nsVoidArray          mFolders;
  char*                             mURI ;        

public:
    
    NS_DECL_ISUPPORTS

    RDF_RESOURCE_METHODS

    NS_IMETHOD GetUser(nsIRDFLiteral**  result) const {
        *result = mUser;
        NS_ADDREF(mUser);
        return NS_OK;
    }

    NS_IMETHOD GetHost(nsIRDFLiteral**  result) const {
        *result = mHost;
        NS_ADDREF(mHost);
        return NS_OK;
    }

    NS_IMETHOD GetFolderList (nsVoidArray** result) {
        *result = &mFolders;
        return NS_OK;
    }
 
    NS_IMETHOD AddFolder (nsIRDFMailFolder* folder) {
        mFolders.AppendElement(folder);
        return NS_OK;
    }

    NS_IMETHOD RemoveFolder (nsIRDFMailFolder* folder) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    nsresult InitMailAccount (const char* uri);

    MailAccount (const char* uri) {
        mURI = PL_strdup(uri);
    }
        

    MailAccount (char* uri,  nsIRDFLiteral* user,  nsIRDFLiteral* host) 
    {
        mURI = PL_strdup(uri);
        mUser = user;
        mHost = host;
        NS_INIT_REFCNT();
        NS_IF_ADDREF(mUser);
        NS_IF_ADDREF(mHost);
        InitMailAccount(uri);
    }                        

    virtual ~MailAccount (void) {
        PL_strfree(mURI);
        NS_IF_RELEASE(mUser);
        NS_IF_RELEASE(mHost);
    }

};

NS_IMPL_ADDREF(MailAccount);
NS_IMPL_RELEASE(MailAccount);

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

/********************************** MailFolder **************************************
 ************************************************************************************/

typedef enum {
    UNINITIALIZED,
    INIT_IN_PROGRESS,
    WRITE_IN_PROGRESS,
    OK
} FolderStatus;

class MailFolder : public nsIRDFMailFolder 
{
private:
    nsVoidArray    mMessages;
    FILE*          mSummaryFile;
    FolderStatus   mStatus;
    MailAccount*   mAccount;
    nsIRDFLiteral* mName;
    char*                             mURI ;
public:

    NS_DECL_ISUPPORTS

    RDF_RESOURCE_METHODS

    NS_IMETHOD GetAccount(nsIRDFMailAccount** account) {
        NS_ADDREF(mAccount);
        *account = mAccount;
        return NS_OK;
    }

    NS_IMETHOD GetName(nsIRDFLiteral**  result) const {
        NS_ADDREF(mName);
        *result = mName;
        return NS_OK;
    }
    
    NS_IMETHOD GetMessageList (nsVoidArray** result) {
        *result = &mMessages;
        return NS_OK;
    }

    NS_IMETHOD AddMessage (nsIRDFMailMessage* msg) {
        PR_ASSERT(0);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD RemoveMessage (nsIRDFMailMessage* msg) {
        PR_ASSERT(0);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    MailFolder (const char* uri) {
        mURI = PL_strdup(uri);
        NS_INIT_REFCNT();
    }

    MailFolder (char* uri, nsIRDFLiteral* name, MailAccount* account) 
    {
        mURI = PL_strdup(uri);
        mName = name;
        mAccount = account;
        NS_INIT_REFCNT();
    }                        
    
    virtual ~MailFolder (void) {
        PL_strfree(mURI);
        NS_IF_RELEASE(mName);
        NS_IF_RELEASE(mAccount);
    }

    nsresult MailFolder::ReadSummaryFile (char* url);

    nsresult 
    AddMessage(char* uri, MailFolder* folder,
               nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
               int summaryFileOffset, int mailFileOffset, char* flags, 
               nsIRDFLiteral* messageID) ;

    nsresult AddMessage(char* uri, MailFolder* folder,
                 nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                        int mailFileOffset, char* flags, nsIRDFLiteral* messageID) ;
           
};

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

/********************************** MailMessage **************************************
 ************************************************************************************/

class MailMessage : public nsIRDFMailMessage 
{
private:
    MailFolder*     mFolder;
    nsIRDFResource* mFrom;
    nsIRDFLiteral*  mSubject;
    int             mSummaryFileOffset;
    int             mMailFileOffset;
    nsIRDFLiteral*  mDate;
    nsIRDFLiteral*  mMessageID;
    char            mFlags[4];
    char*                             mURI ;

public:
    NS_DECL_ISUPPORTS

    RDF_RESOURCE_METHODS

    NS_IMETHOD GetFolder(nsIRDFMailFolder**  result) {
        *result = mFolder;
        return NS_OK;
    }

        
    NS_IMETHOD GetSubject(nsIRDFLiteral**  result) {
        NS_ADDREF(mSubject);
        *result = mSubject;
        return NS_OK;
    }

    NS_IMETHOD GetSender(nsIRDFResource**  result) {
        NS_ADDREF(mFrom);
        *result = mFrom;
        return NS_OK;
    }
        
    NS_IMETHOD GetDate(nsIRDFLiteral**  result) {
        NS_ADDREF(mDate);
        *result = mDate;
        return NS_OK;
    }

    NS_IMETHOD GetContent(char** result) {
        PR_ASSERT(0);
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD GetMessageID(nsIRDFLiteral** id) {
        NS_ADDREF(mMessageID);
        *id = mMessageID;
        return NS_OK;
    }

    NS_IMETHOD GetFlags(char** result) {
        *result = mFlags;
        return NS_OK;
    }
        
    NS_IMETHOD SetFlags(const char* result) {
        memcpy(mFlags, result, 4);
        //xxx write this into the summary file
        return NS_OK;
    }

   nsresult SetupMessage (MailFolder* folder,
                 nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                 int summaryFileOffset, int mailFileOffset, char* flags, nsIRDFLiteral* messageID) 
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

    MailMessage (const char* uri)        
    {
        mURI = PL_strdup(uri);
        NS_INIT_REFCNT();
    }

    virtual ~MailMessage (void) {
        PL_strfree(mURI);
        NS_IF_RELEASE(mFrom);
        NS_IF_RELEASE(mSubject);
        NS_IF_RELEASE(mDate);
        NS_IF_RELEASE(mMessageID);
    }
};


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

class SingletonMailCursor : public nsIRDFAssertionCursor 
{
private:
    nsIRDFNode*     mValue;
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    PRBool          mValueReturnedp;
    PRBool          mInversep;

public:
    SingletonMailCursor(nsIRDFNode* u,
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
    }
            
    
    virtual ~SingletonMailCursor(void) {
        NS_IF_RELEASE(mSource);
        NS_IF_RELEASE(mValue);
        NS_IF_RELEASE(mProperty);
        NS_IF_RELEASE(mTarget);
    }
        

    // nsISupports interface
    NS_DECL_ISUPPORTS
   
    // nsIRDFCursor interface
    NS_IMETHOD Advance(void) {
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
        NS_ADDREF(mValue);
        NS_ADDREF(mValue); 
        // yes, its required twice, one for the value and one for the source/target
        return rv;
    }
        
    NS_IMETHOD GetValue(nsIRDFNode** aValue) {
        NS_ADDREF(mValue);
        *aValue = mValue;
        return NS_OK;
    }

    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        NS_ADDREF(gMailDataSource);
        *aDataSource = gMailDataSource;
        return NS_OK;
    }

    NS_IMETHOD GetSubject(nsIRDFResource** aResource) {
        NS_ADDREF(mSource);
        *aResource = mSource;
        return NS_OK;
    }

    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) {
        NS_ADDREF(mProperty);
        *aPredicate = mProperty;
        return NS_OK;
    }

    NS_IMETHOD GetObject(nsIRDFNode** aObject) {
        NS_ADDREF(mTarget);
        *aObject = mTarget;
        return NS_OK;
    }

    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) {
        *aTruthValue = 1;
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(SingletonMailCursor, kIRDFAssertionCursorIID);

class ArrayMailCursor : public nsIRDFAssertionCursor 
{
private:
    nsIRDFNode*     mValue;
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    int             mCount;
    nsVoidArray*    mArray;

public:
    ArrayMailCursor(nsIRDFResource* u, nsIRDFResource* s, nsVoidArray* array) {
        // lets just assume that getsources and gettargets will call this with the array
        mSource = u;
        mProperty = s;
        mArray = array;
        NS_ADDREF(mProperty);
        NS_ADDREF(u);
        mCount = 0;
    }
                
    virtual ~ArrayMailCursor(void) {
        NS_IF_RELEASE(mSource);
        NS_IF_RELEASE(mValue);
        NS_IF_RELEASE(mProperty);
        NS_IF_RELEASE(mTarget);
    }
        
    // nsISupports interface
    NS_DECL_ISUPPORTS
   
    // nsIRDFCursor interface
    NS_IMETHOD Advance(void) {
        if (mArray->Count() <= mCount) return  NS_ERROR_RDF_NO_VALUE;
        NS_IF_RELEASE(mValue);
        mValue = (nsIRDFNode*) mArray->ElementAt(mCount++);
        NS_ADDREF(mValue);
        return NS_OK;
    }
        
    NS_IMETHOD GetValue(nsIRDFNode** aValue) {
        NS_ADDREF(mValue);
        *aValue = mValue;
        return NS_OK;
    }

    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        NS_ADDREF(gMailDataSource);
        *aDataSource = gMailDataSource;
        return NS_OK;
    }

    NS_IMETHOD GetSubject(nsIRDFResource** aResource) {
        NS_ADDREF(mSource);
        *aResource = mSource;
        return NS_OK;
    }

    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) {
        NS_ADDREF(mProperty);
        *aPredicate = mProperty;
        return NS_OK;
    }

    NS_IMETHOD GetObject(nsIRDFNode** aObject) {
        NS_ADDREF(mTarget);
        *aObject = mTarget;
        return NS_OK;
    }

    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) {
        *aTruthValue = 1;
        return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(ArrayMailCursor, kIRDFAssertionCursorIID);


NS_IMETHODIMP
MailDataSource::GetSources(nsIRDFResource* property,
                           nsIRDFNode* target, PRBool tv,
                           nsIRDFAssertionCursor** sources /* out */) {
    *sources = new SingletonMailCursor(target, property, 1);
    return NS_OK;
}

               
NS_IMETHODIMP 
MailDataSource::GetTargets(nsIRDFResource* source,
                           nsIRDFResource* property,
                           PRBool tv,   nsIRDFAssertionCursor** targets /* out */) {
    nsIRDFMailAccount* ac;
    nsIRDFMailFolder*  fl;
    nsVoidArray*       array = 0;
    nsresult rv = NS_ERROR_RDF_CURSOR_EMPTY;
    if (NS_OK == source->QueryInterface(kIRDFMailFolderIID, (void**)&fl)) {
        if (peq(mResourceChild, property)) {
            rv = fl->GetMessageList(&array);
        } 
        NS_IF_RELEASE(fl);
    } else  if (NS_OK == source->QueryInterface(kIRDFMailAccountIID, (void**)&ac)) {
        if (peq(mResourceChild, property)) {
            rv = ac->GetFolderList(&array);
        } 
        NS_IF_RELEASE(ac);
    }
    if (array) {
        *targets = new ArrayMailCursor(source, property, array);
        NS_ADDREF(*targets);
        return NS_OK;
    }
    *targets = new SingletonMailCursor(source, property, 0);
    return NS_OK;
}





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

#define BUFF_SIZE 4096
#define getMem(x) PR_Calloc(1,(x))

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
    char* flags;
     
    sprintf(fileurl, "Mail\\%s.ssf",  folderURL);
    // fileurl = MCDepFileURL(fileurl);
    mSummaryFile = openFileWR(fileurl);
    sprintf(fileurl, "Mail\\%s",  folderURL);
    //	fileurl = MCDepFileURL(fileurl);
    mf = openFileWR(fileurl);


    while (mSummaryFile && fgets(buff, BUFF_SIZE, mSummaryFile)) {
      if (startsWith("Status:", buff)) {
          summaryFileFound = 1;
          flags = PL_strdup(&buff[8]);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          sscanf(&buff[9], "%d", &summOffset);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          gRDFService->GetResource(&buff[6], &rFrom);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          gRDFService->GetLiteral((const PRUnichar*)&buff[8], &rSubject);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          gRDFService->GetLiteral((const PRUnichar*)&buff[6], &rDate);
          fgets(buff, BUFF_SIZE, mSummaryFile);
          sscanf(&buff[9], "%d", &messageOffset);
          sprintf(nurl, "%s?%d", url, messageOffset);
          AddMessage(nurl, this, rFrom, rSubject, rDate, summOffset,
                     messageOffset, flags, 0);
          PL_strfree(flags);
      }
    }

    rFrom = nsnull;
    if (!summaryFileFound) {
      /* either a new mailbox or need to read BMF to recreate */
      while (mf && fgets(buff, BUFF_SIZE, mf)) {
        if (strncmp("From ", buff, 5) ==0)  { 
          if (rFrom) AddMessage(nurl, this, rFrom, rSubject, rDate, messageOffset, flags, 0);
          messageOffset = ftell(mf);
          PL_strfree(flags);
          sprintf(nurl, "%s?%i", url, messageOffset);
          flags = nsnull;
          rFrom = nsnull;
          rSubject = rDate = nsnull;
        }
        if ((rFrom) && (startsWith("From:", buff))) {
          gRDFService->GetResource(&buff[6], &rFrom);
        } else if ((!rDate) && (startsWith("Date:", buff))) {
          gRDFService->GetLiteral((const PRUnichar*)&buff[6], &rDate);
        } else if ((!rSubject) && (startsWith("Subject:", buff))) {
          gRDFService->GetLiteral((const PRUnichar*)&buff[8], &rSubject);
        } else if ((!flags) && (startsWith("X-Mozilla-Status:", buff))) {
          flags = PL_strdup(&buff[17]);
        }        
      }
      if (rFrom) AddMessage(nurl, this, rFrom, rSubject, rDate, messageOffset, flags, 0);
      fflush(mSummaryFile);
    }
    free(fileurl);
    free(buff);
    free(nurl);
  }
  return NS_OK;
}

nsresult
MailAccount::InitMailAccount (const char* url) {
    char*  fileurl = (char*) getMem(100);
    int32 n = PR_SKIP_BOTH;
    PRDirEntry	*de;
    PRDir* dir ;
    nsIRDFMailFolder* folder;
    sprintf(fileurl, "Mail\\%s",  &url[14]);
    dir =  PR_OpenDir(fileurl);
    if (dir == NULL) {
        //if (CallPRMkDirUsingFileURL(fileurl, 00700) > -1) dir = OpenDir(fileurl);
    }
    while ((dir != NULL) && ((de = PR_ReadDir(dir, (PRDirFlags)(n++))) != NULL)) {
        if ((!endsWith(".ssf", de->name)) && (!endsWith(".dat", de->name)) && 
            (!endsWith(".snm", de->name)) && (!endsWith("~", de->name))) {              
            sprintf(fileurl, "mailbox://%s/%s", &url[14], de->name);
            gRDFService->GetResource(fileurl, (nsIRDFResource**)&folder);
            AddFolder(folder);
        }
    }
    free(fileurl);
    if (dir) PR_CloseDir(dir);
    return NS_OK;
}



nsresult 
MailFolder::AddMessage(char* uri, MailFolder* folder,
                       nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                       int summaryFileOffset, int mailFileOffset, char* flags, 
                       nsIRDFLiteral* messageID) {
    MailMessage* msg;
    gRDFService->GetResource(uri, (nsIRDFResource**)&msg);
    msg->SetupMessage(folder, from, subject, date, summaryFileOffset, mailFileOffset, flags, 
                      messageID);
    mMessages.AppendElement(msg);
    return NS_OK;
}

nsresult 
MailFolder::AddMessage(char* uri, MailFolder* folder,
                       nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                       int mailFileOffset, char* flags, nsIRDFLiteral* messageID) {
    MailMessage* msg;
    int summaryFileOffset;
    const  char *sfrom, *ssubject, *sdate;
    gRDFService->GetResource(uri, (nsIRDFResource**)&msg);
    if (!flags) flags = "0000";
    fseek(mSummaryFile, 0L, SEEK_END);
    summaryFileOffset = ftell(mSummaryFile);
    
    from->GetValue(&sfrom);
    date->GetValue((const unsigned short**) &sdate);
    subject->GetValue((const unsigned short**) &ssubject);
    
    fprintf(mSummaryFile, 
            "Status: %s\nSOffset: %d\nFrom: %s\nSubject: %s\nDate: %s\nMOffset: %d\n", 
            flags, summaryFileOffset, sfrom, ssubject, sdate, mailFileOffset); 
    
    msg->SetupMessage(folder, from, subject, date, summaryFileOffset, mailFileOffset, flags, 
                      messageID);
    mMessages.AppendElement(msg);
    return NS_OK;
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

