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

#ifndef nsReflowTree_h__
#define nsReflowTree_h__
////// Iteration:
// nsReflowTree::Iterator iter(reflowcmd->GetCurrentTarget());
// PRBool amTarget = reflowIterator.IsTarget();
// if (amTarget) {
//    reflowcmd->SetCurrentReflowNode(nsnull);
//    DoIt(me);
// }
// while (iter.NextChild(&childIFrame)) {
//    reflowcmd->SetCurrentReflowNode(iter.CurrentChild());
//    DoIt(childIFrame);
// }

#include "nscore.h"
#include "pldhash.h"
#include <stdio.h>

class nsIFrame;
class nsHTMLReflowCommand;

#define NS_FRAME_TRACE_TREE 0x10

class nsReflowTree
{
public:
    nsReflowTree();
    ~nsReflowTree();

    class Node
    {
    public:
        static Node *Create(nsIFrame *frame);
        static void Destroy(Node *node);

        // Find or create a child of this node corresponding to forFrame.
        // XXX better name
        Node *GetChild(nsIFrame *forFrame);                

        nsIFrame *GetFrame() { return mFrame; }
        void SetFrame(nsIFrame *aFrame) { mFrame = aFrame; }

        class Iterator;

        class ChildChunk {
            friend class nsReflowTree::Node;
            friend class nsReflowTree::Node::Iterator;
            static const int KIDS_CHUNK_SIZE = 10;

            Node       *mKids[KIDS_CHUNK_SIZE];
            ChildChunk *mNext;

            static ChildChunk *Create(Node *firstChild);
            static void Destroy(ChildChunk *chunk);

            ChildChunk() { };
            ~ChildChunk();

            Node *GetChild(nsIFrame *forFrame);
        };

        class Iterator
        {
            // This is just here for the constructor, really
        public:
            Iterator(Node *node) : mNode(node), mPos(nsnull) { }
            ~Iterator() { }
#ifdef NS_DEBUG
            void AssertFrame(const nsIFrame *aIFrame);
#endif
            Node *NextChild();
            Node *NextChild(nsIFrame **aChildIFrame);
            // Set the mPos to the node with this nsIFrame, or null
            Node *SelectChild(nsIFrame *aChildIFrame);
            Node *CurrentChild() { return mPos ? *mPos : nsnull; }
            Node *CurrentNode()  { return mNode; }
#ifdef NS_DEBUG
            // tricky stuff so we can log IsTarget
            PRBool IsTarget();
            PRBool RealIsTarget()
                {
                    return mNode ? mNode->IsTarget() : PR_FALSE;
                }
#else
            PRBool IsTarget()
                {
                    return mNode ? mNode->IsTarget() : PR_FALSE;
                }
#endif
        private:
            Node *mNode;
            Node **mPos;
            ChildChunk *mCurrentChunk;
            friend class nsReflowTree;
        };

        void MakeTarget();
        PRBool IsTarget() { return mFlags & NODE_IS_TARGET; }
        void Dump(FILE *f, int depth);
                
    private:
        Node() { }
        ~Node();
        
        nsIFrame *mFrame;
        union {
            Node *mChild;
            ChildChunk *mChunk;
        } mKidU;
        PRUint32 mFlags;
        PRUint32 mTargetCount;

        static const int KIDS_CHUNKED = 1;
        static const int NODE_IS_TARGET = 2;

        bool HasSingleChild() { return !(mFlags & KIDS_CHUNKED); }
        
        friend class ChildChunk;
        friend class Iterator;
        friend class nsReflowTree;
    };
        
    Node *MergeCommand(nsHTMLReflowCommand *command);

    void Dump();

    Node *Root() { return mRoot; }
    PRBool AddTargettedFrame(nsIFrame *frame);
    PRBool FrameIsTarget(const nsIFrame *frame);

private:
    Node         *AddToTree(nsIFrame *frame);
    Node         *mRoot;
    PLDHashTable mTargettedFrames;
    // Other reflow-state stuff here
};

inline void
nsReflowTree::Node::MakeTarget()
{
    mFlags |= NODE_IS_TARGET;
    mTargetCount++;
}

#ifdef DEBUG
#define REFLOW_ASSERTFRAME(x) reflowIterator.AssertFrame(NS_REINTERPRET_CAST(const nsIFrame*,(x)))
#else
#define REFLOW_ASSERTFRAME(x)
#endif

#endif
