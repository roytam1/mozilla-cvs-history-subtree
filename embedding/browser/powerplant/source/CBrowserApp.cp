/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is the Mozilla browser.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications, Inc.  Portions created by Netscape are
 * Copyright (C) 1999, Mozilla.  All Rights Reserved.
 * 
 * Contributor(s):
 *   Conrad Carlen <ccarlen@netscape.com>
 */

#include "CBrowserApp.h"

#include <LGrowZone.h>
#include <PP_Messages.h>
#include <PP_Resources.h>
#include <UDrawingState.h>
#include <UMemoryMgr.h>
#include <URegistrar.h>
#include <LPushButton.h>
#include <LStaticText.h>

#include <LWindow.h>
#include <LCaption.h>
#include <LTextTableView.h>
#include <LTableMonoGeometry.h>
#include <LTableArrayStorage.h>
#include <LTableSingleSelector.h>
#include <LCellSizeToFit.h>

#include <UControlRegistry.h>
#include <UGraphicUtils.h>
#include <UEnvironment.h>

#include <Appearance.h>

#include "ApplIDs.h"
#include "CBrowserWindow.h"
#include "CBrowserShell.h"
#include "CUrlField.h"
#include "CThrobber.h"
#include "UMacUnicode.h"
#include "nsIImageManager.h"
#include "nsIServiceManager.h"
#include "nsIEventQueueService.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "nsIPref.h"
#include "nsRepeater.h"
#include "nsILocalFile.h"
#include "nsILocalFileMac.h"
#include "nsIFileSpec.h"
#include "nsEmbedAPI.h"
#include "nsMPFileLocProvider.h"
#include "nsXPIDLString.h"
#include "macstdlibextras.h"
#include "SIOUX.h"

#include <TextServices.h>

#if USE_PROFILES
#include "CProfileManager.h"
#include "nsIProfileChangeStatus.h"
#endif

static NS_DEFINE_CID(kPrefCID, NS_PREF_CID);
static const char* kProgramName = "PP Browser";

// ===========================================================================
//		¥ Main Program
// ===========================================================================

int main()
{
								
	SetDebugThrow_(PP_PowerPlant::debugAction_Alert);	// Set Debugging options
	SetDebugSignal_(PP_PowerPlant::debugAction_Alert);

	PP_PowerPlant::InitializeHeap(3);		// Initialize Memory Manager
											// Parameter is number of Master Pointer
											// blocks to allocate
	
	
	PP_PowerPlant::UQDGlobals::InitializeToolbox(&qd);	// Initialize standard Toolbox managers

#if DEBUG
	::InitializeSIOUX(false);
#endif
	
	::InitTSMAwareApplication();
	
	new PP_PowerPlant::LGrowZone(20000);	// Install a GrowZone function to catch low memory situations.

	{
		CBrowserApp	theApp;			// create instance of your application
	
		theApp.Run();
	}

	::CloseTSMAwareApplication();
	
	return 0;
}


// ---------------------------------------------------------------------------
//		¥ CBrowserApp
// ---------------------------------------------------------------------------
//	Constructor

CBrowserApp::CBrowserApp()
{

#if USE_PROFILES
    mRefCnt = 1;
#endif

	if ( PP_PowerPlant::UEnvironment::HasFeature( PP_PowerPlant::env_HasAppearance ) ) {
		::RegisterAppearanceClient();
	}

	RegisterClass_(PP_PowerPlant::LWindow);	// You must register each kind of
	RegisterClass_(PP_PowerPlant::LCaption);	// PowerPlant classes that you use in your PPob resource.
	
	// Register the Appearance Manager/GA classes
	PP_PowerPlant::UControlRegistry::RegisterClasses();
	
	// Register classes used by embedding
	RegisterClass_(CBrowserShell);
	RegisterClass_(CBrowserWindow);
	RegisterClass_(CUrlField);
	RegisterClass_(CThrobber);

#if USE_PROFILES	
	RegisterClass_(LScroller);
	RegisterClass_(LTextTableView);
	RegisterClass_(LColorEraseAttachment);
#endif

   // We need to idle threads often
   SetSleepTime(0);
    
   // Get the directory which contains the mozilla parts
   // In this case it is the app directory but it could
   // be anywhere (an existing install of mozilla)

   nsresult        rv;
   ProcessSerialNumber psn;
   ProcessInfoRec  processInfo;
   FSSpec          appSpec;
   nsCOMPtr<nsILocalFileMac> macDir;
   nsCOMPtr<nsILocalFile>    appDir;  // If this ends up being NULL, default is used

   if (!::GetCurrentProcess(&psn)) {
      processInfo.processInfoLength = sizeof(processInfo);
      processInfo.processName = NULL;
      processInfo.processAppSpec = &appSpec;    
      if (!::GetProcessInformation(&psn, &processInfo)) {
         // Turn the FSSpec of the app into an FSSpec of the app's directory
         ::FSMakeFSSpec(appSpec.vRefNum, appSpec.parID, "\p", &appSpec);
         // Make an nsILocalFile out of it
         rv = NS_NewLocalFileWithFSSpec(&appSpec, PR_TRUE, getter_AddRefs(macDir));
         if (NS_SUCCEEDED(rv))
             appDir = do_QueryInterface(macDir);
      }
   }

   rv = NS_InitEmbedding(appDir, nsnull, kProgramName);

}


// ---------------------------------------------------------------------------
//		¥ ~CBrowserApp
// ---------------------------------------------------------------------------
//	Destructor
//

CBrowserApp::~CBrowserApp()
{
   UMacUnicode::ReleaseUnit();
	   
   NS_TermEmbedding();
}

// ---------------------------------------------------------------------------
//		¥ StartUp
// ---------------------------------------------------------------------------
//	This method lets you do something when the application starts up
//	without a document. For example, you could issue your own new command.

void
CBrowserApp::StartUp()
{
    nsresult rv;

#if USE_PROFILES
    CProfileManager *profileMgr = new CProfileManager(NS_ConvertASCIItoUCS2(kProgramName),
                                                      NS_ConvertASCIItoUCS2("Profiles"));
    profileMgr->StartUp();
    AddAttachment(profileMgr);

    // Register for profile changes    
    NS_WITH_SERVICE(nsIObserverService, observerService, NS_OBSERVERSERVICE_CONTRACTID, &rv);
    ThrowIfNil_(observerService);
    observerService->AddObserver(this, PROFILE_APPROVE_CHANGE_TOPIC);
    observerService->AddObserver(this, PROFILE_CHANGE_TEARDOWN_TOPIC);
    observerService->AddObserver(this, PROFILE_AFTER_CHANGE_TOPIC);

#else
    
    // If we don't want different user profiles, all that's needed is
    // to make an nsMPFileLocProvider. This will provide the same file
    // locations as the profile service but always within the specified folder.
    
    nsCOMPtr<nsIFile> rootDir;   
    nsMPFileLocProvider *locationProvider = new nsMPFileLocProvider;
    ThrowIfNil_(locationProvider);
    rv = NS_GetSpecialDirectory(NS_MAC_PREFS_DIR, getter_AddRefs(rootDir));
    ThrowIfNil_(rootDir);
    rv = locationProvider->Initialize(rootDir, kProgramName);   
    ThrowIfError_(rv);
    
    NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
    ThrowIfNil_(prefs);
    // Needed because things read default prefs during startup
    prefs->ResetPrefs();
    prefs->ReadUserPrefs();

#endif

    InitializePrefs();

	  ObeyCommand(PP_PowerPlant::cmd_New, nil);	// EXAMPLE, create a new window
}


// ---------------------------------------------------------------------------
//	¥ ProcessNextEvent												  [public]
// ---------------------------------------------------------------------------
//	Retrieve and handle the next event in the event queue 

void
CBrowserApp::ProcessNextEvent()
{
	EventRecord		macEvent;

		// When on duty (application is in the foreground), adjust the
		// cursor shape before waiting for the next event. Except for the
		// very first time, this is the same as adjusting the cursor
		// after every event.
	
	if (IsOnDuty()) {
			
			// Calling OSEventAvail with a zero event mask will always
			// pass back a null event. However, it fills the EventRecord
			// with the information we need to set the cursor shape--
			// the mouse location in global coordinates and the state
			// of the modifier keys.
			
		::OSEventAvail(0, &macEvent);
		AdjustCursor(macEvent);
	}
	
		// Retrieve the next event. Context switch could happen here.
	
	SetUpdateCommandStatus(false);
	Boolean	gotEvent = ::WaitNextEvent(everyEvent, &macEvent, mSleepTime,
										mMouseRgn);

		// Let Attachments process the event. Continue with normal
		// event dispatching unless suppressed by an Attachment.
	
	if (LAttachable::ExecuteAttachments(msg_Event, &macEvent)) {
		if (gotEvent) {
#if DEBUG
         if (!SIOUXHandleOneEvent(&macEvent))
#endif
			DispatchEvent(macEvent);
		} else {
			UseIdleTime(macEvent);
			
  		   Repeater::DoIdlers(macEvent);
         // yield to other threads
         ::PR_Sleep(PR_INTERVAL_NO_WAIT);
		}
	}

									// Repeaters get time after every event
	LPeriodical::DevoteTimeToRepeaters(macEvent);
	Repeater::DoRepeaters(macEvent);
	
									// Update status of menu items
	if (IsOnDuty() && GetUpdateCommandStatus()) {
		UpdateMenus();
	}
}


// ---------------------------------------------------------------------------
//		¥ ObeyCommand
// ---------------------------------------------------------------------------
//	This method lets the application respond to commands like Menu commands

Boolean
CBrowserApp::ObeyCommand(
	PP_PowerPlant::CommandT	inCommand,
	void					*ioParam)
{
	Boolean		cmdHandled = true;

	switch (inCommand) {
	
		case PP_PowerPlant::cmd_New:
			{
   			CBrowserWindow	*theWindow = dynamic_cast<CBrowserWindow*>(LWindow::CreateWindow(wind_BrowserWindow, this));
   			ThrowIfNil_(theWindow);
   			// LWindow is not initially visible in PPob resource
   			theWindow->Show();

            // Just for demo sake, load a URL	
            theWindow->GetBrowserShell()->LoadURL("http://www.mozilla.org");
			}
			break;
			
		// Any that you don't handle, such as cmd_About and cmd_Quit,
		// will be passed up to LApplication
		default:
			cmdHandled = PP_PowerPlant::LApplication::ObeyCommand(inCommand, ioParam);
			break;
	}
	
	return cmdHandled;
}

// ---------------------------------------------------------------------------
//		¥ FindCommandStatus
// ---------------------------------------------------------------------------
//	This function enables menu commands.
//

void
CBrowserApp::FindCommandStatus(
	PP_PowerPlant::CommandT	inCommand,
	Boolean					&outEnabled,
	Boolean					&outUsesMark,
	PP_PowerPlant::Char16	&outMark,
	Str255					outName)
{

	switch (inCommand) {
	
		// Return menu item status according to command messages.
		case PP_PowerPlant::cmd_New:
			outEnabled = true;
			break;

		// Any that you don't handle, such as cmd_About and cmd_Quit,
		// will be passed up to LApplication
		default:
			PP_PowerPlant::LApplication::FindCommandStatus(inCommand, outEnabled,
												outUsesMark, outMark, outName);
			break;
	}
}


Boolean CBrowserApp::AttemptQuitSelf(SInt32 inSaveOption)
{	    
   // IMPORTANT: This is one unfortunate thing about Powerplant - Windows don't
   // get destroyed until the destructor of LCommander. We need to delete
   // all of the CBrowserWindows though before we terminate embedding.
    
 	TArrayIterator<LCommander*> iterator(mSubCommanders, LArrayIterator::from_End);
 	LCommander*		theSub;
 	while (iterator.Previous(theSub)) {
 	    if (dynamic_cast<CBrowserWindow*>(theSub)) {
 		    mSubCommanders.RemoveItemsAt(1, iterator.GetCurrentIndex());
 		    delete theSub;
 		}
 	}
    
    return true;
}


nsresult CBrowserApp::InitializePrefs()
{
   nsresult rv;
   NS_WITH_SERVICE(nsIPref, prefs, kPrefCID, &rv);
   if (NS_SUCCEEDED(rv)) {	  

        rv = InitCachePrefs();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Could not initialize cache prefs");
        
		// We are using the default prefs from mozilla. If you were
		// disributing your own, this would be done simply by editing
		// the default pref files.
		
		PRBool inited;
		rv = prefs->GetBoolPref("ppbrowser.prefs_inited", &inited);
		if (NS_FAILED(rv) || !inited)
		{
            prefs->SetIntPref("font.size.variable.x-western", 12);
            prefs->SetIntPref("font.size.fixed.x-western", 12);
            rv = prefs->SetBoolPref("ppbrowser.prefs_inited", PR_TRUE);
            if (NS_SUCCEEDED(rv))
                rv = prefs->SavePrefFile();
        }
        
	}
	else
		NS_ASSERTION(PR_FALSE, "Could not get preferences service");
		
    return rv;
}

nsresult CBrowserApp::InitCachePrefs()
{
	const char * const CACHE_DIR_PREF   = "browser.cache.directory";
	
	nsresult rv;
    PRBool isDir = PR_FALSE;
	NS_WITH_SERVICE(nsIPref, prefs, NS_PREF_CONTRACTID, &rv);
	if (NS_FAILED(rv)) return rv;
	
/*		
	// If the pref is already set don't do anything
	nsCOMPtr<nsILocalFile> cacheDir;
	rv = prefs->GetFileXPref(CACHE_DIR_PREF, getter_AddRefs(cacheDir));
	if (NS_SUCCEEDED(rv) && cacheDir.get()) {
    rv = cacheDir->IsDirectory(&isDir);
    if (NS_SUCCEEDED(rv) && isDir)
      return;
  }
*/

  // Set up the new pref

  nsCOMPtr<nsIFile> profileDir;   
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profileDir));
  NS_ASSERTION(profileDir, "NS_APP_USER_PROFILE_50_DIR is not defined");
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILocalFile> cacheDir(do_QueryInterface(profileDir));
  NS_ASSERTION(cacheDir, "Cannot get nsILocalFile from cache dir");
    
  PRBool exists;
  cacheDir->Append("Cache");
  rv = cacheDir->Exists(&exists);
  if (NS_SUCCEEDED(rv) && !exists)
    rv = cacheDir->Create(nsIFile::DIRECTORY_TYPE, 0775);
  if (NS_FAILED(rv)) return rv;

  return prefs->SetFileXPref(CACHE_DIR_PREF, cacheDir);
}

#if USE_PROFILES

// ---------------------------------------------------------------------------
//  CBrowserApp : nsISupports
// ---------------------------------------------------------------------------

NS_IMPL_ISUPPORTS2(CBrowserApp, nsIObserver, nsISupportsWeakReference);

// ---------------------------------------------------------------------------
//  CBrowserApp : nsIObserver
// ---------------------------------------------------------------------------

NS_IMETHODIMP CBrowserApp::Observe(nsISupports *aSubject, const PRUnichar *aTopic, const PRUnichar *someData)
{
    #define CLOSE_WINDOWS_ON_SWITCH 1

    nsresult rv = NS_OK;
    
    if (nsCRT::strcmp(aTopic, PROFILE_APPROVE_CHANGE_TOPIC) == 0)
    {
        // Ask the user if they want to
        DialogItemIndex item = UModalAlerts::StopAlert(alrt_ConfirmProfileSwitch);
        if (item != kStdOkItemIndex)
        {
            nsCOMPtr<nsIProfileChangeStatus> status = do_QueryInterface(aSubject);
            NS_ENSURE_TRUE(status, NS_ERROR_FAILURE);
            status->BlockChange();
        }
    }
    else if (nsCRT::strcmp(aTopic, PROFILE_CHANGE_TEARDOWN_TOPIC) == 0)
    {
        // Close all open windows. Alternatively, we could just call CBrowserWindow::Stop()
        // on each. Either way, we have to stop all network activity on this phase.
        
        TArrayIterator<LCommander*> iterator(mSubCommanders, LArrayIterator::from_End);
        LCommander*		theSub;
        while (iterator.Previous(theSub)) {
            CBrowserWindow *browserWindow = dynamic_cast<CBrowserWindow*>(theSub);
            if (browserWindow) {
                browserWindow->Stop();
        	    mSubCommanders.RemoveItemsAt(1, iterator.GetCurrentIndex());
        	    delete browserWindow;
        	}
        }
    }
    else if (nsCRT::strcmp(aTopic, PROFILE_AFTER_CHANGE_TOPIC) == 0)
    {
        InitCachePrefs();
        
        // Make a new default window
        ObeyCommand(PP_PowerPlant::cmd_New, nil);
    }
    return rv;
}

#endif // USE_PROFILES
