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
 * Bob Miller, kbob@oblix.com
 *    -- plugged core leak.
 *
 * Marina Mechtcheriakova, mmarina@mindspring.com
 *    -- fixed bug in PathExpr::matches
 *       - foo//bar would not match properly if there was more than
 *         one node in the NodeSet (nodes) on the final iteration
 *
 */

#include "Expr.h"
#include "XMLUtils.h"
#include "txNodeSetContext.h"

  //------------/
 //- PathExpr -/
//------------/

const String PathExpr::RTF_INVALID_OP = 
    "Result tree fragments don't allow location steps";
const String PathExpr::NODESET_EXPECTED = 
    "Filter expression must evaluate to a NodeSet";

/**
 * Creates a new PathExpr
**/
PathExpr::PathExpr()
{
    //-- do nothing
}

/**
 * Destructor, will delete all Expressions
**/
PathExpr::~PathExpr()
{
    txListIterator iter(&expressions);
    while (iter.hasNext()) {
         iter.next();
         PathExprItem* pxi = (PathExprItem*)iter.remove();
         delete pxi->expr;
         delete pxi;
    }
} //-- ~PathExpr

/**
 * Adds the Expr to this PathExpr
 * @param expr the Expr to add to this PathExpr
**/
void PathExpr::addExpr(Expr* expr, PathOperator pathOp)
{
    NS_ASSERTION(expressions.getLength() > 0 || pathOp == RELATIVE_OP,
                 "First step has to be relative in PathExpr");
    if (expr) {
        PathExprItem* pxi = new PathExprItem;
        if (!pxi) {
            // XXX ErrorReport: out of memory
            NS_ASSERTION(0, "out of memory");
            return;
        }
        pxi->expr = expr;
        pxi->pathOp = pathOp;
        expressions.add(pxi);
    }
} //-- addPattenExpr

    //-----------------------------/
  //- Virtual methods from Expr -/
//-----------------------------/

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ContextState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
**/
ExprResult* PathExpr::evaluate(txIEvalContext* aContext)
{
    if (!aContext || (expressions.getLength() == 0)) {
        NS_ASSERTION(aContext, "malformed PathExpr");
        return new NodeSet();
    }

    NodeSet* nodes = new NodeSet(aContext->getContextNode());
    if (!nodes) {
        // XXX ErrorReport: out of memory
        NS_ASSERTION(0, "out of memory");
        return 0;
    }
    NodeSet* resNodes = new NodeSet();
    if (!resNodes) {
        delete nodes;
        // XXX ErrorReport: out of memory
        NS_ASSERTION(0, "out of memory");
        return 0;
    }

    ListIterator iter(&expressions);
    nsresult rv = evalStep(iter, aContext, nodes, *resNodes);
    if (NS_FAILED(rv)) {
        NS_ASSERTION(0, "report an error");
        delete nodes;
        delete resNodes;
        return 0;
    }
    delete nodes;
    return resNodes;
} //-- evaluate

nsresult PathExpr::evalStep(txListIterator& aIter, txIMatchContext* aContext,
                            NodeSet* aNodes, NodeSet& aResult)
{
    NS_ASSERTION(aNodes && aContext, "Internal error");

    PathExprItem* pxi;
    nsresult rv = NS_OK;
    if ((pxi = (PathExprItem*)aIter.next())) {
        txNodeSetContext eContext(aNodes, aContext);
        while (eContext.hasNext()) {
            eContext.next();
            Node* node = eContext.getContextNode();
            if (pxi->pathOp == DESCENDANT_OP) {
                NodeSet resNodes;
                evalDescendants(pxi->expr, node, &eContext, &resNodes);
                if (!resNodes.isEmpty()) {
                    rv = evalStep(aIter, aContext, &resNodes, aResult);
                    if (NS_FAILED(rv))
                        return rv;
                }
            }
            else {
                ExprResult *res = pxi->expr->evaluate(&eContext);
                if (!res || (res->getResultType() != ExprResult::NODESET)) {
                    //XXX ErrorReport: report nonnodeset error
                    delete res;
                    NS_ASSERTION(0,"Step didn't return NodeSet");
                    return NS_ERROR_XPATH_EVAL_FAILED;
                }
                NodeSet* resNodes = (NodeSet*)res;
                if (!resNodes->isEmpty()) {
                    rv = evalStep(aIter, aContext, resNodes, aResult);
                    if (NS_FAILED(rv)) {
                        delete resNodes;
                        return rv;
                    }
                }
                delete resNodes;
            }
        }
    }
    else {
        aResult.add(aNodes);
    }
    aIter.previous(); // done with this step, get back
    return NS_OK;
}


/**
 * Selects from the descendants of the context node
 * all nodes that match the Expr
 * -- this will be moving to a Utility class
**/
void PathExpr::evalDescendants (Expr* aStep, Node* aNode, 
                                txIEvalContext* aContext,
                                NodeSet* resNodes)
{
    NodeSet set(aNode);
    txNodeSetContext eContext(&set, aContext);
    eContext.next();
    ExprResult *res = aStep->evaluate(&eContext);
    if (!res || (res->getResultType() != ExprResult::NODESET)) {
        //XXX ErrorReport: report nonnodeset error
    }
    else {
        resNodes->add((NodeSet*)res);
    }
    delete res;

    MBool filterWS = aContext->isStripSpaceAllowed(aNode);
    
    Node* child = aNode->getFirstChild();
    while (child) {
        if (!(filterWS &&
              (child->getNodeType() == Node::TEXT_NODE ||
               child->getNodeType() == Node::CDATA_SECTION_NODE) &&
              XMLUtils::shouldStripTextnode(child->getNodeValue())))
            evalDescendants(aStep, child, aContext, resNodes);
        child = child->getNextSibling();
    }
} //-- evalDescendants

/**
 * Returns the String representation of this Expr.
 * @param dest the String to use when creating the String
 * representation. The String representation will be appended to
 * any data in the destination String, to allow cascading calls to
 * other #toString() methods for Expressions.
 * @return the String representation of this Expr.
**/
void PathExpr::toString(String& dest)
{
    ListIterator iter(&expressions);
    
    PathExprItem* pxi = (PathExprItem*)iter.next();
    if (pxi) {
        NS_ASSERTION(pxi->pathOp == RELATIVE_OP,
                     "First step should be relative");
        pxi->expr->toString(dest);
    }
    
    while ((pxi = (PathExprItem*)iter.next())) {
        switch (pxi->pathOp) {
            case DESCENDANT_OP:
                dest.append("//");
                break;
            case RELATIVE_OP:
                dest.append('/');
                break;
        }
        pxi->expr->toString(dest);
    }
} //-- toString
