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

  There is a single Mail Data Source Object. When the data source
  is initialized, it reads in the list of accounts and their folders.
  The summary file for a folder is read in on demand (either when the
  folder is asked for its children or when one of the properties of
  a mail message is queried). 

  Mail Accounts, folders and messages are represented by
  nsIMailAccount, nsIMailFolder and nsIMailMessage, which 
  are subclasses of nsIRDFResource. 

  The implementations of these interfaces provided here assume certain
  standard fields for each of these kinds of objects. 

  The MailDataSource can only store information  about  and answer 
  queries pertaining to the standard mail related properties of
  mail objects. Other properties of mail objects and properties about non-mail
  objects which might be in mail folders are taken care off by other data sources
  (such as the default local store). 

  To Do : Need to keep track of all the messages from a person. I don't
   want to create nsIRDFPerson. Maybe we can just keep a hash table of
   vectors? Or maybe nsIRDFPerson is inevitable.

  
 */

#include "nscore.h"
#include "nsIRDFCursor.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIServiceManager.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFMail.h"
#include "nsIRDFService.h"
#include "plhash.h"
#include "plstr.h"

static NS_DEFINE_IID(kIRDFAssertionCursorIID,  NS_IRDFASSERTIONCURSOR_IID);
static NS_DEFINE_IID(kIRDFArcsInCursorIID,     NS_IRDFARCSINCURSOR_IID);
static NS_DEFINE_IID(kIRDFArcsOutCursorIID,    NS_IRDFARCSOUTCURSOR_IID);
static NS_DEFINE_IID(kIRDFCursorIID,           NS_IRDFCURSOR_IID);
static NS_DEFINE_IID(kIRDFDataSourceIID,       NS_IRDFDATASOURCE_IID);
static NS_DEFINE_IID(kIRDFLiteralIID,          NS_IRDFLITERAL_IID);
static NS_DEFINE_IID(kIRDFNodeIID,             NS_IRDFNODE_IID);
static NS_DEFINE_IID(kIRDFResourceIID,         NS_IRDFRESOURCE_IID);
static NS_DEFINE_IID(kIRDFMailAccountIID,      NS_IMAILACCOUNT_IID);
static NS_DEFINE_IID(kIRDFMailFolderIID,       NS_IMAILFOLDER_IID);
static NS_DEFINE_IID(kIRDFMailMessageIID,      NS_IMAILMESSAGE_IID);
static NS_DEFINE_IID(kIRDFMailDataSourceIID,   NS_IRDFMAILDATAOURCE_IID);
static NS_DEFINE_IID(kISupportsIID,            NS_ISUPPORTS_IID);

static nsIRDFDataSource* kMailDataSource; // XXX THE mail data source. need to initialize this.
static nsIRDFService*    kRDFService; // XXX THE rdf service manager. need to initialize this.
// need factories for all these interfaces. Need hooks into GetResource for some of them.
// I am missing about 60% of the add and release refs.

/********************************** MailObject **************************************
 * this is a convenience class that is a super class of mail account, message and folder,
 * that implements all the methods those guys need to be a resource. This code is copied
 * from rdf/base/src. I know thats evil, but ...
 ************************************************************************************/

class MailObject  {
public:
    MailObject(const char* uri);
    virtual ~MailObject(void);

    // nsIRDFNode
    NS_IMETHOD EqualsNode(nsIRDFNode* node, PRBool* result) const;

    // nsIRDFResource
    NS_IMETHOD GetValue(const char* *uri) const;
    NS_IMETHOD EqualsResource(const nsIRDFResource* resource, PRBool* result) const;
    NS_IMETHOD EqualsString(const char* uri, PRBool* result) const;

    // Implementation methods
    const char* GetURI(void) const {
        return mURI;
    }

private:
    char*                             mURI ;
};


MailObject::MailObject(const char* uri)
{
    mURI = PL_strdup(uri);
}


MailObject::~MailObject(void)
{
    // kRDFService->ReleaseNode(this); XXX how should this happen? ReleaseNode is part of ServiceImpl
    // and not nsIRDFService
    PL_strfree(mURI);
}

NS_IMETHODIMP
MailObject::EqualsNode(nsIRDFNode* node, PRBool* result) const
{
    nsresult rv;
    nsIRDFResource* resource;
    if (NS_SUCCEEDED(node->QueryInterface(kIRDFResourceIID, (void**) &resource))) {
        rv = EqualsResource(resource, result);
        NS_RELEASE(resource);
    }
    else {
        *result = PR_FALSE;
        rv = NS_OK;
    }
    return rv;
}

NS_IMETHODIMP
MailObject::GetValue(const char* *uri) const
{
    if (!uri)
        return NS_ERROR_NULL_POINTER;

    *uri = mURI;
    return NS_OK;
}

NS_IMETHODIMP
MailObject::EqualsResource(const nsIRDFResource* resource, PRBool* result) const
{
    if (!resource || !result)
        return NS_ERROR_NULL_POINTER;

    *result = (resource == (nsIRDFResource*) this);
    return NS_OK;
}


NS_IMETHODIMP
MailObject::EqualsString(const char* uri, PRBool* result) const
{
    if (!uri || !result)
        return NS_ERROR_NULL_POINTER;

    *result = (PL_strcmp(uri, mURI) == 0);
    return NS_OK;
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

    MailDataSource (char* uri) {
        NS_INIT_REFCNT();
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
    }
    
    NS_IMETHOD Init(const char* uri) {
        if ((mURI = PL_strdup(uri)) == nsnull)
            return NS_ERROR_OUT_OF_MEMORY;

        //XXX get the service manager, initialize the resources (mResourceChild, etc.)
        
        return NS_OK;
    }


    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target, PRBool tv,
                          nsIRDFAssertionCursor** sources /* out */) {
        return NS_ERROR_NOT_IMPLEMENTED;
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
               
    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,   nsIRDFAssertionCursor** targets /* out */) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

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



/********************************** MailDataAccount **************************************
 ************************************************************************************/

class MailAccount : public MailObject,
                    public nsIRDFMailAccount 
{
private:
  nsIRDFLiteral*       mUser;
  nsIRDFLiteral*       mHost;
  nsVoidArray          mFolders;
        
public:
    
    NS_DECL_ISUPPORTS

    NS_IMETHOD GetUser(nsIRDFLiteral**  result) {
        *result = mUser;
        NS_ADDREF(mUser);
        return NS_OK;
    }

    NS_IMETHOD GetHost(nsIRDFLiteral**  result) {
        *result = mHost;
        NS_ADDREF(mHost);
        return NS_OK;
    }

    NS_IMETHOD GetFolderList (nsVoidArray** result) {
        *result = &mFolders;
        return NS_OK;
    }
 
    NS_IMETHOD AddFolder (nsIRDFMailFolder* folder) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD RemoveFolder (nsIRDFMailFolder* folder) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    MailAccount (char* uri,  nsIRDFLiteral* user,  nsIRDFLiteral* host) : 
        MailObject(uri) {
            mUser = user;
            mHost = host;
            NS_INIT_REFCNT();
            NS_IF_ADDREF(mUser);
            NS_IF_ADDREF(mHost);
    }                        

    virtual ~MailAccount (void) {
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

class MailFolder : public MailObject,
                   public nsIRDFMailFolder 
{
private:
    nsVoidArray    mMessages;
    FILE*          mSummaryFile;
    FolderStatus   mStatus;
    MailAccount*   mAccount;
    nsIRDFLiteral* mName;

public:

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetAccount(nsIRDFMailAccount** account) {
        *account = mAccount;
        return NS_OK;
    }

    NS_IMETHOD GetName(nsIRDFLiteral**  result) {
        *result = mName;
        return NS_OK;
    }
    
    NS_IMETHOD GetMessageList (nsVoidArray** result) {
        *result = &mMessages;
        return NS_OK;
    }

    NS_IMETHOD AddMessage (nsIRDFMailMessage* msg) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD RemoveMessage (nsIRDFMailMessage* msg) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    MailFolder (char* uri, nsIRDFLiteral* name, MailAccount* account) : 
        MailObject(uri) {
            mName = name;
            mAccount = account;
            NS_INIT_REFCNT();
    }                        
    
    virtual ~MailFolder (void) {
        NS_IF_RELEASE(mName);
        NS_IF_RELEASE(mAccount);
    }
        
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

class MailMessage : public MailObject,
                    public nsIRDFMailMessage 
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

public:
    NS_DECL_ISUPPORTS
        
    NS_IMETHOD GetSubject(nsIRDFLiteral**  result) {
        *result = mSubject;
        return NS_OK;
    }

    NS_IMETHOD GetSender(nsIRDFResource**  result) {
        *result = mFrom;
        return NS_OK;
    }
        
    NS_IMETHOD GetDate(nsIRDFLiteral**  result) {
        *result = mDate;
        return NS_OK;
    }

    NS_IMETHOD GetContent(char** result) {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_IMETHOD GetMessageID(nsIRDFLiteral** id) {
        *id = mMessageID;
        return NS_OK;
    }

    NS_IMETHOD GetFlags(char** result) {
        *result = mFlags;
    }
        
    NS_IMETHOD SetFlags(const char* result) {
        memcpy(mFlags, result, 4);
        //xxx write this into the summary file
    }

    MailMessage (char* uri, MailFolder* folder,
                 nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                 int summaryFileOffset, int mailFileOffset, char* flags, nsIRDFLiteral* messageID) :
        MailObject(uri)
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
    }

    virtual ~MailMessage (void) {
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
            rv = kMailDataSource->GetSource(mProperty, mTarget, 1, (nsIRDFResource**)&mValue);
            mSource = (nsIRDFResource*)mValue;
        } else {
            rv = kMailDataSource->GetTarget(mSource, mProperty,  1, &mValue);
            mTarget = mValue;
        }
        NS_ADDREF(mValue);
        NS_ADDREF(mValue); 
        // yes, its required twice, one for the value and one for the source/target
        return rv;
    }
        
    NS_IMETHOD GetValue(nsIRDFNode** aValue) {
        *aValue = mValue;
        return NS_OK;
    }

    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        *aDataSource = kMailDataSource;
        return NS_OK;
    }

    NS_IMETHOD GetSubject(nsIRDFResource** aResource) {
        *aResource = mSource;
        return NS_OK;
    }

    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) {
        *aPredicate = mProperty;
        return NS_OK;
    }

    NS_IMETHOD GetObject(nsIRDFNode** aObject) {
        *aObject = mTarget;
        return NS_OK;
    }

    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) {
        *aTruthValue = 1;
        return NS_OK;
    }
};

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
        *aValue = mValue;
        return NS_OK;
    }

    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) {
        *aDataSource = kMailDataSource;
        return NS_OK;
    }

    NS_IMETHOD GetSubject(nsIRDFResource** aResource) {
        *aResource = mSource;
        return NS_OK;
    }

    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) {
        *aPredicate = mProperty;
        return NS_OK;
    }

    NS_IMETHOD GetObject(nsIRDFNode** aObject) {
        *aObject = mTarget;
        return NS_OK;
    }

    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) {
        *aTruthValue = 1;
        return NS_OK;
    }
};





