#import <Cocoa/Cocoa.h>
#import <PreferencePanes/NSPreferencePane.h>
#import "PreferencePaneBase.h"

class nsIPref;

@interface OrgMozillaChimeraPreferencePrivacy : PreferencePaneBase
{
  IBOutlet id mCookies;
  IBOutlet NSButton* mPromptForCookie;
  
  IBOutlet NSButton* mStorePasswords;
  IBOutlet NSButton* mAutoFillPasswords;
}

-(IBAction) clearCookies:(id)aSender;

-(IBAction) clickPromptForCookie:(id)sender;
-(IBAction) clickEnableCookies:(id)sender;

-(IBAction) clickStorePasswords:(id)sender;
-(IBAction) clickAutoFillPasswords:(id)sender;
-(IBAction) launchKeychainAccess:(id)sender;

@end
