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