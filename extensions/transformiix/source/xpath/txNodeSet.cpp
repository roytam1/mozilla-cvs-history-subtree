/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2003
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Keith Visco <kvisco@ziplink.net>
 *   Peter Van der Beken <peterv@netscape.com>
 *
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

#include "txNodeSet.h"
#include "TxLog.h"
#include "nsMemory.h"

/**
 * Implementation of an XPath nodeset
 */

static const PRInt32 kTxNodeSetMinSize = 4;
static const PRInt32 kTxNodeSetGrowFactor = 2;

#define kForward   1
#define kReversed -1

txNodeSet::txNodeSet(txResultRecycler* aRecycler) : txAExprResult(aRecycler),
                                                    mElements(nsnull),
                                                    mBufferSize(0),
                                                    mElementCount(0),
                                                    mDirection(kForward)
{
}

txNodeSet::txNodeSet(const txXPathNode& aNode, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler),
      mElements(nsnull),
      mBufferSize(0),
      mElementCount(0),
      mDirection(kForward)
{
    if (!ensureSize(1)) {
        NS_ASSERTION(0, "out of memory");
        return;
    }

    txXPathNode* temp = new(mElements) txXPathNode(aNode);
    if (temp) {
        ++mElementCount;
    }
}

txNodeSet::txNodeSet(const txNodeSet& aSource, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler),
      mElements(nsnull),
      mBufferSize(0),
      mElementCount(0),
      mDirection(kForward)
{
    append(aSource);
}

txNodeSet::~txNodeSet()
{
    if (mElements) {
        while (--mElementCount >= 0) {
            mElements[mElementCount].~txXPathNode();
        }
        nsMemory::Free(mElements);
    }
}

nsresult txNodeSet::add(const txXPathNode& aNode)
{
    PRBool nonDup;
    if (mElementCount == 0) {
        return append(aNode);
    }

    PRInt32 pos;
    if (mDirection == kForward) {
        pos = findPosition(aNode, 0, mElementCount - 1, nonDup);
NS_ASSERTION(pos >= -1 && pos <= mElementCount, "Bad pos");
    }
    else {
        pos = findPosition(aNode, mElementCount - 1, 0, nonDup);
NS_ASSERTION(pos >= -1 && pos <= mElementCount, "Bad pos");
    }

    if (!nonDup) {
        return NS_OK;
    }

    if (!ensureSize(mElementCount + 1)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (pos < mElementCount) {
        memmove(&mElements[pos + 1], &mElements[pos],
                (mElementCount - pos) * sizeof(txXPathNode));
    }

    txXPathNode* temp = new(&mElements[pos]) txXPathNode(aNode);
    if (!temp) {
        if (pos < mElementCount) {
            memmove(&mElements[pos], &mElements[pos + 1],
                    (mElementCount - pos) * sizeof(txXPathNode));
        }

        return NS_ERROR_OUT_OF_MEMORY;
    }

    ++mElementCount;

    return NS_OK;
}

/**
 * The code is optimized to make a minimum number of calls to
 * Node::compareDocumentPosition. The idea is this:
 * We have the two nodesets (number indicate "document position")
 * 
 * 1 3 7             <- source 1
 * 2 3 6 8 9         <- source 2
 * _ _ _ _ _ _ _ _   <- result
 * 
 * 
 * We select the last node in the smallest nodeset and find where in the other
 * nodeset it would be inserted. In this case we would take the 7 from the
 * first nodeset and find the position between the 6 and 8 in the second.
 * We then take the nodes after the insert-position and move it to the end of
 * the resulting nodeset, and then do the same for the node from the smaller
 * nodeset. Which in this case means that we'd first move the 8 and 9 nodes,
 * and then the 7 node, giving us the following:
 * 
 * 1 3               <- source 1
 * 2 3 6             <- source 2
 * _ _ _ _ _ 7 8 9   <- result
 * 
 * Repeat until one of the nodesets are empty. If we find a duplicate node
 * when searching for where insertposition we skip the step where we move the
 * node from the smaller nodeset to the resulting nodeset. So in this next
 * step in the example we would only move the 3 and 6 nodes from the second
 * nodeset and then just remove the 3 node from the first nodeset. Giving:
 * 
 * 1                 <- source 1
 * 2                 <- source 2
 * _ _ _ 3 6 7 8 9   <- result
 * 
 * We might therefor end up with some blanks in the beginning of the resulting
 * nodeset, which we simply fix by moving all the nodes one step down.
 *
 */
nsresult txNodeSet::add(const txNodeSet& aNodes)
{
    return add(aNodes, copyElements);
}

nsresult txNodeSet::addAndTransfer(txNodeSet* aNodes)
{
    nsresult rv = add(*aNodes, transferElements);

    if (aNodes->mElements) {
        nsMemory::Free(aNodes->mElements);
        aNodes->mElements = nsnull;
        aNodes->mBufferSize = 0;
        aNodes->mElementCount = 0;
    }

    return rv;
}

nsresult txNodeSet::add(const txNodeSet& aNodes, transferOp aTransfer)
{
    if (aNodes.mElementCount == 0) {
        return NS_OK;
    }

    if (!ensureSize(mElementCount + aNodes.mElementCount)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    // This is probably a rather common case, so lets try to shortcut.
    if (mElementCount == 0 ||
        txXPathNodeUtils::comparePosition(mElements[mDirection == kForward ? mElementCount - 1 : 0],
                                          aNodes.mElements[aNodes.mDirection == kForward ? 0 : aNodes.mElementCount - 1]) < 0) {

        PRInt32 insertPos;
        if (mDirection == kForward) {
            insertPos = mElementCount;
        }
        else {
            insertPos = 0;
            if (mElementCount > 0) {
                memmove(&mElements[aNodes.mElementCount], mElements,
                        mElementCount * sizeof(txXPathNode));
            }
        }

        if (mDirection == aNodes.mDirection) {
            aTransfer(this, aNodes, 0, aNodes.mElementCount, insertPos);
        }
        else {
            aTransfer(this, aNodes, aNodes.mElementCount - 1, -1, insertPos);
        }

        mElementCount += aNodes.mElementCount;

        return NS_OK;
    }

    prepareForModification();

    // Index this nodeset (start at last)
    PRInt32 thisPos = mElementCount - 1;

    // Index and limit of index in other nodeset (dependent on direction)
    PRInt32 otherPos, otherEnd;
    if (aNodes.mDirection == kForward) {
        otherPos = aNodes.mElementCount - 1;
        otherEnd = -1;
    }
    else {
        otherPos = 0;
        otherEnd = aNodes.mElementCount;
    }

    // Index in result where last insert was done.
    PRInt32 lastInsertPos = mElementCount + aNodes.mElementCount;

    PRBool nonDup;
    PRInt32 pos;
    while (thisPos >= 0 && otherPos != otherEnd) {
        if (thisPos > PR_ABS(otherEnd - otherPos) - 1) {
            // Find where in the remaining nodes in this nodeset a node from
            // the other nodeset should be inserted.
            pos = findPosition(aNodes.mElements[otherPos], 0, thisPos, nonDup);
NS_ASSERTION(pos >= -1 && pos <= thisPos + 1, "Bad pos");

            // Move nodes in this nodeset
            lastInsertPos -= thisPos - pos + 1;
            memmove(&mElements[lastInsertPos], &mElements[pos],
                    (thisPos - pos + 1) * sizeof(txXPathNode));

            // Copy node from the other nodeset unless it's a dup
            if (nonDup) {
                aTransfer(this, aNodes, otherPos,
                          otherPos + aNodes.mDirection, --lastInsertPos);
            }

            // Adjust positions in both nodesets
            thisPos = pos - 1;
            otherPos -= aNodes.mDirection;
        }
        else {
            // Find where in the remaining nodes in the other nodeset a node
            // from this nodeset should be inserted
            pos = aNodes.findPosition(mElements[thisPos],
                                      otherEnd + aNodes.mDirection,
                                      otherPos, nonDup);

            // Copy nodes from other nodeset to this
            if ((otherPos - pos) * aNodes.mDirection >= 0) {
                lastInsertPos -= ((otherPos - pos) * aNodes.mDirection) + 1;
                aTransfer(this, aNodes, pos, otherPos + aNodes.mDirection,
                          lastInsertPos);
            }

            // Move node in this nodeset unless it's a dup
            if (nonDup) {
                memmove(&mElements[--lastInsertPos], &mElements[thisPos],
                        sizeof(txXPathNode));
            }
            
            // Adjust positions in both nodesets
            otherPos = pos - aNodes.mDirection;
            --thisPos;
        }
    }
    
    if (thisPos >= 0) {
        // There were some elements still left in this nodeset that need to
        // be moved
        lastInsertPos -= thisPos + 1;
        memmove(&mElements[lastInsertPos], mElements,
                (thisPos + 1) * sizeof(txXPathNode));
    }
    else if (otherPos != otherEnd) {
        // There were some elements still left in the other nodeset that need
        // to be copied
        lastInsertPos -= PR_ABS(otherPos - otherEnd);
        aTransfer(this, aNodes, otherPos, otherEnd,
                  lastInsertPos);
    }

    // if lastInsertPos != 0 then we have found some duplicates causing the
    // first element to not be placed at mElements[0]
    mElementCount += aNodes.mElementCount - lastInsertPos;
    if (lastInsertPos > 0) {
        memmove(mElements, &mElements[lastInsertPos],
                mElementCount * sizeof(txXPathNode));
    }

    return NS_OK;
}

/**
 * Append API
 * These functions should be used with care.
 * They are intended to be used when the caller assures that the resulting
 * nodeset remains in document order.
 * Abuse will break document order, and cause errors in the result.
 * These functions are significantly faster than the add API, as no
 * Node::OrderInfo structs will be generated.
 */

nsresult
txNodeSet::append(const txXPathNode& aNode)
{
    if (!ensureSize(mElementCount + 1)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    prepareForModification();

    txXPathNode* temp = new(&mElements[mElementCount]) txXPathNode(aNode);
    NS_ENSURE_TRUE(temp, NS_ERROR_FAILURE);

    ++mElementCount;

    return NS_OK;
}

nsresult
txNodeSet::append(const txNodeSet& aNodes)
{
    if (aNodes.mElementCount <= 0) {
        return NS_OK;
    }

    if (!ensureSize(mElementCount + aNodes.mElementCount)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    PRInt32 insertPos;
    if (mDirection == kForward) {
        insertPos = mElementCount;
    }
    else {
        insertPos = 0;
        if (mElementCount > 0) {
            memmove(&mElements[aNodes.mElementCount], mElements,
                    mElementCount * sizeof(txXPathNode));
        }
    }

    if (mDirection == aNodes.mDirection) {
        copyElements(this, aNodes, 0, aNodes.mElementCount, insertPos);
    }
    else {
        copyElements(this, aNodes, aNodes.mElementCount - 1, -1, insertPos);
    }

    mElementCount += aNodes.mElementCount;

    return NS_OK;
}

void
txNodeSet::reverse()
{
    if (mElementCount > 1) {
        mDirection = (mDirection == kForward ? kReversed : kForward);
    }
}

void
txNodeSet::clear()
{
    if (mElements) {
        while (--mElementCount >= 0) {
            mElements[mElementCount].~txXPathNode();
        }
        nsMemory::Free(mElements);
        mElements = nsnull;
        mBufferSize = 0;
        mElementCount = 0;
    }
    mDirection = kForward;
}

void
txNodeSet::prepareForModification()
{
    if (mDirection == kForward || mElementCount <= 0) {
        return;
    }

    PRInt32 counter;
    void* tmp = nsMemory::Alloc(sizeof(txXPathNode));
    for (counter = 0; counter < mElementCount / 2; ++counter) {
        memcpy(tmp, &mElements[counter], sizeof(txXPathNode));
        memcpy(&mElements[counter], &mElements[mElementCount - 1 - counter], sizeof(txXPathNode));
        memcpy(&mElements[mElementCount - 1 - counter], tmp, sizeof(txXPathNode));
    }
    nsMemory::Free(tmp);

    mDirection = kForward;
}

PRInt32
txNodeSet::indexOf(const txXPathNode& aNode) const
{
    if (mElementCount <= 0) {
        return -1;
    }

    PRInt32 counter;
    for (counter = 0; counter < mElementCount; ++counter) {
        if (mElements[counter] == aNode) {
            return mDirection == kForward ? counter : mElementCount - 1 - counter;
        }
    }

    return -1;
}

const txXPathNode&
txNodeSet::get(PRInt32 aIndex) const
{
    PRInt32 index = mDirection == kForward ? aIndex
                                           : mElementCount - 1 - aIndex;
    NS_ASSERTION(mElementCount > 0 && index >= 0 && index < mElementCount,
                 "Bad index!");

    return mElements[index];
}

short
txNodeSet::getResultType()
{
    return txAExprResult::NODESET;
}

PRBool
txNodeSet::booleanValue()
{
    return (mElementCount > 0);
}
double
txNodeSet::numberValue()
{
    nsAutoString str;
    stringValue(str);

    return Double::toDouble(str);
}

void
txNodeSet::stringValue(nsAString& aStr)
{
    if (mElementCount > 0) {
        txXPathNodeUtils::getNodeValue(mElements[mDirection == kForward ? 0 : mElementCount - 1], aStr);
    }
}

nsAString*
txNodeSet::stringValuePointer()
{
    return nsnull;
}

PRBool txNodeSet::ensureSize(PRInt32 aSize)
{
    if (aSize <= mBufferSize) {
        return PR_TRUE;
    }

    // This isn't 100% safe. But until someone manages to make a 1gig nodeset
    // it should be ok.
    PRInt32 newSize = mBufferSize > kTxNodeSetMinSize ? mBufferSize : kTxNodeSetMinSize;
    while (newSize < aSize) {
        newSize *= kTxNodeSetGrowFactor;
    }

    txXPathNode* newArr = NS_STATIC_CAST(txXPathNode*,
                                         nsMemory::Alloc(newSize *
                                                         sizeof(txXPathNode)));
    if (!newArr) {
        return PR_FALSE;
    }
    
    if (mElementCount > 0) {
        memcpy(newArr, mElements, mElementCount * sizeof(txXPathNode));
    }

    if (mElements) {
#ifdef DEBUG
        memset(mElements, 0, mBufferSize * sizeof(txXPathNode));
#endif
        nsMemory::Free(mElements);
    }
    mElements = newArr;
    mBufferSize = newSize;

    return PR_TRUE;
}

PRInt32 txNodeSet::findPosition(const txXPathNode& aNode, PRInt32 aFirst,
                                PRInt32 aLast, PRBool& aNonDup) const
{
    if ((aLast - aFirst) * mDirection <= 1) {
        // If we search 2 nodes or less there is no point in further divides
        PRInt32 pos;
        for (pos = aFirst; pos != aLast + mDirection; pos += mDirection) {
            PRIntn cmp = txXPathNodeUtils::comparePosition(aNode, mElements[pos]);
            if (cmp < 0) {
                aNonDup = PR_TRUE;

                return pos;
            }

            if (cmp == 0) {
                aNonDup = PR_FALSE;

                return pos;
            }
        }

        aNonDup = PR_TRUE;

        return pos;
    }

    PRInt32 midpos = (aFirst + aLast) / 2;
    PRIntn cmp = txXPathNodeUtils::comparePosition(aNode, mElements[midpos]);
    if (cmp == 0) {
        aNonDup = PR_FALSE;

        return midpos;
    }

    if (cmp > 0) {
        return findPosition(aNode, midpos + mDirection, aLast, aNonDup);
    }

    return findPosition(aNode, aFirst, midpos - mDirection, aNonDup);
}

/* static */
void txNodeSet::copyElements(txNodeSet* aNodes, const txNodeSet& aOtherNodes,
                             PRInt32 aStart, PRInt32 aEnd, PRInt32 aIndex)
{
    PRInt32 increment = aNodes->mDirection == aOtherNodes.mDirection ? 1 : -1;
    while (aStart != aEnd) {
        txXPathNode* temp = new(&aNodes->mElements[aIndex]) txXPathNode(aOtherNodes.mElements[aStart]);
        if (!temp) {
            break;
        }

        aStart += increment;
        ++aIndex;
    }
}

/* static */
void txNodeSet::transferElements(txNodeSet* aNodes,
                                 const txNodeSet& aOtherNodes, PRInt32 aStart,
                                 PRInt32 aEnd, PRInt32 aIndex)
{
    if (aNodes->mDirection == aOtherNodes.mDirection) {
        PRInt32 start = PR_MIN(aStart, aEnd + aOtherNodes.mDirection);
        memcpy(&aNodes->mElements[aIndex], &aOtherNodes.mElements[start],
               PR_ABS(aEnd - aStart) * sizeof(txXPathNode));
    }
    else {
        while (aStart != aEnd) {
            memcpy(&aNodes->mElements[aIndex], &aOtherNodes.mElements[aStart],
                   sizeof(txXPathNode));
            --aStart;
            ++aIndex;
        }
    }
}
