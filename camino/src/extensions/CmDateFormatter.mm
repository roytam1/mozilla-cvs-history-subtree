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
 * The Original Code is Camino code.
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

#import "CmDateFormatter.h"

// Xcode 2.x's ld dead-strips this symbol.  Xcode 3.0's ld is fine.
asm(".no_dead_strip .objc_class_name_CmDateFormatter");

@interface CmDateFormatter (Private)
- (void)ensureCFDateFormatter;
- (void)releaseCFDateFormatter;
@end

@implementation CmDateFormatter

- (void)ensureCFDateFormatter {
  if (!mCFDateFormatter) {
    CFLocaleRef locale = ::CFLocaleCopyCurrent();
    mCFDateFormatter = ::CFDateFormatterCreate(NULL,
                                               locale,
                                               mDateStyle,
                                               mTimeStyle);
    if (locale) {
      ::CFRelease(locale);
    }
  }
}

- (void)releaseCFDateFormatter {
  if (mCFDateFormatter) {
    ::CFRelease(mCFDateFormatter);
    mCFDateFormatter = NULL;
  }
}

- (void)dealloc {
  [self releaseCFDateFormatter];
  [super dealloc];
}

- (NSDateFormatterStyle)dateStyle {
  return (NSDateFormatterStyle)mDateStyle;
}

- (void)setDateStyle:(NSDateFormatterStyle)dateStyle {
  if (mDateStyle != (CFDateFormatterStyle)dateStyle) {
    [self releaseCFDateFormatter];
    mDateStyle = (CFDateFormatterStyle)dateStyle;
  }
}

- (NSDateFormatterStyle)timeStyle {
  return (NSDateFormatterStyle)mTimeStyle;
}

- (void)setTimeStyle:(NSDateFormatterStyle)timeStyle {
  if (mTimeStyle != (CFDateFormatterStyle)timeStyle) {
    [self releaseCFDateFormatter];
    mTimeStyle = (CFDateFormatterStyle)timeStyle;
  }
}

- (NSString*)stringFromDate:(NSDate*)date {
  [self ensureCFDateFormatter];
  NSString* dateString =
      (NSString*)::CFDateFormatterCreateStringWithDate(NULL,
                                                       mCFDateFormatter,
                                                       (CFDateRef)date);
  return [dateString autorelease];
}

- (NSString*)stringForObjectValue:(id)value {
  if (![value isKindOfClass:[NSDate class]]) {
    return [super stringForObjectValue:value];
  }

  return [self stringFromDate:(NSDate*)value];
}

@end
