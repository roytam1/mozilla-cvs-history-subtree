#include "nsReflowTree.h"
#include "nsIFrame.h"

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
nsReflowTree::AddToTree(nsIFrame *frame)
{
    nsIFrame *parent;
    frame->GetParent(&parent);

    if (!parent) {
        // we're at the root, start merging if we can
        if (frame == mRoot->GetFrame())
            return mRoot;
        
        // Can't merge.
        return nsnull;
    }
    
    nsReflowTree::Node *parentNode =  AddToTree(parent);
    if (!parentNode)
        return nsnull;

    return parentNode->AddChild(frame);
}

