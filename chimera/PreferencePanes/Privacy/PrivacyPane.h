#import <Cocoa/Cocoa.h>
#import <PreferencePanes/NSPreferencePane.h>
#import "PreferencePaneBase.h"

class nsIPref;
class nsIPermissionManager;
class nsISimpleEnumerator;
class nsISupportsArray;

@interface OrgMozillaChimeraPreferencePrivacy : PreferencePaneBase
{
  IBOutlet id mCookies;
  IBOutlet id mCookieSitePanel;
  IBOutlet NSButton* mPromptForCookie;
  
  IBOutlet NSButton* mStorePasswords;
  IBOutlet NSButton* mAutoFillPasswords;
  
  IBOutlet NSTableView* mSiteTable;
  nsIPermissionManager* mManager;         // STRONG (should be nsCOMPtr)  
  nsISupportsArray* mCachedPermissions;		// parallel list of permissions for speed, STRONG (should be nsCOMPtr)
}

-(IBAction) clearCookies:(id)aSender;
-(IBAction) editCookieSites:(id)aSender;
-(IBAction) editCookieSitesDone:(id)aSender;
-(IBAction) removeCookieSite:(id)aSender;
- (void) editCookieSitesSheetDidEnd:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void  *)contextInfo;

// data source informal protocol (NSTableDataSource)
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;

-(IBAction) clickPromptForCookie:(id)sender;
-(IBAction) clickEnableCookies:(id)sender;

-(IBAction) clickStorePasswords:(id)sender;
-(IBAction) clickAutoFillPasswords:(id)sender;
-(IBAction) launchKeychainAccess:(id)sender;

@end
