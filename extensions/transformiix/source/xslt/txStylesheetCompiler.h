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

class txHandlerTable;
class txElementContext;
class txStylesheet;

class txStylesheetCompilerState
{
public:
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
    nsresult parseQName(const String& aQName, txExpandedName& aExName);

    // State-checking functions
    nsresult resolveNamespacePrefix(txAtom* aPrefix, PRInt32& aID);
    MBool fcp();

    txStylesheet* mStylesheet;
    txHandlerTable* mHandlerTable;
    txElementContext* mElementContext;
    MBool mDOE;
    
private:
    Stack mObjectStack;
    Stack mOtherStack;
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
    nsresult startElement(PRInt32 aNamespaceID, txAtom* aLocalName,
                          txAtom* aPrefix, txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount);
    nsresult endElement();
    nsresult characters(const String& aStr);

    void cancel(nsresult aError);

private:
    txStylesheetCompiler(const String& aBaseURI,
                         txStylesheetCompiler* aParent);
    nsresult flushCharacters();
    nsresult ensureNewElementContext();

    txStylesheetCompilerState mState;
    String mCharacters;
};

#endif