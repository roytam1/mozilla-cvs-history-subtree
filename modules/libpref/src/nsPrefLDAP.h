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
 *
 */

#include "nsCOMPtr.h"
#include "nsILDAPConnection.h"
#include "nsILDAPMessageListener.h"
#include "nsIPrefLDAP.h"
#include "nsILDAPURL.h"
#include "nsString.h"



// 0308fb36-1dd2-11b2-b16f-8510e8c5311a
#define NS_PREFLDAP_CID \
{ 0x0308fb36, 0x1dd2, 0x11b2, \
 { 0xb1, 0x6f, 0x85, 0x10, 0xe8, 0xc5, 0x31, 0x1a }}


class nsPrefLDAP : public nsILDAPMessageListener, 
                   public nsIPrefLDAP
{
  public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSILDAPMESSAGELISTENER
    NS_DECL_NSIPREFLDAP

    nsPrefLDAP();
    virtual ~nsPrefLDAP();

  protected:

    nsCOMPtr<nsILDAPConnection> mConnection; // connection used for search
    nsCOMPtr<nsILDAPOperation> mOperation;   // current ldap op
    nsCOMPtr<nsILDAPURL> mServerURL;         // LDAP URL
    PRBool mFinished;                        // control variable for eventQ
    PRUint32 mAttrCount;                     // No. of attrbiutes
    char **mAttrs;                           // Attributes to search
    nsString mResults;                       // values to return

    
    // check that we bound ok and start then call StartLDAPSearch
    nsresult OnLDAPBind(nsILDAPMessage *aMessage); 

    // add to the results set
    nsresult OnLDAPSearchEntry(nsILDAPMessage *aMessage); 


    nsresult OnLDAPSearchResult(nsILDAPMessage *aMessage); 

    // kick off a search
    nsresult StartLDAPSearch();
    
    // Pass results back to JS
    nsresult CallJSFunction(PRUnichar *aResults);
    
    // Clean up after the LDAP Query is done.
    void FinishLDAPQuery();
};

