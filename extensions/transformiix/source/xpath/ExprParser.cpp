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
 * Olivier Gerardin, ogerardin@vo.lu
 *   -- added support for number function calls
 *   -- fixed a bug in CreateExpr (@xxx=/yyy was parsed as @xxx=@xxx/yyy)
 *
 * Marina Mechtcheriakova
 *   -- added support for lang()
 *   -- fixed bug in ::parsePredicates,
 *      made sure we continue looking for more predicates.
 *
 */

/**
 * ExprParser
 * This class is used to parse XSL Expressions
 * @see ExprLexer
**/

#include "ExprParser.h"
#include "FunctionLib.h"
#include "Names.h"
#include "txAtom.h"
#include "txIXPathContext.h"

/**
 * Creates a new ExprParser
**/
ExprParser::ExprParser() {};

/**
 * Default Destructor
**/
ExprParser::~ExprParser() {};

/**
 * Creates an Attribute Value Template using the given value
 * This should move to XSLProcessor class
**/
AttributeValueTemplate* ExprParser::createAttributeValueTemplate
    (const String& attValue, txIParseContext* aContext)
{
    mContext = aContext;

    AttributeValueTemplate* avt = new AttributeValueTemplate();

    if (attValue.isEmpty()) {
        mContext = 0;
        return avt; //XXX should return 0, but that causes crash in lre12
    }

    PRInt32 size = attValue.length();
    int cc = 0;
    UNICODE_CHAR nextCh;
    UNICODE_CHAR ch;
    String buffer;
    MBool inExpr    = MB_FALSE;
    MBool inLiteral = MB_FALSE;
    UNICODE_CHAR endLiteral;

    nextCh = attValue.charAt(cc);
    while (cc++ < size) {
        ch = nextCh;
        nextCh = cc != size ? attValue.charAt(cc) : 0;
        
        // if in literal just add ch to buffer
        if (inLiteral && (ch != endLiteral)) {
                buffer.append(ch);
                continue;
        }
        switch ( ch ) {
            case '\'' :
            case '"' :
                buffer.append(ch);
                if (inLiteral)
                    inLiteral = MB_FALSE;
                else if (inExpr) {
                    inLiteral = MB_TRUE;
                    endLiteral = ch;
                }
                break;
            case  '{' :
                if (!inExpr) {
                    // Ignore case where we find two {
                    if (nextCh == ch) {
                        buffer.append(ch); //-- append '{'
                        cc++;
                        nextCh = cc != size ? attValue.charAt(cc) : 0;
                    }
                    else {
                        if (!buffer.isEmpty())
                            avt->addExpr(new StringExpr(buffer));
                        buffer.clear();
                        inExpr = MB_TRUE;
                    }
                }
                else
                    buffer.append(ch); //-- simply append '{'
                break;
            case '}':
                if (inExpr) {
                    inExpr = MB_FALSE;
                    ExprLexer lexer(buffer);
                    Expr* expr = createExpr(lexer);
                    if (!expr) {
                        delete avt;
                        mContext = 0;
                        return 0;
                    }
                    avt->addExpr(expr);
                    buffer.clear();
                }
                else if (nextCh == ch) {
                    buffer.append(ch);
                    cc++;
                    nextCh = cc != size ? attValue.charAt(cc) : 0;
                }
                else {
                    //XXX ErrorReport: unmatched '}' found
                    delete avt;
                    mContext = 0;
                    return 0;
                }
                break;
            default:
                buffer.append(ch);
                break;
        }
    }

    if (inExpr) {
        //XXX ErrorReport: ending '}' missing
        delete avt;
        mContext = 0;
        return 0;
    }

    if (!buffer.isEmpty())
        avt->addExpr(new StringExpr(buffer));

    mContext = 0;
    return avt;

} //-- createAttributeValueTemplate

Expr* ExprParser::createExpr(const String& aExpression,
                             txIParseContext* aContext)
{
    mContext = aContext;
    ExprLexer lexer(aExpression);
    Expr* expr = createExpr(lexer);
    mContext = 0;
    return expr;
} //-- createExpr

Pattern* ExprParser::createPattern(const String& aPattern,
                                   txIParseContext* aContext)
{
    mContext = aContext;
    ExprLexer lexer(aPattern);
    Pattern* pattern = createUnionExpr(lexer);
    mContext = 0;
    return pattern;
} //-- createPatternExpr

  //--------------------/
 //- Private Methods -/
//-------------------/

/**
 * Creates a binary Expr for the given operator
**/
Expr* ExprParser::createBinaryExpr   (Expr* left, Expr* right, Token* op) {
    if (!op)
        return 0;
    switch (op->type) {
        //-- additive ops
        case Token::ADDITION_OP :
            return new AdditiveExpr(left, right, AdditiveExpr::ADDITION);
        case Token::SUBTRACTION_OP:
            return new AdditiveExpr(left, right, AdditiveExpr::SUBTRACTION);

        //-- case boolean ops
        case Token::AND_OP:
            return new BooleanExpr(left, right, BooleanExpr::AND);
        case Token::OR_OP:
            return new BooleanExpr(left, right, BooleanExpr::OR);

        //-- equality ops
        case Token::EQUAL_OP :
            return new RelationalExpr(left, right, RelationalExpr::EQUAL);
        case Token::NOT_EQUAL_OP :
            return new RelationalExpr(left, right, RelationalExpr::NOT_EQUAL);

        //-- relational ops
        case Token::LESS_THAN_OP:
            return new RelationalExpr(left, right, RelationalExpr::LESS_THAN);
        case Token::GREATER_THAN_OP:
            return new RelationalExpr(left, right, RelationalExpr::GREATER_THAN);
        case Token::LESS_OR_EQUAL_OP:
            return new RelationalExpr(left, right, RelationalExpr::LESS_OR_EQUAL);
        case Token::GREATER_OR_EQUAL_OP:
            return new RelationalExpr(left, right, RelationalExpr::GREATER_OR_EQUAL);

        //-- multiplicative ops
        case Token::DIVIDE_OP :
            return new MultiplicativeExpr(left, right, MultiplicativeExpr::DIVIDE);
        case Token::MODULUS_OP :
            return new MultiplicativeExpr(left, right, MultiplicativeExpr::MODULUS);
        case Token::MULTIPLY_OP :
            return new MultiplicativeExpr(left, right, MultiplicativeExpr::MULTIPLY);
        default:
            break;

    }
    return 0;
    //return new ErrorExpr();
} //-- createBinaryExpr


Expr* ExprParser::createExpr(ExprLexer& lexer) {

    MBool done = MB_FALSE;

    Expr* expr = 0;

    Stack exprs;
    Stack ops;
    
    while (!done) {

        MBool unary = MB_FALSE;
        while (lexer.peek()->type == Token::SUBTRACTION_OP) {
            unary = !unary;
            lexer.nextToken();
        }

        expr = createUnionExpr(lexer);
        if (!expr)
            break;

        if (unary)
            expr = new UnaryExpr(expr);

        Token* tok = lexer.nextToken();
        switch (tok->type) {
            case Token::ADDITION_OP:
            case Token::DIVIDE_OP:
            //-- boolean ops
            case Token::AND_OP :
            case Token::OR_OP :
            //-- equality ops
            case Token::EQUAL_OP:
            case Token::NOT_EQUAL_OP:
            //-- relational ops
            case Token::LESS_THAN_OP:
            case Token::GREATER_THAN_OP:
            case Token::LESS_OR_EQUAL_OP:
            case Token::GREATER_OR_EQUAL_OP:
            //-- multiplicative ops
            case Token::MODULUS_OP:
            case Token::MULTIPLY_OP:
            case Token::SUBTRACTION_OP:
            {
                while (!exprs.empty() &&
                        precedenceLevel(tok->type) 
                       <= precedenceLevel(((Token*)ops.peek())->type)) {
                    expr = createBinaryExpr((Expr*)exprs.pop(),
                                             expr,
                                             (Token*)ops.pop());
                }
                exprs.push(expr);
                ops.push(tok);
                break;
            }
            default:
                lexer.pushBack();
                done = MB_TRUE;
                break;
        }
    }

    // make sure expr != 0
    if (!expr) {
        while (!exprs.empty()) {
            delete (Expr*)exprs.pop();
        }
        return 0;
    }

    while (!exprs.empty()) {
        expr = createBinaryExpr((Expr*)exprs.pop(), expr, (Token*)ops.pop());
    }

    return expr;

} //-- createExpr

Expr* ExprParser::createFilterExpr(ExprLexer& lexer) {

    Token* tok = lexer.nextToken();

    Expr* expr = 0;
    switch (tok->type) {
        case Token::FUNCTION_NAME :
            lexer.pushBack();
            expr = createFunctionCall(lexer);
            break;
        case Token::VAR_REFERENCE :
            {
                String prefix, lName;
                nsresult res = NS_OK;
                PRInt32 nspace;
                res = resolveQName(tok->value, prefix, lName, nspace);
                if (NS_FAILED(res)) {
                    // XXX error report namespace resolve failed
                    return 0;
                }
                expr = new VariableRefExpr(prefix, lName, nspace);
            }
            break;
        case Token::L_PAREN:
            expr = createExpr(lexer);
            if (!expr)
                return 0;

            if (lexer.nextToken()->type != Token::R_PAREN) {
                lexer.pushBack();
                //XXX ErrorReport: right parenthesis expected
                delete expr;
                return 0;
            }
            break;
        case Token::LITERAL :
            expr = new StringExpr(tok->value);
            break;
        case Token::NUMBER:
        {
            expr = new NumberExpr(Double::toDouble(tok->value));
            break;
        }
        default:
            // this should never ever happen.
            lexer.pushBack();
            //XXX ErrorReport: error in parser, please report on bugzilla.mozilla.org
            return 0;
            break;
    }
    if (!expr)
        return 0;

    if (lexer.peek()->type == Token::L_BRACKET) {

        FilterExpr* filterExpr = new FilterExpr(expr);

        //-- handle predicates
        if (!parsePredicates(filterExpr, lexer)) {
            delete filterExpr;
            return 0;
        }
        expr = filterExpr;
    }

    return expr;

} //-- createFilterExpr

FunctionCall* ExprParser::createFunctionCall(ExprLexer& lexer) {

    FunctionCall* fnCall = 0;
    nsresult res = NS_OK;

    Token* tok = lexer.nextToken();
    if (tok->type != Token::FUNCTION_NAME) {
        //XXX ErrorReport: error in parser, please report on bugzilla.mozilla.org
        return 0;
    }

    String fnName = tok->value;

    //-- compare function names
    //-- * we should hash these names for speed

    if (XPathNames::BOOLEAN_FN.isEqual(tok->value)) {
        fnCall = new BooleanFunctionCall(BooleanFunctionCall::TX_BOOLEAN);
    }
    else if (XPathNames::CONCAT_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::CONCAT);
    }
    else if (XPathNames::CONTAINS_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::CONTAINS);
    }
    else if (XPathNames::COUNT_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::COUNT);
    }
    else if (XPathNames::FALSE_FN.isEqual(tok->value)) {
        fnCall = new BooleanFunctionCall(BooleanFunctionCall::TX_FALSE);
    }
    else if (XPathNames::ID_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::ID);
    }
    else if (XPathNames::LANG_FN.isEqual(tok->value)) {
        fnCall = new BooleanFunctionCall(BooleanFunctionCall::TX_LANG);
    }
    else if (XPathNames::LAST_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::LAST);
    }
    else if (XPathNames::LOCAL_NAME_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::LOCAL_NAME);
    }
    else if (XPathNames::NAME_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::NAME);
    }
    else if (XPathNames::NAMESPACE_URI_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::NAMESPACE_URI);
    }
    else if (XPathNames::NORMALIZE_SPACE_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::NORMALIZE_SPACE);
    }
    else if (XPathNames::NOT_FN.isEqual(tok->value)) {
        fnCall = new BooleanFunctionCall(BooleanFunctionCall::TX_NOT);
    }
    else if (XPathNames::POSITION_FN.isEqual(tok->value)) {
        fnCall = new NodeSetFunctionCall(NodeSetFunctionCall::POSITION);
    }
    else if (XPathNames::STARTS_WITH_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::STARTS_WITH);
    }
    else if (XPathNames::STRING_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::STRING);
    }
    else if (XPathNames::STRING_LENGTH_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::STRING_LENGTH);
    }
    else if (XPathNames::SUBSTRING_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::SUBSTRING);
    }
    else if (XPathNames::SUBSTRING_AFTER_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::SUBSTRING_AFTER);
    }
    else if (XPathNames::SUBSTRING_BEFORE_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::SUBSTRING_BEFORE);
    }
    else if (XPathNames::SUM_FN.isEqual(tok->value)) {
        fnCall = new NumberFunctionCall(NumberFunctionCall::SUM);
    }
    else if (XPathNames::TRANSLATE_FN.isEqual(tok->value)) {
        fnCall = new StringFunctionCall(StringFunctionCall::TRANSLATE);
    }
    else if (XPathNames::TRUE_FN.isEqual(tok->value)) {
        fnCall = new BooleanFunctionCall(BooleanFunctionCall::TX_TRUE);
    }
    else if (XPathNames::NUMBER_FN.isEqual(tok->value)) {
        fnCall = new NumberFunctionCall(NumberFunctionCall::NUMBER);
    }
    else if (XPathNames::ROUND_FN.isEqual(tok->value)) {
        fnCall = new NumberFunctionCall(NumberFunctionCall::ROUND);
    }
    else if (XPathNames::CEILING_FN.isEqual(tok->value)) {
        fnCall = new NumberFunctionCall(NumberFunctionCall::CEILING);
    }
    else if (XPathNames::FLOOR_FN.isEqual(tok->value)) {
        fnCall = new NumberFunctionCall(NumberFunctionCall::FLOOR);
    }
    else {
        txAtom* name;
        PRInt32 namespaceID;
        int idx = tok->value.indexOf(':');
        if (idx >= 0) {
            String nameStr, prefixStr;
            tok->value.subString(idx+1, nameStr);
            name = TX_GET_ATOM(nameStr);

            tok->value.subString(0, idx, prefixStr);
            txAtom* prefix = TX_GET_ATOM(prefixStr);
            res = mContext->resolveNamespacePrefix(prefix, namespaceID);
            // XXX report error
            TX_IF_RELEASE_ATOM(prefix);
        }
        else {
            name = TX_GET_ATOM(tok->value);
            namespaceID = kNameSpaceID_None;
        }

        res = mContext->resolveFunctionCall(name, namespaceID, fnCall);
        // XXX report error
        TX_IF_RELEASE_ATOM(name);
    }
    
    if (!fnCall)
        return 0;

    //-- handle parametes
    if (!parseParameters(fnCall, lexer)) {
        delete fnCall;
        return 0;
    }
    return fnCall;
} //-- createFunctionCall

LocationStep* ExprParser::createLocationStep(ExprLexer& lexer) {

    //-- child axis is default
    LocationStep::LocationStepType axisIdentifier = LocationStep::CHILD_AXIS;
    txNodeTest* nodeTest = 0;

    //-- get Axis Identifier or AbbreviatedStep, if present
    Token* tok = lexer.peek();
    switch (tok->type) {
        case Token::AXIS_IDENTIFIER:
        {
            //-- eat token
            lexer.nextToken();
            //-- should switch to a hash here for speed if necessary
            if (ANCESTOR_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::ANCESTOR_AXIS;
            }
            else if (ANCESTOR_OR_SELF_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::ANCESTOR_OR_SELF_AXIS;
            }
            else if (ATTRIBUTE_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::ATTRIBUTE_AXIS;
            }
            else if (CHILD_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::CHILD_AXIS;
            }
            else if (DESCENDANT_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::DESCENDANT_AXIS;
            }
            else if (DESCENDANT_OR_SELF_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::DESCENDANT_OR_SELF_AXIS;
            }
            else if (FOLLOWING_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::FOLLOWING_AXIS;
            }
            else if (FOLLOWING_SIBLING_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::FOLLOWING_SIBLING_AXIS;
            }
            else if (NAMESPACE_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::NAMESPACE_AXIS;
            }
            else if (PARENT_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::PARENT_AXIS;
            }
            else if (PRECEDING_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::PRECEDING_AXIS;
            }
            else if (PRECEDING_SIBLING_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::PRECEDING_SIBLING_AXIS;
            }
            else if (SELF_AXIS.isEqual(tok->value)) {
                axisIdentifier = LocationStep::SELF_AXIS;
            }
            else {
                //XXX ErrorReport: unknow axis
                return 0;
            }
            break;
        }
        case Token::AT_SIGN:
            //-- eat token
            lexer.nextToken();
            axisIdentifier = LocationStep::ATTRIBUTE_AXIS;
            break;
        case Token::PARENT_NODE :
            //-- eat token
            lexer.nextToken();
            axisIdentifier = LocationStep::PARENT_AXIS;
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            if (!nodeTest) {
                //XXX out of memory
                return 0;
            }
            break;
        case Token::SELF_NODE :
            //-- eat token
            lexer.nextToken();
            axisIdentifier = LocationStep::SELF_AXIS;
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            if (!nodeTest) {
                //XXX out of memory
                return 0;
            }
            break;
        default:
            break;
    }

    //-- get NodeTest unless an AbbreviatedStep was found
    if (!nodeTest) {
        tok = lexer.nextToken();

        switch (tok->type) {
            case Token::CNAME :
                {
                    // resolve QName
                    String prefix, lName;
                    nsresult res = NS_OK;
                    PRInt32 nspace;
                    res = resolveQName(tok->value, prefix, lName, nspace);
                    if (NS_FAILED(res)) {
                        // XXX error report namespace resolve failed
                        return 0;
                    }
                    switch (axisIdentifier) {
                        case LocationStep::ATTRIBUTE_AXIS:
                            nodeTest = new txNameTest(prefix, lName, nspace,
                                                      Node::ATTRIBUTE_NODE);
                            break;
                        default:
                            nodeTest = new txNameTest(prefix, lName, nspace,
                                                      Node::ELEMENT_NODE);
                            break;
                    }
                }
                if (!nodeTest) {
                    //XXX ErrorReport: out of memory
                    return 0;
                }
                break;
            default:
                lexer.pushBack();
                nodeTest = createNodeTest(lexer);
                if (!nodeTest) {
                    return 0;
                }
        }
    }
    
    LocationStep* lstep = new LocationStep(nodeTest, axisIdentifier);
    if (!lstep) {
        //XXX out of memory
        delete nodeTest;
        return 0;
    }

    //-- handle predicates
    if (!parsePredicates(lstep, lexer)) {
        delete lstep;
        return 0;
    }

    return lstep;
} //-- createLocationPath

/**
 * This method only handles comment(), text(), processing-instructing() and node()
 *
**/
txNodeTest* ExprParser::createNodeTest(ExprLexer& lexer) {

    txNodeTest* nodeTest = 0;

    Token* nodeTok = lexer.nextToken();

    switch (nodeTok->type) {
        case Token::COMMENT:
            nodeTest = new txNodeTypeTest(txNodeTypeTest::COMMENT_TYPE);
            break;
        case Token::NODE :
            nodeTest = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
            break;
        case Token::PROC_INST :
            nodeTest = new txNodeTypeTest(txNodeTypeTest::PI_TYPE);
            break;
        case Token::TEXT :
            nodeTest = new txNodeTypeTest(txNodeTypeTest::TEXT_TYPE);
            break;
        default:
            lexer.pushBack();
            // XXX ErrorReport: unexpected token
            return 0;
    }
    if (!nodeTest) {
        //XXX out of memory
        return 0;
    }

    if (lexer.nextToken()->type != Token::L_PAREN) {
        lexer.pushBack();
        //XXX ErrorReport: left parenthesis expected
        delete nodeTest;
        return 0;
    }
    if (nodeTok->type == Token::PROC_INST &&
        lexer.peek()->type == Token::LITERAL) {
        Token* tok = lexer.nextToken();
        ((txNodeTypeTest*)nodeTest)->setNodeName(tok->value);
    }
    if (lexer.nextToken()->type != Token::R_PAREN) {
        lexer.pushBack();
        //XXX ErrorReport: right parenthesis expected (or literal for pi)
        delete nodeTest;
        return 0;
    }

    return nodeTest;
} //-- createNodeExpr

/**
 * Creates a PathExpr using the given ExprLexer
 * @param lexer the ExprLexer for retrieving Tokens
**/
Expr* ExprParser::createPathExpr(ExprLexer& lexer) {

    txStep* expr = 0;
    Expr* filter = 0;
    MBool isFilter = MB_FALSE;

    Token* tok = lexer.peek();

    // is this a root expression?
    if (tok->type == Token::PARENT_OP) {
        lexer.nextToken();
        if (!isLocationStepToken(lexer.peek()))
            return new RootExpr(MB_TRUE);

        lexer.pushBack();
    }

    // parse first step (possibly a FilterExpr)
    if (tok->type != Token::PARENT_OP &&
        tok->type != Token::ANCESTOR_OP) {
        if (isFilterExprToken(tok)) {
            filter = createFilterExpr(lexer);
            isFilter = MB_TRUE;
        }
        else
            expr = createLocationStep(lexer);

        if (!expr) 
            return 0;

        // is this a singlestep path expression?
        tok = lexer.peek();
        if (tok->type != Token::PARENT_OP &&
            tok->type != Token::ANCESTOR_OP)
            return expr;
    }
    else {
        expr = new RootExpr(MB_FALSE);
        if (!expr) {
            // XXX ErrorReport: out of memory
            return 0;
        }
    }
    
    // We have a PathExpr containing several steps
    PathExpr* pathExpr = new PathExpr();
    if (!pathExpr) {
        // XXX ErrorReport: out of memory
        delete expr;
        return 0;
    }
    if (!isFilter)
        pathExpr->addExpr(expr, PathExpr::RELATIVE_OP);
    else
        pathExpr->setFilterExpr(filter);

    // this is ugly
    while (1) {
        PathExpr::PathOperator pathOp;
        tok = lexer.nextToken();
        switch (tok->type) {
            case Token::ANCESTOR_OP :
                pathOp = PathExpr::DESCENDANT_OP;
                break;
            case Token::PARENT_OP :
                pathOp = PathExpr::RELATIVE_OP;
                break;
            default:
                lexer.pushBack();
                return pathExpr;
        }
        
        expr = createLocationStep(lexer);
        if (!expr) {
            delete pathExpr;
            return 0;
        }
        
        pathExpr->addExpr(expr, pathOp);
    }

    return pathExpr;
} //-- createPathExpr

/**
 * Creates a PathExpr using the given ExprLexer
 * XXX temporary use as top of XSLT Pattern
 * @param lexer the ExprLexer for retrieving Tokens
**/
Expr* ExprParser::createUnionExpr(ExprLexer& lexer) {

    Expr* expr = createPathExpr(lexer);
    if (!expr)
        return 0;
    
    if (lexer.peek()->type != Token::UNION_OP)
        return expr;

    UnionExpr* unionExpr = new UnionExpr();
    unionExpr->addExpr(expr);

    while (lexer.peek()->type == Token::UNION_OP) {
        lexer.nextToken(); //-- eat token

        expr = createPathExpr(lexer);
        if (!expr) {
            delete unionExpr;
            return 0;
        }
        unionExpr->addExpr(expr);
    }

    return unionExpr;
} //-- createUnionExpr

MBool ExprParser::isFilterExprToken(Token* token) {
    switch (token->type) {
        case Token::LITERAL:
        case Token::NUMBER:
        case Token::FUNCTION_NAME:
        case Token::VAR_REFERENCE:
        case Token::L_PAREN:            // grouping expr
            return MB_TRUE;
        default:
            return MB_FALSE;
    }
} //-- isFilterExprToken

MBool ExprParser::isLocationStepToken(Token* token) {
    switch (token->type) {
        case Token::AXIS_IDENTIFIER :
        case Token::AT_SIGN :
        case Token::PARENT_NODE :
        case Token::SELF_NODE :
            return MB_TRUE;
        default:
            return isNodeTypeToken(token);
    }
} //-- isLocationStepToken

MBool ExprParser::isNodeTypeToken(Token* token) {
    switch (token->type) {
        case Token::CNAME:
        case Token::COMMENT:
        case Token::NODE :
        case Token::PROC_INST :
        case Token::TEXT :
            return MB_TRUE;
        default:
            return MB_FALSE;
    }
} //-- isNodeTypeToken

/**
 * Using the given lexer, parses the tokens if they represent a predicate list
 * If an error occurs a non-zero String pointer will be returned containing the
 * error message.
 * @param predicateList, the PredicateList to add predicate expressions to
 * @param lexer the ExprLexer to use for parsing tokens
 * @return 0 if successful, or a String pointer to the error message
**/
MBool ExprParser::parsePredicates(PredicateList* predicateList, ExprLexer& lexer) {

    while (lexer.peek()->type == Token::L_BRACKET) {
        //-- eat Token
        lexer.nextToken();

        Expr* expr = createExpr(lexer);
        if (!expr)
            return MB_FALSE;

        predicateList->add(expr);

        if (lexer.nextToken()->type != Token::R_BRACKET) {
            lexer.pushBack();
            //XXX ErrorReport: right bracket expected
            return MB_FALSE;
        }
    }
    return MB_TRUE;

} //-- parsePredicates


/**
 * Using the given lexer, parses the tokens if they represent a parameter list
 * If an error occurs a non-zero String pointer will be returned containing the
 * error message.
 * @param list, the List to add parameter expressions to
 * @param lexer the ExprLexer to use for parsing tokens
 * @return MB_TRUE if successful, or a MB_FALSE otherwise
**/
MBool ExprParser::parseParameters(FunctionCall* fnCall, ExprLexer& lexer) {

    if (lexer.nextToken()->type != Token::L_PAREN) {
        lexer.pushBack();
        //XXX ErrorReport: left parenthesis expected
        return MB_FALSE;
    }

    if (lexer.peek()->type == Token::R_PAREN) {
        lexer.nextToken();
        return MB_TRUE;
    }

    while (1) {
        Expr* expr = createExpr(lexer);
        if (!expr)
            return MB_FALSE;

        fnCall->addParam(expr);
            
        switch (lexer.nextToken()->type) {
            case Token::R_PAREN :
                return MB_TRUE;
            case Token::COMMA: //-- param separator
                break;
            default:
                lexer.pushBack();
                //XXX ErrorReport: right parenthesis or comma expected
                return MB_FALSE;
        }
    }

    return MB_FALSE;

} //-- parseParameters

short ExprParser::precedenceLevel(short tokenType) {
    switch (tokenType) {
        case Token::OR_OP:
            return 1;
        case Token::AND_OP:
            return 2;
        //-- equality
        case Token::EQUAL_OP:
        case Token::NOT_EQUAL_OP:
            return 3;
        //-- relational
        case Token::LESS_THAN_OP:
        case Token::GREATER_THAN_OP:
        case Token::LESS_OR_EQUAL_OP:
        case Token::GREATER_OR_EQUAL_OP:
            return 4;
        //-- additive operators
        case Token::ADDITION_OP:
        case Token::SUBTRACTION_OP:
            return 5;
        //-- multiplicative
        case Token::DIVIDE_OP:
        case Token::MULTIPLY_OP:
        case Token::MODULUS_OP:
            return 6;
        default:
            break;
    }
    return 0;
}

nsresult ExprParser::resolveQName(const String& aQName,
                                  String& aPrefix, String& aLocalName,
                                  PRInt32 aNamespace)
{
    nsresult res = NS_OK;
    aNamespace = kNameSpaceID_None;
    int idx = aQName.indexOf(':');
    if (idx > 0) {
        aQName.subString(0, idx, aPrefix);
        txAtom* prefix = TX_GET_ATOM(aPrefix);
        if (!prefix) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        res = mContext->resolveNamespacePrefix(prefix, aNamespace);
        TX_RELEASE_ATOM(prefix);
        if (NS_FAILED(res)) {
            return res;
        }
        aQName.subString(idx+1, aLocalName);
    }
    else {
        // the lexer dealt with idx == 0 
        aLocalName.append(aQName);
    }
    return res;
}
