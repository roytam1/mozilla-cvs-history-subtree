#include "txStylesheetCompiler.h"
#include "txStylesheetCompilerHandlers.h"
#include "Tokenizer.h"

#if 0

#define TX_ENSURE_SUCCESS_OR_FCP(_rv, _state)                               \
    NS_ENSURE_TRUE(NS_SUCCEEDED(_rv) || _state.fcp(), _rv)

#define TX_RETURN_IF_WHITESPACE(_str, _state)                               \
    do {                                                                    \
      if (!_state.mElementContext->mPreserveWhitespace &&                   \
          XMLUtils::isWhitespace(_str)) {                                   \
          return NS_OK;                                                     \
      }                                                                     \
    } while(0)

/**
 * Standard handlers
 */
nsresult
txFnTextIgnore(const String& aStr, txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnTextError(const String& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    return NS_ERROR_XSLT_PARSE_FAILURE;
}

nsresult
txFnStartElementIgnore(PRInt32 aNamespaceID,
                       txAtom* aLocalName,
                       txAtom* aPrefix,
                       txStylesheetAttr* aAttributes,
                       PRInt32 aAttrCount,
                       txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnEndElementIgnore(txStylesheetCompilerState& aState)
{
    return NS_OK;
}

nsresult
txFnStartElementSetIgnore(PRInt32 aNamespaceID,
                          txAtom* aLocalName,
                          txAtom* aPrefix,
                          txStylesheetAttr* aAttributes,
                          PRInt32 aAttrCount,
                          txStylesheetCompilerState& aState)
{
    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndElementSetIgnore(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnStartElementError(PRInt32 aNamespaceID,
                      txAtom* aLocalName,
                      txAtom* aPrefix,
                      txStylesheetAttr* aAttributes,
                      PRInt32 aAttrCount,
                      txStylesheetCompilerState& aState)
{
    return NS_ERROR_XSLT_PARSE_FAILURE;
}

nsresult
txFnEndElementError(txStylesheetCompilerState& aState)
{
    NS_ASSERTION("txFnEndElementError shouldn't be called"); 
    return NS_ERROR_XSLT_PARSE_FAILURE;
}


/**
 * Root handlers
 */
nsresult
txFnStartStylesheet(PRInt32 aNamespaceID,
                    txAtom* aLocalName,
                    txAtom* aPrefix,
                    txStylesheetAttr* aAttributes,
                    PRInt32 aAttrCount,
                    txStylesheetCompilerState& aState)
{
    if (!getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::version)) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    return aState.pushHandlerTable(gTxTopHandler);
}

nsresult
txFnEndStylesheet(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}


/**
 * Top handlers
 */
nsresult
txFnStartOtherTop(PRInt32 aNamespaceID,
                  txAtom* aLocalName,
                  txAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    if (aNamespaceID == kNameSpaceID_None) {
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }
    
    return aState.pushHandlerTable(gTxIgnoreHandlerTable);
}

nsresult
txFnEndOtherTop(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}


nsresult
txFnStartTemplate(PRInt32 aNamespaceID,
                  txAtom* aLocalName,
                  txAtom* aPrefix,
                  txStylesheetAttr* aAttributes,
                  PRInt32 aAttrCount,
                  txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushHandlerTable(gTxTemplateHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txStylesheetAttr* attr = 0;
    
    txExpandedName name;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::name))) {
        rv = aState.parseQName(attr->mValue, name);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    txExpandedName mode;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::mode))) {
        rv = aState.parseQName(attr->mValue, mode);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    double prio = Double::NaN;
    if ((attr = getStyleAttr(aAttributes, aAttrCount,
                             txXSLTAtoms::priority))) {
        prio = Double::toDouble(attr->mValue);
        if (Double::isNaN(prio) && !aState->fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    txPattern* match = 0;
    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::match))) {
        rv = aState.parsePattern(attr->mValue, &match);
        TX_ENSURE_SUCCESS_OR_FCP(rv, aState);
    }

    txTemplateItem* templ = new txTemplateItem(match, name, mode, prio);
    if (!templ) {
        delete match;
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    rv = aState.mStylesheet->addToplevelItem(templ);
    if (NS_FAILED(rv) {
        delete templ;
        return rv;
    }
    
    aState.openInstructionContainer(templ);
    
    return NS_OK;
}

nsresult
txFnEndTemplate(txStylesheetCompilerState& aState)
{
    aState.closeInstructionContainer();
    aState.popHandlerTable();

    return NS_OK;
}

nsresult
txFnStartKey(PRInt32 aNamespaceID,
             txAtom* aLocalName,
             txAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = aState.pushHandlerTable(gTxIgnoreHandler);
    NS_ENSURE_SUCCESS(rv, rv);
    
    txStylesheetAttr* attr = 0;
    
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::name);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    txExpandedName name;
    rv = aState.parseQName(attr->mValue, name);
    NS_ENSURE_SUCCESS(rv, rv);

    txPattern* match = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::match);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    rv = aState.parsePattern(attr->mValue, &match);
    NS_ENSURE_SUCCESS(rv, rv);


    Expr* use = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::use);
    if (!attr) {
        delete match;
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    rv = aState.parseExpr(attr->mValue, &use);
    if (NS_FAILED(rv)) {
        delete match;
        return NS_ERROR_XSLT_PARSE_FAILURE;
    }

    rv = aState.mStylesheet->addKey(name, match, use);
    if (NS_FAILED(rv) {
        delete match;
        delete use;
        return rv;
    }
    
    return NS_OK;
}

nsresult
txFnEndTemplate(txStylesheetCompilerState& aState)
{
    aState.setInstructionContainer(0);
    aState.popHandlerTable();

    return NS_OK;
}

/**
 * Template Handlers
 */
// LRE
nsresult
txFnStartLRE(PRInt32 aNamespaceID,
             txAtom* aLocalName,
             txAtom* aPrefix,
             txStylesheetAttr* aAttributes,
             PRInt32 aAttrCount,
             txStylesheetCompilerState& aState)
{
    nsresult rv = NS_OK;

    txInstruction* instr = new txStartLREElement(aNamespaceID, aLocalName,
                                                 aPrefix);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    rv = aState->addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    txStylesheetAttr* attr = 0;
    
    attr = getStyleAttr(aAttributes, aAttrCount, kNameSpaceID_XSLT,
                        txXSLTAtoms::name);
    if (attr) {
        txTokenizer tok(attr->mValue);
        while (tok.hasMoreTokens()) {
            String qname;
            tok.nextToken(qname);

            txExpandedName name;
            rv = aState.parseQName(qname, name);
            NS_ENSURE_SUCCESS(rv, rv);

            instr = new txInsertAttrSet(name);
            NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

            rv = aState->addInstruction(instr);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        attr = aAttributes + i;
        
        if (attr->mNamespace == kNameSpaceID_XSLT) {
            continue;
        }
        
        Expr* avt = 0;
        rv = aState.parseAVT(attr->mValue, &avt);
        NS_ENSURE_SUCCESS(rv, rv);

        instr = new txLREAttribute(attr->mNamespaceID, attr->mLocalName,
                                   attr->mPrefix, avt);
        if (!instr) {
            delete avt;
            return NS_ERROR_OUT_OF_MEMORY;
        }
        
        rv = aState->addInstruction(instr);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
txFnEndLRE(txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txEndLREElement;
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState->addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
txFnText(const String& aStr, txStylesheetCompilerState& aState)
{
    TX_RETURN_IF_WHITESPACE(aStr, aState);

    txInstruction* instr = new txTextInstruction(aStr, MB_FALSE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState->addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:text
nsresult
txFnStartText(PRInt32 aNamespaceID,
              txAtom* aLocalName,
              txAtom* aPrefix,
              txStylesheetAttr* aAttributes,
              PRInt32 aAttrCount,
              txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = 0;

    if ((attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::disableOutputEscaping))) {
        if (attr->mValue.isEqual(String("yes"))) {
            aState.mDOE = MB_TRUE;
        }
        else if (!attr->mValue.isEqual(String("no")) && !aState.fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    return aState.pushHandlerTable(gTxTextHandler);
}

nsresult
txFnEndText(txStylesheetCompilerState& aState)
{
    aState.mDOE = MB_FALSE;
    aState.popHandlerTable();
    return NS_OK;
}

nsresult
txFnTextText(const String& aStr, txStylesheetCompilerState& aState)
{
    txInstruction* instr = new txTextInstruction(aStr, aState.mDOE);
    NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = aState->addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

// xsl:value-of
nsresult
txFnStartValueOf(PRInt32 aNamespaceID,
                 txAtom* aLocalName,
                 txAtom* aPrefix,
                 txStylesheetAttr* aAttributes,
                 PRInt32 aAttrCount,
                 txStylesheetCompilerState& aState)
{
    txStylesheetAttr* attr = 0;

    MBool doe = MB_FALSE;

    if ((attr = getStyleAttr(aAttributes, aAttrCount,
                             txXSLTAtoms::disableOutputEscaping))) {
        if (attr->mValue.isEqual(String("yes"))) {
            doe = MB_TRUE;
        }
        else if (!attr->mValue.isEqual(String("no")) && !aState.fcp()) {
            return NS_ERROR_XSLT_PARSE_FAILURE;
        }
    }

    Expr* select = 0;
    attr = getStyleAttr(aAttributes, aAttrCount, txXSLTAtoms::select);
    NS_ENSURE_TRUE(attr, NS_ERROR_XSLT_PARSE_FAILURE);

    rv = aState.parseExpr(attr->mValue, &select);
    NS_ENSURE_SUCCESS(rv, rv);

    txInstruction* instr = new txValueOfInstruction(instr, doe);
    if (!instr) {
        delete select;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv = aState->addInstruction(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    return aState.pushHandlerTable(gTxIgnoreHandler);
}

nsresult
txFnEndValueOf(txStylesheetCompilerState& aState)
{
    aState.popHandlerTable();
    return NS_OK;
}

/**
 * Table Datas
 */

txHandlerTable* gTxIgnoreHandler = 0;
txHandlerTableData gTxIgnoreTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // LRE
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // Text
  txFnTextIgnore
};

txHandlerTable* gTxRootHandler = 0;
txHandlerTableData gTxRootTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "stylesheet", txFnStylesheetStart, txFnStylesheetEnd },
    { kNameSpaceID_XSLT, "transform", txFnStylesheetStart, txFnStylesheetEnd },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // LRE
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // Text
  txFnTextError
};

txHandlerTable* gTxTopHandler = 0;
txHandlerTableData gTxTopTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "template", txFnStartTemplate, txFnEndTemplate },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartTopOther, txFnEndTopOther },
  // LRE
  { 0, 0, txFnStartTopOther, txFnEndTopOther },
  // Text
  txFnTextIgnore
};

txHandlerTable* gTxTemplateHandler = 0;
txHandlerTableData gTxTemplateTableData = {
  // Handlers
  { { kNameSpaceID_XSLT, "fallback", txFnStartElementSetIgnore, txFnEndElementSetIgnore },
    { kNameSpaceID_XSLT, "text", txFnStartText, txFnEndText },
    { kNameSpaceID_XSLT, "value-of", txFnStartValueOf, txFnEndValueOf },
    { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementIgnore, txFnEndElementIgnore },
  // LRE
  { 0, 0, txFnStartLRE, txFnEndLRE },
  // Text
  txFnTextIgnore
};

txHandlerTable* gTxTextHandler = 0;
txHandlerTableData gTxTextTableData = {
  // Handlers
  { { 0, 0, 0, 0 } },
  // Other
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // LRE
  { 0, 0, txFnStartElementError, txFnEndElementError },
  // Text
  txFnTextText
};



/**
 * txHandlerTable
 */
txHandlerTable::txHandlerTable() : mHandlers(MB_FALSE)
{
}

nsresult
txHandlerTable::init(txHandlerTableData* aTableData)
{
    nsresult rv = NS_OK;

    mTextHandler = aTableData->mTextHandler;
    mLREHandler = aTableData->mLREHandler;
    mOtherHandler = aTableData->mOtherHandler;

    txElementHandler* handler = aTableData->mHandlers;
    while (handler->mLocalName) {
        txAtom* nameAtom = TX_GET_ATOM(String(handler->mLocalName));
        txExpandedName name(handler->mNamespaceID, nameAtom);
        TX_IF_RELEASE_ATOM(nameAtom);
        // XXX this sucks
        rv = mHandlers.add(name, (TxObject*)handler);
        NS_ENSURE_SUCCESS(rv, rv);

        handler++;
    }
}

txElementHandler*
txHandlerTable::find(PRInt32 aNamespaceID, txAtom* aLocalName)
{
    txExpandedName name(aNamespaceID, aLocalName);
    txElementHandler* handler = (txElementHandler*)mHandlers.get(name);
    if (!handler) {
        handler = mOtherHandler;
    }
    return handler;
}

#define INIT_HANDLER(_name)                                 \
    gTx##_name##Handler = new txHandlerTable();             \
    if (!gTx##_name##Handler)                               \
        return MB_FALSE;                                    \
                                                            \
    rv = gTx##_name##Handler->init(gTx##_name##TableData);  \
    if (NS_FAILED(rv))                                      \
        return MB_FALSE

#define SHUTDOWN_HANDLER(_name)                             \
    delete gTx##_name##Handler;

// static
MBool
txHandlerTable::init()
{
    nsresult rv = NS_OK;

    INIT_HANDLER(Root);
    INIT_HANDLER(Top);
    INIT_HANDLER(Ignore);
    INIT_HANDLER(Template);
    INIT_HANDLER(Text);

    return MB_FALSE;
}

// static
txHandlerTable::shutdown()
{
    SHUTDOWN_HANDLER(Root);
    SHUTDOWN_HANDLER(Top);
    SHUTDOWN_HANDLER(Ignore);
    SHUTDOWN_HANDLER(Template);
    SHUTDOWN_HANDLER(Text);
}

#endif
