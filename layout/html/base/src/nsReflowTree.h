////// Iteration:
// nsReflowTree::Iterator iter;
// nsReflowTree::Node *node, *current = GetCurrentTarget();
// while (node = current->NextChild(iter)) {
//    DoIt();
// }

#include "nscore.h"

class nsIFrame;
class nsHTMLReflowCommand;

class nsReflowTree
{
public:
    nsReflowTree() : mRoot(0) { }
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
            static const int KIDS_CHUNK_SIZE = 5;

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
            Iterator(Node *node) : mNode(node), mPos(0) { }
            ~Iterator() { }
            Node *NextChild();
            Node *CurrentChild() { return mPos ? *mPos : nsnull; }
        private:
            Node *mNode;
            Node **mPos;
            ChildChunk *mCurrentChunk;
            friend class nsReflowTree;
        };

        void MakeTarget(PRBool isTarget = PR_TRUE);
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

        static const int KIDS_CHUNKED = 1;
        static const int NODE_IS_TARGET = 2;

        bool HasSingleChild() { return !(mFlags & KIDS_CHUNKED); }
        
        friend class Iterator;
        friend class nsReflowTree;
    };
        
    Node *MergeCommand(nsHTMLReflowCommand *command);

    void Dump();
    
private:
    Node *AddToTree(nsIFrame *frame);
    Node *mRoot;
    // Other reflow-state stuff here
};

inline void
nsReflowTree::Node::MakeTarget(PRBool isTarget)
{
    if (isTarget)
        mFlags |= NODE_IS_TARGET;
    else
        mFlags &= NODE_IS_TARGET;
}
