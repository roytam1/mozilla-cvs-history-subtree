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
 * The Original Code is Software KeyBoard
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#ifdef WINCE
#include "windows.h"
#include "aygshell.h"
#endif

#include "memory.h"
#include "stdlib.h"

#include "nspr.h"

#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsString.h"
#include "nsArray.h"

#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIWindowWatcher.h"
#include "nsIDOMWindow.h"
#include "nsICategoryManager.h"

#include "nsIDOMWindow.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMEventGroup.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMEventReceiver.h"

#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIContent.h"
#include "nsIFormControl.h"

#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventListener.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

#include "nsITimer.h"

#include "nsISoftKeyBoard.h"


#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentView.h"    
#include "nsIDOMAbstractView.h" 
#include "nsIDOMWindowInternal.h"  
#include "nsPIDOMWindow.h"      
#include "nsIPresShell.h"    
#include "nsIDOMWindowInternal.h"  
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsIDocShell.h"

static PRBool gUseSoftwareKeyboard;

class nsSoftKeyBoard : public nsIDOMEventListener
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  nsSoftKeyBoard(class nsSoftKeyBoardService* aService);

  nsresult Init(nsIDOMWindow *aWindow);
  nsresult Shutdown();
  nsresult GetAttachedWindow(nsIDOMWindow * *aAttachedWindow);

private:
  ~nsSoftKeyBoard();
  PRBool ShouldOpenKeyboardFor(nsIDOMEvent* aEvent);

  nsCOMPtr<nsIDOMWindow> mTopWindow;
  class nsSoftKeyBoardService* mService;

  enum
  {
    eNumbers = 0,
    eLowerCase = 1,
    eUpperCase = 2
  };

  PRUint32 mUsage; 

  PRUint32 mCurrentDigit;
  PRInt32 mCurrentDigitCount;
  nsCOMPtr<nsITimer> mTimer;
};


class nsSoftKeyBoardService: public nsIObserver, public nsISoftKeyBoard
{
public:  
  nsSoftKeyBoardService();  
  virtual ~nsSoftKeyBoardService();  
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSISOFTKEYBOARD

  nsCOMArray<nsSoftKeyBoard> mObjects;

  static void CloseSIP();
  static void DelayCloseSIP();
  static void DoDelayCloseSIPCallback(nsITimer *aTimer, void *aClosure);

  static void OpenSIP();
  static void ScrollElementIntoView(nsIContent *aContent);

  void HandlePref(const char* pref, nsIPrefBranch2* prefBranch);

  static nsCOMPtr<nsITimer> gTimer;
  static nsCOMPtr<nsITimer> gScrollTimer;
};

NS_INTERFACE_MAP_BEGIN(nsSoftKeyBoard)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsSoftKeyBoard)
NS_IMPL_RELEASE(nsSoftKeyBoard)


nsSoftKeyBoard::nsSoftKeyBoard(nsSoftKeyBoardService* aService)
{
  NS_ASSERTION(aService, "Should not create this object without a valid service");

  mService = aService; // back pointer -- no reference

  mCurrentDigit = 0;
  mCurrentDigitCount = 0;
  mUsage = eLowerCase;

  nsSoftKeyBoardService::CloseSIP();
}

nsSoftKeyBoard::~nsSoftKeyBoard()
{
}

NS_IMETHODIMP
nsSoftKeyBoard::HandleEvent(nsIDOMEvent* aEvent)
{
  if (!aEvent)
    return NS_OK;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

  nsCOMPtr<nsIDOMEventTarget> target;
  nsevent->GetOriginalTarget(getter_AddRefs(target));
  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);


  if (!targetContent || !targetContent->IsContentOfType(nsIContent::eHTML_FORM_CONTROL))
    return NS_OK;

  nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(targetContent));
  if (!formControl)
    return NS_OK;


  PRInt32 controlType = formControl->GetType();
      
  if (controlType != NS_FORM_TEXTAREA       &&  controlType != NS_FORM_INPUT_TEXT &&
      controlType != NS_FORM_INPUT_PASSWORD &&  controlType != NS_FORM_INPUT_FILE) 
  {
    return NS_OK;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);

  if (eventType.EqualsLiteral("keypress"))
  {
    PRUint32 keyCode;
    nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aEvent));
    
    if (!keyEvent)
      return NS_OK;
    
    if (NS_FAILED(keyEvent->GetKeyCode(&keyCode)))
      return NS_OK;
    
    if (keyCode == nsIDOMKeyEvent::DOM_VK_RETURN && controlType != NS_FORM_TEXTAREA)
    {
      nsSoftKeyBoardService::CloseSIP();
    }

    return NS_OK;
  }

  if (eventType.EqualsLiteral("click"))
  {
    nsSoftKeyBoardService::OpenSIP();
    nsSoftKeyBoardService::ScrollElementIntoView(targetContent);
    return NS_OK;
  }

  PRBool popupConditions = PR_FALSE;
  nsCOMPtr<nsPIDOMWindow> privateWindow = do_QueryInterface(mTopWindow);
  if (!privateWindow)
    return NS_OK;

  nsIDOMWindowInternal *rootWindow = privateWindow->GetPrivateRoot();
  if (!rootWindow)
    return NS_OK;

  nsCOMPtr<nsIDOMWindow> windowContent;
  rootWindow->GetContent(getter_AddRefs(windowContent));
  privateWindow = do_QueryInterface(windowContent);

  if (privateWindow)
    popupConditions = privateWindow->IsLoadingOrRunningTimeout();
  
  if (eventType.EqualsLiteral("focus"))
  {
    //    if (popupConditions == PR_FALSE)
    nsSoftKeyBoardService::OpenSIP();
    nsSoftKeyBoardService::ScrollElementIntoView(targetContent);
  }
  else
    nsSoftKeyBoardService::CloseSIP();
  
  return NS_OK;
}

nsCOMPtr<nsITimer> nsSoftKeyBoardService::gTimer = nsnull;
nsCOMPtr<nsITimer> nsSoftKeyBoardService::gScrollTimer = nsnull;


static void ScrollElementTimerCB(nsITimer *aTimer, void *aClosure)
{
  nsIContent* content = (nsIContent*) aClosure;

  nsIDocument* doc = content->GetDocument();
  if (!doc)
    return;
  
  nsIFrame* frame;
  nsIPresShell *presShell = doc->GetShellAt(0);

  presShell->GetPrimaryFrameFor(content, &frame);
  presShell->ScrollFrameIntoView(frame, NS_PRESSHELL_SCROLL_ANYWHERE, NS_PRESSHELL_SCROLL_ANYWHERE);

  content->Release();
}

void
nsSoftKeyBoardService::ScrollElementIntoView(nsIContent *content)
{
  NS_ADDREF(content);  // release in timercb.
  nsSoftKeyBoardService::gScrollTimer = do_CreateInstance("@mozilla.org/timer;1");
  nsSoftKeyBoardService::gScrollTimer->InitWithFuncCallback(ScrollElementTimerCB,
                                                            (void*)content,
                                                            250, 
                                                            nsITimer::TYPE_ONE_SHOT);
}



void
nsSoftKeyBoardService::OpenSIP()
{
  if (!gUseSoftwareKeyboard)
    return;

  if (gTimer)
  {
    gTimer->Cancel();
    gTimer = nsnull;
  }

#ifdef WINCE
  HWND hWndSIP = ::FindWindow( _T( "SipWndClass" ), NULL );
  if (hWndSIP)
    ::ShowWindow( hWndSIP, SW_SHOW);

  SHSipPreference(NULL, SIP_UP);

  //  SHFullScreen(GetForegroundWindow(), SHFS_SHOWSIPBUTTON);

  // keep the sip button hidden!
  
  hWndSIP = FindWindow( _T( "MS_SIPBUTTON" ), NULL );
  if (hWndSIP) 
  {
    ShowWindow( hWndSIP, SW_HIDE );
    SetWindowPos(hWndSIP, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
  }

#endif

  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->NotifyObservers(nsnull, "software-keyboard", NS_LITERAL_STRING("open").get());
}


void
nsSoftKeyBoardService::DoDelayCloseSIPCallback(nsITimer *aTimer, void *aClosure)
{
  nsSoftKeyBoardService::CloseSIP();
  nsSoftKeyBoardService::gTimer = nsnull;
}

void
nsSoftKeyBoardService::DelayCloseSIP()
{
  if (nsSoftKeyBoardService::gTimer)
  {
    nsSoftKeyBoardService::gTimer->Cancel();
    nsSoftKeyBoardService::gTimer = nsnull;
  }

  nsSoftKeyBoardService::gTimer = do_CreateInstance("@mozilla.org/timer;1");

  if (!nsSoftKeyBoardService::gTimer)
    return;

  nsSoftKeyBoardService::gTimer->InitWithFuncCallback(nsSoftKeyBoardService::DoDelayCloseSIPCallback,
                                                      nsnull,
                                                      250,
                                                      nsITimer::TYPE_ONE_SHOT);
}

void
nsSoftKeyBoardService::CloseSIP()
{
#ifdef WINCE

  HWND hWndSIP = FindWindow( _T( "SipWndClass" ), NULL );
  if (hWndSIP)
  {
    ShowWindow( hWndSIP, SW_HIDE );
  }

  hWndSIP = FindWindow( _T( "MS_SIPBUTTON" ), NULL );
  if (hWndSIP) 
  {
    ShowWindow( hWndSIP, SW_HIDE );
    SetWindowPos(hWndSIP, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
  }

  SHSipPreference(NULL, SIP_DOWN);

  SHFullScreen(GetForegroundWindow(), SHFS_HIDESIPBUTTON);
#endif

  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->NotifyObservers(nsnull, "software-keyboard", NS_LITERAL_STRING("close").get());

}

NS_IMETHODIMP
nsSoftKeyBoardService::Show(void)
{
  nsSoftKeyBoardService::OpenSIP();
  return NS_OK;
}

NS_IMETHODIMP
nsSoftKeyBoardService::Hide(void)
{
  nsSoftKeyBoardService::CloseSIP();
  return NS_OK;
}


NS_IMETHODIMP
nsSoftKeyBoardService::GetWindowRect(PRInt32 *top, PRInt32 *bottom, PRInt32 *left, PRInt32 *right)
{
#ifdef WINCE
  if (!top || !bottom || !left || !right)
    return NS_ERROR_INVALID_ARG;

  HWND hWndSIP = ::FindWindow( _T( "SipWndClass" ), NULL );
  if (!hWndSIP)
    return NS_ERROR_NULL_POINTER;

  RECT rect;
  BOOL b = ::GetWindowRect(hWndSIP, &rect);
  if (!b)
    return NS_ERROR_UNEXPECTED;

  *top = rect.top;
  *bottom = rect.bottom;
  *left = rect.left;
  *right = rect.right;

  return NS_OK;
#endif

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSoftKeyBoardService::SetWindowRect(PRInt32 top, PRInt32 bottom, PRInt32 left, PRInt32 right)
{
#ifdef WINCE
  HWND hWndSIP = ::FindWindow( _T( "SipWndClass" ), NULL );
  if (!hWndSIP)
    return NS_ERROR_NULL_POINTER;
  
  SetWindowPos(hWndSIP, HWND_TOP, left, top, right-left, bottom-top, SWP_SHOWWINDOW);
  return NS_OK;
#endif
 

 return NS_ERROR_NOT_IMPLEMENTED;
}


PRBool
nsSoftKeyBoard::ShouldOpenKeyboardFor(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMEventTarget> target;
  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));
  nsevent->GetOriginalTarget(getter_AddRefs(target));
  nsCOMPtr<nsIContent> targetContent = do_QueryInterface(target);

  if (targetContent && targetContent->IsContentOfType(nsIContent::eHTML_FORM_CONTROL)) 
  {
    nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(targetContent));
    if (formControl)
    {
      PRInt32 controlType = formControl->GetType();
      
      if (controlType == NS_FORM_TEXTAREA ||
          controlType == NS_FORM_INPUT_TEXT ||
          controlType == NS_FORM_INPUT_PASSWORD ||
          controlType == NS_FORM_INPUT_FILE) 
      {
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}


nsresult
nsSoftKeyBoard::Init(nsIDOMWindow *aWindow)
{
  mTopWindow = aWindow;

  nsCOMPtr<nsPIDOMWindow> privateWindow = do_QueryInterface(aWindow);
  
  if (!privateWindow)
    return NS_ERROR_UNEXPECTED;
  
  nsIChromeEventHandler *chromeEventHandler = privateWindow->GetChromeEventHandler();
  nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(chromeEventHandler));
  if (!receiver)
    return NS_ERROR_UNEXPECTED;
  
  receiver->AddEventListener(NS_LITERAL_STRING("focus"), this, PR_TRUE);
  receiver->AddEventListener(NS_LITERAL_STRING("blur"), this, PR_TRUE);
  receiver->AddEventListener(NS_LITERAL_STRING("keypress"), this, PR_TRUE);

  receiver->AddEventListener(NS_LITERAL_STRING("click"), this, PR_TRUE);
  
  return NS_OK;
}

nsresult
nsSoftKeyBoard::Shutdown()
{
  nsCOMPtr<nsPIDOMWindow> privateWindow = do_QueryInterface(mTopWindow);
  
  if (!privateWindow)
    return NS_ERROR_UNEXPECTED;
  
  nsIChromeEventHandler *chromeEventHandler = privateWindow->GetChromeEventHandler();
  nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(chromeEventHandler));
  if (!receiver)
    return NS_ERROR_UNEXPECTED;

  receiver->RemoveEventListener(NS_LITERAL_STRING("focus"), this, PR_TRUE);
  receiver->RemoveEventListener(NS_LITERAL_STRING("blur"), this, PR_TRUE);
  receiver->RemoveEventListener(NS_LITERAL_STRING("keypress"), this, PR_TRUE);
  receiver->RemoveEventListener(NS_LITERAL_STRING("click"), this, PR_TRUE);

  mTopWindow = nsnull;

  return NS_OK;
}

nsresult 
nsSoftKeyBoard::GetAttachedWindow(nsIDOMWindow * *aAttachedWindow)
{
  NS_IF_ADDREF(*aAttachedWindow = mTopWindow);
  return NS_OK;
}

nsSoftKeyBoardService::nsSoftKeyBoardService()  
{
  gUseSoftwareKeyboard = PR_FALSE;
}  

nsSoftKeyBoardService::~nsSoftKeyBoardService()  
{
}  

NS_IMPL_ISUPPORTS2(nsSoftKeyBoardService, nsIObserver, nsISoftKeyBoard)

void
nsSoftKeyBoardService::HandlePref(const char* pref, nsIPrefBranch2* prefBranch)
{
  if (!strcmp(pref, "skey.enabled"))
  {
    PRBool enabled;
    prefBranch->GetBoolPref(pref, &enabled);
    gUseSoftwareKeyboard = enabled;
  }
}

NS_IMETHODIMP
nsSoftKeyBoardService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
  nsresult rv;

  if (!strcmp(aTopic,"domwindowopened")) 
  {
    nsCOMPtr<nsIDOMWindow> chromeWindow = do_QueryInterface(aSubject);
    
    nsSoftKeyBoard* sn = new nsSoftKeyBoard(this);

    if (!sn)
      return NS_ERROR_OUT_OF_MEMORY;

    sn->Init(chromeWindow);
    
    mObjects.AppendObject(sn);  // the array owns the only reference to sn.

    return NS_OK;
  }
  
  if (!strcmp(aTopic,"domwindowclosed")) 
  {
    nsCOMPtr<nsIDOMWindow> chromeWindow = do_QueryInterface(aSubject);
    // need to find it in our array
  
    PRInt32 count = mObjects.Count();
    for (PRInt32 i = 0; i < count; i++)
    {
      nsSoftKeyBoard* sn = mObjects[i];
      nsCOMPtr<nsIDOMWindow> attachedWindow;
      sn->GetAttachedWindow(getter_AddRefs(attachedWindow));

      if (attachedWindow == chromeWindow) 
      {
        sn->Shutdown();
        mObjects.RemoveObjectAt(i);
        return NS_OK;
      }
    }
    return NS_OK;
  }
  
  if (!strcmp(aTopic,"xpcom-startup")) 
  {
    nsCOMPtr<nsIWindowWatcher> windowWatcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    windowWatcher->RegisterNotification(this);

    nsCOMPtr<nsIPrefBranch2> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    prefBranch->AddObserver("skey.", this, PR_FALSE);

    HandlePref("skey.enabled", prefBranch);
    return NS_OK;
  }

  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) 
  {
    nsCOMPtr<nsIPrefBranch2> prefBranch = do_QueryInterface(aSubject);
    nsXPIDLCString cstr;
    
    const char* pref = NS_ConvertUCS2toUTF8(aData).get();

    HandlePref(pref, prefBranch);
    return NS_OK;
  }
  return NS_OK;
}


//------------------------------------------------------------------------------
//  XPCOM REGISTRATION BELOW
//------------------------------------------------------------------------------

#define SoftKeyBoard_CID \
{ 0x46f2dcf5, 0x25a1, 0x4459, \
{0xa6, 0x30, 0x13, 0x53, 0x9f, 0xe5, 0xa8, 0x6b} }

#define SoftKeyBoard_ContractID "@mozilla.org/softkeyboard;1"

#define SoftKeyBoardService_CID \
{ 0xde3dc3a1, 0x420f, 0x4cec, \
{0x9c, 0x3d, 0xf7, 0x71, 0xab, 0x22, 0xae, 0xf7} }

#define SoftKeyBoardService_ContractID "@mozilla.org/softkbservice/service;1"


static NS_METHOD SoftKeyBoardServiceRegistration(nsIComponentManager *aCompMgr,
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
  
  char* previous = nsnull;
  rv = catman->AddCategoryEntry("xpcom-startup",
                                "SoftKeyBoardService", 
                                SoftKeyBoardService_ContractID,
                                PR_TRUE, 
                                PR_TRUE, 
                                &previous);
  if (previous)
    nsMemory::Free(previous);
  
  return rv;
}

static NS_METHOD SoftKeyBoardServiceUnregistration(nsIComponentManager *aCompMgr,
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
                                   "SoftKeyBoardService", 
                                   PR_TRUE);
  
  return rv;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSoftKeyBoardService)



static const nsModuleComponentInfo components[] =
{
  { "SoftKeyBoardService", 
    SoftKeyBoardService_CID, 
    SoftKeyBoardService_ContractID,
    nsSoftKeyBoardServiceConstructor,
    SoftKeyBoardServiceRegistration,
    SoftKeyBoardServiceUnregistration
  }
};

NS_IMPL_NSGETMODULE(SoftKeyBoardModule, components)
