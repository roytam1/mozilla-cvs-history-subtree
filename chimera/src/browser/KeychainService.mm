
/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *  Benoit Foucher <bfoucher@mac.com>
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

#import "NSString+Utils.h"
#import "KeychainService.h"
#import "CHBrowserService.h"
#import "PreferenceManager.h"

#import <CoreServices/CoreServices.h>
#import <Carbon/Carbon.h>

#include "nsIPref.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIWindowWatcher.h"
#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIURL.h"
#include "nsIDOMWindowCollection.h"
#include "nsIContent.h"
#include "nsIWindowWatcher.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"


nsresult
FindUsernamePasswordFields(nsIDOMHTMLFormElement* inFormElement, nsIDOMHTMLInputElement** outUsername,
                            nsIDOMHTMLInputElement** outPassword, PRBool inStopWhenFound);


@implementation KeychainService

static KeychainService *sInstance = nil;
static const char* const gUseKeychainPref = "chimera.store_passwords_with_keychain";
static const char* const gAutoFillEnabledPref = "chimera.keychain_passwords_autofill";

int KeychainPrefChangedCallback(const char* pref, void* data);

//
// KeychainPrefChangedCallback
//
// Pref callback to tell us when the pref values for using the keychain
// have changed. We need to re-cache them at that time.
//
int KeychainPrefChangedCallback(const char* inPref, void* unused)
{
  BOOL success = NO;
  if ( strcmp(inPref, gUseKeychainPref) == 0 )
    [KeychainService instance]->mIsEnabled = [[PreferenceManager sharedInstance] getBooleanPref:gUseKeychainPref withSuccess:&success];
  else if ( strcmp(inPref, gAutoFillEnabledPref) == 0)
    [KeychainService instance]->mIsAutoFillEnabled = [[PreferenceManager sharedInstance] getBooleanPref:gAutoFillEnabledPref withSuccess:&success];
  
  return NS_OK;
}


+ (KeychainService*) instance
{
  return sInstance ? sInstance : sInstance = [[self alloc] init];
}

- (id) init
{
  if ( (self = [super init]) ) {
    // Add a new form submit observer. We explicitly hold a ref in case the
    // observer service uses a weakref.
    nsCOMPtr<nsIObserverService> svc = do_GetService("@mozilla.org/observer-service;1");
    NS_ASSERTION(svc, "Keychain can't get observer service");
    mFormSubmitObserver = new KeychainFormSubmitObserver(self);
    if ( mFormSubmitObserver && svc ) {
      NS_ADDREF(mFormSubmitObserver);
      svc->AddObserver(mFormSubmitObserver, NS_FORMSUBMIT_SUBJECT, PR_FALSE);
    }
  
    // register for the cocoa notification posted when XPCOM shutdown so we
    // can unregister the pref callbacks we register below
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(shutdown:) name:@"XPCOM Shutdown"
      object:nil];
    
    // cache the values of the prefs and register pref-changed callbacks. Yeah, I know
    // nsIPref is obsolete, but i'm not about to create an nsIObserver just for this.
    mIsEnabled = NO;
    mIsAutoFillEnabled = NO;
    nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID));
    if ( pref ) {
      BOOL success = NO;
      mIsEnabled = [[PreferenceManager sharedInstance] getBooleanPref:gUseKeychainPref withSuccess:&success];
      mIsAutoFillEnabled = [[PreferenceManager sharedInstance] getBooleanPref:gAutoFillEnabledPref withSuccess:&success];
      if ( pref ) {
        pref->RegisterCallback(gUseKeychainPref, KeychainPrefChangedCallback, nsnull);
        pref->RegisterCallback(gAutoFillEnabledPref, KeychainPrefChangedCallback, nsnull);
      }
    }
  }
  return self;
}

- (void) dealloc
{
  // unregister for shutdown notification. It may have already happened, but just in case.
  NS_IF_RELEASE(mFormSubmitObserver);
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  
  [super dealloc];
}


//
// shutdown:
//
// Called in response to the cocoa notification "XPCOM Shutdown" sent by the cocoa
// browser service before it terminates embedding and shuts down xpcom. Allows us
// to get rid of anything we're holding onto for the length of the app.
//
- (void) shutdown:(id)unused
{
  // unregister ourselves as listeners of prefs before prefs go away
  nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID));
  if ( pref ) {
    pref->UnregisterCallback(gUseKeychainPref, KeychainPrefChangedCallback, nsnull);
    pref->UnregisterCallback(gAutoFillEnabledPref, KeychainPrefChangedCallback, nsnull);
  }
}


- (BOOL) isEnabled
{
  return mIsEnabled;
}

- (BOOL) isAutoFillEnabled
{
  return mIsAutoFillEnabled;
}

- (BOOL) getUsernameAndPassword:(NSString*)realm user:(NSMutableString*)username password:(NSMutableString*)pwd
{
  const int kBufferLen = 255;
  OSStatus status;

  char buffer[kBufferLen];
  UInt32 actualSize;
  KCItemRef itemRef;

  if(kcfindinternetpassword([realm cString], 0, 0, kAnyPort, kKCProtocolTypeHTTP, kKCAuthTypeHTTPDigest, 
                            kBufferLen, buffer, &actualSize, &itemRef) != noErr)
    return false;

  //
  // Set password and username
  //
  [pwd setString:[NSString stringWithCString:buffer length:actualSize]];

  KCAttribute attr;
  attr.tag = kAccountKCItemAttr;
  attr.length = kBufferLen;
  attr.data = buffer;
  status = KCGetAttribute( itemRef, &attr, &actualSize );
  
  [username setString:[NSString stringWithCString:buffer length:actualSize]];
  
  return true;
}

- (BOOL) findUsernameAndPassword:(NSString*)realm
{
    return [self getUsernameAndPassword:realm user:[NSMutableString string] password:[NSMutableString string]];
}

- (void) updateUsernameAndPassword:(NSString*)realm user:(NSString*)username password:(NSString*)pwd
{
  const int kBufferLen = 255;
  OSStatus status;

  KCItemRef itemRef;
  char buffer[kBufferLen];
  UInt32 actualSize;

  if(kcfindinternetpassword([realm cString], 0, 0, kAnyPort, kKCProtocolTypeHTTP, kKCAuthTypeHTTPDigest, 
                            kBufferLen, buffer, &actualSize, &itemRef) != noErr)
    return;

  KCAttribute attr;;

  //
  // Update item username and password.
  //
  attr.tag = kAccountKCItemAttr;
  attr.length = [username length];
  attr.data = (char*)[username cString];
  status = KCSetAttribute(itemRef, &attr);
  if(status != noErr)
    NSLog(@"Couldn't update keychain item account");

  status = KCSetData(itemRef, [pwd length], [pwd cString]);
  if(status != noErr)
    NSLog(@"Couldn't update keychain item data");

  //
  // Update the item now.
  //
  status = KCUpdateItem(itemRef);
  if(status != noErr)
    NSLog(@"Couldn't update keychain item");
}

- (void) storeUsernameAndPassword:(NSString*)realm user:(NSString*)username password:(NSString*)pwd
{
  kcaddinternetpassword([realm cString], 0, [username cString], kAnyPort, kKCProtocolTypeHTTP, kKCAuthTypeHTTPDigest,
                        [pwd length], [pwd cString], 0);
}

- (void) removeUsernameAndPassword:(NSString*)realm
{
  const int kBufferLen = 255;
  char buffer[kBufferLen];
  UInt32 actualSize;
  KCItemRef itemRef;

  if(kcfindinternetpassword([realm cString], 0, 0, kAnyPort, kKCProtocolTypeHTTP, kKCAuthTypeHTTPDigest, 
                            kBufferLen, buffer, &actualSize, &itemRef) == noErr)
  {
    KCDeleteItem(itemRef);
  }
}

//
// addListenerToView:
//
// Add a listener to the view to auto fill username and passwords
// fields if the values are stored in the Keychain.
//
- (void) addListenerToView:(CHBrowserView*)view
{
  [view addListener:[[[KeychainBrowserListener alloc] initWithBrowser:self browser:view] autorelease]];
}

@end


//
// Keychain prompt implementation.
// 
NS_IMPL_ISUPPORTS2(KeychainPrompt,
                   nsIAuthPrompt,
                   nsIAuthPromptWrapper)

KeychainPrompt::KeychainPrompt() 
  : mKeychain([KeychainService instance])
{
  NS_INIT_REFCNT();
}

KeychainPrompt::KeychainPrompt(KeychainService* keychain) : mKeychain(keychain)
{
  NS_INIT_REFCNT();
}

KeychainPrompt::~KeychainPrompt()
{
  // NSLog(@"Keychain prompt died.");
}

void
KeychainPrompt::PreFill(const PRUnichar *realm, PRUnichar **user, PRUnichar **pwd)
{
  if(![mKeychain isEnabled] || ![mKeychain isAutoFillEnabled])
    return;

  //
  // TODO: add support for ftp username/password. The given realm for
  // an ftp server has the form ftp://<username>@<server>/<path>, see
  // netwerk/protocol/ftp/src/nsFtpConnectionThread.cpp.
  //
  // Get server name from the password realm ("hostname:port (realm)",
  // see netwerk/protocol/http/src/nsHttpChannel.cpp)
  //
  NSString* passwordRealm = [NSString stringWithPRUnichars:realm];
  NSRange r = [passwordRealm rangeOfString:@":"];
  if(r.location != NSNotFound)
    passwordRealm = [passwordRealm substringToIndex:r.location];

  NSMutableString* username = nil;
  NSMutableString* password = nil;
  if ( user )
    username = [NSMutableString stringWithCharacters:*user length:(*user ? nsCRT::strlen(*user) : 0)];
  if ( pwd )
  	password = [NSMutableString stringWithCharacters:*pwd length:(*pwd ? nsCRT::strlen(*pwd) : 0)];

  //
  // Pre-fill user/password if found in the keychain.
  //
  if([mKeychain getUsernameAndPassword:passwordRealm user:username password:password]) {
    if ( user )
      *user = [username createNewUnicodeBuffer];
    if ( pwd )
      *pwd = [password createNewUnicodeBuffer];
  }
}

void
KeychainPrompt::ProcessPrompt(const PRUnichar* realm, bool checked, PRUnichar* user, PRUnichar *pwd)
{
  //
  // Get server name from the password realm ("hostname:port (realm)",
  // see nsHttpChannel.cpp)
  //
  NSString* passwordRealm = [NSString stringWithPRUnichars:realm];
  NSRange r = [passwordRealm rangeOfString:@":"];
  if(r.location != NSNotFound)
    passwordRealm = [passwordRealm substringToIndex:r.location];

  NSString* username = [NSString stringWithPRUnichars:user];
  NSString* password = [NSString stringWithPRUnichars:pwd];

  NSMutableString* origUsername = [NSMutableString string];
  NSMutableString* origPwd = [NSMutableString string];

  bool found = [mKeychain getUsernameAndPassword:passwordRealm user:origUsername password:origPwd];

  //
  // Update, store or remove the user/password depending on the user
  // choice and whether or not we found the username/password in the
  // keychain.
  //
  if(checked && !found) {
    [mKeychain storeUsernameAndPassword:passwordRealm user:username password:password];
  }
  else if(checked && found && (![origUsername isEqualToString:username] || ![origPwd isEqualToString:password])) {
    [mKeychain updateUsernameAndPassword:passwordRealm user:username password:password];
  }
  else if(!checked && found) {
    [mKeychain removeUsernameAndPassword:passwordRealm];
  }
}

//
// Implementation of nsIAuthPrompt
//
NS_IMETHODIMP
KeychainPrompt::Prompt(const PRUnichar *dialogTitle, 
                        const PRUnichar *text, 
                        const PRUnichar *passwordRealm, 
                        PRUint32 savePassword,
                        const PRUnichar *defaultText, 
                        PRUnichar **result, 
                        PRBool *_retval)
{
  if (defaultText)
    *result = ToNewUnicode(nsDependentString(defaultText));

  mPrompt->Prompt(dialogTitle, text, result, nsnull, nsnull, _retval);

  return NS_OK;
}

NS_IMETHODIMP
KeychainPrompt::PromptUsernameAndPassword(const PRUnichar *dialogTitle,
                                            const PRUnichar *text, 
                                            const PRUnichar *realm,
                                            PRUint32 savePassword, 
                                            PRUnichar **user, 
                                            PRUnichar **pwd, 
                                            PRBool *_retval)
{
  PreFill(realm, user, pwd);

  PRBool checked = [mKeychain isEnabled];
  PRUnichar* checkTitle = [NSLocalizedString(@"KeychainCheckTitle", @"") createNewUnicodeBuffer];

  nsresult rv = mPrompt->PromptUsernameAndPassword(dialogTitle, text, user, pwd, checkTitle, &checked, _retval);
  if ( checkTitle )
    nsMemory::Free(checkTitle);
  if (NS_FAILED(rv))
    return rv;
  
  if(*_retval)
    ProcessPrompt(realm, checked, *user, *pwd);

  return NS_OK;
}

NS_IMETHODIMP
KeychainPrompt::PromptPassword(const PRUnichar *dialogTitle,
                                const PRUnichar *text, 
                                const PRUnichar *realm,
                                PRUint32 savePassword, 
                                PRUnichar **pwd, 
                                PRBool *_retval)
{
  PreFill(realm, nsnull, pwd);

  PRBool checked = [mKeychain isEnabled];
  PRUnichar* checkTitle = [NSLocalizedString(@"KeychainCheckTitle", @"") createNewUnicodeBuffer];

  nsresult rv = mPrompt->PromptPassword(dialogTitle, text, pwd, checkTitle, &checked, _retval);
  if ( checkTitle )
    nsMemory::Free(checkTitle);
  if (NS_FAILED(rv))
    return rv;
  
  if(*_retval)
    ProcessPrompt(realm, checked, nsnull, *pwd);

  return NS_OK;
}

NS_IMETHODIMP
KeychainPrompt::SetPromptDialogs(nsIPrompt* dialogs)
{
  mPrompt = dialogs;
  return NS_OK;
}

//
// Keychain form submit observer implementation.
//
NS_IMPL_ISUPPORTS2(KeychainFormSubmitObserver,
                   nsIObserver,
                   nsIFormSubmitObserver)

KeychainFormSubmitObserver::KeychainFormSubmitObserver(KeychainService* keychain) : mKeychain(keychain)
{
  NS_INIT_REFCNT();
  //NSLog(@"Keychain form submit observer created.");
}

KeychainFormSubmitObserver::~KeychainFormSubmitObserver()
{
  //NSLog(@"Keychain form submit observer died.");
}

NS_IMETHODIMP
KeychainFormSubmitObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData) 
{
  return NS_OK;
}

NS_IMETHODIMP 
KeychainFormSubmitObserver::Notify(nsIContent* node, nsIDOMWindowInternal* window, nsIURI* actionURL, 
                                    PRBool* cancelSubmit)
{
  if (![mKeychain isEnabled])
    return NS_OK;

  nsCOMPtr<nsIDOMHTMLFormElement> formNode(do_QueryInterface(node));
  if (!formNode)
    return NS_OK;

  // seek out the username and password fields. If there are two password fields, we don't
  // want to do anything since it's probably not a signin form. Passing PR_FALSE in the last
  // param will cause FUPF to look for multiple password fields and then bail if that is the 
  // case, seting |passwordElement| to nsnull.
  nsCOMPtr<nsIDOMHTMLInputElement> usernameElement, passwordElement;
  nsresult rv = FindUsernamePasswordFields(formNode, getter_AddRefs(usernameElement),  getter_AddRefs(passwordElement),
                                           PR_FALSE);

  if (NS_SUCCEEDED(rv) && usernameElement && passwordElement) {
  
    // extract username and password from the fields
    nsAutoString uname, pword;
    usernameElement->GetValue(uname);
    passwordElement->GetValue(pword);
    NSString* username = [NSString stringWith_nsAString:uname];
    NSString* password = [NSString stringWith_nsAString:pword];
    
    nsCOMPtr<nsIDocument> doc;
    node->GetDocument(*getter_AddRefs(doc));
    if (!doc)
      return NS_OK;
    
    nsCOMPtr<nsIURI> docURL;
    rv = doc->GetDocumentURL(getter_AddRefs(docURL));
    if (NS_FAILED(rv) || !docURL)
      return NS_OK;

    nsCAutoString hostPort;
    docURL->GetHostPort(hostPort);

    //
    // If there's already an entry in the keychain, do nothing. If
    // there's no entry ask the user if he wants to store the
    // password in the keychain and eventually store it.
    //
    // TODO: also check if the values are different from the one
    // stored in the keychain and ask the user if he wants to update
    // the keychain if that's the case.
    //
    NSString* realm = [NSString stringWithCString:hostPort.get()];
    if (![mKeychain findUsernameAndPassword:realm] && CheckConfirmYN(window))
      [mKeychain storeUsernameAndPassword:realm user:username password:password];
  }

  return NS_OK;
}

BOOL
KeychainFormSubmitObserver::CheckConfirmYN(nsIDOMWindowInternal* window)
{
  //
  // TODO: Refactor: Getting the NSWindow for the nsIDOMWindowInternal
  // is already implemented in CocoaPromptService.
  //
  nsCOMPtr<nsIWindowWatcher> watcher(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (!watcher) {
    return nsnull;
  }

  nsCOMPtr<nsIWebBrowserChrome> chrome;
  watcher->GetChromeForWindow(window, getter_AddRefs(chrome));
  if (!chrome) {
    return nsnull;
  }

  nsCOMPtr<nsIEmbeddingSiteWindow> siteWindow(do_QueryInterface(chrome));
  if (!siteWindow) {
    return nsnull;
  }

  NSWindow* nswindow;
  nsresult rv = siteWindow->GetSiteWindow((void**)&nswindow);
  if (NS_FAILED(rv))
    return nsnull;

  nsAlertController* dialog = CHBrowserService::GetAlertController();
  return [dialog confirmStorePassword:nswindow];
}

@implementation KeychainBrowserListener

- (id)initWithBrowser:(KeychainService*)keychain browser:(CHBrowserView*)aBrowser
{
  if ( (self = [super init]) ) {
    mKeychain = keychain;
    mBrowserView = aBrowser;
  }
  return self;
}

- (void) dealloc
{
  // NSLog(@"Keychain browser listener died.");
  [super dealloc];
}

- (void)onLoadingCompleted:(BOOL)succeeded;
{
  if(!succeeded)
    return;

  if(![mKeychain isEnabled] || ![mKeychain isAutoFillEnabled])
    return;

  nsCOMPtr<nsIDOMWindow> domWin = getter_AddRefs([mBrowserView getContentWindow]);
  if (!domWin)
    return;
  nsCOMPtr<nsIDOMDocument> domDoc;
  domWin->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc ( do_QueryInterface(domDoc) );
  if (!doc) {
    NS_ASSERTION(0, "no document available");
    return;
  }
  
  nsCOMPtr<nsIDOMHTMLDocument> htmldoc(do_QueryInterface(doc));
  if (!htmldoc)
    return;

  nsCOMPtr<nsIDOMHTMLCollection> forms;
  nsresult rv = htmldoc->GetForms(getter_AddRefs(forms));
  if (NS_FAILED(rv) || !forms) 
    return;

  PRUint32 numForms;
  forms->GetLength(&numForms);

  //
  // Seek out username and password element in all forms. If found in
  // a form, check the keychain to see if the username passowrd are
  // stored and prefill the elements.
  //
  for (PRUint32 formX = 0; formX < numForms; formX++) {

    nsCOMPtr<nsIDOMNode> formNode;
    rv = forms->Item(formX, getter_AddRefs(formNode));
    if (NS_FAILED(rv) || formNode == nsnull)
      continue;

    // search the current form for the text fields
    nsCOMPtr<nsIDOMHTMLFormElement> formElement(do_QueryInterface(formNode));
    if (!formElement) 
      continue;
    nsCOMPtr<nsIDOMHTMLInputElement> usernameElement, passwordElement;
    rv = FindUsernamePasswordFields(formElement, getter_AddRefs(usernameElement), getter_AddRefs(passwordElement), 
                                    PR_TRUE);

    if (NS_SUCCEEDED(rv) && usernameElement && passwordElement) {    
      //
      // We found the text field and password field. Check if there's
      // a username/password stored in the keychain for this host and
      // pre-fill the fields if that's the case.
      //
      nsCOMPtr<nsIURI> docURL;
      rv = doc->GetDocumentURL(getter_AddRefs(docURL));
      if (NS_FAILED(rv) || !docURL)
        return;
  
      NSMutableString* username = [NSMutableString string];
      NSMutableString* password = [NSMutableString string];
      
      nsCAutoString hostPort;
      docURL->GetHostPort(hostPort);
      NSString *realm = [NSString stringWithCString:hostPort.get()];
  
      if ([mKeychain getUsernameAndPassword:realm user:username password:password]) {
        nsAutoString user, pwd;
        [username assignTo_nsAString:user];
        [password assignTo_nsAString:pwd];
        
        rv = usernameElement->SetValue(user);
        rv = passwordElement->SetValue(pwd);
      }
        
      // We found the sign-in form so return now. This means we don't
      // support pages where there's multiple sign-in forms.
      return;
    }

  } // for each form on page
}

- (void)onLoadingStarted
{
}

- (void)onProgressChange:(int)currentBytes outOf:(int)maxBytes
{
}

- (void)onLocationChange:(NSString*)urlSpec
{
}

- (void)onStatusChange:(NSString*)aMessage
{
}

- (void)onSecurityStateChange:(unsigned long)newState
{
}

- (void)onShowContextMenu:(int)flags domEvent:(nsIDOMEvent*)aEvent domNode:(nsIDOMNode*)aNode
{
}

- (void)onShowTooltip:(NSPoint)where withText:(NSString*)text
{
}

- (void)onHideTooltip
{
}

@end

//
// FindUsernamePasswordFields
//
// Searches the form for the first username and password fields. If
// none are found, the out params will be nsnull. |inStopWhenFound|
// determines how we proceed once we find things. When true, we bail
// as soon as we find both a username and password field. If false, we
// continue searching the form for a 2nd password field (such as in a
// "change your password" form). If we find one, null out
// |outPassword| since we probably don't want to prefill this form.
//
nsresult
FindUsernamePasswordFields(nsIDOMHTMLFormElement* inFormElement, nsIDOMHTMLInputElement** outUsername,
                            nsIDOMHTMLInputElement** outPassword, PRBool inStopWhenFound)
{
  if ( !outUsername || !outPassword )
    return NS_ERROR_FAILURE;
  *outUsername = *outPassword = nsnull;

  //
  // Search the form the password field and the preceding text field
  // We are only interested in signon forms, so we require
  // only one password in the form. If there's more than one password
  // it's probably a form to setup the password or to change the
  // password. We don't want to handle this kind of form at this
  // point.
  //
  // BENOIT: We keep the first text field and password field we find
  // Shouldn't we keep the text field which is just
  // preceding the password field instead? There's maybe better chance
  // that the text field preceding the password field is the username
  // field in the case of a form where's there's multiple text
  // field...
  //
  nsCOMPtr<nsIDOMHTMLCollection> elements;
  nsresult rv = inFormElement->GetElements(getter_AddRefs(elements));
  if (NS_FAILED(rv) || !elements) 
  return NS_OK;

  PRUint32 numElements;
  elements->GetLength(&numElements);

  for (PRUint32 elementX = 0; elementX < numElements; elementX++) {

    nsCOMPtr<nsIDOMNode> elementNode;
    rv = elements->Item(elementX, getter_AddRefs(elementNode));
    if (NS_FAILED(rv) || !elementNode)
      continue;

    nsCOMPtr<nsIDOMHTMLInputElement> inputElement(do_QueryInterface(elementNode));
    if (!inputElement)
      continue;

    nsAutoString type;
    rv = inputElement->GetType(type);
    if (NS_FAILED(rv))
      continue;

    bool isText = (type.IsEmpty() || type.Equals(NS_LITERAL_STRING("text"), nsCaseInsensitiveStringComparator()));
    bool isPassword = type.Equals(NS_LITERAL_STRING("password"), nsCaseInsensitiveStringComparator());

    if(!isText && !isPassword)
      continue;

    //
    // If there's a second password in the form, it's probably not a
    // signin form, probably a form to setup or change
    // passwords. Stop here and ensure we don't store this password.
    //
    if (!inStopWhenFound && isPassword && *outPassword) {
      NS_RELEASE(*outPassword);
      *outPassword = nsnull;
      return NS_OK;
    }
    
    if(isText && !*outUsername) {
      *outUsername = inputElement;
      NS_ADDREF(*outUsername);
    }
    else if (isPassword && !*outPassword) {
      *outPassword = inputElement;
      NS_ADDREF(*outPassword);
    }
    
    // we've got everything we need, we're done.
    if (inStopWhenFound && *outPassword && *outUsername)
      return NS_OK;
    
  } // for each item in form

  return NS_OK;
}
