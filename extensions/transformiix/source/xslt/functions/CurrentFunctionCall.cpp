#include "XSLTFunctions.h"
#include "Names.h"

/*
  Implementation of XSLT 1.0 extension function: current
*/

/**
 * Creates a new current function call
**/
CurrentFunctionCall::CurrentFunctionCall(ProcessorState* aPs) 
    : FunctionCall(CURRENT_FN), mPs(aPs)
{
}

/*
 * Evaluates this Expr
 *
 * @return The current node of the context node set
 */
ExprResult* CurrentFunctionCall::evaluate(txIEvalContext* aContext)
{
    // mPs->getEvalContext()->getContextNode()
    return new NodeSet();
}

