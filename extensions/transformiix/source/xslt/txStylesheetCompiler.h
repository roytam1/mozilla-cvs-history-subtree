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
 * The Original Code is TransforMiiX XSLT processor.
 *
 * The Initial Developer of the Original Code is
 * Jonas Sicking.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * Jonas Sicking. All Rights Reserved.
 *
 * Contributor(s):
 *   Jonas Sicking <jonas@sicking.cc>
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

#ifndef TRANSFRMX_TXSTYLESHEETCOMPILER_H
#define TRANSFRMX_TXSTYLESHEETCOMPILER_H

#include "baseutils.h"
#include "txError.h"
#include "Stack.h"
#include "TxString.h"
#include "txAtom.h"
#include "txXSLTPatterns.h"
#include "Expr.h"
#include "XMLUtils.h"
#include "txIXPathContext.h"

class txHandlerTable;
class txElementContext;
class txStylesheet;
class txInstructionContainer;
class txInstruction;
class txToplevelItem;

class txStylesheetCompilerState : public txIParseContext
{
public:
    txStylesheetCompilerState(const String& aBase, txStylesheet* aStylesheet);
    ~txStylesheetCompilerState();

    // Stack functions
    nsresult pushHandlerTable(txHandlerTable* aTable);
    void popHandlerTable();
    nsresult pushObject(TxObject* aObject);
    TxObject* popObject();
    nsresult pushPtr(void* aPtr);
    void* popPtr();

    // parsing functions
    nsresult parsePattern(const String& aPattern, txPattern** aResult);
    nsresult parseExpr(const String& aExpr, Expr** aResult);
    nsresult parseAVT(const String& aExpr, Expr** aResult);
    nsresult parseQName(const String& aQName, txExpandedName& aExName,
                        MBool aUseDefault);

    // State-checking functions
    MBool fcp();
    
    // stylesheet functions
    nsresult addToplevelItem(txToplevelItem* aItem);
    nsresult openInstructionContainer(txInstructionContainer* aContainer);
    void closeInstructionContainer();
    nsresult addInstruction(txInstruction* aInstruction);

    // txIParseContext
    nsresult resolveNamespacePrefix(txAtom* aPrefix, PRInt32& aID);
    nsresult resolveFunctionCall(txAtom* aName, PRInt32 aID,
                                 FunctionCall*& aFunction);
    void receiveError(const String& aMsg, nsresult aRes);


    txStylesheet* mStylesheet;
    txHandlerTable* mHandlerTable;
    txElementContext* mElementContext;
    MBool mDOE;
    
private:
    Stack mObjectStack;
    Stack mOtherStack;
    txInstruction** mNextInstrPtr;
    txListIterator mToplevelIterator;
};

struct txStylesheetAttr
{
    PRInt32 mNamespaceID;
    txAtom* mLocalName;
    txAtom* mPrefix;
    String mValue;
};

class txStylesheetCompiler
{
public:
    txStylesheetCompiler(const String& aBaseURI);
    txStylesheetCompiler(const String& aBaseURI,
                         txStylesheetCompiler* aParent);
    nsresult startElement(PRInt32 aNamespaceID, txAtom* aLocalName,
                          txAtom* aPrefix, txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount);
    nsresult endElement();
    nsresult characters(const String& aStr);
    nsresult doneLoading(); // XXX do we want to merge this with cancel?

    void cancel(nsresult aError);

private:
    nsresult flushCharacters();
    nsresult ensureNewElementContext();

    txStylesheetCompilerState mState;
    String mCharacters;
};

class txElementContext : public TxObject
{
public:
    MBool mPreserveWhitespace;
    MBool mForwardsCompatibleParsing;
    String mBaseURI;
    txNamespaceMap mMappings;
    txList mInstructionNamespaces;
    PRInt32 mDepth;
};

#endif