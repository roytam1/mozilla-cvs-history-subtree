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
#include "nsCodebasePrincipal.h"
#include "nsJSPrincipals.h"
#include "xp.h"
#include "plstr.h"

PR_STATIC_CALLBACK(void *) 
nsGetPrincipalArray(JSContext * cx, struct JSPrincipals * prin) {
    return nsnull;
}

//PR_STATIC_CALLBACK(JSBool) 
static JSBool
nsGlobalPrivilegesEnabled(JSContext * cx , struct JSPrincipals *jsprin) {
    return JS_TRUE;
}

PR_STATIC_CALLBACK(void)
nsDestroyJSPrincipals(JSContext * cx, struct JSPrincipals * jsprin) {
    nsJSPrincipals * nsjsprin = (nsJSPrincipals *)jsprin;
    PL_strfree(nsjsprin->jsPrincipals.codebase);
    NS_IF_RELEASE(nsjsprin->nsIPrincipalPtr);
}

nsJSPrincipals::nsJSPrincipals(nsIPrincipal * prin) {
  char * cb;
  nsICodebasePrincipal * cbprin;
  prin->QueryInterface(nsICodebasePrincipal::GetIID(),(void * *)& cbprin);
  cbprin->GetURLString(& cb);
  this->nsIPrincipalPtr = prin;
  jsPrincipals.codebase = PL_strdup(cb);
  jsPrincipals.getPrincipalArray = nsGetPrincipalArray;
  jsPrincipals.globalPrivilegesEnabled = nsGlobalPrivilegesEnabled;
  jsPrincipals.refcount = 0;
  jsPrincipals.destroy = nsDestroyJSPrincipals;
}

