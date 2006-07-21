/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is GPS Support for Minimo
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Doug Turner <dougt@meer.net>
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


#include "memory.h"
#include "stdlib.h"

#include "nspr.h"
#include "nsCRT.h"

#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsArray.h"

#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIObserver.h"
#include "nsISupportsPrimitives.h"

#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"
#include "nsIWebBrowserChrome.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

#include "nsIThread.h"
#include "nsIRunnable.h"

#include "mozIGPSService.h"
#include "nsIDOMClassInfo.h"
#include "nsIScriptNameSpaceManager.h"

#include "nsIPromptService.h"
#include "nsIStringBundle.h"

#include "nsIScriptSecurityManager.h"
#include "nsIPermissionManager.h"
#include "nsIURI.h"

#define MINIMO_PROPERTIES_URL "chrome://minimo/locale/minimo.properties"

class GPSService: public nsIObserver, public mozIGPSService
{
public:  

  static GPSService * GetService();

  GPSService();  
  virtual ~GPSService();  
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_MOZIGPSSERVICE

  private:
  nsresult StartGPS();
  nsresult StopGPS();
  void GetBestLocation(double* aLat, double* aLon, double* aAlt, double *aErr);

  nsresult ReadPref(nsIPrefBranch*, const char* pref);
  PRBool   AskForPermission();

  PRBool  mStarted;
  PRBool  mEnabled;
  PRInt32 mPrecision;

  nsCOMArray<mozIGPSService> mGPSServers;
  
};

static class GPSService *gGPSService = 0;

GPSService *
GPSService::GetService ()
{
  if (!gGPSService)
    gGPSService = new GPSService();
  
 
  NS_IF_ADDREF(gGPSService);
  NS_IF_ADDREF(gGPSService); // one for us!
  return gGPSService;
}

GPSService::GPSService()  
{
  mStarted     = PR_FALSE;
  mEnabled     = PR_TRUE;
  mPrecision   = -1;
}  

GPSService::~GPSService()  
{
  StopGPS();
}  

NS_INTERFACE_MAP_BEGIN(GPSService)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(mozIGPSService)
   NS_INTERFACE_MAP_ENTRY_EXTERNAL_DOM_CLASSINFO(GPSService)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(GPSService)
NS_IMPL_THREADSAFE_RELEASE(GPSService)


nsresult
GPSService::StartGPS()
{
  if (mStarted)
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager = do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> gpss;
  categoryManager->EnumerateCategory("GPS Service", getter_AddRefs(gpss));
  
  if (gpss)
  {
    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(gpss->HasMoreElements(&hasMore)) && hasMore)
    {
      nsCOMPtr<nsISupports> gpsCIDSupports;
      gpss->GetNext(getter_AddRefs(gpsCIDSupports));

      nsCOMPtr<nsISupportsCString> gpsCIDSupportsCString = do_QueryInterface(gpsCIDSupports);
      
      nsCAutoString gpsCID;
      gpsCIDSupportsCString->GetData(gpsCID);

      nsCOMPtr<mozIGPSService> gps = do_GetService(gpsCID.get());
      if (gps)
      {
        //        MessageBox(0, gpsCID.get(), "Adding...", 0);
        mGPSServers.AppendObject(gps);
      }
    }
  }

#ifdef WINCE
  // add the built in here
  extern mozIGPSService* GetWMGPSService();
  mGPSServers.AppendObject(GetWMGPSService());
#endif

  mStarted = PR_TRUE;

  return NS_OK;
}

nsresult
GPSService::StopGPS()
{
  mStarted = PR_FALSE;
  mGPSServers.Clear();
  return NS_OK;
}

nsresult
GPSService::ReadPref(nsIPrefBranch *prefBranch, const char* pref)
{
  if (!strcmp(pref, "gps.enabled"))
  {
    PRBool enabled;
    nsresult rv = prefBranch->GetBoolPref(pref, &enabled);
    if (NS_SUCCEEDED(rv))
    {
      if (mEnabled == enabled)
        return NS_OK;

      mEnabled = enabled;
    }
    return NS_OK;
  }
  
  if (!strcmp(pref, "gps.precision"))
  {
    PRInt32 precision;
    nsresult rv = prefBranch->GetIntPref(pref, &precision);
    if (NS_SUCCEEDED(rv))
      mPrecision = precision;
    
    return NS_OK;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
GPSService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  if (!strcmp(aTopic,"xpcom-startup")) 
  {
    nsCOMPtr<nsIPrefBranch2> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (!prefBranch)
      return NS_OK;

    ReadPref(prefBranch, "gps.enabled");
    ReadPref(prefBranch, "gps.precision");

    prefBranch->AddObserver("gps.", (nsIObserver*)this, PR_FALSE);

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) 
  {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
    nsXPIDLCString cstr;
    
    const char* pref = NS_ConvertUCS2toUTF8(aData).get();
    return ReadPref(prefBranch, pref);
  }
  return NS_OK;
}


PRBool
GPSService::AskForPermission()
{
  if (!mEnabled)
    return PR_FALSE;

  nsCOMPtr<nsIScriptSecurityManager> secman(do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID));
  
  nsCOMPtr<nsIPrincipal> principal;
  nsCOMPtr<nsIPrincipal> sysPrincipal;

  secman->GetSubjectPrincipal(getter_AddRefs(principal));
  secman->GetSystemPrincipal(getter_AddRefs(sysPrincipal));
  
  PRBool equals;

  // we only want to do this for content javascript.  The
  // test is that it does have a principal (if there is no
  // principal, then the caller is JS).  And the principal
  // does not equal the system principal.  If the principal
  // equals the system principal, then the javascript that
  // is running is chrome.  Got it?  

  if (principal && NS_SUCCEEDED(principal->Equals(sysPrincipal, &equals)) && !equals)
  {
    nsCOMPtr<nsIURI> codebase;
    principal->GetURI(getter_AddRefs(codebase));
  
    nsCOMPtr<nsIPermissionManager> permMgr = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
    PRUint32 permissions = nsIPermissionManager::UNKNOWN_ACTION;
    
    permMgr->TestPermission(codebase, "GPS", &permissions);
    
    if (permissions == nsIPermissionManager::ALLOW_ACTION)
      return PR_TRUE;

    if (permissions == nsIPermissionManager::DENY_ACTION)
      return PR_FALSE;
    
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    if (!bundleService)
      return PR_FALSE;
    
    nsCOMPtr<nsIStringBundle> bundle;
    bundleService->CreateBundle(MINIMO_PROPERTIES_URL, getter_AddRefs(bundle));
    
    if (!bundle)
      return PR_FALSE;
    
    nsCAutoString host;
    codebase->GetHost(host);

    const PRUnichar *formatStrings[] = { NS_ConvertUTF8toUTF16(host).get() };
    nsXPIDLString message;
    bundle->FormatStringFromName(NS_LITERAL_STRING("gpsRequest").get(),
                                 formatStrings,
                                 NS_ARRAY_LENGTH(formatStrings),
                                 getter_Copies(message));
    
    nsXPIDLString title;
    bundle->GetStringFromName(NS_LITERAL_STRING("gpsRequestTitle").get(), getter_Copies(title));
 
    nsXPIDLString remember;
    bundle->GetStringFromName(NS_LITERAL_STRING("gpsRequestRemember").get(), getter_Copies(remember));
   
    nsCOMPtr<nsIPromptService> promptService = do_GetService("@mozilla.org/embedcomp/prompt-service;1");


    PRBool rememberCheck = PR_TRUE;
    PRBool okayToContinue = PR_FALSE;
    promptService->ConfirmCheck(nsnull, title.get(), message.get(), remember.get(), &rememberCheck, &okayToContinue);

    if (rememberCheck)
      permMgr->Add(codebase, "GPS", 
                   okayToContinue ? nsIPermissionManager::ALLOW_ACTION : nsIPermissionManager::DENY_ACTION);

    return okayToContinue;
  }

  // Caller is C++ or chrome (per dveditz)
  return PR_TRUE;
}

void GPSService::GetBestLocation(double* aLat, double* aLon, double* aAlt, double *aErr)
{
  StartGPS();
  
  double lastLat = -1, lastLon = -1, lastAlt = -1, lastErr = -1;
  
  nsresult rv;

  PRInt32 count = mGPSServers.Count();
  for (PRInt32 i = 0; i < count; i++)
  {
    nsCOMPtr<mozIGPSService> gps = mGPSServers[i];
    
    if (gps)
    {
      double lat = -1, lon = -1, alt = -1, err = -1;    

      rv = gps->GetLatitude(&lat);
      if (NS_FAILED(rv) || lat == -1 ) continue;

      rv = gps->GetLongitude(&lon);
      if (NS_FAILED(rv) || lon == -1 ) continue;

      // we don't really care that much about alt.
      gps->GetAltitude(&alt);
            
      rv = gps->GetError(&err);
      if (NS_FAILED(rv) || err == -1 ) continue;
      
      if (err < lastErr || lastErr == -1)
      {
        lastLon = lon;
        lastLat = lat;
        lastAlt = alt;
        lastErr = err;
      }
    }
  }

  if (aLat)
    *aLat = lastLat;
  
  if (aLon)
    *aLon = lastLon;

  if (aAlt)
    *aAlt = lastAlt;

  if (aErr)
    *aErr = lastErr;
}

/* readonly attribute double latitude; */
NS_IMETHODIMP GPSService::GetLatitude(double *aLatitude)
{
  *aLatitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

  GetBestLocation(aLatitude, nsnull, nsnull, nsnull);
  return NS_OK;
}

/* readonly attribute double longitude; */
NS_IMETHODIMP GPSService::GetLongitude(double *aLongitude)
{
  *aLongitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

  GetBestLocation(nsnull, aLongitude, nsnull, nsnull);
  return NS_OK;
}

/* readonly attribute double altitude; */
NS_IMETHODIMP GPSService::GetAltitude(double *aAltitude)
{
  *aAltitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

  GetBestLocation(nsnull, nsnull, aAltitude, nsnull);
  return NS_OK;
}

/* readonly attribute double error; */
NS_IMETHODIMP GPSService::GetError(double *aError)
{
  *aError = -1;
  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

  GetBestLocation(nsnull, nsnull, nsnull, aError);
  return NS_OK;
}

//------------------------------------------------------------------------------
//  XPCOM REGISTRATION BELOW
//------------------------------------------------------------------------------

#define GPSService_CID \
{ 0xa5d15efc, 0x8fcb, 0x490d, \
  {0xb9, 0x95, 0xa9, 0x2b, 0x65, 0x0f, 0xf0, 0xc8} }

#define GPSService_ContractID "@mozilla.org/gps/service;1"


#define GPSService_DOMCI_EXTENSION_CID \
{ 0xa5d15efc, 0x8fcb, 0x490d, \
  {0xb9, 0x95, 0xa9, 0x2b, 0x65, 0x0f, 0xf0, 0xc9} }

#define GPSService_DOMCI_EXTENSION_CONTRACTID \
"@mozilla.org/gps/domci;1"


static NS_METHOD GPSServiceRegistration(nsIComponentManager *aCompMgr,
                                           nsIFile *aPath,
                                           const char *registryLocation,
                                           const char *componentType,
                                           const nsModuleComponentInfo *info)
{
  nsresult rv;
  
  nsCOMPtr<nsIServiceManager> servman = do_QueryInterface((nsISupports*)aCompMgr, &rv);
  if (NS_FAILED(rv))
    return rv;  
  
  nsCOMPtr<nsICategoryManager> catman;
  servman->GetServiceByContractID(NS_CATEGORYMANAGER_CONTRACTID, 
                                  NS_GET_IID(nsICategoryManager), 
                                  getter_AddRefs(catman));
  
  if (NS_FAILED(rv))
    return rv;
  
  nsXPIDLCString previous;

  rv = catman->AddCategoryEntry("xpcom-startup",
                                "GPSService", 
                                GPSService_ContractID,
                                PR_TRUE, 
                                PR_TRUE, 
                                getter_Copies(previous));

  rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                "GPSService",
                                GPSService_DOMCI_EXTENSION_CONTRACTID,
                                PR_TRUE,
                                PR_TRUE,
                                getter_Copies(previous));
  

  char* iidString = NS_GET_IID(mozIGPSService).ToString();
  if (!iidString)
    return NS_ERROR_OUT_OF_MEMORY;

  rv = catman->AddCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                "mozIGPSService",
                                iidString,
                                PR_TRUE, 
                                PR_TRUE, 
                                getter_Copies(previous));
  nsCRT::free(iidString);


  return rv;
}

static NS_METHOD GPSServiceUnregistration(nsIComponentManager *aCompMgr,
                                          nsIFile *aPath,
                                          const char *registryLocation,
                                          const nsModuleComponentInfo *info)
{
  nsresult rv;
  
  nsCOMPtr<nsIServiceManager> servman = do_QueryInterface((nsISupports*)aCompMgr, &rv);
  if (NS_FAILED(rv))
    return rv;
  
  nsCOMPtr<nsICategoryManager> catman;
  servman->GetServiceByContractID(NS_CATEGORYMANAGER_CONTRACTID, 
                                  NS_GET_IID(nsICategoryManager), 
                                  getter_AddRefs(catman));
  
  if (NS_FAILED(rv))
    return rv;
  
  rv = catman->DeleteCategoryEntry("xpcom-startup",
                                   "GPSService", 
                                   PR_TRUE);
  
  rv = catman->DeleteCategoryEntry(JAVASCRIPT_DOM_CLASS,
                                   "GPSService", 
                                   PR_TRUE);

  rv = catman->DeleteCategoryEntry(JAVASCRIPT_DOM_INTERFACE,
                                   "mozIGPSService",
                                   PR_TRUE);
  return rv;
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(GPSService, GPSService::GetService)

NS_DECL_DOM_CLASSINFO(GPSService)

NS_DOMCI_EXTENSION(GPSService)

    static NS_DEFINE_CID(kGPSServiceCID, GPSService_CID);
    NS_DOMCI_EXTENSION_ENTRY_BEGIN(GPSService)
        NS_DOMCI_EXTENSION_ENTRY_INTERFACE(mozIGPSService)
    NS_DOMCI_EXTENSION_ENTRY_END_NO_PRIMARY_IF(GPSService, PR_TRUE, &kGPSServiceCID)

NS_DOMCI_EXTENSION_END

static const nsModuleComponentInfo components[] =
{
  { "GPSService", 
    GPSService_CID, 
    GPSService_ContractID,
    GPSServiceConstructor,
    GPSServiceRegistration,
    GPSServiceUnregistration
  },

  { 
    "GPS DOMCI Extender",
    GPSService_DOMCI_EXTENSION_CID,
    GPSService_DOMCI_EXTENSION_CONTRACTID,
    NS_DOMCI_EXTENSION_CONSTRUCTOR(GPSService)
  }

};

void PR_CALLBACK
GPSServiceModuleDestructor(nsIModule* self)
{
  NS_IF_RELEASE(NS_CLASSINFO_NAME(GPSService));
  NS_IF_RELEASE(gGPSService);

}

NS_IMPL_NSGETMODULE_WITH_DTOR(MozGPSModule,
                              components, 
                              GPSServiceModuleDestructor)
