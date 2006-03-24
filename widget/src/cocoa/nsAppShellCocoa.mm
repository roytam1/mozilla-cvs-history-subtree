/* -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
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
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Mentovai <mark@moxienet.com>
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

#import <Cocoa/Cocoa.h>

#include "nsAppShellCocoa.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsString.h"

#define AppDefinedEventLeaveRunLoop 0x4c76 // 'Lv'

nsAppShellCocoa::nsAppShellCocoa()
{
  mainPool = [[NSAutoreleasePool alloc] init];
}

nsAppShellCocoa::~nsAppShellCocoa()
{
  [mainPool release];
}

//-------------------------------------------------------------------------
//
// Create the application shell
//
// There's reallly not a whole lot that needs to be done here. The
// window will register its own interest in the necessary events
// so there's no need for us to create a pump or a sink.
//
//-------------------------------------------------------------------------

NS_IMETHODIMP
nsAppShellCocoa::Init(int* argc, char** argv)
{
  // Get the path of the NIB file, which lives in the GRE location
  nsCOMPtr<nsIFile> nibFile;
  nsresult rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(nibFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nibFile->AppendNative(NS_LITERAL_CSTRING("res"));
  nibFile->AppendNative(NS_LITERAL_CSTRING("MainMenu.nib"));

  nsCAutoString nibPath;
  rv = nibFile->GetNativePath(nibPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // This call initializes NSApplication.  Initialize using AppShell,
  // which provides local overrides.
  [NSBundle loadNibFile:
                [NSString stringWithUTF8String:(const char*)nibPath.get()]
      externalNameTable:
                [NSDictionary dictionaryWithObject:[AppShell sharedApplication]
                                            forKey:@"NSOwner"]
               withZone:NSDefaultMallocZone()];

  return nsBaseAppShell::Init(argc, argv);
}

NS_IMETHODIMP
nsAppShellCocoa::OnDispatchedEvent(nsIThreadInternal* thr)
{
// @@@mm should this check [NSApp isRunning]?
  // Not on the main thread, need an autorelease pool.
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  NSEvent* event = [NSEvent otherEventWithType:NSApplicationDefined
                                      location:NSMakePoint(0, 0)
                                 modifierFlags:0
                                     timestamp:nil
                                  windowNumber:0
                                       context:nil
                                       subtype:AppDefinedEventLeaveRunLoop
                                         data1:0
                                         data2:0];
  [NSApp postEvent:event atStart:YES];
  [pool release];

  return NS_OK;
}

PRBool
nsAppShellCocoa::ProcessNextNativeEvent(PRBool mayWait)
{
  PRBool moreEventsWaiting = PR_FALSE;

  if (!mayWait) {
    if (NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                            untilDate:nil
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES]) {
      [NSApp sendEvent:event];
    }

    if ([NSApp nextEventMatchingMask:NSAnyEventMask
                           untilDate:nil
                              inMode:NSDefaultRunLoopMode
                             dequeue:NO]) {
      moreEventsWaiting = PR_TRUE;
    }
  }
  else {
    [NSApp run];
  }

  return moreEventsWaiting;
}

@implementation AppShell
- (void)sendEvent:(NSEvent*)event
{
  if ([event type] == NSApplicationDefined &&
      [event subtype] == AppDefinedEventLeaveRunLoop) {
    [NSApp stop:self];
  }
  else {
    [super sendEvent:event];
  }
}
@end
