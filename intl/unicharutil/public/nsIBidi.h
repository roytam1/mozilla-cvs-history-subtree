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
 * Corporation.   Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * IBM Corporation
 *
 * This module is based on the ICU (International Components for Unicode)
 *
 *   Copyright (C) 2000, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 */
#ifdef IBMBIDI

#ifndef nsIBidi_h__
#define nsIBidi_h__

#include "nsISupports.h"
#include "prtypes.h"
// IBMBIDI - Egypt - Start
#include "nsTextTransformer.h"
// IBMBIDI - Egypt - End

#define NS_BIDI_CID \
          { 0xd9123b91, 0xf4f2, 0x11d3, \
          {0xb6, 0xf1, 0x0, 0x10, 0x4b, 0x41, 0x19, 0xf8} }

#define NS_BIDI_PROGID "component://netscape/intl/unicharbidiutil"

#define NS_BIDI_IID \
          { 0xd9123b92, 0xf4f2, 0x11d3, \
          { 0xb6, 0xf1, 0x0, 0x10, 0x4b, 0x41, 0x19, 0xf8 } }

/*
 * javadoc-style comments are intended to be transformed into HTML
 * using DOC++ - see
 * http://www.zib.de/Visual/software/doc++/index.html .
 *
 * The HTML documentation is created with
 *  doc++ -H nsIBidi.h
 */

/**
 * @mainpage BIDI algorithm for Mozilla (from ICU)
 *
 * <h2>BIDI algorithm for Mozilla</h2>
 *
 * This is an implementation of the Unicode Bidirectional algorithm.
 * The algorithm is defined in the
 * <a href="http://www.unicode.org/unicode/reports/tr9/">Unicode Technical Report 9</a>,
 * version 5, also described in The Unicode Standard, Version 3.0 .<p>
 *
 * <h3>General remarks about the API:</h3>
 *
 * The <quote>limit</quote> of a sequence of characters is the position just after their
 * last character, i.e., one more than that position.<p>
 *
 * Some of the API functions provide access to <quote>runs</quote>.
 * Such a <quote>run</quote> is defined as a sequence of characters
 * that are at the same embedding level
 * after performing the BIDI algorithm.<p>
 *
 * @author Markus W. Scherer. Ported to Mozilla by Simon Montagu
 * @version 1.0
 */

/**
 * UBidiLevel is the type of the level values in this
 * Bidi implementation.
 * It holds an embedding level and indicates the visual direction
 * by its bit 0 (even/odd value).<p>
 *
 * It can also hold non-level values for the
 * <code>paraLevel</code> and <code>embeddingLevels</code>
 * arguments of <code>setPara</code>; there:
 * <ul>
 * <li>bit 7 of an <code>embeddingLevels[]</code>
 * value indicates whether the using application is
 * specifying the level of a character to <i>override</i> whatever the
 * Bidi implementation would resolve it to.</li>
 * <li><code>paraLevel</code> can be set to the
 * pseudo-level values <code>UBIDI_DEFAULT_LTR</code>
 * and <code>UBIDI_DEFAULT_RTL</code>.</li></ul>
 *
 * @see nsIBidi::setPara
 *
 * <p>The related constants are not real, valid level values.
 * <code>UBIDI_DEFAULT_XXX</code> can be used to specify
 * a default for the paragraph level for
 * when the <code>setPara</code> function
 * shall determine it but there is no
 * strongly typed character in the input.<p>
 *
 * Note that the value for <code>UBIDI_DEFAULT_LTR</code> is even
 * and the one for <code>UBIDI_DEFAULT_RTL</code> is odd,
 * just like with normal LTR and RTL level values -
 * these special values are designed that way. Also, the implementation
 * assumes that UBIDI_MAX_EXPLICIT_LEVEL is odd.
 *
 * @see UBIDI_DEFAULT_LTR
 * @see UBIDI_DEFAULT_RTL
 * @see UBIDI_LEVEL_OVERRIDE
 * @see UBIDI_MAX_EXPLICIT_LEVEL
 */
   typedef PRUint8 UBidiLevel;

/** Paragraph level setting.
 *  If there is no strong character, then set the paragraph level to 0 (left-to-right).
 */
#define UBIDI_DEFAULT_LTR 0xfe

/** Paragraph level setting.
 *  If there is no strong character, then set the paragraph level to 1 (right-to-left).
 */
#define UBIDI_DEFAULT_RTL 0xff

/**
 * Maximum explicit embedding level.
 * (The maximum resolved level can be up to <code>UBIDI_MAX_EXPLICIT_LEVEL+1</code>).
 *
 */
#define UBIDI_MAX_EXPLICIT_LEVEL 61

/** Bit flag for level input. 
 *  Overrides directional properties. 
 */
#define UBIDI_LEVEL_OVERRIDE 0x80

/**
 * <code>UBidiDirection</code> values indicate the text direction.
 */
enum UBidiDirection {
  /** All left-to-right text This is a 0 value. */
  UBIDI_LTR,
  /** All right-to-left text This is a 1 value. */
  UBIDI_RTL,
  /** Mixed-directional text. */
  UBIDI_MIXED
};

typedef enum UBidiDirection UBidiDirection;

/* miscellaneous definitions ------------------------------------------------ */

typedef PRUint8 DirProp;
typedef PRUint32 Flags;

enum UCharDirection   { 
	U_LEFT_TO_RIGHT               = 0, 
	U_RIGHT_TO_LEFT               = 1, 
	U_EUROPEAN_NUMBER             = 2,
	U_EUROPEAN_NUMBER_SEPARATOR   = 3,
	U_EUROPEAN_NUMBER_TERMINATOR  = 4,
	U_ARABIC_NUMBER               = 5,
	U_COMMON_NUMBER_SEPARATOR     = 6,
	U_BLOCK_SEPARATOR             = 7,
	U_SEGMENT_SEPARATOR           = 8,
	U_WHITE_SPACE_NEUTRAL         = 9, 
	U_OTHER_NEUTRAL               = 10, 
	U_LEFT_TO_RIGHT_EMBEDDING     = 11,
	U_LEFT_TO_RIGHT_OVERRIDE      = 12,
	U_RIGHT_TO_LEFT_ARABIC        = 13,
	U_RIGHT_TO_LEFT_EMBEDDING     = 14,
	U_RIGHT_TO_LEFT_OVERRIDE      = 15,
	U_POP_DIRECTIONAL_FORMAT      = 16,
	U_DIR_NON_SPACING_MARK        = 17,
	U_BOUNDARY_NEUTRAL            = 18,
	U_CHAR_DIRECTION_COUNT
};

/**
 * This specifies the language directional property of a character set.
 */
typedef enum UCharDirection UCharDirection;

/*  Comparing the description of the Bidi algorithm with this implementation
    is easier with the same names for the Bidi types in the code as there.
*/
enum { 
	L =   U_LEFT_TO_RIGHT,
	R =   U_RIGHT_TO_LEFT,
	EN =  U_EUROPEAN_NUMBER,
	ES =  U_EUROPEAN_NUMBER_SEPARATOR,
	ET =  U_EUROPEAN_NUMBER_TERMINATOR,
	AN =  U_ARABIC_NUMBER,
	CS =  U_COMMON_NUMBER_SEPARATOR,
	B =   U_BLOCK_SEPARATOR,
	S =   U_SEGMENT_SEPARATOR,
	WS =  U_WHITE_SPACE_NEUTRAL,
	O_N =  U_OTHER_NEUTRAL,
	LRE = U_LEFT_TO_RIGHT_EMBEDDING,
	LRO = U_LEFT_TO_RIGHT_OVERRIDE,
	AL =  U_RIGHT_TO_LEFT_ARABIC,
	RLE = U_RIGHT_TO_LEFT_EMBEDDING,
	RLO = U_RIGHT_TO_LEFT_OVERRIDE,
	PDF = U_POP_DIRECTIONAL_FORMAT,
	NSM = U_DIR_NON_SPACING_MARK,
	BN =  U_BOUNDARY_NEUTRAL,
	dirPropCount
};

/*
 * Sometimes, bit values are more appropriate
 * to deal with directionality properties.
 * Abbreviations in these macro names refer to names
 * used in the Bidi algorithm.
 */
#define DIRPROP_FLAG(dir) (1UL<<(dir))

/* special flag for multiple runs from explicit embedding codes */
#define DIRPROP_FLAG_MULTI_RUNS (1UL<<31)

/* are there any characters that are LTR or RTL? */
#define MASK_LTR (DIRPROP_FLAG(L)|DIRPROP_FLAG(EN)|DIRPROP_FLAG(AN)|DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RTL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))

/* explicit embedding codes */
#define MASK_LRX (DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RLX (DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_OVERRIDE (DIRPROP_FLAG(LRO)|DIRPROP_FLAG(RLO))

#define MASK_EXPLICIT (MASK_LRX|MASK_RLX|DIRPROP_FLAG(PDF))
#define MASK_BN_EXPLICIT (DIRPROP_FLAG(BN)|MASK_EXPLICIT)

/* paragraph and segment separators */
#define MASK_B_S (DIRPROP_FLAG(B)|DIRPROP_FLAG(S))

/* all types that are counted as White Space or Neutral in some steps */
#define MASK_WS (MASK_B_S|DIRPROP_FLAG(WS)|MASK_BN_EXPLICIT)
#define MASK_N (DIRPROP_FLAG(O_N)|MASK_WS)

/* all types that are included in a sequence of European Terminators for (W5) */
#define MASK_ET_NSM_BN (DIRPROP_FLAG(ET)|DIRPROP_FLAG(NSM)|MASK_BN_EXPLICIT)

/* types that are neutrals or could becomes neutrals in (Wn) */
#define MASK_POSSIBLE_N (DIRPROP_FLAG(CS)|DIRPROP_FLAG(ES)|DIRPROP_FLAG(ET)|MASK_N)

/*
 * These types may be changed to "e",
 * the embedding type (L or R) of the run,
 * in the Bidi algorithm (N2)
 */
#define MASK_EMBEDDING (DIRPROP_FLAG(NSM)|MASK_POSSIBLE_N)

/* to avoid some conditional statements, use tiny constant arrays */
static Flags flagLR[2]={ DIRPROP_FLAG(L), DIRPROP_FLAG(R) };
static Flags flagE[2]={ DIRPROP_FLAG(LRE), DIRPROP_FLAG(RLE) };
static Flags flagO[2]={ DIRPROP_FLAG(LRO), DIRPROP_FLAG(RLO) };

#define DIRPROP_FLAG_LR(level) flagLR[(level)&1]
#define DIRPROP_FLAG_E(level) flagE[(level)&1]
#define DIRPROP_FLAG_O(level) flagO[(level)&1]

/* the dirProp's L and R are defined to 0 and 1 values in UCharDirection */
#define GET_LR_FROM_LEVEL(level) ((DirProp)((level)&1))

#define IS_DEFAULT_LEVEL(level) (((level)&0xfe)==0xfe)

/* handle surrogate pairs --------------------------------------------------- */

#define IS_FIRST_SURROGATE(uchar) (((uchar)&0xfc00)==0xd800)
#define IS_SECOND_SURROGATE(uchar) (((uchar)&0xfc00)==0xdc00)

/* get the UTF-32 value directly from the surrogate pseudo-characters */
#define SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)
#define GET_UTF_32(first, second) (((first)<<10UL)+(second)-SURROGATE_OFFSET)


#define UTF_ERROR_VALUE 0xffff
/* definitions with forward iteration --------------------------------------- */

/*
 * all the macros that go forward assume that
 * the initial offset is 0<=i<length;
 * they update the offset
 */

/* fast versions, no error-checking */

#define UTF16_APPEND_CHAR_UNSAFE(s, i, c){ \
  if((PRUint32)(c)<=0xffff) { \
    (s)[(i)++]=(PRUnichar)(c); \
  } else { \
    (s)[(i)++]=(PRUnichar)((c)>>10)+0xd7c0; \
    (s)[(i)++]=(PRUnichar)(c)&0x3ff|0xdc00; \
  } \
}

/* safe versions with error-checking and optional regularity-checking */

#define UTF16_APPEND_CHAR_SAFE(s, i, length, c) { \
  if((PRUInt32)(c)<=0xffff) { \
    (s)[(i)++]=(PRUnichar)(c); \
  } else if((PRUInt32)(c)<=0x10ffff) { \
    if((i)+1<(length)) { \
      (s)[(i)++]=(PRUnichar)((c)>>10)+0xd7c0; \
      (s)[(i)++]=(PRUnichar)(c)&0x3ff|0xdc00; \
    } else /* not enough space */ { \
      (s)[(i)++]=UTF_ERROR_VALUE; \
    } \
  } else /* c>0x10ffff, write error value */ { \
      (s)[(i)++]=UTF_ERROR_VALUE; \
  } \
}

/* definitions with backward iteration -------------------------------------- */

/*
 * all the macros that go backward assume that
 * the valid buffer range starts at offset 0
 * and that the initial offset is 0<i<=length;
 * they update the offset
 */

/* fast versions, no error-checking */

/*
 * Get a single code point from an offset that points behind the last
 * of the code units that belong to that code point.
 * Assume 0<=i<length.
 */
#define UTF16_PREV_CHAR_UNSAFE(s, i, c) { \
  (c)=(s)[--(i)]; \
  if(IS_SECOND_SURROGATE(c)) { \
    (c)=GET_UTF_32((s)[--(i)], (c)); \
  } \
}

#define UTF16_BACK_1_UNSAFE(s, i) { \
  if(IS_SECOND_SURROGATE((s)[--(i)])) { \
     --(i); \
  } \
}

#define UTF16_BACK_N_UNSAFE(s, i, n) { \
  PRInt32 __N=(n); \
  while(__N>0) { \
    UTF16_BACK_1_UNSAFE(s, i); \
    --__N; \
  } \
}

/* safe versions with error-checking and optional regularity-checking */

#define UTF16_PREV_CHAR_SAFE(s, start, i, c, strict) { \
  (c)=(s)[--(i)]; \
  if(IS_SECOND_SURROGATE(c)) { \
    PRUnichar __c2; \
    if((i)>(start) && IS_FIRST_SURROGATE(__c2=(s)[(i)-1])) { \
      --(i); \
      (c)=GET_UTF_32(__c2, (c)); \
      /* strict: ((c)&0xfffe)==0xfffe is caught by UTF_IS_ERROR() */ \
    } else if(strict) {\
      /* unmatched second surrogate */ \
      (c)=UTF_ERROR_VALUE; \
    } \
  } else if(strict && IS_FIRST_SURROGATE(c)) { \
      /* unmatched first surrogate */ \
      (c)=UTF_ERROR_VALUE; \
  /* else strict: (c)==0xfffe is caught by UTF_IS_ERROR() */ \
  } \
}

#define UTF16_BACK_1_SAFE(s, start, i) { \
  if(IS_SECOND_SURROGATE((s)[--(i)]) && (i)>(start) && IS_FIRST_SURROGATE((s)[(i)-1])) { \
    --(i); \
  } \
}

#define UTF16_BACK_N_SAFE(s, start, i, n) { \
  PRInt32 __N=(n); \
  while(__N>0 && (i)>(start)) { \
    UTF16_BACK_1_SAFE(s, start, i); \
    --__N; \
  } \
}

#define UTF_PREV_CHAR_UNSAFE(s, i, c)                UTF16_PREV_CHAR_UNSAFE(s, i, c)
#define UTF_PREV_CHAR_SAFE(s, start, i, c, strict)   UTF16_PREV_CHAR_SAFE(s, start, i, c, strict)
#define UTF_BACK_1_UNSAFE(s, i)                      UTF16_BACK_1_UNSAFE(s, i)
#define UTF_BACK_1_SAFE(s, start, i)                 UTF16_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N_UNSAFE(s, i, n)                   UTF16_BACK_N_UNSAFE(s, i, n)
#define UTF_BACK_N_SAFE(s, start, i, n)              UTF16_BACK_N_SAFE(s, start, i, n)
#define UTF_APPEND_CHAR_UNSAFE(s, i, c)              UTF16_APPEND_CHAR_UNSAFE(s, i, c)
#define UTF_APPEND_CHAR_SAFE(s, i, length, c)        UTF16_APPEND_CHAR_SAFE(s, i, length, c)

#define UTF_PREV_CHAR(s, start, i, c)                UTF_PREV_CHAR_SAFE(s, start, i, c, PR_FALSE)
#define UTF_BACK_1(s, start, i)                      UTF_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N(s, start, i, n)                   UTF_BACK_N_SAFE(s, start, i, n)
#define UTF_APPEND_CHAR(s, i, length, c)             UTF_APPEND_CHAR_SAFE(s, i, length, c)

/* Run structure for reordering --------------------------------------------- */

typedef struct Run {
  PRInt32 logicalStart,  /* first character of the run; b31 indicates even/odd level */
  visualLimit;  /* last visual position of the run +1 */
} Run;

/* in a Run, logicalStart will get this bit set if the run level is odd */
#define INDEX_ODD_BIT (1UL<<31)

#define MAKE_INDEX_ODD_PAIR(index, level) (index|((PRUint32)level<<31))
#define ADD_ODD_BIT_FROM_LEVEL(x, level)  ((x)|=((PRUint32)level<<31))
#define REMOVE_ODD_BIT(x)          ((x)&=~INDEX_ODD_BIT)

#define GET_INDEX(x)   (x&~INDEX_ODD_BIT)
#define GET_ODD_BIT(x) ((PRUint32)x>>31)
#define IS_ODD_RUN(x)  ((x&INDEX_ODD_BIT)!=0)
#define IS_EVEN_RUN(x) ((x&INDEX_ODD_BIT)==0)

  /** option flags for writeReverse() */
  /**
   * option bit for writeReverse():
   * keep combining characters after their base characters in RTL runs
   *
   * @see writeReverse
   */
#define UBIDI_KEEP_BASE_COMBINING       1
 
  /**
   * option bit for writeReverse():
   * replace characters with the "mirrored" property in RTL runs
   * by their mirror-image mappings
   *
   * @see writeReverse
   */
#define UBIDI_DO_MIRRORING              2

  /**
   * option bit for writeReverse():
   * remove Bidi control characters
   *
   * @see writeReverse
   */
#define UBIDI_REMOVE_BIDI_CONTROLS      8

/**
 * This class holds information about a paragraph of text
 * with Bidi-algorithm-related details, or about one line of
 * such a paragraph.<p>
 * Reordering can be done on a line, or on a paragraph which is
 * then interpreted as one single line.<p>
 *
 * On construction, the class is initially empty. It is assigned
 * the Bidi properties of a paragraph by <code>setPara</code>
 * or the Bidi properties of a line of a paragraph by
 * <code>setLine</code>.<p>
 * A Bidi class can be reused for as long as it is not deallocated
 * by calling its destructor.<p>
 * <code>setPara</code> will allocate additional memory for
 * internal structures as necessary.
 */
class nsIBidi : public nsISupports
{
public: 
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_BIDI_IID)

/**
 * Perform the Unicode Bidi algorithm. It is defined in the
 * <a href="http://www.unicode.org/unicode/reports/tr9/">Unicode Technical Report 9</a>,
 * version 5,
 * also described in The Unicode Standard, Version 3.0 .<p>
 *
 * This function takes a single plain text paragraph with or without
 * externally specified embedding levels from <quote>styled</quote> text
 * and computes the left-right-directionality of each character.<p>
 *
 * If the entire paragraph consists of text of only one direction, then
 * the function may not perform all the steps described by the algorithm,
 * i.e., some levels may not be the same as if all steps were performed.
 * This is not relevant for unidirectional text.<br>
 * For example, in pure LTR text with numbers the numbers would get
 * a resolved level of 2 higher than the surrounding text according to
 * the algorithm. This implementation may set all resolved levels to
 * the same value in such a case.<p>
 *
 * The text must be externally split into separate paragraphs (rule P1).
 * Paragraph separators (B) should appear at most at the very end.
 *
 * @param text is a pointer to the single-paragraph text that the
 *      Bidi algorithm will be performed on
 *      (step (P1) of the algorithm is performed externally).
 *      <strong>The text must be (at least) <code>length</code> long.</strong>
 *
 * @param length is the length of the text; if <code>length==-1</code> then
 *      the text must be zero-terminated.
 *
 * @param paraLevel specifies the default level for the paragraph;
 *      it is typically 0 (LTR) or 1 (RTL).
 *      If the function shall determine the paragraph level from the text,
 *      then <code>paraLevel</code> can be set to
 *      either <code>UBIDI_DEFAULT_LTR</code>
 *      or <code>UBIDI_DEFAULT_RTL</code>;
 *      if there is no strongly typed character, then
 *      the desired default is used (0 for LTR or 1 for RTL).
 *      Any other value between 0 and <code>UBIDI_MAX_EXPLICIT_LEVEL</code> is also valid,
 *      with odd levels indicating RTL.
 *
 * @param embeddingLevels (in) may be used to preset the embedding and override levels,
 *      ignoring characters like LRE and PDF in the text.
 *      A level overrides the directional property of its corresponding
 *      (same index) character if the level has the
 *      <code>UBIDI_LEVEL_OVERRIDE</code> bit set.<p>
 *      Except for that bit, it must be
 *      <code>paraLevel<=embeddingLevels[]<=UBIDI_MAX_EXPLICIT_LEVEL</code>.<p>
 *      <strong>Caution: </strong>A copy of this pointer, not of the levels,
 *      will be stored in the <code>nsBidi</code> object;
 *      the <code>embeddingLevels</code> array must not be
 *      deallocated before the <code>nsBidi</code> object is destroyed or reused,
 *      and the <code>embeddingLevels</code>
 *      should not be modified to avoid unexpected results on subsequent Bidi operations.
 *      However, the <code>setPara</code> and
 *      <code>setLine</code> functions may modify some or all of the levels.<p>
 *      After the <code>nsBidi</code> object is reused or destroyed, the caller
 *      must take care of the deallocation of the <code>embeddingLevels</code> array.<p>
 *      <strong>The <code>embeddingLevels</code> array must be
 *      at least <code>length</code> long.</strong>
 */
  NS_IMETHOD setPara(const PRUnichar *text, PRInt32 length, UBidiLevel paraLevel, UBidiLevel *embeddingLevels) = 0;
    
  /**
   * <code>setLine</code> sets an <code>nsBidi</code> to
   * contain the reordering information, especially the resolved levels,
   * for all the characters in a line of text. This line of text is
   * specified by referring to an <code>nsBidi</code> object representing
   * this information for a paragraph of text, and by specifying
   * a range of indexes in this paragraph.<p>
   * In the new line object, the indexes will range from 0 to <code>limit-start</code>.<p>
   *
   * This is used after calling <code>setPara</code>
   * for a paragraph, and after line-breaking on that paragraph.
   * It is not necessary if the paragraph is treated as a single line.<p>
   *
   * After line-breaking, rules (L1) and (L2) for the treatment of
   * trailing WS and for reordering are performed on
   * an <code>nsBidi</code> object that represents a line.<p>
   *
   * <strong>Important:</strong> the line <code>nsBidi</code> object shares data with
   * <code>pParaBidi</code>.
   * You must destroy or reuse this object before <code>pParaBidi</code>.
   * In other words, you must destroy or reuse the <code>UBidi</code> object for a line
   * before the object for its parent paragraph.
   *
   * @param pParaBidi is the parent paragraph object.
   *
   * @param start is the line's first index into the paragraph text.
   *
   * @param limit is just behind the line's last index into the paragraph text
   *      (its last index +1).<br>
   *      It must be <code>0<=start<=limit<=</code>paragraph length.
   *
   * @see setPara
   */
  NS_IMETHOD setLine(nsIBidi* pParaBidi, PRInt32 start, PRInt32 limit) = 0;  

  /**
   * Get the directionality of the text.
   *
   * @param pDirection receives a <code>UBIDI_XXX</code> value that indicates if the entire text
   *       represented by this object is unidirectional,
   *       and which direction, or if it is mixed-directional.
   *
   * @see UBidiDirection
   */
  NS_IMETHOD getDirection(UBidiDirection* pDirection) = 0;

  /**
   * Get the length of the text.
   *
   * @param pLength receives the length of the text that the nsBidi object was created for.
   */
  NS_IMETHOD getLength(PRInt32* pLength) = 0;

  /**
   * Get the paragraph level of the text.
   *
   * @param pParaLevel receives a <code>UBIDI_XXX</code> value indicating the paragraph level
   *
   * @see UBidiLevel
   */
  NS_IMETHOD getParaLevel(UBidiLevel* pParaLevel) = 0;
  
  /**
   * Get the level for one character.
   *
   * @param charIndex the index of a character.
   *
   * @param pLevel receives the level for the character at charIndex.
   *
   * @see UBidiLevel
   */
  NS_IMETHOD getLevelAt(PRInt32 charIndex,  UBidiLevel* pLevel) = 0;

  /**
   * Get an array of levels for each character.<p>
   *
   * Note that this function may allocate memory under some
   * circumstances, unlike <code>getLevelAt</code>.
   *
   * @param levels receives a pointer to the levels array for the text,
   *       or <code>NULL</code> if an error occurs.
   *
   * @see UBidiLevel
   */
  NS_IMETHOD getLevels(UBidiLevel** levels) = 0;

  /**
   * Get the text class for one character.
   *
   * @param charIndex the index of a character.
   *
   * @param pClass receives the text class for the character at charIndex.
   */
  NS_IMETHOD getClassAt(PRInt32 charIndex,  DirProp* pLevel) = 0;

  /**
   * Get a logical run.
   * This function returns information about a run and is used
   * to retrieve runs in logical order.<p>
   * This is especially useful for line-breaking on a paragraph.
   *
   * @param logicalStart is the first character of the run.
   *
   * @param pLogicalLimit will receive the limit of the run.
   *      The l-value that you point to here may be the
   *      same expression (variable) as the one for
   *      <code>logicalStart</code>.
   *      This pointer can be <code>NULL</code> if this
   *      value is not necessary.
   *
   * @param pLevel will receive the level of the run.
   *      This pointer can be <code>NULL</code> if this
   *      value is not necessary.
   */
  NS_IMETHOD getLogicalRun(PRInt32 logicalStart, PRInt32* pLogicalLimit, UBidiLevel* pLevel) = 0;

  /**
   * Get the number of runs.
   * This function may invoke the actual reordering on the
   * <code>nsBidi</code> object, after <code>setPara</code>
   * may have resolved only the levels of the text. Therefore,
   * <code>countRuns</code> may have to allocate memory,
   * and may fail doing so.
   *
   * @param pRunCount will receive the number of runs.
   */
  NS_IMETHOD countRuns(PRInt32* pRunCount) = 0;

  /**
   * Get one run's logical start, length, and directionality,
   * which can be 0 for LTR or 1 for RTL.
   * In an RTL run, the character at the logical start is
   * visually on the right of the displayed run.
   * The length is the number of characters in the run.<p>
   * <code>countRuns</code> should be called
   * before the runs are retrieved.
   *
   * @param runIndex is the number of the run in visual order, in the
   *      range <code>[0..countRuns-1]</code>.
   *
   * @param pLogicalStart is the first logical character index in the text.
   *      The pointer may be <code>NULL</code> if this index is not needed.
   *
   * @param pLength is the number of characters (at least one) in the run.
   *      The pointer may be <code>NULL</code> if this is not needed.
   *
   * @param pDirection will receive the directionality of the run,
   *       <code>UBIDI_LTR==0</code> or <code>UBIDI_RTL==1</code>,
   *       never <code>UBIDI_MIXED</code>.
   *
   * @see countRuns<p>
   *
   * Example:
   * @code
   *  PRInt32 i, count, logicalStart, visualIndex=0, length;
   *  UBidiDirection dir;
   *  pBidi->countRuns(&count);
   *  for(i=0; i<count; ++i) {
   *    pBidi->getVisualRun(i, &logicalStart, &length, &dir);
   *    if(UBIDI_LTR==dir) {
   *      do { // LTR
   *        show_char(text[logicalStart++], visualIndex++);
   *      } while(--length>0);
   *    } else {
   *      logicalStart+=length;  // logicalLimit
   *      do { // RTL
   *        show_char(text[--logicalStart], visualIndex++);
   *      } while(--length>0);
   *    }
   *  }
   * @endcode
   *
   * Note that in right-to-left runs, code like this places
   * modifier letters before base characters and second surrogates
   * before first ones.
   */
  NS_IMETHOD getVisualRun(PRInt32 runIndex, PRInt32* pLogicalStart, PRInt32* pLength, UBidiDirection* pDirection) = 0;

  /**
   * Get the visual position from a logical text position.
   * If such a mapping is used many times on the same
   * <code>nsBidi</code> object, then calling
   * <code>getLogicalMap</code> is more efficient.<p>
   *
   * Note that in right-to-left runs, this mapping places
   * modifier letters before base characters and second surrogates
   * before first ones.
   *
   * @param logicalIndex is the index of a character in the text.
   *
   * @param pVisualIndex will receive the visual position of this character.
   *
   * @see getLogicalMap
   * @see getLogicalIndex
   */
  NS_IMETHOD getVisualIndex(PRInt32 logicalIndex, PRInt32* pVisualIndex) = 0;

  /**
   * Get the logical text position from a visual position.
   * If such a mapping is used many times on the same
   * <code>nsBidi</code> object, then calling
   * <code>getVisualMap</code> is more efficient.<p>
   *
   * This is the inverse function to <code>getVisualIndex</code>.
   *
   * @param visualIndex is the visual position of a character.
   *
   * @param pLogicalIndex will receive the index of this character in the text.
   *
   * @see getVisualMap
   * @see getVisualIndex
   */
  NS_IMETHOD getLogicalIndex(PRInt32 visualIndex, PRInt32* pLogicalIndex) = 0;

  /**
   * Get a logical-to-visual index map (array) for the characters in the nsBidi
   * (paragraph or line) object.
   *
   * @param indexMap is a pointer to an array of <code>getLength</code>
   *      indexes which will reflect the reordering of the characters.
   *      The array does not need to be initialized.<p>
   *      The index map will result in <code>indexMap[logicalIndex]==visualIndex</code>.<p>
   *
   * @see getVisualMap
   * @see getVisualIndex
   */
  NS_IMETHOD getLogicalMap(PRInt32 *indexMap) = 0;

  /**
   * Get a visual-to-logical index map (array) for the characters in the nsBidi
   * (paragraph or line) object.
   *
   * @param indexMap is a pointer to an array of <code>getLength</code>
   *      indexes which will reflect the reordering of the characters.
   *      The array does not need to be initialized.<p>
   *      The index map will result in <code>indexMap[visualIndex]==logicalIndex</code>.<p>
   *
   * @see getLogicalMap
   * @see getLogicalIndex
   */
  NS_IMETHOD getVisualMap(PRInt32 *indexMap) = 0;

  /**
   * This is a convenience function that does not use a nsBidi object.
   * It is intended to be used for when an application has determined the levels
   * of objects (character sequences) and just needs to have them reordered (L2).
   * This is equivalent to using <code>getLogicalMap</code> on a
   * <code>nsBidi</code> object.
   *
   * @param levels is an array with <code>length</code> levels that have been determined by
   *      the application.
   *
   * @param length is the number of levels in the array, or, semantically,
   *      the number of objects to be reordered.
   *      It must be <code>length>0</code>.
   *
   * @param indexMap is a pointer to an array of <code>length</code>
   *      indexes which will reflect the reordering of the characters.
   *      The array does not need to be initialized.<p>
   *      The index map will result in <code>indexMap[logicalIndex]==visualIndex</code>.
   */
  NS_IMETHOD reorderLogical(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap) = 0;

  /**
   * This is a convenience function that does not use a nsBidi object.
   * It is intended to be used for when an application has determined the levels
   * of objects (character sequences) and just needs to have them reordered (L2).
   * This is equivalent to using <code>getVisualMap</code> on a
   * <code>nsBidi</code> object.
   *
   * @param levels is an array with <code>length</code> levels that have been determined by
   *      the application.
   *
   * @param length is the number of levels in the array, or, semantically,
   *      the number of objects to be reordered.
   *      It must be <code>length>0</code>.
   *
   * @param indexMap is a pointer to an array of <code>length</code>
   *      indexes which will reflect the reordering of the characters.
   *      The array does not need to be initialized.<p>
   *      The index map will result in <code>indexMap[visualIndex]==logicalIndex</code>.
   */
  NS_IMETHOD reorderVisual(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap) = 0;

  /**
   * Invert an index map.
   * The one-to-one index mapping of the first map is inverted and written to
   * the second one.
   *
   * @param srcMap is an array with <code>length</code> indexes
   *      which define the original mapping.
   *
   * @param destMap is an array with <code>length</code> indexes
   *      which will be filled with the inverse mapping.
   *
   * @param length is the length of each array.
   */
  NS_IMETHOD invertMap(const PRInt32 *srcMap, PRInt32 *destMap, PRInt32 length) = 0;

// IBMBIDI - EGYPT - Start
	/*
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
	//virtual void Conv_FE_06 (nsAutoTextBuffer * aTextBuffer, PRUint32 * txtSizeChange);
	*/
// IBMBIDI - EGYPT - End
  
 /**
  * Reverse a Right-To-Left run of Unicode text.
  *
  * This function preserves the integrity of characters with multiple
  * code units and (optionally) modifier letters.
  * Characters can be replaced by mirror-image characters
  * in the destination buffer. Note that "real" mirroring has
  * to be done in a rendering engine by glyph selection
  * and that for many "mirrored" characters there are no
  * Unicode characters as mirror-image equivalents.
  * There are also options to insert or remove Bidi control
  * characters; see the description of the <code>destSize</code>
  * and <code>options</code> parameters and of the option bit flags.
  *
  * Since no Bidi controls are inserted here, this function will never
  * write more than <code>srcLength</code> characters to <code>dest</code>.
  *
  * @param src A pointer to the RTL run text.
  *
  * @param srcLength The length of the RTL run.
  *                 If the <code>UBIDI_REMOVE_BIDI_CONTROLS</code> option
  *                 is set, then the destination length may be less than
  *                 <code>srcLength</code>.
  *                 If this option is not set, then the destination length
  *                 will be exactly <code>srcLength</code>.
  *
  * @param dest A pointer to where the reordered text is to be copied.
  *             <code>src[srcLength]</code> and <code>dest[srcLength]</code>
  *             must not overlap.
  *
  * @param options A bit set of options for the reordering that control
  *                how the reordered text is written.
  *                See <code>writeReverse()</code>.
  *
  * @param destSize will receive the number of characters that were written to <code>dest</code>.
  */
  NS_IMETHOD writeReverse(const PRUnichar *src, PRInt32 srcLength, PRUnichar *dest, PRUint16 options, PRInt32 *destSize) = 0;
};

#endif  /* nsIBidi_h__ */

#endif /* IBMBIDI */
