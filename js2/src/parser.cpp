/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is the JavaScript 2 Prototype.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.   Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s):
*
* Alternatively, the contents of this file may be used under the
* terms of the GNU Public License (the "GPL"), in which case the
* provisions of the GPL are applicable instead of those above.
* If you wish to allow use of your version of this file only
* under the terms of the GPL and not to allow others to use your
* version of this file under the NPL, indicate your decision by
* deleting the provisions above and replace them with the notice
* and other provisions required by the GPL.  If you do not delete
* the provisions above, a recipient may use your version of this
* file under either the NPL or the GPL.
*/

#include "numerics.h"
#include "parser.h"

namespace JS = JavaScript;


//
// Parser
//

// Create a new Parser for parsing the provided source code, interning
// identifiers, keywords, and regular expressions in the designated world,
// and allocating the parse tree in the designated arena.
JS::Parser::Parser(World &world, Arena &arena, const String &source, const String &sourceLocation, uint32 initialLineNum):
        lexer(world, source, sourceLocation, initialLineNum), arena(arena), lineBreaksSignificant(true)
{
}


// Report a syntax error at the backUp-th last token read by the Lexer.
// In other words, if backUp is 0, the error is at the next token to be read
// by the Lexer (which must have been peeked already); if backUp is 1, the
// error is at the last token read by the Lexer, and so forth.
void JS::Parser::syntaxError(const char *message, uint backUp)
{
    syntaxError(widenCString(message), backUp);
}


// Same as above, but the error message is already a String.
void JS::Parser::syntaxError(const String &message, uint backUp)
{
    while (backUp--)
        lexer.unget();
    getReader().error(Exception::syntaxError, message, lexer.getPos());
}


// Get the next token using the given preferRegExp setting.  If that token's
// kind matches the given kind, consume that token and return it.  Otherwise
// throw a syntax error.
const JS::Token &JS::Parser::require(bool preferRegExp, Token::Kind kind)
{
    const Token &t = lexer.get(preferRegExp);
    if (!t.hasKind(kind)) {
        String message;
        bool special = Token::isSpecialKind(kind);

        if (special)
            message += '\'';
        message += Token::kindName(kind);
        if (special)
            message += '\'';
        message += " expected";
        syntaxError(message);
    }
    return t;
}


// Copy the Token's chars into the current arena and return the resulting copy.
inline JS::String &JS::Parser::copyTokenChars(const Token &t)
{
    return newArenaString(arena, t.getChars());
}


// If t is an Identifier, return a new IdentifierExprNode corresponding to t.
// Otherwise, return null.
JS::ExprNode *JS::Parser::makeIdentifierExpression(const Token &t) const
{
    if (t.hasIdentifierKind())
        return new(arena) IdentifierExprNode(t);
    return 0;
}


// Parse and return an Identifier.  If the token has been peeked, it should have
// been done with preferRegExp set to true.
const JS::StringAtom &JS::Parser::parseIdentifier()
{
    const Token &t = lexer.get(true);
    if (!t.hasIdentifierKind())
        syntaxError("Identifier expected");
    return t.getIdentifier();
}


// An identifier or parenthesized expression has just been parsed into e.
// If it is followed by one or more ::'s followed by identifiers, construct
// the appropriate qualify parse node and return it and set foundQualifiers
// to true.  If no :: is found, return e and set foundQualifiers to false.
// After parseIdentifierQualifiers finishes, the next token might have been
// peeked with the given preferRegExp setting.
JS::ExprNode *JS::Parser::parseIdentifierQualifiers(ExprNode *e, bool &foundQualifiers, bool preferRegExp)
{
    const Token *tDoubleColon = lexer.eat(preferRegExp, Token::doubleColon);
    if (!tDoubleColon) {
        foundQualifiers = false;
        return e;
    }

    foundQualifiers = true;
    size_t pos = tDoubleColon->getPos();
    return new(arena) QualifyExprNode(pos, e, parseIdentifier());
}


// An opening parenthesis has just been parsed into tParen.  Finish parsing a
// ParenthesizedListExpression (or ParenthesizedExpression if noComma is true).
// If it is in fact a ParenthesizedExpression and is followed by a :: and an identifier,
// construct the appropriate qualify parse node, return it, set foundQualifiers to true.
// Otherwise return the ParenthesizedListExpression and set foundQualifiers to false.
// After parseParenthesesAndIdentifierQualifiers finishes, the next token might have been
// peeked with the given preferRegExp setting.
JS::ExprNode *JS::Parser::parseParenthesesAndIdentifierQualifiers(const Token &tParen, bool noComma, bool &foundQualifiers,
                                                                  bool preferRegExp)
{
    size_t pos = tParen.getPos();
    ExprNode *inner = parseGeneralExpression(false, false, false, noComma);
    ExprNode *e = new(arena) UnaryExprNode(pos, ExprNode::parentheses, inner);
    require(false, Token::closeParenthesis);
    if (inner->hasKind(ExprNode::comma)) {
        foundQualifiers = false;
        return e;
    }
    return parseIdentifierQualifiers(e, foundQualifiers, preferRegExp);
}


// Parse and return a qualifiedIdentifier.  The first token has already been parsed and is in t.
// If the second token was peeked, it should be have been done with the given preferRegExp setting.
// After parseQualifiedIdentifier finishes, the next token might have been peeked with the given
// preferRegExp setting.
JS::ExprNode *JS::Parser::parseQualifiedIdentifier(const Token &t, bool preferRegExp)
{
    bool foundQualifiers;
    ExprNode *e = makeIdentifierExpression(t);
    if (e)
        return parseIdentifierQualifiers(e, foundQualifiers, preferRegExp);
    if (t.hasKind(Token::openParenthesis)) {
        e = parseParenthesesAndIdentifierQualifiers(t, true, foundQualifiers, preferRegExp);
        goto checkQualifiers;
    }
    if (t.hasKind(Token::Public) || t.hasKind(Token::Private)) {
        e = parseIdentifierQualifiers(new(arena) IdentifierExprNode(t), foundQualifiers, preferRegExp);
      checkQualifiers:
        if (!foundQualifiers)
            syntaxError("'::' expected", 0);
        return e;
    }
    syntaxError("Qualified identifier expected");
    return 0;   // Unreachable code here just to shut up compiler warnings
}


// Parse and return an arrayLiteral. The opening bracket has already been
// read into initialToken.
JS::PairListExprNode *JS::Parser::parseArrayLiteral(const Token &initialToken)
{
    size_t initialPos = initialToken.getPos();
    NodeQueue<ExprPairList> elements;

    while (true) {
        ExprNode *element = 0;
        const Token &t = lexer.peek(true);
        if (t.hasKind(Token::comma) || t.hasKind(Token::closeBracket))
            lexer.redesignate(false);   // Safe: neither ',' nor '}' starts with a slash.
        else
            element = parseAssignmentExpression(false);
        elements += new(arena) ExprPairList(0, element);

        const Token &tSeparator = lexer.get(false);
        if (tSeparator.hasKind(Token::closeBracket))
            break;
        if (!tSeparator.hasKind(Token::comma))
            syntaxError("',' expected");
    }
    return new(arena) PairListExprNode(initialPos, ExprNode::arrayLiteral, elements.first);
}


// Parse and return an objectLiteral. The opening brace has already been
// read into initialToken.
JS::PairListExprNode *JS::Parser::parseObjectLiteral(const Token &initialToken)
{
    size_t initialPos = initialToken.getPos();
    NodeQueue<ExprPairList> elements;

    if (!lexer.eat(true, Token::closeBrace))
        while (true) {
            const Token &t = lexer.get(true);
            ExprNode *field = makeIdentifierExpression(t);
            if (!field) {
                if (t.hasKind(Token::string))
                    field = new(arena) StringExprNode(t.getPos(), ExprNode::string, copyTokenChars(t));
                else if (t.hasKind(Token::number))
                    field = new(arena) NumberExprNode(t);
                else if (t.hasKind(Token::openParenthesis)) {
                    field = parseAssignmentExpression(false);
                    require(false, Token::closeParenthesis);
                } else
                    syntaxError("Field name expected");
            }
            require(false, Token::colon);
            elements += new(arena) ExprPairList(field, parseAssignmentExpression(false));

            const Token &tSeparator = lexer.get(false);
            if (tSeparator.hasKind(Token::closeBrace))
                break;
            if (!tSeparator.hasKind(Token::comma))
                syntaxError("',' expected");
        }
    return new(arena) PairListExprNode(initialPos, ExprNode::objectLiteral, elements.first);
}


// e is a UnitExpression or a ParenthesizedListExpression.  If it is followed by one or more
// string literals on the same line, construct and return the appropriate outer UnitExpression;
// otherwise return e.  After parseUnitSuffixes finishes, the next token might have been peeked
// with preferRegExp set to false.
JS::ExprNode *JS::Parser::parseUnitSuffixes(ExprNode *e)
{
    while (true) {
        const Token &t = lexer.peek(false);
        if (lineBreakBefore(t) || !t.hasKind(Token::string))
            return e;
        lexer.get(false);
        e = new(arena) ExprUnitExprNode(t.getPos(), ExprNode::exprUnit, e, copyTokenChars(t));
    }
}


// A super token has just been read.  If superState is ssExpr, parse a SuperExpression;
// if superState is ssStmt, parse either a SuperExpression or a SuperStatement, giving
// preference to SuperExpression.  superState cannot be ssNone.  pos is the position of
// the super token.
// After parseSuper finishes, the next token might have been peeked with preferRegExp
// set to false.
JS::ExprNode *JS::Parser::parseSuper(size_t pos, SuperState superState)
{
    ASSERT(superState != ssNone);
    ExprNode *e = 0;

    if (lexer.eat(false, Token::openParenthesis)) {
        if (superState == ssExpr) {
            e = parseAssignmentExpression(false);
            require(false, Token::closeParenthesis);
        } else {
            InvokeExprNode *se = parseInvoke(0, pos, Token::closeParenthesis, ExprNode::superStmt);
            // Simplify a one-anonymous-argument superStmt into a superExpr.
            ExprPairList *pairs = se->pairs;
            if (!pairs || pairs->next || pairs->field)
                return se;
            e = pairs->value;
        }
    }
    return new(arena) SuperExprNode(pos, e);
}


// Parse and return a PrimaryExpression.  If superState is ssExpr, also allow a SuperExpression;
// if superState is ssStmt, also allow a SuperExpression or a SuperStatement.
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parsePrimaryExpression finishes, the next token might have been peeked with preferRegExp
// set to false.
JS::ExprNode *JS::Parser::parsePrimaryExpression(SuperState superState)
{
    ExprNode *e;
    ExprNode::Kind eKind;

    const Token &t = lexer.get(true);
    switch (t.getKind()) {
      case Token::Null:
        eKind = ExprNode::Null;
        goto makeExprNode;

      case Token::True:
        eKind = ExprNode::True;
        goto makeExprNode;

      case Token::False:
        eKind = ExprNode::False;
        goto makeExprNode;

      case Token::This:
        eKind = ExprNode::This;
      makeExprNode:
        e = new(arena) ExprNode(t.getPos(), eKind);
        break;

      case Token::Public:
        if (lexer.peek(false).hasKind(Token::doubleColon))
            goto makeQualifiedIdentifierNode;
        e = new(arena) IdentifierExprNode(t);
        break;

      case Token::number:
        {
            const Token &tUnit = lexer.peek(false);
            if (!lineBreakBefore(tUnit) &&
                (tUnit.hasKind(Token::unit) || tUnit.hasKind(Token::string))) {
                lexer.get(false);
                e = parseUnitSuffixes(new(arena) NumUnitExprNode(t.getPos(), ExprNode::numUnit, copyTokenChars(t),
                                                                 t.getValue(), copyTokenChars(tUnit)));
            } else
                e = new(arena) NumberExprNode(t);
        }
        break;

      case Token::string:
        e = new(arena) StringExprNode(t.getPos(), ExprNode::string, copyTokenChars(t));
        break;

      case Token::regExp:
        e = new(arena) RegExpExprNode(t.getPos(), ExprNode::regExp, t.getIdentifier(), copyTokenChars(t));
        break;

      case Token::Private:
      case CASE_TOKEN_NONRESERVED:
      makeQualifiedIdentifierNode:
        e = parseQualifiedIdentifier(t, false);
        break;

      case Token::openParenthesis:
        {
            bool foundQualifiers;
            e = parseParenthesesAndIdentifierQualifiers(t, false, foundQualifiers, false);
            if (!foundQualifiers)
                e = parseUnitSuffixes(e);
        }
        break;

      case Token::openBracket:
        e = parseArrayLiteral(t);
        break;

      case Token::openBrace:
        e = parseObjectLiteral(t);
        break;

      case Token::Function:
        {
            FunctionExprNode *f = new(arena) FunctionExprNode(t.getPos());
            const Token &t2 = lexer.get(true);
            f->function.prefix = FunctionName::normal;
            if (!(f->function.name = makeIdentifierExpression(t2)))
                lexer.unget();
            parseFunctionSignature(f->function);
            f->function.body = parseBody(0);
            e = f;
        }
        break;

      case Token::Super:
        if (superState != ssNone) {
            e = parseSuper(t.getPos(), superState);
            break;
        }
        // Fall through to a syntax error if super is not allowed.
      default:
        syntaxError("Expression expected");
        // Unreachable code here just to shut up compiler warnings
        e = 0;
    }

    return e;
}


// Parse a QualifiedIdentifier, ParenthesizedExpression, or 'class' following a dot
// and return the resulting ExprNode.
// tOperator is the . token.  target is the first operand.  If target is a superExpr then
// only a QualifiedIdentifier is allowed after the dot.
// After parseMember finishes, the next token might have been peeked with the given
// preferRegExp setting.
JS::ExprNode *JS::Parser::parseMember(ExprNode *target, const Token &tOperator, bool preferRegExp)
{
    size_t pos = tOperator.getPos();
    const Token &t2 = lexer.get(true);

    if (t2.hasKind(Token::Class) && !target->hasKind(ExprNode::superExpr))
        return new(arena) UnaryExprNode(pos, ExprNode::dotClass, target);

    ExprNode *member;
    ExprNode::Kind kind = ExprNode::dot;
    if (t2.hasKind(Token::openParenthesis) && !target->hasKind(ExprNode::superExpr)) {
        bool foundQualifiers;
        member = parseParenthesesAndIdentifierQualifiers(t2, true, foundQualifiers, false);
        if (!foundQualifiers)
            kind = ExprNode::dotParen;
    } else
        member = parseQualifiedIdentifier(t2, preferRegExp);
    return new(arena) BinaryExprNode(pos, kind, target, member);
}


// Parse an ArgumentsList followed by a closing parenthesis or bracket and return the resulting InvokeExprNode.
// The target function, indexed object, or created class is supplied.  The opening parenthesis
// or bracket has already been read.  pos is the position to use for the generated node.
JS::InvokeExprNode *JS::Parser::parseInvoke(ExprNode *target, size_t pos, Token::Kind closingTokenKind, ExprNode::Kind invokeKind)
{
    NodeQueue<ExprPairList> arguments;
    bool hasNamedArgument = false;

    if (!lexer.eat(true, closingTokenKind))
        while (true) {
            ExprNode *field = 0;
            ExprNode *value = parseAssignmentExpression(false);
            if (lexer.eat(false, Token::colon)) {
                field = value;
                if (!(field->hasKind(ExprNode::identifier) ||
                      field->hasKind(ExprNode::number) ||
                      field->hasKind(ExprNode::string) ||
                      field->hasKind(ExprNode::parentheses) && !static_cast<UnaryExprNode *>(field)->op->hasKind(ExprNode::comma)))
                    syntaxError("Argument name must be an identifier, string, number, or parenthesized expression");
                hasNamedArgument = true;
                value = parseAssignmentExpression(false);
            } else if (hasNamedArgument)
                syntaxError("Unnamed argument cannot follow named argument", 0);
            arguments += new(arena) ExprPairList(field, value);

            const Token &tSeparator = lexer.get(false);
            if (tSeparator.hasKind(closingTokenKind))
                break;
            if (!tSeparator.hasKind(Token::comma))
                syntaxError("',' expected");
        }
    return new(arena) InvokeExprNode(pos, invokeKind, target, arguments.first);
}


// Given an alreaddy parsed PrimaryExpression, parse and return a PostfixExpression.
// If attribute is true, only allow AttributeExpression operators.
// If newExpression is true, the PrimaryExpression is immediately preceded by 'new',
// so don't allow call, postincrement, or postdecrement operators on it.
// If the first token was peeked, it should be have been done with preferRegExp set to
// the value of the attribute parameter.  After parsePostfixOperator finishes, the next token
// might have been peeked with preferRegExp set to the value of the attribute parameter.
JS::ExprNode *JS::Parser::parsePostfixOperator(ExprNode *e, bool newExpression, bool attribute)
{
    while (true) {
        ExprNode::Kind eKind;
        const Token &t = lexer.get(attribute);
        switch (t.getKind()) {
          case Token::openParenthesis:
            if (newExpression)
                goto other;
            e = parseInvoke(e, t.getPos(), Token::closeParenthesis, ExprNode::call);
            break;

          case Token::openBracket:
            e = parseInvoke(e, t.getPos(), Token::closeBracket, ExprNode::index);
            break;

          case Token::dot:
            e = parseMember(e, t, attribute);
            break;

          case Token::increment:
            eKind = ExprNode::postIncrement;
          incDec:
            if (newExpression || attribute || lineBreakBefore(t))
                goto other;
            e = new(arena) UnaryExprNode(t.getPos(), eKind, e);
            break;

          case Token::decrement:
            eKind = ExprNode::postDecrement;
            goto incDec;

          default:
          other:
            lexer.unget();
            return e;
        }
    }
}


// Parse and return a PostfixExpression.  If superState is ssExpr, also allow a SuperExpression;
// if superState is ssStmt, also allow a SuperExpression or a SuperStatement.
// If newExpression is true, this expression is immediately preceded by 'new',
// so don't allow call, postincrement, or postdecrement operators on it.
// If the first token was peeked, it should be have been done with preferRegExp set to true.
//
// After parsePostfixExpression finishes, the next token might have been peeked with preferRegExp
// set to false.
JS::ExprNode *JS::Parser::parsePostfixExpression(SuperState superState, bool newExpression)
{
    ExprNode *e;

    const Token *tNew = lexer.eat(true, Token::New);
    if (tNew) {
        checkStackSize();
        size_t posNew = tNew->getPos();
        e = parsePostfixExpression(ssExpr, true);
        if (lexer.eat(false, Token::openParenthesis))
            e = parseInvoke(e, posNew, Token::closeParenthesis, ExprNode::New);
        else
            e = new(arena) InvokeExprNode(posNew, ExprNode::New, e, 0);
    } else {
        e = parsePrimaryExpression(superState);
        if (e->hasKind(ExprNode::superStmt)) {
            ASSERT(superState == ssStmt);
            return e;
        }
    }

    return parsePostfixOperator(e, newExpression, false);
}


// Ensure that e is a PostfixExpression.  If not, throw a syntax error on
// the current token.
void JS::Parser::ensurePostfix(const ExprNode *e)
{
    ASSERT(e);
    if (!e->isPostfix())
        syntaxError("Only a postfix expression can be used as the target "
                    "of an assignment; enclose this expression in parentheses", 0);
}


// Parse and return an Attribute.  The first token has already been read into t.
//
// After parseAttribute finishes, the next token might have been peeked with preferRegExp
// set to true.
JS::ExprNode *JS::Parser::parseAttribute(const Token &t)
{
    ExprNode::Kind eKind;

    switch (t.getKind()) {
      case Token::True:
        eKind = ExprNode::True;
        goto makeExprNode;

      case Token::False:
        eKind = ExprNode::False;
      makeExprNode:
        return new(arena) ExprNode(t.getPos(), eKind);

      case Token::Public:
      case Token::Private:
        if (lexer.peek(true).hasKind(Token::doubleColon))
            break;
      case Token::Abstract:
      case Token::Final:
      case Token::Static:
      case Token::Volatile:
        return new(arena) IdentifierExprNode(t);

      case CASE_TOKEN_NONRESERVED:
        break;

      default:
        syntaxError("Attribute expected");
    }

    return parsePostfixOperator(parseQualifiedIdentifier(t, true), false, true);
}


// e is a parsed ListExpression or SuperStatement.  Return true if e is also an Attribute.
bool JS::Parser::expressionIsAttribute(const ExprNode *e)
{
    while (true)
        switch (e->getKind()) {
          case ExprNode::identifier:
          case ExprNode::True:
          case ExprNode::False:
            return true;

          case ExprNode::qualify:
            return static_cast<const QualifyExprNode *>(e)->qualifier->hasKind(ExprNode::identifier);

          case ExprNode::call:
          case ExprNode::index:
            e = static_cast<const InvokeExprNode *>(e)->op;
            break;

          case ExprNode::dot:
          case ExprNode::dotParen:
            e = static_cast<const BinaryExprNode *>(e)->op1;
            break;

          case ExprNode::dotClass:
            e = static_cast<const UnaryExprNode *>(e)->op;
            break;

          default:
            return false;
        }
}


// Parse and return a UnaryExpression.  If superState is ssExpr, also allow a SuperExpression;
// if superState is ssStmt, also allow a SuperExpression or a SuperStatement.
// If the first token was peeked, it should be have been done with preferRegExp set to true.
//
// After parseUnaryExpression finishes, the next token might have been peeked with preferRegExp
// set to false.
JS::ExprNode *JS::Parser::parseUnaryExpression(SuperState superState)
{
    ExprNode::Kind eKind;
    ExprNode *e;

    const Token &t = lexer.peek(true);
    size_t pos = t.getPos();
    switch (t.getKind()) {
      case Token::Const:
        eKind = ExprNode::Const;
        goto getPostfixExpression;

      case Token::Delete:
        eKind = ExprNode::Delete;
        goto getPostfixExpression;

      case Token::increment:
        eKind = ExprNode::preIncrement;
        goto getPostfixExpression;

      case Token::decrement:
        eKind = ExprNode::preDecrement;
      getPostfixExpression:
        lexer.get(true);
        e = parsePostfixExpression(ssExpr, false);
        break;

      case Token::Void:
        eKind = ExprNode::Void;
        goto getUnaryExpressionNotSuper;

      case Token::Typeof:
        eKind = ExprNode::Typeof;
        goto getUnaryExpressionNotSuper;

      case Token::plus:
        eKind = ExprNode::plus;
        goto getUnaryExpressionOrSuper;

      case Token::minus:
        eKind = ExprNode::minus;
        goto getUnaryExpressionOrSuper;

      case Token::complement:
        eKind = ExprNode::complement;
      getUnaryExpressionOrSuper:
        superState = ssExpr;
        goto getUnaryExpression;

      case Token::logicalNot:
        eKind = ExprNode::logicalNot;
      getUnaryExpressionNotSuper:
        superState = ssNone;
      getUnaryExpression:
        lexer.get(true);
        checkStackSize();
        e = parseUnaryExpression(superState);
        break;

      default:
        return parsePostfixExpression(superState, false);
    }
    return new(arena) UnaryExprNode(pos, eKind, e);
}


const JS::Parser::BinaryOperatorInfo JS::Parser::tokenBinaryOperatorInfos[Token::kindsEnd] = {
    // Special
    {ExprNode::none, pExpression, pNone, false},                    // Token::end
    {ExprNode::none, pExpression, pNone, false},                    // Token::number
    {ExprNode::none, pExpression, pNone, false},                    // Token::string
    {ExprNode::none, pExpression, pNone, false},                    // Token::unit
    {ExprNode::none, pExpression, pNone, false},                    // Token::regExp

    // Punctuators
    {ExprNode::none, pExpression, pNone, false},                    // Token::openParenthesis
    {ExprNode::none, pExpression, pNone, false},                    // Token::closeParenthesis
    {ExprNode::none, pExpression, pNone, false},                    // Token::openBracket
    {ExprNode::none, pExpression, pNone, false},                    // Token::closeBracket
    {ExprNode::none, pExpression, pNone, false},                    // Token::openBrace
    {ExprNode::none, pExpression, pNone, false},                    // Token::closeBrace
    {ExprNode::comma, pExpression, pExpression, false},             // Token::comma
    {ExprNode::none, pExpression, pNone, false},                    // Token::semicolon
    {ExprNode::none, pExpression, pNone, false},                    // Token::dot
    {ExprNode::none, pExpression, pNone, false},                    // Token::doubleDot
    {ExprNode::none, pExpression, pNone, false},                    // Token::tripleDot
    {ExprNode::none, pExpression, pNone, false},                    // Token::arrow
    {ExprNode::none, pExpression, pNone, false},                    // Token::colon
    {ExprNode::none, pExpression, pNone, false},                    // Token::doubleColon
    {ExprNode::none, pExpression, pNone, false},                    // Token::pound
    {ExprNode::none, pExpression, pNone, false},                    // Token::at
    {ExprNode::none, pExpression, pNone, false},                    // Token::increment
    {ExprNode::none, pExpression, pNone, false},                    // Token::decrement
    {ExprNode::none, pExpression, pNone, false},                    // Token::complement
    {ExprNode::none, pExpression, pNone, false},                    // Token::logicalNot
    {ExprNode::multiply, pMultiplicative, pMultiplicative, true},   // Token::times
    {ExprNode::divide, pMultiplicative, pMultiplicative, true},     // Token::divide
    {ExprNode::modulo, pMultiplicative, pMultiplicative, true},     // Token::modulo
    {ExprNode::add, pAdditive, pAdditive, true},                    // Token::plus
    {ExprNode::subtract, pAdditive, pAdditive, true},               // Token::minus
    {ExprNode::leftShift, pShift, pShift, true},                    // Token::leftShift
    {ExprNode::rightShift, pShift, pShift, true},                   // Token::rightShift
    {ExprNode::logicalRightShift, pShift, pShift, true},            // Token::logicalRightShift
    {ExprNode::logicalAnd, pBitwiseOr, pLogicalAnd, false},         // Token::logicalAnd (right-associative for efficiency)
    {ExprNode::logicalXor, pLogicalAnd, pLogicalXor, false},        // Token::logicalXor (right-associative for efficiency)
    {ExprNode::logicalOr, pLogicalXor, pLogicalOr, false},          // Token::logicalOr (right-associative for efficiency)
    {ExprNode::bitwiseAnd, pBitwiseAnd, pBitwiseAnd, true},         // Token::bitwiseAnd
    {ExprNode::bitwiseXor, pBitwiseXor, pBitwiseXor, true},         // Token::bitwiseXor
    {ExprNode::bitwiseOr, pBitwiseOr, pBitwiseOr, true},            // Token::bitwiseOr
    {ExprNode::assignment, pPostfix, pAssignment, false},           // Token::assignment
    {ExprNode::multiplyEquals, pPostfix, pAssignment, true},        // Token::timesEquals
    {ExprNode::divideEquals, pPostfix, pAssignment, true},          // Token::divideEquals
    {ExprNode::moduloEquals, pPostfix, pAssignment, true},          // Token::moduloEquals
    {ExprNode::addEquals, pPostfix, pAssignment, true},             // Token::plusEquals
    {ExprNode::subtractEquals, pPostfix, pAssignment, true},        // Token::minusEquals
    {ExprNode::leftShiftEquals, pPostfix, pAssignment, true},       // Token::leftShiftEquals
    {ExprNode::rightShiftEquals, pPostfix, pAssignment, true},      // Token::rightShiftEquals
    {ExprNode::logicalRightShiftEquals, pPostfix, pAssignment, true}, // Token::logicalRightShiftEquals
    {ExprNode::logicalAndEquals, pPostfix, pAssignment, false},     // Token::logicalAndEquals
    {ExprNode::logicalXorEquals, pPostfix, pAssignment, false},     // Token::logicalXorEquals
    {ExprNode::logicalOrEquals, pPostfix, pAssignment, false},      // Token::logicalOrEquals
    {ExprNode::bitwiseAndEquals, pPostfix, pAssignment, true},      // Token::bitwiseAndEquals
    {ExprNode::bitwiseXorEquals, pPostfix, pAssignment, true},      // Token::bitwiseXorEquals
    {ExprNode::bitwiseOrEquals, pPostfix, pAssignment, true},       // Token::bitwiseOrEquals
    {ExprNode::equal, pEquality, pEquality, true},                  // Token::equal
    {ExprNode::notEqual, pEquality, pEquality, true},               // Token::notEqual
    {ExprNode::lessThan, pRelational, pRelational, true},           // Token::lessThan
    {ExprNode::lessThanOrEqual, pRelational, pRelational, true},    // Token::lessThanOrEqual
    {ExprNode::greaterThan, pRelational, pRelational, true},        // Token::greaterThan
    {ExprNode::greaterThanOrEqual, pRelational, pRelational, true}, // Token::greaterThanOrEqual
    {ExprNode::identical, pEquality, pEquality, true},              // Token::identical
    {ExprNode::notIdentical, pEquality, pEquality, true},           // Token::notIdentical
    {ExprNode::conditional, pLogicalOr, pConditional, false},       // Token::question

    // Reserved words
    {ExprNode::none, pExpression, pNone, false},                    // Token::Abstract
    {ExprNode::In, pRelational, pRelational, false},                // Token::As
    {ExprNode::none, pExpression, pNone, false},                    // Token::Break
    {ExprNode::none, pExpression, pNone, false},                    // Token::Case
    {ExprNode::none, pExpression, pNone, false},                    // Token::Catch
    {ExprNode::none, pExpression, pNone, false},                    // Token::Class
    {ExprNode::none, pExpression, pNone, false},                    // Token::Const
    {ExprNode::none, pExpression, pNone, false},                    // Token::Continue
    {ExprNode::none, pExpression, pNone, false},                    // Token::Debugger
    {ExprNode::none, pExpression, pNone, false},                    // Token::Default
    {ExprNode::none, pExpression, pNone, false},                    // Token::Delete
    {ExprNode::none, pExpression, pNone, false},                    // Token::Do
    {ExprNode::none, pExpression, pNone, false},                    // Token::Else
    {ExprNode::none, pExpression, pNone, false},                    // Token::Enum
    {ExprNode::none, pExpression, pNone, false},                    // Token::Export
    {ExprNode::none, pExpression, pNone, false},                    // Token::Extends
    {ExprNode::none, pExpression, pNone, false},                    // Token::False
    {ExprNode::none, pExpression, pNone, false},                    // Token::Final
    {ExprNode::none, pExpression, pNone, false},                    // Token::Finally
    {ExprNode::none, pExpression, pNone, false},                    // Token::For
    {ExprNode::none, pExpression, pNone, false},                    // Token::Function
    {ExprNode::none, pExpression, pNone, false},                    // Token::Goto
    {ExprNode::none, pExpression, pNone, false},                    // Token::If
    {ExprNode::none, pExpression, pNone, false},                    // Token::Implements
    {ExprNode::none, pExpression, pNone, false},                    // Token::Import
    {ExprNode::In, pRelational, pRelational, false},                // Token::In
    {ExprNode::Instanceof, pRelational, pRelational, false},        // Token::Instanceof
    {ExprNode::none, pExpression, pNone, false},                    // Token::Interface
    {ExprNode::Is, pRelational, pRelational, false},                // Token::Is
    {ExprNode::none, pExpression, pNone, false},                    // Token::Namespace
    {ExprNode::none, pExpression, pNone, false},                    // Token::Native
    {ExprNode::none, pExpression, pNone, false},                    // Token::New
    {ExprNode::none, pExpression, pNone, false},                    // Token::Null
    {ExprNode::none, pExpression, pNone, false},                    // Token::Package
    {ExprNode::none, pExpression, pNone, false},                    // Token::Private
    {ExprNode::none, pExpression, pNone, false},                    // Token::Protected
    {ExprNode::none, pExpression, pNone, false},                    // Token::Public
    {ExprNode::none, pExpression, pNone, false},                    // Token::Return
    {ExprNode::none, pExpression, pNone, false},                    // Token::Static
    {ExprNode::none, pExpression, pNone, false},                    // Token::Super
    {ExprNode::none, pExpression, pNone, false},                    // Token::Switch
    {ExprNode::none, pExpression, pNone, false},                    // Token::Synchronized
    {ExprNode::none, pExpression, pNone, false},                    // Token::This
    {ExprNode::none, pExpression, pNone, false},                    // Token::Throw
    {ExprNode::none, pExpression, pNone, false},                    // Token::Throws
    {ExprNode::none, pExpression, pNone, false},                    // Token::Transient
    {ExprNode::none, pExpression, pNone, false},                    // Token::True
    {ExprNode::none, pExpression, pNone, false},                    // Token::Try
    {ExprNode::none, pExpression, pNone, false},                    // Token::Typeof
    {ExprNode::none, pExpression, pNone, false},                    // Token::Use
    {ExprNode::none, pExpression, pNone, false},                    // Token::Var
    {ExprNode::none, pExpression, pNone, false},                    // Token::Void
    {ExprNode::none, pExpression, pNone, false},                    // Token::Volatile
    {ExprNode::none, pExpression, pNone, false},                    // Token::While
    {ExprNode::none, pExpression, pNone, false},                    // Token::With

    // Non-reserved words
    {ExprNode::none, pExpression, pNone, false},                    // Token::Eval
    {ExprNode::none, pExpression, pNone, false},                    // Token::Exclude
    {ExprNode::none, pExpression, pNone, false},                    // Token::Get
    {ExprNode::none, pExpression, pNone, false},                    // Token::Include
    {ExprNode::none, pExpression, pNone, false},                    // Token::Set

    {ExprNode::none, pExpression, pNone, false}                     // Token::identifier
};

struct JS::Parser::StackedSubexpression {
    ExprNode::Kind kind;    // The kind of BinaryExprNode the subexpression should generate
    uchar precedence;       // Precedence of an operator with respect to operators on its right
    bool superRight;        // True if the right operand can be super
    size_t pos;             // The operator token's position
    ExprNode *op1;          // First operand of the operator
    ExprNode *op2;          // Second operand of the operator (used for ?: only)
};


// Parse and return a ListExpression.  If allowSuperStmt is true, also allow the expression to be a
// SuperStatement.
// If noAssignment is false, allow the = and op= operators.  If noComma is false, allow the comma
// operator.  If the first token was peeked, it should have been done with preferRegExp set to true.
//
// After parseGeneralExpression finishes, the next token might have been peeked with preferRegExp
// set to false.
JS::ExprNode *JS::Parser::parseGeneralExpression(bool allowSuperStmt, bool noIn, bool noAssignment, bool noComma)
{
    ArrayBuffer<StackedSubexpression, 10> subexpressionStack;

    checkStackSize();
    // Push a limiter onto subexpressionStack.
    subexpressionStack.reserve_advance_back()->precedence = pNone;

    ExprNode *e = parseUnaryExpression(allowSuperStmt ? ssStmt : ssExpr);
    if (e->hasKind(ExprNode::superStmt)) {
        ASSERT(allowSuperStmt);
        return e;
    }
    while (true) {
        const Token &t = lexer.peek(false);
        const BinaryOperatorInfo &binOpInfo = tokenBinaryOperatorInfos[t.getKind()];
        Precedence precedence = binOpInfo.precedenceLeft;
        ExprNode::Kind kind = binOpInfo.kind;
        ASSERT(precedence > pNone);

        // Disqualify assignments, 'in', and comma if the flags indicate that these should end the expression.
        if (precedence == pPostfix && noAssignment || kind == ExprNode::In && noIn || kind == ExprNode::comma && noComma) {
            kind = ExprNode::none;
            precedence = pExpression;
        }

        if (precedence == pPostfix) {
            // Ensure that the target of an assignment is a postfix subexpression or, where permitted,
            // a super subexpression.
            if (!(binOpInfo.superLeft && e->hasKind(ExprNode::superExpr)))
                ensurePostfix(e);
        } else
            // Reduce already stacked operators with precedenceLeft or higher precedence
            while (subexpressionStack.back().precedence >= precedence) {
                StackedSubexpression &s = subexpressionStack.pop_back();
                if (e->hasKind(ExprNode::superExpr) && !s.superRight)
                    syntaxError("super expression not allowed here", 0);
                if (s.kind == ExprNode::conditional) {
                    if (s.op2)
                        e = new(arena) TernaryExprNode(s.pos, s.kind, s.op1, s.op2, e);
                    else {
                        if (!t.hasKind(Token::colon))
                            syntaxError("':' expected", 0);
                        lexer.get(false);
                        subexpressionStack.advance_back();
                        s.op2 = e;
                        goto foundColon;
                    }
                } else
                    e = new(arena) BinaryExprNode(s.pos, s.kind, s.op1, e);
            }

        if (kind == ExprNode::none)
            break;

        // Push the current operator onto the subexpressionStack.
        lexer.get(false);
        {
            StackedSubexpression &s = *subexpressionStack.reserve_advance_back();
            bool superLeft = binOpInfo.superLeft;
            if (e->hasKind(ExprNode::superExpr) && !superLeft)
                syntaxError("super expression not allowed here", 1);
            s.kind = kind;
            s.precedence = binOpInfo.precedenceRight;
            s.superRight = superLeft || s.kind == ExprNode::In;
            s.pos = t.getPos();
            s.op1 = e;
            s.op2 = 0;
        }
      foundColon:
        e = parseUnaryExpression(ssExpr);
    }

    ASSERT(subexpressionStack.size() == 1);
    if (e->hasKind(ExprNode::superExpr))
        if (allowSuperStmt && static_cast<SuperExprNode *>(e)->op) {
            // Convert the superExpr into a superStmt.
            ExprPairList *arg = new(arena) ExprPairList(0, static_cast<SuperExprNode *>(e)->op);
            e = new(arena) InvokeExprNode(e->pos, ExprNode::superStmt, 0, arg);
        } else
            syntaxError("super expression not allowed here", 0);
    return e;
}


// Parse an opening parenthesis, a ListExpression, and a closing parenthesis.
// Return the ListExpression. If the first token was peeked, it should be have
// been done with preferRegExp set to true.
JS::ExprNode *JS::Parser::parseParenthesizedListExpression()
{
    require(true, Token::openParenthesis);
    ExprNode *e = parseListExpression(false);
    require(false, Token::closeParenthesis);
    return e;
}


// Parse and return a TypeExpression.  If noIn is false, allow the in operator.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseTypeExpression finishes, the next token might have been peeked with preferRegExp set to true.
JS::ExprNode *JS::Parser::parseTypeExpression(bool noIn)
{
    ExprNode *type = parseNonAssignmentExpression(noIn);
    if (lexer.peek(false).hasKind(Token::divideEquals))
        syntaxError("'/=' not allowed here", 0);
    lexer.redesignate(true);    // Safe: a '/' would have been interpreted as an operator, so it can't be the next
                                // token; a '/=' was outlawed by the check above.
    return type;
}


// If the next token has the given kind, eat it and parse and return the
// following TypeExpression; otherwise return nil.
// If noIn is false, allow the in operator.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseTypeBinding finishes, the next token might have been peeked with preferRegExp set to true.
JS::ExprNode *JS::Parser::parseTypeBinding(Token::Kind kind, bool noIn)
{
    ExprNode *type = 0;
    if (lexer.eat(true, kind))
        type = parseTypeExpression(noIn);
    return type;
}


// ***** DEAD CODE *****
// If the next token has the given kind, eat it and parse and return the
// following TypeExpressionList; otherwise return nil.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseTypeListBinding finishes, the next token might have been peeked with preferRegExp set to true.
JS::ExprList *JS::Parser::parseTypeListBinding(Token::Kind kind)
{
    NodeQueue<ExprList> types;
    if (lexer.eat(true, kind))
        do types += new(arena) ExprList(parseTypeExpression(false));
        while (lexer.eat(true, Token::comma));
    return types.first;
}


// Parse and return a VariableBinding.  pos is the position of the binding or its first attribute, if any.
// If noIn is false, allow the in operator.  The value of the constant parameter is stored in the returned
// VariableBinding.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseVariableBinding finishes, the next token might have been peeked with preferRegExp set to true.
// The reason preferRegExp is true is to correctly parse the following case of semicolon insertion:
//    var a
//    /regexp/
JS::VariableBinding *JS::Parser::parseVariableBinding(size_t pos, bool noIn, bool constant)
{
    const StringAtom &name = parseIdentifier();
    ExprNode *type = parseTypeBinding(Token::colon, noIn);
    ExprNode *initializer = 0;
    if (lexer.eat(true, Token::assignment)) {
        initializer = parseAssignmentExpression(noIn);
        lexer.redesignate(true);  // Safe: a '/' or a '/=' would have been interpreted as an operator, so
                                  // it can't be the next token.
    }

    return new(arena) VariableBinding(pos, &name, type, initializer, constant);
}


// Parse and return a VariableBinding for a function parameter.  The parameter may optionally be
// preceded by the const attribute.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseParameter finishes, the next token might have been peeked with preferRegExp set to true.
JS::VariableBinding *JS::Parser::parseParameter()
{
    const Token &t = lexer.peek(true);
    size_t pos = t.getPos();
    bool constant = false;
    if (t.hasKind(Token::Const)) {
        lexer.get(true);
        constant = true;
    }
    return parseVariableBinding(pos, false, constant);
}


// Parse a FunctionName and initialize fn with the result.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseFunctionName finishes, the next token might have been peeked with preferRegExp set to true.
void JS::Parser::parseFunctionName(FunctionName &fn)
{
    fn.prefix = FunctionName::normal;
    const Token *t = &lexer.get(true);
    ExprNode *name;

    if (t->hasKind(Token::string))
        name = new(arena) StringExprNode(t->getPos(), ExprNode::string, copyTokenChars(*t));
    else {
        if (t->hasKind(Token::Get) || t->hasKind(Token::Set)) {
            const Token *t2 = &lexer.peek(true);
            if (!lineBreakBefore(*t2) && t2->getFlag(Token::isNonreserved)) {
                fn.prefix = t->hasKind(Token::Get) ? FunctionName::Get : FunctionName::Set;
                t = &lexer.get(true);
            }
        }
        name = makeIdentifierExpression(*t);
        if (!name)
            syntaxError("Identifier expected");
    }
    fn.name = name;
}


// Parse a FunctionSignature and initialize fd with the result.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseFunctionSignature finishes, the next token might have been
// peeked with preferRegExp set to true.
void JS::Parser::parseFunctionSignature(FunctionDefinition &fd)
{
    require(true, Token::openParenthesis);

    NodeQueue<VariableBinding> parameters;
    VariableBinding *optParameters = 0;
    VariableBinding *restParameter = 0;
    if (!lexer.eat(true, Token::closeParenthesis)) {
        while (true) {
            if (lexer.eat(true, Token::tripleDot)) {
                const Token &t1 = lexer.peek(true);
                if (t1.hasKind(Token::closeParenthesis))
                    restParameter = new(arena) VariableBinding(t1.getPos(), 0, 0, 0, false);
                else
                    restParameter = parseParameter();
                if (!optParameters)
                    optParameters = restParameter;
                parameters += restParameter;
                require(true, Token::closeParenthesis);
                break;
            } else {
                VariableBinding *b = parseParameter();
                if (b->initializer) {
                    if (!optParameters)
                        optParameters = b;
                } else
                    if (optParameters)
                        syntaxError("'=' expected", 0);
                parameters += b;
                const Token &t = lexer.get(true);
                if (!t.hasKind(Token::comma))
                    if (t.hasKind(Token::closeParenthesis))
                        break;
                    else
                        syntaxError("',' or ')' expected");
            }
        }
    }
    fd.parameters = parameters.first;
    fd.optParameters = optParameters;
    fd.restParameter = restParameter;
    fd.resultType = parseTypeBinding(Token::colon, false);
}


// Parse a list of Directives ending with a '}'.  Return these directives as a
// linked list threaded through the StmtNodes' next fields.  The opening '{'
// has already been read.  If noCloseBrace is true, an end-of-input terminates
// the block; the end-of-input token is not read.
// If inSwitch is true, only allow case <expr>: and default: statements or Substatements.
// If noCloseBrace is true, after parseBlockContents finishes the next token might
// have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseBlockContents(bool inSwitch, bool noCloseBrace)
{
    NodeQueue<StmtNode> q;
    bool semicolonWanted = false;

    while (true) {
        const Token *t = &lexer.peek(true);
        if (t->hasKind(Token::semicolon) && semicolonWanted) {
            lexer.get(true);
            semicolonWanted = false;
            t = &lexer.peek(true);
        }
        if (noCloseBrace) {
            if (t->hasKind(Token::end))
                return q.first;
        } else if (t->hasKind(Token::closeBrace)) {
            lexer.get(true);
            return q.first;
        }
        if (semicolonWanted && !lineBreakBefore(*t))
            syntaxError("';' expected", 0);
        if (inSwitch && !q.first && !(t->hasKind(Token::Case) || t->hasKind(Token::Default)))
            syntaxError("First statement in a switch block must be 'case expr:' or 'default:'", 0);

        StmtNode *s = parseDirective(inSwitch, inSwitch, semicolonWanted);
        q += s;
    }
}


// Parse an optional block of Directives beginning with a '{' and ending with a '}'.
// Return these directives as a BlockStmtNode.
// If semicolonWanted is nil, the block is required; otherwise, the block is
// optional and if it is omitted, *semicolonWanted is set to true.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseBody finishes, the next token might have been peeked with preferRegExp set to true.
JS::BlockStmtNode *JS::Parser::parseBody(bool *semicolonWanted)
{
    const Token *tBrace = lexer.eat(true, Token::openBrace);
    if (tBrace) {
        size_t pos = tBrace->getPos();
        return new(arena) BlockStmtNode(pos, StmtNode::block, 0, parseBlockContents(false, false));
    } else {
        if (!semicolonWanted)
            syntaxError("'{' expected", 0);
        *semicolonWanted = true;
        return 0;
    }
}


// Parse and return a Directive that takes zero or more initial attributes, which is either
// a Definition or an AnnotatedBlock.  The initial attributes have already been parsed.
// If there were no attributes, the attributes parameter is nil.
// If noIn is false, allow the in operator.
//
// If the directive ends with an optional Semicolon production, then that semicolon is not parsed.
// Instead, parseDefinition returns true in semicolonWanted when either a semicolon, a line break (in
// non-strict mode), or some other closer such as a '}', 'else', or 'while' of a do-while is needed to
// end the directive.
//
// pos is the position of the beginning of the directive (its first attribute if it has attributes).
// The first token of the directive has already been read and is provided in t.
// After parseDefinition finishes, the next token might have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseDefinition(size_t pos, ExprNode *attributes, const Token &t, bool noIn, bool &semicolonWanted)
{
    semicolonWanted = false;
    StmtNode::Kind sKind;

    switch (t.getKind()) {
      case Token::openBrace:
        return new(arena) BlockStmtNode(pos, StmtNode::block, attributes, parseBlockContents(false, false));

      case Token::Const:
        sKind = StmtNode::Const;
        goto constOrVar;
      case Token::Var:
        sKind = StmtNode::Var;
      constOrVar:
        {
            NodeQueue<VariableBinding> bindings;

            do bindings += parseVariableBinding(lexer.peek(true).getPos(), noIn, sKind == StmtNode::Const);
            while (lexer.eat(true, Token::comma));
            semicolonWanted = true;
            return new(arena) VariableStmtNode(pos, sKind, attributes, bindings.first);
        }

      case Token::Function:
        {
            FunctionStmtNode *f = new(arena) FunctionStmtNode(pos, StmtNode::Function, attributes);
            parseFunctionName(f->function);
            parseFunctionSignature(f->function);
            f->function.body = parseBody(&semicolonWanted);
            return f;
        }

      case Token::Class:
        {
            const StringAtom &name = parseIdentifier();
            ExprNode *superclass = parseTypeBinding(Token::Extends, false);
            BlockStmtNode *body = parseBody(superclass ? 0 : &semicolonWanted);
            return new(arena) ClassStmtNode(pos, attributes, name, superclass, body);
        }

      case Token::Namespace:
        semicolonWanted = true;
        return new(arena) NamespaceStmtNode(pos, StmtNode::Namespace, attributes, parseIdentifier());

      default:
        syntaxError("Bad declaration");
        return 0;
    }
}


// Parse and return a Directive that takes initial attributes.
// semicolonWanted behaves as in parseDefinition.
// as restricts the kinds of definitions that are allowed after the attributes:
//   asAny      Any definitions, including blocks, that take attributes can follow
//   asBlock    Only a block can follow
//   asConstVar Only a const or var definition can follow, and the 'in'
//              operator is not allowed at its top level
//
// The first attribute has already been read and is provided in the attribute parameter.
// pos is the position of the first attribute.
// If the next token was peeked, it should be have been done with preferRegExp set to true.
// After parseAttributesAndDefinition finishes, the next token might have been peeked with
// preferRegExp set to true.
JS::StmtNode *JS::Parser::parseAttributesAndDefinition(size_t pos, ExprNode *attribute, AttributeStatement as, bool &semicolonWanted)
{
    ASSERT(attribute);
    while (true) {
        const Token &t = lexer.get(true);
        if (lineBreakBefore(t))
            syntaxError("Line break not allowed here");

        if (!t.getFlag(Token::isAttribute)) {
            switch (as) {
              case asAny:
                break;

              case asBlock:     // ***** This is dead code.
                if (!t.hasKind(Token::openBrace))
                    syntaxError("'{' expected");
                break;

              case asConstVar:
                if (!t.hasKind(Token::Const) && !t.hasKind(Token::Var))
                    syntaxError("const or var expected");
                break;
            }
            return parseDefinition(pos, attribute, t, as == asConstVar, semicolonWanted);
        }
        attribute = new(arena) BinaryExprNode(pos, ExprNode::juxtapose, attribute, parseAttribute(t));
    }
}


// Parse and return a ForStatement.  The 'for' token has already been read; its position is pos.
// If the statement ends with an optional semicolon, that semicolon might not be parsed.
// Instead, parseFor returns a semicolonWanted with the same meaning as that in parseDirective.
//
// After parseFor finishes, the next token might have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseFor(size_t pos, bool &semicolonWanted)
{
    require(true, Token::openParenthesis);
    const Token &t = lexer.get(true);
    size_t tPos = t.getPos();
    StmtNode *initializer = 0;
    ExprNode *expr1 = 0;
    ExprNode *expr2 = 0;
    ExprNode *expr3 = 0;
    StmtNode::Kind sKind = StmtNode::For;

    switch (t.getKind()) {
      case Token::semicolon:
        goto threeExpr;

      case Token::Const:
      case Token::Var:
        initializer = parseDefinition(tPos, 0, t, true, semicolonWanted);
        break;

      case Token::Abstract:
      case Token::Final:
      case Token::Static:
      case Token::Volatile:
        // Token::Private, Token::Public, Token::True, Token::False, and other attributes are
        // handled by the default case below.
        expr1 = new(arena) IdentifierExprNode(t);
      makeAttribute:
        initializer = parseAttributesAndDefinition(tPos, expr1, asConstVar, semicolonWanted);
        expr1 = 0;
        break;

      default:
        lexer.unget();
        expr1 = parseListExpression(true);
        lexer.redesignate(true); // Safe: a '/' or a '/=' would have been interpreted as an operator,
                                 // so it can't be the next token.
        if (expressionIsAttribute(expr1)) {
            const Token &t2 = lexer.peek(true);
            if (!lineBreakBefore(t2) && t2.getFlag(Token::canFollowAttribute))
                goto makeAttribute;
        }
        initializer = new(arena) ExprStmtNode(tPos, StmtNode::expression, expr1);
        break;
    }

    if (lexer.eat(true, Token::semicolon))
      threeExpr: {
        if (!lexer.eat(true, Token::semicolon)) {
            expr2 = parseListExpression(false);
            require(false, Token::semicolon);
        }
        if (lexer.peek(true).hasKind(Token::closeParenthesis))
            lexer.redesignate(false);   // Safe: the token is ')'.
        else
            expr3 = parseListExpression(false);
    }
    else if (lexer.eat(true, Token::In)) {
        sKind = StmtNode::ForIn;
        if (expr1) {
            ASSERT(initializer->hasKind(StmtNode::expression));
            ensurePostfix(expr1);
        } else {
            ASSERT(initializer->hasKind(StmtNode::Const) || initializer->hasKind(StmtNode::Var));
            const VariableBinding *bindings = static_cast<VariableStmtNode *>(initializer)->bindings;
            if (!bindings || bindings->next)
                syntaxError("Only one variable binding can be used in a for-in statement", 0);
        }
        expr2 = parseListExpression(false);
    }
    else
        syntaxError("';' or 'in' expected", 0);

    require(false, Token::closeParenthesis);
    return new(arena) ForStmtNode(pos, sKind, initializer, expr2, expr3, parseDirective(true, false, semicolonWanted));
}


// Parse and return a TryStatement.  The 'try' token has already been read; its position is pos.
// After parseTry finishes, the next token might have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseTry(size_t pos)
{
    StmtNode *tryBlock = parseBody(0);
    NodeQueue<CatchClause> catches;
    const Token *t;

    while ((t = lexer.eat(true, Token::Catch)) != 0) {
        size_t catchPos = t->getPos();
        require(true, Token::openParenthesis);
        bool constant = lexer.eat(true, Token::Const) != 0;
        const StringAtom &name = parseIdentifier();
        ExprNode *type = parseTypeBinding(Token::colon, true);
        require(true, Token::closeParenthesis);
        catches += new(arena) CatchClause(catchPos, name, type, constant, parseBody(0));
    }
    StmtNode *finally = 0;
    if (lexer.eat(true, Token::Finally))
        finally = parseBody(0);
    else if (!catches.first)
        syntaxError("A try statement must be followed by at least one catch or finally", 0);

    return new(arena) TryStmtNode(pos, tryBlock, catches.first, finally);
}


// Parse and return a Directive (if substatement is false) or a Substatement (if substatement is true).
// If inSwitch is true, allow case <expr>: and default: statements.
//
// If the Directive ends with an optional Semicolon production, parseDirective may or may not read a
// semicolon if one is present.  If it does not read a semicolon, parseDirective will set semicolonWanted
// to indicate to the outer parser that either a semicolon, a line break (in non-strict mode), or some other
// closer such as a '}', 'else', or 'while' of a do-while is needed to end the directive.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseDirective finishes, the next token might have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseDirective(bool substatement, bool inSwitch, bool &semicolonWanted)
{
    StmtNode *s;
    ExprNode *e = 0;
    StmtNode::Kind sKind;
    const Token &t = lexer.get(true);
    const Token *t2;
    size_t pos = t.getPos();
    semicolonWanted = false;

    checkStackSize();
    switch (t.getKind()) {
      case Token::semicolon:
        s = new(arena) StmtNode(pos, StmtNode::empty);
        break;

      case Token::openBrace:
      case Token::Import:
      case Token::Export:
      case Token::Const:
      case Token::Var:
      case Token::Function:
      case Token::Class:
      case Token::Namespace:
        s = parseDefinition(pos, 0, t, false, semicolonWanted);
        break;

      case Token::If:
        e = parseParenthesizedListExpression();
        s = parseSubstatement(semicolonWanted);
        if (lexer.eat(true, Token::Else))
            s = new(arena) BinaryStmtNode(pos, StmtNode::IfElse, e, s, parseDirective(true, false, semicolonWanted));
        else {
            sKind = StmtNode::If;
            goto makeUnary;
        }
        break;

      case Token::Switch:
        e = parseParenthesizedListExpression();
        require(true, Token::openBrace);
        s = new(arena) SwitchStmtNode(pos, e, parseBlockContents(true, false));
        break;

      case Token::Case:
        if (!inSwitch)
            goto notInSwitch;
        e = parseListExpression(false);
      makeSwitchCase:
        require(false, Token::colon);
        s = new(arena) ExprStmtNode(pos, StmtNode::Case, e);
        break;

      case Token::Default:
        if (inSwitch)
            goto makeSwitchCase;
      notInSwitch:
        syntaxError("case and default may only be used inside a switch statement");
        break;

      case Token::Do:
        {
            bool semicolonWanted2;      // Ignore semicolonWanted2.
            s = parseSubstatement(semicolonWanted2);
            require(true, Token::While);
            e = parseParenthesizedListExpression();
            sKind = StmtNode::DoWhile;
            goto makeUnary;
        }
        break;

      case Token::With:
        sKind = StmtNode::With;
        goto makeWhileWith;

      case Token::While:
        sKind = StmtNode::While;
      makeWhileWith:
        e = parseParenthesizedListExpression();
        s = parseDirective(true, false, semicolonWanted);
      makeUnary:
        s = new(arena) UnaryStmtNode(pos, sKind, e, s);
        break;

      case Token::For:
        s = parseFor(pos, semicolonWanted);
        break;

      case Token::Continue:
        sKind = StmtNode::Continue;
        goto makeGo;

      case Token::Break:
        sKind = StmtNode::Break;
      makeGo:
        {
            const StringAtom *label = 0;
            t2 = &lexer.peek(true);
            if (t2->hasKind(Token::identifier) && !lineBreakBefore(*t2)) {
                lexer.get(true);
                label = &t2->getIdentifier();
            }
            s = new(arena) GoStmtNode(pos, sKind, label);
        }
        goto insertableSemicolon;

      case Token::Return:
        sKind = StmtNode::Return;
        t2 = &lexer.peek(true);
        if (lineBreakBefore(*t2) || t2->getFlag(Token::canFollowReturn))
            goto makeExprStmtNode;
      makeExpressionNode:
        e = parseListExpression(false);
        // Safe: a '/' or a '/=' would have been interpreted as an
        // operator, so it can't be the next token.
        lexer.redesignate(true);
        goto makeExprStmtNode;

      case Token::Throw:
        sKind = StmtNode::Throw;
        if (lineBreakBefore())
            syntaxError("throw cannot be followed by a line break", 0);
        goto makeExpressionNode;

      case Token::Try:
        s = parseTry(pos);
        break;

      case Token::Debugger:
        s = new(arena) DebuggerStmtNode(pos, StmtNode::Debugger);
        break;

      case Token::Abstract:
      case Token::Final:
      case Token::Static:
      case Token::Volatile:
        e = new(arena) IdentifierExprNode(t);
      makeAttribute:
        s = parseAttributesAndDefinition(pos, e, asAny, semicolonWanted);
        break;

      case CASE_TOKEN_NONRESERVED:
        t2 = &lexer.peek(false);
        if (t2->hasKind(Token::colon)) {
            lexer.get(false);
            // Must do this now because parseDirective can invalidate t.
            const StringAtom &name = t.getIdentifier();
            s = new(arena) LabelStmtNode(pos, name, parseDirective(true, false, semicolonWanted));
            break;
        }
      default:
        lexer.unget();
        e = parseGeneralExpression(true, false, false, false);
        // Safe: a '/' or a '/=' would have been interpreted as an operator, so it can't be the next token.
        lexer.redesignate(true);
        if (expressionIsAttribute(e)) {
            t2 = &lexer.peek(true);
            if (!lineBreakBefore(*t2) && t2->getFlag(Token::canFollowAttribute))
                goto makeAttribute;
        }
        sKind = StmtNode::expression;
      makeExprStmtNode:
        s = new(arena) ExprStmtNode(pos, sKind, e);
      insertableSemicolon:
        semicolonWanted = true;
        break;
    }
    return s;
}


// Parse and return a Substatement.  If the substatement ends with an optional semicolon,
// that semicolon *is* parsed.  parseSubstatement returns true in semicolonWanted only if the
// a Semicolon is needed to close the substatement but one wasn't found.  In other cases
// semicolonWanted is set to false.
//
// If the first token was peeked, it should be have been done with preferRegExp set to true.
// After parseSubstatement finishes, the next token might have been peeked with preferRegExp set to true.
JS::StmtNode *JS::Parser::parseSubstatement(bool &semicolonWanted)
{
    StmtNode *s = parseDirective(true, false, semicolonWanted);
    if (semicolonWanted && lexer.eat(true, Token::semicolon))
        semicolonWanted = false;
    return s;
}


//
// Parser Utilities
//

const bool debugExprNodePrint = true;   // Print extra parentheses around subexpressions?
const int32 basicIndent = 4;            // Size of one level of statement indentation
const int32 caseIndent = basicIndent/2; // Indentation before a case or default statement
const int32 varIndent = 2;              // Indentation of var or const statement bindings
const int32 subexpressionIndent = 4;    // Size of one level of expression indentation
const int32 functionHeaderIndent = 9;   // Indentation of function signature
const int32 namespaceHeaderIndent = 4;  // Indentation of class, interface, or namespace header

static const char functionPrefixNames[3][5] = {"", "get ", "set "};


// Print this onto f.  name must be non-nil.
void JS::FunctionName::print(PrettyPrinter &f) const
{
    f << functionPrefixNames[prefix];
    f << name;
}


// Print this onto f.  if printConst is false, inhibit printing of the const keyword.
void JS::VariableBinding::print(PrettyPrinter &f, bool printConst) const
{
    PrettyPrinter::Block b(f);

    if (printConst && constant)
        f << "const ";
    if (name)
        f << *name;
    PrettyPrinter::Indent i(f, subexpressionIndent);
    if (type) {
        f.fillBreak(0);
        f << ": ";
        f << type;
    }
    if (initializer) {
        f.linearBreak(1);
        f << "= ";
        f << initializer;
    }
}


// Print this onto f.  If attributes is null, this is a function expression;
// if attributes is non-null, this is a function statement with the given attributes.
// When there is no function body, print a trailing semicolon unless noSemi is true.
void JS::FunctionDefinition::print(PrettyPrinter &f, const AttributeStmtNode *attributes, bool noSemi) const
{
    PrettyPrinter::Block b(f);
    if (attributes)
        attributes->printAttributes(f);

    f << "function";

    if (name) {
        f << ' ';
        FunctionName::print(f);
    }
    {
        PrettyPrinter::Indent i(f, functionHeaderIndent);
        f.fillBreak(0);
        f << '(';
        {
            PrettyPrinter::Block b2(f);
            const VariableBinding *p = parameters;
            if (p)
                while (true) {
                    if (p == restParameter) {
                        f << "...";
                        if (p->name)
                            f << ' ';
                    }
                    p->print(f, true);
                    p = p->next;
                    if (!p)
                        break;
                    f << ',';
                    f.fillBreak(1);
                }
            f << ')';
        }
        if (resultType) {
            f.fillBreak(0);
            f << ": ";
            f << resultType;
        }
    }
    if (body) {
        bool loose = attributes != 0;
        f.linearBreak(1, loose);
        body->printBlock(f, loose);
    } else
        StmtNode::printSemi(f, noSemi);
}


//
// ExprNode
//

const char *const JS::ExprNode::kindNames[kindsEnd] = {
    "NIL",                  // none
    0,                      // identifier
    0,                      // number
    0,                      // string
    0,                      // regExp
    "null",                 // Null
    "true",                 // True
    "false",                // False
    "this",                 // This

    0,                      // parentheses
    0,                      // numUnit
    0,                      // exprUnit
    0,                      // qualify

    0,                      // objectLiteral
    0,                      // arrayLiteral
    0,                      // functionLiteral

    0,                      // call
    0,                      // New
    0,                      // index

    ".",                    // dot
    ".class",               // dotClass
    ".(",                   // dotParen

    0,                      // superExpr
    0,                      // superStmt

    "const ",               // Const
    "delete ",              // Delete
    "void ",                // Void
    "typeof ",              // Typeof
    "++ ",                  // preIncrement
    "-- ",                  // preDecrement
    " ++",                  // postIncrement
    " --",                  // postDecrement
    "+ ",                   // plus
    "- ",                   // minus
    "~ ",                   // complement
    "! ",                   // logicalNot

    " ",                    // juxtapose
    "+",                    // add
    "-",                    // subtract
    "*",                    // multiply
    "/",                    // divide
    "%",                    // modulo
    "<<",                   // leftShift
    ">>",                   // rightShift
    ">>>",                  // logicalRightShift
    "&",                    // bitwiseAnd
    "^",                    // bitwiseXor
    "|",                    // bitwiseOr
    "&&",                   // logicalAnd
    "^^",                   // logicalXor
    "||",                   // logicalOr

    "==",                   // equal
    "!=",                   // notEqual
    "<",                    // lessThan
    "<=",                   // lessThanOrEqual
    ">",                    // greaterThan
    ">=",                   // greaterThanOrEqual
    "===",                  // identical
    "!==",                  // notIdentical
    "as",                   // As
    "in",                   // In
    "instanceof",           // Instanceof
    "is",                   // Is

    "=",                    // assignment
    "+=",                   // addEquals
    "-=",                   // subtractEquals
    "*=",                   // multiplyEquals
    "/=",                   // divideEquals
    "%=",                   // moduloEquals
    "<<=",                  // leftShiftEquals
    ">>=",                  // rightShiftEquals
    ">>>=",                 // logicalRightShiftEquals
    "&=",                   // bitwiseAndEquals
    "^=",                   // bitwiseXorEquals
    "|=",                   // bitwiseOrEquals
    "&&=",                  // logicalAndEquals
    "^^=",                  // logicalXorEquals
    "||=",                  // logicalOrEquals

    "?",                    // conditional
    ","                     // comma
};


// Print this onto f.
void JS::ExprNode::print(PrettyPrinter &f) const
{
    f << kindName(kind);
}

void JS::IdentifierExprNode::print(PrettyPrinter &f) const
{
    f << name;
}

void JS::QualifyExprNode::print(PrettyPrinter &f) const
{
    f << qualifier << "::" << name;
}

void JS::NumberExprNode::print(PrettyPrinter &f) const
{
    f << value;
}

void JS::StringExprNode::print(PrettyPrinter &f) const
{
    quoteString(f, str, '"');
}

void JS::RegExpExprNode::print(PrettyPrinter &f) const
{
    f << '/' << re << '/' << flags;
}

void JS::NumUnitExprNode::print(PrettyPrinter &f) const
{
    f << numStr;
    StringExprNode::print(f);
}

void JS::ExprUnitExprNode::print(PrettyPrinter &f) const
{
    f << op;
    StringExprNode::print(f);
}

void JS::FunctionExprNode::print(PrettyPrinter &f) const
{
    function.print(f, 0, false);
}


void JS::PairListExprNode::print(PrettyPrinter &f) const
{
    char beginBracket;
    char endBracket;

    switch (getKind()) {
      case objectLiteral:
        beginBracket = '{';
        endBracket = '}';
        break;

      case arrayLiteral:
      case index:
        beginBracket = '[';
        endBracket = ']';
        break;

      case superStmt:
      case call:
      case New:
        beginBracket = '(';
        endBracket = ')';
        break;

      default:
        NOT_REACHED("Bad kind");
        return;
    }

    f << beginBracket;
    PrettyPrinter::Block b(f);
    const ExprPairList *p = pairs;
    if (p)
        while (true) {
            const ExprNode *field = p->field;
            if (field) {
                f << field << ':';
                f.fillBreak(0);
            }

            const ExprNode *value = p->value;
            if (value)
                f << value;

            p = p->next;
            if (!p)
                break;
            f << ',';
            f.linearBreak(static_cast<uint32>(field || value));
        }
    f << endBracket;
}


void JS::InvokeExprNode::print(PrettyPrinter &f) const
{
    PrettyPrinter::Block b(f);
    if (hasKind(New))
        f << "new ";
    if (hasKind(superStmt))
        f << "super";
    else
        f << op;
    PrettyPrinter::Indent i(f, subexpressionIndent);
    f.fillBreak(0);
    PairListExprNode::print(f);
}


void JS::SuperExprNode::print(PrettyPrinter &f) const
{
    f << "super";
    if (op) {
        f << '(';
        f << op;
        f << ')';
    }
}


void JS::UnaryExprNode::print(PrettyPrinter &f) const
{
    if (hasKind(parentheses)) {
        f << '(';
        f << op;
        f << ')';
    } else {
        if (debugExprNodePrint)
            f << '(';
        const char *name = kindName(getKind());
        if (hasKind(postIncrement) || hasKind(postDecrement) || hasKind(dotClass)) {
            f << op;
            f << name;
        } else {
            f << name;
            f << op;
        }
        if (debugExprNodePrint)
            f << ')';
    }
}


void JS::BinaryExprNode::print(PrettyPrinter &f) const
{
    if (debugExprNodePrint && !hasKind(juxtapose))
        f << '(';
    PrettyPrinter::Block b(f);
    f << op1;
    uint32 nSpaces = hasKind(dot) || hasKind(dotParen) || hasKind(juxtapose) ? (uint32)0 : (uint32)1;
    f.fillBreak(nSpaces);
    f << kindName(getKind());
    f.fillBreak(nSpaces);
    f << op2;
    if (hasKind(dotParen))
        f << ')';
    if (debugExprNodePrint && !hasKind(juxtapose))
        f << ')';
}


void JS::TernaryExprNode::print(PrettyPrinter &f) const
{
    if (debugExprNodePrint)
        f << '(';
    PrettyPrinter::Block b(f);
    f << op1;
    f.fillBreak(1);
    f << '?';
    f.fillBreak(1);
    f << op2;
    f.fillBreak(1);
    f << ':';
    f.fillBreak(1);
    f << op3;
    if (debugExprNodePrint)
        f << ')';
}


//
// ExprList
//


// ***** DEAD CODE *****
// Print a comma-separated ExprList on to f.  Since a method can't be called
// on nil, the list has at least one element.
void JS::ExprList::printCommaList(PrettyPrinter &f) const
{
    PrettyPrinter::Block b(f);
    const ExprList *list = this;
    while (true) {
        f << list->expr;
        list = list->next;
        if (!list)
            break;
        f << ',';
        f.fillBreak(1);
    }
}


// ***** DEAD CODE *****
// If list is nil, do nothing.  Otherwise, emit a linear line break of size 1,
// the name, a space, and the contents of list separated by commas.
void JS::ExprList::printOptionalCommaList(PrettyPrinter &f, const char *name, const ExprList *list)
{
    if (list) {
        f.linearBreak(1);
        f << name << ' ';
        list->printCommaList(f);
    }
}


//
// StmtNode
//


// Print statements on separate lines onto f.  Do not print a line break
// after the last statement.
void JS::StmtNode::printStatements(PrettyPrinter &f, const StmtNode *statements)
{
    if (statements) {
        PrettyPrinter::Block b(f);
        while (true) {
            const StmtNode *next = statements->next;
            statements->print(f, !next);
            statements = next;
            if (!statements)
                break;
            f.requiredBreak();
        }
    }
}


// Print statements as a block enclosed in curly braces onto f.
// If loose is false, do not insist on each statement being on a separate
// line; instead, make breaks between statements be linear breaks in the
// enclosing PrettyPrinter::Block scope. The caller must have placed this
// call inside a PrettyPrinter::Block scope.
void JS::StmtNode::printBlockStatements(PrettyPrinter &f, const StmtNode *statements, bool loose)
{
    f << '{';
    if (statements) {
        {
            PrettyPrinter::Indent i(f, basicIndent);
            uint32 nSpaces = 0;
            while (statements) {
                if (statements->hasKind(Case)) {
                    PrettyPrinter::Indent i2(f, caseIndent - basicIndent);
                    f.linearBreak(nSpaces, loose);
                    statements->print(f, false);
                } else {
                    f.linearBreak(nSpaces, loose);
                    statements->print(f, !statements->next);
                }
                statements = statements->next;
                nSpaces = 1;
            }
        }
        f.linearBreak(0, loose);
    } else
        f.fillBreak(0);
    f << '}';
}


// Print a closing statement semicolon onto f unless noSemi is true.
void JS::StmtNode::printSemi(PrettyPrinter &f, bool noSemi)
{
    if (!noSemi)
        f << ';';
}


// Print this as a substatement of a statement such as if or with.
// If this statement is a block without attributes, begin it on the current
// line and do not indent it -- the block itself will provide the indent.
// Otherwise, begin this statement on a new line and indent it.
// If continuation is non-nil, it specifies a continuation such as 'else' or
// the 'while' of a do-while statement.  If this statement is a block without
// attributes, print a space and the continuation after the closing brace;
// otherwise print the continuation on a new line.
// If noSemi is true, do not print the semicolon unless it is required by the statement.
// The caller must have placed this call inside a PrettyPrinter::Block scope that encloses
// the containining statement.
void JS::StmtNode::printSubstatement(PrettyPrinter &f, bool noSemi, const char *continuation) const
{
    if (hasKind(block) &&
        !static_cast<const BlockStmtNode *>(this)->attributes) {
        f << ' ';
        static_cast<const BlockStmtNode *>(this)->printBlock(f, true);
        if (continuation)
            f << ' ' << continuation;
    } else {
        {
            PrettyPrinter::Indent i(f, basicIndent);
            f.requiredBreak();
            this->print(f, noSemi);
        }
        if (continuation) {
            f.requiredBreak();
            f << continuation;
        }
    }
}


// Print the attributes on a single line separated with and followed by a
// space.
void JS::AttributeStmtNode::printAttributes(PrettyPrinter &f) const
{
    const ExprNode *a = attributes;
    if (a) {
        f << a << ' ';
    }
}


// Print this block, including attributes, onto f.
// If loose is false, do not insist on each statement being on a separate line;
// instead, make breaks between statements be linear breaks in the enclosing
// PrettyPrinter::Block scope.
// The caller must have placed this call inside a PrettyPrinter::Block scope.
void JS::BlockStmtNode::printBlock(PrettyPrinter &f, bool loose) const
{
    printAttributes(f);
    printBlockStatements(f, statements, loose);
}


// Print this onto f.
// If noSemi is true, do not print the trailing semicolon unless it is required by the statement.
void JS::StmtNode::print(PrettyPrinter &f, bool /*noSemi*/) const
{
    ASSERT(hasKind(empty));
    f << ';';
}

void JS::ExprStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    const ExprNode *e = expr;

    switch (getKind()) {
      case Case:
        if (e) {
            f << "case ";
            f << e;
        } else
            f << "default";
        f << ':';
        break;

      case Return:
        f << "return";
        if (e) {
            f << ' ';
            goto showExpr;
        } else
            goto showSemicolon;

      case Throw:
        f << "throw ";
      case expression:
      showExpr:
        f << e;
        showSemicolon:
        printSemi(f, noSemi);
        break;

      default:
        NOT_REACHED("Bad kind");
    }
}

void JS::DebuggerStmtNode::print(PrettyPrinter &f, bool) const
{
    f << "debugger;";
}

void JS::BlockStmtNode::print(PrettyPrinter &f, bool) const
{
    PrettyPrinter::Block b(f, 0);
    printBlock(f, true);
}

void JS::LabelStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    PrettyPrinter::Block b(f, basicIndent);
    f << name << ':';
    f.linearBreak(1);
    stmt->print(f, noSemi);
}

void JS::UnaryStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    PrettyPrinter::Block b(f, 0);
    printContents(f, noSemi);
}


// Same as print except that uses the caller's PrettyPrinter::Block.
void JS::UnaryStmtNode::printContents(PrettyPrinter &f, bool noSemi) const
{
    ASSERT(stmt);
    const char *kindName = 0;

    switch (getKind()) {
      case If:
        kindName = "if";
        break;

      case While:
        kindName = "while";
        break;

      case DoWhile:
        f << "do";
        stmt->printSubstatement(f, true, "while (");
        f << expr << ')';
        printSemi(f, noSemi);
        return;

      case With:
        kindName = "with";
        break;

      default:
        NOT_REACHED("Bad kind");
    }

    f << kindName << " (";
    f << expr << ')';
    stmt->printSubstatement(f, noSemi);
}

void JS::BinaryStmtNode::printContents(PrettyPrinter &f, bool noSemi) const
{
    ASSERT(stmt && stmt2 && hasKind(IfElse));

    f << "if (";
    f << expr << ')';
    stmt->printSubstatement(f, true, "else");
    if (stmt2->hasKind(If) || stmt2->hasKind(IfElse)) {
        f << ' ';
        static_cast<const UnaryStmtNode *>(stmt2)->printContents(f, noSemi);
    } else
        stmt2->printSubstatement(f, noSemi);
}


void JS::ForStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    ASSERT(stmt && (hasKind(For) || hasKind(ForIn)));

    PrettyPrinter::Block b(f, 0);
    f << "for (";
    {
        PrettyPrinter::Block b2(f);
        if (initializer)
            initializer->print(f, true);
        if (hasKind(ForIn)) {
            f.fillBreak(1);
            f << "in";
            f.fillBreak(1);
            ASSERT(expr2 && !expr3);
            f << expr2;
        } else {
            f << ';';
            if (expr2) {
                f.linearBreak(1);
                f << expr2;
            }
            f << ';';
            if (expr3) {
                f.linearBreak(1);
                f << expr3;
            }
        }
        f << ')';
    }
    stmt->printSubstatement(f, noSemi);
}


void JS::SwitchStmtNode::print(PrettyPrinter &f, bool) const
{
    PrettyPrinter::Block b(f);
    f << "switch (";
    f << expr << ") ";
    printBlockStatements(f, statements, true);
}


void JS::GoStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    const char *kindName = 0;

    switch (getKind()) {
      case Break:
        kindName = "break";
        break;

      case Continue:
        kindName = "continue";
        break;

      default:
        NOT_REACHED("Bad kind");
    }

    f << kindName;
    if (name)
        f << " " << *name;
    printSemi(f, noSemi);
}


void JS::TryStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    PrettyPrinter::Block b(f, 0);
    f << "try";
    const StmtNode *s = stmt;
    for (const CatchClause *c = catches; c; c = c->next) {
        s->printSubstatement(f, true, "catch (");
        PrettyPrinter::Block b2(f);
        f << c->name;
        ExprNode *t = c->type;
        if (t) {
            f << ':';
            f.linearBreak(1);
            f << t;
        }
        f << ')';
        s = c->stmt;
    }
    if (finally) {
        s->printSubstatement(f, true, "finally");
        s = finally;
    }
    s->printSubstatement(f, noSemi);
}


void JS::VariableStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    printAttributes(f);
    ASSERT(hasKind(Const) || hasKind(Var));
    f << (hasKind(Const) ? "const" : "var");
    {
        PrettyPrinter::Block b(f, basicIndent);
        const VariableBinding *binding = bindings;
        if (binding)
            while (true) {
                f.linearBreak(1);
                binding->print(f, false);
                binding = binding->next;
                if (!binding)
                    break;
                f << ',';
            }
    }
    printSemi(f, noSemi);
}


void JS::FunctionStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    function.print(f, this, noSemi);
}


void JS::NamespaceStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    printAttributes(f);
    ASSERT(hasKind(Namespace));

    PrettyPrinter::Block b(f, namespaceHeaderIndent);
    f << "namespace " << name;
    printSemi(f, noSemi);
}


void JS::ClassStmtNode::print(PrettyPrinter &f, bool noSemi) const
{
    printAttributes(f);
    ASSERT(hasKind(Class));

    {
        PrettyPrinter::Block b(f, namespaceHeaderIndent);
        f << "class " << name;
        if (superclass) {
            f.linearBreak(1);
            f << "extends ";
            f << superclass;
        }
    }
    if (body) {
        f.requiredBreak();
        body->printBlock(f, true);
    } else
        printSemi(f, noSemi);
}
