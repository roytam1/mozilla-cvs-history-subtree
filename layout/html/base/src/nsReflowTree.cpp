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

#include <stdio.h>
#include "pldhash.h"
#include "prenv.h"
#include "nsReflowTree.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsHTMLReflowCommand.h"

#ifdef NS_DEBUG
#define ITERATOR_FRAME_TRACE(_bit,_args)                              \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsIFrameDebug::GetLogModuleInfo(),_bit)) { \
      if (mNode && mNode->GetFrame()) \
         (NS_REINTERPRET_CAST(nsFrame*,mNode->GetFrame()))->TraceMsg _args ;  \
    }                                                           \
  PR_END_MACRO

#define NODE_FRAME_TRACE(_bit,_args)                              \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsIFrameDebug::GetLogModuleInfo(),_bit)) { \
      if (mFrame) \
         (NS_REINTERPRET_CAST(nsFrame*,mFrame))->TraceMsg _args ;  \
    }                                                           \
  PR_END_MACRO

#define TREE_FRAME_TRACE(_bit,_args)                              \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsIFrameDebug::GetLogModuleInfo(),_bit)) { \
      if (mRoot && mRoot->GetFrame()) \
         (NS_REINTERPRET_CAST(nsFrame*,mRoot->GetFrame()))->TraceMsg _args ;  \
    }                                                           \
  PR_END_MACRO

#define TREE_FRAME_DUMP(_bit,_cmd)                              \
  PR_BEGIN_MACRO                                                \
    if (NS_FRAME_LOG_TEST(nsIFrameDebug::GetLogModuleInfo(),_bit)) { \
       _cmd; \
    }                                                           \
  PR_END_MACRO

#else
#define ITERATOR_FRAME_TRACE(_bit,_args)
#define NODE_FRAME_TRACE(_bit,_args)
#define TREE_FRAME_TRACE(_bit,_args)
#define TREE_FRAME_DUMP(_bit,_cmd)
#endif

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
    if (HasMultipleChildren())
        ChildChunk::Destroy(mKidU.mChunk);
    else if (HasChildren())
        Destroy(mKidU.mChild);
}

nsReflowTree::Node *
nsReflowTree::Node::GetChild(nsIFrame *forFrame)
{
    if (!HasChildren())
        // no kids yet, this becomes one
        return mKidU.mChild = Create(forFrame);
  
    if (!HasMultipleChildren()) {
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
        if (!mNode->HasMultipleChildren()) {
            // if it's 0 children, *mPos == nsnull
            mPos = &mNode->mKidU.mChild;
        } else {
            mCurrentChunk = mNode->mKidU.mChunk;
            mPos = &mCurrentChunk->mKids[0];
        }
        return *mPos;
    }

    if (!mNode->HasMultipleChildren())
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

  ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                 ("NextChild() = %p",result ? result->mFrame : nsnull));
  return result;
}

nsReflowTree::Node *
nsReflowTree::Node::Iterator::SelectChild(nsIFrame *aChildIFrame)
{
    Node **aPos;
    ChildChunk *aCurrentChunk;

    if (!mNode || !mNode->HasChildren()) {
        ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                       ("SelectChild() = %p",nsnull));
        return nsnull;
    }
    
    if (!mNode->HasMultipleChildren()) {
        if (mNode->mKidU.mChild->mFrame == aChildIFrame) {
            ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                     ("SelectChild() = %p",mNode->mKidU.mChild->mFrame));
            return mNode->mKidU.mChild;
        }
        ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                             ("SelectChild() = %p",nsnull));
        return nsnull;
    } else {
        aCurrentChunk = mNode->mKidU.mChunk;
        aPos = &aCurrentChunk->mKids[0];
    }
    if (*aPos && (*aPos)->mFrame == aChildIFrame)
    {
        ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                       ("SelectChild() = %p",(*aPos)->mFrame));
        return *aPos;
    }

    while (aPos && *aPos) {
        if (aPos < &aCurrentChunk->mKids[ChildChunk::KIDS_CHUNK_SIZE]) {
            aPos++;
        } else {
            aCurrentChunk = aCurrentChunk->mNext;
            if (!aCurrentChunk) {
                ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                               ("SelectChild() = %p",nsnull));
                return nsnull;
            } else {
                aPos = &mCurrentChunk->mKids[0];
            }
        }
        if (*aPos && (*aPos)->mFrame == aChildIFrame) {
            ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                           ("SelectChild() = %p",(*aPos)->mFrame));
            return *aPos;
        }
    }

    ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                   ("SelectChild() = %p",nsnull));
    return nsnull;
}

#ifdef NS_DEBUG
void
nsReflowTree::Node::Iterator::AssertFrame(const nsIFrame *aIFrame)
{
    if (mNode) {
        NS_ASSERTION(mNode->mFrame == aIFrame,"Reflow tree out of sync!");
    }
}

PRBool
nsReflowTree::Node::Iterator::IsTarget()
{
    ITERATOR_FRAME_TRACE(NS_FRAME_TRACE_TREE,
                   ("IsTarget(%p) = %d\n",
                    mNode ? mNode->mFrame : nsnull,
                    mNode ? mNode->IsTarget() : PR_FALSE));
    return RealIsTarget();
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
nsReflowTree::Node::Dump(FILE *f, int depth)
{
    char buf[200];
    char *bufptr;

    sprintf(buf, "%*s|-- %p", depth, "", (void *)mFrame);
    bufptr = buf + strlen(buf);
    if (IsTarget())
    {
        sprintf(bufptr, " (%d)", mTargetCount);
        bufptr += strlen(bufptr);
    }

    nsCOMPtr<nsIAtom> typeAtom;
    mFrame->GetFrameType(getter_AddRefs(typeAtom));

    if (typeAtom) {
        const PRUnichar *unibuf;
        typeAtom->GetUnicode(&unibuf);
        sprintf(bufptr, " [%s]", NS_LossyConvertUCS2toASCII(unibuf).get());
        bufptr += strlen(bufptr);
    } else {
        sprintf(bufptr," [unknown] ");
        bufptr += strlen(bufptr);
    }

    nsFrameState state;
    mFrame->GetFrameState(&state);
    if (state & NS_FRAME_IS_DIRTY)
    {
        *bufptr++ = 'D';
        *bufptr = '\0';
    }
    if (state & NS_FRAME_HAS_DIRTY_CHILDREN)
    {
        *bufptr++ = 'C';
        *bufptr = '\0';
    }
    if (f)
        fprintf(f,"%s\n",buf);
    NODE_FRAME_TRACE(NS_FRAME_TRACE_TREE,("%s",buf));

    Iterator iter(this);
    Node *child;
    while ((child = iter.NextChild()))
        child->Dump(f, depth + 2);
}

void
nsReflowTree::Dump()
{
    TREE_FRAME_TRACE(NS_FRAME_TRACE_TREE,( "Reflow tree dump:"));
    static FILE *f;
    static PRBool haveCheckedEnv;
    if (!haveCheckedEnv) {
        char *file = PR_GetEnv("MOZILLA_REFLOW_TREE_LOG");
        if (file) {
            if (!strcmp(file, "stderr"))
                f = stderr;
            else if (!strcmp(file, "stdout"))
                f = stdout;
            else
                f = fopen(file, "w");
        }
        haveCheckedEnv = PR_TRUE;
    }
    if (f) {
        fputs("Reflow tree dump:\n", f);
        mRoot->Dump(f, 0);
    }
    else {
        TREE_FRAME_DUMP(NS_FRAME_TRACE_TREE,mRoot->Dump(f,0));
    }
}
