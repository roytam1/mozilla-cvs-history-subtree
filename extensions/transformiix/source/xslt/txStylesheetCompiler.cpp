#include "txStylesheetCompiler.h"
#include "txStylesheetCompilerHandlers.h"
#include "txAtoms.h"
#include "txURIUtils.h"
#include "Tokenizer.h"
#include "txStylesheet.h"

txStylesheetCompiler::txStylesheetCompiler(const String& aBaseURI)
{
    mState.mStylesheet = new txStylesheet;
    if (!mState.mStylesheet) {
        // XXX invalidate
        return;
    }
    
    // XXX Embedded stylesheets have another handler. Probably
    mState.mHandlerTable = gTxRootHandler;

    mState.mElementContext = new txElementContext;
    if (!mState.mElementContext) {
        // XXX invalidate
        return;
    }

    mState.mElementContext->mPreserveWhitespace = MB_FALSE;
    mState.mElementContext->mForwardsCompatibleParsing = MB_TRUE;
    mState.mElementContext->mBaseURI = aBaseURI;
    mState.mElementContext->mDepth = 0;
    nsresult rv = mState.mElementContext->
        mInstructionNamespaces.add(NS_INT32_TO_PTR(kNameSpaceID_XSLT));
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    // Push the "old" txElementContext
    rv = mState.pushObject(0);
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    mDOE = MB_FALSE;    
}

txStylesheetCompiler::txStylesheetCompiler(const String& aBaseURI,
                                           txStylesheetCompiler* aParent)
{
    mState.mElementContext = new txElementContext;
    if (!mState.mElementContext) {
        // XXX invalidate
        return;
    }

    mState.mElementContext->mPreserveWhitespace = MB_FALSE;
    mState.mElementContext->mForwardsCompatibleParsing = MB_TRUE;
    mState.mElementContext->mBaseURI = aBaseURI;
    mState.mElementContext->mDepth = 0;
    nsresult rv = mState.mElementContext->
        mInstructionNamespaces.add(NS_INT32_TO_PTR(kNameSpaceID_XSLT));
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    // Push the "old" txElementContext
    rv = mState.pushObject(0);
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }

    mStylesheet = aParent->mState.mStylesheet;
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
                    mMappings.addNamespace(0, attr->mValue);
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
                                       aAttributes, mState);
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
}


/**
 * txStylesheetCompilerState
 */

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
