/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 * 
 */

/**
 * Represents a AdditiveExpr, an binary expression that
 * performs an additive operation between it's lvalue and rvalue:
 *  +   : addition
 *  -   : subtraction
**/

#include "Expr.h"

/**
 * Creates a new AdditiveExpr using the given operator
**/
AdditiveExpr::AdditiveExpr(Expr* aLeftExpr, Expr* aRightExpr, short op) 
    :mOp(op), mLeftExpr(aLeftExpr), mRightExpr(aRightExpr)
{
} // AdditiveExpr

AdditiveExpr::~AdditiveExpr() {
    delete mLeftExpr;
    delete mRightExpr;
} //-- ~AdditiveExpr

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* AdditiveExpr::evaluate(txIEvalContext* aContext)
{
    double rightDbl = Double::NaN;
    ExprResult* exprRes = 0;

    if (mRightExpr) {
        exprRes = mRightExpr->evaluate(aContext);
        if (exprRes)
            rightDbl = exprRes->numberValue();
        delete exprRes;
    }

    double leftDbl = Double::NaN;
    if (mLeftExpr) {
        exprRes = mLeftExpr->evaluate(aContext);
        if (exprRes)
            leftDbl = exprRes->numberValue();
        delete exprRes;
    }

    double result = 0;

    switch (mOp) {
        case SUBTRACTION:
            result = leftDbl - rightDbl;
            break;
        default:
            result = leftDbl + rightDbl;
            break;
    }
   return new NumberResult(result);
} //-- evaluate

/**
 * Returns the String representation of this Expr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this Expr.
**/
void AdditiveExpr::toString(String& aDest)
{
    if (mLeftExpr)
        mLeftExpr->toString(aDest);
    else
        aDest.append("null");

    switch (mOp) {
        case SUBTRACTION:
            aDest.append(" - ");
            break;
        default:
            aDest.append(" + ");
            break;
    }
    if (mRightExpr)
        mRightExpr->toString(aDest);
    else
        aDest.append("null");

} // toString

