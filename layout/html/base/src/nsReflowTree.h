////// Iteration:
// nsReflowTree::Iterator iter;
// nsReflowTree::Node *node, *current = GetCurrentTarget();
// while (node = current->NextChild(iter)) {
//    DoIt();
// }

#include "nscore.h"


class nsIFrame;

class nsReflowTree
{
public:
    nsReflowTree();
    ~nsReflowTree();

    class Iterator
    {
        // This is just here for the constructor, really
    public:
        Iterator() : pos(0) { }
    private:
        nsIFrame *&operator *() { return pos; }
        nsIFrame *pos;
        friend class nsReflowTree;
    };

    class Node
    {
    public:
        static Node *Create(nsIFrame *frame);
        static void Destroy(Node *node);
        Node *AddChild(nsIFrame *forFrame);

        // Find or create a child of this node corresponding to forFrame.
        // XXX better name
        Node *GetChild(nsIFrame *forFrame);                

        nsIFrame *GetFrame() { return mFrame; }

        class ChildChunk {
            friend class nsReflowTree::Node;
            static const int KIDS_CHUNK_SIZE = 5;

            Node       *mKids[KIDS_CHUNK_SIZE];
            ChildChunk *mNext;

            static ChildChunk *Create(Node *firstChild);
            static void Destroy(ChildChunk *chunk);

            Node *GetChild(nsIFrame *forFrame);
        };

        void MakeTarget(PRBool isTarget = PR_TRUE);
        PRBool IsTarget() { return mFlags & NODE_IS_TARGET; }
                
    private:
        Node();
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
        
        friend class nsReflowTree;
    };
        
    Node *AddToTree(nsIFrame *frame);
    
private:
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
