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

#include <ctype.h> // for toupper()
#include <stdio.h>
#include "nscore.h"
#include "nsIRDFCursor.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIRDFResourceFactory.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsVoidArray.h"  // XXX introduces dependency on raptorbase
#include "nsRDFCID.h"
#include "rdfutil.h"
#include "nsIRDFMail.h"
#include "nsIRDFService.h"
#include "plhash.h"
#include "plstr.h"
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
static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);

static const char kMailRoot[]  = "MailRoot";

#define NC_NAMESPACE_URI "http://home.netscape.com/NC-rdf#"
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, child);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, subject);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, from);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, date);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Name);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Folder);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Column);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Columns);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, Title);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, user);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, account);
DEFINE_RDF_VOCAB(NC_NAMESPACE_URI, NC, host);

#define BUFF_SIZE 4096
#define getMem(x) PR_Calloc(1,(x))


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

class MailDataSource :  public nsIRDFMailDataSource 
{
private:
    char*       mURI;
    nsVoidArray mAccounts;
    nsVoidArray* mObservers;
    nsIRDFService* mSrv;
    nsIRDFDataSource* mMiscMailData;

    // caching frequently used resources
    nsIRDFResource* mResourceChild;
    nsIRDFResource* mResourceFolder;
    nsIRDFResource* mResourceFrom;
    nsIRDFResource* mResourceSubject;
    nsIRDFResource* mResourceDate;
    nsIRDFResource* mResourceUser;
    nsIRDFResource* mResourceHost;
    nsIRDFResource* mResourceAccount;
    nsIRDFResource* mResourceName;
    nsIRDFResource* mMailRoot;
    nsIRDFResource* mResourceColumns;

    // internal methods
    nsresult InitAccountList (void) ;
    nsresult AddColumns (void);
    PRBool peq (nsIRDFResource* r1, nsIRDFResource* r2) ;

public:
  
    NS_DECL_ISUPPORTS

    MailDataSource(void) ;
    virtual ~MailDataSource (void) ;

    // nsIRDFMailDataSource  methods
    NS_IMETHOD GetAccountList (nsVoidArray** result) ;    
    NS_IMETHOD AddAccount (nsIRDFMailAccount* folder) ;
    NS_IMETHOD RemoveAccount (nsIRDFMailAccount* folder);
    NS_IMETHOD Init(const char* uri) ;
    NS_IMETHOD GetURI(const char* *uri) const ;

    //nsIRDFDataSource methods
    NS_IMETHOD GetSource(nsIRDFResource* property, nsIRDFNode* target,
                         PRBool tv,  nsIRDFResource** source /* out */);
    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,   PRBool tv,  nsIRDFNode** target) ;
    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target, PRBool tv, nsIRDFAssertionCursor** sources) ;
    NS_IMETHOD GetTargets(nsIRDFResource* source,  nsIRDFResource* property,    
                          PRBool tv,   nsIRDFAssertionCursor** targets) ;
    NS_IMETHOD Assert(nsIRDFResource* source, nsIRDFResource* property, 
                      nsIRDFNode* target,  PRBool tv) ;
    NS_IMETHOD Unassert(nsIRDFResource* source,
                        nsIRDFResource* property, nsIRDFNode* target) ;
    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property, nsIRDFNode* target,
                            PRBool tv,    PRBool* hasAssertion) ;
    NS_IMETHOD AddObserver(nsIRDFObserver* n) ;
    NS_IMETHOD RemoveObserver(nsIRDFObserver* n) ;
    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsIRDFArcsInCursor** labels) ;
    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsIRDFArcsOutCursor** labels); 
    NS_IMETHOD Flush() ;
};


class MailAccount : public nsIRDFMailAccount 
{
private:
    nsIRDFLiteral*       mUser;
    nsIRDFLiteral*       mHost;
    nsVoidArray          mFolders;
    char*                mURI ;        
    nsresult InitMailAccount (const char* uri);

public:
    
    NS_DECL_ISUPPORTS

    RDF_RESOURCE_METHODS

    NS_IMETHOD GetUser(nsIRDFLiteral**  result) const;
    NS_IMETHOD GetName(nsIRDFLiteral**  result) const ;
    NS_IMETHOD GetHost(nsIRDFLiteral**  result) const ;
    NS_IMETHOD GetFolderList (nsVoidArray** result) ;
    NS_IMETHOD AddFolder (nsIRDFMailFolder* folder) ;
    NS_IMETHOD RemoveFolder (nsIRDFMailFolder* folder) ;

    MailAccount (const char* uri) ;
    MailAccount (char* uri,  nsIRDFLiteral* user,  nsIRDFLiteral* host) ;
    virtual ~MailAccount (void) ;
};

typedef enum {
    UNINITIALIZED,
    INIT_IN_PROGRESS,
    WRITE_IN_PROGRESS,
    OK
} MailFolderStatus;


class MailFolder : public nsIRDFMailFolder 
{
private:
    nsVoidArray    mMessages;
    FILE*          mSummaryFile;
    MailFolderStatus   mStatus;
    MailAccount*   mAccount;
    nsIRDFLiteral* mName;
    char*                             mURI ;

public:

    NS_DECL_ISUPPORTS

    RDF_RESOURCE_METHODS

    NS_IMETHOD GetAccount(nsIRDFMailAccount** account) ;
    NS_IMETHOD GetName(nsIRDFLiteral**  result) const ;
    NS_IMETHOD GetMessageList (nsVoidArray** result) ;
    NS_IMETHOD AddMessage (nsIRDFMailMessage* msg) ;
    NS_IMETHOD RemoveMessage (nsIRDFMailMessage* msg) ;
    MailFolder (const char* uri) ;
    MailFolder (char* uri, nsIRDFLiteral* name, MailAccount* account) ;
    virtual ~MailFolder (void) ;
    nsresult  AddMessage(PRUnichar* uri, MailFolder* folder,
                         nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                         int summaryFileOffset, int mailFileOffset, char* flags, 
                         nsIRDFLiteral* messageID) ;
    nsresult AddMessage(PRUnichar* uri, MailFolder* folder,
                        nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                        int mailFileOffset, char* flags, nsIRDFLiteral* messageID) ;    
    nsresult ReadSummaryFile(char* uri);
};



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
    char*           mURI ;
    
public:
    NS_DECL_ISUPPORTS
    
    RDF_RESOURCE_METHODS
    
    NS_IMETHOD GetFolder(nsIRDFMailFolder**  result) ;
    NS_IMETHOD GetSubject(nsIRDFLiteral**  result) ;
    NS_IMETHOD GetSender(nsIRDFResource**  result) ;
    NS_IMETHOD GetDate(nsIRDFLiteral**  result) ;
    NS_IMETHOD GetContent(char** result) ;
    NS_IMETHOD GetMessageID(nsIRDFLiteral** id) ;
    NS_IMETHOD GetFlags(char** result) ;
    NS_IMETHOD SetFlags(const char* result) ;
    nsresult SetupMessage (MailFolder* folder,
                           nsIRDFResource* from, nsIRDFLiteral* subject, nsIRDFLiteral* date,
                           int summaryFileOffset, int mailFileOffset, char* flags, 
                           nsIRDFLiteral* messageID) ;
    MailMessage (const char* uri) ;
    virtual ~MailMessage (void) ;
};

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
                        PRBool inversep) ;
    virtual ~SingletonMailCursor(void) ;

    // nsISupports interface
    NS_DECL_ISUPPORTS
   
    // nsIRDFCursor interface
    NS_IMETHOD Advance(void) ;
    NS_IMETHOD GetValue(nsIRDFNode** aValue) ;

    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) ;
    NS_IMETHOD GetSubject(nsIRDFResource** aResource) ;
    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) ;
    NS_IMETHOD GetObject(nsIRDFNode** aObject) ;
    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) ;
};


class ArrayMailCursor : public nsIRDFAssertionCursor , 
                        public nsIRDFArcsOutCursor
{
private:
    nsIRDFNode*     mValue;
    nsIRDFResource* mSource;
    nsIRDFResource* mProperty;
    nsIRDFNode*     mTarget;
    int             mCount;
    nsVoidArray*    mArray;

public:
    ArrayMailCursor(nsIRDFResource* u, nsIRDFResource* s, nsVoidArray* array) ;
    virtual ~ArrayMailCursor(void) ;
    // nsISupports interface
    NS_DECL_ISUPPORTS
   
    // nsIRDFCursor interface
    NS_IMETHOD Advance(void) ;
    NS_IMETHOD GetValue(nsIRDFNode** aValue) ;
    // nsIRDFAssertionCursor interface
    NS_IMETHOD GetDataSource(nsIRDFDataSource** aDataSource) ;
    NS_IMETHOD GetSubject(nsIRDFResource** aResource) ;
    NS_IMETHOD GetPredicate(nsIRDFResource** aPredicate) ;
    NS_IMETHOD GetObject(nsIRDFNode** aObject) ;
    NS_IMETHOD GetTruthValue(PRBool* aTruthValue) ;
};


