#include "txStylesheetCompiler.h"
#include "txStylesheetCompileHandlers.h"
#include "txAtoms.h"
#include "txURIUtils.h"
#include "Tokenizer.h"
#include "txStylesheet.h"
#include "txInstructions.h"
#include "txToplevelItems.h"
#include "ExprParser.h"
#include "txPatternParser.h"
#include "XSLTFunctions.h"

txStylesheetCompiler::txStylesheetCompiler(const String& aBaseURI)
    : mState(aBaseURI, nsnull)
{
}

txStylesheetCompiler::txStylesheetCompiler(const String& aBaseURI,
                                           txStylesheetCompiler* aParent)
    : mState(aBaseURI, aParent->mState.mStylesheet)
{
}

nsresult
txStylesheetCompiler::startElement(PRInt32 aNamespaceID, txAtom* aLocalName,
                                   txAtom* aPrefix,
                                   txStylesheetAttr* aAttributes,
                                   PRInt32 aAttrCount)
{
    nsresult rv = flushCharacters();
    NS_ENSURE_SUCCESS(rv, rv);

    // look for new namespace mappings
    PRInt32 i;
    for (i = 0; i < aAttrCount; ++i) {
        txStylesheetAttr* attr = aAttributes + i;
        if (attr->mNamespaceID == kNameSpaceID_XMLNS) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);

            if (attr->mLocalName == txXMLAtoms::xmlns) {
                mState.mElementContext->
                    mMappings.addNamespace(nsnull, attr->mValue);
            }
            else {
                mState.mElementContext->
                    mMappings.addNamespace(attr->mLocalName, attr->mValue);
            }
        }
    }

    // Update the elementcontext if we have special attributes
    for (i = 0; i < aAttrCount; ++i) {
        txStylesheetAttr* attr = aAttributes + i;

        // xml:space
        if (attr->mNamespaceID == kNameSpaceID_XML &&
            attr->mLocalName == txXMLAtoms::space) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);

            if (attr->mValue.isEqual(String("preserve"))) {
                mState.mElementContext->mPreserveWhitespace = MB_TRUE;
            }
            else if (attr->mValue.isEqual(String("default"))) {
                mState.mElementContext->mPreserveWhitespace = MB_FALSE;
            }
            else {
                return NS_ERROR_XSLT_PARSE_FAILURE;
            }
        }

        // xml:base
        if (attr->mNamespaceID == kNameSpaceID_XML &&
            attr->mLocalName == txXMLAtoms::base &&
            !attr->mValue.isEmpty()) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);
            
            String uri;
            URIUtils::resolveHref(attr->mValue, mState.mElementContext->mBaseURI, uri);
            mState.mElementContext->mBaseURI = uri;
        }

        // extension-element-prefixes
        if ((attr->mNamespaceID == kNameSpaceID_XSLT &&
             attr->mLocalName == txXSLTAtoms::extensionElementPrefixes &&
             aNamespaceID != kNameSpaceID_XSLT) ||
            (attr->mNamespaceID == kNameSpaceID_None &&
             attr->mLocalName == txXSLTAtoms::extensionElementPrefixes &&
             aNamespaceID == kNameSpaceID_XSLT &&
             (aLocalName == txXSLTAtoms::stylesheet ||
              aLocalName == txXSLTAtoms::transform))) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);

            txTokenizer tok(attr->mValue);
            while (tok.hasMoreTokens()) {
                String prefix;
                tok.nextToken(prefix);
                PRInt32 namespaceID = mState.mElementContext->
                    mMappings.lookupNamespaceWithDefault(prefix);
                
                if (namespaceID == kNameSpaceID_Unknown)
                    return NS_ERROR_XSLT_PARSE_FAILURE;

                mState.mElementContext->
                    mInstructionNamespaces.add(NS_INT32_TO_PTR(namespaceID));
            }
        }

        // version
        if ((attr->mNamespaceID == kNameSpaceID_XSLT &&
             attr->mLocalName == txXSLTAtoms::version &&
             aNamespaceID != kNameSpaceID_XSLT) ||
            (attr->mNamespaceID == kNameSpaceID_None &&
             attr->mLocalName == txXSLTAtoms::version &&
             aNamespaceID == kNameSpaceID_XSLT &&
             (aLocalName == txXSLTAtoms::stylesheet ||
              aLocalName == txXSLTAtoms::transform))) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);

            if (attr->mValue.isEqual(String("1.0"))) {
                mState.mElementContext->mForwardsCompatibleParsing = MB_FALSE;
            }
            else {
                mState.mElementContext->mForwardsCompatibleParsing = MB_TRUE;
            }
        }
    }

    // Find the right elementhandler and execute it
    MBool isInstruction = MB_FALSE;
    txListIterator iter(&mState.mElementContext->mInstructionNamespaces);
    PRInt32 instrID;
    while ((instrID = NS_PTR_TO_INT32(iter.next()))) {
        if (instrID == aNamespaceID) {
            isInstruction = MB_TRUE;
            break;
        }
    }

    txElementHandler* handler;
    do {
        handler = isInstruction ?
                  mState.mHandlerTable->find(aNamespaceID, aLocalName) :
                  mState.mHandlerTable->mLREHandler;

        rv = (handler->mStartFunction)(aNamespaceID, aLocalName, aPrefix,
                                       aAttributes, aAttrCount, mState);
    } while (rv == NS_ERROR_XSLT_GET_NEW_HANDLER);

    NS_ENSURE_SUCCESS(rv, rv);

    rv = mState.pushPtr(handler);
    NS_ENSURE_SUCCESS(rv, rv);

    mState.mElementContext->mDepth++;

    return NS_OK;
}

nsresult
txStylesheetCompiler::endElement()
{
    nsresult rv = flushCharacters();
    NS_ENSURE_SUCCESS(rv, rv);

    txElementHandler* handler =
        (txElementHandler*)mState.popPtr();
    rv = (handler->mEndFunction)(mState);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!--mState.mElementContext->mDepth) {
        delete mState.mElementContext;
        mState.mElementContext =
            (txElementContext*)mState.popObject();
    }

    return NS_OK;
}

nsresult
txStylesheetCompiler::characters(const String& aStr)
{
    mCharacters.append(aStr);

    return NS_OK;
}

nsresult
txStylesheetCompiler::flushCharacters()
{
    // bail if we don't have any characters, or if it is ignorable whitespace
    // we might want to do the whitespace-stripping in the handler instead
    if (mCharacters.isEmpty()) {
        return NS_OK;
    }

    nsresult rv = NS_OK;

    do {
        rv = (mState.mHandlerTable->mTextHandler)(mCharacters, mState);
    } while (rv == NS_ERROR_XSLT_GET_NEW_HANDLER);

    NS_ENSURE_SUCCESS(rv, rv);

    mCharacters.clear();

    return NS_OK;
}

nsresult
txStylesheetCompiler::ensureNewElementContext()
{
    // Do we already have a new context?
    if (!mState.mElementContext->mDepth)
        return NS_OK;
    
    txElementContext* context = new txElementContext(*mState.mElementContext);
    NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

    context->mDepth = 0;

    nsresult rv = mState.pushObject(mState.mElementContext);
    if (NS_FAILED(rv)) {
        delete context;
        return rv;
    }

    mState.mElementContext = context;

    return NS_OK;
}


/**
 * txStylesheetCompilerState
 */


txStylesheetCompilerState::txStylesheetCompilerState(const String& aBaseURI,
                                                     txStylesheet* aStylesheet)
    : mStylesheet(aStylesheet),
      mToplevelIterator(nsnull)
{
    nsresult rv = NS_OK;

    if (!mStylesheet) {
        mStylesheet = new txStylesheet;
        if (!mStylesheet) {
            // XXX invalidate
            return;
        }
        
        rv = mStylesheet->init();
        if (NS_FAILED(rv)) {
            // XXX invalidate
            return;
        }
        
        txListIterator tmpIter(&mStylesheet->mRootFrame->mToplevelItems);
        mToplevelIterator = tmpIter;
        mToplevelIterator.next(); // go to the end of the list
    }

    
    // XXX Embedded stylesheets have another handler. Probably
    mHandlerTable = gTxRootHandler;

    mElementContext = new txElementContext;
    if (!mElementContext) {
        // XXX invalidate
        return;
    }

    mElementContext->mPreserveWhitespace = MB_FALSE;
    mElementContext->mForwardsCompatibleParsing = MB_TRUE;
    mElementContext->mBaseURI = aBaseURI;
    mElementContext->mDepth = 0;
    rv = mElementContext->
        mInstructionNamespaces.add(NS_INT32_TO_PTR(kNameSpaceID_XSLT));
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    // Push the "old" txElementContext
    rv = pushObject(0);
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    mDOE = MB_FALSE;    
}


txStylesheetCompilerState::~txStylesheetCompilerState()
{
    delete mElementContext;
    while (!mObjectStack.empty()) {
        delete popObject();
    }
}

nsresult
txStylesheetCompilerState::pushHandlerTable(txHandlerTable* aTable)
{
    nsresult rv = pushPtr(mHandlerTable);
    NS_ENSURE_SUCCESS(rv, rv);

    mHandlerTable = aTable;

    return NS_OK;
}

void
txStylesheetCompilerState::popHandlerTable()
{
    mHandlerTable = (txHandlerTable*)popPtr();
}

nsresult
txStylesheetCompilerState::pushObject(TxObject* aObject)
{
    return mObjectStack.push(aObject);
}

TxObject*
txStylesheetCompilerState::popObject()
{
    return (TxObject*)mObjectStack.pop();
}

nsresult
txStylesheetCompilerState::pushPtr(void* aPtr)
{
    return mOtherStack.push(aPtr);
}

void*
txStylesheetCompilerState::popPtr()
{
    return mOtherStack.pop();
}

MBool
txStylesheetCompilerState::fcp()
{
    return mElementContext->mForwardsCompatibleParsing;
}

nsresult
txStylesheetCompilerState::addToplevelItem(txToplevelItem* aItem)
{
    nsresult rv = mToplevelIterator.addBefore(aItem);
    NS_ENSURE_SUCCESS(rv, rv);

    mToplevelIterator.next();
    return NS_OK;
}

nsresult
txStylesheetCompilerState::openInstructionContainer(txInstructionContainer* aContainer)
{
    NS_PRECONDITION(!mNextInstrPtr, "can't nest instruction-containers");

    mNextInstrPtr = &aContainer->mFirstInstruction;
    return NS_OK;
}

void
txStylesheetCompilerState::closeInstructionContainer()
{
    mNextInstrPtr = 0;
}

nsresult
txStylesheetCompilerState::addInstruction(txInstruction* aInstruction)
{
    NS_PRECONDITION(mNextInstrPtr, "adding instruction outside container");

    *mNextInstrPtr = aInstruction;
    mNextInstrPtr = &aInstruction->mNext;

    return NS_OK;
}

nsresult
txStylesheetCompilerState::parsePattern(const String& aPattern,
                                        txPattern** aResult)
{
    *aResult = txPatternParser::createPattern(aPattern, this);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_XPATH_PARSE_FAILURE);

    return NS_OK;
}

nsresult
txStylesheetCompilerState::parseExpr(const String& aExpr, Expr** aResult)
{
    *aResult = ExprParser::createExpr(aExpr, this);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_XPATH_PARSE_FAILURE);

    return NS_OK;
}

nsresult
txStylesheetCompilerState::parseAVT(const String& aExpr, Expr** aResult)
{
    *aResult = ExprParser::createAttributeValueTemplate(aExpr, this);
    NS_ENSURE_TRUE(*aResult, NS_ERROR_XPATH_PARSE_FAILURE);

    return NS_OK;
}

nsresult
txStylesheetCompilerState::parseQName(const String& aQName,
                                      txExpandedName& aExName,
                                      MBool aUseDefault)
{
    return aExName.init(aQName, mElementContext->mMappings, aUseDefault);
}


nsresult
txStylesheetCompilerState::resolveNamespacePrefix(txAtom* aPrefix,
                                                  PRInt32& aID)
{
#ifdef DEBUG
    if (!aPrefix || aPrefix == txXMLAtoms::_empty) {
        // default namespace is not forwarded to xpath
        NS_ASSERTION(0, "caller should handle default namespace ''");
        aID = kNameSpaceID_None;
        return NS_OK;
    }
#endif
    aID = mElementContext->mMappings.lookupNamespace(aPrefix);
    return (aID != kNameSpaceID_Unknown) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
txStylesheetCompilerState::resolveFunctionCall(txAtom* aName, PRInt32 aID,
                                               FunctionCall*& aFunction)
{
   aFunction = nsnull;

   if (aID != kNameSpaceID_None) {
       return NS_ERROR_XPATH_UNKNOWN_FUNCTION;
   }
   if (aName == txXSLTAtoms::document) {
       aFunction = new DocumentFunctionCall(mElementContext->mBaseURI);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::key) {
       aFunction = new txKeyFunctionCall(mElementContext->mMappings);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::formatNumber) {
       aFunction = new txFormatNumberFunctionCall(mStylesheet,
                                                  mElementContext->mMappings);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::current) {
       aFunction = new CurrentFunctionCall();
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::unparsedEntityUri) {

       return NS_ERROR_NOT_IMPLEMENTED;
   }
   if (aName == txXSLTAtoms::generateId) {
       aFunction = new GenerateIdFunctionCall();
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::systemProperty) {
       aFunction = new SystemPropertyFunctionCall(mElementContext->mMappings);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::elementAvailable) {
       aFunction =
          new ElementAvailableFunctionCall(mElementContext->mMappings);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }
   if (aName == txXSLTAtoms::functionAvailable) {
       aFunction =
          new FunctionAvailableFunctionCall(mElementContext->mMappings);
       NS_ENSURE_TRUE(aFunction, NS_ERROR_OUT_OF_MEMORY);

       return NS_OK;
   }

   return NS_ERROR_XPATH_UNKNOWN_FUNCTION;
}

void
txStylesheetCompilerState::receiveError(const String& aMsg, nsresult aRes)
{
    // XXX implement me
}
