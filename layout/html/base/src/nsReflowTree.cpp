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

#include "nsReflowTree.h"
#include "nsIFrame.h"
#include "nsHTMLReflowCommand.h"
#include "pldhash.h"

// binary and semantically compatible with PLDHashEntryStub
struct FrameEntry : PLDHashEntryHdr
{
    nsIFrame *frame;
};

nsReflowTree::nsReflowTree() : mRoot(0)
{
    /* XXX fallible */
    PL_DHashTableInit(&mTargettedFrames, PL_DHashGetStubOps(), 0,
                      sizeof (FrameEntry), 64 /* SWAG, as always */);
}

nsReflowTree::~nsReflowTree()
{
    PL_DHashTableFinish(&mTargettedFrames);
    delete mRoot;
}

/* static */ nsReflowTree::Node *
nsReflowTree::Node::Create(nsIFrame *forFrame)
{
    Node *node = new Node();
    node->mFrame = forFrame;
    node->mFlags = 0;
    node->mKidU.mChild = 0;
    node->mTargetCount = 0;
    return node;
}

/* static */ void
nsReflowTree::Node::Destroy(nsReflowTree::Node *node)
{
    delete node;
}

nsReflowTree::Node::~Node()
{
    if (HasSingleChild())
        Destroy(mKidU.mChild);
    else
        ChildChunk::Destroy(mKidU.mChunk);
}

nsReflowTree::Node *
nsReflowTree::Node::GetChild(nsIFrame *forFrame)
{
    if (!mKidU.mChild)
        // no kids yet, this becomes one
        return mKidU.mChild = Create(forFrame);
  
    if (HasSingleChild()) {
        // our child is this frame, all done
        if (forFrame == mKidU.mChild->GetFrame())
            return mKidU.mChild;
        
        // make a chunk for the existing kid...
        mKidU.mChunk = ChildChunk::Create(mKidU.mChild);
        if (!mKidU.mChunk)
            return nsnull;
        mFlags |= KIDS_CHUNKED;
        
        // ...and add a new one
        return mKidU.mChunk->mKids[1] = Create(forFrame);
    }
    
    return mKidU.mChunk->GetChild(forFrame);
}

/* static */ nsReflowTree::Node::ChildChunk *
nsReflowTree::Node::ChildChunk::Create(nsReflowTree::Node *node)
{
    ChildChunk *chunk = new ChildChunk();
    memset(&chunk->mKids[1], 0, sizeof(chunk->mKids)-sizeof(chunk->mKids[0]));
    chunk->mKids[0] = node;
    chunk->mNext = 0;
    
    return chunk;
}

/* static */ void
nsReflowTree::Node::ChildChunk::Destroy(nsReflowTree::Node::ChildChunk *chunk)
{
    delete chunk;
}

nsReflowTree::Node::ChildChunk::~ChildChunk()
{
    for (int i = 0; i < KIDS_CHUNK_SIZE; i++) {
        if (!mKids[i])
            break;
        delete mKids[i];
    }
  
    Destroy(mNext);
}

nsReflowTree::Node *
nsReflowTree::Node::ChildChunk::GetChild(nsIFrame *forFrame)
{
    Node *n;
    int i = 0;
    
    for (; i < KIDS_CHUNK_SIZE && (n = mKids[i]); i++) {
        if (forFrame == n->GetFrame())
            return n;
    }
    
    if (i < KIDS_CHUNK_SIZE)
        // didn't find it, and have space here
        return mKids[i] = Node::Create(forFrame);
    
    // ask the next guy, if there is one
    if (mNext)
        return mNext->GetChild(forFrame);
    
    // if we're full but have no next chunk, make one for the new child
    n = Node::Create(forFrame);
    mNext = Create(n);
    
    if (!mNext) {
        Node::Destroy(n);
        return nsnull;
    }
    
    return n;
}

nsReflowTree::Node *
nsReflowTree::MergeCommand(nsHTMLReflowCommand *command)
{
    nsIFrame *frame;
    if (NS_FAILED(command->GetTarget(/* ref */ frame)) || !frame)
        return nsnull;

    if (mRoot) {
        // XXX check that reflow types and other bits match
    }
    
    Node *n = AddToTree(frame);
    if (n) {
        NS_ASSERTION(n->GetFrame() == frame, "node/frame mismatch");
        n->MakeTarget();
        AddTargettedFrame(frame);
    }
    return n;
}

nsReflowTree::Node *
nsReflowTree::AddToTree(nsIFrame *frame)
{
    nsIFrame *parent;
    frame->GetParent(&parent);

    if (!parent) {
        // no root so far, this is the one.
        if (!mRoot)
            return mRoot = Node::Create(frame);

        // we're at the root, start the merging unwind
        if (frame == mRoot->GetFrame())
            return mRoot;
        
        // Can't merge.
        NS_ASSERTION(frame == mRoot->GetFrame(), "mismatched roots in reflow!");
        return nsnull;
    }
    
    nsReflowTree::Node *parentNode =  AddToTree(parent);
    if (!parentNode)
        return nsnull;

    return parentNode->GetChild(frame);
}

nsReflowTree::Node *
nsReflowTree::Node::Iterator::NextChild()
{
    if (!mNode)
        return nsnull;

    if (!mPos) {
        if (mNode->HasSingleChild()) {
            mPos = &mNode->mKidU.mChild;
        } else {
            mCurrentChunk = mNode->mKidU.mChunk;
            mPos = &mCurrentChunk->mKids[0];
        }
        return *mPos;
    }

    if (mNode->HasSingleChild())
        return nsnull;

    if (!*mPos)
        return nsnull;


    if (mPos < &mCurrentChunk->mKids[ChildChunk::KIDS_CHUNK_SIZE - 1]) {
        mPos++;
    } else {
        mCurrentChunk = mCurrentChunk->mNext;
        if (!mCurrentChunk) {
            return mNode = nsnull; // terminates iteration.
        } else {
            mPos = &mCurrentChunk->mKids[0];
        }
    }

    return *mPos;
}

nsReflowTree::Node *
nsReflowTree::Node::Iterator::NextChild(nsIFrame **aChildIFrame)
{
  nsReflowTree::Node *result = NextChild();
  *aChildIFrame = (mPos && *mPos) ?
      NS_STATIC_CAST(nsReflowTree::Node*, *mPos)->GetFrame() : nsnull;

#ifdef DEBUG
  fprintf(stderr,"NextChild() = %p\n",result ? result->mFrame : nsnull);
#endif
  return result;
}

nsReflowTree::Node *
nsReflowTree::Node::Iterator::SelectChild(nsIFrame *aChildIFrame)
{
    Node **aPos;
    ChildChunk *aCurrentChunk;

    if (!mNode) {
#ifdef DEBUG
        fprintf(stderr,"SelectChild() = %p\n",nsnull);
#endif
        return nsnull;
    }
    
    if (mNode->HasSingleChild()) {
        aPos = &mNode->mKidU.mChild;
    } else {
        aCurrentChunk = mNode->mKidU.mChunk;
        aPos = &aCurrentChunk->mKids[0];
    }
    if (*aPos && (*aPos)->mFrame == aChildIFrame)
    {
#ifdef DEBUG
        fprintf(stderr,"SelectChild() = %p\n",(*aPos)->mFrame);
#endif
        return *aPos;
    }

    while (aPos && *aPos) {
        if (aPos < &aCurrentChunk->mKids[ChildChunk::KIDS_CHUNK_SIZE]) {
            aPos++;
        } else {
            aCurrentChunk = aCurrentChunk->mNext;
            if (!aCurrentChunk) {
#ifdef DEBUG
                fprintf(stderr,"SelectChild() = %p\n",nsnull);
#endif
                return nsnull;
            } else {
                aPos = &mCurrentChunk->mKids[0];
            }
        }
        if ((*aPos)->mFrame == aChildIFrame) {
#ifdef DEBUG
            fprintf(stderr,"SelectChild() = %p\n",(*aPos)->mFrame);
#endif
            return *aPos;
        }
    }

#ifdef DEBUG
    fprintf(stderr,"SelectChild() = %p\n",nsnull);
#endif
    return nsnull;
}

#ifdef DEBUG
void
nsReflowTree::Node::Iterator::AssertFrame(nsIFrame *aIFrame)
{
    if (mNode) {
        NS_ASSERTION(mNode->mFrame == aIFrame,"Reflow tree out of sync!");
    }
}
#endif

PRBool
nsReflowTree::AddTargettedFrame(nsIFrame *frame)
{
    FrameEntry *entry =
        NS_STATIC_CAST(FrameEntry *,
                       PL_DHashTableOperate(&mTargettedFrames, frame,
                                            PL_DHASH_ADD));
    if (!entry)
        return PR_FALSE;
    entry->frame = frame;
    return PR_TRUE;
}

PRBool
nsReflowTree::FrameIsTarget(const nsIFrame *frame)
{
    FrameEntry *entry = 
        NS_STATIC_CAST(FrameEntry *, 
                       PL_DHashTableOperate(&mTargettedFrames, frame,
                                            PL_DHASH_LOOKUP));
    return entry && PL_DHASH_ENTRY_IS_BUSY(entry);
}

void
nsReflowTree::Node::Dump(int depth)
{
    fprintf(stderr, "%*s|-- %p", depth, "", (void *)mFrame);
    if (IsTarget())
        fprintf(stderr, " (T: %d)", mTargetCount);

    nsCOMPtr<nsIAtom> typeAtom;
    mFrame->GetFrameType(getter_AddRefs(typeAtom));

    if (typeAtom) {
        const PRUnichar *unibuf;
        typeAtom->GetUnicode(&unibuf);
        fprintf(stderr, " [%s]\n", NS_LossyConvertUCS2toASCII(unibuf).get());
    } else {
        putc('\n', stderr);
    }

    Iterator iter(this);
    Node *child;
    while ((child = iter.NextChild()))
        child->Dump(depth + 2);
}

void
nsReflowTree::Dump()
{
    fprintf(stderr, "Tree dump:\n");
    mRoot->Dump(0);
}
