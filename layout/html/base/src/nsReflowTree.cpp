#include "nsReflowTree.h"
#include "nsIFrame.h"
#include "nsHTMLReflowCommand.h"
#include "pldhash.h"

struct TargettedFrameEntry : public PLDHashEntryStub
{
    const nsReflowTree::Node *node;
};

nsReflowTree::nsReflowTree() : mRoot(0)
{
#if 0
    /* XXX fallible */
    PL_DHashTableInit(&mTargettedFrames, PL_DHashGetStubOps(), 0,
                      sizeof TargettedFrameEntry,
                      45 /* SWAG, but aren't these all? */);
#endif
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
    
    return AddToTree(frame);
}

nsReflowTree::~nsReflowTree()
{
    delete mRoot;
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


    if (mPos < &mCurrentChunk->mKids[ChildChunk::KIDS_CHUNK_SIZE]) {
        mPos++;
    } else {
        mCurrentChunk = mCurrentChunk->mNext;
        if (!mCurrentChunk) {
            mPos = NS_REINTERPRET_CAST(Node **, &mCurrentChunk);
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
    NS_STATIC_CAST(nsReflowTree::Node*,*mPos)->GetFrame() : nsnull;
  return result;
}

nsReflowTree::Node *
nsReflowTree::Node::Iterator::SelectChild(nsIFrame *aChildIFrame)
{
    Node **aPos;
    ChildChunk *aCurrentChunk;

    if (!mNode)
      return nsnull;

    if (mNode->HasSingleChild()) {
      aPos = &mNode->mKidU.mChild;
    } else {
      aCurrentChunk = mNode->mKidU.mChunk;
      aPos = &aCurrentChunk->mKids[0];
    }
    if (*aPos && (*aPos)->mFrame == aChildIFrame)
      return *aPos;

    while (aPos && *aPos)
    {
      if (aPos < &aCurrentChunk->mKids[ChildChunk::KIDS_CHUNK_SIZE]) {
        aPos++;
      } else {
        aCurrentChunk = aCurrentChunk->mNext;
        if (!aCurrentChunk) {
            aPos = NS_REINTERPRET_CAST(Node **, &mCurrentChunk);
        } else {
            aPos = &mCurrentChunk->mKids[0];
        }
      }
      if ((*aPos)->mFrame == aChildIFrame)
	return *aPos;
    }

    return nsnull;
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
