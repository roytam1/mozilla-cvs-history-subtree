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
 * IBM Corporation.   Portions created by IBM are
 * Copyright (C) 2000 IBM Corporation. All
 * Rights Reserved.
 *
 * This module is based on the ICU (International Components for Unicode)
 *
 *   Copyright (C) 2000, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 * Contributor(s):  
 */
#ifdef IBMBIDI
// IBMBIDI - Egypt - Start
#include "nsIUBidiUtils.h"
#include "nsIBidi.h"
// IBMBIDI - Egypt - End

#include "prmem.h"
#include "prtypes.h"
#include "nsBidiImp.h"
#include "nsCRT.h"
#include "nsUUDll.h"
#include "nsIServiceManager.h"
#include "nsIUBidiUtils.h"



NS_DEFINE_IID(kBidiIID, NS_BIDI_IID);

NS_IMPL_ISUPPORTS(nsBidi, kBidiIID);

static NS_DEFINE_CID(kUnicharBidiUtilCID, NS_UNICHARBIDIUTIL_CID);

/*
 * General implementation notes:
 *
 * Throughout the implementation, there are comments like (W2) that refer to
 * rules of the Bidi algorithm in its version 5, in this example to the second
 * rule of the resolution of weak types.
 *
 * For handling surrogate pairs, where two UChar's form one "abstract" (or UTF-32)
 * character according to UTF-16, the second UChar gets the directional property of
 * the entire character assigned, while the first one gets a BN, a boundary
 * neutral, type, which is ignored by most of the algorithm according to
 * rule (X9) and the implementation suggestions of the Bidi algorithm.
 *
 * Later, adjustWSLevels() will set the level for each BN to that of the
 * following character (UChar), which results in surrogate pairs getting the
 * same level on each of their surrogates.
 *
 * In a UTF-8 implementation, the same thing could be done: the last byte of
 * a multi-byte sequence would get the "real" property, while all previous
 * bytes of that sequence would get BN.
 *
 * It is not possible to assign all those parts of a character the same real
 * property because this would fail in the resolution of weak types with rules
 * that look at immediately surrounding types.
 *
 * As a related topic, this implementation does not remove Boundary Neutral
 * types from the input, but ignores them whereever this is relevant.
 * For example, the loop for the resolution of the weak types reads
 * types until it finds a non-BN.
 * Also, explicit embedding codes are neither changed into BN nor removed.
 * They are only treated the same way real BNs are.
 * As stated before, adjustWSLevels() takes care of them at the end.
 * For the purpose of conformance, the levels of all these codes
 * do not matter.
 *
 * Note that this implementation never modifies the dirProps
 * after the initial setup.
 *
 *
 * In this implementation, the resolution of weak types (Wn),
 * neutrals (Nn), and the assignment of the resolved level (In)
 * are all done in one single loop, in resolveImplicitLevels().
 * Changes of dirProp values are done on the fly, without writing
 * them back to the dirProps array.
 *
 *
 * This implementation contains code that allows to bypass steps of the
 * algorithm that are not needed on the specific paragraph
 * in order to speed up the most common cases considerably,
 * like text that is entirely LTR, or RTL text without numbers.
 *
 * Most of this is done by setting a bit for each directional property
 * in a flags variable and later checking for whether there are
 * any LTR characters or any RTL characters, or both, whether
 * there are any explicit embedding codes, etc.
 *
 * If the (Xn) steps are performed, then the flags are re-evaluated,
 * because they will then not contain the embedding codes any more
 * and will be adjusted for override codes, so that subsequently
 * more bypassing may be possible than what the initial flags suggested.
 *
 * If the text is not mixed-directional, then the
 * algorithm steps for the weak type resolution are not performed,
 * and all levels are set to the paragraph level.
 *
 * If there are no explicit embedding codes, then the (Xn) steps
 * are not performed.
 *
 * If embedding levels are supplied as a parameter, then all
 * explicit embedding codes are ignored, and the (Xn) steps
 * are not performed.
 *
 * White Space types could get the level of the run they belong to,
 * and are checked with a test of (flags&MASK_EMBEDDING) to
 * consider if the paragraph direction should be considered in
 * the flags variable.
 *
 * If there are no White Space types in the paragraph, then
 * (L1) is not necessary in adjustWSLevels().
 */
nsBidi::nsBidi()
{
  Init();

  mMayAllocateText=PR_TRUE;
  mMayAllocateRuns=PR_TRUE;
}

nsBidi::nsBidi(PRUint32 maxLength, PRUint32 maxRunCount)
{
  Init();
  nsresult rv = NS_OK;

  /* allocate memory for arrays as requested */
  if(maxLength>0) {
    if( !getInitialDirPropsMemory(maxLength) ||
        !getInitialLevelsMemory(maxLength)
      ) {
      mMayAllocateText=PR_FALSE;
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    mMayAllocateText=PR_TRUE;
  }

  if(maxRunCount>0) {
    if(maxRunCount==1) {
      /* use simpleRuns[] */
      mRunsSize=sizeof(Run);
    } else if(!getInitialRunsMemory(maxRunCount)) {
      mMayAllocateRuns=PR_FALSE;
      rv = NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    mMayAllocateRuns=PR_TRUE;
  }

  if(NS_FAILED(rv)) {
    Free();
  }
}

nsBidi::~nsBidi()
{
  Free();
}

void nsBidi::Init()
{
  NS_INIT_REFCNT();
  PR_AtomicIncrement(&g_InstanceCount);
  /* reset the object, all pointers NULL, all flags PR_FALSE, all sizes 0 */
  mLength = 0;
  mParaLevel = 0;
  mFlags = 0;
  mDirection = UBIDI_LTR;
  mTrailingWSStart = 0;

  mDirPropsSize = 0;
  mLevelsSize = 0;
  mRunsSize = 0;
  mRunCount = -1;

  mDirProps=NULL;
  mLevels=NULL;
  mRuns=NULL;

  mDirPropsMemory=NULL;
  mLevelsMemory=NULL;
  mRunsMemory=NULL;

  mMayAllocateText=PR_FALSE;
  mMayAllocateRuns=PR_FALSE;
  
}

/*
 * We are allowed to allocate memory if memory==NULL or
 * mayAllocate==PR_TRUE for each array that we need.
 * We also try to grow and shrink memory as needed if we
 * allocate it.
 *
 * Assume sizeNeeded>0.
 * If *pMemory!=NULL, then assume *pSize>0.
 *
 * ### this realloc() may unnecessarily copy the old data,
 * which we know we don't need any more;
 * is this the best way to do this??
 */
PRBool nsBidi::getMemory(void **pMemory, PRSize *pSize, PRBool mayAllocate, PRSize sizeNeeded)
{
  /* check for existing memory */
  if(*pMemory==NULL) {
    /* we need to allocate memory */
    if(!mayAllocate) {
      return PR_FALSE;
    } else {
      *pMemory=PR_MALLOC(sizeNeeded);
      if (*pMemory!=NULL) {
        *pSize=sizeNeeded;
        return PR_TRUE;
      } else {
        *pSize=0;
        return PR_FALSE;
      }
    }
  } else {
    /* there is some memory, is it enough or too much? */
    if(sizeNeeded>*pSize && !mayAllocate) {
      /* not enough memory, and we must not allocate */
      return PR_FALSE;
    } else if(sizeNeeded!=*pSize && mayAllocate) {
      /* we may try to grow or shrink */
      void *memory=PR_REALLOC(*pMemory, sizeNeeded);

      if(memory!=NULL) {
        *pMemory=memory;
        *pSize=sizeNeeded;
        return PR_TRUE;
      } else {
        /* we failed to grow */
        return PR_FALSE;
      }
    } else {
      /* we have at least enough memory and must not allocate */
      return PR_TRUE;
    }
  }
}

void nsBidi::Free()
{
  PR_FREEIF(mDirPropsMemory);
  PR_FREEIF(mLevelsMemory);
  PR_FREEIF(mRunsMemory);
  PR_AtomicDecrement(&g_InstanceCount);
}

/* setPara ------------------------------------------------------------ */

nsresult nsBidi::setPara(const PRUnichar *text, PRInt32 length,
             UBidiLevel paraLevel, UBidiLevel *embeddingLevels)
{
  UBidiDirection direction;

  /* check the argument values */
  if(text==NULL ||
     (UBIDI_MAX_EXPLICIT_LEVEL<paraLevel) && !IS_DEFAULT_LEVEL(paraLevel) ||
     length<-1
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  if(length==-1) {
    length=nsCRT::strlen(text);
  }

  /* initialize member data */
  mLength=length;
  mParaLevel=paraLevel;
  mDirection=UBIDI_LTR;
  mTrailingWSStart=length;  /* the levels[] will reflect the WS run */

  mDirProps=NULL;
  mLevels=NULL;
  mRuns=NULL;

  if(length==0) {
    /*
     * For an empty paragraph, create an nsBidi object with the paraLevel and
     * the flags and the direction set but without allocating zero-length arrays.
     * There is nothing more to do.
     */
    if(IS_DEFAULT_LEVEL(paraLevel)) {
      mParaLevel&=1;
    }
    if(paraLevel&1) {
      mFlags=DIRPROP_FLAG(R);
      mDirection=UBIDI_RTL;
    } else {
      mFlags=DIRPROP_FLAG(L);
      mDirection=UBIDI_LTR;
    }

    mRunCount=0;
    return NS_OK;
  }

  mRunCount=-1;

  /*
   * Get the directional properties,
   * the flags bit-set, and
   * determine the partagraph level if necessary.
   */
  if(getDirPropsMemory(length)) {
    mDirProps=mDirPropsMemory;
    getDirProps(text);
  } else {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  /* are explicit levels specified? */
  if(embeddingLevels==NULL) {
    /* no: determine explicit levels according to the (Xn) rules */\
	if(getLevelsMemory(length)) {
      mLevels=mLevelsMemory;
      direction=resolveExplicitLevels();
    } else {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    /* set BN for all explicit codes, check that all levels are paraLevel..UBIDI_MAX_EXPLICIT_LEVEL */
    mLevels=embeddingLevels;
    nsresult rv = checkExplicitLevels(&direction);
    if(NS_FAILED(rv)) {
      return rv;
    }
  }

  /*
   * The steps after (X9) in the Bidi algorithm are performed only if
   * the paragraph text has mixed directionality!
   */
  switch(direction) {
    case UBIDI_LTR:
      /* make sure paraLevel is even */
      mParaLevel=(mParaLevel+1)&~1;

      /* all levels are implicitly at paraLevel (important for getLevels()) */
      mTrailingWSStart=0;
      break;
    case UBIDI_RTL:
      /* make sure paraLevel is odd */
      mParaLevel|=1;

      /* all levels are implicitly at paraLevel (important for getLevels()) */
      mTrailingWSStart=0;
      break;
    default:
      /*
       * If there are no external levels specified and there
       * are no significant explicit level codes in the text,
       * then we can treat the entire paragraph as one run.
       * Otherwise, we need to perform the following rules on runs of
       * the text with the same embedding levels. (X10)
       * "Significant" explicit level codes are ones that actually
       * affect non-BN characters.
       * Examples for "insignificant" ones are empty embeddings
       * LRE-PDF, LRE-RLE-PDF-PDF, etc.
       */
      if(embeddingLevels==NULL && !(mFlags&DIRPROP_FLAG_MULTI_RUNS)) {
        resolveImplicitLevels(0, length,
                    GET_LR_FROM_LEVEL(mParaLevel),
                    GET_LR_FROM_LEVEL(mParaLevel));
      } else {
        /* sor, eor: start and end types of same-level-run */
        UBidiLevel *levels=mLevels;
        PRInt32 start, limit=0;
        UBidiLevel level, nextLevel;
        DirProp sor, eor;

        /* determine the first sor and set eor to it because of the loop body (sor=eor there) */
        level=mParaLevel;
        nextLevel=levels[0];
        if(level<nextLevel) {
          eor=GET_LR_FROM_LEVEL(nextLevel);
        } else {
          eor=GET_LR_FROM_LEVEL(level);
        }

        do {
          /* determine start and limit of the run (end points just behind the run) */

          /* the values for this run's start are the same as for the previous run's end */
          sor=eor;
          start=limit;
          level=nextLevel;

          /* search for the limit of this run */
          while(++limit<length && levels[limit]==level) {}

          /* get the correct level of the next run */
          if(limit<length) {
            nextLevel=levels[limit];
          } else {
            nextLevel=mParaLevel;
          }

          /* determine eor from max(level, nextLevel); sor is last run's eor */
          if((level&~UBIDI_LEVEL_OVERRIDE)<(nextLevel&~UBIDI_LEVEL_OVERRIDE)) {
            eor=GET_LR_FROM_LEVEL(nextLevel);
          } else {
            eor=GET_LR_FROM_LEVEL(level);
          }

          /* if the run consists of overridden directional types, then there
          are no implicit types to be resolved */
          if(!(level&UBIDI_LEVEL_OVERRIDE)) {
            resolveImplicitLevels(start, limit, sor, eor);
          }
        } while(limit<length);
      }

      /* reset the embedding levels for some non-graphic characters (L1), (X9) */
      adjustWSLevels();
      break;
  }

  mDirection=direction;
  return NS_OK;
}

/* perform (P2)..(P3) ------------------------------------------------------- */

/*
 * Get the directional properties for the text,
 * calculate the flags bit-set, and
 * determine the partagraph level if necessary.
 */
void nsBidi::getDirProps(const PRUnichar *text)
{
  DirProp *dirProps=mDirPropsMemory;    /* mDirProps is const */

  PRInt32 i=0, length=mLength;
  Flags flags=0;      /* collect all directionalities in the text */
  PRUnichar uchar;
  DirProp dirProp;
  UCharDirection dir;

//  static NS_DEFINE_CID(kUnicharBidiUtilCID, NS_UNICHARBIDIUTIL_CID);
  static nsresult bcRv;
  NS_WITH_SERVICE(nsIUBidiUtils, BidiUtils, kUnicharBidiUtilCID, &bcRv);

  if (NS_FAILED(bcRv) || !BidiUtils) {
    /* default to left-to-right*/
    flags |= DIRPROP_FLAG(L);
    if(IS_DEFAULT_LEVEL(mParaLevel)) {
      mParaLevel&=1;
    }
    while (i<length) {
      uchar=text[i];
      if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(text[i+1])) {
        /* not a surrogate pair */
        dirProps[i]=L;
      } else {
        /* a surrogate pair */
        dirProps[i++]=BN;   /* second surrogate in the pair gets the BN type */
        dirProps[i]=L;
        flags|=DIRPROP_FLAG(BN);
      }
      ++i;
    }
  } else {

    if(IS_DEFAULT_LEVEL(mParaLevel)) {
      /* determine the paragraph level (P2..P3) */
      for(;;) {
        uchar=text[i];
        if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(text[i+1])) {
          /* not a surrogate pair */
          BidiUtils->GetICU(uchar, &dir);
          flags|=DIRPROP_FLAG(dirProps[i]=dirProp=dir);
        } else {
          /* a surrogate pair */
          dirProps[i++]=BN;   /* first surrogate in the pair gets the BN type */
          BidiUtils->GetICU(GET_UTF_32(uchar, text[i]), &dir);
          flags|=DIRPROP_FLAG(dirProps[i]=dirProp=dir)|DIRPROP_FLAG(BN);
        }
        ++i;
        if(dirProp==L) {
          mParaLevel=0;
          break;
        } else if(dirProp==R || dirProp==AL) {
          mParaLevel=1;
          break;
        } else if(i==length) {
          /*
           * see comment in nsUbidi.h:
           * the DEFAULT_XXX values are designed so that
           * their bit 0 alone yields the intended default
           */
          mParaLevel&=1;
          break;
        }
      }
    }

    /* get the rest of the directional properties and the flags bits */
    while(i<length) {
      uchar=text[i];
      if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(text[i+1])) {
        /* not a surrogate pair */
        BidiUtils->GetICU(uchar, &dir);
        flags|=DIRPROP_FLAG(dirProps[i]=dir);
      } else {
        /* a surrogate pair */
        dirProps[i++]=BN;   /* second surrogate in the pair gets the BN type */
        BidiUtils->GetICU(GET_UTF_32(uchar, text[i]), &dir);
        flags|=DIRPROP_FLAG(dirProps[i]=dir)|DIRPROP_FLAG(BN);
      }
      ++i;
    }
    if(flags&MASK_EMBEDDING) {
      flags|=DIRPROP_FLAG_LR(mParaLevel);
    }
  }
  mFlags=flags;
}

/* perform (X1)..(X9) ------------------------------------------------------- */

/*
 * Resolve the explicit levels as specified by explicit embedding codes.
 * Recalculate the flags to have them reflect the real properties
 * after taking the explicit embeddings into account.
 *
 * The Bidi algorithm is designed to result in the same behavior whether embedding
 * levels are externally specified (from "styled text", supposedly the preferred
 * method) or set by explicit embedding codes (LRx, RLx, PDF) in the plain text.
 * That is why (X9) instructs to remove all explicit codes (and BN).
 * However, in a real implementation, this removal of these codes and their index
 * positions in the plain text is undesirable since it would result in
 * reallocated, reindexed text.
 * Instead, this implementation leaves the codes in there and just ignores them
 * in the subsequent processing.
 * In order to get the same reordering behavior, positions with a BN or an
 * explicit embedding code just get the same level assigned as the last "real"
 * character.
 *
 * Some implementations, not this one, then overwrite some of these
 * directionality properties at "real" same-level-run boundaries by
 * L or R codes so that the resolution of weak types can be performed on the
 * entire paragraph at once instead of having to parse it once more and
 * perform that resolution on same-level-runs.
 * This limits the scope of the implicit rules in effectively
 * the same way as the run limits.
 *
 * Instead, this implementation does not modify these codes.
 * On one hand, the paragraph has to be scanned for same-level-runs, but
 * on the other hand, this saves another loop to reset these codes,
 * or saves making and modifying a copy of dirProps[].
 *
 *
 * Note that (Pn) and (Xn) changed significantly from version 4 of the Bidi algorithm.
 *
 *
 * Handling the stack of explicit levels (Xn):
 *
 * With the Bidi stack of explicit levels,
 * as pushed with each LRE, RLE, LRO, and RLO and popped with each PDF,
 * the explicit level must never exceed UBIDI_MAX_EXPLICIT_LEVEL==61.
 *
 * In order to have a correct push-pop semantics even in the case of overflows,
 * there are two overflow counters:
 * - countOver60 is incremented with each LRx at level 60
 * - from level 60, one RLx increases the level to 61
 * - countOver61 is incremented with each LRx and RLx at level 61
 *
 * Popping levels with PDF must work in the opposite order so that level 61
 * is correct at the correct point. Underflows (too many PDFs) must be checked.
 *
 * This implementation assumes that UBIDI_MAX_EXPLICIT_LEVEL is odd.
 */

UBidiDirection nsBidi::resolveExplicitLevels()
{
  const DirProp *dirProps=mDirProps;
  UBidiLevel *levels=mLevels;

  PRInt32 i=0, length=mLength;
  Flags flags=mFlags;       /* collect all directionalities in the text */
  DirProp dirProp;
  UBidiLevel level=mParaLevel;

  UBidiDirection direction;

  /* determine if the text is mixed-directional or single-directional */
  direction=directionFromFlags(flags);

  /* we may not need to resolve any explicit levels */
  if(direction!=UBIDI_MIXED) {
    /* not mixed directionality: levels don't matter - trailingWSStart will be 0 */
  } else if(!(flags&MASK_EXPLICIT)) {
    /* mixed, but all characters are at the same embedding level */
    /* set all levels to the paragraph level */
    for(i=0; i<length; ++i) {
      levels[i]=level;
    }
  } else {
    /* continue to perform (Xn) */

    /* (X1) level is set for all codes, embeddingLevel keeps track of the push/pop operations */
    /* both variables may carry the UBIDI_LEVEL_OVERRIDE flag to indicate the override status */
    UBidiLevel embeddingLevel=level, newLevel, stackTop=0;

    UBidiLevel stack[UBIDI_MAX_EXPLICIT_LEVEL];        /* we never push anything >=UBIDI_MAX_EXPLICIT_LEVEL */
    PRUint32 countOver60=0, countOver61=0;  /* count overflows of explicit levels */

    /* recalculate the flags */
    flags=0;

    /* since we assume that this is a single paragraph, we ignore (X8) */
    for(i=0; i<length; ++i) {
      dirProp=dirProps[i];
      switch(dirProp) {
        case LRE:
        case LRO:
          /* (X3, X5) */
          newLevel=(embeddingLevel+2)&~(UBIDI_LEVEL_OVERRIDE|1);    /* least greater even level */
          if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
            stack[stackTop]=embeddingLevel;
            ++stackTop;
            embeddingLevel=newLevel;
            if(dirProp==LRO) {
              embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
            } else {
              embeddingLevel&=~UBIDI_LEVEL_OVERRIDE;
            }
          } else if((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)==UBIDI_MAX_EXPLICIT_LEVEL) {
            ++countOver61;
          } else /* (embeddingLevel&~UBIDI_LEVEL_OVERRIDE)==UBIDI_MAX_EXPLICIT_LEVEL-1 */ {
            ++countOver60;
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case RLE:
        case RLO:
          /* (X2, X4) */
          newLevel=((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)+1)|1;    /* least greater odd level */
          if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
            stack[stackTop]=embeddingLevel;
            ++stackTop;
            embeddingLevel=newLevel;
            if(dirProp==RLO) {
              embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
            } else {
              embeddingLevel&=~UBIDI_LEVEL_OVERRIDE;
            }
          } else {
            ++countOver61;
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case PDF:
          /* (X7) */
          /* handle all the overflow cases first */
          if(countOver61>0) {
            --countOver61;
          } else if(countOver60>0 && (embeddingLevel&~UBIDI_LEVEL_OVERRIDE)!=UBIDI_MAX_EXPLICIT_LEVEL) {
            /* handle LRx overflows from level 60 */
            --countOver60;
          } else if(stackTop>0) {
            /* this is the pop operation; it also pops level 61 while countOver60>0 */
            --stackTop;
            embeddingLevel=stack[stackTop];
            /* } else { (underflow) */
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case B:
          /*
           * We do not really expect to see a paragraph separator (B),
           * but we should do something reasonable with it,
           * especially at the end of the text.
           */
          stackTop=0;
          countOver60=countOver61=0;
          embeddingLevel=level=mParaLevel;
          flags|=DIRPROP_FLAG(B);
          break;
        case BN:
          /* BN, LRE, RLE, and PDF are supposed to be removed (X9) */
          /* they will get their levels set correctly in adjustWSLevels() */
          flags|=DIRPROP_FLAG(BN);
          break;
        default:
          /* all other types get the "real" level */
          if(level!=embeddingLevel) {
            level=embeddingLevel;
            if(level&UBIDI_LEVEL_OVERRIDE) {
              flags|=DIRPROP_FLAG_O(level)|DIRPROP_FLAG_MULTI_RUNS;
            } else {
              flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG_MULTI_RUNS;
            }
          }
          if(!(level&UBIDI_LEVEL_OVERRIDE)) {
            flags|=DIRPROP_FLAG(dirProp);
          }
          break;
      }

      /*
       * We need to set reasonable levels even on BN codes and
       * explicit codes because we will later look at same-level runs (X10).
       */
      levels[i]=level;
    }
    if(flags&MASK_EMBEDDING) {
      flags|=DIRPROP_FLAG_LR(mParaLevel);
    }

    /* subsequently, ignore the explicit codes and BN (X9) */

    /* again, determine if the text is mixed-directional or single-directional */
    mFlags=flags;
    direction=directionFromFlags(flags);
  }
  return direction;
}

/*
 * Use a pre-specified embedding levels array:
 *
 * Adjust the directional properties for overrides (->LEVEL_OVERRIDE),
 * ignore all explicit codes (X9),
 * and check all the preset levels.
 *
 * Recalculate the flags to have them reflect the real properties
 * after taking the explicit embeddings into account.
 */
nsresult nsBidi::checkExplicitLevels(UBidiDirection *direction)
{
  const DirProp *dirProps=mDirProps;
  UBidiLevel *levels=mLevels;

  PRInt32 i, length=mLength;
  Flags flags=0;  /* collect all directionalities in the text */
  UBidiLevel level, paraLevel=mParaLevel;

  for(i=0; i<length; ++i) {
    level=levels[i];
    if(level&UBIDI_LEVEL_OVERRIDE) {
      /* keep the override flag in levels[i] but adjust the flags */
      level&=~UBIDI_LEVEL_OVERRIDE;     /* make the range check below simpler */
      flags|=DIRPROP_FLAG_O(level);
    } else {
      /* set the flags */
      flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG(dirProps[i]);
    }
    if(level<paraLevel || UBIDI_MAX_EXPLICIT_LEVEL<level) {
      /* level out of bounds */
      *direction = UBIDI_LTR;
      return NS_ERROR_INVALID_ARG;
    }
  }
  if(flags&MASK_EMBEDDING) {
    flags|=DIRPROP_FLAG_LR(mParaLevel);
  }

  /* determine if the text is mixed-directional or single-directional */
  mFlags=flags;
  *direction = directionFromFlags(flags);
  return NS_OK;
}

/* determine if the text is mixed-directional or single-directional */
UBidiDirection nsBidi::directionFromFlags(Flags flags)
{
  /* if the text contains AN and neutrals, then some neutrals may become RTL */
  if(!(flags&MASK_RTL || flags&DIRPROP_FLAG(AN) && flags&MASK_POSSIBLE_N)) {
    return UBIDI_LTR;
  } else if(!(flags&MASK_LTR)) {
    return UBIDI_RTL;
  } else {
    return UBIDI_MIXED;
  }
}

/* perform rules (Wn), (Nn), and (In) on a run of the text ------------------ */

/*
 * This implementation of the (Wn) rules applies all rules in one pass.
 * In order to do so, it needs a look-ahead of typically 1 character
 * (except for W5: sequences of ET) and keeps track of changes
 * in a rule Wp that affect a later Wq (p<q).
 *
 * historyOfEN is a variable-saver: it contains 4 boolean states;
 * a bit in it set to 1 means:
 *  bit 0: the current code is an EN after W2
 *  bit 1: the current code is an EN after W4
 *  bit 2: the previous code was an EN after W2
 *  bit 3: the previous code was an EN after W4
 * In other words, b0..1 have transitions of EN in the current iteration,
 * while b2..3 have the transitions of EN in the previous iteration.
 * A simple historyOfEN<<=2 suffices for the propagation.
 *
 * The (Nn) and (In) rules are also performed in that same single loop,
 * but effectively one iteration behind for white space.
 *
 * Since all implicit rules are performed in one step, it is not necessary
 * to actually store the intermediate directional properties in dirProps[].
 */

#define EN_SHIFT 2
#define EN_AFTER_W2 1
#define EN_AFTER_W4 2
#define EN_ALL 3
#define PREV_EN_AFTER_W2 4
#define PREV_EN_AFTER_W4 8

void nsBidi::resolveImplicitLevels(PRInt32 start, PRInt32 limit,
                   DirProp sor, DirProp eor)
{
  const DirProp *dirProps=mDirProps;
  UBidiLevel *levels=mLevels;

  PRInt32 i, next, neutralStart=-1;
  DirProp prevDirProp, dirProp, nextDirProp, lastStrong, beforeNeutral;
  PRUint8 historyOfEN;

  /* initialize: current at sor, next at start (it is start<limit) */
  next=start;
  dirProp=lastStrong=sor;
  nextDirProp=dirProps[next];
  historyOfEN=0;

  /*
   * In all steps of this implementation, BN and explicit embedding codes
   * must be treated as if they didn't exist (X9).
   * They will get levels set before a non-neutral character, and remain
   * undefined before a neutral one, but adjustWSLevels() will take care
   * of all of them.
   */
  while(DIRPROP_FLAG(nextDirProp)&MASK_BN_EXPLICIT) {
    if(++next<limit) {
      nextDirProp=dirProps[next];
    } else {
      nextDirProp=eor;
      break;
    }
  }

  /* loop for entire run */
  while(next<limit) {
    /* advance */
    prevDirProp=dirProp;
    dirProp=nextDirProp;
    i=next;
    do {
      if(++next<limit) {
        nextDirProp=dirProps[next];
      } else {
        nextDirProp=eor;
        break;
      }
    } while(DIRPROP_FLAG(nextDirProp)&MASK_BN_EXPLICIT);
    historyOfEN<<=EN_SHIFT;

    /* (W1..W7) */
    switch(dirProp) {
      case L:
        lastStrong=L;
        break;
      case R:
        lastStrong=R;
        break;
      case AL:
        /* (W3) */
        lastStrong=AL;
        dirProp=R;
        break;
      case EN:
        /* we have to set historyOfEN correctly */
        if(lastStrong==AL) {
          /* (W2) */
          dirProp=AN;
        } else {
          if(lastStrong==L) {
            /* (W7) */
            dirProp=L;
          }
          /* this EN stays after (W2) and (W4) - at least before (W7) */
          historyOfEN|=EN_ALL;
        }
        break;
      case ES:
        if( historyOfEN&PREV_EN_AFTER_W2 &&     /* previous was EN before (W4) */
            nextDirProp==EN && lastStrong!=AL   /* next is EN and (W2) won't make it AN */
          ) {
          /* (W4) */
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            /* (W7) */
            dirProp=L;
          }
          historyOfEN|=EN_AFTER_W4;
        } else {
          /* (W6) */
          dirProp=O_N;
        }
        break;
      case CS:
        if( historyOfEN&PREV_EN_AFTER_W2 &&     /* previous was EN before (W4) */
            nextDirProp==EN && lastStrong!=AL   /* next is EN and (W2) won't make it AN */
          ) {
          /* (W4) */
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            /* (W7) */
            dirProp=L;
          }
          historyOfEN|=EN_AFTER_W4;
        } else if(prevDirProp==AN &&                    /* previous was AN */
              (nextDirProp==AN ||                   /* next is AN */
               nextDirProp==EN && lastStrong==AL)   /* or (W2) will make it one */
             ) {
          /* (W4) */
          dirProp=AN;
        } else {
          /* (W6) */
          dirProp=O_N;
        }
        break;
      case ET:
        /* get sequence of ET; advance only next, not current, previous or historyOfEN */
        while(next<limit && DIRPROP_FLAG(nextDirProp)&MASK_ET_NSM_BN /* (W1), (X9) */) {
          if(++next<limit) {
            nextDirProp=dirProps[next];
          } else {
            nextDirProp=eor;
            break;
          }
        }

        if( historyOfEN&PREV_EN_AFTER_W4 ||     /* previous was EN before (W5) */
            nextDirProp==EN && lastStrong!=AL   /* next is EN and (W2) won't make it AN */
          ) {
          /* (W5) */
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            /* (W7) */
            dirProp=L;
          }
        } else {
          /* (W6) */
          dirProp=O_N;
        }

        /* apply the result of (W1), (W5)..(W7) to the entire sequence of ET */
        break;
      case NSM:
        /* (W1) */
        dirProp=prevDirProp;
        /* set historyOfEN back to prevDirProp's historyOfEN */
        historyOfEN>>=EN_SHIFT;
        /*
         * Technically, this should be done before the switch() in the form
         *      if(nextDirProp==NSM) {
         *          dirProps[next]=nextDirProp=dirProp;
         *      }
         *
         * - effectively one iteration ahead.
         * However, whether the next dirProp is NSM or is equal to the current dirProp
         * does not change the outcome of any condition in (W2)..(W7).
         */
        break;
      default:
        break;
    }

    /* here, it is always [prev,this,next]dirProp!=BN; it may be next>i+1 */

    /* perform (Nn) - here, only L, R, EN, AN, and neutrals are left */
    /* this is one iteration late for the neutrals */
    if(DIRPROP_FLAG(dirProp)&MASK_N) {
      if(neutralStart<0) {
        /* start of a sequence of neutrals */
        neutralStart=i;
        beforeNeutral=prevDirProp;
      }
    } else /* not a neutral, can be only one of { L, R, EN, AN } */ {
      /*
       * Note that all levels[] values are still the same at this
       * point because this function is called for an entire
       * same-level run.
       * Therefore, we need to read only one actual level.
       */
      UBidiLevel level=levels[i];

      if(neutralStart>=0) {
        UBidiLevel final;
        /* end of a sequence of neutrals (dirProp is "afterNeutral") */
        if(beforeNeutral==L) {
          if(dirProp==L) {
            final=0;                /* make all neutrals L (N1) */
          } else {
            final=level;            /* make all neutrals "e" (N2) */
          }
        } else /* beforeNeutral is one of { R, EN, AN } */ {
          if(dirProp==L) {
            final=level;            /* make all neutrals "e" (N2) */
          } else {
            final=1;                /* make all neutrals R (N1) */
          }
        }
        /* perform (In) on the sequence of neutrals */
        if((level^final)&1) {
          /* do something only if we need to _change_ the level */
          do {
            ++levels[neutralStart];
          } while(++neutralStart<i);
        }
        neutralStart=-1;
      }

      /* perform (In) on the non-neutral character */
      /*
       * in the cases of (W5), processing a sequence of ET,
       * and of (X9), skipping BN,
       * there may be multiple characters from i to <next
       * that all get (virtually) the same dirProp and (really) the same level
       */
      if(dirProp==L) {
        if(level&1) {
          ++level;
        } else {
          i=next;     /* we keep the levels */
        }
      } else if(dirProp==R) {
        if(!(level&1)) {
          ++level;
        } else {
          i=next;     /* we keep the levels */
        }
      } else /* EN or AN */ {
        level=(level+2)&~1;     /* least greater even level */
      }

      /* apply the new level to the sequence, if necessary */
      while(i<next) {
        levels[i++]=level;
      }
    }
  }

  /* perform (Nn) - here,
  the character after the the neutrals is eor, which is either L or R */
  /* this is one iteration late for the neutrals */
  if(neutralStart>=0) {
    /*
     * Note that all levels[] values are still the same at this
     * point because this function is called for an entire
     * same-level run.
     * Therefore, we need to read only one actual level.
     */
    UBidiLevel level=levels[neutralStart], final;

    /* end of a sequence of neutrals (eor is "afterNeutral") */
    if(beforeNeutral==L) {
      if(eor==L) {
        final=0;                /* make all neutrals L (N1) */
      } else {
        final=level;            /* make all neutrals "e" (N2) */
      }
    } else /* beforeNeutral is one of { R, EN, AN } */ {
      if(eor==L) {
        final=level;            /* make all neutrals "e" (N2) */
      } else {
        final=1;                /* make all neutrals R (N1) */
      }
    }
    /* perform (In) on the sequence of neutrals */
    if((level^final)&1) {
      /* do something only if we need to _change_ the level */
      do {
        ++levels[neutralStart];
      } while(++neutralStart<limit);
    }
  }
}

/* perform (L1) and (X9) ---------------------------------------------------- */

/*
 * Reset the embedding levels for some non-graphic characters (L1).
 * This function also sets appropriate levels for BN, and
 * explicit embedding types that are supposed to have been removed
 * from the paragraph in (X9).
 */
void nsBidi::adjustWSLevels()
{
  const DirProp *dirProps=mDirProps;
  UBidiLevel *levels=mLevels;
  PRInt32 i;

  if(mFlags&MASK_WS) {
    UBidiLevel paraLevel=mParaLevel;
    Flags flag;

    i=mTrailingWSStart;
    while(i>0) {
      /* reset a sequence of WS/BN before eop and B/S to the paragraph paraLevel */
      while(i>0 && DIRPROP_FLAG(dirProps[--i])&MASK_WS) {
        levels[i]=paraLevel;
      }

      /* reset BN to the next character's paraLevel until B/S, which restarts above loop */
      /* here, i+1 is guaranteed to be <length */
      while(i>0) {
        flag=DIRPROP_FLAG(dirProps[--i]);
        if(flag&MASK_BN_EXPLICIT) {
          levels[i]=levels[i+1];
        } else if(flag&MASK_B_S) {
          levels[i]=paraLevel;
          break;
        }
      }
    }
  }

  /* now remove the UBIDI_LEVEL_OVERRIDE flags, if any */
  /* (a separate loop can be optimized more easily by a compiler) */
  if(mFlags&MASK_OVERRIDE) {
    for(i=mTrailingWSStart; i>0;) {
      levels[--i]&=~UBIDI_LEVEL_OVERRIDE;
    }
  }
}

/* -------------------------------------------------------------------------- */

nsresult nsBidi::getDirection(UBidiDirection* direction)
{
  *direction = mDirection;
  return NS_OK;
}

nsresult nsBidi::getLength(PRInt32* length)
{
  *length = mLength;
  return NS_OK;
}

nsresult nsBidi::getParaLevel(UBidiLevel* paraLevel)
{
  *paraLevel = mParaLevel;
  return NS_OK;
}

/*
 * General remarks about the functions in this section:
 *
 * These functions deal with the aspects of potentially mixed-directional
 * text in a single paragraph or in a line of a single paragraph
 * which has already been processed according to
 * the Unicode 3.0 Bidi algorithm as defined in
 * http://www.unicode.org/unicode/reports/tr9/ , version 5,
 * also described in The Unicode Standard, Version 3.0 .
 *
 * This means that there is a nsBidi object with a levels
 * and a dirProps array.
 * paraLevel and direction are also set.
 * Only if the length of the text is zero, then levels==dirProps==NULL.
 *
 * The overall directionality of the paragraph
 * or line is used to bypass the reordering steps if possible.
 * Even purely RTL text does not need reordering there because
 * the getLogical/VisualIndex() functions can compute the
 * index on the fly in such a case.
 *
 * The implementation of the access to same-level-runs and of the reordering
 * do attempt to provide better performance and less memory usage compared to
 * a direct implementation of especially rule (L2) with an array of
 * one (32-bit) integer per text character.
 *
 * Here, the levels array is scanned as soon as necessary, and a vector of
 * same-level-runs is created. Reordering then is done on this vector.
 * For each run of text positions that were resolved to the same level,
 * only 8 bytes are stored: the first text position of the run and the visual
 * position behind the run after reordering.
 * One sign bit is used to hold the directionality of the run.
 * This is inefficient if there are many very short runs. If the average run
 * length is <2, then this uses more memory.
 *
 * In a further attempt to save memory, the levels array is never changed
 * after all the resolution rules (Xn, Wn, Nn, In).
 * Many functions have to consider the field trailingWSStart:
 * if it is less than length, then there is an implicit trailing run
 * at the paraLevel,
 * which is not reflected in the levels array.
 * This allows a line nsBidi object to use the same levels array as
 * its paragraph parent object.
 *
 * When a nsBidi object is created for a line of a paragraph, then the
 * paragraph's levels and dirProps arrays are reused by way of setting
 * a pointer into them, not by copying. This again saves memory and forbids to
 * change the now shared levels for (L1).
 */
nsresult nsBidi::setLine(nsIBidi* pParaBidi, PRInt32 start, PRInt32 limit)
{
	nsBidi* pParent = (nsBidi*)pParaBidi;
  PRInt32 length;

  /* check the argument values */
  if(pParent==NULL) {
    return NS_ERROR_INVALID_ARG;
  } else if(start<0 || start>limit || limit>pParent->mLength) {
    return NS_ERROR_INVALID_POINTER;
  }

  /* set members from our pParaBidi parent */
  length=mLength=limit-start;
  mParaLevel=pParent->mParaLevel;

  mRuns=NULL;
  mFlags=0;

  if(length>0) {
    mDirProps=pParent->mDirProps+start;
    mLevels=pParent->mLevels+start;
    mRunCount=-1;

    if(pParent->mDirection!=UBIDI_MIXED) {
      /* the parent is already trivial */
      mDirection=pParent->mDirection;

      /*
       * The parent's levels are all either
       * implicitly or explicitly ==paraLevel;
       * do the same here.
       */
      if(pParent->mTrailingWSStart<=start) {
        mTrailingWSStart=0;
      } else if(pParent->mTrailingWSStart<limit) {
        mTrailingWSStart=pParent->mTrailingWSStart-start;
      } else {
        mTrailingWSStart=length;
      }
    } else {
      const UBidiLevel *levels=mLevels;
      PRInt32 i, trailingWSStart;
      UBidiLevel level;
      Flags flags=0;

      setTrailingWSStart();
      trailingWSStart=mTrailingWSStart;

      /* recalculate pLineBidi->direction */
      if(trailingWSStart==0) {
        /* all levels are at paraLevel */
        mDirection=(UBidiDirection)(mParaLevel&1);
      } else {
        /* get the level of the first character */
        level=levels[0]&1;

        /* if there is anything of a different level, then the line is mixed */
        if(trailingWSStart<length && (mParaLevel&1)!=level) {
          /* the trailing WS is at paraLevel, which differs from levels[0] */
          mDirection=UBIDI_MIXED;
        } else {
          /* see if levels[1..trailingWSStart-1] have the same direction as levels[0] and paraLevel */
          i=1;
          for(;;) {
            if(i==trailingWSStart) {
              /* the direction values match those in level */
              mDirection=(UBidiDirection)level;
              break;
            } else if((levels[i]&1)!=level) {
              mDirection=UBIDI_MIXED;
              break;
            }
            ++i;
          }
        }
      }

      switch(mDirection) {
        case UBIDI_LTR:
          /* make sure paraLevel is even */
          mParaLevel=(mParaLevel+1)&~1;

          /* all levels are implicitly at paraLevel (important for getLevels()) */
          mTrailingWSStart=0;
          break;
        case UBIDI_RTL:
          /* make sure paraLevel is odd */
          mParaLevel|=1;

          /* all levels are implicitly at paraLevel (important for getLevels()) */
          mTrailingWSStart=0;
          break;
        default:
          break;
      }
    }
  } else {
    /* create an object for a zero-length line */
    mDirection=mParaLevel&1 ? UBIDI_RTL : UBIDI_LTR;
    mTrailingWSStart=mRunCount=0;

    mDirProps=NULL;
    mLevels=NULL;
  }
  return NS_OK;
}

nsresult nsBidi::getLevelAt(PRInt32 charIndex, UBidiLevel* level)
{
  /* return paraLevel if in the trailing WS run, otherwise the real level */
  if(charIndex<0 || mLength<=charIndex) {
    *level = 0;
  } else if(mDirection!=UBIDI_MIXED || charIndex>=mTrailingWSStart) {
    *level = mParaLevel;
  } else {
    *level = mLevels[charIndex];
  }
  return NS_OK;
}

nsresult nsBidi::getLevels(UBidiLevel** pLevels)
{
  PRInt32 start, length;

  length = mLength;
  if(length<=0) {
    *pLevels = NULL;
    return NS_ERROR_INVALID_ARG;
  }

  start = mTrailingWSStart;
  if(start==length) {
    /* the current levels array reflects the WS run */
    *pLevels = mLevels;
    return NS_OK;
  }

  /*
   * After the previous if(), we know that the levels array
   * has an implicit trailing WS run and therefore does not fully
   * reflect itself all the levels.
   * This must be a nsBidi object for a line, and
   * we need to create a new levels array.
   */

  if(getLevelsMemory(length)) {
    UBidiLevel *levels=mLevelsMemory;

    if(start>0 && levels!=mLevels) {
      nsCRT::memcpy(levels, mLevels, start);
    }
    nsCRT::memset(levels+start, mParaLevel, length-start);

    /* this new levels array is set for the line and reflects the WS run */
    mTrailingWSStart=length;
    *pLevels=mLevels=levels;
    return NS_OK;
  } else {
    /* out of memory */
    *pLevels = NULL;
    return NS_ERROR_OUT_OF_MEMORY;
  }
}

nsresult nsBidi::getClassAt(PRInt32 charIndex, DirProp* pClass)
{
  if(charIndex<0 || mLength<=charIndex) {
    return NS_ERROR_INVALID_ARG;
  } else {
    *pClass = mDirProps[charIndex];
    return NS_OK;
  }
}

nsresult nsBidi::getLogicalRun(PRInt32 logicalStart,
                 PRInt32 *pLogicalLimit, UBidiLevel *pLevel)
{
  PRInt32 length = mLength;

  if(logicalStart<0 || length<=logicalStart) {
    return NS_ERROR_INVALID_ARG;
  }

  if(mDirection!=UBIDI_MIXED || logicalStart>=mTrailingWSStart) {
    if(pLogicalLimit!=NULL) {
      *pLogicalLimit=length;
    }
    if(pLevel!=NULL) {
      *pLevel=mParaLevel;
    }
  } else {
    UBidiLevel *levels=mLevels;
    UBidiLevel level=levels[logicalStart];

    /* search for the end of the run */
    length=mTrailingWSStart;
    while(++logicalStart<length && level==levels[logicalStart]) {}

    if(pLogicalLimit!=NULL) {
      *pLogicalLimit=logicalStart;
    }
    if(pLevel!=NULL) {
      *pLevel=level;
    }
  }
  return NS_OK;
}

/* handle trailing WS (L1) -------------------------------------------------- */

/*
 * setTrailingWSStart() sets the start index for a trailing
 * run of WS in the line. This is necessary because we do not modify
 * the paragraph's levels array that we just point into.
 * Using trailingWSStart is another form of performing (L1).
 *
 * To make subsequent operations easier, we also include the run
 * before the WS if it is at the paraLevel - we merge the two here.
 */
void nsBidi::setTrailingWSStart() {
  /* mDirection!=UBIDI_MIXED */

  const DirProp *dirProps=mDirProps;
  UBidiLevel *levels=mLevels;
  PRInt32 start=mLength;
  UBidiLevel paraLevel=mParaLevel;

  /* go backwards across all WS, BN, explicit codes */
  while(start>0 && DIRPROP_FLAG(dirProps[start-1])&MASK_WS) {
    --start;
  }

  /* if the WS run can be merged with the previous run then do so here */
  while(start>0 && levels[start-1]==paraLevel) {
    --start;
  }

  mTrailingWSStart=start;
}

/* runs API functions ------------------------------------------------------- */

nsresult nsBidi::countRuns(PRInt32* runCount)
{
  if(mRunCount<0 && !getRuns()) {
    return NS_ERROR_OUT_OF_MEMORY;
  } else {
	  if (runCount)
		  *runCount = mRunCount;
	  return NS_OK;
  }
}

nsresult nsBidi::getVisualRun(PRInt32 runIndex,
                PRInt32 *pLogicalStart, PRInt32 *pLength, UBidiDirection *pDirection)
{
  if( runIndex<0 ||
      mRunCount==-1 && !getRuns() ||
      runIndex>=mRunCount
    ) {
    *pDirection = UBIDI_LTR;
    return NS_OK;
  } else {
    PRInt32 start=mRuns[runIndex].logicalStart;
    if(pLogicalStart!=NULL) {
      *pLogicalStart=GET_INDEX(start);
    }
    if(pLength!=NULL) {
      if(runIndex>0) {
        *pLength=mRuns[runIndex].visualLimit-
             mRuns[runIndex-1].visualLimit;
      } else {
        *pLength=mRuns[0].visualLimit;
      }
    }
    *pDirection = (UBidiDirection)GET_ODD_BIT(start);
    return NS_OK;
  }
}

/* compute the runs array --------------------------------------------------- */

/*
 * Compute the runs array from the levels array.
 * After getRuns() returns PR_TRUE, runCount is guaranteed to be >0
 * and the runs are reordered.
 * Odd-level runs have visualStart on their visual right edge and
 * they progress visually to the left.
 */
PRBool nsBidi::getRuns()
{
  if(mDirection!=UBIDI_MIXED) {
    /* simple, single-run case - this covers length==0 */
    getSingleRun(mParaLevel);
  } else /* UBIDI_MIXED, length>0 */ {
    /* mixed directionality */
    PRInt32 length=mLength, limit=length;

    /*
     * If there are WS characters at the end of the line
     * and the run preceding them has a level different from
     * paraLevel, then they will form their own run at paraLevel (L1).
     * Count them separately.
     * We need some special treatment for this in order to not
     * modify the levels array which a line nsBidi object shares
     * with its paragraph parent and its other line siblings.
     * In other words, for the trailing WS, it may be
     * levels[]!=paraLevel but we have to treat it like it were so.
     */
    limit=mTrailingWSStart;
    if(limit==0) {
      /* there is only WS on this line */
      getSingleRun(mParaLevel);
    } else {
      UBidiLevel *levels=mLevels;
      PRInt32 i, runCount;
      UBidiLevel level=UBIDI_DEFAULT_LTR;   /* initialize with no valid level */

      /* count the runs, there is at least one non-WS run, and limit>0 */
      runCount=0;
      for(i=0; i<limit; ++i) {
        /* increment runCount at the start of each run */
        if(levels[i]!=level) {
          ++runCount;
          level=levels[i];
        }
      }

      /*
       * We don't need to see if the last run can be merged with a trailing
       * WS run because setTrailingWSStart() would have done that.
       */
      if(runCount==1 && limit==length) {
        /* There is only one non-WS run and no trailing WS-run. */
        getSingleRun(levels[0]);
      } else /* runCount>1 || limit<length */ {
        /* allocate and set the runs */
        Run *runs;
        PRInt32 runIndex, start;
        UBidiLevel minLevel=UBIDI_MAX_EXPLICIT_LEVEL+1, maxLevel=0;

        /* now, count a (non-mergable) WS run */
        if(limit<length) {
          ++runCount;
        }

        /* runCount>1 */
        if(getRunsMemory(runCount)) {
          runs=mRunsMemory;
        } else {
          return PR_FALSE;
        }

        /* set the runs */
        /* this could be optimized, e.g.: 464->444, 484->444, 575->555, 595->555 */
        /* however, that would take longer and make other functions more complicated */
        runIndex=0;

        /* search for the run ends */
        start=0;
        level=levels[0];
        if(level<minLevel) {
          minLevel=level;
        }
        if(level>maxLevel) {
          maxLevel=level;
        }

        /* initialize visualLimit values with the run lengths */
        for(i=1; i<limit; ++i) {
          if(levels[i]!=level) {
            /* i is another run limit */
            runs[runIndex].logicalStart=start;
            runs[runIndex].visualLimit=i-start;
            start=i;

            level=levels[i];
            if(level<minLevel) {
              minLevel=level;
            }
            if(level>maxLevel) {
              maxLevel=level;
            }
            ++runIndex;
          }
        }

        /* finish the last run at i==limit */
        runs[runIndex].logicalStart=start;
        runs[runIndex].visualLimit=limit-start;
        ++runIndex;

        if(limit<length) {
          /* there is a separate WS run */
          runs[runIndex].logicalStart=limit;
          runs[runIndex].visualLimit=length-limit;
          if(mParaLevel<minLevel) {
            minLevel=mParaLevel;
          }
        }

        /* set the object fields */
        mRuns=runs;
        mRunCount=runCount;

        reorderLine(minLevel, maxLevel);

        /* now add the direction flags and adjust the visualLimit's to be just that */
        ADD_ODD_BIT_FROM_LEVEL(runs[0].logicalStart, levels[runs[0].logicalStart]);
        limit=runs[0].visualLimit;
        for(i=1; i<runIndex; ++i) {
          ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, levels[runs[i].logicalStart]);
          limit=runs[i].visualLimit+=limit;
        }

        /* same for the trailing WS run */
        if(runIndex<runCount) {
          ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, mParaLevel);
          runs[runIndex].visualLimit+=limit;
        }
      }
    }
  }
  return PR_TRUE;
}

/* in trivial cases there is only one trivial run; called by getRuns() */
void nsBidi::getSingleRun(UBidiLevel level)
{
  /* simple, single-run case */
  mRuns=mSimpleRuns;
  mRunCount=1;

  /* fill and reorder the single run */
  mRuns[0].logicalStart=MAKE_INDEX_ODD_PAIR(0, level);
  mRuns[0].visualLimit=mLength;
}

/* reorder the runs array (L2) ---------------------------------------------- */

/*
 * Reorder the same-level runs in the runs array.
 * Here, runCount>1 and maxLevel>=minLevel>=paraLevel.
 * All the visualStart fields=logical start before reordering.
 * The "odd" bits are not set yet.
 *
 * Reordering with this data structure lends itself to some handy shortcuts:
 *
 * Since each run is moved but not modified, and since at the initial maxLevel
 * each sequence of same-level runs consists of only one run each, we
 * don't need to do anything there and can predecrement maxLevel.
 * In many simple cases, the reordering is thus done entirely in the
 * index mapping.
 * Also, reordering occurs only down to the lowest odd level that occurs,
 * which is minLevel|1. However, if the lowest level itself is odd, then
 * in the last reordering the sequence of the runs at this level or higher
 * will be all runs, and we don't need the elaborate loop to search for them.
 * This is covered by ++minLevel instead of minLevel|=1 followed
 * by an extra reorder-all after the reorder-some loop.
 * About a trailing WS run:
 * Such a run would need special treatment because its level is not
 * reflected in levels[] if this is not a paragraph object.
 * Instead, all characters from trailingWSStart on are implicitly at
 * paraLevel.
 * However, for all maxLevel>paraLevel, this run will never be reordered
 * and does not need to be taken into account. maxLevel==paraLevel is only reordered
 * if minLevel==paraLevel is odd, which is done in the extra segment.
 * This means that for the main reordering loop we don't need to consider
 * this run and can --runCount. If it is later part of the all-runs
 * reordering, then runCount is adjusted accordingly.
 */
void nsBidi::reorderLine(UBidiLevel minLevel, UBidiLevel maxLevel)
{
  Run *runs;
  UBidiLevel *levels;
  PRInt32 firstRun, endRun, limitRun, runCount,
  temp, trailingWSStart=mTrailingWSStart;

  /* nothing to do? */
  if(maxLevel<=(minLevel|1)) {
    return;
  }

  /*
   * Reorder only down to the lowest odd level
   * and reorder at an odd minLevel in a separate, simpler loop.
   * See comments above for why minLevel is always incremented.
   */
  ++minLevel;

  runs=mRuns;
  levels=mLevels;
  runCount=mRunCount;

  /* do not include the WS run at paraLevel<=old minLevel except in the simple loop */
  if(mTrailingWSStart<mLength) {
    --runCount;
  }

  while(--maxLevel>=minLevel) {
    firstRun=0;

    /* loop for all sequences of runs */
    for(;;) {
      /* look for a sequence of runs that are all at >=maxLevel */
      /* look for the first run of such a sequence */
      while(firstRun<runCount && levels[runs[firstRun].logicalStart]<maxLevel) {
        ++firstRun;
      }
      if(firstRun>=runCount) {
        break;  /* no more such runs */
      }

      /* look for the limit run of such a sequence (the run behind it) */
      for(limitRun=firstRun; ++limitRun<runCount && levels[runs[limitRun].logicalStart]>=maxLevel;) {}

      /* Swap the entire sequence of runs from firstRun to limitRun-1. */
      endRun=limitRun-1;
      while(firstRun<endRun) {
        temp=runs[firstRun].logicalStart;
        runs[firstRun].logicalStart=runs[endRun].logicalStart;
        runs[endRun].logicalStart=temp;

        temp=runs[firstRun].visualLimit;
        runs[firstRun].visualLimit=runs[endRun].visualLimit;
        runs[endRun].visualLimit=temp;

        ++firstRun;
        --endRun;
      }

      if(limitRun==runCount) {
        break;  /* no more such runs */
      } else {
        firstRun=limitRun+1;
      }
    }
  }

  /* now do maxLevel==old minLevel (==odd!), see above */
  if(!(minLevel&1)) {
    firstRun=0;

    /* include the trailing WS run in this complete reordering */
    if(mTrailingWSStart==mLength) {
      --runCount;
    }

    /* Swap the entire sequence of all runs. (endRun==runCount) */
    while(firstRun<runCount) {
      temp=runs[firstRun].logicalStart;
      runs[firstRun].logicalStart=runs[runCount].logicalStart;
      runs[runCount].logicalStart=temp;

      temp=runs[firstRun].visualLimit;
      runs[firstRun].visualLimit=runs[runCount].visualLimit;
      runs[runCount].visualLimit=temp;

      ++firstRun;
      --runCount;
    }
  }
}

/* reorder a line based on a levels array (L2) ------------------------------ */

nsresult nsBidi::reorderLogical(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap)
{
  PRInt32 start, limit, sumOfSosEos;
  UBidiLevel minLevel, maxLevel;

  if(indexMap==NULL || !prepareReorder(levels, length, indexMap, &minLevel, &maxLevel)) {
    return NS_OK;
  }

  /* nothing to do? */
  if(minLevel==maxLevel && (minLevel&1)==0) {
    return NS_OK;
  }

  /* reorder only down to the lowest odd level */
  minLevel|=1;

  /* loop maxLevel..minLevel */
  do {
    start=0;

    /* loop for all sequences of levels to reorder at the current maxLevel */
    for(;;) {
      /* look for a sequence of levels that are all at >=maxLevel */
      /* look for the first index of such a sequence */
      while(start<length && levels[start]<maxLevel) {
        ++start;
      }
      if(start>=length) {
        break;  /* no more such sequences */
      }

      /* look for the limit of such a sequence (the index behind it) */
      for(limit=start; ++limit<length && levels[limit]>=maxLevel;) {}

      /*
       * sos=start of sequence, eos=end of sequence
       *
       * The closed (inclusive) interval from sos to eos includes all the logical
       * and visual indexes within this sequence. They are logically and
       * visually contiguous and in the same range.
       *
       * For each run, the new visual index=sos+eos-old visual index;
       * we pre-add sos+eos into sumOfSosEos ->
       * new visual index=sumOfSosEos-old visual index;
       */
      sumOfSosEos=start+limit-1;

      /* reorder each index in the sequence */
      do {
        indexMap[start]=sumOfSosEos-indexMap[start];
      } while(++start<limit);

      /* start==limit */
      if(limit==length) {
        break;  /* no more such sequences */
      } else {
        start=limit+1;
      }
    }
  } while(--maxLevel>=minLevel);

  return NS_OK;
}

nsresult nsBidi::reorderVisual(const UBidiLevel *levels, PRInt32 length, PRInt32 *indexMap)
{
  PRInt32 start, end, limit, temp;
  UBidiLevel minLevel, maxLevel;

  if(indexMap==NULL || !prepareReorder(levels, length, indexMap, &minLevel, &maxLevel)) {
    return NS_OK;
  }

  /* nothing to do? */
  if(minLevel==maxLevel && (minLevel&1)==0) {
    return NS_OK;
  }

  /* reorder only down to the lowest odd level */
  minLevel|=1;

  /* loop maxLevel..minLevel */
  do {
    start=0;

    /* loop for all sequences of levels to reorder at the current maxLevel */
    for(;;) {
      /* look for a sequence of levels that are all at >=maxLevel */
      /* look for the first index of such a sequence */
      while(start<length && levels[start]<maxLevel) {
        ++start;
      }
      if(start>=length) {
        break;  /* no more such runs */
      }

      /* look for the limit of such a sequence (the index behind it) */
      for(limit=start; ++limit<length && levels[limit]>=maxLevel;) {}

      /*
       * Swap the entire interval of indexes from start to limit-1.
       * We don't need to swap the levels for the purpose of this
       * algorithm: the sequence of levels that we look at does not
       * move anyway.
       */
      end=limit-1;
      while(start<end) {
        temp=indexMap[start];
        indexMap[start]=indexMap[end];
        indexMap[end]=temp;

        ++start;
        --end;
      }

      if(limit==length) {
        break;  /* no more such sequences */
      } else {
        start=limit+1;
      }
    }
  } while(--maxLevel>=minLevel);

  return NS_OK;
}

PRBool nsBidi::prepareReorder(const UBidiLevel *levels, PRInt32 length,
                PRInt32 *indexMap,
                UBidiLevel *pMinLevel, UBidiLevel *pMaxLevel)
{
  PRInt32 start;
  UBidiLevel level, minLevel, maxLevel;

  if(levels==NULL || length<=0) {
    return PR_FALSE;
  }

  /* determine minLevel and maxLevel */
  minLevel=UBIDI_MAX_EXPLICIT_LEVEL+1;
  maxLevel=0;
  for(start=length; start>0;) {
    level=levels[--start];
    if(level>UBIDI_MAX_EXPLICIT_LEVEL+1) {
      return PR_FALSE;
    }
    if(level<minLevel) {
      minLevel=level;
    }
    if(level>maxLevel) {
      maxLevel=level;
    }
  }
  *pMinLevel=minLevel;
  *pMaxLevel=maxLevel;

  /* initialize the index map */
  for(start=length; start>0;) {
    --start;
    indexMap[start]=start;
  }

  return PR_TRUE;
}

/* API functions for logical<->visual mapping ------------------------------- */

nsresult nsBidi::getVisualIndex(PRInt32 logicalIndex, PRInt32* visualIndex) {
  if(logicalIndex<0 || mLength<=logicalIndex) {
    return NS_ERROR_INVALID_ARG;
  } else {
    /* we can do the trivial cases without the runs array */
    switch(mDirection) {
      case UBIDI_LTR:
        *visualIndex = logicalIndex;
        return NS_OK;
      case UBIDI_RTL:
        *visualIndex = mLength-logicalIndex-1;
        return NS_OK;
      default:
        if(mRunCount<0 && !getRuns()) {
          return NS_ERROR_OUT_OF_MEMORY;
        } else {
          Run *runs=mRuns;
          PRInt32 i, visualStart=0, offset, length;

          /* linear search for the run, search on the visual runs */
          for(i=0;; ++i) {
            length=runs[i].visualLimit-visualStart;
            offset=logicalIndex-GET_INDEX(runs[i].logicalStart);
            if(offset>=0 && offset<length) {
              if(IS_EVEN_RUN(runs[i].logicalStart)) {
                /* LTR */
                *visualIndex = visualStart+offset;
                return NS_OK;
              } else {
                /* RTL */
                *visualIndex = visualStart+length-offset-1;
                return NS_OK;
              }
            }
            visualStart+=length;
          }
        }
    }
  }
}

nsresult nsBidi::getLogicalIndex(PRInt32 visualIndex, PRInt32 *logicalIndex)
{
  if(visualIndex<0 || mLength<=visualIndex) {
    return NS_ERROR_INVALID_ARG;
  } else {
    /* we can do the trivial cases without the runs array */
    switch(mDirection) {
      case UBIDI_LTR:
        *logicalIndex = visualIndex;
        return NS_OK;
      case UBIDI_RTL:
        *logicalIndex = mLength-visualIndex-1;
        return NS_OK;
      default:
        if(mRunCount<0 && !getRuns()) {
          return NS_ERROR_OUT_OF_MEMORY;
        } else {
          Run *runs=mRuns;
          PRInt32 i, runCount=mRunCount, start;

          if(runCount<=10) {
            /* linear search for the run */
            for(i=0; visualIndex>=runs[i].visualLimit; ++i) {}
          } else {
            /* binary search for the run */
            PRInt32 start=0, limit=runCount;

            /* the middle if() will guaranteed find the run, we don't need a loop limit */
            for(;;) {
              i=(start+limit)/2;
              if(visualIndex>=runs[i].visualLimit) {
                start=i+1;
              } else if(i==0 || visualIndex>=runs[i-1].visualLimit) {
                break;
              } else {
                limit=i;
              }
            }
          }

          start=runs[i].logicalStart;
          if(IS_EVEN_RUN(start)) {
            /* LTR */
            /* the offset in runs[i] is visualIndex-runs[i-1].visualLimit */
            if(i>0) {
              visualIndex-=runs[i-1].visualLimit;
            }
            *logicalIndex = GET_INDEX(start)+visualIndex;
            return NS_OK;
          } else {
            /* RTL */
            *logicalIndex = GET_INDEX(start)+runs[i].visualLimit-visualIndex-1;
            return NS_OK;
          }
        }
    }
  }
}

nsresult nsBidi::getLogicalMap(PRInt32 *indexMap)
{
  UBidiLevel *levels;
  nsresult rv;

  /* getLevels() checks all of its and our arguments */
  rv = getLevels(&levels);
  if(NS_FAILED(rv)) {
    return rv;
  } else if(indexMap==NULL) {
    return NS_ERROR_INVALID_ARG;
  } else {
    return reorderLogical(levels, mLength, indexMap);
  }
}

nsresult nsBidi::getVisualMap(PRInt32 *indexMap)
{
  PRInt32* runCount=NULL;
  nsresult rv;

  /* countRuns() checks all of its and our arguments */
  rv = countRuns(runCount);
  if(NS_FAILED(rv)) {
    return rv;
  } else if(indexMap==NULL) {
    return NS_ERROR_INVALID_ARG;
  } else {
    /* fill a visual-to-logical index map using the runs[] */
    Run *runs=mRuns, *runsLimit=runs+mRunCount;
    PRInt32 logicalStart, visualStart, visualLimit;

    visualStart=0;
    for(; runs<runsLimit; ++runs) {
      logicalStart=runs->logicalStart;
      visualLimit=runs->visualLimit;
      if(IS_EVEN_RUN(logicalStart)) {
        do { /* LTR */
          *indexMap++ = logicalStart++;
        } while(++visualStart<visualLimit);
      } else {
        REMOVE_ODD_BIT(logicalStart);
        logicalStart+=visualLimit-visualStart;  /* logicalLimit */
        do { /* RTL */
          *indexMap++ = --logicalStart;
        } while(++visualStart<visualLimit);
      }
      /* visualStart==visualLimit; */
    }
    return NS_OK;
  }
}

nsresult nsBidi::invertMap(const PRInt32 *srcMap, PRInt32 *destMap, PRInt32 length)
{
  if(srcMap!=NULL && destMap!=NULL) {
    srcMap+=length;
    while(length>0) {
      destMap[*--srcMap]=--length;
    }
  }
  return NS_OK;
}

// IBMBIDI - EGYPT - Start
/*
void nsBidi::HebrewReordering(const PRUnichar *aString, PRUint32 aLen,
        PRUnichar* aBuf, PRUint32 &aBufLen)
{
   const PRUnichar* src=aString + aLen - 1;
   PRUnichar* dest= aBuf;
   while(src>=aString)
       *dest++ =  *src--;
   aBufLen = aLen;
}

void nsBidi::ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
             PRUnichar* aBuf, PRUint32 &aBufLen, PRUint32* map)
{
   const PRUnichar* src = aString+aLen-1;
   const PRUnichar* p;
   PRUnichar* dest = aBuf;
   
   PRUnichar formB;
   PRInt8 leftJ, thisJ, rightJ;
   PRInt8 leftNoTrJ, rightNoTrJ;
   thisJ = eNJ;
   rightJ = GetJoiningClass(*(src)) ;
   while(src>aString) {
      leftJ = thisJ;
#ifdef IBMBIDI
			//if((eTr != leftJ) || ((leftJ == eTr) && (*(src+1)) == 0x0020))
			if ((eTr != leftJ) || ((leftJ == eTr) && !CHAR_IS_ARABIC(*(src+1))))
				leftNoTrJ = thisJ;
      //for(p=src+2; (eTr == leftNoTrJ) && ((*(p-1)) != 0x0020) && (p <= (aString+aLen-1)); p++)
			for(p=src+2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p-1))) && (p <= (aString+aLen-1)); p++)
          leftNoTrJ = GetJoiningClass(*(p)) ;
#else
      if(eTr != thisJ)
        leftNoTrJ = thisJ;
#endif // IBMBIDI
      thisJ = rightJ;
      rightJ = rightNoTrJ = GetJoiningClass(*(src-1)) ;
#ifdef IBMBIDI
      //for(p=src-2; (eTr == rightNoTrJ) && ((*(src-1)) != 0x0020) && (p >= aString); p--)
			for(p=src-2; (eTr == rightNoTrJ) && (CHAR_IS_ARABIC(*(src-1))) && (p >= aString); p--)
          rightNoTrJ = GetJoiningClass(*(p)) ;
#else
      for(p=src-2; (eTr == rightNoTrJ) && (p >= src); p--) 
          rightNoTrJ = GetJoiningClass(*(p)) ;
#endif // IBMBIDI
      formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
      if(FONT_HAS_GLYPH(map,formB))
          *dest++ = formB;
      else
          *dest++ = PresentationFormB(*src, eIsolated);
//printf("%x %d %d %d %x\n" ,*src,leftJ, thisJ, rightJ, 
//PresentationFormB(*src, DecideForm(leftJ, thisJ, rightJ)));
      src--;
   }
#ifdef IBMBIDI
	  //if((eTr != thisJ) || ((thisJ == eTr) && (*(src+1)) == 0x0020))
	 if((eTr != thisJ) || ((thisJ == eTr) && (!CHAR_IS_ARABIC(*(src+1)))))
	    leftNoTrJ = thisJ;
   //for(p=src+2; (eTr == leftNoTrJ) && ((*(p-1)) != 0x0020) && (p <= (aString+aLen-1)); p++)
	 for(p=src+2; (eTr == leftNoTrJ) && (CHAR_IS_ARABIC(*(p-1))) && (p <= (aString+aLen-1)); p++)
        leftNoTrJ = GetJoiningClass(*(p)) ;
#else
   if(eTr != thisJ)
     leftNoTrJ = thisJ;
#endif // IBMBIDI
   formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
   if(FONT_HAS_GLYPH(map,formB))
       *dest++ = formB;
   else
       *dest++ = PresentationFormB(*src, eIsolated);
//printf("%x %d %d %d %x\n" ,*src, thisJ, rightJ, eNJ,
//PresentationFormB(*src, DecideForm( thisJ, rightJ, eNJ)));
   src--;
   PRUnichar *lSrc = aBuf;
   PRUnichar *lDest = aBuf;
   while(lSrc < (dest-1))
   {
      PRUnichar next = *(lSrc+1);
      if(((0xFEDF == next) || (0xFEE0 == next)) && 
         (0xFE80 == (0xFFF1 & *lSrc))) 
      {
         PRBool done = PR_FALSE;
         PRUint16 key = ((*lSrc) << 8) | ( 0x00FF & next);
         PRUint16 i;
         for(i=0;i<8;i++)
         {
             if(key == gArabicLigatureMap[i])
             {
                done = PR_TRUE;
                *lDest++ = 0xFEF5 + i;
                lSrc+=2;
                break;
             }
         }
         if(! done)
             *lDest++ = *lSrc++; 
      } else {
        *lDest++ = *lSrc++; 
      }
   }
   if(lSrc < dest)
      *lDest++ = *lSrc++; 
   aBufLen = lDest - aBuf; 
#if 0
printf("[");
for(PRUint32 k=0;k<aBufLen;k++)
  printf("%x ", aBuf[k]);
printf("]\n");
#endif
}

PRBool nsBidi::NeedComplexScriptHandling(const PRUnichar *aString, PRUint32 aLen,
       PRBool bFontSupportHebrew, PRBool* oHebrew,
       PRBool bFontSupportArabic, PRBool* oArabic)
{
  PRUint32 i;
  *oHebrew = *oArabic = PR_FALSE;
  if(bFontSupportArabic && bFontSupportHebrew)
  {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=PR_TRUE;
          break;
       } else if(CHAR_IS_ARABIC(aString[i])) {
          *oArabic=PR_TRUE;
          break;
       }
     }
  } else if(bFontSupportHebrew) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_HEBREW(aString[i])) {
          *oHebrew=PR_TRUE;
          break;
       }
     }
  } else if(bFontSupportArabic) {
     for(i=0;i<aLen;i++)
     {
       if(CHAR_IS_ARABIC(aString[i])) {
          *oArabic=PR_TRUE;
          break;
       }
     }
  }
  return *oArabic || *oHebrew;
}

void nsBidi::numbers_to_arabic (PRUnichar* uch)
{
  if ((*uch>=BIDI_START_HINDI_DIGITS) && (*uch<=BIDI_END_HINDI_DIGITS))
    *uch -= (uint16)BIDI_DIGIT_INCREMENT;
}

void nsBidi::numbers_to_hindi (PRUnichar* uch)
{
  if ((*uch>=BIDI_START_ARABIC_DIGITS) && (*uch<=BIDI_END_ARABIC_DIGITS))
    *uch += (uint16)BIDI_DIGIT_INCREMENT;
}

void nsBidi::HandleNumbers (PRUnichar* Buffer, PRUint32 size, PRUint32  Num_Flag)
{
  uint32 i;
	// IBMBIDI_NUMERAL_REGULAR *
	// IBMBIDI_NUMERAL_HINDICONTEXT
	// IBMBIDI_NUMERAL_ARABIC
	// IBMBIDI_NUMERAL_HINDI
	switch (Num_Flag)
	{
  case IBMBIDI_NUMERAL_HINDI:
    for (i=0;i<size;i++)
      nsBidi::numbers_to_hindi(&(Buffer[i]));
		break;
  case IBMBIDI_NUMERAL_ARABIC:
    for (i=0;i<size;i++)
      nsBidi::numbers_to_arabic(&(Buffer[i]));
		break;
	default : // IBMBIDI_NUMERAL_REGULAR, IBMBIDI_NUMERAL_HINDICONTEXT
    for (i=0;i<size;i++)
		{
			if (i>0) // not 1st char
				if (IS_ARABIC_CHAR(Buffer[i-1])) nsBidi::numbers_to_hindi(&(Buffer[i]));
			else nsBidi::numbers_to_arabic(&(Buffer[i]));
		}
		break;
  }
}
*/
/*
void nsBidi::Conv_FE_06 (nsAutoTextBuffer * aTextBuffer, PRUint32 * txtSizeChange)
{
	// Do the conversion here - :)
  uint32 i,j, size = aTextBuffer->GetBufferLength();
	nsAutoTextBuffer DistTextBuffer;
	DistTextBuffer.GrowTo(size);
	PRUnichar* Buffer = DistTextBuffer.mBuffer, CurrentUC;
  for (i=0,j=0;j<size;i++,j++)
	{ // j : Source, i : Distination
		CurrentUC = aTextBuffer->mBuffer[j];
		if (CurrentUC == 0x0000) break; // no need to convert char after the NULL
		Buffer[i] = CurrentUC; // copy it even if it is not in FE range
		if IS_FE_CHAR(CurrentUC)
		{
			Buffer[i] = FE_TO_06[CurrentUC-FE_TO_06_OFFSET][0];
			if (FE_TO_06[CurrentUC-FE_TO_06_OFFSET][1])
			{
				// Two characters, we have to resize the buffer :(
				size++;
				nsresult rv = DistTextBuffer.GrowBy(1); // Increase one char
				if (NS_FAILED(rv)) {
					break;
				}// couldn't graw
				Buffer = DistTextBuffer.mBuffer; // It may be re allocated
				Buffer[i+1] = FE_TO_06[CurrentUC-FE_TO_06_OFFSET][1];
				i++;
			} // if two
		}// is FE
	}// for : loop the buffer

	*txtSizeChange = i - j;

	// Copy Distination to source
	aTextBuffer->GrowTo(size);
  for (i=0;i<size;i++)
	{
		if (DistTextBuffer.mBuffer[i] == 0x0000) break;// no need to copy char after the NULL
		aTextBuffer->mBuffer[i] = DistTextBuffer.mBuffer[i];
	}
  for (i=0;i<NS_TEXT_TRANSFORMER_AUTO_WORD_BUF_SIZE;i++)
	{
		if (DistTextBuffer.mBuffer[i] == 0x0000) break;// no need to copy char after the NULL
		aTextBuffer->mAutoBuffer[i] = DistTextBuffer.mBuffer[i];
	}
}
*/
// IBMBIDI - EGYPT - End
static PRInt32 doWriteReverse(const PRUnichar *src, PRInt32 srcLength,
                              PRUnichar *dest, PRUint16 options) {
  /*
   * RTL run -
   *
   * RTL runs need to be copied to the destination in reverse order
   * of code points, not code units, to keep Unicode characters intact.
   *
   * The general strategy for this is to read the source text
   * in backward order, collect all code units for a code point
   * (and optionally following combining characters, see below),
   * and copy all these code units in ascending order
   * to the destination for this run.
   *
   * Several options request whether combining characters
   * should be kept after their base characters,
   * whether Bidi control characters should be removed, and
   * whether characters should be replaced by their mirror-image
   * equivalent Unicode characters.
   */
  PRInt32 i, j, destSize;
  PRUint32 c;

  static nsresult bcRv;
  NS_WITH_SERVICE(nsIUBidiUtils, BidiUtils, kUnicharBidiUtilCID, &bcRv);

  /* optimize for several combinations of options */
  switch(options&(UBIDI_REMOVE_BIDI_CONTROLS|UBIDI_DO_MIRRORING|UBIDI_KEEP_BASE_COMBINING)) {
    case 0:
    /*
         * With none of the "complicated" options set, the destination
         * run will have the same length as the source run,
         * and there is no mirroring and no keeping combining characters
         * with their base characters.
         */
      destSize=srcLength;

    /* preserve character integrity */
      do {
      /* i is always after the last code unit known to need to be kept in this segment */
        i=srcLength;

      /* collect code units for one base character */
        UTF_BACK_1(src, 0, srcLength);

      /* copy this base character */
        j=srcLength;
        do {
          *dest++=src[j++];
        } while(j<i);
      } while(srcLength>0);
      break;
    case UBIDI_KEEP_BASE_COMBINING:
    /*
         * Here, too, the destination
         * run will have the same length as the source run,
         * and there is no mirroring.
         * We do need to keep combining characters with their base characters.
         */
      destSize=srcLength;

    /* preserve character integrity */
      do {
      /* i is always after the last code unit known to need to be kept in this segment */
        i=srcLength;

      /* collect code units and modifier letters for one base character */
        PRBool isModifier;
        do {
          UTF_PREV_CHAR(src, 0, srcLength, c);
          BidiUtils->Is(c, eBidiCat_NSM, &isModifier);
        } while(srcLength>0 && isModifier);

      /* copy this "user character" */
        j=srcLength;
        do {
          *dest++=src[j++];
        } while(j<i);
      } while(srcLength>0);
      break;
    default:
    /*
         * With several "complicated" options set, this is the most
         * general and the slowest copying of an RTL run.
         * We will do mirroring, remove Bidi controls, and
         * keep combining characters with their base characters
         * as requested.
         */
      if(!(options&UBIDI_REMOVE_BIDI_CONTROLS)) {
        i=srcLength;
      } else {
      /* we need to find out the destination length of the run,
               which will not include the Bidi control characters */
        PRInt32 length=srcLength;
        PRUnichar c;
        PRBool isBidiControl;

        i=0;
        do {
          c=*src++;
          BidiUtils->IsControl(c, &isBidiControl);
          if(!isBidiControl) {
            ++i;
          }
        } while(--length>0);
        src-=srcLength;
      }
      destSize=i;

    /* preserve character integrity */
      do {
      /* i is always after the last code unit known to need to be kept in this segment */
        i=srcLength;

      /* collect code units for one base character */
        UTF_PREV_CHAR(src, 0, srcLength, c);
        if(options&UBIDI_KEEP_BASE_COMBINING) {
        /* collect modifier letters for this base character */
          PRBool isModifier;
          BidiUtils->Is(c, eBidiCat_NSM, &isModifier);
          while(srcLength>0 && isModifier) {
            UTF_PREV_CHAR(src, 0, srcLength, c);
            BidiUtils->Is(c, eBidiCat_NSM, &isModifier);
          }
        }

        PRBool isBidiControl;
        BidiUtils->IsControl(c, &isBidiControl);

        if(options&UBIDI_REMOVE_BIDI_CONTROLS && isBidiControl) {
        /* do not copy this Bidi control character */
          continue;
        }

      /* copy this "user character" */
        j=srcLength;
        if(options&UBIDI_DO_MIRRORING) {
          /* mirror only the base character */
          if (NS_FAILED(bcRv) || !BidiUtils)
            ; /* default to the original form */
          else
            BidiUtils->SymmSwap((PRUnichar*)&c);
          PRInt32 k=0;
          UTF_APPEND_CHAR_UNSAFE(dest, k, c);
          dest+=k;
          j+=k;
        }
        while(j<i) {
          *dest++=src[j++];
        }
      } while(srcLength>0);
      break;
  } /* end of switch */
  return destSize;
}

nsresult nsBidi::writeReverse(const PRUnichar *src, PRInt32 srcLength, PRUnichar *dest, PRUint16 options, PRInt32 *destSize)
{
  if( src==NULL || srcLength<0 ||
      dest==NULL
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  /* do input and output overlap? */
  if( src>=dest && src<dest+srcLength ||
      dest>=src && dest<src+srcLength
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  if(srcLength>0) {
    *destSize = doWriteReverse(src, srcLength, dest, options);
  }
  return NS_OK;
}

#endif // IBMBIDI
