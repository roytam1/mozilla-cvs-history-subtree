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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Calum Robinson <calumr@mac.com>
 *   Simon Fraser <sfraser@netscape.com>
 *   Josh Aas <josh@mozilla.com>
 *   Nick Kreeger <nick.kreeger@park.edu>
 *   Bruce Davidson <mozilla@transoceanic.org.uk>
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


#import "NSString+Utils.h"
#import "NSView+Utils.h"

#import "ProgressViewController.h"
#import "ProgressDlgController.h"
#import "PreferenceManager.h"
#import "ProgressView.h"

// we should really get this from "CHBrowserService.h",
// but that requires linkage and extra search paths.
static NSString* XPCOMShutDownNotificationName = @"XPCOMShutDown";

// This is the OS notification name to make the Dock icon for the downloads folder bounce.
static NSString* const DownloadFinishedOSNotificationName = @"com.apple.DownloadFileFinished";

// Names of notifications we will post on download-related events
NSString* const kDownloadStartedNotificationName = @"DownloadStartedNotificationName";
NSString* const kDownloadFailedNotificationName = @"DownloadFailedNotificationName";
NSString* const kDownloadCompleteNotificationName = @"DownloadCompleteNotificationName";

// Names of keys for objects passed in these notifications
NSString* const kDownloadNotificationFilenameKey = @"DownloadNotificationFilenameKey";
NSString* const kDownloadNotificationTimeElapsedKey = @"DownloadNotificationTimeElapsedKey";

enum {
  kLabelTagFilename = 1000,
  kLabelTagStatus,
  kLabelTagTimeRemaining,
  kLabelTagIcon
};

@interface ProgressViewController(ProgressViewControllerPrivate)

+(unsigned int)getNextIdentifier;
-(void)viewDidLoad;
-(void)setupFileSystemNotification;
-(void)unsubscribeFileSystemNotification;
-(void)checkFileExists;
-(void)refreshDownloadInfo;
-(void)launchFileIfAppropriate;
-(void)setProgressViewFromDictionary:(NSDictionary*)aDict;
-(BOOL)shouldRemoveFromDownloadList;

@end

@implementation ProgressViewController

+(NSString *)formatTime:(int)seconds
{
  NSMutableString *theTime = [NSMutableString stringWithCapacity:8];
  
  NSString *padZero = [NSString stringWithString:@"0"];
  //write out new elapsed time
  if (seconds >= 3600) {
    [theTime appendFormat:@"%d:",(seconds / 3600)];
    seconds = seconds % 3600;
  }

  NSString *elapsedMin = [NSString stringWithFormat:@"%d:",(seconds / 60)];
  if ([elapsedMin length] == 2)
    [theTime appendString:[padZero stringByAppendingString:elapsedMin]];
  else
    [theTime appendString:elapsedMin];
  
  seconds = seconds % 60;
  NSString *elapsedSec = [NSString stringWithFormat:@"%d",seconds];
  
  if ([elapsedSec length] == 2)
    [theTime appendString:elapsedSec];
  else
    [theTime appendString:[padZero stringByAppendingString:elapsedSec]];
  
  return theTime;
}

// fuzzy time gives back strings like "about 5 seconds"
+ (NSString *)formatFuzzyTime:(int)seconds
{
  // check for seconds first
  if (seconds < 60) {
    if (seconds < 7)
      return [NSString stringWithFormat:NSLocalizedString(@"UnderSec", nil), 5];
    if (seconds < 13)
      return [NSString stringWithFormat:NSLocalizedString(@"UnderSec", nil), 10];
    return [NSString stringWithFormat:NSLocalizedString(@"UnderMin", nil)];
  }    
  // seconds becomes minutes and we keep checking.  
  seconds = seconds/60;
  if (seconds < 60) {
    if (seconds < 2)
      return [NSString stringWithFormat:NSLocalizedString(@"AboutMin", nil)];
    // OK, tell the good people how much time we have left.
    return [NSString stringWithFormat:NSLocalizedString(@"AboutMins", nil), seconds];
  }
  //this download will never seemingly never end. now seconds become hours.
  seconds = seconds/60;
  if (seconds < 2)
    return [NSString stringWithFormat:NSLocalizedString(@"AboutHour", nil)];
  return [NSString stringWithFormat:NSLocalizedString(@"AboutHours", nil), seconds];
}

+(NSString*)formatBytes:(float)bytes
{
  // if bytes are negative, we return question marks.
  if (bytes < 0)
    return [NSString stringWithString:@"?"];
  // bytes first.
  if (bytes < 1024)
    return [NSString stringWithFormat:@"%.1f bytes",bytes];
  // kb
  bytes = bytes/1024;
  if (bytes < 1024)
    return [NSString stringWithFormat:@"%.1f KB",bytes];
  // mb
  bytes = bytes/1024;
  if (bytes < 1024)
    return [NSString stringWithFormat:@"%.1f MB",bytes];
  // gb
  bytes = bytes/1024;
  return [NSString stringWithFormat:@"%.1f GB",bytes];
}

+(unsigned int)getNextIdentifier
{
  static unsigned int identifier = 0;
  unsigned int toReturn;
  // Not sure if there is a need for thread-safety here, but better safe than sorry.
  @synchronized(self) {
    // Store the old value, and increment for next time.
    toReturn = identifier++;
  }
  return toReturn;
}

#pragma mark -

-(id)init
{
  if ((self = [super init]))
  {
    [NSBundle loadNibNamed:@"ProgressView" owner:self];
    mUniqueIdentifier = [ProgressViewController getNextIdentifier];
  }
  return self;
}

-(id)initWithWindowController:(ProgressDlgController*)aWindowController
{
  if ((self = [self init]))
    mProgressWindowController = aWindowController;
  
  return self;
}

-(id)initWithDictionary:(NSDictionary*)aDict 
    andWindowController:(ProgressDlgController*)aWindowController
{
  if ((self = [self init]))
  {
    mProgressWindowController = aWindowController;
    [self setProgressViewFromDictionary:aDict];
  }
  
  return self;
}

-(void)awakeFromNib
{
  [self viewDidLoad];
}

-(void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  
  // if we get here because we're quitting, the listener will still be alive
  // yet we're going away. As a result, we need to tell the d/l listener to
  // forget it ever met us and necko will clean it up on its own.
  if (mDownloader) {
    mDownloader->DetachDownloadDisplay();
    NS_RELEASE(mDownloader);
  }

  // the views might outlive us, so clear refs to us
  [mCompletedView setController:nil];
  [mProgressView setController:nil];
  
  [mStartTime release];
  [mSourceURL release];
  [mDestPath release];

  // loading the nib has retained these, we have to release them.
  [mCompletedView release];
  [mProgressView release];

  [super dealloc];
}

- (void)xpcomShutdown:(NSNotification*)notification
{
  // if we get here because we're quitting, the listener will still be alive
  // yet we're going away. As a result, we need to tell the d/l listener to
  // forget it ever met us and necko will clean it up on its own.
  if (mDownloader) {
    mDownloader->DetachDownloadDisplay();
    NS_RELEASE(mDownloader);
  }
}

-(void)viewDidLoad
{
  // this isn't necessarily better. Need to profile.
  [mProgressBar setUsesThreadedAnimation:NO];
  // give the views this controller as their controller
  [mCompletedView setController:self];
  [mProgressView setController:self];
  
  mRefreshIcon = YES;

  // we need to register for xpcom shutdown so that we can clear the
  // services before XPCOM is shut down. We can't rely on dealloc,
  // because we don't know when it will get called (we might be autoreleased).
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(xpcomShutdown:)
                                               name:XPCOMShutDownNotificationName
                                              object:nil];
}

-(ProgressView*)view
{
  if ([self isPaused] || mDownloadDone)
    return mCompletedView;
  else
    return mProgressView;
}

-(void)setupFileSystemNotification
{
  [self checkFileExists];
  if (mFileExists && !mFileIsWatched) {
    // Adding ourselves to the watch kqueue creates an extra ref-count to our instance.
    // We will remove ourselves from the watch kqueue on |displayWillBeRemoved|, which
    // will remove the extra ref count from our instance.
    [mProgressWindowController addFileDelegateToWatchList:self];
    mFileIsWatched = YES;
  }
}

-(void)unsubscribeFileSystemNotification
{
  [mProgressWindowController removeFileDelegateFromWatchList:self];
  mFileIsWatched = NO;
}

-(IBAction)copySourceURL:(id)sender
{
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
  [pasteboard setString:mSourceURL forType:NSStringPboardType];
}

-(IBAction)cancel:(id)sender
{
  // don't allow download to be cancelled twice or we get a nasty crash,
  // and don't cancel completed downloads, because that deletes the file
  if (!mUserCancelled && !mDownloadDone) {
    mUserCancelled = YES;
    
    if (mDownloader) { // we should always have one
      mDownloader->CancelDownload();
    }
    
    // note that we never want to delete downloaded files here,
    // because the file does not have its final path yet.
    // Necko will delete the evil temp file.
  }
}

-(IBAction)reveal:(id)sender
{
  [self checkFileExists];
  if (!mFileExists ||
      ![[NSWorkspace sharedWorkspace] selectFile:mDestPath
      inFileViewerRootedAtPath:[mDestPath stringByDeletingLastPathComponent]])
  {
    NSBeep();
  }
}

-(IBAction)open:(id)sender
{
  [self checkFileExists];
  if (!mFileExists ||
      ![[NSWorkspace sharedWorkspace] openFile:mDestPath])
  {
    NSBeep();
  }
  return;
}

-(IBAction)pause:(id)sender
{
  if (!mUserCancelled && !mDownloadDone && mDownloader) 
  {
    mDownloader->PauseDownload();
    mRefreshIcon = YES;
    [self refreshDownloadInfo];
  }
}

-(IBAction)resume:(id)sender
{
  if (!mUserCancelled && !mDownloadDone && mDownloader) 
  {
    mRefreshIcon = YES;
    mDownloader->ResumeDownload();
    [self refreshDownloadInfo];
  }
}

-(IBAction)remove:(id)sender
{
  [mProgressWindowController removeDownload:self suppressRedraw:NO];
}

// Called just before the view will be shown to the user
-(void)downloadDidStart
{
  mStartTime = [[NSDate alloc] init];
  [self refreshDownloadInfo];

  NSDictionary* userInfo = [NSDictionary dictionaryWithObject:mDestPath
                                                       forKey:kDownloadNotificationFilenameKey];
  [[NSNotificationCenter defaultCenter] postNotificationName:kDownloadStartedNotificationName
                                                      object:self
                                                    userInfo:userInfo];
}

// Move the downloaded file to the trash.
-(BOOL)deleteFile
{
  [self checkFileExists];
  if (!mFileExists) {
    NSBeep();
    return NO;
  }
  NSString* fileName = [mDestPath lastPathComponent];
  NSString* path = [mDestPath stringByDeletingLastPathComponent];

  [[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation
                                               source:path
                                          destination:@""
                                                files:[NSArray arrayWithObject:fileName]
                                                  tag:nil];
  return YES;
}

-(void)downloadDidEnd
{
  if (!mDownloadDone) { // some error conditions can cause this to get called twice
    mDownloadDone = YES;
    mDownloadTime = -[mStartTime timeIntervalSinceNow];
    [mProgressBar stopAnimation:self];
    
    // get the Finder to update
    [[NSWorkspace sharedWorkspace] noteFileSystemChanged:mDestPath];
    
    mRefreshIcon = YES; // let the icon know to update
    
    [self refreshDownloadInfo];
    [self launchFileIfAppropriate];
    if ([self shouldRemoveFromDownloadList])
      [mProgressWindowController removeDownload:self suppressRedraw:NO];
    else if (!mDownloadFailed)
      [self setupFileSystemNotification];

    // Sending this notification makes the Dock icon (if it exists)
    // for the folder containing the downloaded file bounce.
    if (!mUserCancelled && !mDownloadFailed) {
      [[NSDistributedNotificationCenter defaultCenter]
       postNotificationName:DownloadFinishedOSNotificationName object:mDestPath];
    }

    // Notify any listeners (currently, just Growl) if the download completed or failed.
    if (!mUserCancelled) {
      NSDictionary* userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
                                mDestPath, kDownloadNotificationFilenameKey,
                                [NSNumber numberWithDouble: mDownloadTime], kDownloadNotificationTimeElapsedKey,
                                nil];

      NSString* name = mDownloadFailed ? kDownloadFailedNotificationName : kDownloadCompleteNotificationName;
      [[NSNotificationCenter defaultCenter] postNotificationName:name
                                                          object:self
                                                        userInfo:userInfo];
    }
  }
}

-(void)launchFileIfAppropriate
{
  if (!mIsFileSave && !mUserCancelled && !mDownloadFailed) {
    if ([[PreferenceManager sharedInstance] getBooleanPref:kGeckoPrefAutoOpenDownloads withSuccess:NULL]) {
      [[NSWorkspace sharedWorkspace] openFile:mDestPath];
    }
  }
}

-(BOOL)shouldRemoveFromDownloadList
{
  if (![self hasSucceeded])
    return NO;
  int downloadRemoveActionValue = [[PreferenceManager sharedInstance] getIntPref:kGeckoPrefDownloadCleanupPolicy 
                                                                     withSuccess:NULL];
  return (downloadRemoveActionValue == kRemoveDownloadsOnSuccess);
}

// this handles lots of things - all of the status updates
-(void)refreshDownloadInfo
{
  // note that the view you get here depends on whether we're paused/done or not,
  // so we have to be sure to refresh the info if our state changes.
  NSView* curView = [self view];
  NSString* filename = [mDestPath lastPathComponent];
  
  id filenameLabel = [curView viewWithTag:kLabelTagFilename];
  if (![[filenameLabel stringValue] isEqualToString:filename])
    [filenameLabel setStringValue:filename];
  
  NSImageView* iconImageView = [curView viewWithTag:kLabelTagIcon];
  if (iconImageView && mRefreshIcon) { // update the icon image
    NSImage* iconImage = nil;
    if (mDownloadDone && !mFileExists)
    {
      // for "known missing" files, show the missing icon
      iconImage = [NSImage imageNamed:@"unknown_file_icon"];
    }
    else
    {
      // we're currently relying on iconForFile: to give us back a generic
      // document icon when it can't find the file (e.g. at the start of the
      // download). We should probably supply our own icon.
      iconImage = [[NSWorkspace sharedWorkspace] iconForFile:mDestPath];
    }
     
    // sometimes the finder doesn't have an icon for us (rarely)
    // when that happens just leave it at what it was before
    if (iconImage)
      [iconImageView setImage:iconImage];
    
    mRefreshIcon = NO; // dont change unless status changes
  }
  
  if (mDownloadDone) { // just update the status field
    id statusLabel = [curView viewWithTag:kLabelTagStatus];
    if (statusLabel) {
      NSString* statusString;
      if (mUserCancelled) {
        statusString = NSLocalizedString(@"DownloadCancelled", nil);
      }
      else if (mDownloadFailed) {
        statusString = NSLocalizedString(@"DownloadInterrupted", nil);
      }
      else {
        // If the download size was not known then lookup size from the file we downloaded
        if (mDownloadSize < 0) {
          [self checkFileExists];
          if (mFileExists) {
            NSDictionary* fattrs = [[NSFileManager defaultManager] fileAttributesAtPath:mDestPath traverseLink:NO];
            mDownloadSize = (long long) [fattrs fileSize];
          }
        }

        statusString = [NSString stringWithFormat:NSLocalizedString(@"DownloadCompleted", nil),
          [[self class] formatTime:(int)mDownloadTime], [[self class] formatBytes:mDownloadSize]];
      }
      
      if (![[statusLabel stringValue] isEqualToString:statusString])
        [statusLabel setStringValue:statusString];
    }
  }
	
  else if ([self isPaused]) { // update the status field
    id statusLabel = [curView viewWithTag:kLabelTagStatus];
    if (statusLabel) {
      NSString* statusString = NSLocalizedString(@"DownloadPausedStatusString", nil);
      [statusLabel setStringValue:[NSString stringWithFormat:statusString,
        [[self class] formatBytes:mCurrentProgress],
        (mDownloadSize > 0 ? [[self class] formatBytes:mDownloadSize] : @"?")]];
    }
  }
	
  else {
    NSTimeInterval elapsedTime = -[mStartTime timeIntervalSinceNow];
    
    // update status field
    id statusLabel = [curView viewWithTag:kLabelTagStatus];
    if (statusLabel) {
      NSString *statusLabelString = NSLocalizedString(@"DownloadStatusString", nil);
      float byteSec = mCurrentProgress / elapsedTime;
      [statusLabel setStringValue:[NSString stringWithFormat:statusLabelString, 
                   [[self class] formatBytes:mCurrentProgress],
                   (mDownloadSize > 0 ? [[self class] formatBytes:mDownloadSize] : @"?"),
                   [[self class] formatBytes:byteSec]]];
    }
    
    id timeLabel = [curView viewWithTag:kLabelTagTimeRemaining];
    if (timeLabel) {
      if (mDownloadSize > 0) {
        int secToGo = (int)ceil((elapsedTime * mDownloadSize / mCurrentProgress) - elapsedTime);
        [timeLabel setStringValue:[[self class] formatFuzzyTime:secToGo]];
      }
      else { // mDownloadSize is undetermined.  Set remaining time to question marks.
        [timeLabel setStringValue:NSLocalizedString(@"DownloadCalculatingString", nil)];
      }
    }
  }
}

-(BOOL)isActive
{
  return !mDownloadDone;
}

-(BOOL)isCanceled
{
  return mUserCancelled;
}

-(BOOL)isSelected
{
  return mIsSelected;
}

-(void)setSelected:(BOOL)inSelected
{
  if (mIsSelected != inSelected)
  {
    mIsSelected = inSelected;
    [[self view] setNeedsDisplay:YES];
  }
}

-(BOOL)isPaused
{
  if (mDownloader)
    return mDownloader->IsDownloadPaused();
  
  return NO;
}

-(BOOL)fileExists
{
  return mFileExists; 
}

// If the download failed during transfer or the user cancelled return NO
-(BOOL)hasSucceeded
{
  return (!mDownloadFailed && !mUserCancelled);
}

// this is the callback from the file system notification
-(void)checkFileExists
{
  // if dest path doesn't exist, don't check
  if (!mDestPath)
    return;
  
  BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:mDestPath];
  if (fileExists != mFileExists)
  {
    mFileExists = fileExists;
    mRefreshIcon = YES;
    [self refreshDownloadInfo];
  }
}

-(void)setProgressViewFromDictionary:(NSDictionary*)aDict
{
  // set this first, because it affects which view we fill in
  // if we're reading from the dict, the download is either completed or cancelled
  mDownloadDone = YES; 

  [self setDestinationPath:[aDict valueForKey:@"destPath"]];
  [self setSourceURL:[aDict valueForKey:@"sourceURL"]]; 
  mDownloadSize = [[aDict valueForKey:@"downloadSize"] longLongValue];
  mCurrentProgress = [[aDict valueForKey:@"currentProgress"] longLongValue];
  mDownloadTime = [[aDict valueForKey:@"downloadTime"] doubleValue];
  mIsSelected = [[aDict valueForKey:@"isSelected"] boolValue];
  
  // set as cancel if sizes dont match up
  if (mDownloadSize != mCurrentProgress)
    mUserCancelled = YES; 
  
  // we have to force the update here
  mRefreshIcon = YES;
  [self refreshDownloadInfo];
}

-(NSDictionary*)downloadInfoDictionary
{
  NSMutableDictionary* dict = [[[NSMutableDictionary alloc] init] autorelease];
  [dict setObject: mDestPath forKey:@"destPath"];
  [dict setObject: mSourceURL forKey:@"sourceURL"];
  [dict setObject:[NSNumber numberWithLongLong: mDownloadSize] forKey:@"downloadSize"];
  [dict setObject:[NSNumber numberWithLongLong: mCurrentProgress] forKey:@"currentProgress"];
  [dict setObject:[NSNumber numberWithDouble: mDownloadTime] forKey:@"downloadTime"];
  [dict setObject:[NSNumber numberWithBool: mIsSelected] forKey:@"isSelected"];
  
  return dict;
}

-(unsigned int)uniqueIdentifier
{
  return mUniqueIdentifier;
}

-(NSString*)representedFilePath
{
  return mDestPath;
}

-(void)fileRemoved
{
  if (mIsBeingRemoved)
    return;

  // This method gets called on a background thread, so switch the |checkFileExists| call to the main thread.
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [self performSelectorOnMainThread:@selector(checkFileExists)
                         withObject:nil 
                      waitUntilDone:NO];
  [pool release];
}

-(NSMenu*)contextualMenu
{
  NSMenu *menu = [[NSMenu alloc] init];
  NSMenuItem *revealItem;
  NSMenuItem *cancelItem;
  NSMenuItem *removeItem;
  NSMenuItem *pauseResumeItem; // alternates pause and resume
  NSMenuItem *openItem;
  NSMenuItem *deleteItem;
  NSMenuItem *copySourceURLItem;
  
  revealItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlRevealCMLabel", nil)
                                   action:@selector(reveal:) keyEquivalent:@""];
  cancelItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlCancelCMLabel", nil)
                                   action:@selector(cancel:) keyEquivalent:@""];
  if ([self isPaused]) {
    pauseResumeItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlResumeCMLabel", nil) 
                                                 action:@selector(resume:) keyEquivalent:@""];
  }
  else {
    pauseResumeItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlPauseCMLabel", nil)
                                                 action:@selector(pause:) keyEquivalent:@""];
  }
  removeItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlRemoveCMLabel", nil)
                                   action:@selector(remove:) keyEquivalent:@""];
  openItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlOpenCMLabel", nil)
                                   action:@selector(open:) keyEquivalent:@""];
  deleteItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlTrashCMLabel", nil)
                                   action:@selector(deleteDownloads:) keyEquivalent:@""];
  copySourceURLItem = [[NSMenuItem alloc] initWithTitle:NSLocalizedString(@"dlCopySourceURLCMLabel", nil)
                                   action:@selector(copySourceURL:) keyEquivalent:@""];
  [revealItem setTarget:mProgressWindowController];
  [cancelItem setTarget:mProgressWindowController];
  [pauseResumeItem setTarget:mProgressWindowController];
  [removeItem setTarget:mProgressWindowController];
  [openItem setTarget:mProgressWindowController];
  [deleteItem setTarget:mProgressWindowController];
  [copySourceURLItem setTarget:self];
  
  [menu addItem:revealItem];
  [menu addItem:cancelItem];
  [menu addItem:pauseResumeItem];
  [menu addItem:removeItem];
  [menu addItem:openItem];
  [menu addItem:deleteItem];
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItem:copySourceURLItem];
  
  [revealItem release];
  [cancelItem release];
  [pauseResumeItem release];
  [removeItem release];
  [openItem release];
  [deleteItem release];
  [copySourceURLItem release];
  
  return [menu autorelease];    
}

// Returns YES if the action makes sense given the state of the download.
- (BOOL)shouldAllowAction:(SEL)action
{
  if (action == @selector(remove:)) {
    return ![self isActive];
  }
  else if (action == @selector(open:)) {
    return ![self isActive] && ![self isCanceled] && [self fileExists];
  }
  else if (action == @selector(reveal:)) {
    return ![self isCanceled] && [self fileExists];
  }
  else if (action == @selector(cancel:)) {
    return [self isActive] && ![self isCanceled];
  }
  else if (action == @selector(pause:)) {
    return [self isActive] && ![self isPaused];
  }
  else if (action == @selector(resume:)) {
    return [self isActive] && [self isPaused];
  }
  else if (action == @selector(deleteDownloads:)) {
    return ![self isActive] && [self fileExists];
  }

  return YES;
}

- (BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
  // Currently, this method is responsible only for the "Copy Source URL" function;
  // all others are handled by |ProgressDlgController|.
  return YES;
}

#pragma mark -

-(void)updateSelectionWithBehavior:(DownloadSelectionBehavior)behavior
{
  [mProgressWindowController updateSelectionOfDownload:self withBehavior:behavior];
}

-(void)openSelectedDownloads
{
  [mProgressWindowController open:self];
}

-(void)cancelSelectedDownloads
{
  [mProgressWindowController cancel:self];
}

#pragma mark -

-(void)onStartDownload:(BOOL)isFileSave
{
  mIsFileSave = isFileSave;
  [self downloadDidStart];
  [mProgressWindowController didStartDownload:self];
  
  // need to do this after the view as been put in the window, otherwise it doesn't work
  [mProgressBar startAnimation:self];
}

- (void)onEndDownload:(BOOL)completedOK statusCode:(nsresult)aStatus
{
  mDownloadFailed = !completedOK;

  [self downloadDidEnd];
  [mProgressWindowController didEndDownload:self withSuccess:completedOK statusCode:aStatus];
}

-(void)setProgressTo:(long long)aCurProgress ofMax:(long long)aMaxProgress
{
  mCurrentProgress = aCurProgress;         // fall back for stat calcs
  mDownloadSize = aMaxProgress;
  
  if (![mProgressBar isIndeterminate]) {   //most likely - just update value
    if (aCurProgress == aMaxProgress) {    //handles little bug in FTP download size
      [mProgressBar setMaxValue:aMaxProgress];
    }
    [mProgressBar setDoubleValue:aCurProgress];
  }
  else if (aMaxProgress > 0) {            // ok, we're starting up with good max & cur values
    [mProgressBar setIndeterminate:NO];
    [mProgressBar setMaxValue:aMaxProgress];
    [mProgressBar setDoubleValue:aCurProgress];
  } // if neither case was true, it's barber pole city
}

-(void)setDownloadListener:(CHDownloader*)aDownloader
{
  if (mDownloader != aDownloader) {
    NS_IF_RELEASE(mDownloader);
  }
  
  NS_IF_ADDREF(mDownloader = aDownloader);
}

#if 0
/*
 This is kind of a hack. It should probably be done somewhere else so Mozilla can have
 it too, but until Apple fixes the problems with the setting of comments without 
 reverting to Applescript, I have left it in. 
 
 Turned off for now, until we find a better way to do this. Won't Carbon APIs work?
 */
- (void)tryToSetFinderComments
{
  if (mDestPath && mSourceURL) {
    CFURLRef fileURL = CFURLCreateWithFileSystemPath(   NULL,
                                                        (CFStringRef)mDestPath,
                                                        kCFURLPOSIXPathStyle,
                                                        NO);
    
    NSString *hfsPath = (NSString *)CFURLCopyFileSystemPath(fileURL,
                                                            kCFURLHFSPathStyle);
    
    CFRelease(fileURL);
    
    NSAppleScript *setCommentScript = [[NSAppleScript alloc] initWithSource:
      [NSString stringWithFormat:@"tell application \"Finder\" to set comment of file \"%@\" to \"%@\"", hfsPath, mSourceURL]];
    NSDictionary *errorInfo = NULL;
    
    [setCommentScript executeAndReturnError:&errorInfo];
    
    if (errorInfo) {
      NSLog(@"Get error when running AppleScript to set comments for '%@':\n %@", 
            mDestPath,
            [errorInfo objectForKey:NSAppleScriptErrorMessage]);
    }
  }
}

#endif

-(void)setSourceURL:(NSString*)aSourceURL
{
  [mSourceURL autorelease];
  mSourceURL = [aSourceURL copy];
  NSString* toolTipString = [mSourceURL unescapedURI];
  [mProgressView setToolTip:toolTipString];
  [mCompletedView setToolTip:toolTipString];
  //[self tryToSetFinderComments];
}

- (NSString*)sourceURL
{
  return mSourceURL;
}

-(void)setDestinationPath:(NSString*)aDestPath
{
  [mDestPath autorelease];
  mDestPath = [aDestPath copy];
  if (mDownloadDone)
    [self setupFileSystemNotification];
  //[self tryToSetFinderComments];
}

-(NSString*)destinationPath
{
  return mDestPath;
}

//
// This method exists because of an extra ref-count to ourselves in
// |FileChangeWatcher|. To remove this extra refcount before |dealloc|,
// this class gets notified when the view is about to be removed.
//
-(void)displayWillBeRemoved
{
  // Mark ourselves as in tear-down so that we can ignore any file system
  // notification that may come as we are unsubscribing ourselves.
  mIsBeingRemoved = YES;
  // The file can only be watched if the download compeleted sucessfully
  if (mFileIsWatched)
    [self unsubscribeFileSystemNotification];
}

@end
