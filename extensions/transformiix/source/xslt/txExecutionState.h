#include "txError.h"
#include "baseutils.h"
#include "txXMLEventHandler.h"
#include "nsCOMPtr.h"
#include "Stack.h"
#include "XMLUtils.h"

class txInstruction;
class txIOutputHandlerFactory;
class txIEvalContext;
class ExprResult;
class txStylesheet;

class txExecutionState
{
public:

    // Stack functions
    nsresult pushEvalContext(txIEvalContext* aContext);
    txIEvalContext* popEvalContext();
    nsresult pushExprResult(ExprResult* aExprResult);
    ExprResult* popExprResult();
    nsresult pushString(const nsAString& aStr);
    void popString(nsAString& aStr);
    nsresult pushInt(PRInt32 aInt);
    PRInt32 popInt();

    //
    nsresult gotoInstruction(txInstruction* aNext);

    // state-getting functions
    txIEvalContext* getEvalContext();
    txInstruction* getAttributeSet(const txExpandedName& aName);

    // state-modification functions
    nsresult runTemplate(txInstruction* aInstruction);

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
};