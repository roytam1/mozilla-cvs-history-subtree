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
 * The Original Code is a Cocoa keyboard-equivalent-processing fake view.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Mark Mentovai <mark@moxienet.com> (Original Author)
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

#ifndef CmDateFormatter_h__
#define CmDateFormatter_h__

#import <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_4  // SDK < 10.4
// Let callers use the NS* constants instead of the kCF* constants to provide
// forward compatibility and isolate them from the Core Foundation innards.
// The system headers on 10.4 do straight NS*-kCF* mappings just like this.
typedef enum {
   NSDateFormatterNoStyle     = kCFDateFormatterNoStyle,
   NSDateFormatterShortStyle  = kCFDateFormatterShortStyle,
   NSDateFormatterMediumStyle = kCFDateFormatterMediumStyle,
   NSDateFormatterLongStyle   = kCFDateFormatterLongStyle,
   NSDateFormatterFullStyle   = kCFDateFormatterFullStyle
} NSDateFormatterStyle;
#endif  // SDK < 10.4

// CmDateFormatter operates like NSDateFormatter does on Tiger and above, when
// NSDateFormatter is configured to use NSDateFormatterBehavior10_4.  This
// class wraps CFDateFormatter, just like the new NSDateFormatter, providing
// access to proper localized date, time, or date-and-time strings of varying
// lengths according to the user's preferences.
//
// This class can be used just like the new NSFormatter, except there is no
// need to call setFormatterBehavior: or setDefaultFormatterBehavior:, because
// NSDateFormatterBehavior10_4 is always selected.  These methods (and the
// associated getters) do not exist.
//
// This implementation only supports the necessary calls to support
// stringFromDate: and stringForObjectValue:.  The locale cannot be changed:
// the user's locale is used if available, otherwise, the system's default
// locale is used.

@interface CmDateFormatter : NSFormatter
{
  CFDateFormatterRef   mCFDateFormatter;
  CFDateFormatterStyle mDateStyle;
  CFDateFormatterStyle mTimeStyle;
}

// Accessors and mutators
- (NSDateFormatterStyle)dateStyle;
- (void)setDateStyle:(NSDateFormatterStyle)dateStyle;
- (NSDateFormatterStyle)timeStyle;
- (void)setTimeStyle:(NSDateFormatterStyle)timeStyle;

// Action
- (NSString*)stringFromDate:(NSDate*)date;

@end

#endif  // CmDateFormatter_h__
