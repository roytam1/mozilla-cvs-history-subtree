#ifndef NSREFLOWTREE_H
#define NSREFLOWTREE_H
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

class nsIFrame;
class nsHTMLReflowCommand;

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

        class Iterator;

        class ChildChunk {
            friend class nsReflowTree::Node;
            friend class nsReflowTree::Node::Iterator;
            static const int KIDS_CHUNK_SIZE = 110;

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
            Node *NextChild();
            Node *NextChild(nsIFrame **aChildIFrame);
	    // Set the mPos to the node with this nsIFrame, or null
            Node *SelectChild(nsIFrame *aChildIFrame);
            inline Node *CurrentChild() { return mPos ? *mPos : nsnull; }
            inline Node *CurrentNode()  { return mNode; }
	    inline PRBool IsTarget()
	      { return mNode ? mNode->IsTarget() : PR_FALSE; }
        private:
            Node *mNode;
            Node **mPos;
            ChildChunk *mCurrentChunk;
            friend class nsReflowTree;
        };

        void MakeTarget();
        PRBool IsTarget() { return mFlags & NODE_IS_TARGET; }
        void Dump(int depth);
                
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
        
        friend class Iterator;
        friend class nsReflowTree;
    };
        
    Node *MergeCommand(nsHTMLReflowCommand *command);

    void Dump();

    Node *Root() { return mRoot; }

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
#endif

