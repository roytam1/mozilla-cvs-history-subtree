/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Mozilla capabilities security API.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>.
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "capsPrivate.h"

#include "nsAutoLock.h"
#include "nsXPCOMCID.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

#include "nsICategoryManager.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"
#include "nsIStringEnumerator.h"

nsCOMArray<capsIPermissionPolicy>
CAPSGlobal::GetPermissionPolicies()
{
  nsAutoLock lock(sLock);
  return mPolicies;
}

already_AddRefed<CAPSGlobal>
CAPSGlobal::GetGlobal(nsresult &rv)
{
  if (!sLock) {
    sLock = PR_NewLock();
    if (!sLock) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      return nsnull;
    }
  }

  nsAutoLock lock(sLock);

  if (!sGlobal) {
    if (sIsShuttingDown) {
      NS_WARNING("Trying to use CAPS service after xpcom shutdown!");

      rv = NS_ERROR_FAILURE;
      return nsnull;
    }

    sGlobal = new CAPSGlobal(rv);
    if (!sGlobal) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      return nsnull;
    }
    if (NS_FAILED(rv)) {
      delete sGlobal;
      sGlobal = nsnull;
      return nsnull;
    }
  }

  sGlobal->AddRef();
  return sGlobal;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(CAPSGlobal, nsIObserver);

NS_IMETHODIMP
CAPSGlobal::Observe(nsISupports* aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
  sIsShuttingDown = PR_TRUE;
  return NS_OK;
}

CAPSGlobal::CAPSGlobal(nsresult &rv)
{
  // GetGlobal is supposed to lock sLock before it calls us.
  NS_ASSERTION(sLock, "Creating CAPSGlobal without a lock!");

  NS_ASSERTION(sGlobal, "Creating second CAPSGlobal?");
  NS_ASSERTION(sIsShuttingDown, "Creating CAPSGlobal during shutdown?");

  nsCOMPtr<nsICategoryManager> catMan
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsISimpleEnumerator> e;
  rv = catMan->EnumerateCategory("caps-permission-policy", getter_AddRefs(e));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIUTF8StringEnumerator> se (do_QueryInterface(e, &rv));
  if (NS_FAILED(rv))
    return;

  PRBool more;
  while (NS_SUCCEEDED(rv = se->HasMore(&more)) && more) {
    nsCAutoString contract;
    se->GetNext(contract);

    nsCOMPtr<capsIPermissionPolicy> policy
      (do_CreateInstance(contract.get(), &rv));
      // XXXbsmedberg: do we really want to hard-fail here? I guess so.
    if (NS_FAILED(rv))
      return;

    if (!mPolicies.AppendObject(policy)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      return;
    }
  }

  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIObserverService> obs
    (do_GetService("@mozilla.org/observer-service;1", &rv));
  if (NS_FAILED(rv))
    return;

  rv = obs->AddObserver(this, "xpcom-loader-shutdown", PR_FALSE);

  return;
}

CAPSGlobal::~CAPSGlobal()
{
  {
    nsAutoLock lock(sLock);
    sGlobal = nsnull;
  }

  PR_DestroyLock(sLock);
  sLock = nsnull;
}

CAPSGlobal*
CAPSGlobal::sGlobal = nsnull;

PRBool
CAPSGlobal::sIsShuttingDown = PR_FALSE;

PRLock*
CAPSGlobal::sLock = nsnull;
