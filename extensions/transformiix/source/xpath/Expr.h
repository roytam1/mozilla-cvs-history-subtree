/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * Larry Fitzpatick, OpenText, lef@opentext.com
 *   -- 19990806
 *     - changed constant short declarations in many of the classes
 *       with enumerations, commented with //--LF
 * Jonas Sicking, sicking@bigfoot.com
 *   -- removal of Patterns and some restructuring
 *      in the class set
 *
 */


#ifndef TRANSFRMX_EXPR_H
#define TRANSFRMX_EXPR_H

#include "txError.h"
#include "TxString.h"
#include "ErrorObserver.h"
#include "NodeSet.h"
#include "Stack.h"
#include "ExprResult.h"
#include "baseutils.h"
#include "TxObject.h"
#include "primitives.h"

/*
  XPath class definitions.
  Much of this code was ported from XSL:P.
*/

/*
 * necessary prototypes
 */
class txIParseContext;
class txIMatchContext;
class txIEvalContext;

typedef class Expr Pattern;
typedef class Expr PatternExpr;


/**
 * A Base Class for all XSL Expressions
**/
class Expr : public TxObject
{
public:
    /**
     * Virtual destructor, important for subclasses
    **/
    virtual ~Expr();

    /**
     * Evaluates this Expr based on the given context node and processor state
     * @param context the context node for evaluation of this Expr
     * @param ps the ContextState containing the stack information needed
     * for evaluation
     * @return the result of the evaluation
    **/
    virtual ExprResult* evaluate(txIEvalContext* aContext) = 0;

    /**
     * Returns the String representation of this Expr.
     * @param dest the String to use when creating the String
     * representation. The String representation will be appended to
     * any data in the destination String, to allow cascading calls to
     * other #toString() methods for Expressions.
     * @return the String representation of this Expr.
    **/
    virtual void toString(String& aDest) = 0;

};

#define TX_DECL_EXPR \
    ExprResult* evaluate(txIEvalContext* aContext); \
    void toString(String& aDest)

class txPattern : public TxObject
{
public:
    virtual ~txPattern();

    /*
     * Determines whether this Pattern matches the given node.
     */
    virtual MBool matches(Node* aNode, txIMatchContext* aContext) = 0;

    /*
     * Returns the default priority of this Pattern.
     *
     * Simple Patterns return the values as specified in XPath 5.5.
     * Returns -Inf for union patterns, as it shouldn't be called on them.
     */
    virtual double getDefaultPriority() = 0;

    /*
     * Returns the String representation of this Pattern.
     * @param dest the String to use when creating the String
     * representation. The String representation will be appended to
     * any data in the destination String, to allow cascading calls to
     * other #toString() methods for Patterns.
     * @return the String representation of this Pattern.
     */
    virtual void toString(String& aDest) = 0;

    /*
     * Adds the simple Patterns to the List.
     * For union patterns, add all sub patterns,
     * all other (simple) patterns just add themselves
     */
    virtual nsresult getSimplePatterns(txList aList);
};

#define TX_DECL_PATTERN \
    MBool matches(Node* aNode, txIMatchContext* aContext); \
    double getDefaultPriority(); \
    void toString(String& aDest)
#define TX_DECL_PATTERN2 \
    TX_DECL_PATTERN; \
    nsresult getSimplePatterns(txList aList)

#define TX_DECL_EXPR_PATTERN \
    ExprResult* evaluate(txIEvalContext* aContext); \
    TX_DECL_PATTERN

class txStep : virtual public Expr, public txPattern
{
public:
    virtual ~txStep();
    virtual nsresult evalStep(Node* aNode, txIMatchContext* aContext,
                              NodeSet* aResult) = 0;
    virtual void toString(String& aDest) = 0;
};

#define TX_DECL_STEP \
    TX_DECL_EXPR_PATTERN; \
    nsresult evalStep(Node* aNode, txIMatchContext* aContext, NodeSet* aResult)

/**
 * This class represents a FunctionCall as defined by the XPath 1.0
 * Recommendation.
**/
class FunctionCall : public Expr
{
public:
    static const String INVALID_PARAM_COUNT;
    static const String INVALID_PARAM_VALUE;

    virtual ~FunctionCall();

    /**
     * Virtual methods from Expr 
    **/
    virtual ExprResult* evaluate(txIEvalContext* aContext) = 0;
    virtual void toString(String& aDest);

    /**
     * Adds the given parameter to this FunctionCall's parameter list
     * @param expr the Expr to add to this FunctionCall's parameter list
    **/
    nsresult addParam(Expr* expr);

    /*
     * XXX txIEvalContext should be txIParseContest, to do
     */
    virtual MBool requireParams(int aParamCountMin, txIEvalContext* aContext);
    virtual MBool requireParams(int aParamCountMin,
                                int aParamCountMax,
                                txIEvalContext* aContext);

protected:
    txList params;
    String name;

    FunctionCall();
    FunctionCall(const String& aName);

    /*
     * Evaluates the given Expression and converts its result to a String.
     * The value is appended to the given destination String
     */
    void evaluateToString(Expr* aExpr, txIEvalContext* aContext,
                          String& aDest);

    /*
     * Evaluates the given Expression and converts its result to a number.
     */
    double evaluateToNumber(Expr* aExpr, txIEvalContext* aContext);

    /*
     * Evaluates the given Expression and converts its result to a boolean.
     */
    MBool evaluateToBoolean(Expr* aExpr, txIEvalContext* aContext);

    /*
     * Evaluates the given Expression and converts its result to a NodeSet.
     * If the result is not a NodeSet NULL is returned.
     */
    NodeSet* evaluateToNodeSet(Expr* aExpr, txIEvalContext* aContext);
}; // FunctionCall


/**
 * Represents an AttributeValueTemplate
**/
class AttributeValueTemplate: public Expr {

public:

    AttributeValueTemplate();

    virtual ~AttributeValueTemplate();

    /**
     * Adds the given Expr to this AttributeValueTemplate
    **/
    void addExpr(Expr* aExpr);

    TX_DECL_EXPR;

private:
    txList mExpressions;
};


/*
 * This class represents a NodeTest as defined by the XPath spec
 */
class txNodeTest : public txStep
{
public:
    virtual ~txNodeTest() {}
    /*
     * Virtual methods from txStep
     * txNodeTests use the txPattern::getSimplePatterns implementation,
     * so don't overload it here
     */
    virtual MBool matches(Node* aNode, txIMatchContext* aContext) = 0;
    virtual double getDefaultPriority() = 0;
    virtual ExprResult* evaluate(txIEvalContext* aContext) = 0;
    virtual nsresult evalStep(Node* aNode, txIMatchContext* aContext,
                              NodeSet* aResult) = 0;
    virtual void toString(String& aDest) = 0;
};

/*
 * This class represents a NameTest as defined by the XPath spec
 */
class txNameTest : public txNodeTest
{
public:
    /*
     * Creates a new txNameTest with the given type and the given
     * principal node type
     */
    txNameTest(String& aPrefix, String& aLocalName, PRUint32 aNSID,
               Node::NodeType aNodeType);

    ~txNameTest();

    TX_DECL_STEP;

private:
    String mPrefix;
    txAtom* mLocalName;
    PRUint32 mNamespace;
    Node::NodeType mNodeType;
};

/*
 * This class represents a NodeType as defined by the XPath spec
 */
class txNodeTypeTest : public txNodeTest
{
public:
    enum NodeType {
        COMMENT_TYPE,
        TEXT_TYPE,
        PI_TYPE,
        NODE_TYPE
    };

    /*
     * Creates a new txNodeTypeTest of the given type
     */
    txNodeTypeTest(NodeType aNodeType);

    ~txNodeTypeTest();

    /*
     * Makes this step a standalone expression or pattern.
     * If this is set, the step will assume an implicit
     * child:: axis, therefor node() will not match a document node
     */
    void setStandalone()
    {
        mStandalone = MB_TRUE;
    }

    /*
     * Sets the name of the node to match. Only availible for pi nodes
     */
    void setNodeName(const String& aName);

    TX_DECL_STEP;

private:
    NodeType mNodeType;
    txAtom* mNodeName;
    MBool mStandalone;
};

/**
 * Represents an ordered list of Predicates,
 * for use with Step and Filter Expressions
**/
class PredicateList  {

public:

    /**
     * Creates a new PredicateList
    **/
    PredicateList();
    /**
     * Destructor, will delete all Expressions in the list, so remove
     * any you may need
    **/
    virtual ~PredicateList();

    /**
     * Adds the given Expr to the list
     * @param expr the Expr to add to the list
    **/
    void add(Expr* expr);

    void evaluatePredicates(NodeSet* nodes, txIMatchContext* aContext);
    MBool matchPredicates(txIEvalContext* aContext);

    /**
     * returns true if this predicate list is empty
    **/
    MBool isEmpty();

    /**
     * Returns the String representation of this PredicateList.
     * @param dest the String to use when creating the String
     * representation. The String representation will be appended to
     * any data in the destination String, to allow cascading calls to
     * other #toString() methods for Expressions.
     * @return the String representation of this PredicateList.
    **/
    virtual void toString(String& aDest);

private:
    //-- list of predicates
    List predicates;
}; //-- PredicateList

class LocationStep : public PredicateList, public txStep
{
public:

    // Axis Identifier Types
    //-- LF changed from static const short to enum
    enum LocationStepType {
        ANCESTOR_AXIS = 0,
        ANCESTOR_OR_SELF_AXIS,
        ATTRIBUTE_AXIS,
        CHILD_AXIS,
        DESCENDANT_AXIS,
        DESCENDANT_OR_SELF_AXIS,
        FOLLOWING_AXIS,
        FOLLOWING_SIBLING_AXIS,
        NAMESPACE_AXIS,
        PARENT_AXIS,
        PRECEDING_AXIS,
        PRECEDING_SIBLING_AXIS,
        SELF_AXIS
    };

    /**
     * Creates a new LocationStep using the given NodeExpr and Axis Identifier
     * @param nodeExpr the NodeExpr to use when matching Nodes
     * @param axisIdentifier the Axis Identifier in which to search for nodes
    **/
    LocationStep(txNodeTest* aNodeTest, LocationStepType aAxisIdentifier);

    /**
     * Destructor, will delete all predicates and the given NodeExpr
    **/
    virtual ~LocationStep();

    TX_DECL_STEP;

private:

    txNodeTest* mNodeTest;
    LocationStepType mAxisIdentifier;

    void fromDescendants(Node* node, txIMatchContext* aContext,
                         NodeSet* nodes);
    void fromDescendantsRev(Node* node, txIMatchContext* aContext,
                            NodeSet* nodes);

}; //-- LocationStep


class FilterExpr : public PredicateList, public Expr {

public:

    /**
     * Creates a new FilterExpr using the given Expr
     * @param expr the Expr to use for evaluation
    **/
    FilterExpr(Expr* aExpr);

    /**
     * Destructor, will delete all predicates and the given Expr
    **/
    virtual ~FilterExpr();

    TX_DECL_EXPR;

private:
    Expr* expr;

}; //-- FilterExpr


class NumberExpr : public Expr {

public:

    NumberExpr(double dbl);

    TX_DECL_EXPR;

private:

    double _value;
};

/**
 * Represents a String expression
**/
class StringExpr : public Expr {

public:

    StringExpr(const String& aValue);

    TX_DECL_EXPR;

private:

    String value;
}; //-- StringExpr


/**
 * Represents an AdditiveExpr, a binary expression that
 * performs an additive operation between it's lvalue and rvalue:
 *  +   : add
 *  -   : subtract
**/
class AdditiveExpr : public Expr {

public:

    //-- AdditiveExpr Types
    //-- LF, changed from static const short to enum
    enum _AdditiveExprType { ADDITION = 1, SUBTRACTION };

     AdditiveExpr(Expr* aLeftExpr, Expr* aRightExpr, short op);
     ~AdditiveExpr();

    TX_DECL_EXPR;

private:
    short mOp;
    Expr* mLeftExpr;
    Expr* mRightExpr;
}; //-- AdditiveExpr

/**
 * Represents an UnaryExpr. Returns the negative value of it's expr.
**/
class UnaryExpr : public Expr {

public:

     UnaryExpr(Expr* aExpr);
     ~UnaryExpr();

    TX_DECL_EXPR;

private:
    Expr* mExpr;
}; //-- UnaryExpr

/**
 * Represents a BooleanExpr, a binary expression that
 * performs a boolean operation between it's lvalue and rvalue.
**/
class BooleanExpr : public Expr {

public:

    //-- BooleanExpr Types
    enum _BooleanExprType { AND = 1, OR };

     BooleanExpr(Expr* leftExpr, Expr* rightExpr, short op);
     ~BooleanExpr();

    TX_DECL_EXPR;

private:
    short mOp;
    Expr* mLeftExpr;
    Expr* mRightExpr;
}; //-- BooleanExpr

/**
 * Represents a MultiplicativeExpr, a binary expression that
 * performs a multiplicative operation between it's lvalue and rvalue:
 *  *   : multiply
 * mod  : modulus
 * div  : divide
 *
**/
class MultiplicativeExpr : public Expr {

public:

    //-- MultiplicativeExpr Types
    //-- LF, changed from static const short to enum
    enum _MultiplicativeExprType { DIVIDE = 1, MULTIPLY, MODULUS };

     MultiplicativeExpr(Expr* aLeftExpr, Expr* aRightExpr, short aOp);
     ~MultiplicativeExpr();

    TX_DECL_EXPR;

private:
    Expr* mLeftExpr;
    Expr* mRightExpr;
    short mOp;
}; //-- MultiplicativeExpr

/**
 * Represents a RelationalExpr, an expression that compares it's lvalue
 * to it's rvalue using:
 * =  : equal to
 * <  : less than
 * >  : greater than
 * <= : less than or equal to
 * >= : greater than or equal to
 *
**/
class RelationalExpr : public Expr {

public:

    //-- RelationalExpr Types
    //-- LF, changed from static const short to enum
    enum _RelationalExprType {
        EQUAL = 1,
        NOT_EQUAL,
        LESS_THAN,
        GREATER_THAN,
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL
    };

     RelationalExpr(Expr* leftExpr, Expr* rightExpr, short op);
     ~RelationalExpr();

    TX_DECL_EXPR;

private:
    short op;
    Expr* leftExpr;
    Expr* rightExpr;

    MBool compareResults(ExprResult* left, ExprResult* right);
}; //-- RelationalExpr

/**
 * VariableRefExpr
 * Represents a variable reference ($refname)
**/
class VariableRefExpr : public Expr {

public:

    VariableRefExpr(const String& aPrefix,
                    const String& aLocalName, PRInt32 aID);

    TX_DECL_EXPR;

private:
    String mPrefix;
    txAtom* mLocalName;
    PRUint32 mNamespace;
};

/**
 *  Represents a PathExpr
**/
class PathExpr : public Expr {

public:

    //-- Path Operators
    //-- RELATIVE_OP is the default
    //-- LF, changed from static const short to enum
    enum PathOperator { RELATIVE_OP, DESCENDANT_OP };

    /**
     * Creates a new PathExpr
    **/
    PathExpr();

    /**
     * Destructor, will delete all Expressions
    **/
    virtual ~PathExpr();

    /**
     * Adds the Expr to this PathExpr
     * @param expr the Expr to add to this PathExpr
    **/
    void addExpr(txStep* expr, PathOperator pathOp);

    void setFilterExpr(Expr* aExpr);

    TX_DECL_EXPR;

private:
    static const String RTF_INVALID_OP;
    static const String NODESET_EXPECTED;
    struct PathExprItem {
        txStep* expr;
        PathOperator pathOp;
    };

    txList expressions;
    Expr* mFilter;

    /**
     * Selects from the descendants of the context node
     * all nodes that match the Expr
     * -- this will be moving to a Utility class
     **/
    nsresult evalDescendants(txStep* aStep, Node* aNode,
                         txIMatchContext* aContext,
                         NodeSet* resNodes);

}; //-- PathExpr

/**
 * This class represents a RootExpr, which only matches the Document node
**/
class RootExpr : public txStep
{
public:

    /**
     * Creates a new RootExpr
     * @param aSerialize should this RootExpr be serialized
     */
    RootExpr(MBool aSerialize);

    TX_DECL_STEP;

private:
    // When a RootExpr is used in a PathExpr it shouldn't be serialized
    MBool mSerialize;

}; //-- RootExpr

/**
 *  Represents a UnionExpr
**/
class UnionExpr : public Expr {

public:

    /**
     * Creates a new UnionExpr
    **/
    UnionExpr();

    /**
     * Destructor, will delete all Path Expressions
    **/
    virtual ~UnionExpr();

    /**
     * Adds the PathExpr to this UnionExpr
     * @param expr the Expr to add to this UnionExpr
    **/
    void addExpr(Expr* expr);

    TX_DECL_EXPR;

private:

   List expressions;

}; //-- UnionExpr

#endif


