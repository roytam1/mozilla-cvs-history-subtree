/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * IBM Corporation
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 2000
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 14/02/2000   IBM Corp.       Retrieve and test for Bidi character classification codes
 *
 */
#ifdef IBMBIDI

#ifndef nsIUBidiCategory_h__
#define nsIUBidiCategory_h__

#include "nsISupports.h"
#include "nscore.h"
#include "nsIBidi.h"

#ifndef __NS_UBIDICAT__
#define __NS_UBIDICAT__
#endif

   /**
    *  Read ftp://ftp.unicode.org/Public/UNIDATA/ReadMe-Latest.txt
    *  section BIDIRECTIONAL PROPERTIES
    *  for the detailed definition of the following categories
	*
	*  The values here must match the equivalents in %map in
	*	mozilla/intl/unicharutil/tools/genbidicattable.pl
    */

typedef enum {
  eBidiCat_Undefined,
	eBidiCat_L,          /* Left-to-Right               */
	eBidiCat_R,          /* Right-to-Left               */
	eBidiCat_AL,         /* Right-to-Left Arabic        */
	eBidiCat_AN,         /* Arabic Number               */
	eBidiCat_EN,         /* European Number             */
	eBidiCat_ES,         /* European Number Separator   */
	eBidiCat_ET,         /* European Number Terminator  */
	eBidiCat_CS,         /* Common Number Separator     */
	eBidiCat_ON,         /* Other Neutrals              */
	eBidiCat_NSM,        /* Non-Spacing Mark            */
	eBidiCat_BN,         /* Boundary Neutral            */
	eBidiCat_B,          /* Paragraph Separator         */
	eBidiCat_S,          /* Segment Separator           */
	eBidiCat_WS,         /* Whitespace                  */
	eBidiCat_CC = 0xf,   /* Control Code                */
						  /* (internal use only - will never be outputed)	*/
	eBidiCat_LRE = 0x2a, /* Left-to-Right Embedding     */
	eBidiCat_RLE = 0x2b, /* Right-to-Left Embedding     */
	eBidiCat_PDF = 0x2c, /* Pop Directional Formatting  */
	eBidiCat_LRO = 0x2d, /* Left-to-Right Override      */
	eBidiCat_RLO = 0x2e	 /* Right-to-Left Override      */
} eBidiCategory;

static UCharDirection ebc2ucd[15] = {
	U_OTHER_NEUTRAL, /* Placeholder -- there will never be a 0 index value */
	U_LEFT_TO_RIGHT,
	U_RIGHT_TO_LEFT,
	U_RIGHT_TO_LEFT_ARABIC,
	U_ARABIC_NUMBER,
	U_EUROPEAN_NUMBER,
	U_EUROPEAN_NUMBER_SEPARATOR,
	U_EUROPEAN_NUMBER_TERMINATOR,
	U_COMMON_NUMBER_SEPARATOR,
	U_OTHER_NEUTRAL,
	U_DIR_NON_SPACING_MARK,
	U_BOUNDARY_NEUTRAL,
	U_BLOCK_SEPARATOR,
	U_SEGMENT_SEPARATOR,
	U_WHITE_SPACE_NEUTRAL
};

static UCharDirection cc2ucd[5] = {
	U_LEFT_TO_RIGHT_EMBEDDING,
	U_RIGHT_TO_LEFT_EMBEDDING,
	U_POP_DIRECTIONAL_FORMAT,
	U_LEFT_TO_RIGHT_OVERRIDE,
	U_RIGHT_TO_LEFT_OVERRIDE
};
	
/* {D23D2DD0-E2F9-11d3-B6DF-00104B4119F8} */
#define NS_UNICHARBIDIUTIL_CID \
							   { 0xd23d2dd0, 0xe2f9, 0x11d3, \
							   {0xb6, 0xdf, 0x0, 0x10, 0x4b, 0x41, 0x19, 0xf8} }

#define NS_UNICHARBIDIUTIL_PROGID "component://netscape/intl/unicharbidiutil"


/* {49926730-E221-11d3-B6DE-00104B4119F8} */
#define NS_IUBIDICATEGORY_IID \
							  {	0x49926730, 0xe221, 0x11d3, \
							  { 0xb6, 0xde, 0x0, 0x10, 0x4b, 0x41, 0x19, 0xf8} }

class nsIUBidiCategory : public nsISupports {

	public: 

		NS_DEFINE_STATIC_IID_ACCESSOR(NS_IUBIDICATEGORY_IID)

   /**
    * Give a Unichar, return an eBidiCategory
    */
		NS_IMETHOD Get( PRUnichar aChar, eBidiCategory* oResult) = 0 ;

   /**
    * Give a Unichar, and a eBidiCategory, 
    * return PR_TRUE if the Unichar is in that category, 
    * return PR_FALSE, otherwise
    */
		NS_IMETHOD Is( PRUnichar aChar, eBidiCategory aBidiCategory, PRBool* oResult) = 0;

   /**
    * Give a Unichar
    * return PR_TRUE if the Unichar is a Bidi control character (LRE, RLE, PDF, LRO, RLO, LRM, RLM)
    * return PR_FALSE, otherwise
    */
	NS_IMETHOD IsControl( PRUnichar aChar, PRBool* oResult) = 0;

   /**
    * Give a Unichar, return a UCharDirection (compatible with ICU)
    */
		NS_IMETHOD GetICU( PRUnichar aChar, UCharDirection* oResult) = 0 ;

   /**
    * Give a Unichar, return the symmetric equivalent
    */
    NS_IMETHOD SymmSwap( PRUnichar* aChar) = 0 ;

};


/***************************************/
#define IS_RTL_LEVEL(x) (x&1)
#define IS_LTR_LEVEL(x) !IS_RTL_LEVEL(x)

#define CLASS_IS_STRONG(val) ( ( (val) == eBidiCat_L) || ( (val) == eBidiCat_R) || ( (val) == eBidiCat_AL) \
                              || ( (val) == eBidiCat_LRE) || ( (val) == eBidiCat_RLE) \
                              || ( (val) == eBidiCat_LRO) || ( (val) == eBidiCat_RLO) )

#define CLASS_IS_UNRESOLVED(val) ( ( (val) == eBidiCat_Undefined) \
                                 || ( ( (val) >= eBidiCat_ES) && ( (val) <= eBidiCat_ON) ) )

#define CLASS_FOR_EMBEDDING_LEVEL(baseEL) ( (IS_RTL_LEVEL(baseEL) ) ? eBidiCat_R : eBidiCat_L)

struct BidiStruct {
	
	PRInt32        tag;       /* the tag that affected Bidi state                     */
  PRInt32        level;     /* embedding level. '-1' if it's implicit               */
	PRInt32        direction;  
	eBidiCategory  override;  /* Bidi override status                                 */
  PRBool         isWeak;    /* weak embedding level can be popped by PopWeakLevels
                               (This is used for tags that are closed implicitly
                               by other tags: <p>, <h1>, etc.
                               Examples of tags that can't be closed implicitly:
                               <span>, <bdo>                                        */
};

#endif  /* nsIUbidiCategory_h__ */

#endif // IBMBIDI
