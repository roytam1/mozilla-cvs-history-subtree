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


#import "CHDownloadProgressDisplay.h"
#import "FileChangeWatcher.h"

class CHDownloader;
@class ProgressDlgController;
@class ProgressView;

// do this because there is no "no" key in the set of key modifiers.
// it is in this header because both classes that need it include this file
// (ProgressDlgController and ProgressView)
enum {
  kNoKey = 0,
  kShiftKey = 1,
  kCommandKey = 2
};
typedef enum {
  DownloadSelectExclusively,
  DownloadSelectByInverting,
  DownloadSelectByExtending
} DownloadSelectionBehavior;

// Names of notifications we will post on download-related events
extern NSString* const kDownloadStartedNotificationName;
extern NSString* const kDownloadFailedNotificationName;
extern NSString* const kDownloadCompleteNotificationName;

// Names of keys for objects passed in these notifications
extern NSString* const kDownloadNotificationFilenameKey;
extern NSString* const kDownloadNotificationTimeElapsedKey;

//
// interface ProgressViewController
//
// There is a 1-to-1 correspondence between a download in the manager and
// one of these objects. It implements what you can do with the item as 
// well as managing the download. It holds onto two views, one for while
// the item is downloading, the other for after it has completed.
//
@interface ProgressViewController : NSObject<CHDownloadProgressDisplay, WatchedFileDelegate>
{
  IBOutlet NSProgressIndicator *mProgressBar;

  IBOutlet ProgressView *mProgressView;      // in-progress view, STRONG ref
  IBOutlet ProgressView *mCompletedView;     // completed view, STRONG ref
  
  unsigned int    mUniqueIdentifier;      // unique identifier for a given session

  BOOL            mIsFileSave;
  BOOL            mUserCancelled;
  BOOL            mDownloadFailed;
  BOOL            mDownloadDone;
  BOOL            mRefreshIcon;
  BOOL            mFileExists;
  BOOL            mFileIsWatched;
  BOOL            mIsBeingRemoved;
  BOOL            mIsSelected;

  NSTimeInterval  mDownloadTime; // only set when done

  long long       mCurrentProgress; // if progress bar is indeterminate, can still calc stats.
  long long       mDownloadSize;
  
  NSString        *mSourceURL;
  NSString        *mDestPath;
  NSDate          *mStartTime;
  
  CHDownloader    *mDownloader;             // wraps gecko download, STRONG ref
  
  ProgressDlgController *mProgressWindowController;    // window controller, WEAK ref (owns us)
}

+(NSString *)formatTime:(int)aSeconds;
+(NSString *)formatFuzzyTime:(int)aSeconds;
+(NSString *)formatBytes:(float)aBytes;

-(id)initWithWindowController:(ProgressDlgController*)aWindowController;
-(id)initWithDictionary:(NSDictionary*)aDict 
    andWindowController:(ProgressDlgController*)aWindowController;

-(ProgressView *)view;

-(IBAction)cancel:(id)sender;
-(IBAction)remove:(id)sender;
-(IBAction)reveal:(id)sender;
-(IBAction)open:(id)sender;
-(IBAction)pause:(id)sender;
-(IBAction)resume:(id)sender;

-(BOOL)deleteFile;

-(BOOL)isActive;
-(BOOL)isCanceled;
-(BOOL)isSelected;
-(BOOL)isPaused;
-(BOOL)fileExists;

-(BOOL)hasSucceeded;

// Directly sets the selection of this item (should not be called by the view).
-(void)setSelected:(BOOL)inSelected;

-(NSDictionary*)downloadInfoDictionary;
-(unsigned int)uniqueIdentifier;

-(NSMenu*)contextualMenu;
-(BOOL)shouldAllowAction:(SEL)action;

// Handlers for actions that are triggered by user action on a view but aren't
// specific to the view.
-(void)updateSelectionWithBehavior:(DownloadSelectionBehavior)behavior;
-(void)openSelectedDownloads;
-(void)cancelSelectedDownloads;

@end
