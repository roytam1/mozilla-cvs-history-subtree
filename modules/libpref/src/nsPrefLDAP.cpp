/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
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
 * Copyright (C) 2001 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s): Mitesh Shah <mitesh@netscape.com> (Original Author)
 *                 Dan Mosedale <dmose@netscape.com>
 */

#if defined(MOZ_LDAP_XPCOM)

#include "nsPrefLDAP.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "nsXPIDLString.h"
#include "nsILDAPErrors.h"
#include "nsIEventQueueService.h"
#include "prefapi.h"

// nsISupports Implementation

NS_IMPL_THREADSAFE_ISUPPORTS2(nsPrefLDAP, nsIPrefLDAP, nsILDAPMessageListener)

// Constructor
//
nsPrefLDAP::nsPrefLDAP() :
        mFinished(PR_FALSE), // This is a control variable for event loop
        mAttrCount(0), mAttrs(0)
{
    NS_INIT_ISUPPORTS();
}

// Destructor
//
nsPrefLDAP::~nsPrefLDAP()
{
}


// Messages received are passed back via this function.
// void OnLDAPMessage (in nsILDAPMessage aMessage) 
//
NS_IMETHODIMP 
nsPrefLDAP::OnLDAPMessage(nsILDAPMessage *aMessage)
{
    PRInt32 messageType;

    // just in case.
    //
    if (!aMessage) {
        return NS_OK;
    }

    // figure out what sort of message was returned
    //
    nsresult rv = aMessage->GetType(&messageType);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsPrefLDAP::OnLDAPMessage(): unexpected "
                 "error in aMessage->GetType()");
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    switch (messageType) {

    case nsILDAPMessage::RES_BIND:

        // a bind has completed
        //
        return OnLDAPBind(aMessage);

    case nsILDAPMessage::RES_SEARCH_ENTRY:
        
        // a search entry has been returned
        //
        return OnLDAPSearchEntry(aMessage);

    case nsILDAPMessage::RES_SEARCH_RESULT:

        // the search is finished; we're all done
        //  
        return OnLDAPSearchResult(aMessage);

    default:
        
        // Given the LDAP operations nsPrefLDAP uses, we should
        // never get here.  If we do get here in a release build, it's
        // probably a bug, but maybe it's the LDAP server doing something
        // weird.  Might as well try and continue anyway.  The session should
        // eventually get reaped by the timeout code, if necessary.
        //
        NS_ERROR("nsPrefLDAP::OnLDAPMessage(): unexpected "
                 "LDAP message received");
        return NS_OK;
    }
}

nsresult
nsPrefLDAP::OnLDAPBind(nsILDAPMessage *aMessage)
{

    PRInt32 errCode;

    mOperation = 0;  // done with bind op; make nsCOMPtr release it

    // get the status of the bind
    //
    nsresult rv = aMessage->GetErrorCode(&errCode);
    if (NS_FAILED(rv)) {
        
        NS_ERROR("nsPrefLDAP::OnLDAPBind(): couldn't get "
                 "error code from aMessage");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }


    // check to be sure the bind succeeded
    //
    if ( errCode != nsILDAPErrors::SUCCESS) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // ok, we're starting a search
    //
    return StartLDAPSearch();
}

nsresult
nsPrefLDAP::OnLDAPSearchEntry(nsILDAPMessage *aMessage)
{
    nsresult rv;       

    // Attributes are retrived in StartLDAPSearch
    // iterate through them
    //
    for ( PRUint32 i=0 ; i < mAttrCount ; i++ ) {

        PRUnichar **vals;
        PRUint32 valueCount;

        // get the values of this attribute
        // XXX better failure handling
        //
        rv = aMessage->GetValues(mAttrs[i], &valueCount, &vals);
        if (NS_FAILED(rv)) {
            NS_WARNING("nsPrefLDAP:OnLDAPSearchEntry(): "
                       "aMessage->GetValues() failed\n");
            FinishLDAPQuery();
            return rv;;
        }

        // store  all values of this attribute in the mResults.
        //
        for ( PRUint32 j=0 ; j < valueCount; j++ ) {
        
            mResults += NS_LITERAL_STRING("\n") +
                        NS_ConvertASCIItoUCS2(mAttrs[i]) +
                        NS_LITERAL_STRING("= ") + 
                        nsDependentString(vals[j]);
            
        }
        
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(valueCount, vals);

    }

    return NS_OK;
}


nsresult
nsPrefLDAP::OnLDAPSearchResult(nsILDAPMessage *aMessage)
{
    PRUnichar * results;
    nsresult rv;

    // we are done with the LDAP operation, now get the results.
    //
    rv = GetResults(&results);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return rv;
    }

    // Pass the data back to JS callback. 
    //
    rv = CallJSFunction(results);
    
    // Free results in any case
    if (results)
        nsMemory::Free(results);
        
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return rv;
    }

    // We are done with the LDAP search.
    // Release the control variable for the eventloop and other members
    // 
    FinishLDAPQuery();
    
    // Release memory allocated for mAttrs
    
    if (mAttrCount > 0)
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mAttrCount, mAttrs);

    return NS_OK;
}

nsresult
nsPrefLDAP::CallJSFunction(PRUnichar * aResults)
{
    const char *funcName = "processLDAPValues";
    unsigned int argc = 1;    
    JSBool ok;
    jsval rval, argv;

    // Get the Pref JS Context
    //
    JSContext *prefContext;
    if (PREF_GetConfigContext(&prefContext) != PREF_NOERROR) 
        return NS_ERROR_FAILURE;

    // Get the Pref Object
    JSObject *prefObject;
    if (PREF_GetPrefConfigObject(&prefObject) != PREF_NOERROR)
        return NS_ERROR_FAILURE;

    // Convert results to a JS String. 
    //
    if (aResults) {
        JSString * str = JS_NewUCStringCopyZ(prefContext, (const jschar *)aResults);
        if (str) {
            argv = STRING_TO_JSVAL(str);
        }
    }

    // Call JS function 'processLDAPValues' with results as an argument.
    //
    JS_BeginRequest(prefContext);
    
    ok = JS_CallFunctionName(prefContext, prefObject, funcName, 
                             argc, &argv, &rval); 
    JS_EndRequest(prefContext);

    // Return an error if the call to JS function fails
    //
    if (!ok) 
        return NS_ERROR_FAILURE;

    // Everything went smooth return NS_OK
    //
    return NS_OK;
}

nsresult
nsPrefLDAP::StartLDAPSearch()
{
    nsresult rv; 
    nsCOMPtr<nsILDAPMessageListener> selfProxy; // for callback


    // create and initialize an LDAP operation (to be used for the search
    //  
    mOperation = 
        do_CreateInstance("@mozilla.org/network/ldap-operation;1", &rv);

    if (NS_FAILED(rv)) {
        NS_ERROR("nsPrefLDAP::StartLDAPSearch(): couldn't "
                 "create @mozilla.org/network/ldap-operation;1");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // get a proxy object so the callback happens on the main thread
    //
    rv = NS_GetProxyForObject(NS_CURRENT_EVENTQ, 
                              NS_GET_IID(nsILDAPMessageListener),
                              NS_STATIC_CAST(nsILDAPMessageListener *, this),
                              PROXY_ASYNC | PROXY_ALWAYS,
                              getter_AddRefs(selfProxy));
    if (NS_FAILED(rv)) {
        NS_ERROR("nsPrefLDAP::StartLDAPSearch(): couldn't "
                 "create proxy to this object for callback");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // initialize the LDAP operation object
    //
    rv = mOperation->Init(mConnection, selfProxy);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsPrefLDAP::StartLDAPSearch(): couldn't "
                 "initialize LDAP operation");
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    // get the search filter associated with the directory server url; 
    //
    nsXPIDLCString urlFilter;
    rv = mServerURL->GetFilter(getter_Copies(urlFilter));
    if ( NS_FAILED(rv) ){
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    // get the base dn to search
    //
    nsXPIDLCString dn;
    rv = mServerURL->GetDn(getter_Copies(dn));
    if ( NS_FAILED(rv) ){
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    // and the scope
    //
    PRInt32 scope;
    rv = mServerURL->GetScope(&scope);
    if ( NS_FAILED(rv) ){
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }

    
    rv = mServerURL->GetAttributes(&mAttrCount, &mAttrs);
    if ( NS_FAILED(rv) ) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED;
    }


    // time to kick off the search.
    //
    rv = mOperation->SearchExt(NS_ConvertUTF8toUCS2(dn).get(), scope, 
                               NS_ConvertUTF8toUCS2(urlFilter).get(), 
                               mAttrCount,
                               NS_CONST_CAST(const char **, mAttrs), 0, 0);

    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

// void initConnection (); 
//
NS_IMETHODIMP
nsPrefLDAP::InitConnection()
{
    nsCOMPtr<nsILDAPMessageListener> selfProxy;
    nsresult rv;        // temp for xpcom return values
    
    // create an LDAP connection
    //
    mConnection = do_CreateInstance("@mozilla.org/network/ldap-connection;1",
                                    &rv);
    if (NS_FAILED(rv)) {
        NS_ERROR("nsPrefLDAP::InitConnection(): could "
                 "not create @mozilla.org/network/ldap-connection;1");
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // have we been properly initialized?
    //
    if (!mServerURL) {
        NS_ERROR("nsPrefLDAP::InitConnection(): mServerURL "
                 "is NULL");
        FinishLDAPQuery();
        return NS_ERROR_NOT_INITIALIZED;
    }

    // host to connect to
    //
    nsXPIDLCString host;
    rv = mServerURL->GetHost(getter_Copies(host));
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // on which port
    //
    PRInt32 port;
    rv = mServerURL->GetPort(&port);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }
        
    rv = mConnection->Init(host, port, 0);
    if NS_FAILED(rv) {
        FinishLDAPQuery();
        return rv;
    }

    // create and initialize an LDAP operation (to be used for the bind)
    //  
    mOperation = do_CreateInstance("@mozilla.org/network/ldap-operation;1", 
                                   &rv);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // get a proxy object so the callback happens on the main thread
    //
    rv = NS_GetProxyForObject(NS_CURRENT_EVENTQ,
                              NS_GET_IID(nsILDAPMessageListener), 
                              NS_STATIC_CAST(nsILDAPMessageListener *, this),
                              PROXY_ASYNC | PROXY_ALWAYS, 
                              getter_AddRefs(selfProxy));
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }

    // our OnLDAPMessage accepts all result callbacks
    //
    rv = mOperation->Init(mConnection, selfProxy);
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_UNEXPECTED; // this should never happen
    }

    // kick off a bind operation 
    // 
    rv = mOperation->SimpleBind(NULL); 
    if (NS_FAILED(rv)) {
        FinishLDAPQuery();
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}

void
nsPrefLDAP::FinishLDAPQuery()
{
    // We are done with the LDAP operation. 
    // Release the Control variable for the eventloop
    //
    mFinished = PR_TRUE;
    
    // Release member variables
    //
    mConnection = 0;
    mOperation = 0;
    mServerURL = 0;
 
}

//  attribute nsILDAPURL serverURL;
//
NS_IMETHODIMP 
nsPrefLDAP::GetServerURL(nsILDAPURL * *aServerURL)
{
    if (!aServerURL) {
        return NS_ERROR_NULL_POINTER;
    }
    
    *aServerURL = mServerURL;
    NS_IF_ADDREF(*aServerURL);

    return NS_OK;
}

NS_IMETHODIMP 
nsPrefLDAP::SetServerURL(nsILDAPURL * aServerURL)
{
    if (!aServerURL) {
        return NS_ERROR_NULL_POINTER;
    }

    mServerURL = aServerURL;

    return NS_OK;
}

// readonly attribute string results;
//
NS_IMETHODIMP nsPrefLDAP::GetResults(PRUnichar **aResults)
{
    // Convert the values to a C String
    //
    if (!mResults.IsEmpty())
        *aResults = mResults.ToNewUnicode();
    return NS_OK;
}

// readonly attribute boolean isDone;
//
NS_IMETHODIMP nsPrefLDAP::GetIsDone(PRBool *aIsDone) 
{
    *aIsDone = mFinished;
    return NS_OK;
}

#endif
