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

#include "txStylesheet.h"
#include "Expr.h"
#include "txXSLTPatterns.h"
#include "txToplevelItems.h"
#include "txInstructions.h"
#include "primitives.h"

txStylesheet::txStylesheet()
    : mRootFrame(nsnull),
      mNamedTemplates(PR_FALSE),
      mDecimalFormats(PR_TRUE),
      mAttributeSets(PR_TRUE),
      mGlobalVariables(PR_TRUE),
      mKeys(PR_TRUE),
      mContainerTemplate(nsnull),
      mCharactersTemplate(nsnull),
      mEmptyTemplate(nsnull)
{
}

nsresult
txStylesheet::init()
{
    mRootFrame = new ImportFrame;
    NS_ENSURE_TRUE(mRootFrame, NS_ERROR_OUT_OF_MEMORY);
    
    // Create default templates
    // element/root template
    txInstruction** instrp = &mContainerTemplate;
    *instrp = new txPushParams;
    NS_ENSURE_TRUE(*instrp, NS_ERROR_OUT_OF_MEMORY);

    txNodeTest* nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
    NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

    Expr* nodeExpr = new LocationStep(nt, LocationStep::CHILD_AXIS);
    NS_ENSURE_TRUE(nodeExpr, NS_ERROR_OUT_OF_MEMORY);

    instrp = &(*instrp)->mNext;
    *instrp = new txPushNewContext(nodeExpr);
    NS_ENSURE_TRUE(*instrp, NS_ERROR_OUT_OF_MEMORY);

    // XXX ToDo: need special instruction that gets the correct mode
    instrp = &(*instrp)->mNext;
    *instrp = new txApplyTemplates(txExpandedName());
    NS_ENSURE_TRUE(*instrp, NS_ERROR_OUT_OF_MEMORY);

    instrp = &(*instrp)->mNext;
    *instrp = new txPopParams;
    NS_ENSURE_TRUE(*instrp, NS_ERROR_OUT_OF_MEMORY);

    instrp = &(*instrp)->mNext;
    *instrp = new txReturn();
    NS_ENSURE_TRUE(*instrp, NS_ERROR_OUT_OF_MEMORY);

    // attribute/textnode template
    nt = new txNodeTypeTest(txNodeTypeTest::NODE_TYPE);
    NS_ENSURE_TRUE(nt, NS_ERROR_OUT_OF_MEMORY);

    nodeExpr = new LocationStep(nt, LocationStep::SELF_AXIS);
    NS_ENSURE_TRUE(nodeExpr, NS_ERROR_OUT_OF_MEMORY);

    mCharactersTemplate = new txValueOf(nodeExpr, PR_FALSE);
    NS_ENSURE_TRUE(mContainerTemplate, NS_ERROR_OUT_OF_MEMORY);

    mCharactersTemplate->mNext = new txReturn();
    NS_ENSURE_TRUE(mContainerTemplate->mNext, NS_ERROR_OUT_OF_MEMORY);

    // pi/comment/namespace template
    mEmptyTemplate = new txReturn();
    NS_ENSURE_TRUE(mEmptyTemplate, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
}

txStylesheet::~txStylesheet()
{
    // Delete all ImportFrames
    txListIterator frameIter(&mImportFrames);
    while (frameIter.hasNext()) {
        delete (ImportFrame*)frameIter.next();
    }

    txListIterator instrIter(&mTemplateInstructions);
    while (instrIter.hasNext()) {
        delete (txInstruction*)instrIter.next();
    }
    
    delete mContainerTemplate;
    delete mCharactersTemplate;
    delete mEmptyTemplate;
}

txInstruction*
txStylesheet::findTemplate(Node* aNode,
                           const txExpandedName& aMode,
                           txIMatchContext* aContext,
                           ImportFrame* aImportedBy,
                           ImportFrame** aImportFrame)
{
    NS_ASSERTION(aImportFrame, "missing ImportFrame pointer");
    NS_ASSERTION(aNode, "missing node");

    txInstruction* matchTemplate = 0;
    ImportFrame* endFrame = 0;
    txListIterator frameIter(&mImportFrames);

    if (aImportedBy) {
        ImportFrame* curr = (ImportFrame*)frameIter.next();
        while (curr != aImportedBy)
               curr = (ImportFrame*)frameIter.next();

        endFrame = aImportedBy->mFirstNotImported;
    }

#ifdef PR_LOGGING
    txPattern match = 0;
#endif

    ImportFrame* frame;
    while (!matchTemplate &&
           (frame = (ImportFrame*)frameIter.next()) &&
           frame != endFrame) {

        // get templatelist for this mode
        txList* templates;
        templates = (txList*)frame->mMatchableTemplates.get(aMode);

        if (templates) {
            txListIterator templateIter(templates);

            // Find template with highest priority
            MatchableTemplate* templ;
            while (!matchTemplate &&
                   (templ = (MatchableTemplate*)templateIter.next())) {
                if (templ->mMatch->matches(aNode, aContext)) {
                    matchTemplate = templ->mFirstInstruction;
                    *aImportFrame = frame;
#ifdef PR_LOGGING
                    match = templ->mMatch;
#endif
                }
            }
        }
    }

#ifdef PR_LOGGING
    String mode;
    if (aMode.mLocalName) {
        TX_GET_ATOM_STRING(aMode.mLocalName, mode);
    }
    if (matchTemplate) {
        String matchAttr;
        match->toString(matchAttr);
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("MatchTemplate, Pattern %s, Mode %s, Node %s\n",
                NS_LossyConvertUCS2toASCII(matchAttr).get(),
                NS_LossyConvertUCS2toASCII(mode).get(),
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get()));
    }
    else {
        PR_LOG(txLog::xslt, PR_LOG_DEBUG,
               ("No match, Node %s, Mode %s\n", 
                NS_LossyConvertUCS2toASCII(aNode->getNodeName()).get(),
                NS_LossyConvertUCS2toASCII(mode).get()));
    }
#endif

    if (!matchTemplate) {
        switch(aNode->getNodeType()) {
            case Node::ELEMENT_NODE :
            case Node::DOCUMENT_NODE :
                matchTemplate = mContainerTemplate;
                break;

            case Node::ATTRIBUTE_NODE :
            case Node::TEXT_NODE :
            case Node::CDATA_SECTION_NODE :
                matchTemplate = mCharactersTemplate;
                break;

            default:
                matchTemplate = mEmptyTemplate;
                break;
        }
    }

    return matchTemplate;
}

txDecimalFormat*
txStylesheet::getDecimalFormat(const txExpandedName& aName)
{
    return (txDecimalFormat*)mDecimalFormats.get(aName);
}

txInstruction*
txStylesheet::getAttributeSet(const txExpandedName& aName)
{
    return (txInstruction*)mAttributeSets.get(aName);
}

txInstruction*
txStylesheet::getNamedTemplate(const txExpandedName& aName)
{
    return (txInstruction*)mNamedTemplates.get(aName);
}

txOutputFormat*
txStylesheet::getOutputFormat()
{
    return &mOutputFormat;
}

nsresult
txStylesheet::getGlobalVariable(const txExpandedName& aName, Expr*& aExpr,
                                txInstruction*& aInstr)
{
    GlobalVariable* var = (GlobalVariable*)mGlobalVariables.get(aName);
    NS_ENSURE_TRUE(var, NS_ERROR_FAILURE);
    
    aExpr = var->mExpr;
    aInstr = var->mFirstInstruction;

    return NS_OK;
}

const txExpandedNameMap&
txStylesheet::getKeyMap()
{
    return mKeys;
}


nsresult
txStylesheet::doneCompiling()
{
    nsresult rv = NS_OK;
    // Collect all importframes into a single ordered list
    rv = mImportFrames.add(mRootFrame);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mRootFrame = nsnull;
    
    // XXX ToDo: traverse tree of importframes and fill mImportFrames

    // Loop through importframes in decreasing-precedence-order and process
    // all items
    txListIterator frameIter(&mImportFrames);
    ImportFrame* frame;
    while ((frame = (ImportFrame*)frameIter.next())) {
        txListIterator itemIter(&frame->mToplevelItems);
        itemIter.resetToEnd();
        txToplevelItem* item;
        while ((item = (txToplevelItem*)itemIter.previous())) {
            switch (item->getType()) {
                case txToplevelItem::dummy:
                case txToplevelItem::import:
                {
                    delete item;
                    break;
                }
                case txToplevelItem::output:
                {
                    mOutputFormat.merge(((txOutputItem*)item)->mFormat);
                    delete item;
                    break;
                }
                case txToplevelItem::templ:
                {
                    rv = addTemplate((txTemplateItem*)item, frame);
                    NS_ENSURE_SUCCESS(rv, rv);

                    delete item;
                    break;
                }
                case txToplevelItem::variable:
                {
                    rv = addGlobalVariable((txVariableItem*)item);
                    NS_ENSURE_SUCCESS(rv, rv);

                    delete item;
                    break;
                }
            }
            itemIter.remove(); //remove() moves to the previous
            itemIter.next();
        }
    }

    return NS_OK;
}

// XXX This one isn't OOM safe.
nsresult
txStylesheet::addTemplate(txTemplateItem* aTemplate,
                          ImportFrame* aImportFrame)
{
    NS_ASSERTION(aTemplate, "missing template");

    txInstruction* instr = aTemplate->mFirstInstruction;
    nsresult rv = mTemplateInstructions.add(instr);
    NS_ENSURE_SUCCESS(rv, rv);

    // mTemplateInstructions now owns the instructions
    aTemplate->mFirstInstruction = 0;

    rv = NS_OK;
    if (!aTemplate->mName.isNull()) {
        rv = mNamedTemplates.add(aTemplate->mName, instr);
        NS_ENSURE_TRUE(NS_SUCCEEDED(rv) || rv == NS_ERROR_XSLT_ALREADY_SET,
                       rv);
    }

    if (!aTemplate->mMatch) {
        // This is no error, see section 6 Named Templates

        return NS_OK;
    }

    // get the txList for the right mode
    txList* templates =
        (txList*)aImportFrame->mMatchableTemplates.get(aTemplate->mMode);

    if (!templates) {
        templates = new txList;
        if (!templates) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        rv = aImportFrame->mMatchableTemplates.add(aTemplate->mMode,
                                                   templates);
        if (NS_FAILED(rv)) {
            delete templates;
            return rv;
        }
    }

    // Add the simple patterns to the list of matchable templates, according
    // to default priority
    txList simpleMatches;
    aTemplate->mMatch->getSimplePatterns(simpleMatches);
    txListIterator simples(&simpleMatches);
    while (simples.hasNext()) {
        txPattern* simple = (txPattern*)simples.next();
        if (simple != aTemplate->mMatch && aTemplate->mMatch) {
            // txUnionPattern, it doesn't own the txLocPathPatterns no more,
            // so delete it. (only once, of course)
            delete aTemplate->mMatch;
            aTemplate->mMatch = 0;
        }
        double priority = aTemplate->mPrio;
        if (Double::isNaN(priority)) {
            priority = simple->getDefaultPriority();
            NS_ASSERTION(!Double::isNaN(priority),
                         "simple pattern without default priority");
        }
        MatchableTemplate* nt = new MatchableTemplate(instr, simple, priority);

        txListIterator templ(templates);
        MBool added = MB_FALSE;
        while (templ.hasNext()) {
            MatchableTemplate* mt = (MatchableTemplate*)templ.next();
            if (priority > mt->mPriority) {
                templ.addBefore(nt);
                added = MB_TRUE;
                break;
            }
        }
        if (!added) {
            templates->add(nt);
        }
    }
    aTemplate->mMatch = 0;

    return NS_OK;
}

nsresult
txStylesheet::addGlobalVariable(txVariableItem* aVariable)
{
    if (mGlobalVariables.get(aVariable->mName)) {
        return NS_OK;
    }
    GlobalVariable* var = new GlobalVariable(aVariable->mValue,
                                             aVariable->mFirstInstruction);
    NS_ENSURE_TRUE(var, NS_ERROR_OUT_OF_MEMORY);
    
    aVariable->mValue = nsnull;
    aVariable->mFirstInstruction = nsnull;
    
    nsresult rv = mGlobalVariables.add(aVariable->mName, var);
    if (NS_FAILED(rv)) {
        delete var;
        return rv;
    }
    
    return NS_OK;
    
}

nsresult
txStylesheet::addKey(const txExpandedName& aName,
                     txPattern* aMatch, Expr* aUse)
{
    nsresult rv = NS_OK;

    txXSLKey* xslKey = (txXSLKey*)mKeys.get(aName);
    if (!xslKey) {
        xslKey = new txXSLKey(aName);
        NS_ENSURE_TRUE(xslKey, NS_ERROR_OUT_OF_MEMORY);

        rv = mKeys.add(aName, xslKey);
        if (NS_FAILED(rv)) {
            delete xslKey;
            return rv;
        }
    }
    if (!xslKey->addKey(aMatch, aUse)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
}

txStylesheet::ImportFrame::ImportFrame()
    : mMatchableTemplates(MB_TRUE),
      mFirstNotImported(nsnull)
{
}

txStylesheet::ImportFrame::~ImportFrame()
{
    // Delete templates in mMatchableTemplates
    txExpandedNameMap::iterator mapIter(mMatchableTemplates);
    while (mapIter.next()) {
        txListIterator templIter((txList*)mapIter.value());
        MatchableTemplate* templ;
        while ((templ = (MatchableTemplate*)templIter.next())) {
            delete templ->mMatch;
            delete templ;
        }
    }
    
    txListIterator tlIter(&mToplevelItems);
    while (tlIter.hasNext()) {
        delete (txToplevelItem*)tlIter.next();
    }
}

txStylesheet::GlobalVariable::GlobalVariable(Expr* aExpr, txInstruction* aFirstInstruction)
    : mExpr(aExpr), mFirstInstruction(aFirstInstruction)
{
}

txStylesheet::GlobalVariable::~GlobalVariable()
{
    delete mExpr;
    delete mFirstInstruction;
}
