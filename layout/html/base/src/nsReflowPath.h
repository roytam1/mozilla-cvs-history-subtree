/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *    Mike Shaver <shaver@mozilla.org>
 *    Randell Jesup <rjesup@wgate.com>
 *    Chris Waterson <waterson@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsReflowPath_h__
#define nsReflowPath_h__

#include "nscore.h"
#include "pldhash.h"
#include "nsReflowType.h"
#include "nsVoidArray.h"

#ifdef DEBUG
#include <stdio.h>
#endif

class nsIFrame;
class nsHTMLReflowCommand;
class nsIPresContext;

class nsReflowPath
{
public:
    nsReflowPath(nsIFrame *aFrame)
        : mFrame(aFrame),
          mReflowCommand(nsnull) {}

    ~nsReflowPath();

    // Find or create a child of this node corresponding to forFrame.
    // XXX better name
    nsReflowPath *EnsureChild(nsIFrame *aFrame);

    class iterator
    {
    protected:
        nsReflowPath *mNode;
        PRInt32 mIndex;

        friend class nsReflowPath;

        iterator(nsReflowPath *aNode, PRInt32 aIndex)
            : mNode(aNode), mIndex(aIndex) {}

        void
        Advance() { --mIndex; }

    public:
        iterator()
            : mNode(nsnull) {}

        iterator(const iterator &iter)
            : mNode(iter.mNode), mIndex(iter.mIndex) {}

        iterator &
        operator=(const iterator &iter) {
            mNode = iter.mNode;
            mIndex = iter.mIndex;
            return *this; }

        nsReflowPath *
        get() const {
            return NS_STATIC_CAST(nsReflowPath *, mNode->mChildren[mIndex]); }

        nsReflowPath *
        get() {
            return NS_STATIC_CAST(nsReflowPath *, mNode->mChildren[mIndex]); }

        nsIFrame *
        operator*() const {
            return get()->mFrame; }

        nsIFrame *&
        operator*() {
            return get()->mFrame; }

        iterator &
        operator++() { Advance(); return *this; }

        iterator
        operator++(int) {
            iterator temp(*this);
            Advance();
            return temp; }

        PRBool
        operator==(const iterator &iter) const {
            return (mNode == iter.mNode) && (mIndex == iter.mIndex); }

        PRBool
        operator!=(const iterator &iter) const {
            return !iter.operator==(*this); }
    };

    iterator FirstChild() { return iterator(this, mChildren.Count() - 1); }
    iterator EndChildren() { return iterator(this, -1); }

    PRBool
    HasChild(nsIFrame *aFrame) const {
        return GetSubtreeFor(aFrame) != nsnull; }

    iterator
    FindChild(nsIFrame *aFrame);

    void
    RemoveChild(nsIFrame *aFrame) { 
        iterator iter = FindChild(aFrame);
        Remove(iter); }

    nsReflowPath *
    GetSubtreeFor(nsIFrame *aFrame) const;

    nsReflowPath *
    EnsureSubtreeFor(nsIFrame *aFrame);

    void
    Remove(iterator &aIterator) {
        NS_ASSERTION(aIterator.mNode == this, "inconsistent iterator");
        mChildren.RemoveElementAt(aIterator.mIndex); }

#ifdef DEBUG
    void
    Dump(nsIPresContext *aPresContext, FILE *aFile, int aDepth);
#endif

    nsIFrame            *mFrame;
    nsHTMLReflowCommand *mReflowCommand;

protected:
    nsSmallVoidArray     mChildren;

    friend class iterator;
};

#endif
