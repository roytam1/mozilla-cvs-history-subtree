/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is Crocodile Clips Ltd.
 * Portions created by Crocodile Clips are 
 * Copyright (C) 2001 Crocodile Clips Ltd. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 *    Alex Fritze <alex.fritze@crocodile-clips.com> (original author)
 *
 */


#ifndef __NS_ISVGSTYLEVALUE_H__
#define __NS_ISVGSTYLEVALUE_H__

#include "nsISupports.h"

class nsIStyleRule;
class nsIDocument;

// {BD099C4C-8FA5-47c4-A44E-189B5AA5DBAF}
#define NS_ISVGSTYLEVALUE_IID \
{ 0xbd099c4c, 0x8fa5, 0x47c4, { 0xa4, 0x4e, 0x18, 0x9b, 0x5a, 0xa5, 0xdb, 0xaf } }

class nsISVGStyleValue : public nsISupports
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_ISVGSTYLEVALUE_IID; return iid; }
  
  NS_IMETHOD GetStyleRule(nsIDocument* baseDocument, nsIStyleRule** rule)=0;
};

#endif // __NS_ISVGSTYLEVALUE_H__

