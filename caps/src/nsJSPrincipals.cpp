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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsJSPrincipals.h"
#include "xp.h"
#include "plstr.h"

PR_STATIC_CALLBACK(void *) 
getPrincipalArray(JSContext *, struct JSPrincipals *) {
    return nsnull;
}

PR_STATIC_CALLBACK(JSBool) 
globalPrivilegesEnabled(JSContext *, struct JSPrincipals *) {
    return JS_TRUE;
}

PR_STATIC_CALLBACK(void)
destroy(JSContext *, struct JSPrincipals * jsPrincipals) {
    PL_strfree(jsPrincipals->codebase);
    NS_IF_RELEASE(((nsJSPrincipals *) jsPrincipals)->nsIPrincipalPtr);
}

nsJSPrincipals::nsJSPrincipals(nsIPrincipal * prin) {
  char * cb;
  nsICodebasePrincipal * cbprin;
  prin->QueryInterface(nsICodebasePrincipal::GetIID(),(void * *)& cbprin);
  cbprin->GetURLString(& cb);
  jsPrincipals.codebase = PL_strdup(cb);
  jsPrincipals.getPrincipalArray = getPrincipalArray;
  jsPrincipals.globalPrivilegesEnabled = globalPrivilegesEnabled;
  jsPrincipals.refcount = 0;
  jsPrincipals.destroy = destroy;
}

