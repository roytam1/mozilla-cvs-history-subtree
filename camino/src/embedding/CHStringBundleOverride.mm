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
* The Initial Developer of the Original Code is
*   Nick Kreeger
* Portions created by the Initial Developer are Copyright (C) 2007
* the Initial Developer. All Rights Reserved.
*
* Contributor(s):
*   Nick Kreeger <nick.kreeger@park.edu>
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

#import "CHStringBundleOverride.h"
#import "NSString+Gecko.h"
#import "NSString+Utils.h"
#import <Foundation/Foundation.h>

NS_IMPL_ISUPPORTS1(CHStringBundleOverride, nsIStringBundleOverride);

CHStringBundleOverride::CHStringBundleOverride()
{
}

CHStringBundleOverride::~CHStringBundleOverride()
{
}

NS_IMETHODIMP CHStringBundleOverride::GetStringFromName(const nsACString& url, const nsACString& key, nsAString& aRetVal)
{
  /**
   * When gecko requests a localized string from a bundle (i.e. a jar file) it requests it 
   * through the |nsIStringBundle| service. Every time a caller asks the service for a string
   * it checks to see if a override service has been registered (i.e. this class) and asks
   * to see if it wants to replace the chrome string. 
   *
   * If we define a string in Localizable.strings, then we will tell the |nsIStringBundle| 
   * service to replace the string. If not, then the |nsIStringBundle| will use the string
   * defined in the bundled chrome resource.
   */
  @try {
    // Create an autorelease pool, since this may be called on a background thread.
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSString* keyStr = [NSString stringWith_nsACString:key];
    NSString* tableName = [NSString stringWith_nsACString:url];
    // Stip off the chrome:// prefix (9 characters) if it's there
    if ([tableName hasPrefix:@"chrome://"])
      tableName = [tableName substringFromIndex:9];
    NSCharacterSet* replacementSet = [NSCharacterSet characterSetWithCharactersInString:@"/."];
    tableName = [tableName stringByReplacingCharactersInSet:replacementSet
                                                 withString:@"_"];
    NSString* overrideStr = NSLocalizedStringFromTable(keyStr, tableName, nil);  
    if (!overrideStr || [overrideStr isEqualToString:keyStr]) {
      [pool release];
      return NS_ERROR_FAILURE;
    }
      
    [overrideStr assignTo_nsAString:aRetVal];

    [pool release];
  }
  @catch (id exception) {
    // Note that we may leak the autorelease pool if this happens on a
    // background thread, but there's not really a safe way to handle that
    // case; see the discussion at:
    // http://lists.apple.com/archives/objc-language//2007/Aug/msg00023.html
    NSLog(@"Exception caught in CHStringBundleOverride::GetStringFromName %@", exception);
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP CHStringBundleOverride::EnumerateKeysInBundle(const nsACString & url, nsISimpleEnumerator **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
