#include "txError.h"
#include "txNamespaceMap.h"
#include "txExpandedNameMap.h"

extern txHandlerTable* gTxRootHandler;

typedef nsresult (*HandleStartFn) (PRInt32 aNamespaceID,
                                   txAtom* aLocalName,
                                   txAtom* aPrefix,
                                   txStylesheetAttr* aAttributes,
                                   PRInt32 aAttrCount,
                                   txStylesheetCompilerState& aState);
typedef nsresult (*HandleEndFn)   (txStylesheetCompilerState& aState);
typedef nsresult (*HandleTextFn)  (const String& aStr,
                                   txStylesheetCompilerState& aState);

struct txElementHandler {
    PRInt32 mNamespaceID;
    char* mLocalName;
    HandleStartFn mStartFunction;
    HandleEndFn mEndFunction;
};

struct txHandlerTableData {
    txElementHandler* mHandlers;
    txElementHandler mOtherHandler;
    txElementHandler mLREHandler;
    HandleTextFn mTextHandler;
};

class txHandlerTable
{
public:
    txHandlerTable();
    nsresult init(txHandlerTableData* aTableData);
    txElementHandler* find(PRInt32 aNamespaceID, txAtom* aLocalName);
    
    HandleTextFn mTextHandler;
    txElementHandler* mLREHandler;

    static MBool init();
    static void shutdown();

private:
    txElementHandler* mOtherHandler;
    txExpandedNameMap mHandlers;
};
