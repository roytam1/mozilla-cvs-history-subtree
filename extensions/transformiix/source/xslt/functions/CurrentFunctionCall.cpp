#include "XSLTFunctions.h"
#include "Names.h"

/*
  Implementation of XSLT 1.0 extension function: current
*/

/**
 * Creates a new current function call
**/
CurrentFunctionCall::CurrentFunctionCall() :
        FunctionCall(CURRENT_FN)
{
}

/*
 * Evaluates this Expr
 *
 * @return The current node of the context node set
 */
ExprResult* CurrentFunctionCall::evaluate(txIEvalContext* aContext)
{
    return new NodeSet();
}

