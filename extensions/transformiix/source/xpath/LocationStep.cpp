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

/*
  Implementation of an XPath LocationStep
*/

#include "Expr.h"
#include "txNodeSet.h"
#include "txIXPathContext.h"

/**
 * Creates a new LocationStep using the given NodeExpr and Axis Identifier
 * @param nodeExpr the NodeExpr to use when matching Nodes
 * @param axisIdentifier the Axis Identifier in which to search for nodes
**/
LocationStep::LocationStep(nsAutoPtr<txNodeTest> aNodeTest,
                           LocationStepType aAxisIdentifier)
    : mNodeTest(aNodeTest), mAxisIdentifier(aAxisIdentifier)
{
} //-- LocationStep

  //-----------------------------/
 //- Virtual methods from Expr -/
//-----------------------------/

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ProcessorState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 * @see Expr
**/
nsresult
LocationStep::evaluate(txIEvalContext* aContext, txAExprResult** aResult)
{
    NS_ASSERTION(aContext, "internal error");
    *aResult = nsnull;

    nsRefPtr<NodeSet> nodes;
    nsresult rv = aContext->recycler()->getNodeSet(getter_AddRefs(nodes));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool reverse = PR_FALSE;
    txXPathTreeWalker walker(aContext->getContextNode());

    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS:
        {
            if (!walker.moveToParent()) {
                break;
            }
            // do not break here
        }
        case ANCESTOR_OR_SELF_AXIS:
        {
            reverse = PR_TRUE;

            if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                nodes->append(walker.getCurrentPosition());
            }
            while (walker.moveToParent()) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
            }
            break;
        }
        case ATTRIBUTE_AXIS:
        {
            PRBool haveAttr = walker.moveToFirstAttribute();
            while (haveAttr) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                haveAttr = walker.moveToNextSibling();
            }
            break;
        }
        case DESCENDANT_OR_SELF_AXIS:
        {
            if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                nodes->append(walker.getCurrentPosition());
            }
            // do not break here
        }
        case DESCENDANT_AXIS:
        {
            PRBool hasDescendant = walker.moveToFirstDescendant();
            while (hasDescendant) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasDescendant = walker.moveToNextDescendant();
            }
            break;
        }
        case FOLLOWING_AXIS:
        {
            PRBool hasFollowing = walker.moveToFirstFollowing();
            while (hasFollowing) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasFollowing = walker.moveToNextFollowing();
            }
            break;
        }
        case FOLLOWING_SIBLING_AXIS:
        {
            PRBool hasFollowing = walker.moveToFirstFollowingSibling();
            while (hasFollowing) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasFollowing = walker.moveToNextSibling();
            }
            break;
        }
        case NAMESPACE_AXIS: //-- not yet implemented
#if 0
            // XXX DEBUG OUTPUT
            cout << "namespace axis not yet implemented"<<endl;
#endif
            break;
        case PARENT_AXIS :
        {
            if (walker.moveToParent() &&
                mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                nodes->append(walker.getCurrentPosition());
            }
            break;
        }
        case PRECEDING_AXIS:
        {
            reverse = PR_TRUE;

            PRBool hasPreceding = walker.moveToFirstPreceding();
            while (hasPreceding) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasPreceding = walker.moveToNextPreceding();
            }
            break;
        }
        case PRECEDING_SIBLING_AXIS:
        {
            reverse = PR_TRUE;

            PRBool hasSibling = walker.moveToFirstPrecedingSibling();
            while (hasSibling) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasSibling = walker.moveToPreviousSibling();
            }
            break;
        }
        case SELF_AXIS:
        {
            if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                nodes->append(walker.getCurrentPosition());
            }
            break;
        }
        default: // Children Axis
        {
            PRBool hasChild = walker.moveToFirstChild();
            while (hasChild) {
                if (mNodeTest->matches(walker.getCurrentPosition(), aContext)) {
                    nodes->append(walker.getCurrentPosition());
                }
                hasChild = walker.moveToNextSibling();
            }
            break;
        }
    }

    // Apply predicates
    if (!isEmpty()) {
        rv = evaluatePredicates(nodes, aContext);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (reverse) {
        nodes->reverse();
    }

    *aResult = nodes;
    NS_ADDREF(*aResult);

    return NS_OK;
}

/**
 * Creates a String representation of this Expr
 * @param str the destination String to append to
 * @see Expr
**/
void LocationStep::toString(nsAString& str) {
    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS :
            str.Append(NS_LITERAL_STRING("ancestor::"));
            break;
        case ANCESTOR_OR_SELF_AXIS :
            str.Append(NS_LITERAL_STRING("ancestor-or-self::"));
            break;
        case ATTRIBUTE_AXIS:
            str.Append(PRUnichar('@'));
            break;
        case DESCENDANT_AXIS:
            str.Append(NS_LITERAL_STRING("descendant::"));
            break;
        case DESCENDANT_OR_SELF_AXIS:
            str.Append(NS_LITERAL_STRING("descendant-or-self::"));
            break;
        case FOLLOWING_AXIS :
            str.Append(NS_LITERAL_STRING("following::"));
            break;
        case FOLLOWING_SIBLING_AXIS:
            str.Append(NS_LITERAL_STRING("following-sibling::"));
            break;
        case NAMESPACE_AXIS:
            str.Append(NS_LITERAL_STRING("namespace::"));
            break;
        case PARENT_AXIS :
            str.Append(NS_LITERAL_STRING("parent::"));
            break;
        case PRECEDING_AXIS :
            str.Append(NS_LITERAL_STRING("preceding::"));
            break;
        case PRECEDING_SIBLING_AXIS :
            str.Append(NS_LITERAL_STRING("preceding-sibling::"));
            break;
        case SELF_AXIS :
            str.Append(NS_LITERAL_STRING("self::"));
            break;
        default:
            break;
    }
    NS_ASSERTION(mNodeTest, "mNodeTest is null, that's verboten");
    mNodeTest->toString(str);

    PredicateList::toString(str);
} // toString

