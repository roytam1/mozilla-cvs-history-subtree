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
/* describes principals by thier orginating uris*/
#include "nsCodebasePrincipal.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "xp.h"
#include "nsIURL.h"

static NS_DEFINE_IID(kURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_IID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
static NS_DEFINE_IID(kICodebasePrincipalIID, NS_ICODEBASEPRINCIPAL_IID);

NS_IMPL_ISUPPORTS(nsCodebasePrincipal, kICodebasePrincipalIID);
/*
NS_IMPL_QUERY_INTERFACE(nsCodebasePrincipal, kICodebasePrincipalIID);

NS_IMETHODIMP
nsCodebasePrincipal::AddRef(void)
{
  
}

NS_IMETHODIMP
nsCodebasePrincipal::Release(void)
{
  
}
*/
NS_IMETHODIMP
nsCodebasePrincipal::ToJSPrincipal(JSPrincipals * * jsprin)
{
    if (itsJSPrincipals.refcount == 0) {
        this->AddRef();
    }
    *jsprin = &itsJSPrincipals.jsPrincipals;
    return NS_OK;
/*
    char * cb;
  this->GetURLString(& cb);
  * jsprin = NS_STATIC_CAST(JSPrincipals *,this);
  (* jsprin)->codebase = PL_strdup(cb);
  return NS_OK;
  */
}

NS_IMETHODIMP
nsCodebasePrincipal::GetURLString(char **cburl)
{
  return itsURL->GetSpec(cburl);
}

NS_IMETHODIMP
nsCodebasePrincipal::GetURL(nsIURI * * url) 
{
  return itsURL->Clone(url);
}

NS_IMETHODIMP
nsCodebasePrincipal::IsCodebaseExact(PRBool * result)
{
	* result = (this->itsType == nsIPrincipal::PrincipalType_CodebaseExact) ? PR_TRUE : PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP
nsCodebasePrincipal::IsCodebaseRegex(PRBool * result)
{
	* result = (itsType == nsIPrincipal::PrincipalType_CodebaseRegex) ? PR_TRUE : PR_FALSE;
	return NS_OK;
}

NS_IMETHODIMP
nsCodebasePrincipal::GetType(PRInt16 * type)
{
	* type = itsType;
	return NS_OK;
}

NS_IMETHODIMP 
nsCodebasePrincipal::IsSecure(PRBool * result)
{ 
//	if ((0 == memcmp("https:", itsKey, strlen("https:"))) ||
//      (0 == memcmp("file:", itsKey, strlen("file:"))))
//    return PR_TRUE;
	return PR_FALSE;
}

NS_IMETHODIMP
nsCodebasePrincipal::ToString(char * * result)
{
	return NS_OK;
}

NS_IMETHODIMP
nsCodebasePrincipal::HashCode(PRUint32 * code)
{
	code=0;
	return NS_OK;
}

NS_IMETHODIMP
nsCodebasePrincipal::Equals(nsIPrincipal * other, PRBool * result)
{
	PRInt16 oType = 0;
//	char ** oCodeBase;
	other->GetType(& oType);
	* result = (itsType == oType) ? PR_TRUE : PR_FALSE;	
//XXXariel fix this
//	if (* result) {
//		nsICodebasePrincipal * cbother = (nsCodebasePrincipal)other;
//		cbother->GetURL(& oCodeBase);
//	}
//	* result = (itsCodebase == oCodeBase) ? PR_TRUE : PR_FALSE;	
	return NS_OK;
}

nsCodebasePrincipal::nsCodebasePrincipal(PRInt16 type, const char * codeBaseURL)
{
  nsresult rv;
  nsIURI * uri;
  NS_WITH_SERVICE(nsIComponentManager, compMan,kComponentManagerCID,&rv);
  if (!NS_SUCCEEDED(rv)) 
    compMan->CreateInstance(kURLCID,NULL,nsIURL::GetIID(),(void * *)& uri);
  uri->SetSpec((char *) codeBaseURL);
  this->Init(type,uri);
}

nsCodebasePrincipal::nsCodebasePrincipal(PRInt16 type, nsIURI * url)
{
  this->Init(type,url);
}

void // nsresult
nsCodebasePrincipal::Init(PRInt16 type, nsIURI * uri)
{
  NS_INIT_REFCNT();
  NS_ADDREF(this);
  this->itsType = type;
  if(NS_SUCCEEDED(uri->Clone(& this->itsURL))) 
  {
    NS_ADDREF(this->itsURL);
  }
  itsJSPrincipals = new nsJSPrincipals(this);
  // XXX check result
}

nsCodebasePrincipal::~nsCodebasePrincipal(void)
{
  JSPrincipals * jsprin;
  this->ToJSPrincipal(& jsprin);
  PL_strfree(jsprin->codebase);
}
