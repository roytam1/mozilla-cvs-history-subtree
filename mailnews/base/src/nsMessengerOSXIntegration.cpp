/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Scott MacGregor <mscott@mozilla.org>
 *  Jon Baumgartner <jon@bergenstreetsoftware.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif


#include "nscore.h"

#include "nsMessengerOSXIntegration.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccount.h"
#include "nsIRDFResource.h"
#include "nsIMsgFolder.h"
#include "nsCOMPtr.h"
#include "nsMsgBaseCID.h"
#include "nsMsgFolderFlags.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIDirectoryService.h"

#include "nsIWindowMediator.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIObserverService.h"

#include "nsIMessengerWindowService.h"
#include "prprf.h"
#include "nsIWeakReference.h"

#include "prlog.h"
#if defined(PR_LOGGING)
//
// export NSPR_LOG_MODULES=OSXIntegeration:5
//
static PRLogModuleInfo *gOSXIntegrationLog = nsnull;
#define LOG(args) PR_LOG(gOSXIntegrationLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

#include <Carbon/Carbon.h>

#define kBiffBadgeIcon "mail-biff-badge.png"

nsMessengerOSXIntegration::nsMessengerOSXIntegration()
{
#if defined(PR_LOGGING)
    if (!gOSXIntegrationLog)
        gOSXIntegrationLog = PR_NewLogModule("OSXIntegration");
#endif

  mBiffStateAtom = do_GetAtom("BiffState");

  mBiffIconVisible = PR_FALSE;
  mSuppressBiffIcon = PR_FALSE;
  mAlertInProgress = PR_FALSE;
  NS_NewISupportsArray(getter_AddRefs(mFoldersWithNewMail));
}

nsMessengerOSXIntegration::~nsMessengerOSXIntegration()
{
  if (mBiffIconVisible) 
  {
    RestoreApplicationDockTileImage();
    mBiffIconVisible = PR_FALSE;
  }
}

NS_IMPL_ADDREF(nsMessengerOSXIntegration)
NS_IMPL_RELEASE(nsMessengerOSXIntegration)

NS_INTERFACE_MAP_BEGIN(nsMessengerOSXIntegration)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIFolderListener)
NS_INTERFACE_MAP_END


nsresult
nsMessengerOSXIntegration::Init()
{
  nsresult rv;
    
  nsCOMPtr <nsIMsgAccountManager> accountManager = 
    do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // because we care if the default server changes
  rv = accountManager->AddRootFolderListener(this);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  // because we care if the unread total count changes
  rv = mailSession->AddFolderListener(this, nsIFolderListener::boolPropertyChanged | nsIFolderListener::intPropertyChanged);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyChanged(nsISupports *, nsIAtom *, char const *, char const *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemUnicharPropertyChanged(nsISupports *, nsIAtom *, const PRUnichar *, const PRUnichar *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemRemoved(nsISupports *, nsISupports *, const char *)
{
  return NS_OK;
}

nsresult nsMessengerOSXIntegration::OnAlertFinished(const PRUnichar * aAlertCookie)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIPref> prefService;
  prefService = do_GetService(NS_PREF_CONTRACTID, &rv);  
  PRBool bounceDockIcon = PR_FALSE; 
  
  if (prefService)
    prefService->GetBoolPref("mail.biff.animate_dock_icon", &bounceDockIcon);

   // This will call GetAttention(), which will bounce the dock icon.
  if (!mSuppressBiffIcon)
  {
    nsCOMPtr<nsIWindowMediator> mediator (do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
    if (bounceDockIcon && mediator)
    {
        nsCOMPtr<nsIDOMWindowInternal> domWindow;
        mediator->GetMostRecentWindow(NS_LITERAL_STRING("mail:3pane").get(), getter_AddRefs(domWindow));
        if (domWindow)
        {         
            nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(domWindow));
            chromeWindow->GetAttention();
        }
    }

    // This will change the dock icon.     
    // If we want to overlay the number of new messages on top of
    // the icon ...
    
    // use OverlayApplicationDockTileImage
    // -- you'll have to pass it a CGImage, and somehow we have to
    // create the CGImage with the numbers. tricky.
    
    CFBundleRef appBundle = CFBundleGetMainBundle();
    CGDataProviderRef   provider;
    if (appBundle)
    {
      CFStringRef fileName = CFStringCreateWithCString( NULL, kBiffBadgeIcon, kCFStringEncodingASCII );
      if (fileName)
      {
        CFURLRef url = CFBundleCopyResourceURL( appBundle, fileName, NULL, NULL );
        provider = CGDataProviderCreateWithURL( url );

        CGImageRef iOverlayIcon = NULL;

        iOverlayIcon = CGImageCreateWithPNGDataProvider( provider, NULL, false,  kCGRenderingIntentDefault );
        if (iOverlayIcon)
        {
          OverlayApplicationDockTileImage(iOverlayIcon);
          mBiffIconVisible = PR_TRUE;
        }
        
        CGDataProviderRelease( provider );
        CFRelease( url );
        CFRelease(fileName);
      }
    }
  }

  mSuppressBiffIcon = PR_FALSE;
  mAlertInProgress = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyFlagChanged(nsISupports *item, nsIAtom *property, PRUint32 oldFlag, PRUint32 newFlag)
{
  nsresult rv = NS_OK;
   
  // if we got new mail show a icon in the system tray
	if (mBiffStateAtom == property && mFoldersWithNewMail)
	{
    nsCOMPtr<nsIMsgFolder> folder = do_QueryInterface(item);
    NS_ENSURE_TRUE(folder, NS_OK);

		if (newFlag == nsIMsgFolder::nsMsgBiffState_NewMail) 
    {
      // if the icon is not already visible, only show a system tray icon iff 
      // we are performing biff (as opposed to the user getting new mail)
      if (!mBiffIconVisible)
      {
        PRBool performingBiff = PR_FALSE;
        nsCOMPtr<nsIMsgIncomingServer> server;
        folder->GetServer(getter_AddRefs(server));
        if (server)
          server->GetPerformingBiff(&performingBiff);
        if (!performingBiff) 
          return NS_OK; // kick out right now...
      }
      nsCOMPtr<nsIWeakReference> weakFolder = do_GetWeakReference(folder); 

      // remove the element if it is already in the array....
      PRUint32 count = 0;
      PRUint32 index = 0; 
      mFoldersWithNewMail->Count(&count);
      nsCOMPtr<nsISupports> supports;
      nsCOMPtr<nsIMsgFolder> oldFolder;
      nsCOMPtr<nsIWeakReference> weakReference;
      for (index = 0; index < count; index++)
      {
        supports = getter_AddRefs(mFoldersWithNewMail->ElementAt(index));
        weakReference = do_QueryInterface(supports);
        oldFolder = do_QueryReferent(weakReference);
        if (oldFolder == folder) // if they point to the same folder
          break;
        oldFolder = nsnull;
      }

      if (oldFolder)
        mFoldersWithNewMail->ReplaceElementAt(weakFolder, index);
      else
        mFoldersWithNewMail->AppendElement(weakFolder);
      // now regenerate the tooltip
      OnAlertFinished(nsnull);  
    }
    else if (newFlag == nsIMsgFolder::nsMsgBiffState_NoMail)
    {
      // we are always going to remove the icon whenever we get our first no mail
      // notification. 

      mFoldersWithNewMail->Clear(); 
      if (mBiffIconVisible) 
      {
        RestoreApplicationDockTileImage();
        mBiffIconVisible = PR_FALSE;
      }
    }
  } // if the biff property changed
  
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemAdded(nsISupports *, nsISupports *, const char *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemBoolPropertyChanged(nsISupports *aItem,
                                                         nsIAtom *aProperty,
                                                         PRBool aOldValue,
                                                         PRBool aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemEvent(nsIMsgFolder *, nsIAtom *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemIntPropertyChanged(nsISupports *aItem, nsIAtom *aProperty, PRInt32 aOldValue, PRInt32 aNewValue)
{
  nsresult rv;
  return NS_OK;
}
