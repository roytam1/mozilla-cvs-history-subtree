/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.  Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *
 */
#ifdef IBMBIDI

#ifndef nsIUBidiUtils_h__
#define nsIUBidiUtils_h__

#include "nsISupports.h"
#include "nscore.h"
#include "nsIBidi.h"

// IBMBIDI - IBM EGYPT - Start
#include "nscore.h"
#include "nsIFontMetrics.h"
#include "nsIFontEnumerator.h"
#include "nsFont.h"
#include "nsICharRepresentable.h"
// IBMBIDI - Egypt - Start
#include "nsTextTransformer.h"
//#include "nsBidiOptions.h"
//#include "nsIUBidiUtils.h"
//class nsAutoTextBuffer;
// IBMBIDI - Egypt - End

// IBMBIDI - IBM EGYPT - End


//#ifndef __NS_UBIDICAT__
//#define __NS_UBIDICAT__
//#endif

   /**
    *  Read ftp://ftp.unicode.org/Public/UNIDATA/ReadMe-Latest.txt
    *  section BIDIRECTIONAL PROPERTIES
    *  for the detailed definition of the following categories
    *
    *  The values here must match the equivalents in %map in
    * mozilla/intl/unicharutil/tools/genbidicattable.pl
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
                       /* (internal use only - will never be outputed) */
  eBidiCat_LRE = 0x2a, /* Left-to-Right Embedding     */
  eBidiCat_RLE = 0x2b, /* Right-to-Left Embedding     */
  eBidiCat_PDF = 0x2c, /* Pop Directional Formatting  */
  eBidiCat_LRO = 0x2d, /* Left-to-Right Override      */
  eBidiCat_RLO = 0x2e  /* Right-to-Left Override      */
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
#define NS_IUBIDIUTILS_IID \
                { 0x49926730, 0xe221, 0x11d3, \
                { 0xb6, 0xde, 0x0, 0x10, 0x4b, 0x41, 0x19, 0xf8} }

class nsIUBidiUtils : public nsISupports {

  public: 

    NS_DEFINE_STATIC_IID_ACCESSOR(NS_IUBIDIUTILS_IID)

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



// IBMBIDI - EGYPT - Start
	virtual void HebrewReordering(const PRUnichar *aString, PRUint32 aLen,
							PRUnichar* aBuf, PRUint32 &aBufLen)=0;
	virtual void ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
             PRUnichar* aBuf, PRUint32 &aBufLen, PRUint32* map)=0;
	virtual PRBool NeedComplexScriptHandling(const PRUnichar *aString, PRUint32 aLen,
       PRBool bFontSupportHebrew, PRBool* oHebrew,
       PRBool bFontSupportArabic, PRBool* oArabic)=0;

	virtual void numbers_to_arabic (PRUnichar* uch)=0;
	virtual void numbers_to_hindi (PRUnichar* uch)=0;
	virtual void HandleNumbers (PRUnichar* Buffer, PRUint32 size, PRUint32  Num_Flag)=0;
	virtual void Conv_FE_06 (const nsString aSrc, nsString & aDst) = 0;
//ahmed	
 virtual void Conv_FE_06_WithReverse (const nsString aSrc, nsString & aDst) = 0;
 virtual void Conv_06_FE_WithReverse (const nsString aSrc, nsString & aDst,PRUint32* map) = 0;
 virtual PRBool NeedReverseHandling(const PRUnichar *aString, PRUint32 aLen)=0;
 virtual void GetSystem()=0;
 virtual PRBool IsWin95()=0;
 virtual PRBool IsWin98()=0;
 virtual PRBool IsWinNT()=0;
// IBMBIDI - EGYPT - End
};

#define IS_RTL_LEVEL(x) (x&1)
#define IS_LTR_LEVEL(x) !IS_RTL_LEVEL(x)

#define CLASS_IS_RTL(val) ( ( (val) == U_RIGHT_TO_LEFT) || ( (val) == U_RIGHT_TO_LEFT_ARABIC) )

#define CLASS_IS_WEAK(val) ( ( (val) == U_EUROPEAN_NUMBER_SEPARATOR)    \
                            || ( (val) == U_EUROPEAN_NUMBER_TERMINATOR) \
                            || ( ( (val) > U_ARABIC_NUMBER) && ( (val) != U_RIGHT_TO_LEFT_ARABIC) ) )
#ifndef nsBidiOptions_h___
#define nsBidiOptions_h___

// --------------------------------------------------
// IBMBIDI 
// --------------------------------------------------
//
// These values are shared with Preferences dialog
//  ------------------
//  If Pref values are to be changed
//  in the XUL file of Prefs. the values
//  Must be changed here too..
//	------------------
//
#define IBMBIDI_TEXTDIRECTION_STR				"bidi.direction"
#define IBMBIDI_TEXTTYPE_STR						"bidi.texttype"
#define IBMBIDI_CONTROLSTEXTMODE_STR		"bidi.controlstextmode"
#define IBMBIDI_CLIPBOARDTEXTMODE_STR		"bidi.clipboardtextmode"
#define IBMBIDI_NUMERAL_STR							"bidi.numeral"
#define IBMBIDI_SUPPORTMODE_STR					"bidi.support"
#define IBMBIDI_CHARSET_STR							"bidi.characterset"

#define IBMBIDI_TEXTDIRECTION				1
#define IBMBIDI_TEXTTYPE						2
#define IBMBIDI_CONTROLSTEXTMODE		3
#define IBMBIDI_CLIPBOARDTEXTMODE		4
#define IBMBIDI_NUMERAL							5
#define IBMBIDI_SUPPORTMODE					6
#define IBMBIDI_CHARSET							7

//	------------------
// 	Text Direction
//	------------------
//	bidi.direction
#define IBMBIDI_TEXTDIRECTION_LTR			1	// 	1 = directionLTRBidi *
#define IBMBIDI_TEXTDIRECTION_RTL			2	//	2 = directionRTLBidi
//	------------------
// 	Text Type
//	------------------
//	bidi.texttype
#define IBMBIDI_TEXTTYPE_CHARSET			1	// 	1 = charsettexttypeBidi *
#define IBMBIDI_TEXTTYPE_LOGICAL			2	//	2 = logicaltexttypeBidi
#define IBMBIDI_TEXTTYPE_VISUAL				3	//	3 = visualtexttypeBidi
//	------------------
// 	Controls Text Mode
//	------------------
//	bidi.controlstextmode
#define IBMBIDI_CONTROLSTEXTMODE_LOGICAL	1	// 	1 = logicalcontrolstextmodeBidiCmd
#define IBMBIDI_CONTROLSTEXTMODE_VISUAL		2	//	2 = visiualcontrolstextmodeBidi
#define IBMBIDI_CONTROLSTEXTMODE_CONTAINER	3	//	3 = containercontrolstextmodeBidi *
//	------------------
// 	Clipboard Text Mode
//	------------------
//	bidi.clipboardtextmode
#define IBMBIDI_CLIPBOARDTEXTMODE_LOGICAL	1	// 	1 = logicalclipboardtextmodeBidi
#define IBMBIDI_CLIPBOARDTEXTMODE_VISUAL	2	//	2 = visiualclipboardtextmodeBidi
#define IBMBIDI_CLIPBOARDTEXTMODE_SOURCE	3	//	3 = sourceclipboardtextmodeBidi *
//	------------------
// 	Numeral Style
//	------------------
//	bidi.numeral
#define IBMBIDI_NUMERAL_REGULAR				1	// 	1 = regularcontextnumeralBidi *
#define IBMBIDI_NUMERAL_HINDICONTEXT		2	//	2 = hindicontextnumeralBidi
#define IBMBIDI_NUMERAL_ARABIC				3	//	3 = arabicnumeralBidi
#define IBMBIDI_NUMERAL_HINDI				4	//	4 = hindinumeralBidi
//	------------------
// 	Support Mode
//	------------------
//	bidi.support
#define IBMBIDI_SUPPORTMODE_MOZILLA			1	// 	1 = mozillaBidisupport *
#define IBMBIDI_SUPPORTMODE_OSBIDI			2	//	2 = OsBidisupport
#define IBMBIDI_SUPPORTMODE_DISABLE			3	//	3 = disableBidisupport
//	------------------
// 	Charset Mode
//	------------------
//	bidi.characterset
#define IBMBIDI_CHARSET_BIDI				1	// 	1 = doccharactersetBidi *
#define IBMBIDI_CHARSET_DEFAULT				2	//	2 = defaultcharactersetBidi

struct nsBidiOptions
{
  PRUint8 mdirection;
  PRUint8 mtexttype;
  PRUint8 mcontrolstextmode;
  PRUint8 mclipboardtextmode;
  PRUint8 mnumeral;
  PRUint8 msupport;
  PRUint8 mcharacterset;
};

/* Constants related to the position of numerics in the codepage */
#define START_ARABIC_DIGITS     0x30
#define END_ARABIC_DIGITS       0x39
#define START_HINDI_DIGITS      0xB0
#define END_HINDI_DIGITS        0xB9
#define DIGIT_INCREMENT         0x80

#define BIDI_START_HINDI_DIGITS   0x0660
#define BIDI_END_HINDI_DIGITS     0x0669
#define BIDI_START_ARABIC_DIGITS  0x0030
#define BIDI_END_ARABIC_DIGITS    0x0039
#define BIDI_DIGIT_INCREMENT      0x0630

// the Array Index = FE_CHAR - FE_TO_06_OFFSET

#define FE_TO_06_OFFSET 0xfe70

static PRUint16  FE_TO_06 [][2] = {
{0x064a,0x0000},{0x064a,0x0640},{0x064c,0x0000},
{0x0000,0x0000},{0x064d,0x0000},{0x0000,0x0000},
{0x064e,0x0000},{0x064e,0x0640},{0x064f,0x0000},
{0x064f,0x0640},{0x0650,0x0000},{0x0650,0x0640},
{0x0651,0x0000},{0x0651,0x0640},{0x0652,0x0000},
{0x0652,0x0640},{0x0621,0x0000},{0x0622,0x0000},
{0x0622,0x0000},{0x0623,0x0000},{0x0623,0x0000},
{0x0624,0x0000},{0x0624,0x0000},{0x0625,0x0000},
{0x0625,0x0000},{0x0626,0x0000},{0x0626,0x0000},
{0x0626,0x0000},{0x0626,0x0000},{0x0627,0x0000},
{0x0627,0x0000},{0x0628,0x0000},{0x0628,0x0000},
{0x0628,0x0000},{0x0628,0x0000},{0x0629,0x0000},
{0x0629,0x0000},{0x062a,0x0000},{0x062a,0x0000},
{0x062a,0x0000},{0x062a,0x0000},{0x062b,0x0000},
{0x062b,0x0000},{0x062b,0x0000},{0x062b,0x0000},
{0x062c,0x0000},{0x062c,0x0000},{0x062c,0x0000},
{0x062c,0x0000},{0x062d,0x0000},{0x062d,0x0000},
{0x062d,0x0000},{0x062d,0x0000},{0x062e,0x0000},
{0x062e,0x0000},{0x062e,0x0000},{0x062e,0x0000},
{0x062f,0x0000},{0x062f,0x0000},{0x0630,0x0000},
{0x0630,0x0000},{0x0631,0x0000},{0x0631,0x0000},
{0x0632,0x0000},{0x0632,0x0000},{0x0633,0x0000},
{0x0633,0x0000},{0x0633,0x0000},{0x0633,0x0000},
{0x0634,0x0000},{0x0634,0x0000},{0x0634,0x0000},
{0x0634,0x0000},{0x0635,0x0000},{0x0635,0x0000},
{0x0635,0x0000},{0x0635,0x0000},{0x0636,0x0000},
{0x0636,0x0000},{0x0636,0x0000},{0x0636,0x0000},
{0x0637,0x0000},{0x0637,0x0000},{0x0637,0x0000},
{0x0637,0x0000},{0x0638,0x0000},{0x0638,0x0000},
{0x0638,0x0000},{0x0638,0x0000},{0x0639,0x0000},
{0x0639,0x0000},{0x0639,0x0000},{0x0639,0x0000},
{0x063a,0x0000},{0x063a,0x0000},{0x063a,0x0000},
{0x063a,0x0000},{0x0641,0x0000},{0x0641,0x0000},
{0x0641,0x0000},{0x0641,0x0000},{0x0642,0x0000},
{0x0642,0x0000},{0x0642,0x0000},{0x0642,0x0000},
{0x0643,0x0000},{0x0643,0x0000},{0x0643,0x0000},
{0x0643,0x0000},{0x0644,0x0000},{0x0644,0x0000},
{0x0644,0x0000},{0x0644,0x0000},{0x0645,0x0000},
{0x0645,0x0000},{0x0645,0x0000},{0x0645,0x0000},
{0x0646,0x0000},{0x0646,0x0000},{0x0646,0x0000},
{0x0646,0x0000},{0x0647,0x0000},{0x0647,0x0000},
{0x0647,0x0000},{0x0647,0x0000},{0x0648,0x0000},
{0x0648,0x0000},{0x0649,0x0000},{0x0649,0x0000},
{0x064a,0x0000},{0x064a,0x0000},{0x064a,0x0000},
{0x064a,0x0000},{0x0644,0x0622},{0x0644,0x0622},
{0x0644,0x0623},{0x0644,0x0623},{0x0644,0x0625},
{0x0644,0x0625},{0x0644,0x0627},{0x0644,0x0627},
{0x0000,0x0000},{0x0000,0x0000},{0x0000,0x0000}
};

#define IS_DIACRITIC(u)	( \
	( (u) >= 0x0591 && (u) <= 0x05A1) || ( (u) >= 0x05A3 && (u) <= 0x05B9) \
		|| ( (u) >= 0x05BB && (u) <= 0x05BD) || ( (u) == 0x05BF) || ( (u) == 0x05C1) \
		|| ( (u) == 0x05C2) || ( (u) == 0x05C4) \
		|| ( (u) >= 0x064B && (u) <= 0x0652) || ( (u) == 0x0670) \
		|| ( (u) >= 0x06D7 && (u) <= 0x06E4) || ( (u) == 0x06E7) || ( (u) == 0x06E8) \
		|| ( (u) >= 0x06EA && (u) <= 0x06ED) )

#define IS_HINDI_DIGIT(u)		( ( (u) >= BIDI_START_HINDI_DIGITS )	&& ( (u) <= BIDI_END_HINDI_DIGITS ) )
#define IS_ARABIC_DIGIT(u)	( ( (u) >= BIDI_START_ARABIC_DIGITS ) && ( (u) <= BIDI_END_ARABIC_DIGITS ) )
#define IS_HEBREW_CHAR(c) ((0x0590 <= (c)) && ((c)<= 0x05FF))
#define IS_06_CHAR(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))
#define IS_FE_CHAR(c) ((0xfe70 <= (c)) && ((c)<= 0xfeFF))
//#define IS_ARABIC_CHAR(c) (IS_06_CHAR(c) && IS_FE_CHAR(c))
#define IS_ARABIC_CHAR(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))

#ifdef FONT_HAS_GLYPH
#undef FONT_HAS_GLYPH
#endif
//#define FONT_HAS_GLYPH(map, g) (((map)[(g) >> 3] >> ((g) & 7)) & 1)
#define FONT_HAS_GLYPH(map, g) IS_REPRESENTABLE(map, g)

#ifdef ADD_GLYPH
#undef ADD_GLYPH
#endif
//#define ADD_GLYPH(map, g) (map)[(g) >> 3] |= (1 << ((g) & 7))
#define ADD_GLYPH(map, g) SET_REPRESENTABLE(map, g)

#define HAS_ARABIC_PRESENTATION_FORM_B(font) (FONT_HAS_GLYPH((font)->mMap, 0xFE81))
#define HAS_HEBREW_GLYPH(font)               (FONT_HAS_GLYPH((font)->mMap, 0x05D0))

#define CHAR_IS_HEBREW(c) ((0x0590 <= (c)) && ((c)<= 0x05FF))
#define CHAR_IS_ARABIC(c) ((0x0600 <= (c)) && ((c)<= 0x06FF))

//============ Begin Arabic Basic to Presentation Form B Code ============

static PRUint8 gArabicMap1[] = {
            0x81, 0x83, 0x85, 0x87, 0x89, 0x8D, // 0622-0627
0x8F, 0x93, 0x95, 0x99, 0x9D, 0xA1, 0xA5, 0xA9, // 0628-062F
0xAB, 0xAD, 0xAF, 0xB1, 0xB5, 0xB9, 0xBD, 0xC1, // 0630-0637
0xC5, 0xC9, 0xCD                                // 0638-063A
};

static PRUint8 gArabicMap2[] = {
      0xD1, 0xD5, 0xD9, 0xDD, 0xE1, 0xE5, 0xE9, // 0641-0647
0xED, 0xEF, 0xF1                                // 0648-064A
};

#define PresentationFormB(c, form)                           \
  (((0x0622<=(c)) && ((c)<=0x063A)) ?                        \
    (0xFE00|(gArabicMap1[(c)-0x0622] + (form))) :            \
     (((0x0641<=(c)) && ((c)<=0x064A)) ?                     \
      (0xFE00|(gArabicMap2[(c)-0x0641] + (form))) : (c)))

static enum {
   eIsolated,  // or Char N
   eFinal,     // or Char R
   eInitial,   // or Char L
   eMedial,    // or Char M
} eArabicForm;
static enum {
   eTr = 0, // Transparent
   eRJ = 1, // Right-Joining
   eLJ = 2, // Left-Joining
   eDJ = 3, // Dual-Joining
   eNJ  = 4,// Non-Joining
   eJC = 7, // Joining Causing
   eRightJCMask = 2, // bit of Right-Join Causing 
   eLeftJCMask = 1   // bit of Left-Join Causing 
} eArabicJoiningClass;

#define RightJCClass(j) (eRightJCMask&(j))
#define LeftJCClass(j)  (eLeftJCMask&(j))

#define DecideForm(jl,j,jr)                                 \
  (((eRJ == (j)) && RightJCClass(jr)) ? eFinal              \
                                      :                     \
   ((eDJ == (j)) ?                                          \
    ((RightJCClass(jr)) ?                                   \
     (((LeftJCClass(jl)) ? eMedial                          \
                         : eFinal))                         \
                        :                                   \
     (((LeftJCClass(jl)) ? eInitial                         \
                         : eIsolated))                      \
    )                     : eIsolated))                     \
  

static PRInt8 gJoiningClass[] = {
          eRJ, eRJ, eRJ, eRJ, eDJ, eRJ, // 0620-0627
eDJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, eRJ, // 0628-062F
eRJ, eRJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0630-0637
eDJ, eDJ, eDJ, eNJ, eNJ, eNJ, eNJ, eNJ, // 0638-063F
eJC, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, // 0640-0647
eRJ, eRJ, eDJ, eTr, eTr, eTr, eTr, eTr, // 0648-064F
eTr, eTr, eTr                           // 0650-0652
};

#define GetJoiningClass(c)                   \
  (((0x0622 <= (c)) && ((c) <= 0x0652)) ?    \
       (gJoiningClass[(c) - 0x0622]) :       \
      ((0x200D == (c)) ? eJC : eTr))

static PRUint16 gArabicLigatureMap[] = 
{
0x82DF, // 0xFE82 0xFEDF -> 0xFEF5
0x82E0, // 0xFE82 0xFEE0 -> 0xFEF6
0x84DF, // 0xFE84 0xFEDF -> 0xFEF7
0x84E0, // 0xFE84 0xFEE0 -> 0xFEF8
0x88DF, // 0xFE88 0xFEDF -> 0xFEF9
0x88E0, // 0xFE88 0xFEE0 -> 0xFEFA
0x8EDF, // 0xFE8E 0xFEDF -> 0xFEFB
0x8EE0  // 0xFE8E 0xFEE0 -> 0xFEFC
};


#endif // nsBidiOptions_h___

#endif  /* nsIUbidiUtils_h__ */

#endif // IBMBIDI
