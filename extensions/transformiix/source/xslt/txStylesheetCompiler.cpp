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

#include "txStylesheetCompiler.h"
#include "txStylesheetCompileHandlers.h"
#include "txAtoms.h"
#include "txURIUtils.h"
#include "txTokenizer.h"
#include "txStylesheet.h"
#include "txInstructions.h"
#include "txToplevelItems.h"
#include "ExprParser.h"
#include "txPatternParser.h"
#include "txStringUtils.h"
#include "XSLTFunctions.h"

txStylesheetCompiler::txStylesheetCompiler(const nsAString& aBaseURI)
    : mState(aBaseURI, nsnull)
{
}

txStylesheetCompiler::txStylesheetCompiler(const nsAString& aBaseURI,
                                           txStylesheetCompiler* aParent)
    : mState(aBaseURI, aParent->mState.mStylesheet)
{
}

nsrefcnt
txStylesheetCompiler::AddRef()
{
    return ++mRefCnt;
}

nsrefcnt
txStylesheetCompiler::Release()
{
    if (--mRefCnt == 0) {
        mRefCnt = 1; //stabilize
        delete this;
        return 0;
    }
    return mRefCnt;
}

nsresult
txStylesheetCompiler::startElement(PRInt32 aNamespaceID, nsIAtom* aLocalName,
                                   nsIAtom* aPrefix,
                                   txStylesheetAttr* aAttributes,
                                   PRInt32 aAttrCount)
{
    nsresult rv = flushCharacters();
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 i;
    for (i = mState.mInScopeVariables.Count() - 1; i >= 0; --i) {
        ++((txInScopeVariable*)mState.mInScopeVariables[i])->mLevel;
    }

    // look for new namespace mappings
    PRBool hasOwnNamespaceMap = PR_FALSE;
    for (i = 0; i < aAttrCount; ++i) {
        txStylesheetAttr* attr = aAttributes + i;
        if (attr->mNamespaceID == kNameSpaceID_XMLNS) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);

            if (!hasOwnNamespaceMap) {
                mState.mElementContext->mMappings =
                    new txNamespaceMap(*mState.mElementContext->mMappings);
                NS_ENSURE_TRUE(mState.mElementContext->mMappings,
                               NS_ERROR_OUT_OF_MEMORY);
                hasOwnNamespaceMap = PR_TRUE;
            }

            if (attr->mLocalName == txXMLAtoms::xmlns) {
                mState.mElementContext->mMappings->
                    addNamespace(nsnull, attr->mValue);
            }
            else {
                mState.mElementContext->mMappings->
                    addNamespace(attr->mLocalName, attr->mValue);
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

            if (TX_StringEqualsAtom(attr->mValue, txXMLAtoms::preserve)) {
                mState.mElementContext->mPreserveWhitespace = MB_TRUE;
            }
            else if (TX_StringEqualsAtom(attr->mValue, txXMLAtoms::_default)) {
                mState.mElementContext->mPreserveWhitespace = MB_FALSE;
            }
            else {
                return NS_ERROR_XSLT_PARSE_FAILURE;
            }
        }

        // xml:base
        if (attr->mNamespaceID == kNameSpaceID_XML &&
            attr->mLocalName == txXMLAtoms::base &&
            !attr->mValue.IsEmpty()) {
            rv = ensureNewElementContext();
            NS_ENSURE_SUCCESS(rv, rv);
            
            nsAutoString uri;
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
                PRInt32 namespaceID = mState.mElementContext->mMappings->
                    lookupNamespaceWithDefault(tok.nextToken());
                
                if (namespaceID == kNameSpaceID_Unknown)
                    return NS_ERROR_XSLT_PARSE_FAILURE;

                if (!mState.mElementContext->mInstructionNamespaces.
                        AppendElement(NS_INT32_TO_PTR(namespaceID))) {
                    return NS_ERROR_OUT_OF_MEMORY;
                }
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

            if (attr->mValue.Equals(NS_LITERAL_STRING("1.0"))) {
                mState.mElementContext->mForwardsCompatibleParsing = MB_FALSE;
            }
            else {
                mState.mElementContext->mForwardsCompatibleParsing = MB_TRUE;
            }
        }
    }

    // Find the right elementhandler and execute it
    MBool isInstruction = MB_FALSE;
    PRInt32 count = mState.mElementContext->mInstructionNamespaces.Count();
    for (i = 0; i < count; ++i) {
        if (NS_PTR_TO_INT32(mState.mElementContext->mInstructionNamespaces[i]) ==
            aNamespaceID) {
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

    PRInt32 i;
    for (i = mState.mInScopeVariables.Count() - 1; i >= 0; --i) {
        txInScopeVariable* var =
            (txInScopeVariable*)mState.mInScopeVariables[i];
        if (!--(var->mLevel)) {
            txInstruction* instr = new txRemoveVariable(var->mName);
            NS_ENSURE_TRUE(instr, NS_ERROR_OUT_OF_MEMORY);

            rv = mState.addInstruction(instr);
            NS_ENSURE_SUCCESS(rv, rv);
            
            mState.mInScopeVariables.RemoveElementAt(i);
            delete var;
        }
    }

    txElementHandler* handler =
        (txElementHandler*)mState.popPtr();
    rv = (handler->mEndFunction)(mState);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!--mState.mElementContext->mDepth) {
        // this will delete the old object
        mState.mElementContext =
            (txElementContext*)mState.popObject();
    }

    return NS_OK;
}

nsresult
txStylesheetCompiler::characters(const nsAString& aStr)
{
    mCharacters.Append(aStr);

    return NS_OK;
}

nsresult
txStylesheetCompiler::doneLoading()
{
    // XXX Check if there are any parent or child sheets still loading and only
    // call doneCompiling if we're really done.
    // Otherwise just return NS_OK
    return mState.mStylesheet->doneCompiling();
}

txStylesheet*
txStylesheetCompiler::getStylesheet()
{
    return mState.mStylesheet;
}

nsresult
txStylesheetCompiler::flushCharacters()
{
    // Bail if we don't have any characters. The handler will detect
    // ignoreable whitespace
    if (mCharacters.IsEmpty()) {
        return NS_OK;
    }

    nsresult rv = NS_OK;

    do {
        rv = (mState.mHandlerTable->mTextHandler)(mCharacters, mState);
    } while (rv == NS_ERROR_XSLT_GET_NEW_HANDLER);

    NS_ENSURE_SUCCESS(rv, rv);

    mCharacters.Truncate();

    return NS_OK;
}

nsresult
txStylesheetCompiler::ensureNewElementContext()
{
    // Do we already have a new context?
    if (!mState.mElementContext->mDepth)
        return NS_OK;
    
    nsAutoPtr<txElementContext>
        context(new txElementContext(*mState.mElementContext));
    NS_ENSURE_TRUE(context, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = mState.pushObject(mState.mElementContext);
    NS_ENSURE_SUCCESS(rv, rv);

    mState.mElementContext.forget();
    mState.mElementContext = context;

    return NS_OK;
}


/**
 * txStylesheetCompilerState
 */


txStylesheetCompilerState::txStylesheetCompilerState(const nsAString& aBaseURI,
                                                     txStylesheet* aStylesheet)
    : mStylesheet(aStylesheet),
      mHandlerTable(nsnull),
      mSorter(nsnull),
      mDOE(nsnull),
      mNextInstrPtr(nsnull),
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

    mElementContext = new txElementContext(aBaseURI);
    if (!mElementContext || !mElementContext->mMappings) {
        // XXX invalidate
        return;
    }

    // Push the "old" txElementContext
    rv = pushObject(0);
    if (NS_FAILED(rv)) {
        // XXX invalidate
        return;
    }
}


txStylesheetCompilerState::~txStylesheetCompilerState()
{
    while (!mObjectStack.isEmpty()) {
        delete popObject();
    }
    
    PRInt32 i;
    for (i = mInScopeVariables.Count() - 1; i >= 0; --i) {
        delete (txInScopeVariable*)mInScopeVariables[i];
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
txStylesheetCompilerState::pushSorter(txPushNewContext* aSorter)
{
    nsresult rv = pushPtr(mSorter);
    NS_ENSURE_SUCCESS(rv, rv);

    mSorter = aSorter;

    return NS_OK;
}

void
txStylesheetCompilerState::popSorter()
{
    mSorter = (txPushNewContext*)popPtr();
}

nsresult
txStylesheetCompilerState::pushChooseGotoList()
{
    nsresult rv = pushObject(mChooseGotoList);
    NS_ENSURE_SUCCESS(rv, rv);

    mChooseGotoList.forget();
    mChooseGotoList = new txList;
    NS_ENSURE_TRUE(mChooseGotoList, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

void
txStylesheetCompilerState::popChooseGotoList()
{
    // this will delete the old value
    mChooseGotoList = (txList*)popObject();
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

    mNextInstrPtr = aContainer->mFirstInstruction.StartAssignment();
    return NS_OK;
}

void
txStylesheetCompilerState::closeInstructionContainer()
{
    NS_ASSERTION(mGotoTargetPointers.Count() == 0,
                 "GotoTargets still exists, did you forget to add txReturn?");
    mNextInstrPtr = 0;
}

nsresult
txStylesheetCompilerState::addInstruction(txInstruction* aInstruction)
{
    NS_PRECONDITION(mNextInstrPtr, "adding instruction outside container");

    *mNextInstrPtr = aInstruction;
    mNextInstrPtr = &aInstruction->mNext;
    
    PRInt32 i, count = mGotoTargetPointers.Count();
    for (i = 0; i < count; ++i) {
        *(txInstruction**)mGotoTargetPointers[i] = aInstruction;
    }
    mGotoTargetPointers.Clear();

    return NS_OK;
}

nsresult
txStylesheetCompilerState::addGotoTarget(txInstruction** aTargetPointer)
{
    if (!mGotoTargetPointers.AppendElement(aTargetPointer)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    return NS_OK;
}

nsresult
txStylesheetCompilerState::addVariable(const txExpandedName& aName)
{
    txInScopeVariable* var = new txInScopeVariable(aName);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);

    if (!mInScopeVariables.AppendElement(var)) {
        delete var;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

nsresult
txStylesheetCompilerState::resolveNamespacePrefix(nsIAtom* aPrefix,
                                                  PRInt32& aID)
{
    NS_ASSERTION(aPrefix && aPrefix != txXMLAtoms::_empty,
                 "caller should handle default namespace ''");
    aID = mElementContext->mMappings->lookupNamespace(aPrefix);
    return (aID != kNameSpaceID_Unknown) ? NS_OK : NS_ERROR_FAILURE;
}

nsresult
txStylesheetCompilerState::resolveFunctionCall(nsIAtom* aName, PRInt32 aID,
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

PRBool
txStylesheetCompilerState::caseInsensitiveNameTests()
{
    return PR_FALSE;
}

void
txStylesheetCompilerState::receiveError(const nsAString& aMsg, nsresult aRes)
{
    // XXX implement me
}

txElementContext::txElementContext(const nsAString& aBaseURI)
    : mPreserveWhitespace(PR_FALSE),
      mForwardsCompatibleParsing(PR_TRUE),
      mBaseURI(aBaseURI),
      mMappings(new txNamespaceMap),
      mDepth(0)
{
    mInstructionNamespaces.AppendElement(NS_INT32_TO_PTR(kNameSpaceID_XSLT));
}

txElementContext::txElementContext(const txElementContext& aOther)
    : mPreserveWhitespace(aOther.mPreserveWhitespace),
      mForwardsCompatibleParsing(aOther.mForwardsCompatibleParsing),
      mBaseURI(aOther.mBaseURI),
      mMappings(aOther.mMappings),
      mDepth(0)
{
      mInstructionNamespaces = aOther.mInstructionNamespaces;
}
