/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is TransforMiiX XSLT Processor.
 *
 * The Initial Developer of the Original Code is
 * Axel Hecht.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Axel Hecht <axel@pike.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "txXSLTPatterns.h"
#include "txForwardContext.h"

/*
 * txPattern
 *
 * Base class of all patterns
 * Implements only a default getSimplePatterns
 */
nsresult txPattern::getSimplePatterns(txList& aList)
{
    aList.add(this);
    return NS_OK;
}

txPattern::~txPattern()
{
}


/*
 * txUnionPattern
 *
 * This pattern is returned by the parser for "foo | bar" constructs.
 * |xsl:template|s should use the simple patterns
 */

/*
 * Destructor, deletes all LocationPathPatterns
 */
txUnionPattern::~txUnionPattern()
{
    ListIterator iter(&mLocPathPatterns);
    while (iter.hasNext()) {
        delete (Pattern*)iter.next();
    }
}

nsresult txUnionPattern::addPattern(txPattern* aPattern)
{
    if (!aPattern)
        return NS_ERROR_NULL_POINTER;
    mLocPathPatterns.add(aPattern);
    return NS_OK;
}

/*
 * Returns the default priority of this Pattern.
 * UnionPatterns don't like this.
 * This should be called on the simple patterns.
 */
double txUnionPattern::getDefaultPriority()
{
    NS_ASSERTION(0, "Don't call getDefaultPriority on txUnionPattern");
    return Double::NaN;
}

/*
 * Determines whether this Pattern matches the given node within
 * the given context
 * This should be called on the simple patterns for xsl:template,
 * but is fine for xsl:key and xsl:number
 */
MBool txUnionPattern::matches(Node* aNode, txIMatchContext* aContext)
{
    ListIterator iter(&mLocPathPatterns);
    while (iter.hasNext()) {
        txPattern* p = (txPattern*)iter.next();
        if (p->matches(aNode, aContext)) {
            return MB_TRUE;
        }
    }
    return MB_FALSE;
}

nsresult txUnionPattern::getSimplePatterns(txList& aList)
{
    ListIterator iter(&mLocPathPatterns);
    while (iter.hasNext()) {
        aList.add(iter.next());
    }
    return NS_OK;
}

/*
 * The String representation will be appended to any data in the
 * destination String, to allow cascading calls to other
 * toString() methods for mLocPathPatterns.
 */
void txUnionPattern::toString(String& aDest)
{
    #ifdef DEBUG
    aDest.append("txUnionPattern{");
    #endif
    txListIterator iter(&mLocPathPatterns);
    if (iter.hasNext())
        ((Pattern*)iter.next())->toString(aDest);
    while (iter.hasNext()) {
        aDest.append(" | ");
        ((Pattern*)iter.next())->toString(aDest);
    }
    #ifdef DEBUG
    aDest.append("}");
    #endif
} // toString


/*
 * LocationPathPattern
 *
 * a list of step patterns, can start with id or key
 * (dealt with by the parser)
 */

/*
 * Destructor, deletes all PathPatterns
 */
txLocPathPattern::~txLocPathPattern()
{
    ListIterator iter(&mSteps);
    while (iter.hasNext()) {
         delete (Step*)iter.next();
    }
}

nsresult txLocPathPattern::addStep(txPattern* aPattern, MBool isChild)
{
    if (!aPattern)
        return NS_ERROR_NULL_POINTER;
    Step* step = new Step(aPattern, isChild);
    if (!step)
        return NS_ERROR_OUT_OF_MEMORY;
    mSteps.add(step);
    return NS_OK;
}

MBool txLocPathPattern::matches(Node* aNode, txIMatchContext* aContext)
{
    NS_ASSERTION(aNode && mSteps.getLength(), "Internal error");

    ListIterator iter(&mSteps);
    iter.resetToEnd();

    Step* step;
    step = (Step*)iter.previous();
    if (!step->pattern->matches(aNode, aContext))
        return MB_FALSE;
    MBool isChild = step->isChild;
    Node* node = aNode;
    MBool matches;

    while ((step = (Step*)iter.previous())) {
        while (node && !(matches = step->pattern->matches(node, aContext))
               && !isChild) {
            node = node->getXPathParent();
        }
        if (!matches)
            return MB_FALSE;

        isChild = step->isChild;
    }
    return MB_TRUE;
} // txLocPathPattern::matches

double txLocPathPattern::getDefaultPriority()
{
    if (mSteps.getLength()>1) {
        return 0.5;
    }

    return ((Step*)mSteps.get(0))->pattern->getDefaultPriority();
}

void txLocPathPattern::toString(String& aDest)
{
    ListIterator iter(&mSteps);
    #ifdef DEBUG
    aDest.append("txLocPathPattern{");
    #endif
    Step* step;
    step = (Step*)iter.previous();
    if (step) {
        step->pattern->toString(aDest);
    }
    while ((step = (Step*)iter.previous())) {
        if (step->isChild)
            aDest.append("/");
        else
            aDest.append("//");
        step->pattern->toString(aDest);
    }
    #ifdef DEBUG
    aDest.append("}");
    #endif
} // txLocPathPattern::toString

/*
 * txRootPattern
 *
 * a txPattern matching the document node, or '/'
 */

txRootPattern::~txRootPattern()
{
}

MBool txRootPattern::matches(Node* aNode, txIMatchContext* aContext)
{
    return aNode && (aNode->getNodeType() == Node::DOCUMENT_NODE);
}

double txRootPattern::getDefaultPriority()
{
    return 0.5;
}

void txRootPattern::toString(String& aDest)
{
    #ifdef DEBUG
    aDest.append("txRootPattern{");
    #endif
    aDest.append("/");
    #ifdef DEBUG
    aDest.append("}");
    #endif
}

/*
 * txStepPattern
 *
 * a txPattern to hold the NodeTest and the Predicates of a StepPattern
 */

txStepPattern::~txStepPattern()
{
    delete mNodeTest;
}

MBool txStepPattern::matches(Node* aNode, txIMatchContext* aContext)
{
    NS_ASSERTION(mNodeTest && aNode, "Internal error");
    if (!aNode)
        return MB_FALSE;

    if (!mNodeTest->matches(aNode, aContext))
        return MB_FALSE;

    MBool result = MB_TRUE;
    if (isEmpty()) {
        if (!mIsAttr && !aNode->getXPathParent())
            return MB_FALSE;
        return MB_TRUE;
    }
    // Create the context node set for evaluating the predicates
    NodeSet nodes;
    Node* parent = aNode->getXPathParent();
    if (mIsAttr) {
        NamedNodeMap* atts = parent->getAttributes();
        if (atts) {
            PRUint32 i;
            for (i = 0; i < atts->getLength(); i++) {
                Node* attr = atts->item(i);
                if (mNodeTest->matches(attr, aContext))
                    nodes.append(attr);
            }
        }
    }
    else {
        Node* tmpNode = parent->getFirstChild();
        while (tmpNode) {
            if (mNodeTest->matches(tmpNode, aContext))
                nodes.append(tmpNode);
            tmpNode = tmpNode->getNextSibling();
        }
    }

    txForwardContext evalContext(aContext, aNode, &nodes);
    return matchPredicates(&evalContext);
} // matches

double txStepPattern::getDefaultPriority()
{
    if (isEmpty())
        return mNodeTest->getDefaultPriority();
    return 0.5;
}

void txStepPattern::toString(String& aDest)
{
    #ifdef DEBUG
    aDest.append("txStepPattern{");
    #endif
    if (mIsAttr)
        aDest.append("@");
    if (mNodeTest)
        mNodeTest->toString(aDest);

    PredicateList::toString(aDest);
    #ifdef DEBUG
    aDest.append("}");
    #endif
}
