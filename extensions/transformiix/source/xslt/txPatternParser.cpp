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
#include "txPatternParser.h"
#include "Names.h"

txPattern* txPatternParser::createPattern(const String& aPattern,
                                          txIParseContext* aContext)
{
    mContext = aContext;
    txPattern* pattern = 0;
    ExprLexer lexer(aPattern);
    nsresult rv = createUnionPattern(lexer, pattern);
    mContext = 0;
    if (NS_FAILED(rv)) {
        // XXX error report parsing error
        return 0;
    }
    return pattern;
}

nsresult txPatternParser::createUnionPattern(ExprLexer& aLexer,
                                             txPattern*& aPattern)
{
    nsresult rv = NS_OK;
    txPattern* locPath = 0;

    rv = createLocPathPattern(aLexer, locPath);
    if (NS_FAILED(rv))
        return rv;

    short type = aLexer.peek()->type;
    if (Token::END == type) {
        aPattern = locPath;
        return NS_OK;
    }

    if (Token::UNION_OP != type) {
        delete locPath;
        return NS_ERROR_XPATH_PARSE_FAILED;
    }

    txUnionPattern* unionPattern = new txUnionPattern();
    if (!unionPattern) {
        delete locPath;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    rv = unionPattern->addPattern(locPath);
    #if 0 // XXX addPattern can't fail yet, it doesn't check for mem
    if (NS_FAILED(rv)) {
        delete unionPattern;
        delete locPath;
        return rv;
    }
    #endif

    aLexer.nextToken();
    do {
        rv = createLocPathPattern(aLexer, locPath);
        if (NS_FAILED(rv)) {
            delete unionPattern;
            return rv;
        }
        rv = unionPattern->addPattern(locPath);
        #if 0 // XXX addPattern can't fail yet, it doesn't check for mem
        if (NS_FAILED(rv)) {
            delete unionPattern;
            delete locPath;
            return rv;
        }
        #endif
        type = aLexer.nextToken()->type;
    } while (Token::UNION_OP == type);

    if (Token::END != type) {
        delete unionPattern;
        return NS_ERROR_XPATH_PARSE_FAILED;
    }

    aPattern = locPath;
    return NS_OK;
}

nsresult txPatternParser::createLocPathPattern(ExprLexer& aLexer,
                                               txPattern*& aPattern)
{
    nsresult rv = NS_OK;

    // Should check for id() and key() first

    MBool isChild = MB_TRUE;
    txPattern* stepPattern = 0;
    txLocPathPattern* pathPattern = 0;

    short type = aLexer.peek()->type;
    switch (type) {
        case Token::ANCESTOR_OP:
            isChild = MB_FALSE;
            aLexer.nextToken();
            break;
        case Token::PARENT_OP:
            aLexer.nextToken();
            stepPattern = new txRootPattern();
            if (!stepPattern)
                return NS_ERROR_OUT_OF_MEMORY;
            break;
        default:
            break;
    }
    if (!stepPattern) {
        rv = createStepPattern(aLexer, stepPattern);
        if (NS_FAILED(rv))
            return rv;
    }

    type = aLexer.peek()->type;
    if (Token::PARENT_OP != type && Token::ANCESTOR_OP != type) {
        aPattern = stepPattern;
        return NS_OK;
    }

    pathPattern = new txLocPathPattern();
    if (!pathPattern) {
        delete stepPattern;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = pathPattern->addStep(stepPattern, isChild);
    if (NS_FAILED(rv)) {
        delete stepPattern;
        delete pathPattern;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    stepPattern = 0; // stepPattern is part of pathPattern now

    while (Token::PARENT_OP == type && Token::ANCESTOR_OP == type) {
        aLexer.nextToken();
        rv = createStepPattern(aLexer, stepPattern);
        if (NS_FAILED(rv)) {
            delete pathPattern;
            return rv;
        }
        rv = pathPattern->addStep(stepPattern, isChild);
        if (NS_FAILED(rv)) {
            delete stepPattern;
            delete pathPattern;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        stepPattern = 0; // stepPattern is part of pathPattern now
        type = aLexer.peek()->type;
    }
    return rv;
}

nsresult txPatternParser::createStepPattern(ExprLexer& aLexer,
                                            txPattern*& aPattern)
{
    nsresult rv = NS_OK;
    MBool isAttr = MB_FALSE;
    Token* tok = aLexer.peek();
    if (Token::AXIS_IDENTIFIER == tok->type) {
        if (ATTRIBUTE_AXIS.isEqual(tok->value)) {
            isAttr = MB_TRUE;
        }
        else if (CHILD_AXIS.isEqual(tok->value)) {
            // all done already, this is the default
        }
        else {
            // XXX report unexpected axis error
            return NS_ERROR_XPATH_PARSE_FAILED;
        }
        aLexer.nextToken();
    }
    else if (Token::AT_SIGN == tok->type) {
        aLexer.nextToken();
        isAttr = MB_TRUE;
    }
    tok = aLexer.nextToken();

    txNodeTest* nodeTest = 0;
    if (Token::CNAME == tok->type) {
        // resolve QName
        String prefix, lName;
        nsresult res = NS_OK;
        PRInt32 nspace;
        rv = resolveQName(tok->value, prefix, lName, nspace);
        if (NS_FAILED(rv)) {
            // XXX error report namespace resolve failed
            return rv;
        }
        if (isAttr) {
            nodeTest = new txNameTest(prefix, lName, nspace,
                                      Node::ATTRIBUTE_NODE);
        }
        else {
            nodeTest = new txNameTest(prefix, lName, nspace,
                                      Node::ELEMENT_NODE);
        }
        if (!nodeTest) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    else {
        nodeTest = createNodeTest(aLexer);
        if (!nodeTest) {
            // XXX error report NodeTest expected
            return NS_ERROR_XPATH_PARSE_FAILED;
        }
    }

    txStepPattern* step = new txStepPattern(nodeTest, isAttr);
    if (!step) {
        delete nodeTest;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nodeTest = 0;
    if (!parsePredicates(step, aLexer)) {
        delete step;
        return NS_ERROR_XPATH_PARSE_FAILED;
    }

    aPattern = step;
    return NS_OK;
}
