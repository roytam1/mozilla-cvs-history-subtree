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
 * Corporation.    Portions created by Netscape are
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

#ifndef nsBidiImp_h__
#define nsBidiImp_h__

#include "nsCom.h"
#include "nsISupports.h"
#include "nsIBidi.h"

// IBMBIDI - Egypt - Start
#include "nsITextContent.h"
#include "nsTextFragment.h"
#include "nsTextTransformer.h"
#include "nsIUBidiUtils.h"
// IBMBIDI - Egypt - End

/* helper macros for each allocated array member */
#define getDirPropsMemory(length) \
                  getMemory((void **)&mDirPropsMemory, &mDirPropsSize, \
                  mMayAllocateText, (length))

#define getLevelsMemory(length) \
         getMemory((void **)&mLevelsMemory, &mLevelsSize, \
         mMayAllocateText, (length))

#define getRunsMemory(length) \
         getMemory((void **)&mRunsMemory, &mRunsSize, \
         mMayAllocateRuns, (length)*sizeof(Run))

/* additional macros used by constructor - always allow allocation */
#define getInitialDirPropsMemory(length) \
         getMemory((void **)&mDirPropsMemory, &mDirPropsSize, \
         PR_TRUE, (length))

#define getInitialLevelsMemory(length) \
         getMemory((void **)&mLevelsMemory, &mLevelsSize, \
         PR_TRUE, (length))

#define getInitialRunsMemory(length) \
         getMemory((void **)&mRunsMemory, &mRunsSize, \
         PR_TRUE, (length)*sizeof(Run))

  /**
  * Implementation of the nsIBidi interface
  */
class nsBidi : public nsIBidi {
  NS_DECL_ISUPPORTS
      
public:
  /** @brief Default constructor.
   * 
   * The nsBidi object is initially empty. It is assigned
   * the Bidi properties of a paragraph by <code>setPara()</code>
   * or the Bidi properties of a line of a paragraph by
   * <code>getLine()</code>.<p>
   * This object can be reused for as long as it is not destroyed.<p>
   * <code>setPara()</code> will allocate additional memory for
   * internal structures as necessary.
   *
   */
  nsBidi();

  /** @brief Preallocating constructor
   * Allocate an <code>nsBidi</code>
   * object with preallocated memory for internal structures.   This
   * constructor provides an <code>nsBidi</code> object like
   * the default constructor, but it also
   * preallocates memory for internal structures according to the sizings
   * supplied by the caller.<p> Subsequent functions will not allocate
   * any more memory, and are thus guaranteed not to fail because of lack
   * of memory.<p> The preallocation can be limited to some of the
   * internal memory by setting some values to 0 here. That means that
   * if, e.g., <code>maxRunCount</code> cannot be reasonably
   * predetermined and should not be set to <code>maxLength</code> (the
   * only failproof value) to avoid wasting memory, then
   * <code>maxRunCount</code> could be set to 0 here and the internal
   * structures that are associated with it will be allocated on demand,
   * just like with the default constructor.
   *
   * If sufficient memory could not be allocated, no exception is thrown.
   * Test whether mDirPropsSize == maxLength and/or mRunsSize == maxRunCount.
   *
   * @param maxLength is the maximum paragraph or line length that internal memory
   *      will be preallocated for. An attempt to associate this object with a
   *      longer text will fail, unless this value is 0, which leaves the allocation
   *      up to the implementation.
   *
   * @param maxRunCount is the maximum anticipated number of same-level runs
   *      that internal memory will be preallocated for. An attempt to access
   *      visual runs on an object that was not preallocated for as many runs
   *      as the text was actually resolved to will fail,
   *      unless this value is 0, which leaves the allocation up to the implementation.<p>
   *      The number of runs depends on the actual text and maybe anywhere between
   *      1 and <code>maxLength</code>. It is typically small.<p>
   */
  nsBidi(PRUint32 maxLength, PRUint32 maxRunCount);

  /** @brief Destructor. */
  virtual ~nsBidi();

  NS_IMETHOD setPara(const PRUnichar *text, PRInt32 length, UBidiLevel paraLevel, UBidiLevel *embeddingLevels);
  
  NS_IMETHOD setLine(nsIBidi* pParaBidi, PRInt32 start, PRInt32 limit);
  
  NS_IMETHOD getDirection(UBidiDirection* pDirection);
  
  NS_IMETHOD getLength(PRInt32* pLength);
  
  NS_IMETHOD getParaLevel(UBidiLevel* pParaLevel);

  NS_IMETHOD getLevelAt(PRInt32 charIndex,  UBidiLevel* pLevel);
  
  NS_IMETHOD getLevels(UBidiLevel** levels);
  
  NS_IMETHOD getClassAt(PRInt32 charIndex,  DirProp* pClass);

  NS_IMETHOD getLogicalRun(PRInt32 logicalStart, PRInt32* pLogicalLimit, UBidiLevel* pLevel);
  
  NS_IMETHOD countRuns(PRInt32* pRunCount);
  
  NS_IMETHOD getVisualRun(PRInt32 runIndex, PRInt32* pLogicalStart, PRInt32* pLength, UBidiDirection* pDirection);
  
  NS_IMETHOD getVisualIndex(PRInt32 logicalIndex, PRInt32* pVisualIndex);
  
  NS_IMETHOD getLogicalIndex(PRInt32 visualIndex, PRInt32* pLogicalIndex);
  
  NS_IMETHOD getLogicalMap(PRInt32 *indexMap);
  
  NS_IMETHOD getVisualMap(PRInt32 *indexMap);
  
  NS_IMETHOD reorderLogical(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap);
  
  NS_IMETHOD reorderVisual(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap);
  
  NS_IMETHOD invertMap(const PRInt32 *srcMap, PRInt32 *destMap, PRInt32 length);

  NS_IMETHOD writeReverse(const PRUnichar *src, PRInt32 srcLength, PRUnichar *dest, PRUint16 options, PRInt32 *destSize);

protected:
  /** length of the current text */
  PRInt32 mLength;

  /** memory sizes in bytes */
  PRSize mDirPropsSize, mLevelsSize, mRunsSize;

  /** allocated memory */
  DirProp* mDirPropsMemory;
  UBidiLevel* mLevelsMemory;
  Run* mRunsMemory;

  /** indicators for whether memory may be allocated after construction */
  PRBool mMayAllocateText, mMayAllocateRuns;

	const DirProp* mDirProps;
  UBidiLevel* mLevels;

  /** the paragraph level */
  UBidiLevel mParaLevel;

  /** flags is a bit set for which directional properties are in the text */
  Flags mFlags;

  /** the overall paragraph or line directionality - see UBidiDirection */
  UBidiDirection mDirection;

  /** characters after trailingWSStart are WS and are */
  /* implicitly at the paraLevel (rule (L1)) - levels may not reflect that */
  PRInt32 mTrailingWSStart;
  
  /** fields for line reordering */
  PRInt32 mRunCount;     /* ==-1: runs not set up yet */
  Run* mRuns;

  /** for non-mixed text, we only need a tiny array of runs (no malloc()) */
  Run mSimpleRuns[1];

private:

  void Init();
  
  PRBool getMemory(void **pMemory, PRSize* pSize, PRBool mayAllocate, PRSize sizeNeeded);

  void Free();
  
  void getDirProps(const PRUnichar *text);

  UBidiDirection resolveExplicitLevels();

  nsresult checkExplicitLevels(UBidiDirection *direction);

  UBidiDirection directionFromFlags(Flags flags);

  void resolveImplicitLevels(PRInt32 start, PRInt32 limit, DirProp sor, DirProp eor);

  void adjustWSLevels();

  void setTrailingWSStart();

  PRBool getRuns();

  void getSingleRun(UBidiLevel level);

  void reorderLine(UBidiLevel minLevel, UBidiLevel maxLevel);

  PRBool prepareReorder(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap, UBidiLevel *pMinLevel, UBidiLevel *pMaxLevel);
};
#endif

#endif // IBMBIDI
