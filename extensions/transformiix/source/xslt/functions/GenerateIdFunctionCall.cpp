/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is XSL:P XSLT processor.
 *
 * The Initial Developer of the Original Code is Keith Visco.
 * Portions created by Keith Visco (C) 1999, 2000 Keith Visco.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * Keith Visco, kvisco@ziplink.net
 *    -- original author.
 *
 */

#include "ExprResult.h"
#include "txAtoms.h"
#include "txIXPathContext.h"
#include "txNodeSet.h"
#include "XSLTFunctions.h"

/*
  Implementation of XSLT 1.0 extension function: generate-id
*/

/**
 * Creates a new generate-id function call
**/
GenerateIdFunctionCall::GenerateIdFunctionCall()
{
}

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 * @see FunctionCall.h
**/
nsresult
GenerateIdFunctionCall::evaluate(txIEvalContext* aContext,
                                 txAExprResult** aResult)
{
    *aResult = nsnull;
    if (!requireParams(0, 1, aContext))
        return NS_ERROR_XPATH_BAD_ARGUMENT_COUNT;

    nsresult rv = NS_OK;
    if (params.getLength() != 1) {
        nsRefPtr<StringResult> strRes;
        rv = aContext->recycler()->getStringResult(getter_AddRefs(strRes));
        NS_ENSURE_SUCCESS(rv, rv);

        txXPathNodeUtils::getXSLTId(aContext->getContextNode(),
                                    strRes->mValue);

        *aResult = strRes;
        NS_ADDREF(*aResult);
 
        return NS_OK;
    }

    txListIterator iter(&params);
    Expr* param = (Expr*)iter.next();

    nsRefPtr<txAExprResult> exprResult;
    rv = param->evaluate(aContext, getter_AddRefs(exprResult));
    NS_ENSURE_SUCCESS(rv, rv);

    if (exprResult->getResultType() != txAExprResult::NODESET) {
        NS_NAMED_LITERAL_STRING(err, "Invalid argument passed to generate-id(), expecting NodeSet");
        aContext->receiveError(err, NS_ERROR_XSLT_NODESET_EXPECTED);
        return NS_ERROR_XSLT_NODESET_EXPECTED;
    }

    txNodeSet* nodes = NS_STATIC_CAST(txNodeSet*,
                                      NS_STATIC_CAST(txAExprResult*,
                                                     exprResult));
    if (nodes->isEmpty()) {
        aContext->recycler()->getEmptyStringResult(aResult);

        return NS_OK;
    }
    
    nsRefPtr<StringResult> strRes;
    rv = aContext->recycler()->getStringResult(getter_AddRefs(strRes));
    NS_ENSURE_SUCCESS(rv, rv);

    txXPathNodeUtils::getXSLTId(nodes->get(0), strRes->mValue);

    *aResult = strRes;
    NS_ADDREF(*aResult);
 
    return NS_OK;
}

nsresult GenerateIdFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    *aAtom = txXSLTAtoms::generateId;
    NS_ADDREF(*aAtom);
    return NS_OK;
}
