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
#include "txIXPathContext.h"

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
PathExpr::PathExpr() : mFilter(0)
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
void PathExpr::addExpr(txStep* expr, PathOperator pathOp)
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

void PathExpr::setFilterExpr(Expr* aExpr)
{
    NS_ASSERTION(expressions.getLength() == 0,
                 "FilterExpr must be first step.");
    NS_ASSERTION(!mFilter, "FilterExpr already set");
    mFilter = aExpr;
}

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

    // XXX WORK TO DO

    // Evaluate FilterExpr, if set
    if (mFilter) {
        ExprResult* result = mFilter->evaluate(aContext);
        if (!result)
            return 0;
        // we do have more steps, or the parser wouldn't have 
        // created a PathExpr
        switch (result->getResultType()) {
            case ExprResult::NODESET:
                nodes = (NodeSet*)result;
                break;
            case ExprResult::TREE_FRAGMENT:
                aContext->receiveError(RTF_INVALID_OP, txLevelError);
                return 0;
            default:
                aContext->receiveError(NODESET_EXPECTED, txLevelError);
                return 0;
        }
    }

    ListIterator iter(&expressions);
    PathExprItem* pxi;

    while ((pxi = (PathExprItem*)iter.next())) {
        NodeSet* tmpNodes = 0;
        for (int i = 0; i < nodes->size(); i++) {
            Node* node = nodes->get(i);
            
            NodeSet* resNodes;
            if (pxi->pathOp == DESCENDANT_OP) {
                resNodes = new NodeSet;
                evalDescendants(pxi->expr, node, aContext, resNodes);
            }
            else {
                ExprResult *res = pxi->expr->evaluate(aContext);
                if (!res || (res->getResultType() != ExprResult::NODESET)) {
                    //XXX ErrorReport: report nonnodeset error
                    delete res;
                    res = new NodeSet;
                }
                resNodes = (NodeSet*)res;
            }

            if (tmpNodes) {
                tmpNodes->add(resNodes);
                delete resNodes;
            }
            else
                tmpNodes = resNodes;

        }
        delete nodes;
        nodes = tmpNodes;
        if (!nodes || (nodes->size() == 0)) break;
    }
    return nodes;
} //-- evaluate

/**
 * Selects from the descendants of the context node
 * all nodes that match the Expr
 * -- this will be moving to a Utility class
**/
nsresult PathExpr::evalDescendants (txStep* aStep, Node* aNode, 
                                    txIMatchContext* aContext,
                                    NodeSet* resNodes)
{
    nsresult res = NS_OK;
    res = aStep->evalStep(aNode, aContext, resNodes);
    if (NS_FAILED(res)) {
        return res;
    }

    MBool filterWS = aContext->isStripSpaceAllowed(aNode);
    
    Node* child = aNode->getFirstChild();
    while (child) {
        if (!(filterWS &&
              (child->getNodeType() == Node::TEXT_NODE ||
               child->getNodeType() == Node::CDATA_SECTION_NODE) &&
              XMLUtils::shouldStripTextnode(child->getNodeValue())))
            res = evalDescendants(aStep, child, aContext, resNodes);
        if (NS_FAILED(res)) {
            return res;
        }
        child = child->getNextSibling();
    }

    return NS_OK;
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
