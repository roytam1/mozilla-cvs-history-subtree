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

#include <windows.h>
#include <Winbase.h>

#ifdef WINCE
// UNTIL WE EXCLUSIVELY CARE ABOUT >=WM5
#ifndef _GPSAPI_H_

#define GPS_MAX_SATELLITES      12
#define GPS_MAX_PREFIX_NAME     16
#define GPS_MAX_FRIENDLY_NAME   64

#define GPS_VERSION_1           1
#define GPS_VERSION_CURRENT     GPS_VERSION_1

typedef enum {
	GPS_FIX_UNKNOWN = 0,
	GPS_FIX_2D,
	GPS_FIX_3D
}
GPS_FIX_TYPE;

typedef enum {
	GPS_FIX_SELECTION_UNKNOWN = 0,
	GPS_FIX_SELECTION_AUTO,
	GPS_FIX_SELECTION_MANUAL
}
GPS_FIX_SELECTION;

typedef enum {
	GPS_FIX_QUALITY_UNKNOWN = 0,
	GPS_FIX_QUALITY_GPS,
	GPS_FIX_QUALITY_DGPS
}
GPS_FIX_QUALITY;

//
// GPS_VALID_XXX bit flags in GPS_POSITION structure are valid.
//
#define GPS_VALID_UTC_TIME                                 0x00000001
#define GPS_VALID_LATITUDE                                 0x00000002
#define GPS_VALID_LONGITUDE                                0x00000004
#define GPS_VALID_SPEED                                    0x00000008
#define GPS_VALID_HEADING                                  0x00000010
#define GPS_VALID_MAGNETIC_VARIATION                       0x00000020
#define GPS_VALID_ALTITUDE_WRT_SEA_LEVEL                   0x00000040
#define GPS_VALID_ALTITUDE_WRT_ELLIPSOID                   0x00000080
#define GPS_VALID_POSITION_DILUTION_OF_PRECISION           0x00000100
#define GPS_VALID_HORIZONTAL_DILUTION_OF_PRECISION         0x00000200
#define GPS_VALID_VERTICAL_DILUTION_OF_PRECISION           0x00000400
#define GPS_VALID_SATELLITE_COUNT                          0x00000800
#define GPS_VALID_SATELLITES_USED_PRNS                     0x00001000
#define GPS_VALID_SATELLITES_IN_VIEW                       0x00002000
#define GPS_VALID_SATELLITES_IN_VIEW_PRNS                  0x00004000
#define GPS_VALID_SATELLITES_IN_VIEW_ELEVATION             0x00008000
#define GPS_VALID_SATELLITES_IN_VIEW_AZIMUTH               0x00010000
#define GPS_VALID_SATELLITES_IN_VIEW_SIGNAL_TO_NOISE_RATIO 0x00020000


//
// GPS_DATA_FLAGS_XXX bit flags set in GPS_POSITION dwFlags field
// provide additional information about the state of the query.
// 

// Set when GPS hardware is not connected to GPSID and we 
// are returning cached data.
#define GPS_DATA_FLAGS_HARDWARE_OFF                        0x00000001

//
// GPS_POSITION contains our latest physical coordinates, the time, 
// and satellites used in determining these coordinates.
// 
typedef struct _GPS_POSITION {
	DWORD dwVersion;             // Current version of GPSID client is using.
	DWORD dwSize;                // sizeof(_GPS_POSITION)

	// Not all fields in the structure below are guaranteed to be valid.  
	// Which fields are valid depend on GPS device being used, how stale the API allows
	// the data to be, and current signal.
	// Valid fields are specified in dwValidFields, based on GPS_VALID_XXX flags.
	DWORD dwValidFields;

	// Additional information about this location structure (GPS_DATA_FLAGS_XXX)
	DWORD dwFlags;
	
	//** Time related
	SYSTEMTIME stUTCTime; 	//  UTC according to GPS clock.
	
	//** Position + heading related
	double dblLatitude;            // Degrees latitude.  North is positive
	double dblLongitude;           // Degrees longitude.  East is positive
	float  flSpeed;                // Speed in knots
	float  flHeading;              // Degrees heading (course made good).  True North=0
	double dblMagneticVariation;   // Magnetic variation.  East is positive
	float  flAltitudeWRTSeaLevel;  // Altitute with regards to sea level, in meters
	float  flAltitudeWRTEllipsoid; // Altitude with regards to ellipsoid, in meters

	//** Quality of this fix
	GPS_FIX_QUALITY     FixQuality;        // Where did we get fix from?
	GPS_FIX_TYPE        FixType;           // Is this 2d or 3d fix?
	GPS_FIX_SELECTION   SelectionType;     // Auto or manual selection between 2d or 3d mode
	float flPositionDilutionOfPrecision;   // Position Dilution Of Precision
	float flHorizontalDilutionOfPrecision; // Horizontal Dilution Of Precision
	float flVerticalDilutionOfPrecision;   // Vertical Dilution Of Precision

	//** Satellite information
	DWORD dwSatelliteCount;                                            // Number of satellites used in solution
	DWORD rgdwSatellitesUsedPRNs[GPS_MAX_SATELLITES];                  // PRN numbers of satellites used in the solution

	DWORD dwSatellitesInView;                      	                   // Number of satellites in view.  From 0-GPS_MAX_SATELLITES
	DWORD rgdwSatellitesInViewPRNs[GPS_MAX_SATELLITES];                // PRN numbers of satellites in view
	DWORD rgdwSatellitesInViewElevation[GPS_MAX_SATELLITES];           // Elevation of each satellite in view
	DWORD rgdwSatellitesInViewAzimuth[GPS_MAX_SATELLITES];             // Azimuth of each satellite in view
	DWORD rgdwSatellitesInViewSignalToNoiseRatio[GPS_MAX_SATELLITES];  // Signal to noise ratio of each satellite in view
} GPS_POSITION, *PGPS_POSITION;


//
// GPS_DEVICE contains information about the device driver and the
// service itself and is returned on a call to GPSGetDeviceState().
// States are indicated with SERVICE_STATE_XXX flags defined in service.h
// 
typedef struct _GPS_DEVICE {
	DWORD    dwVersion;                                 // Current version of GPSID client is using.
	DWORD    dwSize;                                    // sizeof this structure
	DWORD    dwServiceState;                            // State of the GPS Intermediate Driver service.  
	DWORD    dwDeviceState;                             // Status of the actual GPS device driver.
	FILETIME ftLastDataReceived;                        // Last time that the actual GPS device sent information to the intermediate driver.
	WCHAR    szGPSDriverPrefix[GPS_MAX_PREFIX_NAME];    // Prefix name we are using to communicate to the base GPS driver
	WCHAR    szGPSMultiplexPrefix[GPS_MAX_PREFIX_NAME]; // Prefix name that GPS Intermediate Driver Multiplexer is running on
	WCHAR    szGPSFriendlyName[GPS_MAX_FRIENDLY_NAME];  // Friendly name real GPS device we are currently using
} *PGPS_DEVICE, GPS_DEVICE;

#endif // _GPSAPI_H_


typedef HANDLE (*OpenDeviceProc)(HANDLE hNewLocationData, HANDLE hDeviceStateChange, const WCHAR *szDeviceName, DWORD dwFlags);
typedef DWORD  (*CloseDeviceProc)(HANDLE hGPSDevice);
typedef DWORD  (*GetPositionProc)(HANDLE hGPSDevice, GPS_POSITION *pGPSPosition, DWORD dwMaximumAge, DWORD dwFlags);
#endif

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

  nsresult ReadPref(nsIPrefBranch*, const char* pref);
  PRBool   AskForPermission();

  PRBool  mEnabled;
  PRInt32 mPrecision;

#ifdef WINCE
  HINSTANCE mGPSInst;
  HANDLE mGPSDevice;
  OpenDeviceProc mOpenDevice;
  CloseDeviceProc mCloseDevice;
  GetPositionProc mGetPosition;
#endif

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
  mEnabled     = PR_TRUE;
  mPrecision   = -1;

#ifdef WINCE
  mGPSInst = LoadLibrary("GPSAPI.dll");

  mOpenDevice  = (OpenDeviceProc) GetProcAddress( mGPSInst, "GPSOpenDevice");
  mCloseDevice = (CloseDeviceProc) GetProcAddress( mGPSInst, "GPSCloseDevice");
  mGetPosition = (GetPositionProc) GetProcAddress( mGPSInst, "GPSGetPosition");

  mGPSInst = NULL;
  mGPSDevice = NULL;

  StartGPS();
#endif

}  

GPSService::~GPSService()  
{
  MessageBox(0, "~", "~", 0);
#ifdef WINCE
  StopGPS();
  FreeLibrary(mGPSInst);
#endif
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
#ifdef WINCE

  if (mGPSDevice)
    return NS_OK;

  mGPSDevice = mOpenDevice(NULL, NULL, NULL, 0);

  if (!mGPSDevice)
    MessageBox(0, "No GPS Device Found", "ERROR", 0);
#endif
  return NS_OK;
}

nsresult
GPSService::StopGPS()
{
#ifdef WINCE
  if (mGPSDevice)
  {
    mCloseDevice(mGPSDevice);
    mGPSDevice = NULL;
  }
#endif
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

        if (mEnabled)
          StartGPS();//
        else
          StopGPS();//
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
    ReadPref(prefBranch, "gps.port");
    ReadPref(prefBranch, "gps.updateDelay");

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
/* readonly attribute double latitude; */
NS_IMETHODIMP GPSService::GetLatitude(double *aLatitude)
{
  *aLatitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

#ifdef WINCE

  GPS_POSITION pos;
  memset(&pos, 0, sizeof(GPS_POSITION));
  pos.dwVersion = GPS_VERSION_1;
  pos.dwSize = sizeof(GPS_POSITION);

  int attempts = 10;

  while (attempts &&
         (! (pos.dwValidFields & GPS_VALID_LATITUDE) ||
          pos.dwFlags == GPS_DATA_FLAGS_HARDWARE_OFF ||
          pos.FixQuality == GPS_FIX_QUALITY_UNKNOWN))
  {
    mGetPosition(mGPSDevice, &pos, 500000, 0);
    attempts--;
  }
  
  if (attempts == 0)
    return NS_ERROR_NOT_AVAILABLE;

  *aLatitude = pos.dblLatitude;
#endif
  return NS_OK;
}

/* readonly attribute double longitude; */
NS_IMETHODIMP GPSService::GetLongitude(double *aLongitude)
{
  *aLongitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;

#ifdef WINCE
  GPS_POSITION pos;
  memset(&pos, 0, sizeof(GPS_POSITION));
  pos.dwVersion = GPS_VERSION_1;
  pos.dwSize = sizeof(GPS_POSITION);

  int attempts = 10;

  while (attempts &&
         (! (pos.dwValidFields & GPS_VALID_LONGITUDE) ||
          pos.dwFlags == GPS_DATA_FLAGS_HARDWARE_OFF ||
          pos.FixQuality == GPS_FIX_QUALITY_UNKNOWN))
  {
    mGetPosition(mGPSDevice, &pos, 500000, 0); 
    attempts--;
  }
  
  if (attempts == 0)
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aLongitude = pos.dblLongitude;
#endif
  return NS_OK;
}

/* readonly attribute double altitude; */
NS_IMETHODIMP GPSService::GetAltitude(double *aAltitude)
{
  *aAltitude = -1;

  PRBool allow = AskForPermission();
  if (!allow)
    return NS_ERROR_ABORT;
  
#ifdef WINCE

  GPS_POSITION pos;
  memset(&pos, 0, sizeof(GPS_POSITION));
  pos.dwVersion = GPS_VERSION_1;
  pos.dwSize = sizeof(GPS_POSITION);

  int attempts = 10;

  if (attempts &&
      (! (pos.dwValidFields & GPS_VALID_ALTITUDE_WRT_SEA_LEVEL) || 
       pos.dwFlags == GPS_DATA_FLAGS_HARDWARE_OFF ||
       pos.FixQuality == GPS_FIX_QUALITY_UNKNOWN))
  {
    mGetPosition(mGPSDevice, &pos, 500000, 0); 
    attempts--;
  }

  if (attempts == 0)
  {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aAltitude = pos.flAltitudeWRTSeaLevel;
#endif
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
