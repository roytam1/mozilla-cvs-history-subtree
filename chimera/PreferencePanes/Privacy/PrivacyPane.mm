#import "PrivacyPane.h"

#include "nsCOMPtr.h"
#include "nsIServiceManagerUtils.h"
#include "nsIPref.h"
#include "nsCCookieManager.h"


// prefs for keychain password autofill
static const char* const gUseKeychainPref = "chimera.store_passwords_with_keychain";
static const char* const gAutoFillEnabledPref = "chimera.keychain_passwords_autofill";

@implementation OrgMozillaChimeraPreferencePrivacy

- (void) dealloc
{
  [super dealloc];
}


- (void)mainViewDidLoad
{
  if ( !mPrefService )
    return;
    
  // Hookup cookie prefs. Relies on the tags of the radio buttons in the matrix being
  // set such that "enable all" is 0 and "disable all" is 2. If mozilla has other prefs
  // that we don't quite know about, we assume they were remapped by the PreferenceManager
  // at startup.
  PRInt32 acceptCookies = 0;
  mPrefService->GetIntPref("network.cookie.cookieBehavior", &acceptCookies);
  if ( [mCookies selectCellWithTag:acceptCookies] != YES )
    NS_WARNING("Bad value for network.cookie.cookieBehavior");
    
  PRBool warnAboutCookies = PR_TRUE;
  mPrefService->GetBoolPref("network.cookie.warnAboutCookies", &warnAboutCookies);
  [mPromptForCookie setState:(warnAboutCookies ? NSOnState : NSOffState)];
  
  // Keychain checkboxes	

  PRBool storePasswords = PR_TRUE;
  mPrefService->GetBoolPref(gUseKeychainPref, &storePasswords);
  [mStorePasswords setState:(storePasswords ? NSOnState : NSOffState)];
  [mAutoFillPasswords setEnabled:storePasswords ? YES : NO];

  PRBool autoFillPasswords = PR_TRUE;
  mPrefService->GetBoolPref(gAutoFillEnabledPref, &autoFillPasswords);
  [mAutoFillPasswords setState:(autoFillPasswords ? NSOnState : NSOffState)];
}

//
// clearCookies:
//
// Clear all the user's cookies.
//
-(IBAction) clearCookies:(id)aSender
{
  nsCOMPtr<nsICookieManager> cookieMonster ( do_GetService(NS_COOKIEMANAGER_CONTRACTID) );
  if ( cookieMonster )
    cookieMonster->RemoveAll();
}

//
// clickPromptForCookie:
//
// Set if the user should be prompted for each cookie
//
-(IBAction) clickPromptForCookie:(id)sender
{
  [self setPref:"network.cookie.warnAboutCookies" toBoolean:[sender state] == NSOnState];
}

//
// clickEnableCookies:
//
// Set cookie prefs. Relies on the tags of the radio buttons in the matrix being
// set such that "enable all" is 0 and "disable all" is 2.
//
-(IBAction) clickEnableCookies:(id)sender
{
  if ( !mPrefService )
    return;
  mPrefService->SetIntPref("network.cookie.cookieBehavior", [[mCookies selectedCell] tag]);
}

//
// clickStorePasswords
//
-(IBAction) clickStorePasswords:(id)sender
{
  if ( !mPrefService )
    return;
  if([mStorePasswords state] == NSOnState)
  {
      mPrefService->SetBoolPref("chimera.store_passwords_with_keychain", PR_TRUE);
      [mAutoFillPasswords setEnabled:YES];
  }
  else
  {
      mPrefService->SetBoolPref("chimera.store_passwords_with_keychain", PR_FALSE);
      [mAutoFillPasswords setEnabled:NO];
  }        
}

//
// clickAutoFillPasswords
//
// Set pref if autofill is enabled
//
-(IBAction) clickAutoFillPasswords:(id)sender
{
  if ( !mPrefService )
    return;
  mPrefService->SetBoolPref("chimera.keychain_passwords_autofill",
                            [mAutoFillPasswords state] == NSOnState ? PR_TRUE : PR_FALSE);
}

-(IBAction) launchKeychainAccess:(id)sender
{
  if ([[NSWorkspace sharedWorkspace] launchApplication:NSLocalizedString(@"Keychain Access", @"Keychain Access")] == NO) {
    // XXXw. pop up a dialog warning that Keychain couldn't be launched?
    NSLog(@"Failed to launch Keychain.");
  }
}
@end
