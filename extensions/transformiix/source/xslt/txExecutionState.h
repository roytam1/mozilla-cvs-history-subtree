#include "txError.h"
#include "baseutils.h"

class txInstruction;

class txExecutionState
{
public:
    nsresult pushEvalContext(txIEvalContext* aContext);
    txIEvalContext* popEvalContext();
    
    nsresult pushExprResult(ExprResult* aExprResult);
    ExprResult* popExprResult();
    
    nsresult gotoInstruction(txInstruction* aNext);



#ifdef TX_EXE
    txIOutputXMLEventHandler* mOutputHandler;
#else
    nsCOMPtr<txIOutputXMLEventHandler> mOutputHandler;
#endif
    txXMLEventHandler* mResultHandler;
    txIOutputHandlerFactory* mOutputHandlerFactory;

private:
    txStylesheet* mStylesheet;
    Stack mStack;
}