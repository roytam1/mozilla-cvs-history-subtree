#import <Cocoa/Cocoa.h>
#import "CHBrowserView.h"

@interface MyBrowserView : NSView <CHBrowserListener, CHBrowserContainer>
{
    IBOutlet id urlbar;
    IBOutlet id status;
    IBOutlet id progress;
    IBOutlet id progressSuper;
    CHBrowserView* browserView;
    NSString* defaultStatus;
    NSString* loadingStatus;
}
- (IBAction)load:(id)sender;
- (void)awakeFromNib;
- (void)setFrame:(NSRect)frameRect;

// NSBrowserListener messages
- (void)onLoadingStarted;
- (void)onLoadingCompleted:(BOOL)succeeded;
- (void)onProgressChange:(int)currentBytes outOf:(int)maxBytes;
- (void)onLocationChange:(NSURL*)url;

// NSBrowserContainer messages
- (void)setStatus:(NSString *)statusString ofType:(NSStatusType)type;
- (NSString *)title;
- (void)setTitle:(NSString *)title;
- (void)sizeBrowserTo:(NSSize)dimensions;
- (CHBrowserView*)createBrowserWindow:(unsigned int)mask;

@end
