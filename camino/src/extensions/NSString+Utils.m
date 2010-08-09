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
 * The Original Code is Chimera code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Simon Fraser <sfraser@netscape.com>
 *   David Haas   <haasd@cae.wisc.edu>
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

#import <AppKit/AppKit.h>		// for NSStringDrawing.h

#import "NSString+Utils.h"


@implementation NSString (ChimeraStringUtils)

+ (id)ellipsisString
{
  static NSString* sEllipsisString = nil;
  if (!sEllipsisString) {
    unichar ellipsisChar = 0x2026;
    sEllipsisString = [[NSString alloc] initWithCharacters:&ellipsisChar length:1];
  }

  return sEllipsisString;
}

+ (NSString*)stringWithUUID
{
  NSString* uuidString = nil;
  CFUUIDRef newUUID = CFUUIDCreate(kCFAllocatorDefault);
  if (newUUID) {
    uuidString = (NSString *)CFUUIDCreateString(kCFAllocatorDefault, newUUID);
    CFRelease(newUUID);
  }
  return [uuidString autorelease];
}

- (BOOL)isEqualToStringIgnoringCase:(NSString*)inString
{
  return ([self compare:inString options:NSCaseInsensitiveSearch] == NSOrderedSame);
}

- (BOOL)hasCaseInsensitivePrefix:(NSString*)inString
{
  if ([self length] < [inString length])
    return NO;
  return ([self compare:inString options:NSCaseInsensitiveSearch range:NSMakeRange(0, [inString length])] == NSOrderedSame);
}

- (BOOL)isLooselyValidatedURI
{
  return ([self hasCaseInsensitivePrefix:@"javascript:"] || [self hasCaseInsensitivePrefix:@"data:"]);
}

- (BOOL)isPotentiallyDangerousURI
{
  return ([self hasCaseInsensitivePrefix:@"javascript:"] || [self hasCaseInsensitivePrefix:@"data:"]);
}

- (BOOL)isValidURI
{
  // This will only return a non-nil object for valid, well-formed URI strings
  NSURL* testURL = [NSURL URLWithString:self];

  // |javascript:| and |data:| URIs might not have passed the test,
  // but spaces will work OK, so evaluate them separately.
  if ((testURL) || [self isLooselyValidatedURI]) {
    return YES;
  }
  return NO;
}

- (NSString *)stringByRemovingCharactersInSet:(NSCharacterSet*)characterSet
{
  NSScanner*       cleanerScanner = [NSScanner scannerWithString:self];
  NSMutableString* cleanString    = [NSMutableString stringWithCapacity:[self length]];
  // Make sure we don't skip whitespace, which NSScanner does by default
  [cleanerScanner setCharactersToBeSkipped:[NSCharacterSet characterSetWithCharactersInString:@""]];

  while (![cleanerScanner isAtEnd]) {
    NSString* stringFragment;
    if ([cleanerScanner scanUpToCharactersFromSet:characterSet intoString:&stringFragment])
      [cleanString appendString:stringFragment];

    [cleanerScanner scanCharactersFromSet:characterSet intoString:nil];
  }

  return cleanString;
}

- (NSString *)stringByReplacingCharactersInSet:(NSCharacterSet*)characterSet
                                    withString:(NSString*)string
{
  NSScanner*       cleanerScanner = [NSScanner scannerWithString:self];
  NSMutableString* cleanString    = [NSMutableString stringWithCapacity:[self length]];
  // Make sure we don't skip whitespace, which NSScanner does by default
  [cleanerScanner setCharactersToBeSkipped:[NSCharacterSet characterSetWithCharactersInString:@""]];

  while (![cleanerScanner isAtEnd])
  {
    NSString* stringFragment;
    if ([cleanerScanner scanUpToCharactersFromSet:characterSet intoString:&stringFragment])
      [cleanString appendString:stringFragment];

    if ([cleanerScanner scanCharactersFromSet:characterSet intoString:nil])
      [cleanString appendString:string];
  }

  return cleanString;
}

- (NSString*)stringByTruncatingTo:(unsigned int)maxCharacters at:(ETruncationType)truncationType
{
  if ([self length] > maxCharacters)
  {
    NSMutableString *mutableCopy = [self mutableCopy];
    [mutableCopy truncateTo:maxCharacters at:truncationType];
    return [mutableCopy autorelease];
  }

  return self;
}

- (NSString *)stringByTruncatingToWidth:(float)inWidth at:(ETruncationType)truncationType
                         withAttributes:(NSDictionary *)attributes
{
  if ([self sizeWithAttributes:attributes].width > inWidth)
  {
    NSMutableString *mutableCopy = [self mutableCopy];
    [mutableCopy truncateToWidth:inWidth at:truncationType withAttributes:attributes];
    return [mutableCopy autorelease];
  }

  return self;
}

- (NSString *)stringByTrimmingWhitespace
{
  return [self stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
}

-(NSString *)stringByRemovingAmpEscapes
{
  NSMutableString* dirtyStringMutant = [NSMutableString stringWithString:self];
  [dirtyStringMutant replaceOccurrencesOfString:@"&amp;"
                                     withString:@"&"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"&quot;"
                                     withString:@"\""
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"&lt;"
                                     withString:@"<"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"&gt;"
                                     withString:@">"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"&mdash;"
                                     withString:@"-"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"&apos;"
                                     withString:@"'"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  // fix import from old Firefox versions, which exported &#39; instead of a plain apostrophe
  [dirtyStringMutant replaceOccurrencesOfString:@"&#39;"
                                     withString:@"'"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  return [dirtyStringMutant stringByRemovingCharactersInSet:[NSCharacterSet controlCharacterSet]];
}

-(NSString *)stringByAddingAmpEscapes
{
  NSMutableString* dirtyStringMutant = [NSMutableString stringWithString:self];
  [dirtyStringMutant replaceOccurrencesOfString:@"&"
                                     withString:@"&amp;"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"\""
                                     withString:@"&quot;"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@"<"
                                     withString:@"&lt;"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  [dirtyStringMutant replaceOccurrencesOfString:@">"
                                     withString:@"&gt;"
                                        options:NSLiteralSearch
                                          range:NSMakeRange(0,[dirtyStringMutant length])];
  return [NSString stringWithString:dirtyStringMutant];
}

@end


@implementation NSMutableString (ChimeraMutableStringUtils)

- (void)truncateTo:(unsigned)maxCharacters at:(ETruncationType)truncationType
{
  if ([self length] <= maxCharacters)
    return;

  NSRange replaceRange;
  replaceRange.length = [self length] - maxCharacters;

  switch (truncationType) {
    case kTruncateAtStart:
      replaceRange.location = 0;
      break;

    case kTruncateAtMiddle:
      replaceRange.location = maxCharacters / 2;
      break;

    case kTruncateAtEnd:
      replaceRange.location = maxCharacters;
      break;

    default:
#if DEBUG
      NSLog(@"Unknown truncation type in stringByTruncatingTo::");
#endif
      replaceRange.location = maxCharacters;
      break;
  }

  [self replaceCharactersInRange:replaceRange withString:[NSString ellipsisString]];
}


- (void)truncateToWidth:(float)maxWidth
                     at:(ETruncationType)truncationType
         withAttributes:(NSDictionary *)attributes
{
  // First check if we have to truncate at all.
  if ([self sizeWithAttributes:attributes].width <= maxWidth)
    return;

  // Essentially, we perform a binary search on the string length
  // which fits best into maxWidth.

  float width = maxWidth;
  int lo = 0;
  int hi = [self length];
  int mid;

  // Make a backup copy of the string so that we can restore it if we fail low.
  NSMutableString *backup = [self mutableCopy];

  while (hi >= lo) {
    mid = (hi + lo) / 2;

    // Cut to mid chars and calculate the resulting width
    [self truncateTo:mid at:truncationType];
    width = [self sizeWithAttributes:attributes].width;

    if (width > maxWidth) {
      // Fail high - string is still to wide. For the next cut, we can simply
      // work on the already cut string, so we don't restore using the backup.
      hi = mid - 1;
    }
    else if (width == maxWidth) {
      // Perfect match, abort the search.
      break;
    }
    else {
      // Fail low - we cut off too much. Restore the string before cutting again.
      lo = mid + 1;
      [self setString:backup];
    }
  }
  // Perform the final cut (unless this was already a perfect match).
  if (width != maxWidth)
    [self truncateTo:hi at:truncationType];
  [backup release];
}

@end

@implementation NSString (ChimeraFilePathStringUtils)

- (NSString*)volumeNamePathComponent
{
  // if the file doesn't exist, then componentsToDisplayForPath will return nil,
  // so back up to the nearest existing dir
  NSString* curPath = self;
  while (![[NSFileManager defaultManager] fileExistsAtPath:curPath])
  {
    NSString* parentDirPath = [curPath stringByDeletingLastPathComponent];
    if ([parentDirPath isEqualToString:curPath])
      break;  // avoid endless loop
    curPath = parentDirPath;
  }

  NSArray* displayComponents = [[NSFileManager defaultManager] componentsToDisplayForPath:curPath];
  if ([displayComponents count] > 0)
    return [displayComponents objectAtIndex:0];

  return self;
}

- (NSString*)displayNameOfLastPathComponent
{
  return [[NSFileManager defaultManager] displayNameAtPath:self];
}

@end

@implementation NSString (CaminoURLStringUtils)

- (BOOL)isBlankURL
{
  return ([self isEqualToString:@"about:blank"] || [self isEqualToString:@""]);
}

// Excluded character list comes from RFC2396, by examining Safari's behaviour,
// and from the characters Firefox excluded in bug 397815, bug 410726, and
// bug 452979.
- (NSString*)unescapedURI
{
  // The Xcode toolchain on 10.4 does not like Unicode string literals, even as
  // escape sequences; once we only build on 10.5+, we can replace the format
  // string with a string using escape sequences.
  static NSString *charactersNotToUnescape = nil;
  if (!charactersNotToUnescape) {
    NSString *excludedAsciiChars = @"% \"\';/?:@&=+$,#";
    unichar excludedUnicodeChars[] = {0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009,
                                      0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011, 0x0012, 0x0013,
                                      0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d,
                                      0x001e, 0x001f, 0x00a0, 0x00ad, 0x2000, 0x2002, 0x2003, 0x2004, 0x2005, 0x2006,
                                      0x2007, 0x2008, 0x2009, 0x200a, 0x200b, 0x200e, 0x200f, 0x2028, 0x2029, 0x202a,
                                      0x202b, 0x202c, 0x202d, 0x202e, 0x202f, 0x205f, 0x2060, 0x2062, 0x2063, 0x3000,
                                      0xfeff, 0xfffc, 0xfffe, 0xffff};
    unsigned int excludedUnicodeCount = sizeof(excludedUnicodeChars) / sizeof(unichar);
    NSMutableString *characters = [NSMutableString stringWithCapacity:(excludedUnicodeCount + [excludedAsciiChars length])];
    [characters appendString:excludedAsciiChars];
    for (unsigned int i = 0; i < excludedUnicodeCount; ++i) {
      [characters appendFormat:@"%C", excludedUnicodeChars[i]];
    }
    charactersNotToUnescape = [characters copy];
  }
  
  NSString *unescapedURI = (NSString*)CFURLCreateStringByReplacingPercentEscapesUsingEncoding(kCFAllocatorDefault,
                                                                            (CFStringRef)self,
                                                                            (CFStringRef)charactersNotToUnescape,
                                                                            kCFStringEncodingUTF8);
  return unescapedURI ? [unescapedURI autorelease] : self;
}

@end
