#import "PrivacyPane.h"

#include "nsCOMPtr.h"
#include "nsIServiceManagerUtils.h"
#include "nsIPref.h"
#include "nsCCookieManager.h"
#include "nsIPermissionManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIPermission.h"
#include "nsISupportsArray.h"
#include "nsXPIDLString.h"


// prefs for keychain password autofill
static const char* const gUseKeychainPref = "chimera.store_passwords_with_keychain";
static const char* const gAutoFillEnabledPref = "chimera.keychain_passwords_autofill";

@implementation OrgMozillaChimeraPreferencePrivacy

- (void) dealloc
{
  NS_IF_RELEASE(mManager);
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
  
  // store permission manager service and cache the enumerator.
  nsCOMPtr<nsIPermissionManager> pm ( do_GetService(NS_PERMISSIONMANAGER_CONTRACTID) );
  mManager = pm.get();
  NS_IF_ADDREF(mManager);
  
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


-(IBAction) editCookieSites:(id)aSender
{
  nsCOMPtr<nsISimpleEnumerator> permEnum;
  if ( mManager ) 
    mManager->GetEnumerator(getter_AddRefs(permEnum));

  // build parallel permission list for speed with a lot of blocked sites
  NS_NewISupportsArray(&mCachedPermissions);     // ADDREFs
  if ( mCachedPermissions && permEnum ) {
    PRBool hasMoreElements = PR_FALSE;
    permEnum->HasMoreElements(&hasMoreElements);
    while ( hasMoreElements ) {
      nsCOMPtr<nsISupports> curr;
      permEnum->GetNext(getter_AddRefs(curr));
      mCachedPermissions->AppendElement(curr);
      
      permEnum->HasMoreElements(&hasMoreElements);
    }
  }
  
	[NSApp beginSheet:mCookieSitePanel
        modalForWindow:[mCookies window]		// any old window accessor
        modalDelegate:self
        didEndSelector:@selector(editCookieSitesSheetDidEnd:returnCode:contextInfo:)
        contextInfo:NULL];
        
  // ensure a row is selected (cocoa doesn't do this for us, but will keep
  // us from unselecting a row once one is set; go figure).
  [mSiteTable selectRow:0 byExtendingSelection:NO];
}

-(IBAction) editCookieSitesDone:(id)aSender
{
  // save stuff
  
  [mCookieSitePanel orderOut:self];
  [NSApp endSheet:mCookieSitePanel];
  
	NS_IF_RELEASE(mCachedPermissions);
}

- (void)editCookieSitesSheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void  *)contextInfo
{
}


-(IBAction) removeCookieSite:(id)aSender
{
  if ( mCachedPermissions && mManager ) {
    // remove from parallel array and cookie permissions list
    int row = [mSiteTable selectedRow];
    
    // remove from permission manager (which is done by host, not by row), then 
    // remove it from our parallel array (which is done by row). Since we keep a
    // parallel array, removing multiple items by row is very difficult since after 
    // deleting, the array is out of sync with the next cocoa row we're told to remove. Punt!
    nsCOMPtr<nsISupports> rowItem = dont_AddRef(mCachedPermissions->ElementAt(row));
    nsCOMPtr<nsIPermission> perm ( do_QueryInterface(rowItem) );
    if ( perm ) {
      nsXPIDLCString host;
      perm->GetHost(getter_Copies(host));
      mManager->Remove(host.get(), 0);           // could this api _be_ any worse? Come on!
      
      mCachedPermissions->RemoveElementAt(row);
    }
    [mSiteTable reloadData];
  }
}


//
// NSTableDataSource protocol methods
//

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
  PRUint32 numRows = 0;
  if ( mCachedPermissions )
    mCachedPermissions->Count(&numRows);

  return (int) numRows;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
  NSString* retVal = nil;
  if ( mCachedPermissions ) {
    nsCOMPtr<nsISupports> rowItem = dont_AddRef(mCachedPermissions->ElementAt(rowIndex));
    nsCOMPtr<nsIPermission> perm ( do_QueryInterface(rowItem) );
    if ( perm ) {
      if ( [[aTableColumn identifier] isEqualToString:@"Website"] ) {
        // website url column
        nsXPIDLCString host;
        perm->GetHost(getter_Copies(host));
        retVal = [NSString stringWithCString:host];
      }
      else {
        // allow/deny column
        PRBool capability = PR_FALSE;			// false = deny, true = accept;
        perm->GetCapability(&capability);
        if ( capability )
          retVal = [self getLocalizedString:@"Allow"];
        else
          retVal = [self getLocalizedString:@"Deny"];
      }
    }
  }
  
  return retVal;
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
