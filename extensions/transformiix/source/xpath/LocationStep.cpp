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
#include "txIXPathContext.h"

/**
 * Creates a new LocationStep using the given NodeExpr and Axis Identifier
 * @param nodeExpr the NodeExpr to use when matching Nodes
 * @param axisIdentifier the Axis Identifier in which to search for nodes
**/
LocationStep::LocationStep(txNodeTest* aNodeTest,
                           LocationStepType aAxisIdentifier)
{
    mNodeTest = aNodeTest;
    mAxisIdentifier = aAxisIdentifier;
} //-- LocationStep

/**
 * Destroys this LocationStep
 * All predicates will be deleted
 * The NodeExpr will be deleted
**/
LocationStep::~LocationStep() {
    delete mNodeTest;
} //-- ~LocationStep

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ProcessorState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 * @see Expr
**/
ExprResult* LocationStep::evaluate(txIEvalContext* aContext)
{
    NodeSet* result = new NodeSet();
    if (!result)
        return 0;
    if (NS_FAILED(evalStep(aContext->getContextNode(), aContext, result))) {
        delete result;
        return 0;
    }
    return result;
}
   
/*
 * Evaluates this LocationStep as step.
 * This doesn't need a context node set. That is needed for the 
 * predicates, and is generated on evalution.
 */
nsresult LocationStep::evalStep(Node* aNode, txIMatchContext* aContext,
                                NodeSet* aResult)
{
    if (!aContext || !mNodeTest || !aResult)
        return NS_ERROR_INVALID_POINTER;

    MBool reverse = MB_FALSE;

    Node* node = aNode;
    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS :
            node = node->getXPathParent();
            //-- do not break here
        case ANCESTOR_OR_SELF_AXIS :
            reverse = MB_TRUE;
            while (node) {
                if (mNodeTest->matches(node, aContext)) {
                    aResult->append(node);
                }
                node = node->getXPathParent();
            }
            break;
        case ATTRIBUTE_AXIS :
        {
            NamedNodeMap* atts = node->getAttributes();
            if (atts) {
                for (PRUint32 i = 0; i < atts->getLength(); i++) {
                    Node* attr = atts->item(i);
                    if (mNodeTest->matches(attr, aContext))
                        aResult->append(attr);
                }
            }
            break;
        }
        case DESCENDANT_OR_SELF_AXIS :
            if (mNodeTest->matches(node, aContext))
                aResult->append(node);
            //-- do not break here
        case DESCENDANT_AXIS :
            fromDescendants(node, aContext, aResult);
            break;
        case FOLLOWING_AXIS :
        {
            if (node->getNodeType() == Node::ATTRIBUTE_NODE) {
                node = node->getXPathParent();
                fromDescendants(node, aContext, aResult);
            }
            while (node && !node->getNextSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getNextSibling();

                if (mNodeTest->matches(node, aContext))
                    aResult->append(node);

                if (node->hasChildNodes())
                    fromDescendants(node, aContext, aResult);

                while (node && !node->getNextSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        }
        case FOLLOWING_SIBLING_AXIS :
            node = node->getNextSibling();
            while (node) {
                if (mNodeTest->matches(node, aContext))
                    aResult->append(node);
                node = node->getNextSibling();
            }
            break;
        case NAMESPACE_AXIS : //-- not yet implemented
            return NS_ERROR_NOT_IMPLEMENTED;
            break;
        case PARENT_AXIS :
        {
            Node* parent = node->getXPathParent();
            if (mNodeTest->matches(parent, aContext))
                    aResult->append(parent);
            break;
        }
        case PRECEDING_AXIS :
            reverse = MB_TRUE;
            while (node && !node->getPreviousSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getPreviousSibling();

                if (node->hasChildNodes())
                    fromDescendantsRev(node, aContext, aResult);

                if (mNodeTest->matches(node, aContext))
                    aResult->append(node);

                while (node && !node->getPreviousSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        case PRECEDING_SIBLING_AXIS:
            reverse = MB_TRUE;
            node = node->getPreviousSibling();
            while (node) {
                if (mNodeTest->matches(node, aContext))
                    aResult->append(node);
                node = node->getPreviousSibling();
            }
            break;
        case SELF_AXIS :
            if (mNodeTest->matches(node, aContext))
                    aResult->append(node);
            break;
        default: //-- Children Axis
        {
            Node* tmpNode = node->getFirstChild();
            while (tmpNode) {
                if (mNodeTest->matches(tmpNode, aContext))
                    aResult->append(tmpNode);
                tmpNode = tmpNode->getNextSibling();
            }
            break;
        }
    } //-- switch

    //-- apply predicates
    if (!isEmpty())
        evaluatePredicates(aResult, aContext);

    if (reverse)
        aResult->reverse();

    return NS_OK;
} //-- evalStep

void LocationStep::fromDescendants(Node* node, txIMatchContext* cs,
                                   NodeSet* nodes)
{
    if (!node)
        return;

    Node* child = node->getFirstChild();
    while (child) {
        if (mNodeTest->matches(child, cs))
            nodes->append(child);
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendants(child, cs, nodes);

        child = child->getNextSibling();
    }

} //-- fromDescendants

void LocationStep::fromDescendantsRev(Node* node, txIMatchContext* cs,
                                      NodeSet* nodes)
{
    if (!node)
        return;

    Node* child = node->getLastChild();
    while (child) {
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendantsRev(child, cs, nodes);

        if (mNodeTest->matches(child, cs))
            nodes->append(child);

        child = child->getPreviousSibling();
    }

} //-- fromDescendantsRev

/**
 * Creates a String representation of this Expr
 * @param str the destination String to append to
 * @see Expr
**/
void LocationStep::toString(String& aDest) {
    switch (mAxisIdentifier) {
        case ANCESTOR_AXIS :
            aDest.append("ancestor::");
            break;
        case ANCESTOR_OR_SELF_AXIS :
            aDest.append("ancestor-or-self::");
            break;
        case ATTRIBUTE_AXIS:
            aDest.append("@");
            break;
        case DESCENDANT_AXIS:
            aDest.append("descendant::");
            break;
        case DESCENDANT_OR_SELF_AXIS:
            aDest.append("descendant-or-self::");
            break;
        case FOLLOWING_AXIS :
            aDest.append("following::");
            break;
        case FOLLOWING_SIBLING_AXIS:
            aDest.append("following-sibling::");
            break;
        case NAMESPACE_AXIS:
            aDest.append("namespace::");
            break;
        case PARENT_AXIS :
            aDest.append("parent::");
            break;
        case PRECEDING_AXIS :
            aDest.append("preceding::");
            break;
        case PRECEDING_SIBLING_AXIS :
            aDest.append("preceding-sibling::");
            break;
        case SELF_AXIS :
            aDest.append("self::");
            break;
        default:
            break;
    }
    if (mNodeTest)
        mNodeTest->toString(aDest);

    PredicateList::toString(aDest);
} // toString

