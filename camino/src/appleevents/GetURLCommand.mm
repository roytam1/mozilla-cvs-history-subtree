/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Mozilla browser.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Hyatt <hyatt@mozilla.org> (Original Author)
 *   Sean Murphy <murph@seanmurph.com>
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

#import "GetURLCommand.h"
#import <AppKit/AppKit.h>

#import "MainController.h"
#import "NSString+Utils.h"
#import "NSURL+Utils.h"

static NSString* const kMainControllerIsInitializedKey = @"initialized";

@implementation GetURLCommand

- (id)performDefaultImplementation 
{
  MainController* mainController = (MainController*)[NSApp delegate];

  // We can get here before the application is fully initialized and the previous
  // session is restored.  We want to avoid opening URLs before that happens.
  if ([mainController isInitialized]) {
    NSString* urlString = [self directParameter];
    // If the string has a shell tilde-prefix, standardize the path.
    if ([urlString hasPrefix:@"~"])
      urlString = [urlString stringByStandardizingPath];

    NSURL* url = [NSURL URLWithString:urlString];
    NSString* urlScheme = [url scheme];
    // NSURL will fail to generate schemes from URLs that are not strictly 
    // RFC-compliant, which would allow, e.g., a creative mailto: to bypass
    // the supported protocol check below.  If NSURL does not provide a scheme,
    // fall back to a manual attempt to determine a scheme.
    //
    // Ensure that the string contains a colon (thus the string before the
    // colon is a probable scheme) and that the probable scheme does not
    // contain a slash (which could occur if the colon were in a filename or
    // path component of a POSIX file or a website-like string).
    if (!urlScheme) {
      unsigned int colonLocation = [urlString rangeOfString:@":"].location;
      if ((colonLocation != NSNotFound) &&
          ([urlString rangeOfString:@"/"
                            options:0
                              range:NSMakeRange(0, colonLocation)].location == NSNotFound))
      {
        urlScheme = [urlString substringToIndex:colonLocation];
      }
    }
    urlScheme = [urlScheme lowercaseString];
    // Setting Camino as the default app for protocols we do not handle creates
    // an infinite loop when those URIs are opened; avoid that by only handling
    // supported protocols.  However, we want to make sure to allow
    // website-like strings (www.apple.com) and raw POSIX paths to bypass the
    // protocol check so that they still load (see bug 629850); these will have
    // null schemes and will fail the scheme check unless explicitly excluded.
    if (urlScheme && !([urlScheme isEqualToString:@"http"] ||
                       [urlScheme isEqualToString:@"https"] ||
                       [urlScheme isEqualToString:@"ftp"] ||
                       [urlScheme isEqualToString:@"file"] ||
                       [urlScheme isEqualToString:@"data"] ||
                       [urlScheme isEqualToString:@"about"] ||
                       [urlScheme isEqualToString:@"view-source"]))
    {
      return nil;
    }
    if ([url isFileURL])
      urlString = [[NSURL decodeLocalFileURL:url] absoluteString];
    [mainController showURL:urlString];
  }
  else {
    [self suspendExecution];
    [mainController addObserver:self 
                     forKeyPath:kMainControllerIsInitializedKey
                        options:NSKeyValueObservingOptionNew 
                        context:NULL];
  }
  return nil;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
  if ([keyPath isEqualToString:kMainControllerIsInitializedKey] && [[change objectForKey:NSKeyValueChangeNewKey] boolValue]) {
    [object removeObserver:self forKeyPath:kMainControllerIsInitializedKey];
    // explicitly perform the command's default implementation again, 
    // since |resumeExecutionWithResult:| will not.
    id result = [self executeCommand];
    [self resumeExecutionWithResult:result];
  }
}

- (void)dealloc
{
  [super dealloc];
}

@end
